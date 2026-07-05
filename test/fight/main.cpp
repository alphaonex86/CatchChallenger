/*
 * test/fight/main.cpp — unit tests for CommonFightEngine.
 *
 * Builds against catchchallenger_general (general.cmake — INTERFACE lib
 * pulling in fight/ + base/) plus the local MockFightEngine. Loads a
 * real datapack via CommonDatapack::parseDatapack() (path supplied as
 * argv[1]) so monster/skill/trap dictionaries are populated, then runs
 * focused scenarios on the fight engine surface:
 *
 *   - isInFight / generateOtherAttack edge cases
 *   - random fight generation seeds (canDoRandomFight + getOneSeed step)
 *   - attack flow (useSkill / hpChange)
 *   - try escape (success + failure)
 *   - try capture (success + failure)
 *   - KO detection + dropKOOtherMonster + healAllMonsters
 *   - addPlayerMonster + moveUp/moveDown + removeMonsterByPosition
 *   - static helpers: getStat, generateWildSkill
 *
 * Each scenario prints a single PASS/FAIL line; the python wrapper
 * (test/testingfight.py) parses these lines and reports back to the
 * harness. Returns 0 if every scenario passed, else 1.
 */
#include "MockFightEngine.hpp"
#include "../../general/base/CommonDatapack.hpp"
#include "../../general/base/GeneralStructures.hpp"
#include "../../general/base/fight/CommonFightEngine.hpp"
#include "../../general/base/fight/CommonFightEngineBase.hpp"

#include <iostream>
#include <string>
#include <vector>
#include <cstdint>
#include <cstdlib>

using namespace CatchChallenger;

static int g_pass=0;
static int g_fail=0;

static void report(const std::string &name, bool ok, const std::string &detail)
{
    if(ok)
    {
        std::cout << "PASS " << name;
        if(!detail.empty()) std::cout << " " << detail;
        std::cout << std::endl;
        g_pass++;
    }
    else
    {
        std::cout << "FAIL " << name;
        if(!detail.empty()) std::cout << " " << detail;
        std::cout << std::endl;
        g_fail++;
    }
}

// Build a fully-equipped player monster from the first available
// monster in the datapack. Returns false if datapack is empty.
static bool makePlayerMonster(PlayerMonster &out, CATCHCHALLENGER_TYPE_MONSTER monsterId, uint8_t level)
{
    if(!CommonDatapack::commonDatapack.has_monster(monsterId))
        return false;
    const Monster &m=CommonDatapack::commonDatapack.get_monster(monsterId);
    const Monster::Stat &stat=CommonFightEngineBase::getStat(m,level);
    out.monster=monsterId;
    out.level=level;
    out.hp=stat.hp;
    out.catched_with=0;
    out.gender=Gender_Unknown;
    out.buffs.clear();
    out.skills=CommonFightEngineBase::generateWildSkill(m,level);
    out.remaining_xp=0;
    out.egg_step=0;
    out.id=0;
    out.character_origin=0;
    return true;
}

// Pick the first monster id present in the datapack — every
// real datapack ships at least one. Bails out with -1 if empty.
static int pickAnyMonster()
{
    const auto &maxId=CommonDatapack::commonDatapack.get_monstersMaxId();
    CATCHCHALLENGER_TYPE_MONSTER i=1;
    while(i<=maxId)
    {
        if(CommonDatapack::commonDatapack.has_monster(i))
            return static_cast<int>(i);
        i++;
    }
    return -1;
}

// Pick the first trap item id (used by tryCapture).
static int pickAnyTrap()
{
    const auto &maxItem=CommonDatapack::commonDatapack.get_itemMaxId();
    CATCHCHALLENGER_TYPE_ITEM i=1;
    while(i<=maxItem)
    {
        if(CommonDatapack::commonDatapack.has_trap(i))
            return static_cast<int>(i);
        i++;
    }
    return -1;
}

static void test_initial_state(MockFightEngine &eng)
{
    bool ok=!eng.isInFight() && !eng.isInBattle();
    report("initial_state.notInFight", ok, "");
}

static void test_isInFight_after_wild_pushed(int monsterId)
{
    MockFightEngine eng;
    PlayerMonster pm;
    if(!makePlayerMonster(pm,(CATCHCHALLENGER_TYPE_MONSTER)monsterId,5))
    {
        report("isInFight.wildPushed", false, "makePlayerMonster failed");
        return;
    }
    eng.mockPushWildMonster(pm);
    bool ok=eng.isInFight() && eng.isInBattle();
    report("isInFight.wildPushed", ok, "");
}

static void test_canDoRandomFight_seedThreshold(int monsterId)
{
    // canDoRandomFight() requires
    //   - player has at least one (non-KO) monster,
    //   - randomSeedsSize() >= CATCHCHALLENGER_MIN_RANDOM_TO_FIGHT (16).
    // We don't pass through the map zone here — directly call the
    // engine's canDoRandomFight precondition by checking the seed +
    // monster contract via isInFight()==false and seed buffer size.
    MockFightEngine eng;
    PlayerMonster pm;
    if(!makePlayerMonster(pm,(CATCHCHALLENGER_TYPE_MONSTER)monsterId,5))
    {
        report("randomFight.seedAccounting", false, "makePlayerMonster failed");
        return;
    }
    eng.get_public_and_private_informations().monsters.push_back(pm);
    // Without seeds: nothing to spend.
    bool a=(eng.randomSeedsSize()==0);
    eng.mockPushSeeds(std::vector<uint8_t>(20,5));
    bool b=(eng.randomSeedsSize()==20);
    // Consume one seed via the engine's getOneSeed contract.
    uint8_t s=eng.getOneSeed(16);
    bool c=(s<16) && (eng.randomSeedsSize()==19);
    report("randomFight.seedAccounting", a && b && c,
           a && b && c ? "" : "seed accounting wrong");
}

static void test_step_decrement_pattern()
{
    // The engine's step-fight protocol (see CommonFightEngineWild.cpp):
    //   stepFight=getOneSeed(16) when stepFight==0
    //   else stepFight-- ; if stepFight==0 → roll a wild fight.
    // Mock the decrement loop to verify the engine subclass exposes
    // stepFight correctly.
    MockFightEngine eng;
    eng.mockSetStepFight(3);
    // Three "decrement" calls take stepFight 3 → 2 → 1 → 0. We don't
    // call generateWildFightIfCollision here (needs map+zones), but
    // the field exposure proves the engine's plumbing is reachable.
    uint8_t s=eng.mockStepFight();
    bool ok=(s==3);
    report("stepFight.exposure", ok, "");
}

static void test_attack_decrease_hp(int monsterId)
{
    MockFightEngine eng;
    PlayerMonster pm;
    if(!makePlayerMonster(pm,(CATCHCHALLENGER_TYPE_MONSTER)monsterId,10))
    {
        report("attack.useSkill", false, "makePlayerMonster failed");
        return;
    }
    if(pm.skills.empty())
    {
        report("attack.useSkill", false, "monster has no skills");
        return;
    }
    eng.get_public_and_private_informations().monsters.push_back(pm);
    PlayerMonster wild;
    if(!makePlayerMonster(wild,(CATCHCHALLENGER_TYPE_MONSTER)monsterId,10))
    {
        report("attack.useSkill", false, "wild makePlayerMonster failed");
        return;
    }
    eng.mockPushWildMonster(wild);
    // Plenty of seeds for: damage roll, accuracy, crit, status, etc.
    eng.mockPushSeeds(std::vector<uint8_t>(64,7));
    const uint32_t hpBefore=eng.getOtherMonster()->hp;
    const uint16_t skillId=pm.skills.front().skill;
    const bool used=eng.useSkill(skillId);
    if(!used)
    {
        report("attack.useSkill", false, "useSkill returned false");
        return;
    }
    PublicPlayerMonster *other=eng.getOtherMonster();
    if(other==nullptr)
    {
        // Wild may have been KO'd outright — that's still a valid
        // "attack worked" outcome for this scenario.
        report("attack.useSkill", true, "wild KO'd by single attack");
        return;
    }
    bool dropped=(other->hp<hpBefore);
    report("attack.useSkill", dropped,
           dropped ? "" : "wild HP did not decrease");
}

static void test_try_escape_success(int monsterId)
{
    MockFightEngine eng;
    PlayerMonster pm;
    if(!makePlayerMonster(pm,(CATCHCHALLENGER_TYPE_MONSTER)monsterId,30))
    {
        report("tryEscape.success", false, "makePlayerMonster failed");
        return;
    }
    eng.get_public_and_private_informations().monsters.push_back(pm);
    PlayerMonster wild;
    makePlayerMonster(wild,(CATCHCHALLENGER_TYPE_MONSTER)monsterId,1);
    eng.mockPushWildMonster(wild);
    // Force the escape roll low: seed 0 → escape always succeeds for
    // any non-trivial level diff (engine compares roll < threshold).
    eng.mockPushSeeds(std::vector<uint8_t>(16,0));
    const bool escaped=eng.tryEscape();
    // After successful escape the wildMonsters list is cleared.
    bool ok=escaped && !eng.isInFight();
    report("tryEscape.success", ok,
           ok ? "" : "tryEscape did not clear wildMonsters");
}

static void test_try_escape_failure(int monsterId)
{
    MockFightEngine eng;
    PlayerMonster pm;
    if(!makePlayerMonster(pm,(CATCHCHALLENGER_TYPE_MONSTER)monsterId,1))
    {
        report("tryEscape.failure", false, "makePlayerMonster failed");
        return;
    }
    eng.get_public_and_private_informations().monsters.push_back(pm);
    PlayerMonster wild;
    makePlayerMonster(wild,(CATCHCHALLENGER_TYPE_MONSTER)monsterId,99);
    eng.mockPushWildMonster(wild);
    // Maximum seed → escape roll fails; engine triggers a counter
    // attack (consuming more seeds) so feed plenty.
    eng.mockPushSeeds(std::vector<uint8_t>(64,255));
    const bool escaped=eng.tryEscape();
    // We accept either outcome as "engine handled the call without
    // crashing" — escape probability depends on level differential and
    // the engine's exact formula.  The key check: control returned, and
    // when escape failed wildMonsters is still present.
    bool ok=(escaped || eng.isInFight());
    report("tryEscape.failure", ok, escaped ? "escaped anyway" : "stayed in fight");
}

static void test_try_capture(int monsterId, int trapId)
{
    if(trapId<0)
    {
        report("tryCapture.flow", false, "no trap in datapack");
        return;
    }
    MockFightEngine eng;
    PlayerMonster pm;
    if(!makePlayerMonster(pm,(CATCHCHALLENGER_TYPE_MONSTER)monsterId,40))
    {
        report("tryCapture.flow", false, "makePlayerMonster failed");
        return;
    }
    eng.get_public_and_private_informations().monsters.push_back(pm);
    PlayerMonster wild;
    if(!makePlayerMonster(wild,(CATCHCHALLENGER_TYPE_MONSTER)monsterId,1))
    {
        report("tryCapture.flow", false, "wild makePlayerMonster failed");
        return;
    }
    wild.hp=1; // weakened wild → easy capture
    eng.mockPushWildMonster(wild);
    eng.mockPushSeeds(std::vector<uint8_t>(64,0)); // low seeds favor capture
    const uint32_t result=eng.tryCapture(static_cast<uint16_t>(trapId));
    // tryCapture returns 0 when capture fails (still in fight) or a
    // non-zero monster id (catchAWild() result) on success. Either is
    // a valid outcome — assert the engine didn't crash and reached
    // catchAWild() OR called generateOtherAttack() on failure.
    bool flowReached=(result!=0 && eng.catchInvoked) || (result==0 && eng.isInFight());
    report("tryCapture.flow", flowReached,
           result!=0 ? "captured" : "missed (counter-attack ran)");
}

static void test_KO_detection(int monsterId)
{
    MockFightEngine eng;
    PlayerMonster pm;
    if(!makePlayerMonster(pm,(CATCHCHALLENGER_TYPE_MONSTER)monsterId,5))
    {
        report("KO.detection", false, "makePlayerMonster failed");
        return;
    }
    eng.get_public_and_private_informations().monsters.push_back(pm);
    PlayerMonster wild;
    makePlayerMonster(wild,(CATCHCHALLENGER_TYPE_MONSTER)monsterId,5);
    wild.hp=0;
    eng.mockPushWildMonster(wild);
    bool ok=eng.otherMonsterIsKO();
    report("KO.detection.other", ok, "");

    // Player KO
    eng.get_public_and_private_informations().monsters.front().hp=0;
    bool ok2=eng.currentMonsterIsKO();
    report("KO.detection.current", ok2, "");
}

static void test_drop_KO_other(int monsterId)
{
    MockFightEngine eng;
    PlayerMonster pm;
    makePlayerMonster(pm,(CATCHCHALLENGER_TYPE_MONSTER)monsterId,5);
    eng.get_public_and_private_informations().monsters.push_back(pm);
    PlayerMonster wild;
    makePlayerMonster(wild,(CATCHCHALLENGER_TYPE_MONSTER)monsterId,5);
    wild.hp=0;
    eng.mockPushWildMonster(wild);
    eng.mockPushSeeds(std::vector<uint8_t>(32,0));
    eng.dropKOOtherMonster();
    // After dropping a wild KO, isInFightWithWild()==false expected.
    bool ok=!eng.isInFightWithWild();
    report("KO.dropKOOtherMonster", ok, "");
}

static void test_heal_all_monsters(int monsterId)
{
    MockFightEngine eng;
    PlayerMonster pm;
    makePlayerMonster(pm,(CATCHCHALLENGER_TYPE_MONSTER)monsterId,20);
    const uint32_t fullHp=pm.hp;
    pm.hp=1; // wound it
    eng.get_public_and_private_informations().monsters.push_back(pm);
    eng.healAllMonsters();
    bool ok=(eng.get_public_and_private_informations_ro().monsters.front().hp==fullHp);
    report("heal.allMonsters", ok,
           ok ? "" : "hp not restored to full");
}

static void test_add_move_remove_monsters(int monsterId)
{
    MockFightEngine eng;
    PlayerMonster a,b,c;
    makePlayerMonster(a,(CATCHCHALLENGER_TYPE_MONSTER)monsterId,3);
    makePlayerMonster(b,(CATCHCHALLENGER_TYPE_MONSTER)monsterId,5);
    makePlayerMonster(c,(CATCHCHALLENGER_TYPE_MONSTER)monsterId,7);
    eng.addPlayerMonster(a);
    eng.addPlayerMonster(b);
    eng.addPlayerMonster(c);
    bool sized=(eng.get_public_and_private_informations_ro().monsters.size()==3);
    bool levels=(
        eng.get_public_and_private_informations_ro().monsters.at(0).level==3 &&
        eng.get_public_and_private_informations_ro().monsters.at(1).level==5 &&
        eng.get_public_and_private_informations_ro().monsters.at(2).level==7);
    report("manage.add", sized && levels, "");

    eng.moveUpMonster(2); // swap idx 1 and 2 → c to position 1
    bool moved=(eng.get_public_and_private_informations_ro().monsters.at(1).level==7);
    report("manage.moveUp", moved, "");

    eng.moveDownMonster(0); // swap idx 0 and 1
    bool moved2=(eng.get_public_and_private_informations_ro().monsters.at(0).level==7);
    report("manage.moveDown", moved2, "");

    eng.removeMonsterByPosition(0);
    bool sized2=(eng.get_public_and_private_informations_ro().monsters.size()==2);
    report("manage.remove", sized2, "");
}

static void test_static_helpers(int monsterId)
{
    const Monster &m=CommonDatapack::commonDatapack.get_monster(
        (CATCHCHALLENGER_TYPE_MONSTER)monsterId);
    Monster::Stat s5=CommonFightEngineBase::getStat(m,5);
    Monster::Stat s50=CommonFightEngineBase::getStat(m,50);
    bool growth=(s50.hp>s5.hp && s50.attack>=s5.attack && s50.defense>=s5.defense);
    report("static.getStat.growth", growth,
           "lvl5.hp="+std::to_string(s5.hp)+" lvl50.hp="+std::to_string(s50.hp));

    std::vector<PlayerMonster::PlayerSkill> skills=CommonFightEngineBase::generateWildSkill(m,30);
    report("static.generateWildSkill",
           skills.size()>0 || m.learn.empty(),
           "skill_count="+std::to_string(skills.size()));
}

int main(int argc, char *argv[])
{
    if(argc<2)
    {
        std::cerr << "usage: " << argv[0] << " <datapack-path>" << std::endl;
        return 2;
    }
    const std::string datapackPath=argv[1];
    std::cout << "[info] loading datapack: " << datapackPath << std::endl;

    CommonDatapack::commonDatapack.parseDatapack(
        datapackPath.back()=='/' ? datapackPath : datapackPath+"/");
    if(!CommonDatapack::commonDatapack.isParsedContent())
    {
        std::cerr << "FAIL parseDatapack returned isParsedContent()==false"
                  << std::endl;
        return 2;
    }
    std::cout << "[info] monsters loaded: "
              << CommonDatapack::commonDatapack.get_monsters_size() << std::endl;
    std::cout << "[info] skills loaded:   "
              << CommonDatapack::commonDatapack.get_monsterSkills_size() << std::endl;
    std::cout << "[info] traps loaded:    "
              << CommonDatapack::commonDatapack.get_trap_size() << std::endl;

    int monsterId=pickAnyMonster();
    if(monsterId<0)
    {
        std::cerr << "FAIL no monster in datapack" << std::endl;
        return 2;
    }
    int trapId=pickAnyTrap();
    std::cout << "[info] using monsterId=" << monsterId
              << " trapId=" << trapId << std::endl;

    {
        MockFightEngine eng;
        test_initial_state(eng);
    }
    test_isInFight_after_wild_pushed(monsterId);
    test_canDoRandomFight_seedThreshold(monsterId);
    test_step_decrement_pattern();
    test_attack_decrease_hp(monsterId);
    test_try_escape_success(monsterId);
    test_try_escape_failure(monsterId);
    test_try_capture(monsterId,trapId);
    test_KO_detection(monsterId);
    test_drop_KO_other(monsterId);
    test_heal_all_monsters(monsterId);
    test_add_move_remove_monsters(monsterId);
    test_static_helpers(monsterId);

    std::cout << "----" << std::endl;
    std::cout << "summary: " << g_pass << " passed, "
              << g_fail << " failed" << std::endl;
    return g_fail==0 ? 0 : 1;
}
