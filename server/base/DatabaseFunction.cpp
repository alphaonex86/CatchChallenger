#include "DatabaseFunction.h"
#include "../../general/base/cpp11addition.h"
#include "../../general/base/GeneralVariable.h"
#include "../VariableServer.h"

#ifdef CATCHCHALLENGER_EXTRA_CHECK
#include <iostream>
#endif

using namespace CatchChallenger;

#ifdef CATCHCHALLENGER_SERVER_TRUSTINTODATABASENUMBERRETURN
uint8_t DatabaseFunction::stringtouint8(const std::string &string,bool *ok)
{
    if(ok!=NULL)
        *ok=true;
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(string.empty())
    {
        std::cerr << "string.empty() for database convertion to number" << std::endl;
        if(ok!=NULL)
            *ok=false;
        abort();
    }
    #endif
    return static_cast<uint8_t>(std::stoul(string));
}

uint16_t DatabaseFunction::stringtouint16(const std::string &string,bool *ok)
{
    if(ok!=NULL)
        *ok=true;
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(string.empty())
    {
        std::cerr << "string.empty() for database convertion to number" << std::endl;
        if(ok!=NULL)
            *ok=false;
        abort();
    }
    #endif
    return static_cast<uint16_t>(std::stoul(string));
}

uint32_t DatabaseFunction::stringtouint32(const std::string &string,bool *ok)
{
    if(ok!=NULL)
        *ok=true;
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(string.empty())
    {
        std::cerr << "string.empty() for database convertion to number" << std::endl;
        if(ok!=NULL)
            *ok=false;
        abort();
    }
    #endif
    return static_cast<uint32_t>(std::stoul(string));
}

bool DatabaseFunction::stringtobool(const std::string &string,bool *ok)
{
    if(ok!=NULL)
        *ok=true;
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(string.empty())
    {
        std::cerr << "string.empty() for database convertion to number" << std::endl;
        if(ok!=NULL)
            *ok=false;
        abort();
    }
    #endif
    if(string=="1" || string=="true" || string=="t" /*postgresql*/ || string=="TRUE")
        return true;
    else
        return false;
}

uint64_t DatabaseFunction::stringtouint64(const std::string &string,bool *ok)
{
    if(ok!=NULL)
        *ok=true;
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(string.empty())
    {
        std::cerr << "string.empty() for database convertion to number" << std::endl;
        if(ok!=NULL)
            *ok=false;
        abort();
    }
    #endif
    return std::stoull(string);
}

int8_t DatabaseFunction::stringtoint8(const std::string &string,bool *ok)
{
    if(ok!=NULL)
        *ok=true;
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(string.empty())
    {
        std::cerr << "string.empty() for database convertion to number" << std::endl;
        if(ok!=NULL)
            *ok=false;
        abort();
    }
    #endif
    return static_cast<int8_t>(std::stoi(string));
}

int16_t DatabaseFunction::stringtoint16(const std::string &string,bool *ok)
{
    if(ok!=NULL)
        *ok=true;
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(string.empty())
    {
        std::cerr << "string.empty() for database convertion to number" << std::endl;
        if(ok!=NULL)
            *ok=false;
        abort();
    }
    #endif
    return static_cast<int16_t>(std::stoi(string));
}

int32_t DatabaseFunction::stringtoint32(const std::string &string,bool *ok)
{
    if(ok!=NULL)
        *ok=true;
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(string.empty())
    {
        std::cerr << "string.empty() for database convertion to number" << std::endl;
        if(ok!=NULL)
            *ok=false;
        abort();
    }
    #endif
    return std::stoi(string);
}

int64_t DatabaseFunction::stringtoint64(const std::string &string,bool *ok)
{
    if(ok!=NULL)
        *ok=true;
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(string.empty())
    {
        std::cerr << "string.empty() for database convertion to number" << std::endl;
        if(ok!=NULL)
            *ok=false;
        abort();
    }
    #endif
    return std::stoll(string);
}

float DatabaseFunction::stringtofloat(const std::string &string,bool *ok)
{
    if(ok!=NULL)
        *ok=true;
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(string.empty())
    {
        std::cerr << "string.empty() for database convertion to number" << std::endl;
        if(ok!=NULL)
            *ok=false;
        abort();
    }
    #endif
    return std::stof(string);
}

double DatabaseFunction::stringtodouble(const std::string &string,bool *ok)
{
    if(ok!=NULL)
        *ok=true;
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(string.empty())
    {
        std::cerr << "string.empty() for database convertion to number" << std::endl;
        if(ok!=NULL)
            *ok=false;
        abort();
    }
    #endif
    return std::stod(string);
}

std::vector<char> DatabaseFunction::hexatoBinary(const std::string &data,bool *ok)
{
    if(data.size()%2!=0)
    {
        if(ok!=NULL)
            *ok=false;
        return std::vector<char>();
    }
    bool ok2;
    std::vector<char> out;
    out.reserve(data.length()/2);
    for(size_t i=0;i<data.length();i+=2)
    {
        const std::string &partpfchain=data.substr(i,2);
        const uint8_t &x=::hexToDecUnit(partpfchain,&ok2);
        if(!ok2)
        {
            if(ok!=NULL)
                *ok=false;
            return std::vector<char>();
        }
        out.push_back(x);
    }
    if(ok!=NULL)
        *ok=true;
    return out;
}
#else
uint8_t DatabaseFunction::stringtouint8(const std::string &string,bool *ok)
{
    return ::stringtouint8(string,ok);
}

uint16_t DatabaseFunction::stringtouint16(const std::string &string,bool *ok)
{
    return ::stringtouint16(string,ok);
}

uint32_t DatabaseFunction::stringtouint32(const std::string &string,bool *ok)
{
    return ::stringtouint32(string,ok);
}

bool DatabaseFunction::stringtobool(const std::string &string,bool *ok)
{
    return ::stringtobool(string,ok);
}

uint64_t DatabaseFunction::stringtouint64(const std::string &string,bool *ok)
{
    return ::stringtouint64(string,ok);
}

int8_t DatabaseFunction::stringtoint8(const std::string &string,bool *ok)
{
    return ::stringtoint8(string,ok);
}

int16_t DatabaseFunction::stringtoint16(const std::string &string,bool *ok)
{
    return ::stringtoint16(string,ok);
}

int32_t DatabaseFunction::stringtoint32(const std::string &string,bool *ok)
{
    return ::stringtoint32(string,ok);
}

int64_t DatabaseFunction::stringtoint64(const std::string &string,bool *ok)
{
    return ::stringtoint64(string,ok);
}

float DatabaseFunction::stringtofloat(const std::string &string,bool *ok)
{
    return ::stringtofloat(string,ok);
}

double DatabaseFunction::stringtodouble(const std::string &string,bool *ok)
{
    return ::stringtodouble(string,ok);
}

std::vector<char> DatabaseFunction::hexatoBinary(const std::string &data,bool *ok)
{
    return ::hexatoBinary(data);
}
#endif
