#include "Generator.hpp"
#include "GeneratorItems.hpp"
#include "GeneratorMonsters.hpp"
#include "GeneratorPlants.hpp"
#include "GeneratorSkills.hpp"
#include "GeneratorTypes.hpp"
#include "GeneratorBuffs.hpp"
#include "GeneratorCrafting.hpp"
#include "GeneratorIndustries.hpp"
#include "GeneratorMaps.hpp"
#include "GeneratorQuests.hpp"
#include "GeneratorIndex.hpp"
#include "GeneratorStart.hpp"
#include "GeneratorTree.hpp"
#include "Helper.hpp"
#include "MapStore.hpp"
#include "../../general/base/CommonDatapack.hpp"

#include <iostream>
#include <chrono>
#include <fstream>

namespace Generator {

static void timedGenerate(const char *label, void(*fn)())
{
    std::cout << "Generating " << label << "..." << std::flush;
    auto t0=std::chrono::steady_clock::now();
    fn();
    auto t1=std::chrono::steady_clock::now();
    long long ms=std::chrono::duration_cast<std::chrono::milliseconds>(t1-t0).count();
    std::cout << " " << ms << "ms" << std::endl;
}

void generateAll()
{
    auto totalStart=std::chrono::steady_clock::now();
    timedGenerate("items", GeneratorItems::generate);
    timedGenerate("monsters", GeneratorMonsters::generate);
    timedGenerate("plants", GeneratorPlants::generate);
    timedGenerate("skills", GeneratorSkills::generate);
    timedGenerate("types", GeneratorTypes::generate);
    timedGenerate("buffs", GeneratorBuffs::generate);
    timedGenerate("crafting recipes", GeneratorCrafting::generate);
    timedGenerate("industries", GeneratorIndustries::generate);
    timedGenerate("maps", GeneratorMaps::generate);
    timedGenerate("quests", GeneratorQuests::generate);
    timedGenerate("index", GeneratorIndex::generate);
    timedGenerate("start", GeneratorStart::generate);
    timedGenerate("tree", GeneratorTree::generate);
    // Generate contentstat.json (matches jsonstat.php)
    {
        int mapCount=0;
        for(const auto &set : MapStore::sets())
            mapCount+=(int)set.mapPaths.size();
        int monsterCount=(int)CatchChallenger::CommonDatapack::commonDatapack.get_monsters_size();
        int itemCount=(int)CatchChallenger::CommonDatapack::commonDatapack.get_items_size();
        // Bot count: count all non-empty bots across all maps (approximation)
        int botCount=0;
        for(const auto &set : MapStore::sets())
            for(const auto &m : set.mapList)
                botCount+=(int)m.botFights.size();

        std::string jsonPath=Helper::localPath()+"contentstat.json";
        std::ofstream jf(jsonPath);
        if(jf.is_open())
        {
            jf << "{\"map_count\":" << mapCount
               << ",\"bot_count\":" << botCount
               << ",\"monster_count\":" << monsterCount
               << ",\"item_count\":" << itemCount << "}";
        }
    }

    auto totalEnd=std::chrono::steady_clock::now();
    long long totalMs=std::chrono::duration_cast<std::chrono::milliseconds>(totalEnd-totalStart).count();
    std::cout << "Done. Total generation: " << totalMs << "ms" << std::endl;
}

} // namespace Generator
