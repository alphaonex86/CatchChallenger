#include "Gen3Text.hpp"

#include <cctype>

static char gen3Char(uint8_t b)
{
    if(b==0x00)
        return ' ';
    if(b>=0xA1 && b<=0xAA)
        return static_cast<char>('0'+(b-0xA1));
    if(b>=0xBB && b<=0xD4)
        return static_cast<char>('A'+(b-0xBB));
    if(b>=0xD5 && b<=0xEE)
        return static_cast<char>('a'+(b-0xD5));
    switch(b)
    {
        case 0x1B: return 'e'; // accented e (POKeMON)
        case 0xAD: return '.';
        case 0xAE: return '-';
        case 0xB4: return '\'';
        case 0xBA: return '/';
        case 0xAC: return ',';
        case 0xFC: return ' '; // RSE two-line layout control -> space
        default: return 0;     // drop everything else (control bytes)
    }
}

std::string Gen3Text::decode(const GbaRom &rom, uint32_t offset, size_t maxLen)
{
    std::string out;
    size_t n=0;
    while(n<maxLen)
    {
        uint8_t b=rom.u8(offset+static_cast<uint32_t>(n));
        if(b==0xFF)
            break;
        char c=gen3Char(b);
        if(c!=0)
            out.push_back(c);
        n++;
    }
    // trim trailing spaces
    while(!out.empty() && out.back()==' ')
        out.pop_back();
    return out;
}

std::string Gen3Text::slug(const std::string &s)
{
    std::string out;
    bool lastDash=false;
    size_t i=0;
    while(i<s.size())
    {
        char c=s[i];
        if((c>='A' && c<='Z') || (c>='a' && c<='z') || (c>='0' && c<='9'))
        {
            out.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
            lastDash=false;
        }
        else
        {
            // any separator/punctuation collapses to a single dash
            if(!lastDash && !out.empty())
            {
                out.push_back('-');
                lastDash=true;
            }
        }
        i++;
    }
    while(!out.empty() && out.back()=='-')
        out.pop_back();
    if(out.empty())
        out="x";
    return out;
}

std::string Gen3Text::display(const std::string &s)
{
    // Decoded names are ALL-CAPS in the ROM; present as Title Case.
    std::string out;
    bool startWord=true;
    size_t i=0;
    while(i<s.size())
    {
        char c=s[i];
        if(c==' ' || c=='-' || c=='/')
        {
            out.push_back(c);
            startWord=true;
        }
        else
        {
            if(startWord)
                out.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(c))));
            else
                out.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
            startWord=false;
        }
        i++;
    }
    return out;
}
