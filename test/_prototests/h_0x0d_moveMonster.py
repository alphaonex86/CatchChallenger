"""Exercise moveMonster framing and rejection on the short-party test fixture.

Client::moveUpMonster/moveDownMonster override CommonFightEngine: a failed
reorder KICKS the client. A socket left open after that kick is not proof of
success. Keep every original input, use a fresh session after each rejection,
and verify an untouched witness survives with its identity and position intact.
The five position cases cover up/down, empty/short-party bounds and index 255;
the three invalid mode cases and three fragmented/junk cases cover the parser.
"""

import _protoharness as H

NAME = "0x0D moveMonster"


def _identity(session):
    return session.character_id, session.x, session.y, session.mapIndex


def _persisted(server, session):
    snapshot = H.reload_state(server, session.login_creds, session.pass_creds)
    return tuple(snapshot.get(key) for key in ("character_id", "x", "y", "mapIndex"))


def run(server):
    try:
        witness = H.Session(server, pseudo="Witness")
        witness_before = _identity(witness)
        positions = ((1, 1), (2, 0), (2, 255), (1, 2), (2, 1))
        modes = ((0, 0), (3, 1), (255, 5))
        for mode, position in positions + modes:
            trainer = H.Session(server, pseudo="Trainer")
            before = _identity(trainer)
            trainer.m(0x0D, H.u8(mode) + H.u8(position))
            trainer.drain(timeout=0.2)
            if not H.session_was_kicked(trainer):
                return False, "invalid reorder mode=%d position=%d did not kick" % (mode, position)
            trainer.close()
            if _persisted(server, trainer) != before:
                return False, "rejected reorder changed the trainer identity or position"
            if not server.alive() or server.crash_report() is not None:
                return False, "server failed after rejected reorder"

        malformed = (
            (bytes([0x0D, 0x01]), bytes([0x0D, 0x02, 0x00])),
            (bytes([0x0D, 0x01, 0x01, 0xFF, 0xFF, 0xFF, 0xFF]),),
            (bytes([0x0D]), bytes([0xAB, 0xCD]), bytes([0xDE, 0xAD, 0xBE, 0xEF])),
        )
        for fragments in malformed:
            trainer = H.Session(server, pseudo="Framing")
            for fragment in fragments:
                try:
                    trainer.send_raw(fragment)
                    trainer.drain(timeout=0.15)
                except OSError:
                    if not H.session_was_kicked(trainer):
                        return False, "malformed input lost its connection without a kick"
                    break
            trainer.close()
            if not server.alive() or server.crash_report() is not None:
                return False, "server failed after malformed reorder framing"

        # This valid zero-step look does not depend on party size or map geometry.
        witness.m(0x02, H.u8(0) + H.u8(1))
        witness.drain(timeout=0.2)
        if H.session_was_kicked(witness, timeout=0):
            return False, "another client's rejected reorder kicked the witness"
        witness.close()
        if _persisted(server, witness) != witness_before:
            return False, "another client's rejected reorder changed the witness"
        other = H.Session(server, pseudo="After")
        other.close()
        if not server.alive() or server.crash_report() is not None:
            return False, "server unusable after reorder cases"
        return True, ("position-rejections=%d mode-rejections=%d malformed=%d; "
                      "all invalid reorders kicked, persisted identity/position intact, "
                      "witness unaffected and fresh login OK" %
                      (len(positions), len(modes), len(malformed)))
    except Exception as exc:
        return False, "unexpected exception: %r" % exc
