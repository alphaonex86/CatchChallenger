"""Closed connections must return their slots before character selection."""

import socket
import struct
import sys
import time
import re
from contextlib import ExitStack
from pathlib import Path
import xml.etree.ElementTree as ET

sys.dont_write_bytecode = True
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import _protoharness as H

NAME = "01 connection slots reused before login"


def _receive(sock, size):
    data = bytearray()
    while len(data) < size:
        chunk = sock.recv(size - len(data))
        if not chunk:
            raise AssertionError("connection closed before the protocol reply")
        data.extend(chunk)
    return bytes(data)


def _hello(sock):
    sock.sendall(b"\xa0\x01" + H.PROTOCOL_HEADER_LOGIN)
    header = _receive(sock, 6)
    size = struct.unpack("<I", header[2:])[0]
    if header[:2] != b"\x7f\x01" or not 17 <= size <= 1024:
        raise AssertionError("invalid protocol reply: " + header.hex())
    payload = _receive(sock, size)
    if payload[0] != 0x04:
        raise AssertionError("protocol negotiation failed")
    return payload[1:17]


def _close_and_wait(sock):
    # EOF acknowledges that the server processed this disconnect: no sleeps.
    sock.shutdown(socket.SHUT_WR)
    while sock.recv(4096):
        pass


def _fill_and_drain(server, limit):
    with ExitStack() as stack:
        clients = [stack.enter_context(socket.create_connection(
            ("127.0.0.1", server.port), timeout=5)) for _ in range(limit)]
        with socket.create_connection(("127.0.0.1", server.port), timeout=5) as extra:
            if extra.recv(1):
                raise AssertionError("unexpected reply to excess connection")
        for sock in clients:
            _hello(sock)
            _close_and_wait(sock)
    return limit


def run(server):
    limit = int(ET.parse(Path(server.run_dir) / "server-properties.xml")
                .getroot().find("max-players").get("value"))
    completed = 0
    phase = "connect"
    started = time.monotonic()
    try:
        # Keep one slot occupied; a double release must not alias this client.
        with socket.create_connection(("127.0.0.1", server.port), timeout=5) as keeper:
            for phase in ("empty", "protocol", "partial-header"):
                for _ in range(limit + 1):
                    with socket.create_connection(("127.0.0.1", server.port), timeout=5) as sock:
                        if phase == "protocol":
                            _hello(sock)
                        elif phase == "partial-header":
                            sock.sendall(b"\xa0\x01\x9c")
                        _close_and_wait(sock)
                    completed += 1
            _hello(keeper)
            _close_and_wait(keeper)
        phase = "capacity"
        completed += _fill_and_drain(server, limit)
        with socket.create_connection(("127.0.0.1", server.port), timeout=5) as sock:
            _hello(sock)
            _close_and_wait(sock)
        phase = "pending-token eviction"
        login, password = H._creds(b"slot-regression", b"slot-password")
        with socket.create_connection(("127.0.0.1", server.port), timeout=5) as sock:
            phase = "account-creation handshake"
            _hello(sock)
            phase = "account-creation reply"
            sock.sendall(b"\xa9\x02" + H.blake3(login) + password)
            if _receive(sock, 3) != b"\x7f\x02\x01":
                raise AssertionError("account creation failed")
            phase = "account-creation disconnect"
            _close_and_wait(sock)
        definitions = (Path(__file__).resolve().parents[2] /
                       "server/base/VariableServer.hpp").read_text()
        token_limit = int(re.search(
            r"#define CATCHCHALLENGER_SERVER_MAXNOTLOGGEDCONNECTION (\d+)",
            definitions).group(1))
        with ExitStack() as stack:
            clients = [stack.enter_context(socket.create_connection(
                ("127.0.0.1", server.port), timeout=5)) for _ in range(token_limit + 1)]
            # A reply from the last socket proves the whole batch was accepted.
            phase = "pending-token acceptance fence"
            _hello(clients[-1])
            tokens = []
            for index, sock in enumerate(clients[:-1]):
                phase = "pending-token handshake %d" % index
                tokens.append(_hello(sock))
            phase = "pending-token eviction EOF"
            if clients[-1].recv(1):
                raise AssertionError("oldest pending connection was not closed")
            phase = "login with retained token"
            connection = H._RawConn(clients[0])
            reply = connection.query(0xA8, login + H.blake3(password + tokens[0]),
                                     dynamic=False)
            if not reply or reply[0] != 0x01:
                raise AssertionError("eviction removed another client's auth token")
            for sock in clients[:-1]:
                phase = "authenticated/pending disconnect"
                _close_and_wait(sock)
                completed += 1
        phase = "capacity after token eviction"
        completed += _fill_and_drain(server, limit)
        if server.proc.poll() is not None:
            raise AssertionError("server exited: " + str(server.proc.returncode))
    except (OSError, AssertionError) as exc:
        return False, "%s after %d disconnects: %s" % (phase, completed, exc)
    elapsed = time.monotonic() - started
    return True, "%d disconnects, retained client and fresh handshake OK (%.3fs)" % (completed, elapsed)


if __name__ == "__main__":
    import argparse
    import tempfile

    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--binary", required=True)
    parser.add_argument("--valgrind", action="store_true")
    args = parser.parse_args()
    work = Path(tempfile.mkdtemp(prefix="cc-connection-slots-"))
    server = H.Server(str(Path(args.binary).resolve()), str(work / "run"),
                      valgrind=args.valgrind)
    try:
        ok, detail = run(server)
    finally:
        server.stop()
    print(("[PASS] " if ok else "[FAIL] ") + detail, flush=True)
    print("Artifacts: " + str(work), flush=True)
    raise SystemExit(0 if ok else 1)
