#!/usr/bin/env python3
"""CatchChallenger Process Manager for Claude Code integration.

Uses file-based state (PID files, log files) so each invocation is stateless
but can control persistent background processes.

Commands:
    kill              Kill all catchchallenger-server-cli-epoll processes
    build-server      Build server with make -j32
    build-bot         Build bot with make clean && make -j32
    save-cache        Run server with 'save' arg to create datapack-cache.bin
    start-server      Start server in background (loads from cache)
    wait-bind [T]     Wait for "correctly bind:" in server output (default 60s)
    start-bot         Start bot in background
    wait-bot [T]      Wait for bot to finish (exit 0 = on map, default 60s)
    run-bot [T]       Start bot + wait (combined, default 60s)
    compare           Compare server and bot datapack directories
    get-server-output Get server output buffer
    get-bot-output    Get bot output buffer
    clear-output      Clear all output buffers
    status            Show process status
    full-test         Run complete test sequence
    testcase          Run full test, write server.log and bot-tools.log, and quit
    stop-server       Stop server gracefully (SIGTERM then SIGKILL)
"""

import subprocess
import sys
import os
import time
import signal
import shutil
import filecmp

SERVER_DIR = "/home/user/Desktop/CatchChallenger/working/server/epoll/build/llvm-Debug"
BOT_DIR = "/home/user/Desktop/CatchChallenger/working/tools/bot-test-connect-to-gameserver/build/llvm-Debug"
SERVER_BIN = "catchchallenger-server-cli-epoll"
BOT_BIN = "bot-test-connect-to-gameserver"

# State files in /tmp for persistence across invocations
STATE_DIR = "/tmp/cc-process-manager"
SERVER_PID_FILE = os.path.join(STATE_DIR, "server.pid")
SERVER_LOG_FILE = os.path.join(STATE_DIR, "server.log")
BOT_PID_FILE = os.path.join(STATE_DIR, "bot.pid")
BOT_LOG_FILE = os.path.join(STATE_DIR, "bot.log")

ALLOWED_COMMANDS = {
    "kill", "build-server", "build-bot", "save-cache",
    "start-server", "wait-bind", "start-bot", "wait-bot", "run-bot",
    "compare", "get-server-output", "get-bot-output", "clear-output",
    "status", "full-test", "testcase", "stop-server",
}

# Output files written by `testcase` for Claude Code to read
TESTCASE_SERVER_LOG = "/home/user/Desktop/CatchChallenger/working/server/epoll/server.log"
TESTCASE_BOT_LOG = "/home/user/Desktop/CatchChallenger/working/server/epoll/bot-tools.log"


def ensure_state_dir():
    os.makedirs(STATE_DIR, exist_ok=True)


def read_pid(pid_file):
    """Read PID from file, return None if not running."""
    try:
        with open(pid_file) as f:
            pid = int(f.read().strip())
        os.kill(pid, 0)  # Check if alive
        return pid
    except (FileNotFoundError, ValueError, ProcessLookupError, PermissionError):
        return None


def write_pid(pid_file, pid):
    with open(pid_file, "w") as f:
        f.write(str(pid))


def remove_pid(pid_file):
    try:
        os.unlink(pid_file)
    except FileNotFoundError:
        pass


def kill_pid(pid, timeout=5):
    """Kill a process by PID, SIGTERM then SIGKILL."""
    try:
        os.kill(pid, signal.SIGTERM)
    except ProcessLookupError:
        return True
    deadline = time.time() + timeout
    while time.time() < deadline:
        try:
            os.kill(pid, 0)
            time.sleep(0.2)
        except ProcessLookupError:
            return True
    try:
        os.kill(pid, signal.SIGKILL)
        time.sleep(0.5)
    except ProcessLookupError:
        pass
    return True


def tail_log(log_file, max_bytes=10000):
    """Read last max_bytes of a log file."""
    try:
        size = os.path.getsize(log_file)
        with open(log_file, "r", errors="replace") as f:
            if size > max_bytes:
                f.seek(size - max_bytes)
                f.readline()  # Skip partial line
            return f.read()
    except FileNotFoundError:
        return ""


def log_contains(log_file, needle, after_byte=0):
    """Check if log file contains needle after given byte offset."""
    try:
        with open(log_file, "r", errors="replace") as f:
            if after_byte > 0:
                f.seek(after_byte)
            for line in f:
                if needle in line:
                    return True
        return False
    except FileNotFoundError:
        return False


def log_size(log_file):
    try:
        return os.path.getsize(log_file)
    except FileNotFoundError:
        return 0


# ---- Commands ----

def cmd_kill():
    """Kill all catchchallenger-server-cli-epoll processes."""
    ensure_state_dir()
    # Kill tracked server
    pid = read_pid(SERVER_PID_FILE)
    if pid:
        kill_pid(pid)
        remove_pid(SERVER_PID_FILE)
    # Kill tracked bot
    pid = read_pid(BOT_PID_FILE)
    if pid:
        kill_pid(pid)
        remove_pid(BOT_PID_FILE)
    # Kill any remaining system-wide
    subprocess.run(["killall", SERVER_BIN], capture_output=True, timeout=10)
    time.sleep(0.3)
    subprocess.run(["killall", "-9", SERVER_BIN], capture_output=True, timeout=5)
    time.sleep(0.3)
    print("OK: All server processes killed")
    return 0


def cmd_build_server():
    """Build server with make -j32."""
    print("Building server...")
    r = subprocess.run(["make", "-j32"], cwd=SERVER_DIR, capture_output=True, text=True, timeout=300)
    if r.returncode != 0:
        output = r.stderr if r.stderr else r.stdout
        print(f"FAIL: Server build failed:\n{output[-3000:]}")
        return 1
    binary = os.path.join(SERVER_DIR, SERVER_BIN)
    if not os.path.isfile(binary):
        print("FAIL: Server binary not found after build")
        return 1
    print(f"OK: Server built ({os.path.getsize(binary)} bytes)")
    return 0


def cmd_build_bot():
    """Build bot with make clean && make -j32."""
    print("Building bot (clean + build)...")
    subprocess.run(["make", "clean"], cwd=BOT_DIR, capture_output=True, timeout=60)
    r = subprocess.run(["make", "-j32"], cwd=BOT_DIR, capture_output=True, text=True, timeout=300)
    if r.returncode != 0:
        output = r.stderr if r.stderr else r.stdout
        print(f"FAIL: Bot build failed:\n{output[-3000:]}")
        return 1
    binary = os.path.join(BOT_DIR, BOT_BIN)
    if not os.path.isfile(binary):
        print("FAIL: Bot binary not found after build")
        return 1
    print(f"OK: Bot built ({os.path.getsize(binary)} bytes)")
    return 0


def cmd_save_cache():
    """Run server with 'save' argument to save datapack cache, then rename .tmp to final."""
    ensure_state_dir()
    print("Saving datapack cache...")

    cache_tmp = os.path.join(SERVER_DIR, "datapack-cache.bin.tmp")
    cache_final = os.path.join(SERVER_DIR, "datapack-cache.bin")

    # Remove old tmp if present
    if os.path.exists(cache_tmp):
        os.unlink(cache_tmp)

    binary = os.path.join(SERVER_DIR, SERVER_BIN)
    if not os.path.isfile(binary):
        print("FAIL: Server binary not found. Build first.")
        return 1

    with open(SERVER_LOG_FILE, "w") as log:
        r = subprocess.run(
            [binary, "save"],
            cwd=SERVER_DIR, stdout=log, stderr=subprocess.STDOUT,
            timeout=120
        )

    output = tail_log(SERVER_LOG_FILE, 5000)
    print(output)

    if r.returncode != 0:
        print(f"FAIL: Cache save exited with code {r.returncode}")
        return 1

    # The server writes datapack-cache.bin.tmp, rename to final
    if os.path.isfile(cache_tmp):
        shutil.move(cache_tmp, cache_final)
        size = os.path.getsize(cache_final)
        print(f"OK: Datapack cache saved ({size} bytes)")
        return 0
    elif os.path.isfile(cache_final):
        size = os.path.getsize(cache_final)
        print(f"OK: Datapack cache exists ({size} bytes)")
        return 0
    else:
        print("FAIL: datapack-cache.bin was not created")
        return 1


def cmd_start_server():
    """Start server in background (loads from cache)."""
    ensure_state_dir()

    # Kill any existing
    pid = read_pid(SERVER_PID_FILE)
    if pid:
        kill_pid(pid)
        remove_pid(SERVER_PID_FILE)
    subprocess.run(["killall", SERVER_BIN], capture_output=True, timeout=10)
    time.sleep(0.3)

    binary = os.path.join(SERVER_DIR, SERVER_BIN)
    if not os.path.isfile(binary):
        print("FAIL: Server binary not found. Build first.")
        return 1

    # Clear log
    with open(SERVER_LOG_FILE, "w") as f:
        pass

    log_fd = open(SERVER_LOG_FILE, "a")
    proc = subprocess.Popen(
        [binary],
        cwd=SERVER_DIR,
        stdout=log_fd, stderr=subprocess.STDOUT,
        start_new_session=True
    )
    write_pid(SERVER_PID_FILE, proc.pid)
    log_fd.close()  # The child inherited the fd, we can close our copy

    print(f"OK: Server started (PID {proc.pid})")
    return 0


def cmd_wait_bind(timeout=60):
    """Wait for server to output 'correctly bind:'."""
    ensure_state_dir()
    pid = read_pid(SERVER_PID_FILE)
    if not pid:
        print("FAIL: Server not running")
        return 1

    print(f"Waiting for server bind (timeout {timeout}s)...")
    start = time.time()
    while time.time() - start < timeout:
        # Check server still alive
        try:
            os.kill(pid, 0)
        except ProcessLookupError:
            output = tail_log(SERVER_LOG_FILE, 5000)
            print(f"FAIL: Server died during startup\n{output}")
            remove_pid(SERVER_PID_FILE)
            return 1

        if log_contains(SERVER_LOG_FILE, "correctly bind:"):
            elapsed = time.time() - start
            print(f"OK: Server bound after {elapsed:.1f}s")
            return 0
        time.sleep(0.3)

    output = tail_log(SERVER_LOG_FILE, 5000)
    print(f"FAIL: Timeout ({timeout}s) waiting for 'correctly bind:'\n{output}")
    return 1


def cmd_start_bot():
    """Start bot in background."""
    ensure_state_dir()

    # Kill any existing bot
    pid = read_pid(BOT_PID_FILE)
    if pid:
        kill_pid(pid)
        remove_pid(BOT_PID_FILE)

    binary = os.path.join(BOT_DIR, BOT_BIN)
    if not os.path.isfile(binary):
        print("FAIL: Bot binary not found. Build first.")
        return 1

    # Clear log
    with open(BOT_LOG_FILE, "w") as f:
        pass

    log_fd = open(BOT_LOG_FILE, "a")
    proc = subprocess.Popen(
        [binary],
        cwd=BOT_DIR,
        stdout=log_fd, stderr=subprocess.STDOUT,
        start_new_session=True
    )
    write_pid(BOT_PID_FILE, proc.pid)
    log_fd.close()

    print(f"OK: Bot started (PID {proc.pid})")
    return 0


def cmd_wait_bot(timeout=60):
    """Wait for bot to finish. Exit 0 means reached map."""
    ensure_state_dir()
    pid = read_pid(BOT_PID_FILE)
    if not pid:
        print("FAIL: Bot not running")
        return 1

    print(f"Waiting for bot to finish (timeout {timeout}s)...")
    start = time.time()
    while time.time() - start < timeout:
        try:
            os.kill(pid, 0)
        except ProcessLookupError:
            # Process exited - get return code via waitpid
            try:
                _, status = os.waitpid(pid, os.WNOHANG)
                rc = os.WEXITSTATUS(status) if os.WIFEXITED(status) else -1
            except ChildProcessError:
                # Not our child, check log for success indicators
                output = tail_log(BOT_LOG_FILE, 5000)
                if "datapackMainSubIsReady" in output:
                    rc = 0
                else:
                    rc = 1
            remove_pid(BOT_PID_FILE)
            output = tail_log(BOT_LOG_FILE, 5000)
            print(output)
            if rc == 0:
                print("OK: Bot connected and reached map")
                return 0
            else:
                print(f"FAIL: Bot exited with code {rc}")
                return rc if rc != 0 else 1
        time.sleep(0.5)

    # Timeout - kill bot
    kill_pid(pid, timeout=3)
    remove_pid(BOT_PID_FILE)
    output = tail_log(BOT_LOG_FILE, 5000)
    print(f"FAIL: Bot timed out after {timeout}s\n{output}")
    return 124


def cmd_run_bot(timeout=60):
    """Start bot + wait for it to finish (combined)."""
    ret = cmd_start_bot()
    if ret != 0:
        return ret
    return cmd_wait_bot(timeout)


def cmd_compare():
    """Compare server and bot datapack directories recursively."""
    server_dp = os.path.join(SERVER_DIR, "datapack")
    bot_dp = os.path.join(BOT_DIR, "datapack")

    if not os.path.isdir(server_dp):
        print(f"FAIL: Server datapack directory not found: {server_dp}")
        return 1
    if not os.path.isdir(bot_dp):
        print(f"FAIL: Bot datapack directory not found: {bot_dp}")
        return 1

    print("Comparing datapacks...")
    diffs = []
    _collect_diffs(filecmp.dircmp(server_dp, bot_dp), diffs)

    if not diffs:
        print("OK: Datapacks are identical")
        return 0
    else:
        for d in diffs[:50]:
            print(f"  DIFF: {d}")
        if len(diffs) > 50:
            print(f"  ... and {len(diffs) - 50} more")
        print(f"FAIL: {len(diffs)} difference(s) found")
        return 1


def _collect_diffs(cmp_result, diffs, prefix=""):
    for f in cmp_result.left_only:
        diffs.append(f"Only in server: {os.path.join(prefix, f)}")
    for f in cmp_result.right_only:
        diffs.append(f"Only in bot: {os.path.join(prefix, f)}")
    for f in cmp_result.diff_files:
        diffs.append(f"Files differ: {os.path.join(prefix, f)}")
    for subdir, sub_cmp in cmp_result.subdirs.items():
        _collect_diffs(sub_cmp, diffs, os.path.join(prefix, subdir))


def cmd_get_server_output():
    output = tail_log(SERVER_LOG_FILE, 10000)
    print(output if output else "(empty)")
    return 0


def cmd_get_bot_output():
    output = tail_log(BOT_LOG_FILE, 10000)
    print(output if output else "(empty)")
    return 0


def cmd_clear_output():
    ensure_state_dir()
    for f in [SERVER_LOG_FILE, BOT_LOG_FILE]:
        try:
            with open(f, "w"):
                pass
        except FileNotFoundError:
            pass
    print("OK: Output buffers cleared")
    return 0


def cmd_status():
    """Show process status."""
    pid = read_pid(SERVER_PID_FILE)
    if pid:
        print(f"Server: running (PID {pid})")
    else:
        print("Server: not running")
    pid = read_pid(BOT_PID_FILE)
    if pid:
        print(f"Bot: running (PID {pid})")
    else:
        print("Bot: not running")
    r = subprocess.run(["pgrep", "-a", SERVER_BIN], capture_output=True, text=True)
    if r.stdout.strip():
        print(f"System processes:\n{r.stdout.strip()}")
    return 0


def cmd_stop_server():
    """Stop server gracefully."""
    pid = read_pid(SERVER_PID_FILE)
    if not pid:
        print("Server not running")
        return 0
    kill_pid(pid)
    remove_pid(SERVER_PID_FILE)
    print("OK: Server stopped")
    return 0


def cmd_full_test():
    """Run complete test sequence."""
    print("=" * 60)
    print("FULL TEST: CatchChallenger integration test")
    print("=" * 60)

    steps = [
        ("Kill existing servers", cmd_kill),
        ("Build server", cmd_build_server),
        ("Save datapack cache", cmd_save_cache),
        ("Start server from cache", cmd_start_server),
        ("Wait for server bind", lambda: cmd_wait_bind(60)),
        ("Build bot", cmd_build_bot),
        ("Run bot test", lambda: cmd_run_bot(60)),
        ("Compare datapacks", cmd_compare),
        ("Kill server (cleanup)", cmd_kill),
    ]

    for i, (name, func) in enumerate(steps, 1):
        print(f"\n--- Step {i}/{len(steps)}: {name} ---")
        ret = func()
        if ret != 0:
            print(f"\n{'=' * 60}")
            print(f"FAILED at step {i}: {name}")
            print(f"{'=' * 60}")
            cmd_kill()
            return ret

    print(f"\n{'=' * 60}")
    print("ALL TESTS PASSED")
    print(f"{'=' * 60}")
    return 0


def _write_testcase_server_log():
    """Write TESTCASE_SERVER_LOG with content from 'correctly bind:' onward.
    If the marker is not found, write the entire log so failures are visible.
    """
    try:
        with open(SERVER_LOG_FILE, "r", errors="replace") as f:
            content = f.read()
    except FileNotFoundError:
        content = ""

    marker = "correctly bind:"
    idx = content.find(marker)
    if idx >= 0:
        out = content[idx:]
    else:
        out = content  # Marker missing => server failed; keep full log
    with open(TESTCASE_SERVER_LOG, "w") as f:
        f.write(out)


def _write_testcase_bot_log():
    try:
        with open(BOT_LOG_FILE, "r", errors="replace") as f:
            content = f.read()
    except FileNotFoundError:
        content = ""
    with open(TESTCASE_BOT_LOG, "w") as f:
        f.write(content)


def cmd_testcase():
    """Run full test, write server.log + bot-tools.log to project dir, then quit.

    server.log contains server output starting from 'correctly bind:'.
    bot-tools.log contains the full bot output.
    Always kills any running server at start and end.
    """
    print("=" * 60)
    print("TESTCASE: CatchChallenger automated test run")
    print("=" * 60)

    # Always start clean
    cmd_kill()

    # Build server (incremental)
    if cmd_build_server() != 0:
        _write_testcase_server_log()
        _write_testcase_bot_log()
        cmd_kill()
        return 1

    # Save datapack cache
    if cmd_save_cache() != 0:
        _write_testcase_server_log()
        _write_testcase_bot_log()
        cmd_kill()
        return 1

    # Start server (loads from cache)
    if cmd_start_server() != 0:
        _write_testcase_server_log()
        _write_testcase_bot_log()
        cmd_kill()
        return 1

    # Wait for bind
    if cmd_wait_bind(60) != 0:
        _write_testcase_server_log()
        _write_testcase_bot_log()
        cmd_kill()
        return 1

    # Build bot
    if cmd_build_bot() != 0:
        _write_testcase_server_log()
        _write_testcase_bot_log()
        cmd_kill()
        return 1

    # Run bot
    bot_ret = cmd_run_bot(60)

    # Compare datapacks regardless (informational)
    cmp_ret = cmd_compare()

    # Always write logs and clean up
    _write_testcase_server_log()
    _write_testcase_bot_log()
    cmd_kill()

    print(f"\n{'=' * 60}")
    print(f"TESTCASE summary: bot={bot_ret} compare={cmp_ret}")
    print(f"  server log -> {TESTCASE_SERVER_LOG}")
    print(f"  bot log    -> {TESTCASE_BOT_LOG}")
    print(f"{'=' * 60}")

    if bot_ret != 0 or cmp_ret != 0:
        return 1
    return 0


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        return 1

    cmd = sys.argv[1].lower()
    if cmd not in ALLOWED_COMMANDS:
        print(f"ERROR: Unknown or disallowed command: {cmd}")
        print(f"Allowed: {', '.join(sorted(ALLOWED_COMMANDS))}")
        return 1

    ensure_state_dir()

    timeout_arg = int(sys.argv[2]) if len(sys.argv) > 2 else 60

    dispatch = {
        "kill": cmd_kill,
        "build-server": cmd_build_server,
        "build-bot": cmd_build_bot,
        "save-cache": cmd_save_cache,
        "start-server": cmd_start_server,
        "wait-bind": lambda: cmd_wait_bind(timeout_arg),
        "start-bot": cmd_start_bot,
        "wait-bot": lambda: cmd_wait_bot(timeout_arg),
        "run-bot": lambda: cmd_run_bot(timeout_arg),
        "compare": cmd_compare,
        "get-server-output": cmd_get_server_output,
        "get-bot-output": cmd_get_bot_output,
        "clear-output": cmd_clear_output,
        "status": cmd_status,
        "full-test": cmd_full_test,
        "testcase": cmd_testcase,
        "stop-server": cmd_stop_server,
    }

    return dispatch[cmd]()


if __name__ == "__main__":
    sys.exit(main() or 0)
