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
            const uint16_t &arraysize=1+numberOfReplace+query.size()+2;
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
            //the last part:
            const uint16_t &size=query.size()-previousStringPos;
            *reinterpret_cast<uint16_t *>(preparedQueryTemp+pos)=size;
            pos+=2;
            if(size>0)
            {
                const std::string extractedPart(query.substr(previousStringPos,size));
                memcpy(preparedQueryTemp+pos,extractedPart.data(),size);
                pos+=size;
            }
            previousStringPos=size;
            *reinterpret_cast<uint16_t *>(preparedQueryTemp+1)=pos-3-(numberOfReplace*2+1);
            //copy
            preparedQuery=(unsigned char *)malloc(pos+1);
            memcpy(preparedQuery,preparedQueryTemp,pos);
            //dump:
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            //std::cout << "StringWithReplacement: saved: " << originalQuery() << std::endl;
            #endif
            return;
        }
        --numberOfReplace;
        if(numberOfReplace<=0)
            return;
    }
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

std::string StringWithReplacement::compose(const std::string &arg1) const
{
    if(preparedQuery==NULL)
    {
        std::cerr << "StringWithReplacement::compose(): preparedQuery==NULL" << std::endl;
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        abort();
        #endif
        return std::string();
    }
    if(preparedQuery[0]!=1)
    {
        std::cerr << "StringWithReplacement::compose(): compose with wrong arguements count" << originalQuery() << std::endl;
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        abort();
        #endif
        return std::string();
    }
    if((*reinterpret_cast<uint16_t *>(preparedQuery+1)+arg1.size()+1)>=sizeof(composeBuffer))
    {
        std::cerr << "StringWithReplacement::compose(): argument too big" << std::endl;
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
        switch(index)
        {
            case 0:
            memcpy(composeBuffer+posComposeBuffer,arg1.data(),arg1.size());
            posComposeBuffer+=arg1.size();
            break;
            default:
            break;
        }
        memcpy(composeBuffer+posComposeBuffer,preparedQuery+pos+2,*reinterpret_cast<uint16_t *>(preparedQuery+pos));
        posComposeBuffer+=preparedQuery[pos];
        pos+=2+preparedQuery[pos];
        ++index;
    }
    return std::string(composeBuffer,posComposeBuffer);
}

std::string StringWithReplacement::compose(const std::string &arg1,
                    const std::string &arg2
                    ) const
{
    if(preparedQuery==NULL)
    {
        std::cerr << "StringWithReplacement::compose(): preparedQuery==NULL" << std::endl;
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        abort();
        #endif
        return std::string();
    }
    if(preparedQuery[0]!=2)
    {
        std::cerr << "StringWithReplacement::compose(): compose with wrong arguements count" << originalQuery() << std::endl;
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        abort();
        #endif
        return std::string();
    }
    if((*reinterpret_cast<uint16_t *>(preparedQuery+1)+arg1.size()+arg2.size()+1)>=sizeof(composeBuffer))
    {
        std::cerr << "StringWithReplacement::compose(): argument too big" << std::endl;
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
        switch(index)
        {
            case 0:
            memcpy(composeBuffer+posComposeBuffer,arg1.data(),arg1.size());
            posComposeBuffer+=arg1.size();
            break;
            case 1:
            memcpy(composeBuffer+posComposeBuffer,arg2.data(),arg2.size());
            posComposeBuffer+=arg2.size();
            break;
            default:
            break;
        }
        memcpy(composeBuffer+posComposeBuffer,preparedQuery+pos+2,*reinterpret_cast<uint16_t *>(preparedQuery+pos));
        posComposeBuffer+=preparedQuery[pos];
        pos+=2+preparedQuery[pos];
        ++index;
    }
    return std::string(composeBuffer,posComposeBuffer);
}

std::string StringWithReplacement::compose(const std::string &arg1,
                    const std::string &arg2,
                    const std::string &arg3
                    ) const
{
    if(preparedQuery==NULL)
    {
        std::cerr << "StringWithReplacement::compose(): preparedQuery==NULL" << std::endl;
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        abort();
        #endif
        return std::string();
    }
    if(preparedQuery[0]!=3)
    {
        std::cerr << "StringWithReplacement::compose(): compose with wrong arguements count" << originalQuery() << std::endl;
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        abort();
        #endif
        return std::string();
    }
    if((*reinterpret_cast<uint16_t *>(preparedQuery+1)+
        arg1.size()+arg2.size()+arg3.size()+
        1)>=sizeof(composeBuffer))
    {
        std::cerr << "StringWithReplacement::compose(): argument too big" << std::endl;
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
        switch(index)
        {
            case 0:
            memcpy(composeBuffer+posComposeBuffer,arg1.data(),arg1.size());
            posComposeBuffer+=arg1.size();
            break;
            case 1:
            memcpy(composeBuffer+posComposeBuffer,arg2.data(),arg2.size());
            posComposeBuffer+=arg2.size();
            break;
            case 2:
            memcpy(composeBuffer+posComposeBuffer,arg3.data(),arg3.size());
            posComposeBuffer+=arg3.size();
            break;
            default:
            break;
        }
        memcpy(composeBuffer+posComposeBuffer,preparedQuery+pos+2,*reinterpret_cast<uint16_t *>(preparedQuery+pos));
        posComposeBuffer+=preparedQuery[pos];
        pos+=2+preparedQuery[pos];
        ++index;
    }
    return std::string(composeBuffer,posComposeBuffer);
}

std::string StringWithReplacement::compose(const std::string &arg1,
                    const std::string &arg2,
                    const std::string &arg3,
                    const std::string &arg4
                    ) const
{
    if(preparedQuery==NULL)
    {
        std::cerr << "StringWithReplacement::compose(): preparedQuery==NULL" << std::endl;
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        abort();
        #endif
        return std::string();
    }
    if(preparedQuery[0]!=4)
    {
        std::cerr << "StringWithReplacement::compose(): compose with wrong arguements count" << originalQuery() << std::endl;
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        abort();
        #endif
        return std::string();
    }
    if((*reinterpret_cast<uint16_t *>(preparedQuery+1)+
        arg1.size()+arg2.size()+arg3.size()+arg4.size()+
        1)>=sizeof(composeBuffer))
    {
        std::cerr << "StringWithReplacement::compose(): argument too big" << std::endl;
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
        switch(index)
        {
            case 0:
            memcpy(composeBuffer+posComposeBuffer,arg1.data(),arg1.size());
            posComposeBuffer+=arg1.size();
            break;
            case 1:
            memcpy(composeBuffer+posComposeBuffer,arg2.data(),arg2.size());
            posComposeBuffer+=arg2.size();
            break;
            case 2:
            memcpy(composeBuffer+posComposeBuffer,arg3.data(),arg3.size());
            posComposeBuffer+=arg3.size();
            break;
            case 3:
            memcpy(composeBuffer+posComposeBuffer,arg4.data(),arg4.size());
            posComposeBuffer+=arg4.size();
            break;
            default:
            break;
        }
        memcpy(composeBuffer+posComposeBuffer,preparedQuery+pos+2,*reinterpret_cast<uint16_t *>(preparedQuery+pos));
        posComposeBuffer+=preparedQuery[pos];
        pos+=2+preparedQuery[pos];
        ++index;
    }
    return std::string(composeBuffer,posComposeBuffer);
}

std::string StringWithReplacement::compose(const std::string &arg1,
                    const std::string &arg2,
                    const std::string &arg3,
                    const std::string &arg4,
                    const std::string &arg5
                    ) const
{
    if(preparedQuery==NULL)
    {
        std::cerr << "StringWithReplacement::compose(): preparedQuery==NULL" << std::endl;
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        abort();
        #endif
        return std::string();
    }
    if(preparedQuery[0]!=5)
    {
        std::cerr << "StringWithReplacement::compose(): compose with wrong arguements count" << originalQuery() << std::endl;
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        abort();
        #endif
        return std::string();
    }
    if((*reinterpret_cast<uint16_t *>(preparedQuery+1)+
        arg1.size()+arg2.size()+arg3.size()+arg4.size()+
        arg5.size()+
        1)>=sizeof(composeBuffer))
    {
        std::cerr << "StringWithReplacement::compose(): argument too big" << std::endl;
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
        switch(index)
        {
            case 0:
            memcpy(composeBuffer+posComposeBuffer,arg1.data(),arg1.size());
            posComposeBuffer+=arg1.size();
            break;
            case 1:
            memcpy(composeBuffer+posComposeBuffer,arg2.data(),arg2.size());
            posComposeBuffer+=arg2.size();
            break;
            case 2:
            memcpy(composeBuffer+posComposeBuffer,arg3.data(),arg3.size());
            posComposeBuffer+=arg3.size();
            break;
            case 3:
            memcpy(composeBuffer+posComposeBuffer,arg4.data(),arg4.size());
            posComposeBuffer+=arg4.size();
            break;
            case 4:
            memcpy(composeBuffer+posComposeBuffer,arg5.data(),arg5.size());
            posComposeBuffer+=arg5.size();
            break;
            default:
            break;
        }
        memcpy(composeBuffer+posComposeBuffer,preparedQuery+pos+2,*reinterpret_cast<uint16_t *>(preparedQuery+pos));
        posComposeBuffer+=preparedQuery[pos];
        pos+=2+preparedQuery[pos];
        ++index;
    }
    return std::string(composeBuffer,posComposeBuffer);
}

std::string StringWithReplacement::compose(const std::string &arg1,
                    const std::string &arg2,
                    const std::string &arg3,
                    const std::string &arg4,
                    const std::string &arg5,
                    const std::string &arg6
                    ) const
{
    if(preparedQuery==NULL)
    {
        std::cerr << "StringWithReplacement::compose(): preparedQuery==NULL" << std::endl;
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        abort();
        #endif
        return std::string();
    }
    if(preparedQuery[0]!=6)
    {
        std::cerr << "StringWithReplacement::compose(): compose with wrong arguements count" << originalQuery() << std::endl;
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        abort();
        #endif
        return std::string();
    }
    if((*reinterpret_cast<uint16_t *>(preparedQuery+1)+
        arg1.size()+arg2.size()+arg3.size()+arg4.size()+
        arg5.size()+arg6.size()+
        1)>=sizeof(composeBuffer))
    {
        std::cerr << "StringWithReplacement::compose(): argument too big" << std::endl;
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
        switch(index)
        {
            case 0:
            memcpy(composeBuffer+posComposeBuffer,arg1.data(),arg1.size());
            posComposeBuffer+=arg1.size();
            break;
            case 1:
            memcpy(composeBuffer+posComposeBuffer,arg2.data(),arg2.size());
            posComposeBuffer+=arg2.size();
            break;
            case 2:
            memcpy(composeBuffer+posComposeBuffer,arg3.data(),arg3.size());
            posComposeBuffer+=arg3.size();
            break;
            case 3:
            memcpy(composeBuffer+posComposeBuffer,arg4.data(),arg4.size());
            posComposeBuffer+=arg4.size();
            break;
            case 4:
            memcpy(composeBuffer+posComposeBuffer,arg5.data(),arg5.size());
            posComposeBuffer+=arg5.size();
            break;
            case 5:
            memcpy(composeBuffer+posComposeBuffer,arg6.data(),arg6.size());
            posComposeBuffer+=arg6.size();
            break;
            default:
            break;
        }
        memcpy(composeBuffer+posComposeBuffer,preparedQuery+pos+2,*reinterpret_cast<uint16_t *>(preparedQuery+pos));
        posComposeBuffer+=preparedQuery[pos];
        pos+=2+preparedQuery[pos];
        ++index;
    }
    return std::string(composeBuffer,posComposeBuffer);
}

std::string StringWithReplacement::compose(const std::string &arg1,
                    const std::string &arg2,
                    const std::string &arg3,
                    const std::string &arg4,
                    const std::string &arg5,
                    const std::string &arg6,
                    const std::string &arg7
                    ) const
{
    if(preparedQuery==NULL)
    {
        std::cerr << "StringWithReplacement::compose(): preparedQuery==NULL" << std::endl;
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        abort();
        #endif
        return std::string();
    }
    if(preparedQuery[0]!=7)
    {
        std::cerr << "StringWithReplacement::compose(): compose with wrong arguements count" << originalQuery() << std::endl;
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        abort();
        #endif
        return std::string();
    }
    if((*reinterpret_cast<uint16_t *>(preparedQuery+1)+
        arg1.size()+arg2.size()+arg3.size()+arg4.size()+
        arg5.size()+arg6.size()+arg7.size()+
        1)>=sizeof(composeBuffer))
    {
        std::cerr << "StringWithReplacement::compose(): argument too big" << std::endl;
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
        switch(index)
        {
            case 0:
            memcpy(composeBuffer+posComposeBuffer,arg1.data(),arg1.size());
            posComposeBuffer+=arg1.size();
            break;
            case 1:
            memcpy(composeBuffer+posComposeBuffer,arg2.data(),arg2.size());
            posComposeBuffer+=arg2.size();
            break;
            case 2:
            memcpy(composeBuffer+posComposeBuffer,arg3.data(),arg3.size());
            posComposeBuffer+=arg3.size();
            break;
            case 3:
            memcpy(composeBuffer+posComposeBuffer,arg4.data(),arg4.size());
            posComposeBuffer+=arg4.size();
            break;
            case 4:
            memcpy(composeBuffer+posComposeBuffer,arg5.data(),arg5.size());
            posComposeBuffer+=arg5.size();
            break;
            case 5:
            memcpy(composeBuffer+posComposeBuffer,arg6.data(),arg6.size());
            posComposeBuffer+=arg6.size();
            break;
            case 6:
            memcpy(composeBuffer+posComposeBuffer,arg7.data(),arg7.size());
            posComposeBuffer+=arg7.size();
            break;
            default:
            break;
        }
        memcpy(composeBuffer+posComposeBuffer,preparedQuery+pos+2,*reinterpret_cast<uint16_t *>(preparedQuery+pos));
        posComposeBuffer+=preparedQuery[pos];
        pos+=2+preparedQuery[pos];
        ++index;
    }
    return std::string(composeBuffer,posComposeBuffer);
}

std::string StringWithReplacement::compose(const std::string &arg1,
                    const std::string &arg2,
                    const std::string &arg3,
                    const std::string &arg4,
                    const std::string &arg5,
                    const std::string &arg6,
                    const std::string &arg7,
                    const std::string &arg8
                    ) const
{
    if(preparedQuery==NULL)
    {
        std::cerr << "StringWithReplacement::compose(): preparedQuery==NULL" << std::endl;
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        abort();
        #endif
        return std::string();
    }
    if(preparedQuery[0]!=8)
    {
        std::cerr << "StringWithReplacement::compose(): compose with wrong arguements count" << originalQuery() << std::endl;
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        abort();
        #endif
        return std::string();
    }
    if((*reinterpret_cast<uint16_t *>(preparedQuery+1)+
        arg1.size()+arg2.size()+arg3.size()+arg4.size()+
        arg5.size()+arg6.size()+arg7.size()+arg8.size()+
        1)>=sizeof(composeBuffer))
    {
        std::cerr << "StringWithReplacement::compose(): argument too big" << std::endl;
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
        switch(index)
        {
            case 0:
            memcpy(composeBuffer+posComposeBuffer,arg1.data(),arg1.size());
            posComposeBuffer+=arg1.size();
            break;
            case 1:
            memcpy(composeBuffer+posComposeBuffer,arg2.data(),arg2.size());
            posComposeBuffer+=arg2.size();
            break;
            case 2:
            memcpy(composeBuffer+posComposeBuffer,arg3.data(),arg3.size());
            posComposeBuffer+=arg3.size();
            break;
            case 3:
            memcpy(composeBuffer+posComposeBuffer,arg4.data(),arg4.size());
            posComposeBuffer+=arg4.size();
            break;
            case 4:
            memcpy(composeBuffer+posComposeBuffer,arg5.data(),arg5.size());
            posComposeBuffer+=arg5.size();
            break;
            case 5:
            memcpy(composeBuffer+posComposeBuffer,arg6.data(),arg6.size());
            posComposeBuffer+=arg6.size();
            break;
            case 6:
            memcpy(composeBuffer+posComposeBuffer,arg7.data(),arg7.size());
            posComposeBuffer+=arg7.size();
            break;
            case 7:
            memcpy(composeBuffer+posComposeBuffer,arg8.data(),arg8.size());
            posComposeBuffer+=arg8.size();
            break;
            default:
            break;
        }
        memcpy(composeBuffer+posComposeBuffer,preparedQuery+pos+2,*reinterpret_cast<uint16_t *>(preparedQuery+pos));
        posComposeBuffer+=preparedQuery[pos];
        pos+=2+preparedQuery[pos];
        ++index;
    }
    return std::string(composeBuffer,posComposeBuffer);
}

std::string StringWithReplacement::compose(const std::string &arg1,
                    const std::string &arg2,
                    const std::string &arg3,
                    const std::string &arg4,
                    const std::string &arg5,
                    const std::string &arg6,
                    const std::string &arg7,
                    const std::string &arg8,
                    const std::string &arg9
                    ) const
{
    if(preparedQuery==NULL)
    {
        std::cerr << "StringWithReplacement::compose(): preparedQuery==NULL" << std::endl;
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        abort();
        #endif
        return std::string();
    }
    if(preparedQuery[0]!=9)
    {
        std::cerr << "StringWithReplacement::compose(): compose with wrong arguements count" << originalQuery() << std::endl;
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        abort();
        #endif
        return std::string();
    }
    if((*reinterpret_cast<uint16_t *>(preparedQuery+1)+
        arg1.size()+arg2.size()+arg3.size()+arg4.size()+
        arg5.size()+arg6.size()+arg7.size()+arg8.size()+
        arg9.size()+
        1)>=sizeof(composeBuffer))
    {
        std::cerr << "StringWithReplacement::compose(): argument too big, preparedQuery[1]: "
                  << std::to_string(preparedQuery[1])
                  << ", sizeof(composeBuffer): "
                  << sizeof(composeBuffer)
                  << ", arg size: "
                  << arg1.size()+arg2.size()+arg3.size()+arg4.size()+
                     arg5.size()+arg6.size()+arg7.size()+arg8.size()+
                     arg9.size()
                  << std::endl;
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
        switch(index)
        {
            case 0:
            memcpy(composeBuffer+posComposeBuffer,arg1.data(),arg1.size());
            posComposeBuffer+=arg1.size();
            break;
            case 1:
            memcpy(composeBuffer+posComposeBuffer,arg2.data(),arg2.size());
            posComposeBuffer+=arg2.size();
            break;
            case 2:
            memcpy(composeBuffer+posComposeBuffer,arg3.data(),arg3.size());
            posComposeBuffer+=arg3.size();
            break;
            case 3:
            memcpy(composeBuffer+posComposeBuffer,arg4.data(),arg4.size());
            posComposeBuffer+=arg4.size();
            break;
            case 4:
            memcpy(composeBuffer+posComposeBuffer,arg5.data(),arg5.size());
            posComposeBuffer+=arg5.size();
            break;
            case 5:
            memcpy(composeBuffer+posComposeBuffer,arg6.data(),arg6.size());
            posComposeBuffer+=arg6.size();
            break;
            case 6:
            memcpy(composeBuffer+posComposeBuffer,arg7.data(),arg7.size());
            posComposeBuffer+=arg7.size();
            break;
            case 7:
            memcpy(composeBuffer+posComposeBuffer,arg8.data(),arg8.size());
            posComposeBuffer+=arg8.size();
            break;
            case 8:
            memcpy(composeBuffer+posComposeBuffer,arg9.data(),arg9.size());
            posComposeBuffer+=arg9.size();
            break;
            default:
            break;
        }
        memcpy(composeBuffer+posComposeBuffer,preparedQuery+pos+2,*reinterpret_cast<uint16_t *>(preparedQuery+pos));
        posComposeBuffer+=preparedQuery[pos];
        pos+=2+preparedQuery[pos];
        ++index;
    }
    return std::string(composeBuffer,posComposeBuffer);
}

std::string StringWithReplacement::compose(const std::string &arg1,
                    const std::string &arg2,
                    const std::string &arg3,
                    const std::string &arg4,
                    const std::string &arg5,
                    const std::string &arg6,
                    const std::string &arg7,
                    const std::string &arg8,
                    const std::string &arg9,
                    const std::string &arg10
                    ) const
{
    if(preparedQuery==NULL)
    {
        std::cerr << "StringWithReplacement::compose(): preparedQuery==NULL" << std::endl;
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        abort();
        #endif
        return std::string();
    }
    if(preparedQuery[0]!=10)
    {
        std::cerr << "StringWithReplacement::compose(): compose with wrong arguements count" << originalQuery() << std::endl;
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        abort();
        #endif
        return std::string();
    }
    if((*reinterpret_cast<uint16_t *>(preparedQuery+1)+
        arg1.size()+arg2.size()+arg3.size()+arg4.size()+
        arg5.size()+arg6.size()+arg7.size()+arg8.size()+
        arg9.size()+arg10.size()+
        1)>=sizeof(composeBuffer))
    {
        std::cerr << "StringWithReplacement::compose(): argument too big" << std::endl;
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
        switch(index)
        {
            case 0:
            memcpy(composeBuffer+posComposeBuffer,arg1.data(),arg1.size());
            posComposeBuffer+=arg1.size();
            break;
            case 1:
            memcpy(composeBuffer+posComposeBuffer,arg2.data(),arg2.size());
            posComposeBuffer+=arg2.size();
            break;
            case 2:
            memcpy(composeBuffer+posComposeBuffer,arg3.data(),arg3.size());
            posComposeBuffer+=arg3.size();
            break;
            case 3:
            memcpy(composeBuffer+posComposeBuffer,arg4.data(),arg4.size());
            posComposeBuffer+=arg4.size();
            break;
            case 4:
            memcpy(composeBuffer+posComposeBuffer,arg5.data(),arg5.size());
            posComposeBuffer+=arg5.size();
            break;
            case 5:
            memcpy(composeBuffer+posComposeBuffer,arg6.data(),arg6.size());
            posComposeBuffer+=arg6.size();
            break;
            case 6:
            memcpy(composeBuffer+posComposeBuffer,arg7.data(),arg7.size());
            posComposeBuffer+=arg7.size();
            break;
            case 7:
            memcpy(composeBuffer+posComposeBuffer,arg8.data(),arg8.size());
            posComposeBuffer+=arg8.size();
            break;
            case 8:
            memcpy(composeBuffer+posComposeBuffer,arg9.data(),arg9.size());
            posComposeBuffer+=arg9.size();
            break;
            case 9:
            memcpy(composeBuffer+posComposeBuffer,arg10.data(),arg10.size());
            posComposeBuffer+=arg10.size();
            break;
            default:
            break;
        }
        memcpy(composeBuffer+posComposeBuffer,preparedQuery+pos+2,*reinterpret_cast<uint16_t *>(preparedQuery+pos));
        posComposeBuffer+=preparedQuery[pos];
        pos+=2+preparedQuery[pos];
        ++index;
    }
    return std::string(composeBuffer,posComposeBuffer);
}

std::string StringWithReplacement::compose(const std::string &arg1,
                    const std::string &arg2,
                    const std::string &arg3,
                    const std::string &arg4,
                    const std::string &arg5,
                    const std::string &arg6,
                    const std::string &arg7,
                    const std::string &arg8,
                    const std::string &arg9,
                    const std::string &arg10,
                    const std::string &arg11
                    ) const
{
    if(preparedQuery==NULL)
    {
        std::cerr << "StringWithReplacement::compose(): preparedQuery==NULL" << std::endl;
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        abort();
        #endif
        return std::string();
    }
    if(preparedQuery[0]!=11)
    {
        std::cerr << "StringWithReplacement::compose(): compose with wrong arguements count" << originalQuery() << std::endl;
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        abort();
        #endif
        return std::string();
    }
    if((*reinterpret_cast<uint16_t *>(preparedQuery+1)+
        arg1.size()+arg2.size()+arg3.size()+arg4.size()+
        arg5.size()+arg6.size()+arg7.size()+arg8.size()+
        arg9.size()+arg10.size()+arg11.size()+
        1)>=sizeof(composeBuffer))
    {
        std::cerr << "StringWithReplacement::compose(): argument too big" << std::endl;
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
        switch(index)
        {
            case 0:
            memcpy(composeBuffer+posComposeBuffer,arg1.data(),arg1.size());
            posComposeBuffer+=arg1.size();
            break;
            case 1:
            memcpy(composeBuffer+posComposeBuffer,arg2.data(),arg2.size());
            posComposeBuffer+=arg2.size();
            break;
            case 2:
            memcpy(composeBuffer+posComposeBuffer,arg3.data(),arg3.size());
            posComposeBuffer+=arg3.size();
            break;
            case 3:
            memcpy(composeBuffer+posComposeBuffer,arg4.data(),arg4.size());
            posComposeBuffer+=arg4.size();
            break;
            case 4:
            memcpy(composeBuffer+posComposeBuffer,arg5.data(),arg5.size());
            posComposeBuffer+=arg5.size();
            break;
            case 5:
            memcpy(composeBuffer+posComposeBuffer,arg6.data(),arg6.size());
            posComposeBuffer+=arg6.size();
            break;
            case 6:
            memcpy(composeBuffer+posComposeBuffer,arg7.data(),arg7.size());
            posComposeBuffer+=arg7.size();
            break;
            case 7:
            memcpy(composeBuffer+posComposeBuffer,arg8.data(),arg8.size());
            posComposeBuffer+=arg8.size();
            break;
            case 8:
            memcpy(composeBuffer+posComposeBuffer,arg9.data(),arg9.size());
            posComposeBuffer+=arg9.size();
            break;
            case 9:
            memcpy(composeBuffer+posComposeBuffer,arg10.data(),arg10.size());
            posComposeBuffer+=arg10.size();
            break;
            case 10:
            memcpy(composeBuffer+posComposeBuffer,arg11.data(),arg11.size());
            posComposeBuffer+=arg11.size();
            break;
            default:
            break;
        }
        memcpy(composeBuffer+posComposeBuffer,preparedQuery+pos+2,*reinterpret_cast<uint16_t *>(preparedQuery+pos));
        posComposeBuffer+=preparedQuery[pos];
        pos+=2+preparedQuery[pos];
        ++index;
    }
    return std::string(composeBuffer,posComposeBuffer);
}

std::string StringWithReplacement::compose(const std::string &arg1,
                    const std::string &arg2,
                    const std::string &arg3,
                    const std::string &arg4,
                    const std::string &arg5,
                    const std::string &arg6,
                    const std::string &arg7,
                    const std::string &arg8,
                    const std::string &arg9,
                    const std::string &arg10,
                    const std::string &arg11,
                    const std::string &arg12
                    ) const
{
    if(preparedQuery==NULL)
    {
        std::cerr << "StringWithReplacement::compose(): preparedQuery==NULL" << std::endl;
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        abort();
        #endif
        return std::string();
    }
    if(preparedQuery[0]!=12)
    {
        std::cerr << "StringWithReplacement::compose(): compose with wrong arguements count" << originalQuery() << std::endl;
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        abort();
        #endif
        return std::string();
    }
    if((*reinterpret_cast<uint16_t *>(preparedQuery+1)+
        arg1.size()+arg2.size()+arg3.size()+arg4.size()+
        arg5.size()+arg6.size()+arg7.size()+arg8.size()+
        arg9.size()+arg10.size()+arg11.size()+arg12.size()+
        1)>=sizeof(composeBuffer))
    {
        std::cerr << "StringWithReplacement::compose(): argument too big" << std::endl;
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
        switch(index)
        {
            case 0:
            memcpy(composeBuffer+posComposeBuffer,arg1.data(),arg1.size());
            posComposeBuffer+=arg1.size();
            break;
            case 1:
            memcpy(composeBuffer+posComposeBuffer,arg2.data(),arg2.size());
            posComposeBuffer+=arg2.size();
            break;
            case 2:
            memcpy(composeBuffer+posComposeBuffer,arg3.data(),arg3.size());
            posComposeBuffer+=arg3.size();
            break;
            case 3:
            memcpy(composeBuffer+posComposeBuffer,arg4.data(),arg4.size());
            posComposeBuffer+=arg4.size();
            break;
            case 4:
            memcpy(composeBuffer+posComposeBuffer,arg5.data(),arg5.size());
            posComposeBuffer+=arg5.size();
            break;
            case 5:
            memcpy(composeBuffer+posComposeBuffer,arg6.data(),arg6.size());
            posComposeBuffer+=arg6.size();
            break;
            case 6:
            memcpy(composeBuffer+posComposeBuffer,arg7.data(),arg7.size());
            posComposeBuffer+=arg7.size();
            break;
            case 7:
            memcpy(composeBuffer+posComposeBuffer,arg8.data(),arg8.size());
            posComposeBuffer+=arg8.size();
            break;
            case 8:
            memcpy(composeBuffer+posComposeBuffer,arg9.data(),arg9.size());
            posComposeBuffer+=arg9.size();
            break;
            case 9:
            memcpy(composeBuffer+posComposeBuffer,arg10.data(),arg10.size());
            posComposeBuffer+=arg10.size();
            break;
            case 10:
            memcpy(composeBuffer+posComposeBuffer,arg11.data(),arg11.size());
            posComposeBuffer+=arg11.size();
            break;
            case 11:
            memcpy(composeBuffer+posComposeBuffer,arg12.data(),arg12.size());
            posComposeBuffer+=arg12.size();
            break;
            default:
            break;
        }
        memcpy(composeBuffer+posComposeBuffer,preparedQuery+pos+2,*reinterpret_cast<uint16_t *>(preparedQuery+pos));
        posComposeBuffer+=preparedQuery[pos];
        pos+=2+preparedQuery[pos];
        ++index;
    }
    return std::string(composeBuffer,posComposeBuffer);
}

std::string StringWithReplacement::compose(const std::string &arg1,
                    const std::string &arg2,
                    const std::string &arg3,
                    const std::string &arg4,
                    const std::string &arg5,
                    const std::string &arg6,
                    const std::string &arg7,
                    const std::string &arg8,
                    const std::string &arg9,
                    const std::string &arg10,
                    const std::string &arg11,
                    const std::string &arg12,
                    const std::string &arg13
                    ) const
{
    if(preparedQuery==NULL)
    {
        std::cerr << "StringWithReplacement::compose(): preparedQuery==NULL" << std::endl;
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        abort();
        #endif
        return std::string();
    }
    if(preparedQuery[0]!=13)
    {
        std::cerr << "StringWithReplacement::compose(): compose with wrong arguements count" << originalQuery() << std::endl;
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        abort();
        #endif
        return std::string();
    }
    if((*reinterpret_cast<uint16_t *>(preparedQuery+1)+
        arg1.size()+arg2.size()+arg3.size()+arg4.size()+
        arg5.size()+arg6.size()+arg7.size()+arg8.size()+
        arg9.size()+arg10.size()+arg11.size()+arg12.size()+
        arg13.size()+
        1)>=sizeof(composeBuffer))
    {
        std::cerr << "StringWithReplacement::compose(): argument too big" << std::endl;
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
        switch(index)
        {
            case 0:
            memcpy(composeBuffer+posComposeBuffer,arg1.data(),arg1.size());
            posComposeBuffer+=arg1.size();
            break;
            case 1:
            memcpy(composeBuffer+posComposeBuffer,arg2.data(),arg2.size());
            posComposeBuffer+=arg2.size();
            break;
            case 2:
            memcpy(composeBuffer+posComposeBuffer,arg3.data(),arg3.size());
            posComposeBuffer+=arg3.size();
            break;
            case 3:
            memcpy(composeBuffer+posComposeBuffer,arg4.data(),arg4.size());
            posComposeBuffer+=arg4.size();
            break;
            case 4:
            memcpy(composeBuffer+posComposeBuffer,arg5.data(),arg5.size());
            posComposeBuffer+=arg5.size();
            break;
            case 5:
            memcpy(composeBuffer+posComposeBuffer,arg6.data(),arg6.size());
            posComposeBuffer+=arg6.size();
            break;
            case 6:
            memcpy(composeBuffer+posComposeBuffer,arg7.data(),arg7.size());
            posComposeBuffer+=arg7.size();
            break;
            case 7:
            memcpy(composeBuffer+posComposeBuffer,arg8.data(),arg8.size());
            posComposeBuffer+=arg8.size();
            break;
            case 8:
            memcpy(composeBuffer+posComposeBuffer,arg9.data(),arg9.size());
            posComposeBuffer+=arg9.size();
            break;
            case 9:
            memcpy(composeBuffer+posComposeBuffer,arg10.data(),arg10.size());
            posComposeBuffer+=arg10.size();
            break;
            case 10:
            memcpy(composeBuffer+posComposeBuffer,arg11.data(),arg11.size());
            posComposeBuffer+=arg11.size();
            break;
            case 11:
            memcpy(composeBuffer+posComposeBuffer,arg12.data(),arg12.size());
            posComposeBuffer+=arg12.size();
            break;
            case 12:
            memcpy(composeBuffer+posComposeBuffer,arg13.data(),arg13.size());
            posComposeBuffer+=arg13.size();
            break;
            default:
            break;
        }
        memcpy(composeBuffer+posComposeBuffer,preparedQuery+pos+2,*reinterpret_cast<uint16_t *>(preparedQuery+pos));
        posComposeBuffer+=preparedQuery[pos];
        pos+=2+preparedQuery[pos];
        ++index;
    }
    return std::string(composeBuffer,posComposeBuffer);
}

std::string StringWithReplacement::compose(const std::string &arg1,
                    const std::string &arg2,
                    const std::string &arg3,
                    const std::string &arg4,
                    const std::string &arg5,
                    const std::string &arg6,
                    const std::string &arg7,
                    const std::string &arg8,
                    const std::string &arg9,
                    const std::string &arg10,
                    const std::string &arg11,
                    const std::string &arg12,
                    const std::string &arg13,
                    const std::string &arg14
                    ) const
{
    if(preparedQuery==NULL)
    {
        std::cerr << "StringWithReplacement::compose(): preparedQuery==NULL" << std::endl;
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        abort();
        #endif
        return std::string();
    }
    if(preparedQuery[0]!=14)
    {
        std::cerr << "StringWithReplacement::compose(): compose with wrong arguements count" << originalQuery() << std::endl;
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        abort();
        #endif
        return std::string();
    }
    if((*reinterpret_cast<uint16_t *>(preparedQuery+1)+
        arg1.size()+arg2.size()+arg3.size()+arg4.size()+
        arg5.size()+arg6.size()+arg7.size()+arg8.size()+
        arg9.size()+arg10.size()+arg11.size()+arg12.size()+
        arg13.size()+arg14.size()+
        1)>=sizeof(composeBuffer))
    {
        std::cerr << "StringWithReplacement::compose(): argument too big" << std::endl;
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
        switch(index)
        {
            case 0:
            memcpy(composeBuffer+posComposeBuffer,arg1.data(),arg1.size());
            posComposeBuffer+=arg1.size();
            break;
            case 1:
            memcpy(composeBuffer+posComposeBuffer,arg2.data(),arg2.size());
            posComposeBuffer+=arg2.size();
            break;
            case 2:
            memcpy(composeBuffer+posComposeBuffer,arg3.data(),arg3.size());
            posComposeBuffer+=arg3.size();
            break;
            case 3:
            memcpy(composeBuffer+posComposeBuffer,arg4.data(),arg4.size());
            posComposeBuffer+=arg4.size();
            break;
            case 4:
            memcpy(composeBuffer+posComposeBuffer,arg5.data(),arg5.size());
            posComposeBuffer+=arg5.size();
            break;
            case 5:
            memcpy(composeBuffer+posComposeBuffer,arg6.data(),arg6.size());
            posComposeBuffer+=arg6.size();
            break;
            case 6:
            memcpy(composeBuffer+posComposeBuffer,arg7.data(),arg7.size());
            posComposeBuffer+=arg7.size();
            break;
            case 7:
            memcpy(composeBuffer+posComposeBuffer,arg8.data(),arg8.size());
            posComposeBuffer+=arg8.size();
            break;
            case 8:
            memcpy(composeBuffer+posComposeBuffer,arg9.data(),arg9.size());
            posComposeBuffer+=arg9.size();
            break;
            case 9:
            memcpy(composeBuffer+posComposeBuffer,arg10.data(),arg10.size());
            posComposeBuffer+=arg10.size();
            break;
            case 10:
            memcpy(composeBuffer+posComposeBuffer,arg11.data(),arg11.size());
            posComposeBuffer+=arg11.size();
            break;
            case 11:
            memcpy(composeBuffer+posComposeBuffer,arg12.data(),arg12.size());
            posComposeBuffer+=arg12.size();
            break;
            case 12:
            memcpy(composeBuffer+posComposeBuffer,arg13.data(),arg13.size());
            posComposeBuffer+=arg13.size();
            break;
            case 13:
            memcpy(composeBuffer+posComposeBuffer,arg14.data(),arg14.size());
            posComposeBuffer+=arg14.size();
            break;
            default:
            break;
        }
        memcpy(composeBuffer+posComposeBuffer,preparedQuery+pos+2,*reinterpret_cast<uint16_t *>(preparedQuery+pos));
        posComposeBuffer+=preparedQuery[pos];
        pos+=2+preparedQuery[pos];
        ++index;
    }
    return std::string(composeBuffer,posComposeBuffer);
}

std::string StringWithReplacement::compose(const std::string &arg1,
                    const std::string &arg2,
                    const std::string &arg3,
                    const std::string &arg4,
                    const std::string &arg5,
                    const std::string &arg6,
                    const std::string &arg7,
                    const std::string &arg8,
                    const std::string &arg9,
                    const std::string &arg10,
                    const std::string &arg11,
                    const std::string &arg12,
                    const std::string &arg13,
                    const std::string &arg14,
                    const std::string &arg15
                    ) const
{
    if(preparedQuery==NULL)
    {
        std::cerr << "StringWithReplacement::compose(): preparedQuery==NULL" << std::endl;
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        abort();
        #endif
        return std::string();
    }
    if(preparedQuery[0]!=15)
    {
        std::cerr << "StringWithReplacement::compose(): compose with wrong arguements count" << originalQuery() << std::endl;
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        abort();
        #endif
        return std::string();
    }
    if((*reinterpret_cast<uint16_t *>(preparedQuery+1)+
        arg1.size()+arg2.size()+arg3.size()+arg4.size()+
        arg5.size()+arg6.size()+arg7.size()+arg8.size()+
        arg9.size()+arg10.size()+arg11.size()+arg12.size()+
        arg13.size()+arg14.size()+arg15.size()+
        1)>=sizeof(composeBuffer))
    {
        std::cerr << "StringWithReplacement::compose(): argument too big" << std::endl;
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
        switch(index)
        {
            case 0:
            memcpy(composeBuffer+posComposeBuffer,arg1.data(),arg1.size());
            posComposeBuffer+=arg1.size();
            break;
            case 1:
            memcpy(composeBuffer+posComposeBuffer,arg2.data(),arg2.size());
            posComposeBuffer+=arg2.size();
            break;
            case 2:
            memcpy(composeBuffer+posComposeBuffer,arg3.data(),arg3.size());
            posComposeBuffer+=arg3.size();
            break;
            case 3:
            memcpy(composeBuffer+posComposeBuffer,arg4.data(),arg4.size());
            posComposeBuffer+=arg4.size();
            break;
            case 4:
            memcpy(composeBuffer+posComposeBuffer,arg5.data(),arg5.size());
            posComposeBuffer+=arg5.size();
            break;
            case 5:
            memcpy(composeBuffer+posComposeBuffer,arg6.data(),arg6.size());
            posComposeBuffer+=arg6.size();
            break;
            case 6:
            memcpy(composeBuffer+posComposeBuffer,arg7.data(),arg7.size());
            posComposeBuffer+=arg7.size();
            break;
            case 7:
            memcpy(composeBuffer+posComposeBuffer,arg8.data(),arg8.size());
            posComposeBuffer+=arg8.size();
            break;
            case 8:
            memcpy(composeBuffer+posComposeBuffer,arg9.data(),arg9.size());
            posComposeBuffer+=arg9.size();
            break;
            case 9:
            memcpy(composeBuffer+posComposeBuffer,arg10.data(),arg10.size());
            posComposeBuffer+=arg10.size();
            break;
            case 10:
            memcpy(composeBuffer+posComposeBuffer,arg11.data(),arg11.size());
            posComposeBuffer+=arg11.size();
            break;
            case 11:
            memcpy(composeBuffer+posComposeBuffer,arg12.data(),arg12.size());
            posComposeBuffer+=arg12.size();
            break;
            case 12:
            memcpy(composeBuffer+posComposeBuffer,arg13.data(),arg13.size());
            posComposeBuffer+=arg13.size();
            break;
            case 13:
            memcpy(composeBuffer+posComposeBuffer,arg14.data(),arg14.size());
            posComposeBuffer+=arg14.size();
            break;
            case 14:
            memcpy(composeBuffer+posComposeBuffer,arg15.data(),arg15.size());
            posComposeBuffer+=arg15.size();
            break;
            default:
            break;
        }
        memcpy(composeBuffer+posComposeBuffer,preparedQuery+pos+2,*reinterpret_cast<uint16_t *>(preparedQuery+pos));
        posComposeBuffer+=preparedQuery[pos];
        pos+=2+preparedQuery[pos];
        ++index;
    }
    return std::string(composeBuffer,posComposeBuffer);
}

