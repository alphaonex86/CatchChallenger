#include "FacilityLibGateway.hpp"
#include "../../general/base/cpp11addition.hpp"

#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <iostream>

using namespace CatchChallenger;

bool FacilityLibGateway::dolocalfolder(const std::string &dir)
{
    struct stat myStat;
    if(stat(dir.c_str(),&myStat)==0)
    {
        if((myStat.st_mode&S_IFMT)==S_IFDIR)
            return true;
        else
            return false;
    }
    else
        return (mkdir(dir.c_str(),0700)==0);
}

bool FacilityLibGateway::mkpath(const std::string &dir)
{
#ifndef __linux__
#error this only work on linux
#endif
    std::string tempdir=dir;
    stringreplaceAll(tempdir,"//","/");
    const std::vector<std::string> &folderdir=stringsplit(dir,'/');
    unsigned int index=2;
    while(index<folderdir.size())
    {
        std::vector<std::string> tempsplit;
        std::copy(folderdir.cbegin(),folderdir.cbegin()+index,std::back_inserter(tempsplit));
        const std::string &pathtodo=stringimplode(tempsplit,'/');
        if(pathtodo.size()<2)
        {
            std::cerr << "FacilityLibGateway::mkpath(" << dir << "): pathtodo.size()<2" << std::endl;
            return false;
        }
        if(!dolocalfolder(pathtodo))
            return false;
        index++;
    }
    return dolocalfolder(tempdir);
}
