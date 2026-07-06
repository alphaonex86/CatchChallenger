#include "Localization.hpp"

#include <QDir>
#include <QFile>
#include <QFileInfoList>
#include <QTextStream>
#include <QString>

#include <algorithm>
#include <cctype>

namespace tuxemon {

Localization::Localization() {}

// Extract the contents between the first and last double quote of a .po line,
// unescaping \" \\ \n \t.
static std::string unquote(const std::string &line)
{
    const std::size_t a = line.find('"');
    const std::size_t b = line.rfind('"');
    if(a == std::string::npos || b == std::string::npos || b <= a)
        return std::string();
    std::string out;
    std::size_t i = a + 1;
    while(i < b)
    {
        const char c = line[i];
        if(c == '\\' && (i + 1) < b)
        {
            const char n = line[i + 1];
            if(n == 'n')      out.push_back('\n');
            else if(n == 't') out.push_back('\t');
            else if(n == '"') out.push_back('"');
            else if(n == '\\')out.push_back('\\');
            else              out.push_back(n);
            i += 2;
        }
        else
        {
            out.push_back(c);
            i += 1;
        }
    }
    return out;
}

void Localization::parsePo(const std::string &file,
                           std::unordered_map<std::string,std::string> &out)
{
    QFile f(QString::fromStdString(file));
    if(!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return;
    QTextStream in(&f);
    in.setEncoding(QStringConverter::Utf8);

    std::string curId;
    std::string curStr;
    int mode = 0; // 0 none, 1 reading msgid, 2 reading msgstr
    bool haveId = false;

    while(!in.atEnd())
    {
        const std::string line = in.readLine().toStdString();
        // trim leading whitespace
        std::size_t s = 0;
        while(s < line.size() && (line[s] == ' ' || line[s] == '\t'))
            ++s;
        const std::string t = line.substr(s);

        if(t.empty() || t[0] == '#')
        {
            if(haveId && !curId.empty())
                out[curId] = curStr;
            curId.clear();
            curStr.clear();
            haveId = false;
            mode = 0;
        }
        else if(t.rfind("msgid", 0) == 0)
        {
            // flush a completed previous entry before starting a new id
            if(haveId && !curId.empty())
                out[curId] = curStr;
            curId = unquote(t);
            curStr.clear();
            haveId = true;
            mode = 1;
        }
        else if(t.rfind("msgstr", 0) == 0)
        {
            curStr = unquote(t);
            mode = 2;
        }
        else if(t[0] == '"')
        {
            if(mode == 1)
                curId += unquote(t);
            else if(mode == 2)
                curStr += unquote(t);
        }
    }
    if(haveId && !curId.empty())
        out[curId] = curStr;
}

void Localization::load(const std::string &modRoot)
{
    // English is the CC default (<name> with no lang attribute).
    parsePo(modRoot + "/l18n/en_US/LC_MESSAGES/base.po", en_);
    // Every other locale under l18n/ is emitted as lang="<short code>"
    // (de_DE -> "de").  On a short-code collision (es_ES/es_MX) the
    // alphabetically first locale wins (QDir::Name sort).
    const QDir dir(QString::fromStdString(modRoot + "/l18n"));
    const QFileInfoList dirs = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
    int i = 0;
    while(i < dirs.size())
    {
        const std::string full = dirs.at(i).fileName().toStdString();
        std::string shortCode = full;
        const std::size_t us = full.find('_');
        if(us != std::string::npos)
            shortCode = full.substr(0, us);
        if(shortCode != "en" && byLocale_.find(shortCode) == byLocale_.end())
        {
            std::unordered_map<std::string,std::string> table;
            parsePo(dirs.at(i).absoluteFilePath().toStdString() + "/LC_MESSAGES/base.po", table);
            if(!table.empty())
            {
                byLocale_[shortCode].swap(table);
                locales_.push_back(shortCode);
            }
        }
        ++i;
    }
    std::sort(locales_.begin(), locales_.end());
}

// Title-case a slug ("fire_claw" -> "Fire Claw") for missing English names.
static std::string prettify(const std::string &slug)
{
    std::string out;
    bool up = true;
    std::size_t i = 0;
    while(i < slug.size())
    {
        char c = slug[i];
        if(c == '_' || c == '-')
        {
            out.push_back(' ');
            up = true;
        }
        else
        {
            if(up)
                out.push_back((char)std::toupper((unsigned char)c));
            else
                out.push_back(c);
            up = false;
        }
        ++i;
    }
    return out;
}

std::string Localization::lookup(const std::unordered_map<std::string,std::string> &m,
                                 const std::string &key) const
{
    std::unordered_map<std::string,std::string>::const_iterator it = m.find(key);
    if(it != m.end())
        return it->second;
    return std::string();
}

std::string Localization::nameEn(const std::string &slug) const
{
    const std::string v = lookup(en_, slug);
    if(!v.empty())
        return v;
    return prettify(slug);
}

std::string Localization::descEn(const std::string &slug) const
{
    return lookup(en_, slug + "_description");
}

const std::vector<std::string> &Localization::locales() const
{
    return locales_;
}

std::string Localization::name(const std::string &locale, const std::string &slug) const
{
    std::map<std::string,std::unordered_map<std::string,std::string> >::const_iterator it = byLocale_.find(locale);
    if(it == byLocale_.end())
        return std::string();
    return lookup(it->second, slug);
}

std::string Localization::desc(const std::string &locale, const std::string &slug) const
{
    std::map<std::string,std::unordered_map<std::string,std::string> >::const_iterator it = byLocale_.find(locale);
    if(it == byLocale_.end())
        return std::string();
    return lookup(it->second, slug + "_description");
}

std::string Localization::nameFr(const std::string &slug) const
{
    return name("fr", slug);
}

std::string Localization::descFr(const std::string &slug) const
{
    return desc("fr", slug);
}

} // namespace tuxemon
