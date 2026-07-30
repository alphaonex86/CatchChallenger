#!/usr/bin/env python3
"""Rewrite the NPC text of a generated datapack with a LOCAL LLM (Ollama).

The generator always writes a valid, playable datapack first (offline lines from
dialog.txt and per-role sentences) and records WHAT it wrote, with the context of
the city, in `dest/npc-slots.json`. This script replaces those texts with lines
written for that exact place: the city name, its size, its level, its element and
the kind of building the NPC stands in.

Two modes:
  bucket (default) one call per (building, role, field, style, size, level tier)
                   -> a pool of lines reused across the cities that share the
                      context. ~150 calls for a whole world.
  --per-city       one call per building. Unique lines everywhere, but one call
                   per building (hours on a big world).

Everything is cached by the sha256 of the prompt, so a second run is instant and
gives the SAME datapack (fixed seed + temperature): the generation stays
reproducible.

HARD RULE: no trademarked/franchise wording may reach the datapack. The prompt
forbids it and every produced line is checked against FORBIDDEN; a line that
mentions any of it is dropped and the offline fallback is kept.
"""
import argparse
import hashlib
import json
import os
import re
import sys
import urllib.error
import urllib.request

FORBIDDEN = ["pokemon", "pokémon", "pokeball", "poke ball", "poké", "pokedex",
             "pokemart", "pokecenter", "poke center", "pikachu", "nintendo",
             "game freak", "charizard", "bulbasaur", "squirtle", "charmander",
             "eevee", "digimon", "pokestop", "pokedollar", "gotta catch"]

DEFAULT_MODEL = "mistral-small-4:119b"
CACHE_DIR = os.path.join(
    os.path.expanduser("~"), ".local", "share", "CatchChallenger",
    "map-procedural-generation")
CACHE_FILE = os.path.join(CACHE_DIR, "npc-cache.json")
LINES_PER_BUCKET = 8

FIELD_ASK = {
    "text": "one spoken line",
    "name": "a person name (one or two words)",
    "start": "the line the trainer says when the fight starts",
    "win": "the line the trainer says after losing the fight",
}


def level_tier(level):
    """Cities of nearby levels share their pool: 4 tiers over the world."""
    return min(3, int(level) // 15)


def bucket_key(slot, per_city):
    """What makes two lines interchangeable.

    The building GROUP (heal-small, shop-big, gym-building or the city style)
    already carries the size of a key building, and the element only changes
    what a gym LEADER says (their team is typed). Keeping the key coarse is what
    makes a whole world ~400 model calls instead of thousands.
    """
    group = slot["building"].strip("/").split("/")[0]
    parts = [group, slot["role"], slot["field"], str(level_tier(slot["level"]))]
    if group.endswith("-city"):
        parts.append(slot["size"])
    if slot["role"] == "gym leader":
        parts.append(slot["element"])
    if per_city:
        parts.append(slot["city"])
    return "|".join(parts)


def prompt_of(slot, per_city, count):
    place = {"healer": "the healing house of the town",
             "shopkeeper": "the town store",
             "storage": "the monster storage counter",
             "trainer": "a training hall",
             "gym leader": "the training hall of the town champion",
             "villager": "a house of the town"}.get(slot["role"], "a building")
    where = ("the %s town of %s" % (slot["size"], slot["city"]) if per_city
             else "a %s town" % slot["size"])
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
        "Setting: %s, built in a %s landscape, %s element, adventurers around "
        "level %d.\n"
        "Speaker: %s, standing in %s.\n"
        "Write %d different lines, each %s.\n"
        "Style: English, ONE sentence each, at most 110 characters, plain "
        "words, no quotes, no emoji, no markdown, no numbering, no name of a "
        "real place or brand.\n"
        "Answer with a JSON array of %d strings and nothing else." % (
            where, slot["style"].replace("-city", ""), element,
            int(slot["level"]), slot["role"], place, count,
            FIELD_ASK.get(slot["field"], "one spoken line"), count))


def load_cache():
    if os.path.exists(CACHE_FILE):
        try:
            return json.load(open(CACHE_FILE, encoding="utf-8"))
        except ValueError:
            print("cache unreadable, starting a new one")
    return {}


def save_cache(cache):
    if not os.path.isdir(CACHE_DIR):
        os.makedirs(CACHE_DIR)
    temporary = CACHE_FILE + ".tmp"
    json.dump(cache, open(temporary, "w", encoding="utf-8"))
    os.replace(temporary, CACHE_FILE)


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
    """The model is asked for a JSON array; accept a fenced, prefixed or
    {"lines": [...]} shaped one, and fall back to plain lines."""
    text = answer.strip()
    start = text.find("[")
    end = text.rfind("]")
    lines = []
    if start >= 0 and end > start:
        try:
            parsed = json.loads(text[start:end + 1])
            lines = [str(x) for x in parsed if isinstance(x, str)]
        except ValueError:
            lines = []
    if not lines and start >= 0 and end > start:
        #a malformed array (trailing comma, unescaped quote, comment after it):
        #take the quoted strings back out of it
        lines = [m.group(1) for m in
                 re.finditer(r'"((?:[^"\\]|\\.)*)"', text[start:end + 1])]
    if not lines:
        start = text.find("{")
        end = text.rfind("}")
        if start >= 0 and end > start:
            try:
                parsed = json.loads(text[start:end + 1])
                for value in parsed.values():
                    if isinstance(value, list):
                        lines = [str(x) for x in value if isinstance(x, str)]
            except ValueError:
                lines = []
    if not lines:
        #plain text answer: one line per line, drop everything that still looks
        #like json debris (a half parsed array leaves `"foo",` fragments)
        for raw in text.split("\n"):
            raw = raw.strip().strip("-*0123456789. ").strip()
            if raw.endswith(",") or raw.endswith('",'):
                raw = raw.rstrip(",").strip()
            raw = raw.strip('"').strip()
            if len(raw) > 3 and not raw.startswith("```"):
                lines.append(raw)
    return lines


#a name is a name, not a sentence
MAX_LENGTH = {"name": 28, "text": 160, "start": 160, "win": 160}


#the game font is a small bitmap font: keep the text to plain ASCII punctuation
TYPOGRAPHIC = {"\u2019": "'", "\u2018": "'", "\u201c": '"', "\u201d": '"',
               "\u2013": " - ", "\u2014": " - ", "\u2026": "...",
               "\u00a0": " "}


#a model sometimes answers with a piece of its own instructions ("Answer with a
#valid JSON array"): that is not dialogue and must never reach a NPC
META = ["json", "array", "answer with", "as an ai", "here are", "here's a",
        "certainly", "of course!", "instruction", "prompt", "markdown",
        "one sentence", "characters or less", "at most", "no quotes",
        "trademark", "video game", "npc", "dialogue", "output", "list of",
        "sure, ", "note:", "example"]


def clean(line, field, prompt=""):
    """None when the line may not go into the datapack."""
    line = str(line)
    for fancy, plain in TYPOGRAPHIC.items():
        line = line.replace(fancy, plain)
    line = " ".join(line.split())
    line = line.strip().strip('"').strip()
    if not line or len(line) > MAX_LENGTH.get(field, 160):
        return None
    lowered = line.lower()
    for word in FORBIDDEN:
        if word in lowered:
            return None
    #json debris, markup, or the example of the prompt itself
    for bad in ("```", "<", ">", "]]>", "[", "]", "{", "}", '"', "line1"):
        if bad in line:
            return None
    if line.endswith(","):
        return None
    for word in META:
        if word in lowered:
            return None
    #a line that repeats the instructions word for word is an echo, not dialogue
    if prompt:
        words = lowered.split()
        lowered_prompt = prompt.lower()
        index = 0
        while index + 5 <= len(words):
            if " ".join(words[index:index + 5]) in lowered_prompt:
                return None
            index += 1
    return line


def pick(lines, seed):
    digest = hashlib.sha256(seed.encode()).digest()
    return lines[int.from_bytes(digest[:4], "big") % len(lines)]


def bot_blocks(text):
    return list(re.finditer(r"<bot id=\"\d+\".*?</bot>", text, re.S))


def apply_to_file(path, slots, texts, dry_run):
    """Replace the recorded CDATA / bot names of one generated xml."""
    text = open(path, encoding="utf-8").read()
    original = text
    # names: the Nth bot block owns the name slot N
    for slot in sorted([s for s in slots if s["field"] == "name"],
                       key=lambda s: -s["cdata"]):
        blocks = bot_blocks(text)
        if slot["cdata"] < len(blocks):
            block = blocks[slot["cdata"]]
            replaced = re.sub(r"<name>.*?</name>",
                              "<name>" + texts[id(slot)] + "</name>",
                              block.group(0), count=1, flags=re.S)
            text = text[:block.start()] + replaced + text[block.end():]
    # texts: the Nth CDATA of the file, in generation order
    for slot in sorted([s for s in slots if s["field"] != "name"],
                       key=lambda s: -s["cdata"]):
        sections = list(re.finditer(r"<!\[CDATA\[(.*?)\]\]>", text, re.S))
        if slot["cdata"] < len(sections):
            section = sections[slot["cdata"]]
            body = section.group(1)
            # the <a href=..> links ARE the bot flow: keep them untouched
            links = "".join(re.findall(r"<br />\s*<a href=.*", body, re.S))
            text = (text[:section.start()] + "<![CDATA[" + texts[id(slot)] +
                    links + "]]>" + text[section.end():])
    if text != original and not dry_run:
        open(path, "w", encoding="utf-8").write(text)
    return text != original


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("dest", nargs="?", default="dest",
                        help="the dest/ directory of the generator")
    parser.add_argument("--model", default=DEFAULT_MODEL)
    parser.add_argument("--host",
                        default=os.environ.get("OLLAMA_HOST",
                                               "http://127.0.0.1:11434"))
    parser.add_argument("--per-city", action="store_true",
                        help="one call per building instead of per context")
    parser.add_argument("--timeout", type=int, default=1800)
    parser.add_argument("--limit", type=int, default=0,
                        help="stop after N model calls (test runs)")
    parser.add_argument("--names", action="store_true",
                        help="also ask the model for the NPC names (off: the "
                             "invented names of the generator are kept)")
    parser.add_argument("--only", default="",
                        help="only the buckets whose key contains this text")
    parser.add_argument("--dry-run", action="store_true",
                        help="show the buckets, ask the model nothing")
    arguments = parser.parse_args()
    if "://" not in arguments.host:
        arguments.host = "http://" + arguments.host

    slotsFile = os.path.join(arguments.dest, "npc-slots.json")
    if not os.path.exists(slotsFile):
        print("no " + slotsFile + " (run the generator first)")
        return 2
    slots = json.load(open(slotsFile, encoding="utf-8"))["slots"]
    if not arguments.names:
        #NPC names keep the invented pool of the generator: a model asked for
        #names returns the ones of existing games far too often, and the filter
        #then refuses most of the answer
        slots = [s for s in slots if s["field"] != "name"]
    buckets = {}
    for slot in slots:
        buckets.setdefault(bucket_key(slot, arguments.per_city), []).append(slot)
    print("%d text slots, %d buckets (%s mode)" %
          (len(slots), len(buckets),
           "per-city" if arguments.per_city else "bucket"))
    if arguments.dry_run:
        for key in sorted(buckets)[:20]:
            print("   %-70s %d slots" % (key, len(buckets[key])))
        return 0

    cache = load_cache()
    texts = {}
    calls = 0
    failed = 0
    rejected = 0
    keys = [k for k in sorted(buckets) if arguments.only in k]
    for index, key in enumerate(keys):
        slotList = buckets[key]
        prompt = prompt_of(slotList[0], arguments.per_city, LINES_PER_BUCKET)
        digest = hashlib.sha256((arguments.model + "\n" + prompt).encode()
                                ).hexdigest()
        lines = cache.get(digest)
        if lines is not None:
            #re-filter: the cache was written by an older, weaker filter
            kept = [c for c in (clean(l, slotList[0]["field"], prompt)
                                for l in lines) if c]
            if len(kept) != len(lines):
                print("   %d cached lines dropped by the filter" %
                      (len(lines) - len(kept)))
                cache[digest] = kept
                save_cache(cache)
            lines = kept if len(kept) >= 4 else None
        if lines is None:
            if arguments.limit and calls >= arguments.limit:
                continue
            try:
                answer = ask_ollama(arguments.host, arguments.model, prompt,
                                    arguments.timeout)
                produced = parse_lines(answer)
                lines = [c for c in (clean(l, slotList[0]["field"], prompt)
                                     for l in produced) if c]
                calls += 1
                rejected += len(produced) - len(lines)
                #more than a third of the answer refused (trademark, markup,
                #too long): ask once more, reminding the rule
                if len(lines) < 4 or (produced and
                                      len(lines) * 3 < len(produced) * 2):
                    print("   only %d usable of %d, asking again" %
                          (len(lines), len(produced)))
                    answer = ask_ollama(
                        arguments.host, arguments.model,
                        prompt + "\nYour previous answer was refused. Write "
                        "plain original sentences, no trademark, no name of an "
                        "existing game or character.", arguments.timeout)
                    produced = parse_lines(answer)
                    retryLines = [c for c in (clean(l, slotList[0]["field"],
                                                    prompt)
                                              for l in produced) if c]
                    calls += 1
                    rejected += len(produced) - len(retryLines)
                    lines = lines + [l for l in retryLines if l not in lines]
            except (urllib.error.URLError, OSError, ValueError) as error:
                print("   model call failed (%s): keeping the offline lines"
                      % error)
                lines = []
                calls += 1
            if lines:
                cache[digest] = lines
                save_cache(cache)
            else:
                failed += 1
        if lines:
            for slot in slotList:
                texts[id(slot)] = pick(lines, slot["file"] + "/" +
                                       str(slot["cdata"]) + slot["field"])
        print("[%d/%d] %-64s %d lines" % (index + 1, len(keys), key[:64],
                                          len(lines) if lines else 0))

    byFile = {}
    for slot in slots:
        if id(slot) in texts:
            byFile.setdefault(slot["file"], []).append(slot)
    written = 0
    for relative in sorted(byFile):
        path = os.path.join(arguments.dest, "map", relative)
        if os.path.exists(path):
            if apply_to_file(path, byFile[relative], texts, arguments.dry_run):
                written += 1
        else:
            print("   missing generated file: " + path)
    print("%d model calls (%d without usable answer, %d lines refused by the "
          "trademark/format filter), %d files rewritten" %
          (calls, failed, rejected, written))
    return 0


sys.exit(main())
