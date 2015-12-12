#include "FacilityLibGateway.h"

#include <sys/stat.h>
#include <stdio.h>
#include <string.h>

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
    char *temppath=(char *)malloc(dir.size()+1);
    strcpy(temppath,dir.c_str());
    char *separator=NULL;
    while((separator=strchr(temppath,'/'))!=0)
    {
        if(separator!=temppath)
        {
            *separator='\0';
            if(!dolocalfolder(dir))
            {
                delete temppath;
                return false;
            }
            *separator='/';
        }
        temppath=separator+1;
    }
    delete temppath;
    return dolocalfolder(dir);
}
