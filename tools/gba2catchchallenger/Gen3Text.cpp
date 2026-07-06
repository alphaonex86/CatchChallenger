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
        case 0xB8: return ',';
        case 0xBA: return '/';
        case 0xAC: return '?';
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

std::string Gen3Text::strictName(const GbaRom &rom, uint32_t offset, size_t maxLen)
{
    std::string out;
    size_t n=0;
    while(n<maxLen)
    {
        uint8_t b=rom.u8(offset+static_cast<uint32_t>(n));
        if(b==0xFF)
            break;
        char c=gen3Char(b);
        if(c==0)
            return std::string();   // any control/invalid byte => not a name
        out.push_back(c);
        n++;
    }
    // must terminate within maxLen and be at least 2 chars (no 1-letter "names")
    if(n<2 || n>=maxLen || rom.u8(offset+static_cast<uint32_t>(n))!=0xFF)
        return std::string();
    while(!out.empty() && out.back()==' ')
        out.pop_back();
    return out;
}

// Fuller charmap for sign text (adds punctuation the name decoder doesn't need).
static char signChar(uint8_t b)
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
        case 0xAB: return '!';
        case 0xAC: return '?';
        case 0xAD: return '.';
        case 0xAE: return '-';
        case 0xB1: return '"';
        case 0xB2: return '"';
        case 0xB3: return '\'';
        case 0xB4: return '\'';
        case 0xB8: return ',';
        case 0xBA: return '/';
        default:   return 0;   // drop other control/symbol bytes
    }
}

static std::string trimSignPage(std::string s)
{
    bool changed=true;
    const std::string br="<br />";
    while(changed)
    {
        changed=false;
        while(!s.empty() && s.back()==' ') { s.pop_back(); changed=true; }
        if(s.size()>=br.size() && s.compare(s.size()-br.size(),br.size(),br)==0)
        { s.erase(s.size()-br.size()); changed=true; }
    }
    size_t i=0;
    while(i<s.size() && s[i]==' ')
        i++;
    return s.substr(i);
}

std::vector<std::string> Gen3Text::decodeSign(const GbaRom &rom, uint32_t offset, size_t maxLen)
{
    std::vector<std::string> pages;
    std::string cur;
    size_t n=0;
    while(n<maxLen)
    {
        uint8_t b=rom.u8(offset+static_cast<uint32_t>(n));
        if(b==0xFF)
            break;                          // end of string
        else if(b==0xFB || b==0xFA)         // paragraph / scroll -> next page
        {
            std::string t=trimSignPage(cur);
            if(!t.empty())
                pages.push_back(t);
            cur.clear();
        }
        else if(b==0xFE)                    // newline
            cur+="<br />";
        else if(b==0xB0)                    // ellipsis
            cur+="...";
        else if(b==0xFD)                    // placeholder: next byte = variable id
        {
            uint8_t id=rom.u8(offset+static_cast<uint32_t>(n)+1);
            if(id==0xFF)
                break;                      // dangling 0xFD: don't eat the terminator
            if(id==0x01)
                cur+="Player";              // PLAYER name (dynamic)
            else if(id==0x06)
                cur+="Rival";               // RIVAL name (dynamic)
            // else: a script-set STR_VAR (monster/item/number) we can't resolve
            n++;                            // skip the id byte
        }
        else if(b==0xFC)                    // extended formatting control: cmd + args
        {
            uint8_t cmd=rom.u8(offset+static_cast<uint32_t>(n)+1);
            size_t args=1;                  // most commands take 1 argument byte
            if(cmd==0x04)                   // COLOR_HIGHLIGHT_SHADOW
                args=3;
            else if(cmd==0x0B)              // PLAY_BGM (u16)
                args=2;
            else if(cmd==0x07 || cmd==0x09 || cmd==0x0A) // RESET_SIZE/PAUSE_UNTIL_PRESS/WAIT_SE
                args=0;
            n+=1+args;                      // skip the cmd byte + its arguments
        }
        else
        {
            char c=signChar(b);
            if(c!=0)
                cur.push_back(c);
        }
        n++;
    }
    std::string t=trimSignPage(cur);
    if(!t.empty())
        pages.push_back(t);
    return pages;
}

std::string Gen3Text::decodeParagraph(const GbaRom &rom, uint32_t offset, size_t maxLen)
{
    std::string out;
    size_t n=0;
    while(n<maxLen)
    {
        uint8_t b=rom.u8(offset+static_cast<uint32_t>(n));
        if(b==0xFF)
            break;                          // end of string
        else if(b==0xFE || b==0xFA || b==0xFB) // newline / scroll / paragraph
        {
            if(!out.empty() && out.back()!=' ')
                out.push_back(' ');
        }
        else if(b==0xB0)                    // ellipsis
            out+="...";
        else if(b==0xFD)                    // placeholder: skip the variable id
            n++;
        else if(b==0xFC)                    // formatting control: skip cmd + arg
            n++;
        else
        {
            char c=signChar(b);
            if(c!=0)
            {
                if(c!=' ' || out.empty() || out.back()!=' ')
                    out.push_back(c);
            }
        }
        n++;
    }
    while(!out.empty() && out.back()==' ')
        out.pop_back();
    return out;
}

std::string Gen3Text::speciesName(const GbaRom &rom, uint16_t internalId)
{
    const GameInfo &gi=rom.game();
    if(gi.speciesNames==0 || internalId==0)
        return std::string();
    return display(decode(rom,gi.speciesNames+static_cast<uint32_t>(internalId)*11,11));
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
