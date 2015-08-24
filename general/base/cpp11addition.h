#ifndef CATCHCHALLENGER_CPP11ADDITION_H
#define CATCHCHALLENGER_CPP11ADDITION_H

#include <vector>
#include <string>
#include <regex>

#if ! defined(Q_LIKELY)
    #if defined(__GNUC__)
    #  define Q_LIKELY(expr)    __builtin_expect(!!(expr), true)
    #  define Q_UNLIKELY(expr)  __builtin_expect(!!(expr), false)
    #else
    #  define Q_LIKELY(x) (x)
    #endif
#endif

std::regex isaunsignednumber("^[0-9]+$");
std::regex isasignednumber("^-?[0-9]+$");

struct pairhash {
public:
  std::size_t operator()(const std::pair<uint8_t, uint8_t> &x) const
  {
      return (x.first << 8) + x.second;
  }
};

bool stringreplace(std::string& str, const std::string& from, const std::string& to) {
    size_t start_pos = str.find(from);
    if(start_pos == std::string::npos)
        return false;
    str.replace(start_pos, from.length(), to);
    return true;
}

uint8_t stringreplaceAll(std::string& str, const std::string& from, const std::string& to) {
    if(from.empty())
        return 0;
    size_t start_pos = 0;
    uint8_t count=0;
    while((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
        count++;
    }
    return count;
}

std::vector<std::string> stringregexsplit(const std::string& input, const std::regex& regex) {
    // passing -1 as the submatch index parameter performs splitting
    std::sregex_token_iterator
        first{input.begin(), input.end(), regex, -1},
        last;
    return {first, last};
}

std::vector<std::string> &stringsplit(const std::string &s, char delim, std::vector<std::string> &elems) {
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        elems.push_back(item);
    }
    return elems;
}


std::vector<std::string> stringsplit(const std::string &s, char delim) {
    std::vector<std::string> elems;
    stringsplit(s, delim, elems);
    return elems;
}

bool stringEndsWith(std::string const &fullString, std::string const &ending) {
    if (fullString.length() >= ending.length()) {
        return (0 == fullString.compare (fullString.length() - ending.length(), ending.length(), ending));
    } else {
        return false;
    }
}

std::string& stringimplode(const std::vector<std::string>& elems, char delim, std::string& s)
{
    for (std::vector<std::string>::const_iterator ii = elems.begin(); ii != elems.end(); ++ii)
    {
        s += (*ii);
        if ( ii + 1 != elems.end() ) {
            s += delim;
        }
    }

    return s;
}

std::string stringimplode(const std::vector<std::string>& elems, char delim)
{
    std::string s;
    return stringimplode(elems, delim, s);
}

uint8_t stringtouint8(const std::string &string,bool *ok=NULL)
{
    if(Q_LIKELY(std::regex_match(string,isaunsignednumber)))
    {
        if(Q_LIKELY(ok!=NULL))
            *ok=true;
        const unsigned int &tempValue=std::stoull(string);
        if(Q_LIKELY(tempValue<=0xFF))
            return tempValue;
        else
        {
            if(ok!=NULL)
                *ok=false;
            return 0;
        }
    }
    else
    {
        if(ok!=NULL)
            *ok=false;
        return 0;
    }
}

uint16_t stringtouint16(const std::string &string,bool *ok=NULL)
{
    if(Q_LIKELY(std::regex_match(string,isaunsignednumber)))
    {
        if(Q_LIKELY(ok!=NULL))
            *ok=true;
        const unsigned int &tempValue=std::stoull(string);
        if(Q_LIKELY(tempValue<=0xFFFF))
            return tempValue;
        else
        {
            if(ok!=NULL)
                *ok=false;
            return 0;
        }
    }
    else
    {
        if(ok!=NULL)
            *ok=false;
        return 0;
    }
}

uint32_t stringtouint32(const std::string &string,bool *ok=NULL)
{
    if(Q_LIKELY(std::regex_match(string,isaunsignednumber)))
    {
        if(Q_LIKELY(ok!=NULL))
            *ok=true;
        const unsigned int &tempValue=std::stoull(string);
        if(Q_LIKELY(tempValue<=0xFFFFFFFF))
            return tempValue;
        else
        {
            if(ok!=NULL)
                *ok=false;
            return 0;
        }
    }
    else
    {
        if(ok!=NULL)
            *ok=false;
        return 0;
    }
}

uint64_t stringtouint64(const std::string &string,bool *ok=NULL)
{
    if(Q_LIKELY(std::regex_match(string,isaunsignednumber)))
    {
        if(Q_LIKELY(ok!=NULL))
            *ok=true;
        return std::stoull(string);
    }
    else
    {
        if(ok!=NULL)
            *ok=false;
        return 0;
    }
}

int8_t stringtoint8(const std::string &string,bool *ok=NULL)
{
    if(Q_LIKELY(std::regex_match(string,isasignednumber)))
    {
        if(Q_LIKELY(ok!=NULL))
            *ok=true;
        const unsigned int &tempValue=std::stoi(string);
        if(Q_LIKELY(tempValue<=0x7F))
            return tempValue;
        else
        {
            if(ok!=NULL)
                *ok=false;
            return 0;
        }
    }
    else
    {
        if(ok!=NULL)
            *ok=false;
        return 0;
    }
}

int16_t stringtoint16(const std::string &string,bool *ok=NULL)
{
    if(Q_LIKELY(std::regex_match(string,isasignednumber)))
    {
        if(Q_LIKELY(ok!=NULL))
            *ok=true;
        const unsigned int &tempValue=std::stoi(string);
        if(Q_LIKELY(tempValue<=0x7FFF))
            return tempValue;
        else
        {
            if(ok!=NULL)
                *ok=false;
            return 0;
        }
    }
    else
    {
        if(ok!=NULL)
            *ok=false;
        return 0;
    }
}

int32_t stringtoint32(const std::string &string,bool *ok=NULL)
{
    if(Q_LIKELY(std::regex_match(string,isasignednumber)))
    {
        if(Q_LIKELY(ok!=NULL))
            *ok=true;
        return std::stoi(string);
    }
    else
    {
        if(ok!=NULL)
            *ok=false;
        return 0;
    }
}

int64_t stringtoint64(const std::string &string,bool *ok=NULL)
{
    if(Q_LIKELY(std::regex_match(string,isasignednumber)))
    {
        if(Q_LIKELY(ok!=NULL))
            *ok=true;
        return std::stoll(string);
    }
    else
    {
        if(ok!=NULL)
            *ok=false;
        return 0;
    }
}

#endif
