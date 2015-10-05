#include "cpp11addition.h"

static const std::regex isaunsignednumber("^[0-9]+$");
static const std::regex isasignednumber("^-?[0-9]+$");

static const char* const lut = "0123456789ABCDEF";

std::size_t pairhash::operator()(const std::pair<uint8_t, uint8_t> &x) const
{
    return (x.first << 8) + x.second;
}

bool stringreplaceOne(std::string& str, const std::string& from, const std::string& to)
{
    const size_t start_pos = str.find(from);
    if(start_pos == std::string::npos)
        return false;
    str.replace(start_pos, from.length(), to);
    return true;
}

uint8_t stringreplaceAll(std::string& str, const std::string& from, const std::string& to)
{
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

std::vector<std::string> stringregexsplit(const std::string& input, const std::regex& regex)
{
    // passing -1 as the submatch index parameter performs splitting
    std::sregex_token_iterator
        first{input.begin(), input.end(), regex, -1},
        last;
    return {first, last};
}

std::vector<std::string> &stringsplit(const std::string &s, char delim, std::vector<std::string> &elems)
{
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        elems.push_back(item);
    }
    return elems;
}


std::vector<std::string> stringsplit(const std::string &s, char delim)
{
    std::vector<std::string> elems;
    stringsplit(s, delim, elems);
    return elems;
}

bool stringEndsWith(std::string const &fullString, std::string const &ending)
{
    if (fullString.length() >= ending.length()) {
        return (0 == fullString.compare (fullString.length() - ending.length(), ending.length(), ending));
    } else {
        return false;
    }
}

bool stringEndsWith(std::string const &fullString, char const &ending)
{
    if (fullString.length()>0) {
        return fullString[fullString.size()-1]==ending;
    } else {
        return false;
    }
}

bool stringStartWith(std::string const &fullString, std::string const &starting)
{
    if (fullString.length() >= starting.length()) {
        return (fullString.substr(0,starting.length())==starting);
    } else {
        return false;
    }
}

bool stringStartWith(std::string const &fullString, char const &starting)
{
    if (fullString.length()>0) {
        return fullString[0]==starting;
    } else {
        return false;
    }
}

std::string& stringimplode(const std::vector<std::string>& elems, char delim, std::string& s)
{
    for (std::vector<std::string>::const_iterator ii = elems.begin(); ii != elems.cend(); ++ii)
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

std::string stringimplode(const std::queue<std::string>& elems, char delim)
{
    std::string newString;
    std::queue<std::string> copy=elems;
    unsigned int count=0;
    while(!copy.empty())
    {
        if(count>0)
            newString+=delim;
        newString+=copy.front();
        copy.pop();
        ++count;
    }

    return newString;
}

std::string stringimplode(const std::vector<std::string>& elems, const std::string &delim)
{
    std::string newString;
    for (std::vector<std::string>::const_iterator ii = elems.begin(); ii != elems.cend(); ++ii)
    {
        newString += (*ii);
        if ( ii + 1 != elems.end() ) {
            newString += delim;
        }
    }
    return newString;
}

uint8_t stringtouint8(const std::string &string,bool *ok)
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

uint16_t stringtouint16(const std::string &string,bool *ok)
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

uint32_t stringtouint32(const std::string &string,bool *ok)
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
    if(string=="true" || string=="TRUE")
    {
        if(ok!=NULL)
            *ok=true;
        return true;
    }
    if(string=="false" || string=="FALSE")
    {
        if(ok!=NULL)
            *ok=true;
        return false;
    }
    if(ok!=NULL)
        *ok=false;
    return false;
}

uint64_t stringtouint64(const std::string &string,bool *ok)
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

int8_t stringtoint8(const std::string &string,bool *ok)
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

int16_t stringtoint16(const std::string &string,bool *ok)
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

int32_t stringtoint32(const std::string &string,bool *ok)
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

int64_t stringtoint64(const std::string &string,bool *ok)
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

std::string binarytoHexa(const std::vector<char> &data)
{
    std::string output;
    output.reserve(2*data.size());
    for(size_t i=0;i<data.size();++i)
    {
        const unsigned char c = data[i];
        output.push_back(lut[c >> 4]);
        output.push_back(lut[c & 15]);
    }
    return output;
}

std::string binarytoHexa(const char * const data,const uint32_t &size)
{
    std::string output;
    output.reserve(2*size);
    for(size_t i=0;i<size;++i)
    {
        const unsigned char c = data[i];
        output.push_back(lut[c >> 4]);
        output.push_back(lut[c & 15]);
    }
    return output;
}

std::vector<char> hexatoBinary(const std::string &data)
{
    std::vector<char> out;
    out.resize(data.length()/2);
    for(size_t i = 0; i < data.length(); i += 2) {
        std::istringstream strm(data.substr(i, 2));
        uint8_t x;
        strm >> std::hex >> x;
        out.push_back(x);
    }
    return out;
}

void binaryAppend(std::vector<char> &data,const std::vector<char> &add)
{
    if(add.empty())
        return;
    if(data.empty())
    {
        data=add;
        return;
    }
    const int oldsize=data.size();
    data.resize(oldsize+add.size());
    memcpy(data.data()+oldsize,add.data(),add.size());
}

void binaryAppend(std::vector<char> &data,const char * const add,const uint32_t &addSize)
{
    if(addSize==0)
        return;
    if(data.empty())
    {
        data.resize(addSize);
        memcpy(data.data(),add,addSize);
        return;
    }
    const int oldsize=data.size();
    data.resize(oldsize+addSize);
    memcpy(data.data()+oldsize,add,addSize);
}
