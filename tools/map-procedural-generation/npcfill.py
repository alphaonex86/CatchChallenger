#!/usr/bin/env python3
"""Fill the NPC line bank of the map generator with a LOCAL LLM (Ollama).

    python3 npcfill.py                   # every missing bucket, hours
    python3 npcfill.py --time-limit 30   # stop after 30 minutes, resume later
    python3 npcfill.py --check           # no model: re-filter what is in the bank

The generator writes `dest/npc-requests.json` (every text it produced, with the
context of the city and the BUCKET it belongs to) and reads back
`npc-slots.json` - the reviewed line bank, which lives next to the sources and
is TRACKED BY GIT. This script only touches that bank: a human reads the git
diff, and CMake copies the validated file next to the binary on the next build,
so a generated world is reproducible from committed data.

Nothing under `dest/` is modified. Re-run the generator to see the lines land in
the maps.

A bucket is one context: `<template group>|<role>|<field>|<level tier>` plus the
city size for a house and the element for a gym leader. Every NPC of the same
context draws from the same lines, deterministically, so ~175 buckets cover a
whole world.

HARD RULE: no trademark and no model chatter may reach the datapack. The prompt
forbids it, every line is filtered (FORBIDDEN + META + prompt echo), and --check
re-applies the filter to the whole bank, so improving the filter repairs it.
"""
import argparse
import json
import os
import re
import sys
import time
import urllib.error
import urllib.request

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import franchise_denylist

BANK_FILE = os.path.join(HERE, "npc-slots.json")
DEFAULT_DEST = os.path.join(HERE, "build", "release", "dest")
DEFAULT_MODEL = "mistral-small-4:119b"
LINES_PER_BUCKET = 8
#a bucket holding at least this many reviewed lines is never asked again
ENOUGH_LINES = 6

#trademarked or franchise wording: the generated world is ORIGINAL. The words are what
#we must not carry, so the list is operator-local, see franchise_denylist.py
FORBIDDEN = franchise_denylist.load()
#the model sometimes answers with a piece of its own instructions
META = ["json", "array", "answer with", "as an ai", "here are", "here's a",
        "certainly", "of course!", "instruction", "prompt", "markdown",
        "one sentence", "characters or less", "at most", "no quotes",
        "trademark", "video game", "npc", "dialogue", "output", "list of",
        "sure, ", "note:", "example"]
#the game font is a small bitmap font: plain ASCII punctuation only
TYPOGRAPHIC = {"’": "'", "‘": "'", "“": '"', "”": '"',
               "–": " - ", "—": " - ", "…": "...",
               " ": " "}
MAX_LENGTH = {"name": 28, "text": 160, "start": 160, "win": 160}

FIELD_ASK = {
    "text": "one spoken line",
    "name": "a person name (one or two words)",
    "start": "the line the trainer says when the fight starts",
    "win": "the line the trainer says after losing the fight",
}
PLACE = {"healer": "the healing house of the town",
         "shopkeeper": "the town store",
         "storage": "the creature storage counter",
         "trainer": "a training hall",
         "gym leader": "the training hall of the town champion",
         "villager": "a house of the town"}


def prompt_of(slot, count):
    """A bucket is a CONTEXT, so the prompt describes the context, never one
    particular city: its lines are shared by every city of that bucket."""
    element = slot["element"] if slot["role"] == "gym leader" else "mixed"
    return (
        "You write dialogue for an ORIGINAL video game. It is NOT a licensed "
        "game and has no connection to any existing game.\n"
        "\n"
        "ABSOLUTE RULE - the text must contain NO trademark and NO reference to "
        "an existing work. NEVER write, in any spelling or language: Pokemon, "
        "Pokeball, Pokedex, Pokemart, Pokemon Center, Pokemon League, "
        "Nintendo, Game Freak, Digimon, Yo-kai, or the name of ANY creature, "
        "character, town, region, item or organisation from an existing game, "
        "book, film or series. Invent everything, or stay generic: say "
        "\"creature\", \"team\", \"trainer\", \"the hall\", \"the town\". "
        "A single trademarked word makes the whole answer unusable.\n"
        "\n"
        "Setting: a %s town in a %s landscape, %s element, adventurers around "
        "level %d.\n"
        "Speaker: %s, standing in %s.\n"
        "Write %d different lines, each %s.\n"
        "Style: English, ONE sentence each, at most 110 characters, plain "
        "words, no quotes, no emoji, no markdown, no numbering, no name of a "
        "real place or brand.\n"
        "Answer with a JSON array of %d strings and nothing else." % (
            slot["size"], slot["style"].replace("-city", "") or "mixed",
            element, int(slot["level"]), slot["role"],
            PLACE.get(slot["role"], "a building"), count,
            FIELD_ASK.get(slot["field"], "one spoken line"), count))


def load_bank(path):
    if os.path.exists(path):
        try:
            return json.load(open(path, encoding="utf-8")).get("buckets", {})
        except ValueError:
            print("npc-slots.json unreadable, starting a new bank")
    return {}


def save_bank(path, buckets):
    """Sorted and indented: the git diff of a review has to be readable."""
    temporary = path + ".tmp"
    handle = open(temporary, "w", encoding="utf-8")
    handle.write("{\n \"buckets\": {\n")
    keys = sorted(buckets)
    for position, key in enumerate(keys):
        handle.write("  " + json.dumps(key) + ": [\n")
        for index, line in enumerate(buckets[key]):
            handle.write("   " + json.dumps(line) +
                         (",\n" if index + 1 < len(buckets[key]) else "\n"))
        handle.write("  ]" + (",\n" if position + 1 < len(keys) else "\n"))
    handle.write(" }\n}\n")
    handle.close()
    os.replace(temporary, path)


def ask_ollama(host, model, prompt, timeout):
    body = json.dumps({
        "model": model, "prompt": prompt, "stream": False,
        "options": {"seed": 42, "temperature": 0.8, "num_predict": 900},
    }).encode()
    request = urllib.request.Request(
        host.rstrip("/") + "/api/generate", data=body,
        headers={"Content-Type": "application/json"})
    with urllib.request.urlopen(request, timeout=timeout) as answer:
        return json.loads(answer.read().decode())["response"]


def parse_lines(answer):
    """A JSON array is asked for; accept it fenced, prefixed, wrapped in an
    object, malformed, or as plain text lines."""
    text = answer.strip()
    start = text.find("[")
    end = text.rfind("]")
    lines = []
    if start >= 0 and end > start:
        try:
            lines = [x for x in json.loads(text[start:end + 1])
                     if isinstance(x, str)]
        except ValueError:
            lines = [m.group(1) for m in
                     re.finditer(r'"((?:[^"\\]|\\.)*)"', text[start:end + 1])]
    if not lines:
        start = text.find("{")
        end = text.rfind("}")
        if start >= 0 and end > start:
            try:
                for value in json.loads(text[start:end + 1]).values():
                    if isinstance(value, list):
                        lines = [x for x in value if isinstance(x, str)]
            except ValueError:
                lines = []
    if not lines:
        for raw in text.split("\n"):
            raw = raw.strip().strip("-*0123456789. ").strip()
            raw = raw.rstrip(",").strip().strip('"').strip()
            if len(raw) > 3 and not raw.startswith("```"):
                lines.append(raw)
    return lines


def clean(line, field, prompt=""):
    """None when the line may not go into the datapack."""
    line = str(line)
    for fancy, plain in TYPOGRAPHIC.items():
        line = line.replace(fancy, plain)
    line = " ".join(line.split()).strip().strip('"').strip()
    if not line or len(line) > MAX_LENGTH.get(field, 160):
        return None
    lowered = line.lower()
    for word in FORBIDDEN + META:
        if word in lowered:
            return None
    for bad in ("```", "<", ">", "]]>", "[", "]", "{", "}", '"'):
        if bad in line:
            return None
    if line.endswith(","):
        return None
    if prompt:
        #a line repeating the instructions word for word is an echo, not speech
        words = lowered.split()
        lowered_prompt = prompt.lower()
        index = 0
        while index + 5 <= len(words):
            if " ".join(words[index:index + 5]) in lowered_prompt:
                return None
            index += 1
    return line


def field_of(key):
    parts = key.split("|")
    return parts[2] if len(parts) > 2 else "text"


def check_bank(bank, verbose):
    """Re-apply the filter to every line of the bank."""
    dropped = 0
    for key in sorted(bank):
        kept = []
        for line in bank[key]:
            good = clean(line, field_of(key))
            if good is None:
                dropped += 1
                print("DROP  " + key + ": " + line)
            else:
                kept.append(good)
        if verbose and kept:
            print("%-52s %d lines" % (key, len(kept)))
        bank[key] = kept
    for key in [k for k in bank if not bank[k]]:
        del bank[key]
    return dropped


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--dest", default=DEFAULT_DEST,
                        help="dest/ of the generator (holds npc-requests.json)")
    parser.add_argument("--bank", default=BANK_FILE,
                        help="the reviewed line bank (git tracked)")
    parser.add_argument("--model", default=DEFAULT_MODEL)
    parser.add_argument("--host",
                        default=os.environ.get("OLLAMA_HOST",
                                               "http://127.0.0.1:11434"))
    parser.add_argument("--time-limit", type=float, default=0,
                        help="stop after that many MINUTES (0 = no limit)")
    parser.add_argument("--limit", type=int, default=0,
                        help="stop after N model calls (0 = no limit)")
    parser.add_argument("--timeout", type=int, default=1800,
                        help="seconds allowed to ONE model answer")
    parser.add_argument("--names", action="store_true",
                        help="also ask the model for the NPC names (off: the "
                             "invented names of the generator are kept, a model "
                             "returns names of existing games far too often)")
    parser.add_argument("--only", default="",
                        help="only the buckets whose key contains this text")
    parser.add_argument("--check", action="store_true",
                        help="no model call: re-filter the bank and report")
    parser.add_argument("--dry-run", action="store_true",
                        help="list what would be asked, ask nothing")
    arguments = parser.parse_args()
    if "://" not in arguments.host:
        arguments.host = "http://" + arguments.host

    bank = load_bank(arguments.bank)
    if arguments.check:
        dropped = check_bank(bank, True)
        print("\n%d buckets, %d lines, %d dropped by the filter" %
              (len(bank), sum(len(v) for v in bank.values()), dropped))
        if dropped:
            save_bank(arguments.bank, bank)
            print("npc-slots.json rewritten without them")
        return 1 if dropped else 0

    requests = os.path.join(arguments.dest, "npc-requests.json")
    if not os.path.exists(requests):
        print("no " + requests + "\nrun the generator first:")
        print("  <build>/map-procedural-generation --datapack <datapack>")
        return 2
    slots = json.load(open(requests, encoding="utf-8"))["slots"]
    wanted = {}
    for slot in slots:
        if slot["field"] == "name" and not arguments.names:
            continue
        if arguments.only and arguments.only not in slot["bucket"]:
            continue
        wanted.setdefault(slot["bucket"], slot)
    todo = [key for key in sorted(wanted)
            if len(bank.get(key, [])) < ENOUGH_LINES]
    print("%d contexts used by the maps, %d already in the bank, %d to ask" %
          (len(wanted), len(wanted) - len(todo), len(todo)))
    if arguments.dry_run:
        for key in todo[:25]:
            print("   " + key)
        return 0

    started = time.time()
    calls = 0
    refused = 0
    for index, key in enumerate(todo):
        if arguments.limit and calls >= arguments.limit:
            print("call limit reached, %d contexts left" % (len(todo) - index))
            break
        if arguments.time_limit and \
                time.time() - started > arguments.time_limit * 60:
            print("time limit reached, %d contexts left (run it again to "
                  "continue)" % (len(todo) - index))
            break
        slot = wanted[key]
        prompt = prompt_of(slot, LINES_PER_BUCKET)
        lines = list(bank.get(key, []))
        attempt = 0
        while attempt < 2 and len(lines) < ENOUGH_LINES:
            try:
                answer = ask_ollama(arguments.host, arguments.model,
                                    prompt if attempt == 0 else prompt +
                                    "\nYour previous answer was refused. Write "
                                    "plain original sentences, no trademark, no "
                                    "name of an existing game or character.",
                                    arguments.timeout)
                produced = parse_lines(answer)
                good = [c for c in (clean(l, slot["field"], prompt)
                                    for l in produced) if c]
                refused += len(produced) - len(good)
                lines += [l for l in good if l not in lines]
            except (urllib.error.URLError, OSError, ValueError) as error:
                print("   model call failed (%s)" % error)
            calls += 1
            attempt += 1
        if lines:
            bank[key] = lines
            save_bank(arguments.bank, bank)
        print("[%d/%d] %-52s %d lines" % (index + 1, len(todo), key[:52],
                                          len(lines)))
    print("\n%d model calls, %d lines refused by the filter, %d buckets in %s" %
          (calls, refused, len(bank), os.path.relpath(arguments.bank, HERE)))
    print("review the git diff of npc-slots.json, then rebuild: CMake copies it "
          "next to the binary and the generator uses it")
    return 0


sys.exit(main())
