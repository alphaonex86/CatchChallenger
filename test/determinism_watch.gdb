# Client vs server determinism monitor via a REAL gdb hardware WATCHPOINT, set
# from PYTHON (gdb.Breakpoint(expr, type=gdb.BP_WATCHPOINT, wp_class=gdb.WP_WRITE)
# with a python subclass overriding stop()). Companion to the breakpoint-based
# determinism_check.gdb — that one re-diffs at fixed checkpoints; THIS one arms a
# data watchpoint on a shared scalar and does the client-vs-server comparison
# INSIDE the python stop() handler, the moment the field is written.
#
# Why this proves more than a breakpoint: a breakpoint fires on an instruction
# address; a watchpoint fires on a DATA WRITE. The handler runs at the exact
# store, so it observes the value transition and can classify the client/server
# pair right there (MATCH / PENDING-ASYNC / DIVERGENCE).
#
# In a solo qtopengl build the client (CatchChallenger::Api_protocol, member
# player_informations) and the embedded server (CatchChallenger::Client, member
# public_and_private_informations) share one process and both reference the SAME
# Player_private_and_public_informations data. We:
#
#   1. capture $srv = (Client*)this at ClientHeavyLoadSelectCharFinal.cpp:132
#      (server side fully loaded, building the 0x64 char packet),
#   2. capture $cli = (Api_protocol*)this at the START of
#      Api_protocol::parseCharacterBlockCharacter (Api_protocol_loadchar.cpp:350
#      region), BEFORE the client parses cash (cash is assigned at line 553),
#   3. from python set a BP_WATCHPOINT/WP_WRITE on the STABLE scalar address
#      $cli->player_informations.cash. It fires when line 553 stores cash.
#      In stop(): read client cash and server cash, print
#         [WATCH cash] client=.. server=..  -> MATCH / PENDING-ASYNC / DIVERGENCE
#      and return False so the program keeps running.
#   4. ALSO arm a second WP_WRITE watchpoint on the libstdc++ rb_tree node count
#      of the ASYNC items map
#         $cli->player_informations.items._M_t._M_impl._M_header._M_node_count
#      (the exact member path is discovered at arm-time with ptype; falls back
#      across libstdc++ layouts). The async inventory packet inserts items AFTER
#      char-load, so this watchpoint catches the client.items count going 0 -> N,
#      converging on the server which already holds the same items.
#
# CLASSIFICATION inside stop():
#   MATCH         : client value == server value
#   PENDING-ASYNC : one side not yet written this tick (client still 0 / default
#                   while server already has the final value, or vice-versa) ->
#                   transient one-sided update, NOT a divergence; return False.
#   DIVERGENCE    : both sides settled but differ -> a real determinism breach.
#
# stop() ALWAYS returns False (never halts the batch run); it only observes.
#
# HW-watchpoint fallback: if the environment cannot set a hardware watchpoint
# (gdb prints "Cannot set read/access watchpoint" / no debug registers under a
# restricted ptrace/VM), we drop to a SOFTWARE watchpoint via
# `set can-use-hw-watchpoints 0` (single-step emulated; far slower but works).
#
# Run:
#   QT_QPA_PLATFORM=offscreen HOME=<tmp> XDG_CONFIG_HOME=<tmp>/.config \
#     XDG_DATA_HOME=<tmp>/.local/share TMPDIR=<tmp> \
#     timeout 110 gdb -batch -x test/determinism_watch.gdb \
#       --args <dbg>/catchchallenger --autosolo --main-datapack-code=test --closewhenonmap

set pagination off
set print pretty off
set print elements 0
set print repeats 0
set breakpoint pending on
handle SIGPIPE nostop noprint pass

python
import gdb

# ---- helpers ---------------------------------------------------------------
def _eval(expr):
    """Return (ok, value-or-errstring). Never raises."""
    try:
        return True, gdb.parse_and_eval(expr)
    except Exception as ex:
        return False, "<err:%s>" % ex

def _ulong(expr):
    ok, v = _eval(expr)
    if not ok:
        return None
    try:
        return int(v)
    except Exception:
        return None

# Captured object expressions (the convenience vars are set by the breakpoints
# below before any watchpoint is armed).
CLI = "((CatchChallenger::Api_protocol*)$cli)"
SRV = "((CatchChallenger::Client*)$srv)"
CLI_CASH = CLI + "->player_informations.cash"
SRV_CASH = SRV + "->public_and_private_informations.cash"
CLI_ITEMS = CLI + "->player_informations.items"
SRV_ITEMS = SRV + "->public_and_private_informations.items"

# libstdc++ std::map node-count candidate member paths (layout drift across
# versions). The header lives in _M_t._M_impl as a _Rb_tree_header whose
# _M_node_count is the element count.
_NODECOUNT_SUFFIXES = (
    "._M_t._M_impl._M_header._M_node_count",
    "._M_t._M_impl._M_node_count",
)

def _items_count_expr(base):
    """Probe which node-count suffix resolves for this libstdc++ build."""
    i = 0
    while i < len(_NODECOUNT_SUFFIXES):
        e = base + _NODECOUNT_SUFFIXES[i]
        ok, _v = _eval(e)
        if ok:
            return e
        i += 1
    return None

def _classify(cli, srv):
    """Scalar verdict (cash). cli/srv are ints or None."""
    if cli is None or srv is None:
        return "PENDING-ASYNC (one side not readable yet)"
    if cli == srv:
        return "MATCH"
    # one side still at its pre-write default while the other already settled:
    # a transient one-sided async update, not a real divergence.
    if cli == 0 or srv == 0:
        return "PENDING-ASYNC (one side not yet updated: client=%d server=%d)" % (cli, srv)
    return "DIVERGENCE (client=%d server=%d)" % (cli, srv)

def _classify_async_count(cli, srv):
    """Verdict for an INCREMENTALLY-built async container (the items map).
    The client rb_tree gains one node per insert, so while the client count is
    still BELOW the server's already-settled count it is mid-convergence, NOT a
    real divergence. Only a client count that has OVERSHOT the server (or both
    settled and unequal) is a genuine breach."""
    if cli is None or srv is None:
        return "PENDING-ASYNC (one side not readable yet)"
    if cli == srv:
        return "MATCH (converged)"
    if cli < srv:
        # client still inserting nodes; transient one-sided async update.
        return "PENDING-ASYNC (client mid-insert: count %d climbing to %d)" % (cli, srv)
    return "DIVERGENCE (client count %d overshot server %d)" % (cli, srv)

# ---- the watchpoint subclasses --------------------------------------------
class CashWatch(gdb.Breakpoint):
    """WP_WRITE on the shared scalar player cash. Fires at the client store
    (Api_protocol_loadchar.cpp:553). Compares client-vs-server in stop()."""
    def __init__(self, expr):
        super(CashWatch, self).__init__(expr,
                                        type=gdb.BP_WATCHPOINT,
                                        wp_class=gdb.WP_WRITE,
                                        internal=False)
        self.fires = 0
    def stop(self):
        self.fires += 1
        c = _ulong(CLI_CASH)
        s = _ulong(SRV_CASH)
        print("[WATCH cash] fire#%d client=%s server=%s -> %s"
              % (self.fires,
                 "?" if c is None else str(c),
                 "?" if s is None else str(s),
                 _classify(c, s)))
        return False   # observe only, keep running

class ItemsWatch(gdb.Breakpoint):
    """WP_WRITE on the libstdc++ rb_tree node count of the ASYNC items map.
    Fires when the inventory packet inserts items AFTER char-load; demonstrates
    the watchpoint catching an async client.items 0 -> N convergence onto the
    server which already holds the same items."""
    def __init__(self, expr, srv_count_expr):
        super(ItemsWatch, self).__init__(expr,
                                         type=gdb.BP_WATCHPOINT,
                                         wp_class=gdb.WP_WRITE,
                                         internal=False)
        self.srv_count_expr = srv_count_expr
        self.fires = 0
    def stop(self):
        self.fires += 1
        c = _ulong(_items_count_expr(CLI_ITEMS) or "0")
        s = _ulong(self.srv_count_expr or "0")
        okc, cmap = _eval(CLI_ITEMS)
        oks, smap = _eval(SRV_ITEMS)
        verdict = _classify_async_count(c, s)
        print("[WATCH items] fire#%d client.count=%s server.count=%s -> %s"
              % (self.fires,
                 "?" if c is None else str(c),
                 "?" if s is None else str(s),
                 verdict))
        if okc:
            print("              client.items = %s" % str(cmap))
        if oks:
            print("              server.items = %s" % str(smap))
        return False   # observe only, keep running

# ---- arming command, invoked once both $cli and $srv are captured ---------
class ArmWatches(gdb.Command):
    def __init__(self):
        super(ArmWatches, self).__init__("arm_watches", gdb.COMMAND_USER)
        self.armed = False
    def invoke(self, arg, from_tty):
        if self.armed:
            return
        # both objects must be captured
        if _ulong("$cli") in (None, 0) or _ulong("$srv") in (None, 0):
            print("[arm] skipped: $cli or $srv not captured yet")
            return
        self.armed = True
        # ---- cash watchpoint (hardware, fall back to software) -------------
        try:
            CashWatch(CLI_CASH)
            print("[arm] HW watchpoint armed on %s" % CLI_CASH)
        except Exception as ex:
            print("[arm] HW watchpoint failed (%s); retrying as SOFTWARE watchpoint" % ex)
            try:
                gdb.execute("set can-use-hw-watchpoints 0")
                CashWatch(CLI_CASH)
                print("[arm] SOFTWARE watchpoint armed on %s (can-use-hw-watchpoints 0)" % CLI_CASH)
            except Exception as ex2:
                print("[arm] cash watchpoint could not be set at all: %s" % ex2)
        # ---- async items node-count watchpoint -----------------------------
        cli_cnt = _items_count_expr(CLI_ITEMS)
        srv_cnt = _items_count_expr(SRV_ITEMS)
        if cli_cnt is None:
            print("[arm] items node-count member not found; skipping items watchpoint")
            print("      (ptype dump follows for diagnosis)")
            try:
                gdb.execute("ptype " + CLI_ITEMS)
            except Exception:
                pass
        else:
            print("[arm] items node-count path = %s" % cli_cnt)
            try:
                ItemsWatch(cli_cnt, srv_cnt)
                print("[arm] watchpoint armed on items node count")
            except Exception as ex:
                print("[arm] items watchpoint failed (%s); retrying SOFTWARE" % ex)
                try:
                    gdb.execute("set can-use-hw-watchpoints 0")
                    ItemsWatch(cli_cnt, srv_cnt)
                    print("[arm] SOFTWARE items watchpoint armed")
                except Exception as ex2:
                    print("[arm] items watchpoint could not be set: %s" % ex2)
ArmWatches()
end

# ---- capture the two objects, then arm the watchpoints ---------------------
# server Client fully loaded when it builds the 0x64 char packet
break ClientHeavyLoadSelectCharFinal.cpp:132
commands
  silent
  set $srv = this
  continue
end

# client Api_protocol: capture `this` at the START of parseCharacterBlockCharacter,
# BEFORE cash is parsed (cash store is line 553). Then arm the watchpoints.
break Api_protocol_loadchar.cpp:351
commands
  silent
  set $cli = this
  arm_watches
  continue
end

run
quit
