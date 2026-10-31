"""Valgrind/protocol test for the "/take objectId player [quantity]" admin chat
command (handler 0x03, server/base/ClientEvents/LocalClientHandlerCommand.cpp).

Regression: the connected-player check was inverted (isNull()==true is an EMPTY
slot), so /take answered "player is not connected" for every connected player
and never removed anything.

Strategy (everyBodyIsRoot unlocks the admin commands):
  * "Taker" grants "Target" 3 Level books (item 16) with /give, then takes 2 back
    with /take: Target's live inventory (0x55 add / 0x56 remove) must drop 3 -> 1.
  * /take on the taker itself: its own copy goes 1 -> 0.
  * Target disconnects, reload_state: the persisted quantity is 1.
  * The server stays up and clean throughout.
"""

import time
import _protoharness as H

NAME = "0x03 chat /take admin command"

NEEDS_EVERY_BODY_IS_ROOT = True

ITEM_LEVELBOOK = 16      # "Level book", not in the starter inventory


def _uniq(prefix):
    return ("%s%x" % (prefix, int(time.time() * 1000) & 0xFFFFFFFF)).encode()


def _command(sess, text, settle=0.6):
    """Send a local chat line starting with '/': [chatType=0x01][textSize:u8][text]."""
    raw = text.encode()
    sess.m(0x03, bytes([0x01, len(raw)]) + raw, dynamic=True)
    sess.drain(timeout=settle)


def _alive_clean(server):
    if not server.alive():
        return False, "server.alive() == False"
    cr = server.crash_report()
    if cr is not None:
        return False, "crash_report: %s" % cr[:300]
    return True, ""


def run(server):
    try:
        tlogin, tpassw = _uniq("tgt"), _uniq("tpw")
        try:
            taker = H.Session(server, login=_uniq("tkr"), passh=_uniq("kpw"), pseudo="Taker")
            target = H.Session(server, login=tlogin, passh=tpassw, pseudo="Target")
        except H.HandshakeError as e:
            return (False, "handshake failed: %s" % e)
        _command(taker, "/give %d %s 3" % (ITEM_LEVELBOOK, target.pseudo))
        target.drain(timeout=0.6)
        granted = target.items.get(ITEM_LEVELBOOK, 0)
        if granted != 3:
            return (False, "/give precondition: Target has %d Level books, expected 3" % granted)
        _command(taker, "/take %d %s 2" % (ITEM_LEVELBOOK, target.pseudo))
        target.drain(timeout=0.6)
        left = target.items.get(ITEM_LEVELBOOK, 0)
        if left != 1:
            return (False, "/take 2 from a connected player: Target has %d Level books, expected 1" % left)
        # on the taker itself
        H.grant_item(taker, ITEM_LEVELBOOK, 1)
        _command(taker, "/take %d %s" % (ITEM_LEVELBOOK, taker.pseudo))
        own = taker.items.get(ITEM_LEVELBOOK, 0)
        if own != 0:
            return (False, "/take on itself: Taker still has %d Level books" % own)
        ok, why = _alive_clean(server)
        if not ok:
            return (False, "after /take: %s" % why)
        # persisted
        try:
            target.close()
        except OSError:
            pass
        time.sleep(0.35)
        snap = H.reload_state(server, tlogin, tpassw)
        saved = (snap.get("items") or {}).get(ITEM_LEVELBOOK, 0)
        if saved != 1:
            return (False, "persisted Target Level books=%d, expected 1 (items=%r)" % (saved, snap.get("items")))
        ok, why = _alive_clean(server)
        if not ok:
            return (False, "after reload_state: %s" % why)
        return (True, "/give 3 then /take 2 -> 1 (live and persisted), /take on itself -> 0")
    except Exception as e:
        return (False, "unexpected: %r" % e)
