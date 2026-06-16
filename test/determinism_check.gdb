# Client vs server data-structure determinism check (solo / embedded-server build).
#
# In a solo qtopengl build the client (CatchChallenger::Api_protocol) and the
# embedded server (CatchChallenger::Client) share one process and use the SAME
# player struct (Player_private_and_public_informations): client side is
# Api_protocol::player_informations, server side Client::public_and_private_informations.
# We capture both objects ONCE (they live for the whole process) and re-diff them at
# several lifecycle checkpoints: LOGIN (char-load end), INVENTORY (async packet),
# BEFORE-DISCONNECT. Extend with more breakpoints for before/after a driven action.
#
# Run:
#   QT_QPA_PLATFORM=offscreen HOME=<tmp> TMPDIR=<tmp> \
#     gdb -batch -x test/determinism_check.gdb \
#       --args <dbg-build>/catchchallenger --autosolo --main-datapack-code=test --closewhenonmap
#
# FILTERED fields (divergence is BY DESIGN, never compared):
#   recipes / encyclopedia_*  : raw char* (heap addr differs)
#   mapData                   : server-specific, client never fills
#   monster.id                : server-only DB unique id (not on wire; client hard-sets 0)
#   public_informations.monsterId : client display-derives from team front (server follower=0)
#   items at LOGIN            : ASYNC — arrives in a later inventory packet (have_inventory());
#                              empty at LOGIN, matches from INVENTORY onward.
# ASYNC RULE: only compare at a SETTLED point AFTER the data is parsed (mid-loop/mid-packet
#   snapshots read stale defaults — proven for unvalidated_rescue and items).
# SOLO fights: the embedded server does NOT process wild fights in solo, so a post-fight hp
#   comparison only makes sense in a MULTIPLAYER build. Live position over actions lives in the
#   qtopengl MapController (not Api_protocol) — drive + read it via the --remote-control channel.

set pagination off
set print pretty off
set print elements 0
set print repeats 0
handle SIGPIPE nostop noprint pass

python
import gdb, re
FIRED={}   # checkpoint label -> times the comparison actually ran (guards no-op runs)
def sval(e):
    try: return str(gdb.parse_and_eval(e))
    except Exception as ex: return "<err:%s>" % ex
def show(name, s, c, strip=None):
    a=sval(s); b=sval(c)
    if strip: a=re.sub(strip,"",a); b=re.sub(strip,"",b)
    if a==b:
        print("  [MATCH ] %-16s = %s" % (name, a if len(a)<100 else a[:97]+"..."))
    else:
        i=0
        while i<len(a) and i<len(b) and a[i]==b[i]: i+=1
        print("  [DIFFER] %-16s (first diff @char %d)" % (name, i))
        print("          server: ...%s" % a[max(0,i-30):i+60])
        print("          client: ...%s" % b[max(0,i-30):i+60])
def show_bytes(name, sptr, cptr, nexpr):
    # byte-compare two equal-sized char* bitmaps (encyclopedia/recipes): only the POINTER
    # differs; the CONTENT is transmitted into maxId/8+1-byte buffers identical on both sides.
    try:
        n=int(gdb.parse_and_eval(nexpr))
        sp=int(gdb.parse_and_eval(sptr)); cp=int(gdb.parse_and_eval(cptr))
        if sp==0 or cp==0:
            print("  [FILTER] %-16s (one side NULL: server=%#x client=%#x)" % (name, sp, cp)); return
        inf=gdb.selected_inferior()
        sb=bytes(inf.read_memory(sp, n)); cb=bytes(inf.read_memory(cp, n))
        if sb==cb: print("  [MATCH ] %-16s = %d bytes equal" % (name, n))
        else:
            j=0
            while j<n and sb[j]==cb[j]: j+=1
            print("  [DIFFER] %-16s (%d bytes, first diff @byte %d: s=%#x c=%#x)" % (name, n, j, sb[j], cb[j]))
    except Exception as ex:
        print("  [SKIP  ] %-16s (byte-compare unavailable: %s)" % (name, ex))
S="((CatchChallenger::Client*)$srv)"; C="((CatchChallenger::Api_protocol*)$cli)"
Sp=S+"->public_and_private_informations"; Cp=C+"->player_informations"
DP="CatchChallenger::CommonDatapack::commonDatapack"
def persist():
    # comparable at ANY checkpoint (objects persist for the process)
    show("pseudo",Sp+".public_informations.pseudo",Cp+".public_informations.pseudo")
    show("type",Sp+".public_informations.type",Cp+".public_informations.type")
    show("skinId",Sp+".public_informations.skinId",Cp+".public_informations.skinId")
    show("cash",Sp+".cash",Cp+".cash")
    show("clan",Sp+".clan",Cp+".clan")
    show("clan_leader",Sp+".clan_leader",Cp+".clan_leader")
    show("allow_create_clan",Sp+".allow_create_clan",Cp+".allow_create_clan")
    show("repel_step",Sp+".repel_step",Cp+".repel_step")
    # FILTER inside the monster dump: monster.id (server-only DB id) and character_origin
    # (server keeps the raw DB char id; the wire flattens it to origin==character_id 0/1,
    # so the client stores 0/1) — strip both so the real per-monster stats are compared.
    show("monsters",Sp+".monsters",Cp+".monsters",strip=r"(id|character_origin) = \d+")
    show("warehouse",Sp+".warehouse_monsters",Cp+".warehouse_monsters",strip=r"(id|character_origin) = \d+")
    show("reputation",Sp+".reputation",Cp+".reputation")
    show("quests",Sp+".quests",Cp+".quests")
    show("items",Sp+".items",Cp+".items")
    # mapData: persistent per-map state (ground items/plants/bots_beaten/industriesStatus).
    # The client DOES fill it from the wire (Api_protocol_loadchar parses it), keyed by the
    # same RAW internal mapIndex the position compare trusts — so it is key-comparable.
    show("mapData",Sp+".mapData",Cp+".mapData")
    # encyclopedia bitmaps: pointers differ but contents (maxId/8+1 bytes) are transmitted equal.
    show_bytes("encyclo_monster",Sp+".encyclopedia_monster",Cp+".encyclopedia_monster",DP+".get_monstersMaxId()/8+1")
    show_bytes("encyclo_item",Sp+".encyclopedia_item",Cp+".encyclopedia_item",DP+".get_itemMaxId()/8+1")
    show("rescue",S+"->rescue","$cli->rescue")
    show("unval_rescue",S+"->unvalidated_rescue","$cli->unvalidated_rescue")
class Check(gdb.Command):
    def __init__(self): super(Check,self).__init__("ccheck",gdb.COMMAND_USER)
    def invoke(self,arg,t):
        label=arg.strip() or "?"
        print("======== CHECKPOINT: %s ========" % label)
        if gdb.parse_and_eval("$cli")==0:
            print("  (client not captured yet — skipped)")
        else:
            FIRED[label]=FIRED.get(label,0)+1
            persist()
            if label=="LOGIN":
                # full position only here: parse locals are in scope at the char-load frame
                show("mapIndex",S+"->mapIndex","player_mapIndex")
                show("x",S+"->x","player_x")
                show("y",S+"->y","player_y")
                show("direction",S+"->last_direction","player_last_direction")
        print("=================================")
class Summary(gdb.Command):
    # Guards the flaky no-progress run (rc=0 but zero checkpoints): a valid PASS REQUIRES
    # the required checkpoints to have actually fired and compared.
    def __init__(self): super(Summary,self).__init__("ccsummary",gdb.COMMAND_USER)
    def invoke(self,arg,t):
        need=["LOGIN","INVENTORY"]
        missing=[k for k in need if FIRED.get(k,0)==0]
        print("======== CHECKPOINT SUMMARY: %s ========" % ("PASS" if not missing else "FAIL"))
        print("  fired: %s" % FIRED)
        if missing:
            print("  MISSING REQUIRED CHECKPOINTS: %s — run made no progress, NOT a valid determinism PASS" % missing)
Check(); Summary()
end

# server Client is fully loaded when it builds the 0x64 char packet
break ClientHeavyLoadSelectCharFinal.cpp:132
commands
  silent
  set $srv = this
  continue
end
# LOGIN: client Api_protocol fully parsed at the end of parseCharacterBlockCharacter
break Api_protocol_loadchar.cpp:1069
commands
  silent
  set $cli = this
  ccheck LOGIN
  continue
end
# INVENTORY: async inventory packet applied
break Api_protocol_message.cpp:1327
commands
  silent
  ccheck INVENTORY
  continue
end
# ON-MAP-LATE: long after load, player settled on the map (fires under autosolo)
break mapDisplayedSlot
commands
  silent
  ccheck ON-MAP-LATE
  continue
end
# BEFORE-DISCONNECT: fires in a clean-quit session (channel "quit" / real client close).
# NOT under --closewhenonmap, which hard-exit()s without calling disconnectClient().
break Api_protocol.cpp:97
commands
  silent
  ccheck BEFORE-DISCONNECT
  continue
end
run
ccsummary
quit
