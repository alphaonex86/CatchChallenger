#include "../../general/base/DatapackGeneralLoader.h"
#include "../../general/base/CommonDatapack.h"
#include "../../general/base/FacilityLibGeneral.h"

#include <chrono>
#include <iostream>
#include <map>
#include <unordered_map>
#include <chrono>

#define LISTCOUNT 1000000
#define LISTCOUNTR 10000
#define LISTCOUNTZ 10000
#define LISTCOUNTP 1000

int main(int argc, char *argv[])
{
    if(argc<1)
        return 52;
    CatchChallenger::FacilityLibGeneral::applicationDirPath=argv[0];

    CatchChallenger::CommonDatapack::commonDatapack.parseDatapack(
                CatchChallenger::FacilityLibGeneral::getFolderFromFile(CatchChallenger::FacilityLibGeneral::applicationDirPath)+"/datapack/"
                );
    if(CatchChallenger::CommonDatapack::commonDatapack.items.item.empty())
    {
        std::cerr << "Unable to load datapack item list" << std::endl;
        return 53;
    }

    {
        std::unordered_map<uint16_t, CatchChallenger::Item> item;
        {
            auto i=CatchChallenger::CommonDatapack::commonDatapack.items.item.begin();
            while(i!=CatchChallenger::CommonDatapack::commonDatapack.items.item.cend())
            {
                item[i->first]=i->second;
                ++i;
            }
        }

        {
            auto start = std::chrono::high_resolution_clock::now();

            srand(0);
            const uint32_t &size=item.size();
            uint64_t totalPrice=0;
            uint32_t index=0;
            while(index<LISTCOUNT)
            {
                const uint32_t &indexToSearch=rand()%size;
                if(item.find(indexToSearch)!=item.cend())
                {
                    const CatchChallenger::Item &entry=item.at(indexToSearch);
                    totalPrice+=entry.price;
                }
                index++;
            }

            auto end = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double, std::milli> elapsed = end-start;

            std::cerr << "Time: " << elapsed.count() << "ms" << std::endl;
        }

        {
            auto start = std::chrono::high_resolution_clock::now();

            srand(0);
            const uint32_t &size=item.size();
            uint64_t totalPrice=0;
            uint32_t index=0;
            while(index<LISTCOUNTR)
            {
                uint32_t offset=0;
                while(offset<30)
                {
                    const uint32_t &indexToSearch=rand()%size;
                    if(item.find(indexToSearch)!=item.cend())
                    {
                        const CatchChallenger::Item &entry=item.at(indexToSearch);
                        totalPrice+=entry.price;
                    }
                    offset++;
                }
                index++;
            }

            auto end = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double, std::milli> elapsed = end-start;

            std::cerr << "Time repeat: " << elapsed.count() << "ms" << std::endl;
        }

        {
            auto start = std::chrono::high_resolution_clock::now();

            srand(0);
            const uint32_t &size=item.size();
            uint64_t totalPrice=0;
            uint32_t index=0;
            while(index<LISTCOUNTZ)
            {
                uint32_t offset=0;
                while(offset<30)
                {
                    const uint32_t &indexToSearch=rand()%size;
                    if(item.find(indexToSearch+offset)!=item.cend())
                    {
                        const CatchChallenger::Item &entry=item.at(indexToSearch+offset);
                        totalPrice+=entry.price;
                    }
                    offset++;
                }
                index++;
            }

            auto end = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double, std::milli> elapsed = end-start;

            std::cerr << "Time same zone: " << elapsed.count() << "ms" << std::endl;
        }

        {
            auto start = std::chrono::high_resolution_clock::now();
            uint32_t index=0;
            while(index<LISTCOUNTP)
            {
                auto i=item.begin();
                while(i!=item.cend())
                {
                    ++i;
                }
                index++;
            }
            auto end = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double, std::milli> elapsed = end-start;
            std::cerr << "Time list: " << elapsed.count() << "ms" << std::endl;
        }
    }

    {
        std::map<uint16_t, CatchChallenger::Item> item;
        {
            auto i=CatchChallenger::CommonDatapack::commonDatapack.items.item.begin();
            while(i!=CatchChallenger::CommonDatapack::commonDatapack.items.item.cend())
            {
                item[i->first]=i->second;
                ++i;
            }
        }

        auto start = std::chrono::high_resolution_clock::now();

        srand(0);
        const uint32_t &size=item.size();
        uint64_t totalPrice=0;
        uint32_t index=0;
        while(index<LISTCOUNT)
        {
            const uint32_t &indexToSearch=rand()%size;
            if(item.find(indexToSearch)!=item.cend())
            {
                const CatchChallenger::Item &entry=item.at(indexToSearch);
                totalPrice+=entry.price;
            }
            index++;
        }

        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> elapsed = end-start;

        std::cerr << "Time: " << elapsed.count() << "ms" << std::endl;

        {
            auto start = std::chrono::high_resolution_clock::now();

            srand(0);
            const uint32_t &size=item.size();
            uint64_t totalPrice=0;
            uint32_t index=0;
            while(index<LISTCOUNTR)
            {
                uint32_t offset=0;
                while(offset<30)
                {
                    const uint32_t &indexToSearch=rand()%size;
                    if(item.find(indexToSearch)!=item.cend())
                    {
                        const CatchChallenger::Item &entry=item.at(indexToSearch);
                        totalPrice+=entry.price;
                    }
                    offset++;
                }
                index++;
            }

            auto end = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double, std::milli> elapsed = end-start;

            std::cerr << "Time repeat: " << elapsed.count() << "ms" << std::endl;
        }

        {
            auto start = std::chrono::high_resolution_clock::now();

            srand(0);
            const uint32_t &size=item.size();
            uint64_t totalPrice=0;
            uint32_t index=0;
            while(index<LISTCOUNTZ)
            {
                uint32_t offset=0;
                while(offset<30)
                {
                    const uint32_t &indexToSearch=rand()%size;
                    if(item.find(indexToSearch+offset)!=item.cend())
                    {
                        const CatchChallenger::Item &entry=item.at(indexToSearch+offset);
                        totalPrice+=entry.price;
                    }
                    offset++;
                }
                index++;
            }

            auto end = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double, std::milli> elapsed = end-start;

            std::cerr << "Time same zone: " << elapsed.count() << "ms" << std::endl;
        }

        {
            auto start = std::chrono::high_resolution_clock::now();
            uint32_t index=0;
            while(index<LISTCOUNTP)
            {
                auto i=item.begin();
                while(i!=item.cend())
                {
                    ++i;
                }
                index++;
            }
            auto end = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double, std::milli> elapsed = end-start;
            std::cerr << "Time list: " << elapsed.count() << "ms" << std::endl;
        }
    }

    {
        std::unordered_map<uint8_t, CatchChallenger::Plant> plant;
        {
            auto i=CatchChallenger::CommonDatapack::commonDatapack.plants.begin();
            while(i!=CatchChallenger::CommonDatapack::commonDatapack.plants.cend())
            {
                plant[i->first]=i->second;
                ++i;
            }
        }

        auto start = std::chrono::high_resolution_clock::now();

        srand(0);
        const uint32_t &size=plant.size();
        uint64_t totalPrice=0;
        uint32_t index=0;
        while(index<LISTCOUNT)
        {
            const uint32_t &indexToSearch=rand()%size;
            if(plant.find(indexToSearch)!=plant.cend())
            {
                const CatchChallenger::Plant &entry=plant.at(indexToSearch);
                totalPrice+=entry.itemUsed;
            }
            index++;
        }

        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> elapsed = end-start;

        std::cerr << "Time: " << elapsed.count() << "ms" << std::endl;

        {
            auto start = std::chrono::high_resolution_clock::now();

            srand(0);
            const uint32_t &size=plant.size();
            uint64_t totalPrice=0;
            uint32_t index=0;
            while(index<LISTCOUNTR)
            {
                uint32_t offset=0;
                while(offset<30)
                {
                    const uint32_t &indexToSearch=rand()%size;
                    if(plant.find(indexToSearch)!=plant.cend())
                    {
                        const CatchChallenger::Plant &entry=plant.at(indexToSearch);
                        totalPrice+=entry.itemUsed;
                    }
                    offset++;
                }
                index++;
            }

            auto end = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double, std::milli> elapsed = end-start;

            std::cerr << "Time repeat: " << elapsed.count() << "ms" << std::endl;
        }

        {
            auto start = std::chrono::high_resolution_clock::now();

            srand(0);
            const uint32_t &size=plant.size();
            uint64_t totalPrice=0;
            uint32_t index=0;
            while(index<LISTCOUNTZ)
            {
                uint32_t offset=0;
                while(offset<30)
                {
                    const uint32_t &indexToSearch=rand()%size;
                    if(plant.find(indexToSearch+offset)!=plant.cend())
                    {
                        const CatchChallenger::Plant &entry=plant.at(indexToSearch+offset);
                        totalPrice+=entry.itemUsed;
                    }
                    offset++;
                }
                index++;
            }

            auto end = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double, std::milli> elapsed = end-start;

            std::cerr << "Time same zone: " << elapsed.count() << "ms" << std::endl;
        }

        {
            auto start = std::chrono::high_resolution_clock::now();
            uint32_t index=0;
            while(index<LISTCOUNTP)
            {
                auto i=plant.begin();
                while(i!=plant.cend())
                {
                    ++i;
                }
                index++;
            }
            auto end = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double, std::milli> elapsed = end-start;
            std::cerr << "Time list: " << elapsed.count() << "ms" << std::endl;
        }
    }

    {
        std::map<uint8_t, CatchChallenger::Plant> plant;
        {
            auto i=CatchChallenger::CommonDatapack::commonDatapack.plants.begin();
            while(i!=CatchChallenger::CommonDatapack::commonDatapack.plants.cend())
            {
                plant[i->first]=i->second;
                ++i;
            }
        }

        auto start = std::chrono::high_resolution_clock::now();

        srand(0);
        const uint32_t &size=plant.size();
        uint64_t totalPrice=0;
        uint32_t index=0;
        while(index<LISTCOUNT)
        {
            const uint32_t &indexToSearch=rand()%size;
            if(plant.find(indexToSearch)!=plant.cend())
            {
                const CatchChallenger::Plant &entry=plant.at(indexToSearch);
                totalPrice+=entry.itemUsed;
            }
            index++;
        }

        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> elapsed = end-start;

        std::cerr << "Time: " << elapsed.count() << "ms" << std::endl;

        {
            auto start = std::chrono::high_resolution_clock::now();

            srand(0);
            const uint32_t &size=plant.size();
            uint64_t totalPrice=0;
            uint32_t index=0;
            while(index<LISTCOUNTR)
            {
                uint32_t offset=0;
                while(offset<30)
                {
                    const uint32_t &indexToSearch=rand()%size;
                    if(plant.find(indexToSearch)!=plant.cend())
                    {
                        const CatchChallenger::Plant &entry=plant.at(indexToSearch);
                        totalPrice+=entry.itemUsed;
                    }
                    offset++;
                }
                index++;
            }

            auto end = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double, std::milli> elapsed = end-start;

            std::cerr << "Time repeat: " << elapsed.count() << "ms" << std::endl;
        }

        {
            auto start = std::chrono::high_resolution_clock::now();

            srand(0);
            const uint32_t &size=plant.size();
            uint64_t totalPrice=0;
            uint32_t index=0;
            while(index<LISTCOUNTZ)
            {
                uint32_t offset=0;
                while(offset<30)
                {
                    const uint32_t &indexToSearch=rand()%size;
                    if(plant.find(indexToSearch+offset)!=plant.cend())
                    {
                        const CatchChallenger::Plant &entry=plant.at(indexToSearch+offset);
                        totalPrice+=entry.itemUsed;
                    }
                    offset++;
                }
                index++;
            }

            auto end = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double, std::milli> elapsed = end-start;

            std::cerr << "Time same zone: " << elapsed.count() << "ms" << std::endl;
        }

        {
            auto start = std::chrono::high_resolution_clock::now();
            uint32_t index=0;
            while(index<LISTCOUNTP)
            {
                auto i=plant.begin();
                while(i!=plant.cend())
                {
                    ++i;
                }
                index++;
            }
            auto end = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double, std::milli> elapsed = end-start;
            std::cerr << "Time list: " << elapsed.count() << "ms" << std::endl;
        }
    }

    return 0;
}
