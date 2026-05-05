"""
cmd_helpers.py — shared timeout / ssh / rsync constants for testing*.py.

Single source of truth for two policy decisions that every testing*.py
must agree on:

  1. ssh / rsync MUST set ConnectTimeout=5 so a dead remote (DNS gone,
     TCP half-open, host unreachable) doesn't sit in connect() for the
     kernel default ~2 minutes before failing. Five seconds is plenty
     for any in-room IPv6/v4 hop.

  2. Every subprocess.run / ssh call MUST be bounded:
       * SSH command:    ≤ SSH_MAX_TIMEOUT   (1200s) — covers slow
         remote builds (qemu-mips full rebuild) but stops a runaway.
       * Local command:  ≤ LOCAL_MAX_TIMEOUT (600s)  — local boxes are
         fast; a 10-minute local cmake build is the longest plausible
         step and anything over that is hung.

Plus the reusable bits every script needs:
  SSH_OPTS_LIST  — list[str] for argv-style ssh invocations
  RSYNC_SSH_E    — str for rsync's -e argument (caller appends -p N)
  SSH_TIMEOUT_MARKER + is_ssh_timeout(out)
                 — let callers tell a TIMEOUT-on-stdout apart from a
                   genuine rc!=0 so log_fail messages can say so.
  clamp_ssh / clamp_local
                 — silently clamp a caller-supplied timeout to the
                   policy bound, so a stray request for 3600s gets
                   trimmed to 1200s without anyone editing every
                   call site.
"""

SSH_CONNECT_TIMEOUT = 5
SSH_MAX_TIMEOUT = 1200
LOCAL_MAX_TIMEOUT = 600

SSH_OPTS_LIST = [
    "-o", "StrictHostKeyChecking=no",
    "-o", "BatchMode=yes",
    "-o", "ConnectTimeout=" + str(SSH_CONNECT_TIMEOUT),
]

RSYNC_SSH_E = ("ssh -o StrictHostKeyChecking=no -o BatchMode=yes "
               "-o ConnectTimeout=" + str(SSH_CONNECT_TIMEOUT))

SSH_TIMEOUT_MARKER = "__SSH_TIMEOUT__"


def is_ssh_timeout(out):
    """True iff `out` is the marker string emitted by an ssh wrapper
    that hit subprocess.TimeoutExpired. Callers branch on this to
    surface 'TIMEOUT' in log_fail() instead of a generic rc=-1."""
    return isinstance(out, str) and out.startswith(SSH_TIMEOUT_MARKER)


def clamp_ssh(timeout):
    """Cap a caller-supplied ssh timeout at SSH_MAX_TIMEOUT (1200s).
    None / 0 → SSH_MAX_TIMEOUT. Negative / non-numeric → fail loud
    via TypeError so the broken caller surfaces in tests."""
    if timeout is None or timeout == 0:
        return SSH_MAX_TIMEOUT
    t = int(timeout)
    if t > SSH_MAX_TIMEOUT:
        return SSH_MAX_TIMEOUT
    return t


def clamp_local(timeout):
    """Cap a caller-supplied local subprocess timeout at LOCAL_MAX_TIMEOUT
    (600s). Same None / 0 / negative semantics as clamp_ssh."""
    if timeout is None or timeout == 0:
        return LOCAL_MAX_TIMEOUT
    t = int(timeout)
    if t > LOCAL_MAX_TIMEOUT:
        return LOCAL_MAX_TIMEOUT
    return t


# ── TCP port reachability probe ──────────────────────────────────────────────

TCP_PROBE_TIMEOUT = 5


def wait_tcp_port_open(host, port, timeout=TCP_PROBE_TIMEOUT):
    """Try to TCP-connect to (host, port) within `timeout` seconds.
    Returns (True, "") on success or (False, detail) on failure.

    Used before every client launch in testing*.py: the server reports
    "correctly bind: detected" but binding only proves it ran listen()
    successfully — it doesn't tell us whether something between client
    and server (firewall, routing, IPv6 vs IPv4 bind, container netns,
    sandbox, etc.) blocks the actual TCP handshake. Probing here
    surfaces those issues with an explicit message instead of letting
    the client time out 60-120s later with a generic "no map shown".

    Resolves both v4 and v6 (the server binds dual-stack on IN6ADDR_ANY
    by default, but a misconfigured node could be v6-only); we accept
    the first family that connects."""
    import socket
    import time
    deadline = time.monotonic() + max(0.1, float(timeout))
    last_err = "no addresses resolved"
    try:
        infos = socket.getaddrinfo(host, int(port), 0, socket.SOCK_STREAM)
    except (socket.gaierror, OSError) as e:
        return False, f"DNS lookup failed: {e}"
    while time.monotonic() < deadline:
        ai = 0
        while ai < len(infos):
            family, socktype, proto, _, sockaddr = infos[ai]
            ai += 1
            remaining = deadline - time.monotonic()
            if remaining <= 0:
                break
            s = socket.socket(family, socktype, proto)
            try:
                s.settimeout(min(1.0, remaining))
                s.connect(sockaddr)
                s.close()
                return True, ""
            except (socket.timeout, OSError) as e:
                last_err = f"{family.name if hasattr(family, 'name') else family} {sockaddr}: {e}"
            finally:
                try:
                    s.close()
                except OSError:
                    pass
        # brief pause before re-trying so a server that hasn't quite
        # finished epoll setup gets a couple of retries
        time.sleep(0.2)
    return False, last_err


def _local_outbound_ip(target_host=None, target_port=None):
    """Discover which local IP the kernel would route a packet to
    (target_host, target_port) through. Used for the FROM=<ip> field in
    TCP-probe FAIL messages so the operator sees the EXACT pair of
    endpoints involved.

    The routing decision matters: a probe TO=127.0.0.1 / ::1 / localhost
    goes through the loopback interface, so the FROM is also a loopback
    address — printing the host's outbound 192.x.x.x there would be
    misleading. We do this with a UDP "connect" (no packet actually
    leaves the box, the kernel just resolves the route) and read the
    local address it would have used.

    target_host=None → fall back to the legacy "outbound IP" (UDP
    connect to 8.8.8.8) for callers that don't know their target yet.
    Resolution failures fall back to "127.0.0.1" so the message stays
    readable in container netns / no-route environments."""
    import socket
    if target_host is None:
        probe_addr = ("8.8.8.8", 80)
        family = socket.AF_INET
    else:
        try:
            infos = socket.getaddrinfo(target_host,
                                       int(target_port) if target_port else 0,
                                       0, socket.SOCK_DGRAM)
        except (socket.gaierror, OSError):
            return "127.0.0.1"
        if not infos:
            return "127.0.0.1"
        family, _, _, _, probe_addr = infos[0]
    try:
        s = socket.socket(family, socket.SOCK_DGRAM)
        try:
            s.connect(probe_addr)
            ip = s.getsockname()[0]
        finally:
            s.close()
        return ip
    except OSError:
        return "127.0.0.1"


def assert_port_or_fail(host, port, log_fail_fn, label,
                       timeout=TCP_PROBE_TIMEOUT):
    """Probe (host, port) within `timeout`; on failure, call log_fail_fn
    with a detail message that points the operator at firewall / routing
    since the server already announced "correctly bind: detected", and
    names the exact FROM and TO endpoints that were tested so the
    failure can be diagnosed without re-running.
    Returns True if the port is reachable, False (after log_fail) otherwise.

    The server is expected to bind on a deterministic, fixed port
    (config.json's `server_port`, 61917 by default); the probe key is
    therefore (host, port) — same on every run, no dynamic ephemerals
    that would defeat the FROM/TO message."""
    ok, detail = wait_tcp_port_open(host, port, timeout=timeout)
    if ok:
        return True
    # Pass (host, port) so FROM reflects the route to *this specific
    # destination*: a probe to localhost:N has FROM=127.0.0.1 (loopback),
    # a probe to a LAN IP has FROM=<the host's LAN-side address>. Without
    # this the FROM would always be the global outbound IP, which is
    # confusing when TO is loopback.
    from_ip = _local_outbound_ip(host, port)
    log_fail_fn(label,
                f"TCP probe FAIL  FROM={from_ip} (local)  "
                f"TO={host}:{port}  not reachable in {timeout}s "
                f"despite 'correctly bind: detected' — "
                f"check firewall / routing / netns / IPv6-vs-IPv4 bind "
                f"({detail})")
    return False


_LOOPBACK_HOSTS = ("localhost", "127.0.0.1", "::1")


def resolve_target_for_remote(exec_node, target_host):
    """When the test driver passes target_host="localhost" (or 127.0.0.1
    / ::1), it means "the box running this script". A remote probe from
    `exec_node` to "localhost" would hit the EXEC NODE's loopback —
    never the server. Translate that here to the test box's IP that
    routes toward the exec_node, so the actual SSH probe targets the
    correct address.

    Returns (translated_target, test_box_ip) — the second value is what
    callers print as FROM in the FAIL message because the test driver
    "owns" the server endpoint."""
    if target_host not in _LOOPBACK_HOSTS:
        return target_host, target_host
    # _local_outbound_ip(exec_node_host) → the local IP the kernel uses
    # to reach the exec node, which is exactly the address the exec node
    # would dial back to.
    test_box_ip = _local_outbound_ip(exec_node.get("host"))
    return test_box_ip, test_box_ip


def wait_tcp_port_open_remote(exec_node, target_host, target_port,
                              timeout=TCP_PROBE_TIMEOUT):
    """SSH into `exec_node` and probe TCP (target_host, target_port) from
    there. Used to detect routing / firewall asymmetry: a server that's
    reachable locally but blocked from a real-hardware exec_node would
    pass a local-only probe and silently break the cross-machine run.

    Returns (True, "") on success, (False, detail) otherwise.

    `target_host` is auto-translated when it's a loopback alias —
    "localhost"/"127.0.0.1"/"::1" gets rewritten to the test driver's
    IP reachable from the exec_node, otherwise the remote probe would
    hit the exec_node's own loopback. Callers should use
    resolve_target_for_remote() to discover the address they should
    print as FROM/TO in their failure message.

    The remote probe uses bash's built-in /dev/tcp redirection so we
    don't need nc / netcat / python3 on the exec node — bash itself is
    enough. The timeout(1) wrapper is the outer bound; bash's connect
    syscall has no portable timeout option."""
    import subprocess
    actual_target, _ = resolve_target_for_remote(exec_node, target_host)
    user = exec_node["user"]
    host = exec_node["host"]
    port = int(exec_node.get("port", 22))
    cmd = (f"timeout {int(timeout)} bash -c "
           f"'exec 3<>/dev/tcp/{actual_target}/{int(target_port)} "
           f"&& exec 3>&-' 2>&1")
    ssh_args = ["ssh"] + SSH_OPTS_LIST + ["-p", str(port),
                                           f"{user}@{host}", cmd]
    # ssh wrapper (timeout includes the SSH connect itself; ConnectTimeout=5
    # already caps the SSH side)
    try:
        p = subprocess.run(ssh_args, timeout=timeout + 5,
                           stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    except subprocess.TimeoutExpired:
        return False, f"ssh probe TIMEOUT after {timeout + 5}s"
    if p.returncode == 0:
        return True, ""
    out = p.stdout.decode(errors="replace").strip()
    return False, (f"rc={p.returncode}: " + (out or "no output"))


def assert_port_or_fail_with_remotes(target_host, target_port,
                                     exec_nodes, log_fail_fn, label,
                                     timeout=TCP_PROBE_TIMEOUT):
    """Probe (target_host, target_port) locally AND from every enabled
    entry in `exec_nodes` (list of dicts from remote_nodes.json's
    `execution_nodes` field). FAILs the test on the first unreachable
    path with a precise FROM=<ip> / TO=<host:port> message so the
    operator knows exactly which leg of the network is blocked.

    Skips entries with `enabled: false` — intentionally off, probing
    them would just generate noise. When `exec_nodes` is empty / None,
    behaves identically to assert_port_or_fail.

    Server-side contract: the server binds on a deterministic, fixed
    port (config.json's `server_port`, 61917 by default). The probe
    key (target_host, target_port) is therefore stable across every
    run, so the FROM/TO output is reproducible.

    Failure budget: ~5s local + 5s + 5s ssh-connect-cap per exec node;
    much better than letting a cross-machine client time out 60-120s
    later with a generic 'no map shown'."""
    # 1. Local probe (the host running the test driver).
    if not assert_port_or_fail(target_host, target_port,
                               log_fail_fn, label, timeout=timeout):
        return False
    # 2. Per-exec-node probe.
    if not exec_nodes:
        return True
    ei = 0
    while ei < len(exec_nodes):
        node = exec_nodes[ei]
        ei += 1
        if not bool(node.get("enabled", True)):
            continue
        node_label = node.get("label", node.get("host", "?"))
        from_ip = node.get("host", "?")
        # Resolve the target the same way wait_tcp_port_open_remote
        # does internally, so the FAIL message reports the exact
        # address the bash-on-exec-node actually attempted: a
        # "localhost"/"127.0.0.1" target gets rewritten to the test
        # driver's IP that routes toward the exec node, otherwise the
        # remote bash hits its own loopback (no server) and the
        # FROM/TO line would look meaningless ("TO=localhost" doesn't
        # tell the operator which IP was actually contacted).
        resolved_target, _ = resolve_target_for_remote(node, target_host)
        ok, detail = wait_tcp_port_open_remote(
            node, target_host, target_port, timeout=timeout)
        if not ok:
            log_fail_fn(label,
                        f"TCP probe FAIL  FROM={from_ip} ({node_label})  "
                        f"TO={resolved_target}:{target_port}  not reachable "
                        f"in {timeout}s despite 'correctly bind: detected' "
                        f"— check firewall / routing between {node_label} "
                        f"and {resolved_target} ({detail})")
            return False
    return True
