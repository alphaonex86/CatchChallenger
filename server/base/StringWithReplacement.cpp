#include "StringWithReplacement.h"
#include "../../general/base/cpp11addition.h"
#include "../../general/base/GeneralVariable.h"
#include <iostream>
#include <string.h>//memcpy

char StringWithReplacement::composeBuffer[];

StringWithReplacement::StringWithReplacement() :
    preparedQuery(NULL)
{
}

StringWithReplacement::StringWithReplacement(const std::string &query) :
    preparedQuery(NULL)
{
    set(query);
}

StringWithReplacement::~StringWithReplacement()
{
    if(preparedQuery!=NULL)
    {
        delete preparedQuery;
        preparedQuery=NULL;
    }
}

void StringWithReplacement::set(const std::string &query)
{
    if(preparedQuery!=NULL)
    {
        delete preparedQuery;
        preparedQuery=NULL;
    }
    if((query.size()+16+2)>=65535 || (query.size()+16+2)>=sizeof(composeBuffer))
    {
        std::cerr << "StringWithReplacement::operator=(): query to big to de stored: (query.size(): " << query.size() << "+16+2)>255" << std::endl;
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        abort();
        #endif
        return;
    }
    uint8_t numberOfReplace=15;
    while(1)
    {
        const auto &found = query.find("%"+std::to_string(numberOfReplace));
        if(found!=std::string::npos)
        {
            /* [0]: occurence to replace
             * [1,2]: total size of the String
             * List of: 16Bit header + string content */
            const uint16_t &arraysize=1+2+(numberOfReplace+1/*if one %1 mean 2 string, if  %1,%2 mean 3 string*/)*sizeof(uint16_t)+query.size();
            char preparedQueryTemp[arraysize];
            preparedQueryTemp[0]=numberOfReplace;
            preparedQueryTemp[1]=0;
            preparedQueryTemp[2]=0;
            uint16_t pos=3;
            uint16_t previousStringPos=0;
            uint8_t index=1;
            do
            {
                const std::string testToFind("%"+std::to_string(index));
                const auto &foundinternal = query.find(testToFind);
                if(foundinternal!=std::string::npos)
                {
                    if(foundinternal<previousStringPos)
                    {
                        std::cerr << "StringWithReplacement::operator=(): id to replace need be ordened" << std::endl;
                        #ifdef CATCHCHALLENGER_EXTRA_CHECK
                        abort();
                        #endif
                        return;
                    }
                    const uint16_t &size=foundinternal-previousStringPos;
                    *reinterpret_cast<uint16_t *>(preparedQueryTemp+pos)=size;
                    pos+=2;
                    if(size>0)
                    {
                        const std::string extractedPart(query.substr(previousStringPos,size));
                        memcpy(preparedQueryTemp+pos,extractedPart.data(),size);
                        pos+=size;
                    }
                    previousStringPos=foundinternal+testToFind.size();
                }
                else
                {
                    std::cerr << "StringWithReplacement::operator=(): missing id to replace" << std::endl;
                    #ifdef CATCHCHALLENGER_EXTRA_CHECK
                    abort();
                    #endif
                    return;
                }
                index++;
            } while(index<=numberOfReplace);
            if(numberOfReplace>15)
            {
                std::cerr << "StringWithReplacement::set(): numberOfReplace>15: " << numberOfReplace << std::endl;
                #ifdef CATCHCHALLENGER_EXTRA_CHECK
                abort();
                #endif
                return;
            }
            //the last part:
            const uint16_t &size=query.size()-previousStringPos;
            *reinterpret_cast<uint16_t *>(preparedQueryTemp+pos)=size;
            pos+=2;
            if(size>0)
            {
                const std::string extractedPart(query.substr(previousStringPos,size));
                #ifdef CATCHCHALLENGER_EXTRA_CHECK
                if(sizeof(preparedQueryTemp)<(pos+size+1))
                    abort();
                #endif
                memcpy(preparedQueryTemp+pos,extractedPart.data(),size);
                pos+=size;
            }
            //previousStringPos=size;
            *reinterpret_cast<uint16_t *>(preparedQueryTemp+1)=pos-3-(numberOfReplace*2+1);
            //copy
            preparedQuery=(unsigned char *)malloc(pos+1);
            memcpy(preparedQuery,preparedQueryTemp,pos);
            if(((uint32_t)*reinterpret_cast<uint16_t *>(preparedQuery+1)+1+100)>=sizeof(composeBuffer))
            {
                std::cerr << "StringWithReplacement::set(): preparedQuery header size too big: " << *reinterpret_cast<uint16_t *>(preparedQuery+1) << "+1+100>" << sizeof(composeBuffer) << ", query: " << originalQuery() << std::endl;
                std::cerr << "StringWithReplacement::set(): pos-3-(numberOfReplace*2+1): " << std::to_string(pos) << "-3-(" << std::to_string(numberOfReplace) << "*2+1)" << std::endl;
                #ifdef CATCHCHALLENGER_EXTRA_CHECK
                abort();
                #endif
                return;
            }
            //dump:
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            //std::cout << "StringWithReplacement: saved: " << originalQuery() << std::endl;
            #endif
            return;
        }
        --numberOfReplace;
        if(numberOfReplace<=0)
        {
            std::cerr << "StringWithReplacement: numberOfReplace<=0, no %1 found" << std::endl;
            return;
        }
    }
    std::cerr << "StringWithReplacement: out of loop" << std::endl;
}

bool StringWithReplacement::empty() const
{
    return (preparedQuery==NULL);
}

void StringWithReplacement::operator=(const std::string &query)
{
    set(query);
}

std::string StringWithReplacement::originalQuery() const
{
    if(preparedQuery==NULL)
    {
        std::cerr << "StringWithReplacement::compose(): preparedQuery==NULL" << std::endl;
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        abort();
        #endif
        return std::string();
    }
    //copy the first segments
    const uint16_t &firstChunkSize=*reinterpret_cast<uint16_t *>(preparedQuery+1+2);
    memcpy(composeBuffer,preparedQuery+1+2+2,firstChunkSize);
    uint16_t posComposeBuffer=firstChunkSize;
    uint16_t pos=1+2+2+firstChunkSize;
    uint8_t index=0;
    while(index<preparedQuery[0])
    {
        std::string arg("%"+std::to_string(index+1));
        memcpy(composeBuffer+posComposeBuffer,arg.data(),arg.size());
        posComposeBuffer+=arg.size();

        memcpy(composeBuffer+posComposeBuffer,preparedQuery+pos+2,*reinterpret_cast<uint16_t *>(preparedQuery+pos));
        posComposeBuffer+=preparedQuery[pos];
        pos+=2+preparedQuery[pos];
        ++index;
    }
    return std::string(composeBuffer,posComposeBuffer);
}

StringWithReplacement::StringWithReplacement(const StringWithReplacement& other) // copy constructor
{
    if(other.preparedQuery==nullptr)
    {
        preparedQuery=nullptr;
        return;
    }
    const uint16_t &size=StringWithReplacement::preparedQuerySize(other.preparedQuery);
    preparedQuery = new unsigned char[size + 1];
    memcpy(preparedQuery,other.preparedQuery,size);
}

StringWithReplacement::StringWithReplacement(StringWithReplacement&& other) // move constructor
    : preparedQuery(other.preparedQuery)
{
    other.preparedQuery = nullptr;
}

StringWithReplacement& StringWithReplacement::operator=(const StringWithReplacement& other) // copy assignment
{
    if(preparedQuery!=NULL)
        delete preparedQuery;
    if(other.preparedQuery==nullptr)
    {
        preparedQuery=nullptr;
        return *this;
    }
    const uint16_t &size=StringWithReplacement::preparedQuerySize(other.preparedQuery);
    preparedQuery = new unsigned char[size + 1];
    memcpy(preparedQuery,other.preparedQuery,size);
    return *this;
}

StringWithReplacement& StringWithReplacement::operator=(StringWithReplacement&& other) // move assignment
{
    if(preparedQuery!=NULL)
        delete preparedQuery;

    preparedQuery = other.preparedQuery;
    /*if you don't care about these errors you may set ASAN_OPTIONS=alloc_dealloc_mismatch=0
    const uint16_t &size=StringWithReplacement::preparedQuerySize(other.preparedQuery);
    preparedQuery = new unsigned char[size + 1];
    memcpy(preparedQuery,other.preparedQuery,size);*/

    other.preparedQuery = nullptr;
    return *this;
}

uint16_t StringWithReplacement::preparedQuerySize(const unsigned char * const preparedQuery)
{
    //copy the first segments
    const uint16_t &firstChunkSize=*reinterpret_cast<const uint16_t *>(preparedQuery+1+2);
    uint16_t pos=1+2+2+firstChunkSize;
    uint8_t index=0;
    while(index<preparedQuery[0])
    {
        pos+=2+preparedQuery[pos];
        ++index;
    }
    return pos;
}

std::string StringWithReplacement::compose(const std::vector<std::string> &values) const
{
    if(preparedQuery==NULL)
    {
        std::cerr << "StringWithReplacement::compose(): preparedQuery==NULL" << std::endl;
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        abort();
        #endif
        return std::string();
    }
    if(preparedQuery[0]!=values.size())
    {
        std::cerr << "StringWithReplacement::compose(): compose with wrong arguements count: " << originalQuery() << std::endl;
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        abort();
        #endif
        return std::string();
    }
    uint16_t totalSize=*reinterpret_cast<uint16_t *>(preparedQuery+1)+1;
    {
        uint8_t index=0;
        while(index<values.size())
        {
            totalSize+=values.at(index).size();
            index++;
        }
    }
    if(totalSize>=sizeof(composeBuffer))
    {
        std::cerr << "StringWithReplacement::compose(): (X) argument too big: " << *reinterpret_cast<uint16_t *>(preparedQuery+1) << " + arg:";
        uint8_t index=0;
        while(index<values.size())
        {
            std::cerr << " " << values.at(index);
            totalSize+=values.at(index).size();
            index++;
        }
        std::cerr << ">" << sizeof(composeBuffer) << ", query: " << originalQuery() << std::endl;
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        abort();
        #endif
        return std::string();
    }
    //copy the first segments
    const uint16_t &firstChunkSize=*reinterpret_cast<uint16_t *>(preparedQuery+1+2);
    memcpy(composeBuffer,preparedQuery+1+2+2,firstChunkSize);
    uint16_t posComposeBuffer=firstChunkSize;
    uint16_t pos=1+2+2+firstChunkSize;
    uint8_t index=0;
    while(index<preparedQuery[0])
    {
        memcpy(composeBuffer+posComposeBuffer,values.at(index).data(),values.at(index).size());
        posComposeBuffer+=values.at(index).size();
        memcpy(composeBuffer+posComposeBuffer,preparedQuery+pos+2,*reinterpret_cast<uint16_t *>(preparedQuery+pos));
        posComposeBuffer+=preparedQuery[pos];
        pos+=2+preparedQuery[pos];
        ++index;
    }
    return std::string(composeBuffer,posComposeBuffer);
}

uint8_t StringWithReplacement::argumentsCount() const
{
    if(preparedQuery==NULL)
    {
        std::cerr << "StringWithReplacement::argumentsCount(): preparedQuery==NULL" << std::endl;
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        abort();
        #endif
        return 0;
    }
    return preparedQuery[0];
}
