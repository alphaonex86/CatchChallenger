#include "FiledbConverter.hpp"

#include "base/GeneralStructures.hpp"
#include "base/GeneralType.hpp"
#include "hps/hps.h"
#include "tinyXML2/tinyxml2.hpp"

#include <dirent.h>
#include <sys/stat.h>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

using namespace CatchChallenger;
using tinyxml2::XMLDocument;
using tinyxml2::XMLElement;

namespace FiledbConverter {

// ---------------------------------------------------------------- helpers --

static bool endsWith(const std::string &s, const std::string &suffix) {
    return s.size() >= suffix.size() &&
           s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0;
}

static std::string baseName(const std::string &path) {
    const auto p = path.find_last_of('/');
    return p == std::string::npos ? path : path.substr(p + 1);
}

static std::string dirName(const std::string &path) {
    const auto p = path.find_last_of('/');
    return p == std::string::npos ? std::string(".") : path.substr(0, p);
}

static std::string parentBaseName(const std::string &path) { return baseName(dirName(path)); }
static std::string grandParentBaseName(const std::string &path) { return baseName(dirName(dirName(path))); }

static bool isDir(const std::string &p) {
    struct stat sb{};
    return ::stat(p.c_str(), &sb) == 0 && S_ISDIR(sb.st_mode);
}

static bool isFile(const std::string &p) {
    struct stat sb{};
    return ::stat(p.c_str(), &sb) == 0 && S_ISREG(sb.st_mode);
}

static std::string bytesToHex(const void *data, size_t size) {
    static const char *digits = "0123456789abcdef";
    const unsigned char *p = static_cast<const unsigned char *>(data);
    std::string out;
    out.resize(size * 2);
    for (size_t i = 0; i < size; i++) {
        out[2 * i + 0] = digits[p[i] >> 4];
        out[2 * i + 1] = digits[p[i] & 0x0f];
    }
    return out;
}

static std::string bytesToHex(const std::string &s) { return bytesToHex(s.data(), s.size()); }
static std::string bytesToHex(const std::vector<char> &v) { return bytesToHex(v.data(), v.size()); }

static bool hexToBytes(const char *hex, std::string &out) {
    out.clear();
    if (!hex) return true;
    const size_t n = std::strlen(hex);
    if (n % 2) return false;
    out.reserve(n / 2);
    auto dv = [](char c, int &v) {
        if (c >= '0' && c <= '9') { v = c - '0'; return true; }
        if (c >= 'a' && c <= 'f') { v = 10 + c - 'a'; return true; }
        if (c >= 'A' && c <= 'F') { v = 10 + c - 'A'; return true; }
        return false;
    };
    for (size_t i = 0; i < n; i += 2) {
        int hi = 0, lo = 0;
        if (!dv(hex[i], hi) || !dv(hex[i + 1], lo)) return false;
        out.push_back(static_cast<char>((hi << 4) | lo));
    }
    return true;
}

static bool hexToBytes(const char *hex, std::vector<char> &out) {
    std::string tmp;
    if (!hexToBytes(hex, tmp)) return false;
    out.assign(tmp.begin(), tmp.end());
    return true;
}

// ---- XML API shim (this tinyxml2 predates InsertNewChildElement / 64-bit
// accessors). Keep these helpers narrow so call sites stay readable.
static XMLElement *addChild(XMLElement *parent, const char *tag) {
    XMLElement *e = parent->GetDocument()->NewElement(tag);
    parent->InsertEndChild(e);
    return e;
}

static unsigned aU32(const XMLElement *e, const char *name, unsigned def = 0) {
    unsigned v = def; e->QueryUnsignedAttribute(name, &v); return v;
}
static uint64_t aU64(const XMLElement *e, const char *name, uint64_t def = 0) {
    const char *s = e->Attribute(name);
    if (!s || !*s) return def;
    return std::strtoull(s, nullptr, 10);
}
static int aI32(const XMLElement *e, const char *name, int def = 0) {
    int v = def; e->QueryIntAttribute(name, &v); return v;
}
static bool aBool(const XMLElement *e, const char *name, bool def = false) {
    bool v = def; e->QueryBoolAttribute(name, &v); return v;
}
static const char *aStr(const XMLElement *e, const char *name, const char *def = "") {
    const char *v = e->Attribute(name); return v ? v : def;
}
static uint64_t textU64(const XMLElement *e, uint64_t def = 0) {
    const char *s = e->GetText();
    if (!s || !*s) return def;
    return std::strtoull(s, nullptr, 10);
}
static unsigned textU32(const XMLElement *e, unsigned def = 0) {
    unsigned v = def; e->QueryUnsignedText(&v); return v;
}
static bool textBool(const XMLElement *e, bool def = false) {
    bool v = def; e->QueryBoolText(&v); return v;
}
static std::string textStr(const XMLElement *e) {
    const char *s = e->GetText(); return s ? std::string(s) : std::string();
}

static void sU32(XMLElement *e, const char *n, uint32_t v) { e->SetAttribute(n, v); }
static void sU64(XMLElement *e, const char *n, uint64_t v) { e->SetAttribute(n, std::to_string(v).c_str()); }
static void sI32(XMLElement *e, const char *n, int32_t v) { e->SetAttribute(n, v); }
static void sBool(XMLElement *e, const char *n, bool v) { e->SetAttribute(n, v); }
static void sStr(XMLElement *e, const char *n, const std::string &v) { e->SetAttribute(n, v.c_str()); }

// ---------------------------------------------------------- kind detection --

FileKind detectKind(const std::string &path) {
    const std::string name = baseName(path);
    const std::string pname = parentBaseName(path);
    const std::string gname = grandParentBaseName(path);
    if (name == "server" && pname == "server") return FileKind::ServerCounters;
    if (name == "dictionary_map" && pname == "server") return FileKind::DictionaryMap;
    if (name == "dictionary_reputation" && pname == "common") return FileKind::DictionaryReputation;
    if (name == "dictionary_skin" && pname == "common") return FileKind::DictionarySkin;
    if (name == "dictionary_starter" && pname == "common") return FileKind::DictionaryStarter;
    if (pname == "login") return FileKind::Login;
    if (pname == "accounts" && gname == "common") return FileKind::Account;
    if (pname == "characters" && gname == "common") return FileKind::CharacterCommon;
    if (pname == "characters" && gname == "server") return FileKind::CharacterServer;
    if (pname == "clans" && gname == "server") return FileKind::Clan;
    if (pname == "zone" && gname == "server") return FileKind::Zone;
    return FileKind::Unknown;
}

const char *kindName(FileKind k) {
    switch (k) {
        case FileKind::ServerCounters: return "server-counters";
        case FileKind::DictionaryMap: return "dictionary-map";
        case FileKind::DictionaryReputation: return "dictionary-reputation";
        case FileKind::DictionarySkin: return "dictionary-skin";
        case FileKind::DictionaryStarter: return "dictionary-starter";
        case FileKind::Login: return "login";
        case FileKind::Account: return "account";
        case FileKind::CharacterCommon: return "character-common";
        case FileKind::CharacterServer: return "character-server";
        case FileKind::Clan: return "clan";
        case FileKind::Zone: return "zone";
        default: return "unknown";
    }
}

// ================================================================ bin I/O ==
// All load/save below mirror the byte-for-byte format used by
// catchchallenger-server-cli-epoll built from catchchallenger-server-filedb.pro.
// HPS operators come from hps/hps.h; the struct serialize()/parse() methods
// live in general/base/GeneralStructures.hpp (same header the server uses).

struct ServerCounters {
    uint32_t maxClanId = 0;
    uint32_t maxAccountId = 0;
    uint32_t maxCharacterId = 0;
    uint32_t maxCity = 0;
};

static bool loadServerCounters(const std::string &path, ServerCounters &c) {
    std::ifstream in(path, std::ifstream::binary);
    if (!in.good() || !in.is_open()) return false;
    hps::StreamInputBuffer s(in);
    s >> c.maxClanId >> c.maxAccountId >> c.maxCharacterId >> c.maxCity;
    return true;
}
static bool saveServerCounters(const std::string &path, const ServerCounters &c) {
    std::ofstream out(path, std::ofstream::binary | std::ofstream::trunc);
    if (!out.good() || !out.is_open()) return false;
    hps::to_stream(c.maxClanId, out);
    hps::to_stream(c.maxAccountId, out);
    hps::to_stream(c.maxCharacterId, out);
    hps::to_stream(c.maxCity, out);
    return true;
}

// Append-only dictionary: <uint8_t len><bytes>...  (NOT HPS)
static bool loadDictionary(const std::string &path, std::vector<std::string> &entries) {
    std::ifstream in(path, std::ifstream::binary);
    if (!in.good() || !in.is_open()) return false;
    entries.clear();
    while (in.peek() != EOF) {
        uint8_t len = 0;
        in.read(reinterpret_cast<char *>(&len), 1);
        if (!in.good()) break;
        std::string s(len, '\0');
        in.read(&s[0], len);
        if (!in.good()) break;
        entries.push_back(std::move(s));
    }
    return true;
}
static bool saveDictionary(const std::string &path, const std::vector<std::string> &entries) {
    std::ofstream out(path, std::ofstream::binary | std::ofstream::trunc);
    if (!out.good() || !out.is_open()) return false;
    for (const auto &e : entries) {
        const uint8_t len = static_cast<uint8_t>(e.size());
        out.write(reinterpret_cast<const char *>(&len), 1);
        out.write(e.data(), len);
    }
    return true;
}

struct LoginRecord {
    std::vector<char> secretTokenBinary; // SHA224, 28 bytes
    uint32_t account_id_db = 0;
};
static bool loadLogin(const std::string &path, LoginRecord &r) {
    std::ifstream in(path, std::ifstream::binary);
    if (!in.good() || !in.is_open()) return false;
    hps::StreamInputBuffer s(in);
    s >> r.secretTokenBinary;
    s >> r.account_id_db;
    return true;
}
static bool saveLogin(const std::string &path, const LoginRecord &r) {
    std::ofstream out(path, std::ofstream::binary | std::ofstream::trunc);
    if (!out.good() || !out.is_open()) return false;
    hps::to_stream(r.secretTokenBinary, out);
    hps::to_stream(r.account_id_db, out);
    return true;
}

static bool loadAccount(const std::string &path, std::vector<CharacterEntry> &list) {
    std::ifstream in(path, std::ifstream::binary);
    if (!in.good() || !in.is_open()) return false;
    hps::StreamInputBuffer s(in);
    s >> list;
    return true;
}
static bool saveAccount(const std::string &path, const std::vector<CharacterEntry> &list) {
    std::ofstream out(path, std::ofstream::binary | std::ofstream::trunc);
    if (!out.good() || !out.is_open()) return false;
    hps::to_stream(list, out);
    return true;
}

struct ClanRecord {
    std::string name;
    uint64_t cash = 0;
};
static bool loadClan(const std::string &path, ClanRecord &c) {
    std::ifstream in(path, std::ifstream::binary);
    if (!in.good() || !in.is_open()) return false;
    hps::StreamInputBuffer s(in);
    s >> c.name;
    s >> c.cash;
    return true;
}
static bool saveClan(const std::string &path, const ClanRecord &c) {
    std::ofstream out(path, std::ofstream::binary | std::ofstream::trunc);
    if (!out.good() || !out.is_open()) return false;
    hps::to_stream(c.name, out);
    hps::to_stream(c.cash, out);
    return true;
}

static bool loadZone(const std::string &path, uint32_t &clanId) {
    FILE *fp = std::fopen(path.c_str(), "rb");
    if (!fp) return false;
    const size_t n = std::fread(&clanId, sizeof(clanId), 1, fp);
    std::fclose(fp);
    return n == 1;
}
static bool saveZone(const std::string &path, uint32_t clanId) {
    FILE *fp = std::fopen(path.c_str(), "wb");
    if (!fp) return false;
    const size_t n = std::fwrite(&clanId, sizeof(clanId), 1, fp);
    std::fclose(fp);
    return n == 1;
}

// -- Character (non-map) --------------------------------------------------
// Mirrors Client::serialize / Client::parse field by field, but stores raw
// db_ids (no dictionary resolution is needed in a converter). Encyclopedia/
// recipes blobs are kept as opaque std::string; they are sized by
// CommonDatapack in the server but their on-disk representation is just raw
// bytes.

struct CharacterPosition {
    uint32_t map_db_id = 0;
    uint8_t x = 0, y = 0, orientation = 0;
};

struct CharacterCommonRecord {
    uint32_t character_id_db = 0;
    Player_public_informations public_info;
    uint64_t cash = 0;
    std::string recipes;
    std::vector<PlayerMonster> monsters;
    std::vector<PlayerMonster> warehouse_monsters;
    std::string encyclopedia_monster;
    std::string encyclopedia_item;
    uint32_t repel_step = 0;
    bool clan_leader = false;
    uint32_t clan = 0;
    std::map<uint8_t, PlayerReputation> reputation;
    std::map<uint16_t, uint32_t> items;
    bool allowCreateClan = false;
    bool ableToFight = false;
    std::vector<PlayerMonster> wildMonsters;
    std::vector<PlayerMonster> botFightMonsters;
    uint16_t randomIndex = 0;
    uint16_t randomSize = 0;
    uint8_t number_of_character = 0;
    std::unordered_map<uint8_t, std::vector<MonsterDrops>> questsDrop;
    uint64_t connectedSince = 0;
    uint8_t profileIndex = 0;
    CharacterPosition map_entry;
    CharacterPosition rescue;
    CharacterPosition unvalidated_rescue;
    struct CurrentPos {
        uint32_t map_db_id = 0;
        uint8_t x = 0, y = 0, last_direction = 0;
    } current;
};

static bool loadCharacterCommon(const std::string &path, CharacterCommonRecord &c) {
    std::ifstream in(path, std::ifstream::binary);
    if (!in.good() || !in.is_open()) return false;
    hps::StreamInputBuffer s(in);
    s >> c.character_id_db;
    s >> c.public_info;
    s >> c.cash;
    s >> c.recipes;
    s >> c.monsters;
    s >> c.warehouse_monsters;
    s >> c.encyclopedia_monster;
    s >> c.encyclopedia_item;
    s >> c.repel_step;
    s >> c.clan_leader;
    s >> c.clan;
    s >> c.reputation;
    s >> c.items;
    s >> c.allowCreateClan;
    //ableToFight/wildMonsters/botFightMonsters are dynamic CommonFightEngine
    //state, not serialised by Client; mirror that here.
    s >> c.randomIndex >> c.randomSize >> c.number_of_character;
    s >> c.questsDrop >> c.connectedSince >> c.profileIndex;
    s >> c.map_entry.map_db_id >> c.map_entry.x >> c.map_entry.y >> c.map_entry.orientation;
    s >> c.rescue.map_db_id >> c.rescue.x >> c.rescue.y >> c.rescue.orientation;
    s >> c.unvalidated_rescue.map_db_id >> c.unvalidated_rescue.x >> c.unvalidated_rescue.y >> c.unvalidated_rescue.orientation;
    s >> c.current.map_db_id >> c.current.x >> c.current.y >> c.current.last_direction;
    return true;
}

static bool saveCharacterCommon(const std::string &path, const CharacterCommonRecord &c) {
    std::ofstream out(path, std::ofstream::binary | std::ofstream::trunc);
    if (!out.good() || !out.is_open()) return false;
    hps::StreamOutputBuffer s(out);
    s << c.character_id_db;
    s << c.public_info;
    s << c.cash;
    s << c.recipes;
    s << c.monsters;
    s << c.warehouse_monsters;
    s << c.encyclopedia_monster;
    s << c.encyclopedia_item;
    s << c.repel_step;
    s << c.clan_leader;
    s << c.clan;
    s << c.reputation;
    s << c.items;
    s << c.allowCreateClan;
    //ableToFight/wildMonsters/botFightMonsters are dynamic CommonFightEngine
    //state, not serialised by Client; mirror that here.
    s << c.randomIndex << c.randomSize << c.number_of_character;
    s << c.questsDrop << c.connectedSince << c.profileIndex;
    s << c.map_entry.map_db_id << c.map_entry.x << c.map_entry.y << c.map_entry.orientation;
    s << c.rescue.map_db_id << c.rescue.x << c.rescue.y << c.rescue.orientation;
    s << c.unvalidated_rescue.map_db_id << c.unvalidated_rescue.x << c.unvalidated_rescue.y << c.unvalidated_rescue.orientation;
    s << c.current.map_db_id << c.current.x << c.current.y << c.current.last_direction;
    s.flush();
    return true;
}

// -- Character (server, map-linked) --
struct CharacterServerRecord {
    std::vector<std::pair<uint32_t, Player_private_and_public_informations_Map>> mapData;
    std::map<uint8_t, PlayerQuest> quests;
};

static bool loadCharacterServer(const std::string &path, CharacterServerRecord &c) {
    std::ifstream in(path, std::ifstream::binary);
    if (!in.good() || !in.is_open()) return false;
    hps::StreamInputBuffer s(in);
    size_t mapDataCount = 0;
    s >> mapDataCount;
    c.mapData.clear();
    c.mapData.reserve(mapDataCount);
    for (size_t i = 0; i < mapDataCount; i++) {
        uint32_t db_id = 0;
        Player_private_and_public_informations_Map v;
        s >> db_id;
        s >> v;
        c.mapData.emplace_back(db_id, std::move(v));
    }
    s >> c.quests;
    return true;
}

static bool saveCharacterServer(const std::string &path, const CharacterServerRecord &c) {
    std::ofstream out(path, std::ofstream::binary | std::ofstream::trunc);
    if (!out.good() || !out.is_open()) return false;
    hps::StreamOutputBuffer s(out);
    size_t mapDataCount = c.mapData.size();
    s << mapDataCount;
    for (const auto &entry : c.mapData) {
        s << entry.first;
        s << entry.second;
    }
    s << c.quests;
    s.flush();
    return true;
}

// ================================================================ xml I/O ==

// --- Player monster pieces ---
static void xmlWriteBuffs(XMLElement *parent, const std::vector<PlayerBuff> &buffs) {
    for (const auto &b : buffs) {
        auto *e = addChild(parent, "buff");
        sU32(e, "buff", b.buff);
        sU32(e, "level", b.level);
        sU32(e, "remainingNumberOfTurn", b.remainingNumberOfTurn);
    }
}
static void xmlReadBuffs(const XMLElement *parent, std::vector<PlayerBuff> &buffs) {
    for (const auto *e = parent->FirstChildElement("buff"); e; e = e->NextSiblingElement("buff")) {
        PlayerBuff b;
        b.buff = static_cast<uint8_t>(aU32(e, "buff"));
        b.level = static_cast<uint8_t>(aU32(e, "level"));
        b.remainingNumberOfTurn = static_cast<uint8_t>(aU32(e, "remainingNumberOfTurn"));
        buffs.push_back(b);
    }
}

static void xmlWriteSkills(XMLElement *parent, const std::vector<PlayerMonster::PlayerSkill> &skills) {
    for (const auto &sk : skills) {
        auto *e = addChild(parent, "skill");
        sU32(e, "skill", sk.skill);
        sU32(e, "level", sk.level);
        sU32(e, "endurance", sk.endurance);
    }
}
static void xmlReadSkills(const XMLElement *parent, std::vector<PlayerMonster::PlayerSkill> &skills) {
    for (const auto *e = parent->FirstChildElement("skill"); e; e = e->NextSiblingElement("skill")) {
        PlayerMonster::PlayerSkill sk;
        sk.skill = static_cast<uint16_t>(aU32(e, "skill"));
        sk.level = static_cast<uint8_t>(aU32(e, "level"));
        sk.endurance = static_cast<uint8_t>(aU32(e, "endurance"));
        skills.push_back(sk);
    }
}

static void xmlWriteMonsters(XMLElement *parent, const std::vector<PlayerMonster> &monsters, const char *tag) {
    auto *wrap = addChild(parent, tag);
    for (const auto &m : monsters) {
        auto *e = addChild(wrap, "monster");
        sU32(e, "monster", m.monster);
        sU32(e, "level", m.level);
        sU32(e, "hp", m.hp);
        sU32(e, "catched_with", m.catched_with);
        sU32(e, "gender", static_cast<uint8_t>(m.gender));
        sU32(e, "remaining_xp", m.remaining_xp);
        sU32(e, "egg_step", m.egg_step);
        sU32(e, "character_origin", m.character_origin);
        xmlWriteBuffs(addChild(e, "buffs"), m.buffs);
        xmlWriteSkills(addChild(e, "skills"), m.skills);
    }
}
static void xmlReadMonsters(const XMLElement *parent, std::vector<PlayerMonster> &monsters, const char *tag) {
    const auto *wrap = parent->FirstChildElement(tag);
    if (!wrap) return;
    for (const auto *e = wrap->FirstChildElement("monster"); e; e = e->NextSiblingElement("monster")) {
        PlayerMonster m;
        m.monster = static_cast<uint16_t>(aU32(e, "monster"));
        m.level = static_cast<uint8_t>(aU32(e, "level"));
        m.hp = aU32(e, "hp");
        m.catched_with = static_cast<uint16_t>(aU32(e, "catched_with"));
        m.gender = static_cast<Gender>(aU32(e, "gender"));
        m.remaining_xp = aU32(e, "remaining_xp");
        m.egg_step = aU32(e, "egg_step");
        m.character_origin = aU32(e, "character_origin");
        if (const auto *buffs = e->FirstChildElement("buffs")) xmlReadBuffs(buffs, m.buffs);
        if (const auto *skills = e->FirstChildElement("skills")) xmlReadSkills(skills, m.skills);
        monsters.push_back(std::move(m));
    }
}

// --- Player_private_and_public_informations_Map ---
static void xmlWriteMapData(XMLElement *parent,
                            const std::vector<std::pair<uint32_t, Player_private_and_public_informations_Map>> &entries) {
    for (const auto &kv : entries) {
        auto *mp = addChild(parent, "map");
        sU32(mp, "db_id", kv.first);
        const auto &v = kv.second;
        auto *items = addChild(mp, "items");
        for (const auto &p : v.items) {
            auto *it = addChild(items, "item");
            sU32(it, "x", p.first);
            sU32(it, "y", p.second);
        }
        auto *plants = addChild(mp, "plants");
        for (const auto &p : v.plants) {
            auto *pl = addChild(plants, "plant");
            sU32(pl, "x", p.first.first);
            sU32(pl, "y", p.first.second);
            sU32(pl, "plant", p.second.plant);
            sU64(pl, "mature_at", p.second.mature_at);
        }
        auto *bots = addChild(mp, "bots_beaten");
        for (const auto &id : v.bots_beaten) {
            auto *b = addChild(bots, "bot");
            sU32(b, "id", id);
        }
        auto *industries = addChild(mp, "industriesStatus");
        for (const auto &ind : v.industriesStatus) {
            auto *i = addChild(industries, "industry");
            sU64(i, "last_update", ind.last_update);
            auto *res = addChild(i, "resources");
            for (const auto &rq : ind.resources) {
                auto *r = addChild(res, "r");
                sU32(r, "item", rq.first);
                sU32(r, "quantity", rq.second);
            }
            auto *prd = addChild(i, "products");
            for (const auto &pq : ind.products) {
                auto *p = addChild(prd, "p");
                sU32(p, "item", pq.first);
                sU32(p, "quantity", pq.second);
            }
        }
    }
}

static void xmlReadMapData(const XMLElement *parent,
                           std::vector<std::pair<uint32_t, Player_private_and_public_informations_Map>> &entries) {
    for (const auto *mp = parent->FirstChildElement("map"); mp; mp = mp->NextSiblingElement("map")) {
        Player_private_and_public_informations_Map v;
        const uint32_t db_id = aU32(mp, "db_id");
        if (const auto *items = mp->FirstChildElement("items"))
            for (const auto *it = items->FirstChildElement("item"); it; it = it->NextSiblingElement("item"))
                v.items.insert({static_cast<uint8_t>(aU32(it, "x")), static_cast<uint8_t>(aU32(it, "y"))});
        if (const auto *plants = mp->FirstChildElement("plants"))
            for (const auto *pl = plants->FirstChildElement("plant"); pl; pl = pl->NextSiblingElement("plant")) {
                PlayerPlant p;
                p.plant = static_cast<uint8_t>(aU32(pl, "plant"));
                p.mature_at = aU64(pl, "mature_at");
                v.plants[{static_cast<uint8_t>(aU32(pl, "x")),
                          static_cast<uint8_t>(aU32(pl, "y"))}] = p;
            }
        if (const auto *bots = mp->FirstChildElement("bots_beaten"))
            for (const auto *b = bots->FirstChildElement("bot"); b; b = b->NextSiblingElement("bot"))
                v.bots_beaten.insert(static_cast<uint8_t>(aU32(b, "id")));
        if (const auto *industries = mp->FirstChildElement("industriesStatus"))
            for (const auto *i = industries->FirstChildElement("industry"); i; i = i->NextSiblingElement("industry")) {
                IndustryStatus ind;
                ind.last_update = aU64(i, "last_update");
                if (const auto *res = i->FirstChildElement("resources"))
                    for (const auto *r = res->FirstChildElement("r"); r; r = r->NextSiblingElement("r"))
                        ind.resources[static_cast<uint16_t>(aU32(r, "item"))] = aU32(r, "quantity");
                if (const auto *prd = i->FirstChildElement("products"))
                    for (const auto *p = prd->FirstChildElement("p"); p; p = p->NextSiblingElement("p"))
                        ind.products[static_cast<uint16_t>(aU32(p, "item"))] = aU32(p, "quantity");
                v.industriesStatus.push_back(std::move(ind));
            }
        entries.emplace_back(db_id, std::move(v));
    }
}

// ---------- per-kind XML builders / parsers ----------

// ServerCounters
static void toXml(const ServerCounters &c, XMLDocument &doc) {
    auto *e = doc.NewElement("server"); doc.InsertEndChild(e);
    sU32(e, "maxClanId", c.maxClanId);
    sU32(e, "maxAccountId", c.maxAccountId);
    sU32(e, "maxCharacterId", c.maxCharacterId);
    sU32(e, "maxCity", c.maxCity);
}
static bool fromXml(const XMLDocument &doc, ServerCounters &c) {
    const auto *e = doc.RootElement(); if (!e) return false;
    c.maxClanId = aU32(e, "maxClanId");
    c.maxAccountId = aU32(e, "maxAccountId");
    c.maxCharacterId = aU32(e, "maxCharacterId");
    c.maxCity = aU32(e, "maxCity");
    return true;
}

// Dictionary (with root tag)
static void toXmlDict(const std::vector<std::string> &entries, XMLDocument &doc, const char *rootTag) {
    auto *r = doc.NewElement(rootTag); doc.InsertEndChild(r);
    for (size_t i = 0; i < entries.size(); i++) {
        auto *e = addChild(r, "entry");
        sU32(e, "id", static_cast<uint32_t>(i));
        sStr(e, "name", entries[i]);
    }
}
static bool fromXmlDict(const XMLDocument &doc, std::vector<std::string> &entries) {
    const auto *r = doc.RootElement(); if (!r) return false;
    entries.clear();
    for (const auto *e = r->FirstChildElement("entry"); e; e = e->NextSiblingElement("entry"))
        entries.push_back(aStr(e, "name"));
    return true;
}

// Login
static void toXml(const LoginRecord &r, XMLDocument &doc) {
    auto *e = doc.NewElement("login"); doc.InsertEndChild(e);
    auto *t = addChild(e, "secretTokenHex");
    t->SetText(bytesToHex(r.secretTokenBinary).c_str());
    auto *a = addChild(e, "accountId");
    a->SetText(r.account_id_db);
}
static bool fromXml(const XMLDocument &doc, LoginRecord &r) {
    const auto *e = doc.RootElement(); if (!e) return false;
    if (const auto *t = e->FirstChildElement("secretTokenHex"))
        hexToBytes(t->GetText(), r.secretTokenBinary);
    if (const auto *a = e->FirstChildElement("accountId"))
        r.account_id_db = textU32(a);
    return true;
}

// Account (vector<CharacterEntry>)
static void toXml(const std::vector<CharacterEntry> &list, XMLDocument &doc) {
    auto *r = doc.NewElement("account"); doc.InsertEndChild(r);
    for (const auto &c : list) {
        auto *e = addChild(r, "character");
        sU32(e, "id", c.character_id);
        sStr(e, "pseudo", c.pseudo);
        sU32(e, "groupIndex", c.charactersGroupIndex);
        sU32(e, "skinId", c.skinId);
        sU32(e, "deleteTimeLeft", c.delete_time_left);
        sU32(e, "playedTime", c.played_time);
        sU64(e, "lastConnect", c.last_connect);
    }
}
static bool fromXml(const XMLDocument &doc, std::vector<CharacterEntry> &list) {
    const auto *r = doc.RootElement(); if (!r) return false;
    list.clear();
    for (const auto *e = r->FirstChildElement("character"); e; e = e->NextSiblingElement("character")) {
        CharacterEntry c;
        c.character_id = aU32(e, "id");
        c.pseudo = aStr(e, "pseudo");
        c.charactersGroupIndex = static_cast<uint8_t>(aU32(e, "groupIndex"));
        c.skinId = static_cast<uint8_t>(aU32(e, "skinId"));
        c.delete_time_left = aU32(e, "deleteTimeLeft");
        c.played_time = aU32(e, "playedTime");
        c.last_connect = aU64(e, "lastConnect");
        list.push_back(std::move(c));
    }
    return true;
}

// Clan
static void toXml(const ClanRecord &c, XMLDocument &doc) {
    auto *e = doc.NewElement("clan"); doc.InsertEndChild(e);
    auto *n = addChild(e, "name");
    n->SetText(c.name.c_str());
    auto *cash = addChild(e, "cash");
    cash->SetText(std::to_string(c.cash).c_str());
}
static bool fromXml(const XMLDocument &doc, ClanRecord &c) {
    const auto *e = doc.RootElement(); if (!e) return false;
    if (const auto *n = e->FirstChildElement("name")) c.name = textStr(n);
    if (const auto *cash = e->FirstChildElement("cash")) c.cash = textU64(cash);
    return true;
}

// Zone
static void toXmlZone(uint32_t clanId, XMLDocument &doc) {
    auto *e = doc.NewElement("zone"); doc.InsertEndChild(e);
    sU32(e, "clanId", clanId);
}
static bool fromXmlZone(const XMLDocument &doc, uint32_t &clanId) {
    const auto *e = doc.RootElement(); if (!e) return false;
    clanId = aU32(e, "clanId");
    return true;
}

// CharacterCommon
static void toXml(const CharacterCommonRecord &c, XMLDocument &doc) {
    auto *r = doc.NewElement("character"); doc.InsertEndChild(r);
    sU32(r, "id", c.character_id_db);
    auto *pub = addChild(r, "public");
    sStr(pub, "pseudo", c.public_info.pseudo);
    sU32(pub, "type", static_cast<uint8_t>(c.public_info.type));
    sU32(pub, "skinId", c.public_info.skinId);
    sU32(pub, "monsterId", c.public_info.monsterId);
    auto *cash = addChild(r, "cash");
    cash->SetText(std::to_string(c.cash).c_str());
    auto *recipes = addChild(r, "recipes");
    recipes->SetText(bytesToHex(c.recipes).c_str());
    xmlWriteMonsters(r, c.monsters, "monsters");
    xmlWriteMonsters(r, c.warehouse_monsters, "warehouse_monsters");
    auto *em = addChild(r, "encyclopedia_monster");
    em->SetText(bytesToHex(c.encyclopedia_monster).c_str());
    auto *ei = addChild(r, "encyclopedia_item");
    ei->SetText(bytesToHex(c.encyclopedia_item).c_str());
    auto *rs = addChild(r, "repel_step"); rs->SetText(c.repel_step);
    auto *cl = addChild(r, "clan_leader"); cl->SetText(c.clan_leader);
    auto *clid = addChild(r, "clan"); clid->SetText(c.clan);
    auto *rep = addChild(r, "reputation");
    for (const auto &kv : c.reputation) {
        auto *e = addChild(rep, "entry");
        sU32(e, "key", kv.first);
        sI32(e, "level", kv.second.level);
        sI32(e, "point", kv.second.point);
    }
    auto *items = addChild(r, "items");
    for (const auto &kv : c.items) {
        auto *e = addChild(items, "item");
        sU32(e, "id", kv.first);
        sU32(e, "quantity", kv.second);
    }
    auto *acc = addChild(r, "allowCreateClan"); acc->SetText(c.allowCreateClan);
    auto *atf = addChild(r, "ableToFight"); atf->SetText(c.ableToFight);
    xmlWriteMonsters(r, c.wildMonsters, "wildMonsters");
    xmlWriteMonsters(r, c.botFightMonsters, "botFightMonsters");
    auto *ri = addChild(r, "randomIndex"); ri->SetText(c.randomIndex);
    auto *rsz = addChild(r, "randomSize"); rsz->SetText(c.randomSize);
    auto *nc = addChild(r, "number_of_character"); nc->SetText(c.number_of_character);
    auto *qd = addChild(r, "questsDrop");
    for (const auto &kv : c.questsDrop) {
        auto *q = addChild(qd, "questDrops");
        sU32(q, "questId", kv.first);
        for (const auto &d : kv.second) {
            auto *dr = addChild(q, "drop");
            sU32(dr, "item", d.item);
            sU32(dr, "quantity_min", d.quantity_min);
            sU32(dr, "quantity_max", d.quantity_max);
            sU32(dr, "luck", d.luck);
        }
    }
    auto *cs = addChild(r, "connectedSince");
    cs->SetText(std::to_string(c.connectedSince).c_str());
    auto *pi = addChild(r, "profileIndex"); pi->SetText(c.profileIndex);
    auto *pos = addChild(r, "position");
    auto writePos = [&](const CharacterPosition &p, const char *tag) {
        auto *e = addChild(pos, tag);
        sU32(e, "db_id", p.map_db_id);
        sU32(e, "x", p.x);
        sU32(e, "y", p.y);
        sU32(e, "orientation", p.orientation);
    };
    writePos(c.map_entry, "map_entry");
    writePos(c.rescue, "rescue");
    writePos(c.unvalidated_rescue, "unvalidated_rescue");
    auto *cur = addChild(pos, "current");
    sU32(cur, "db_id", c.current.map_db_id);
    sU32(cur, "x", c.current.x);
    sU32(cur, "y", c.current.y);
    sU32(cur, "last_direction", c.current.last_direction);
}

static bool fromXml(const XMLDocument &doc, CharacterCommonRecord &c) {
    const auto *r = doc.RootElement(); if (!r) return false;
    c.character_id_db = aU32(r, "id");
    if (const auto *pub = r->FirstChildElement("public")) {
        c.public_info.pseudo = aStr(pub, "pseudo");
        c.public_info.type = static_cast<Player_type>(aU32(pub, "type"));
        c.public_info.skinId = static_cast<uint8_t>(aU32(pub, "skinId"));
        c.public_info.monsterId = static_cast<uint16_t>(aU32(pub, "monsterId"));
    }
    if (const auto *e = r->FirstChildElement("cash")) c.cash = textU64(e);
    if (const auto *e = r->FirstChildElement("recipes")) hexToBytes(e->GetText(), c.recipes);
    xmlReadMonsters(r, c.monsters, "monsters");
    xmlReadMonsters(r, c.warehouse_monsters, "warehouse_monsters");
    if (const auto *e = r->FirstChildElement("encyclopedia_monster")) hexToBytes(e->GetText(), c.encyclopedia_monster);
    if (const auto *e = r->FirstChildElement("encyclopedia_item")) hexToBytes(e->GetText(), c.encyclopedia_item);
    if (const auto *e = r->FirstChildElement("repel_step")) c.repel_step = textU32(e);
    if (const auto *e = r->FirstChildElement("clan_leader")) c.clan_leader = textBool(e);
    if (const auto *e = r->FirstChildElement("clan")) c.clan = textU32(e);
    if (const auto *rep = r->FirstChildElement("reputation"))
        for (const auto *e = rep->FirstChildElement("entry"); e; e = e->NextSiblingElement("entry")) {
            PlayerReputation pr;
            pr.level = static_cast<int8_t>(aI32(e, "level"));
            pr.point = aI32(e, "point");
            c.reputation[static_cast<uint8_t>(aU32(e, "key"))] = pr;
        }
    if (const auto *items = r->FirstChildElement("items"))
        for (const auto *e = items->FirstChildElement("item"); e; e = e->NextSiblingElement("item"))
            c.items[static_cast<uint16_t>(aU32(e, "id"))] = aU32(e, "quantity");
    if (const auto *e = r->FirstChildElement("allowCreateClan")) c.allowCreateClan = textBool(e);
    if (const auto *e = r->FirstChildElement("ableToFight")) c.ableToFight = textBool(e);
    xmlReadMonsters(r, c.wildMonsters, "wildMonsters");
    xmlReadMonsters(r, c.botFightMonsters, "botFightMonsters");
    if (const auto *e = r->FirstChildElement("randomIndex")) c.randomIndex = static_cast<uint16_t>(textU32(e));
    if (const auto *e = r->FirstChildElement("randomSize")) c.randomSize = static_cast<uint16_t>(textU32(e));
    if (const auto *e = r->FirstChildElement("number_of_character")) c.number_of_character = static_cast<uint8_t>(textU32(e));
    if (const auto *qd = r->FirstChildElement("questsDrop"))
        for (const auto *q = qd->FirstChildElement("questDrops"); q; q = q->NextSiblingElement("questDrops")) {
            const uint8_t qid = static_cast<uint8_t>(aU32(q, "questId"));
            auto &vec = c.questsDrop[qid];
            for (const auto *dr = q->FirstChildElement("drop"); dr; dr = dr->NextSiblingElement("drop")) {
                MonsterDrops d;
                d.item = static_cast<uint16_t>(aU32(dr, "item"));
                d.quantity_min = aU32(dr, "quantity_min");
                d.quantity_max = aU32(dr, "quantity_max");
                d.luck = static_cast<uint8_t>(aU32(dr, "luck"));
                vec.push_back(d);
            }
        }
    if (const auto *e = r->FirstChildElement("connectedSince")) c.connectedSince = textU64(e);
    if (const auto *e = r->FirstChildElement("profileIndex")) c.profileIndex = static_cast<uint8_t>(textU32(e));
    if (const auto *pos = r->FirstChildElement("position")) {
        auto readPos = [&](const XMLElement *e, CharacterPosition &p) {
            p.map_db_id = aU32(e, "db_id");
            p.x = static_cast<uint8_t>(aU32(e, "x"));
            p.y = static_cast<uint8_t>(aU32(e, "y"));
            p.orientation = static_cast<uint8_t>(aU32(e, "orientation"));
        };
        if (const auto *e = pos->FirstChildElement("map_entry")) readPos(e, c.map_entry);
        if (const auto *e = pos->FirstChildElement("rescue")) readPos(e, c.rescue);
        if (const auto *e = pos->FirstChildElement("unvalidated_rescue")) readPos(e, c.unvalidated_rescue);
        if (const auto *e = pos->FirstChildElement("current")) {
            c.current.map_db_id = aU32(e, "db_id");
            c.current.x = static_cast<uint8_t>(aU32(e, "x"));
            c.current.y = static_cast<uint8_t>(aU32(e, "y"));
            c.current.last_direction = static_cast<uint8_t>(aU32(e, "last_direction"));
        }
    }
    return true;
}

// CharacterServer
static void toXml(const CharacterServerRecord &c, XMLDocument &doc) {
    auto *r = doc.NewElement("character_server"); doc.InsertEndChild(r);
    auto *md = addChild(r, "mapData");
    xmlWriteMapData(md, c.mapData);
    auto *qs = addChild(r, "quests");
    for (const auto &kv : c.quests) {
        auto *q = addChild(qs, "quest");
        sU32(q, "id", kv.first);
        sU32(q, "step", kv.second.step);
        sBool(q, "finish_one_time", kv.second.finish_one_time);
    }
}
static bool fromXml(const XMLDocument &doc, CharacterServerRecord &c) {
    const auto *r = doc.RootElement(); if (!r) return false;
    if (const auto *md = r->FirstChildElement("mapData")) xmlReadMapData(md, c.mapData);
    if (const auto *qs = r->FirstChildElement("quests"))
        for (const auto *q = qs->FirstChildElement("quest"); q; q = q->NextSiblingElement("quest")) {
            PlayerQuest pq;
            pq.step = static_cast<uint8_t>(aU32(q, "step"));
            pq.finish_one_time = aBool(q, "finish_one_time");
            c.quests[static_cast<uint8_t>(aU32(q, "id"))] = pq;
        }
    return true;
}

// =============================================================== dispatch ==

static bool saveXmlDoc(XMLDocument &doc, const std::string &path) {
    return doc.SaveFile(path.c_str()) == tinyxml2::XML_SUCCESS;
}
static bool loadXmlDoc(XMLDocument &doc, const std::string &path) {
    return doc.LoadFile(path.c_str()) == tinyxml2::XML_SUCCESS;
}

static std::string xmlPathFor(const std::string &binPath) { return binPath + ".xml"; }
static std::string binPathFor(const std::string &xmlPath) {
    if (endsWith(xmlPath, ".xml")) return xmlPath.substr(0, xmlPath.size() - 4);
    return xmlPath + ".bin";
}

int convertFile(const std::string &inputPath, Direction dir) {
    const bool b2x = (dir == Direction::BinToXml);
    const std::string binPath = b2x ? inputPath : binPathFor(inputPath);
    const std::string xmlPath = b2x ? xmlPathFor(inputPath) : inputPath;
    const FileKind kind = detectKind(binPath);
    if (kind == FileKind::Unknown) {
        std::cerr << "[skip] unknown file kind: " << binPath << std::endl;
        return 1;
    }

    std::cerr << (b2x ? "[b2x] " : "[x2b] ") << kindName(kind) << " "
              << (b2x ? binPath : xmlPath) << " -> "
              << (b2x ? xmlPath : binPath) << std::endl;

    auto fail = [&](const char *what) {
        std::cerr << "  " << what << " failed" << std::endl;
        return 1;
    };

    // Handle each kind. Kept verbose (rather than macro-based) so compiler
    // errors are easy to follow.
    switch (kind) {
        case FileKind::ServerCounters: {
            if (b2x) {
                ServerCounters c; if (!loadServerCounters(binPath, c)) return fail("read");
                XMLDocument doc; toXml(c, doc);
                return saveXmlDoc(doc, xmlPath) ? 0 : fail("write-xml");
            } else {
                XMLDocument doc; if (!loadXmlDoc(doc, xmlPath)) return fail("load-xml");
                ServerCounters c; if (!fromXml(doc, c)) return fail("parse-xml");
                return saveServerCounters(binPath, c) ? 0 : fail("write-bin");
            }
        }
        case FileKind::DictionaryMap:
        case FileKind::DictionaryReputation:
        case FileKind::DictionarySkin:
        case FileKind::DictionaryStarter: {
            const char *tag =
                kind == FileKind::DictionaryMap ? "dictionary_map" :
                kind == FileKind::DictionaryReputation ? "dictionary_reputation" :
                kind == FileKind::DictionarySkin ? "dictionary_skin" : "dictionary_starter";
            if (b2x) {
                std::vector<std::string> v; if (!loadDictionary(binPath, v)) return fail("read");
                XMLDocument doc; toXmlDict(v, doc, tag);
                return saveXmlDoc(doc, xmlPath) ? 0 : fail("write-xml");
            } else {
                XMLDocument doc; if (!loadXmlDoc(doc, xmlPath)) return fail("load-xml");
                std::vector<std::string> v; if (!fromXmlDict(doc, v)) return fail("parse-xml");
                return saveDictionary(binPath, v) ? 0 : fail("write-bin");
            }
        }
        case FileKind::Login: {
            if (b2x) {
                LoginRecord r; if (!loadLogin(binPath, r)) return fail("read");
                XMLDocument doc; toXml(r, doc);
                return saveXmlDoc(doc, xmlPath) ? 0 : fail("write-xml");
            } else {
                XMLDocument doc; if (!loadXmlDoc(doc, xmlPath)) return fail("load-xml");
                LoginRecord r; if (!fromXml(doc, r)) return fail("parse-xml");
                return saveLogin(binPath, r) ? 0 : fail("write-bin");
            }
        }
        case FileKind::Account: {
            if (b2x) {
                std::vector<CharacterEntry> list; if (!loadAccount(binPath, list)) return fail("read");
                XMLDocument doc; toXml(list, doc);
                return saveXmlDoc(doc, xmlPath) ? 0 : fail("write-xml");
            } else {
                XMLDocument doc; if (!loadXmlDoc(doc, xmlPath)) return fail("load-xml");
                std::vector<CharacterEntry> list; if (!fromXml(doc, list)) return fail("parse-xml");
                return saveAccount(binPath, list) ? 0 : fail("write-bin");
            }
        }
        case FileKind::Clan: {
            if (b2x) {
                ClanRecord c; if (!loadClan(binPath, c)) return fail("read");
                XMLDocument doc; toXml(c, doc);
                return saveXmlDoc(doc, xmlPath) ? 0 : fail("write-xml");
            } else {
                XMLDocument doc; if (!loadXmlDoc(doc, xmlPath)) return fail("load-xml");
                ClanRecord c; if (!fromXml(doc, c)) return fail("parse-xml");
                return saveClan(binPath, c) ? 0 : fail("write-bin");
            }
        }
        case FileKind::Zone: {
            if (b2x) {
                uint32_t id = 0; if (!loadZone(binPath, id)) return fail("read");
                XMLDocument doc; toXmlZone(id, doc);
                return saveXmlDoc(doc, xmlPath) ? 0 : fail("write-xml");
            } else {
                XMLDocument doc; if (!loadXmlDoc(doc, xmlPath)) return fail("load-xml");
                uint32_t id = 0; if (!fromXmlZone(doc, id)) return fail("parse-xml");
                return saveZone(binPath, id) ? 0 : fail("write-bin");
            }
        }
        case FileKind::CharacterCommon: {
            if (b2x) {
                CharacterCommonRecord c; if (!loadCharacterCommon(binPath, c)) return fail("read");
                XMLDocument doc; toXml(c, doc);
                return saveXmlDoc(doc, xmlPath) ? 0 : fail("write-xml");
            } else {
                XMLDocument doc; if (!loadXmlDoc(doc, xmlPath)) return fail("load-xml");
                CharacterCommonRecord c; if (!fromXml(doc, c)) return fail("parse-xml");
                return saveCharacterCommon(binPath, c) ? 0 : fail("write-bin");
            }
        }
        case FileKind::CharacterServer: {
            if (b2x) {
                CharacterServerRecord c; if (!loadCharacterServer(binPath, c)) return fail("read");
                XMLDocument doc; toXml(c, doc);
                return saveXmlDoc(doc, xmlPath) ? 0 : fail("write-xml");
            } else {
                XMLDocument doc; if (!loadXmlDoc(doc, xmlPath)) return fail("load-xml");
                CharacterServerRecord c; if (!fromXml(doc, c)) return fail("parse-xml");
                return saveCharacterServer(binPath, c) ? 0 : fail("write-bin");
            }
        }
        case FileKind::Unknown:
            return 1;
    }
    return 1;
}

// Directory walk
static void walk(const std::string &dir, std::vector<std::string> &out) {
    DIR *d = ::opendir(dir.c_str());
    if (!d) return;
    while (auto *ent = ::readdir(d)) {
        const std::string n = ent->d_name;
        if (n == "." || n == "..") continue;
        const std::string full = dir + "/" + n;
        if (isDir(full)) walk(full, out);
        else if (isFile(full)) out.push_back(full);
    }
    ::closedir(d);
}

int convertDirectory(const std::string &dir, Direction d) {
    if (!isDir(dir)) { std::cerr << "not a directory: " << dir << std::endl; return 1; }
    std::vector<std::string> files; walk(dir, files);
    int failures = 0;
    for (const auto &f : files) {
        const bool isXml = endsWith(f, ".xml");
        if (d == Direction::BinToXml) {
            if (isXml) continue;
            if (detectKind(f) == FileKind::Unknown) continue;
            if (convertFile(f, d) != 0) failures++;
        } else {
            if (!isXml) continue;
            if (convertFile(f, d) != 0) failures++;
        }
    }
    return failures == 0 ? 0 : 1;
}

} // namespace FiledbConverter
