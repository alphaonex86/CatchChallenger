#include "cpp11addition.hpp"
#include "GeneralVariable.hpp"
#include <cstdio>
#include <stdexcept>
#ifdef CATCHCHALLENGER_HARDENED
#include <iostream>
#endif

/// \todo, check number validity by http://en.cppreference.com/w/c/string/byte/strtol
/* to check: bool my_strtol(const std::string &str, long &v) {
char *end = nullptr;
v = strtol(str.c_str(), &end, 10);
return end != nullptr;
}*/

// Hand-coded equivalents of the three classic numeric-validation regexes.
// std::regex_match against "^[0-9]+$" / "^-?[0-9]+$" / "^-?[0-9]+(\.[0-9]+)?$"
// shows up at ~10% of CPU time during datapack parse (FightLoader::loadMonster
// calls stringtouint16 per XML attribute). The regex engine has to construct
// an executor on every call; a single linear pass is two orders of magnitude
// faster and matches the same set of strings byte-for-byte.
static inline bool is_a_unsignednumber(const std::string &s)
{
    const size_t n=s.size();
    if(n==0) return false;
    const char *p=s.data();
    size_t i=0;
    while(i<n)
    {
        const unsigned char c=static_cast<unsigned char>(p[i]);
        if(c<'0' || c>'9') return false;
        ++i;
    }
    return true;
}
static inline bool is_a_signednumber(const std::string &s)
{
    const size_t n=s.size();
    if(n==0) return false;
    const char *p=s.data();
    size_t i=0;
    if(p[0]=='-') i=1;
    if(i>=n) return false;//"-" alone isn't a number
    while(i<n)
    {
        const unsigned char c=static_cast<unsigned char>(p[i]);
        if(c<'0' || c>'9') return false;
        ++i;
    }
    return true;
}
static inline bool is_a_double(const std::string &s)
{
    const size_t n=s.size();
    if(n==0) return false;
    const char *p=s.data();
    size_t i=0;
    if(p[0]=='-') i=1;
    if(i>=n) return false;
    bool sawDigit=false;
    while(i<n && p[i]>='0' && p[i]<='9') { sawDigit=true; ++i; }
    if(!sawDigit) return false;
    if(i==n) return true;
    if(p[i]!='.') return false;
    ++i;
    bool sawFrac=false;
    while(i<n && p[i]>='0' && p[i]<='9') { sawFrac=true; ++i; }
    return sawFrac && i==n;
}

uint8_t stringtouint8(const std::string &string,bool *ok)
{
    #ifdef __EXCEPTIONS
    unsigned int tempValue;
    try {
      tempValue = static_cast<unsigned int>(std::stoul(string));
    }
    catch(...) {
        #ifdef CATCHCHALLENGER_HARDENED
        std::cerr << "Convertion failed and repported at " << __FILE__ << ":" << __LINE__ << std::endl;
        #endif
        if(Q_LIKELY(ok!=NULL))
            *ok=false;
        return 0;
    }
    if(Q_LIKELY(tempValue<=0xFF))
    {
        if(Q_LIKELY(ok!=NULL))
            *ok=true;
        return static_cast<uint8_t>(tempValue);
    }
    else
    {
        #ifdef CATCHCHALLENGER_HARDENED
        std::cerr << "Convertion failed and repported at " << __FILE__ << ":" << __LINE__ << std::endl;
        #endif
        if(ok!=NULL)
            *ok=false;
        return 0;
    }
    #else
    if(Q_LIKELY(is_a_unsignednumber(string)))
    {
        if(Q_LIKELY(ok!=NULL))
            *ok=true;
        const unsigned int &tempValue=std::stoul(string);
        if(Q_LIKELY(tempValue<=0xFF))
            return tempValue;
        else
        {
            #ifdef CATCHCHALLENGER_HARDENED
            std::cerr << "Convertion failed and repported at " << __FILE__ << ":" << __LINE__ << std::endl;
            #endif
            if(ok!=NULL)
                *ok=false;
            return 0;
        }
    }
    else
    {
        #ifdef CATCHCHALLENGER_HARDENED
        std::cerr << "Convertion failed and repported at " << __FILE__ << ":" << __LINE__ << std::endl;
        #endif
        if(ok!=NULL)
            *ok=false;
        return 0;
    }
    #endif
}

uint16_t stringtouint16(const std::string &string,bool *ok)
{
    #ifdef __EXCEPTIONS
    unsigned int tempValue;
    try {
      tempValue = static_cast<unsigned int>(std::stoul(string));
    }
    catch(...) {
        #ifdef CATCHCHALLENGER_HARDENED
        std::cerr << "Convertion failed and repported at " << __FILE__ << ":" << __LINE__ << std::endl;
        #endif
        if(Q_LIKELY(ok!=NULL))
            *ok=false;
        return 0;
    }
    if(Q_LIKELY(tempValue<=0xFFFF))
    {
        if(Q_LIKELY(ok!=NULL))
            *ok=true;
        return static_cast<uint16_t>(tempValue);
    }
    else
    {
        #ifdef CATCHCHALLENGER_HARDENED
        std::cerr << "Convertion failed and repported at " << __FILE__ << ":" << __LINE__ << std::endl;
        #endif
        if(ok!=NULL)
            *ok=false;
        return 0;
    }
    #else
    if(Q_LIKELY(is_a_unsignednumber(string)))
    {
        if(Q_LIKELY(ok!=NULL))
            *ok=true;
        const unsigned int &tempValue=std::stoul(string);
        if(Q_LIKELY(tempValue<=0xFFFF))
            return tempValue;
        else
        {
            #ifdef CATCHCHALLENGER_HARDENED
            std::cerr << "Convertion failed and repported at " << __FILE__ << ":" << __LINE__ << std::endl;
            #endif
            if(ok!=NULL)
                *ok=false;
            return 0;
        }
    }
    else
    {
        #ifdef CATCHCHALLENGER_HARDENED
        std::cerr << "Convertion failed and repported at " << __FILE__ << ":" << __LINE__ << std::endl;
        #endif
        if(ok!=NULL)
            *ok=false;
        return 0;
    }
    #endif
}

uint32_t stringtouint32(const std::string &string,bool *ok)
{
    #ifdef __EXCEPTIONS
    uint32_t tempValue;
    try {
      tempValue = static_cast<unsigned int>(std::stoul(string));
    }
    catch(...) {
        #ifdef CATCHCHALLENGER_HARDENED
        std::cerr << "Convertion failed and repported at " << __FILE__ << ":" << __LINE__ << std::endl;
        #endif
        if(Q_LIKELY(ok!=NULL))
            *ok=false;
        return 0;
    }
    if(Q_LIKELY(ok!=NULL))
        *ok=true;
    return tempValue;
    #else
    if(Q_LIKELY(is_a_unsignednumber(string)))
    {
        if(Q_LIKELY(ok!=NULL))
            *ok=true;
        const unsigned int &tempValue=std::stoull(string);
        if(Q_LIKELY(tempValue<=0xFFFFFFFF))
            return tempValue;
        else
        {
            #ifdef CATCHCHALLENGER_HARDENED
            std::cerr << "Convertion failed and repported at " << __FILE__ << ":" << __LINE__ << std::endl;
            #endif
            if(ok!=NULL)
                *ok=false;
            return 0;
        }
    }
    else
    {
        #ifdef CATCHCHALLENGER_HARDENED
        std::cerr << "Convertion failed and repported at " << __FILE__ << ":" << __LINE__ << std::endl;
        #endif
        if(ok!=NULL)
            *ok=false;
        return 0;
    }
    #endif
}

bool stringtobool(const std::string &string,bool *ok)
{
    if(string=="1")
    {
        if(ok!=NULL)
            *ok=true;
        return true;
    }
    if(string=="0")
    {
        if(ok!=NULL)
            *ok=true;
        return false;
    }
    if(string=="true" || string=="t" /*postgresql*/ || string=="TRUE")
    {
        if(ok!=NULL)
            *ok=true;
        return true;
    }
    if(string=="false" || string=="f" /*postgresql*/ || string=="FALSE")
    {
        if(ok!=NULL)
            *ok=true;
        return false;
    }
    #ifdef CATCHCHALLENGER_HARDENED
    std::cerr << "Convertion failed and repported at " << __FILE__ << ":" << __LINE__ << std::endl;
    #endif
    if(ok!=NULL)
        *ok=false;
    return false;
}

uint64_t stringtouint64(const std::string &string,bool *ok)
{
    #ifdef __EXCEPTIONS
    uint64_t tempValue;
    try {
      tempValue = std::stoull(string);
    }
    catch(...) {
        #ifdef CATCHCHALLENGER_HARDENED
        std::cerr << "Convertion failed and repported at " << __FILE__ << ":" << __LINE__ << std::endl;
        #endif
        if(Q_LIKELY(ok!=NULL))
            *ok=false;
        return 0;
    }
    if(Q_LIKELY(ok!=NULL))
        *ok=true;
    return tempValue;
    #else
    if(Q_LIKELY(is_a_unsignednumber(string)))
    {
        if(Q_LIKELY(ok!=NULL))
            *ok=true;
        return std::stoull(string);
    }
    else
    {
        #ifdef CATCHCHALLENGER_HARDENED
        std::cerr << "Convertion failed and repported at " << __FILE__ << ":" << __LINE__ << std::endl;
        #endif
        if(ok!=NULL)
            *ok=false;
        return 0;
    }
    #endif
}

int8_t stringtoint8(const std::string &string,bool *ok)
{
    #ifdef __EXCEPTIONS
    int tempValue;
    try {
      tempValue = static_cast<int>(std::stol(string));
    }
    catch(...) {
        #ifdef CATCHCHALLENGER_HARDENED
        std::cerr << "Convertion failed and repported at " << __FILE__ << ":" << __LINE__ << std::endl;
        #endif
        if(Q_LIKELY(ok!=NULL))
            *ok=false;
        return 0;
    }
    if(Q_LIKELY(tempValue<=0x7F && tempValue>=-0x7F))
    {
        if(Q_LIKELY(ok!=NULL))
            *ok=true;
        return static_cast<int8_t>(tempValue);
    }
    else
    {
        #ifdef CATCHCHALLENGER_HARDENED
        std::cerr << "Convertion failed and repported at " << __FILE__ << ":" << __LINE__ << std::endl;
        #endif
        if(ok!=NULL)
            *ok=false;
        return 0;
    }
    #else
    if(Q_LIKELY(is_a_signednumber(string)))
    {
        if(Q_LIKELY(ok!=NULL))
            *ok=true;
        const unsigned int &tempValue=std::stoi(string);
        if(Q_LIKELY(tempValue<=0x7F))
            return tempValue;
        else
        {
            #ifdef CATCHCHALLENGER_HARDENED
            std::cerr << "Convertion failed and repported at " << __FILE__ << ":" << __LINE__ << std::endl;
            #endif
            if(ok!=NULL)
                *ok=false;
            return 0;
        }
    }
    else
    {
        #ifdef CATCHCHALLENGER_HARDENED
        std::cerr << "Convertion failed and repported at " << __FILE__ << ":" << __LINE__ << std::endl;
        #endif
        if(ok!=NULL)
            *ok=false;
        return 0;
    }
    #endif
}

int16_t stringtoint16(const std::string &string,bool *ok)
{
    #ifdef __EXCEPTIONS
    int tempValue;
    try {
      tempValue = static_cast<int>(std::stol(string));
    }
    catch(...) {
        #ifdef CATCHCHALLENGER_HARDENED
        std::cerr << "Convertion failed and repported at " << __FILE__ << ":" << __LINE__ << std::endl;
        #endif
        if(Q_LIKELY(ok!=NULL))
            *ok=false;
        return 0;
    }
    if(Q_LIKELY(tempValue<=0x7FFF && tempValue>=-0x7FFF))
    {
        if(Q_LIKELY(ok!=NULL))
            *ok=true;
        return static_cast<int16_t>(tempValue);
    }
    else
    {
        #ifdef CATCHCHALLENGER_HARDENED
        std::cerr << "Convertion failed and repported at " << __FILE__ << ":" << __LINE__ << std::endl;
        #endif
        if(ok!=NULL)
            *ok=false;
        return 0;
    }
    #else
    if(Q_LIKELY(is_a_signednumber(string)))
    {
        if(Q_LIKELY(ok!=NULL))
            *ok=true;
        const unsigned int &tempValue=std::stoi(string);
        if(Q_LIKELY(tempValue<=0x7FFF))
            return tempValue;
        else
        {
            #ifdef CATCHCHALLENGER_HARDENED
            std::cerr << "Convertion failed and repported at " << __FILE__ << ":" << __LINE__ << std::endl;
            #endif
            if(ok!=NULL)
                *ok=false;
            return 0;
        }
    }
    else
    {
        #ifdef CATCHCHALLENGER_HARDENED
        std::cerr << "Convertion failed and repported at " << __FILE__ << ":" << __LINE__ << std::endl;
        #endif
        if(ok!=NULL)
            *ok=false;
        return 0;
    }
    #endif
}

int32_t stringtoint32(const std::string &string,bool *ok)
{
    #ifdef __EXCEPTIONS
    int32_t tempValue;
    try {
      tempValue = static_cast<int>(std::stol(string));
    }
    catch(...) {
        #ifdef CATCHCHALLENGER_HARDENED
        std::cerr << "Convertion failed and repported at " << __FILE__ << ":" << __LINE__ << std::endl;
        #endif
        if(Q_LIKELY(ok!=NULL))
            *ok=false;
        return 0;
    }
    if(Q_LIKELY(ok!=NULL))
        *ok=true;
    return tempValue;
    #else
    if(Q_LIKELY(is_a_signednumber(string)))
    {
        if(Q_LIKELY(ok!=NULL))
            *ok=true;
        return std::stoi(string);
    }
    else
    {
        #ifdef CATCHCHALLENGER_HARDENED
        std::cerr << "Convertion failed and repported at " << __FILE__ << ":" << __LINE__ << std::endl;
        #endif
        if(ok!=NULL)
            *ok=false;
        return 0;
    }
    #endif
}

int64_t stringtoint64(const std::string &string,bool *ok)
{
    #ifdef __EXCEPTIONS
    int64_t tempValue;
    try {
      tempValue = std::stoll(string);
    }
    catch(...) {
        #ifdef CATCHCHALLENGER_HARDENED
        std::cerr << "Convertion failed and repported at " << __FILE__ << ":" << __LINE__ << std::endl;
        #endif
        if(Q_LIKELY(ok!=NULL))
            *ok=false;
        return 0;
    }
    if(Q_LIKELY(ok!=NULL))
        *ok=true;
    return tempValue;
    #else
    if(Q_LIKELY(is_a_signednumber(string)))
    {
        if(Q_LIKELY(ok!=NULL))
            *ok=true;
        return std::stoll(string);
    }
    else
    {
        #ifdef CATCHCHALLENGER_HARDENED
        std::cerr << "Convertion failed and repported at " << __FILE__ << ":" << __LINE__ << std::endl;
        #endif
        if(ok!=NULL)
            *ok=false;
        return 0;
    }
    #endif
}

float stringtofloat(const std::string &string,bool *ok)
{
    if(Q_LIKELY(is_a_double(string)))
    {
        if(Q_LIKELY(ok!=NULL))
            *ok=true;
        return std::stof(string);
    }
    else
    {
        #ifdef CATCHCHALLENGER_HARDENED
        std::cerr << "Convertion failed and repported at " << __FILE__ << ":" << __LINE__ << std::endl;
        #endif
        if(ok!=NULL)
            *ok=false;
        return 0;
    }
}

double stringtodouble(const std::string &string,bool *ok)
{
    if(Q_LIKELY(is_a_double(string)))
    {
        if(Q_LIKELY(ok!=NULL))
            *ok=true;
        return std::stod(string);
    }
    else
    {
        #ifdef CATCHCHALLENGER_HARDENED
        std::cerr << "Convertion failed and repported at " << __FILE__ << ":" << __LINE__ << std::endl;
        #endif
        if(ok!=NULL)
            *ok=false;
        return 0;
    }
}
