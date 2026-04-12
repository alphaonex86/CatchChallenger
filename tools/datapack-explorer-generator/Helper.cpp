#include "Helper.hpp"

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <dirent.h>
#include <cerrno>
#include <cstring>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <regex>

namespace {
std::string g_template;
std::string g_localPath;
std::string g_datapackPath;
std::string g_mainDatapackCode;
std::string g_subDatapackCode;
std::string g_currentPage;
std::string g_map2pngPath;
}

namespace Helper {

bool fileExists(const std::string &path)
{
    struct stat st;
    if(::stat(path.c_str(),&st)!=0)
        return false;
    return true;
}

bool isDir(const std::string &path)
{
    struct stat st;
    if(::stat(path.c_str(),&st)!=0)
        return false;
    return S_ISDIR(st.st_mode);
}

bool mkpath(const std::string &path)
{
    if(path.empty())
        return true;
    if(isDir(path))
        return true;
    // find parent
    size_t pos=path.find_last_of('/');
    if(pos!=std::string::npos && pos>0)
    {
        std::string parent=path.substr(0,pos);
        if(!parent.empty() && !isDir(parent))
            if(!mkpath(parent))
                return false;
    }
    if(::mkdir(path.c_str(),0777)!=0)
    {
        if(errno==EEXIST)
            return true;
        return false;
    }
    return true;
}

bool writeFile(const std::string &path, const std::string &content)
{
    size_t pos=path.find_last_of('/');
    if(pos!=std::string::npos)
    {
        std::string parent=path.substr(0,pos);
        if(!mkpath(parent))
            return false;
    }
    std::ofstream ofs(path, std::ios::binary|std::ios::trunc);
    if(!ofs.is_open())
        return false;
    ofs.write(content.data(),(std::streamsize)content.size());
    ofs.close();
    return true;
}

bool readFile(const std::string &path, std::string &out)
{
    std::ifstream ifs(path, std::ios::binary);
    if(!ifs.is_open())
        return false;
    std::ostringstream oss;
    oss << ifs.rdbuf();
    out=oss.str();
    return true;
}

std::string replaceAll(std::string str, const std::string &from, const std::string &to)
{
    if(from.empty())
        return str;
    size_t pos=0;
    while((pos=str.find(from,pos))!=std::string::npos)
    {
        str.replace(pos,from.length(),to);
        pos+=to.length();
    }
    return str;
}

std::string toLowerCase(const std::string &text)
{
    std::string out=text;
    // UTF-8 aware replacements for accented characters
    out=replaceAll(out,"\xC3\x82","\xC3\xA2");//Â -> â
    out=replaceAll(out,"\xC3\x80","\xC3\xA0");//À -> à
    out=replaceAll(out,"\xC3\x84","\xC3\xA4");//Ä -> ä
    out=replaceAll(out,"\xC3\x87","\xC3\xA7");//Ç -> ç
    out=replaceAll(out,"\xC3\x8A","\xC3\xAA");//Ê -> ê
    out=replaceAll(out,"\xC3\x88","\xC3\xA8");//È -> è
    out=replaceAll(out,"\xC3\x8B","\xC3\xAB");//Ë -> ë
    out=replaceAll(out,"\xC3\x89","\xC3\xA9");//É -> é
    out=replaceAll(out,"\xC3\x8F","\xC3\xAF");//Ï -> ï
    out=replaceAll(out,"\xC3\x96","\xC3\xB6");//Ö -> ö
    out=replaceAll(out,"\xC3\x94","\xC3\xB4");//Ô -> ô
    out=replaceAll(out,"\xC3\x9B","\xC3\xBB");//Û -> û
    out=replaceAll(out,"\xC3\x99","\xC3\xB9");//Ù -> ù
    out=replaceAll(out,"\xC3\x9C","\xC3\xBC");//Ü -> ü
    for(char &c : out)
        c=(char)std::tolower((unsigned char)c);
    return out;
}

std::string cleanText(const std::string &text, size_t maximumStringLength)
{
    std::string t=toLowerCase(text);

    // strip accents to ASCII
    t=replaceAll(t,"\xC3\xA2","a");//â
    t=replaceAll(t,"\xC3\xA0","a");//à
    t=replaceAll(t,"\xC3\xA1","a");//á
    t=replaceAll(t,"\xC3\xA3","a");//ã
    t=replaceAll(t,"\xC3\xA4","a");//ä
    t=replaceAll(t,"\xC3\xA7","c");//ç
    t=replaceAll(t,"\xC3\xA9","e");//é
    t=replaceAll(t,"\xC3\xA8","e");//è
    t=replaceAll(t,"\xC3\xAA","e");//ê
    t=replaceAll(t,"\xC3\xAB","e");//ë
    t=replaceAll(t,"\xC3\xAC","i");//ì
    t=replaceAll(t,"\xC3\xAD","i");//í
    t=replaceAll(t,"\xC3\xAE","i");//î
    t=replaceAll(t,"\xC3\xAF","i");//ï
    t=replaceAll(t,"\xC3\xB1","n");//ñ
    t=replaceAll(t,"\xC3\xB5","o");
    t=replaceAll(t,"\xC3\xB6","o");
    t=replaceAll(t,"\xC3\xB3","o");
    t=replaceAll(t,"\xC3\xB4","o");
    t=replaceAll(t,"\xC3\xB2","o");
    t=replaceAll(t,"\xC3\xBB","u");
    t=replaceAll(t,"\xC3\xBC","u");
    t=replaceAll(t,"\xC3\xBA","u");
    t=replaceAll(t,"\xC3\xB9","u");
    t=replaceAll(t,"\xC3\xBD","y");
    t=replaceAll(t,"\xC3\xBF","y");

    if(t.size()>maximumStringLength)
        t=t.substr(0,maximumStringLength);

    // Keep only a-zA-Z0-9_-, replace the rest with spaces
    std::string result;
    result.reserve(t.size());
    for(char c : t)
    {
        if((c>='a' && c<='z') || (c>='A' && c<='Z') || (c>='0' && c<='9') || c=='_' || c=='-')
            result.push_back(c);
        else
            result.push_back(' ');
    }
    // collapse spaces
    std::string collapsed;
    collapsed.reserve(result.size());
    bool lastSpace=true;
    for(char c : result)
    {
        if(c==' ')
        {
            if(!lastSpace)
                collapsed.push_back(' ');
            lastSpace=true;
        }
        else
        {
            collapsed.push_back(c);
            lastSpace=false;
        }
    }
    // trim
    while(!collapsed.empty() && collapsed.back()==' ')
        collapsed.pop_back();
    while(!collapsed.empty() && collapsed.front()==' ')
        collapsed.erase(collapsed.begin());
    return collapsed;
}

std::string textForUrl(const std::string &text)
{
    std::string t=cleanText(text);
    for(char &c : t)
        if(c==' ')
            c='-';
    // collapse dashes
    std::string out;
    out.reserve(t.size());
    char prev=0;
    for(char c : t)
    {
        if(c=='-' && prev=='-')
            continue;
        out.push_back(c);
        prev=c;
    }
    while(!out.empty() && out.front()=='-') out.erase(out.begin());
    while(!out.empty() && out.back()=='-') out.pop_back();
    if(out.empty())
        out="untitled";
    return out;
}

std::string firstLetterUpper(const std::string &text)
{
    if(text.empty())
        return text;
    std::string out=text;
    out[0]=(char)std::toupper((unsigned char)out[0]);
    return out;
}

std::string lowerCaseFirstLetterUpper(const std::string &text)
{
    std::string out=toLowerCase(text);
    if(!out.empty())
        out[0]=(char)std::toupper((unsigned char)out[0]);
    return out;
}

std::string htmlEscape(const std::string &text)
{
    std::string out;
    out.reserve(text.size());
    for(char c : text)
    {
        switch(c)
        {
            case '&': out+="&amp;"; break;
            case '<': out+="&lt;"; break;
            case '>': out+="&gt;"; break;
            case '"': out+="&quot;"; break;
            case '\'': out+="&#39;"; break;
            default: out.push_back(c);
        }
    }
    return out;
}

bool endsWith(const std::string &str, const std::string &suffix)
{
    if(str.size()<suffix.size())
        return false;
    return std::equal(suffix.rbegin(),suffix.rend(),str.rbegin());
}

static void listFiles(const std::string &dir, const std::string &subDir, const std::string &suffix, std::vector<std::string> &result)
{
    std::string full=dir+subDir;
    DIR *d=::opendir(full.c_str());
    if(d==nullptr)
        return;
    struct dirent *ent;
    while((ent=::readdir(d))!=nullptr)
    {
        std::string name=ent->d_name;
        if(name=="." || name=="..")
            continue;
        std::string rel=subDir+name;
        std::string fullEntry=dir+rel;
        if(isDir(fullEntry))
            listFiles(dir,rel+"/",suffix,result);
        else if(endsWith(name,suffix))
            result.push_back(rel);
    }
    ::closedir(d);
}

std::vector<std::string> getTmxList(const std::string &dir, const std::string &subDir)
{
    std::vector<std::string> list;
    listFiles(dir,subDir,".tmx",list);
    std::sort(list.begin(),list.end());
    return list;
}

std::vector<std::string> getXmlList(const std::string &dir, const std::string &subDir)
{
    std::vector<std::string> list;
    listFiles(dir,subDir,".xml",list);
    std::sort(list.begin(),list.end());
    return list;
}

std::string pathJoin(const std::string &a, const std::string &b)
{
    if(a.empty())
        return b;
    if(b.empty())
        return a;
    if(a.back()=='/' && b.front()=='/')
        return a+b.substr(1);
    if(a.back()!='/' && b.front()!='/')
        return a+"/"+b;
    return a+b;
}

std::string toStringInt(long long v)
{
    std::ostringstream oss;
    oss << v;
    return oss.str();
}

std::string toStringUint(unsigned long long v)
{
    std::ostringstream oss;
    oss << v;
    return oss.str();
}

void setTemplate(const std::string &t) { g_template=t; }
const std::string &templateHtml() { return g_template; }

void setLocalPath(const std::string &p)
{
    g_localPath=p;
    if(!g_localPath.empty() && g_localPath.back()!='/')
        g_localPath.push_back('/');
}
void setDatapackPath(const std::string &p)
{
    g_datapackPath=p;
    if(!g_datapackPath.empty() && g_datapackPath.back()!='/')
        g_datapackPath.push_back('/');
}
void setMainDatapackCode(const std::string &code) { g_mainDatapackCode=code; }
void setSubDatapackCode(const std::string &code) { g_subDatapackCode=code; }
const std::string &localPath() { return g_localPath; }
const std::string &datapackPath() { return g_datapackPath; }
const std::string &mainDatapackCode() { return g_mainDatapackCode; }
const std::string &subDatapackCode() { return g_subDatapackCode; }

void setMap2PngPath(const std::string &p) { g_map2pngPath=p; }
const std::string &map2pngPath() { return g_map2pngPath; }

void setCurrentPage(const std::string &relativePath) { g_currentPage=relativePath; }
const std::string &currentPage() { return g_currentPage; }

// Compute a relative URL from fromPage's directory to targetRelativePath.
// Both inputs are paths relative to the output root (no leading slash).
std::string relUrlFrom(const std::string &fromPage, const std::string &targetRelativePath)
{
    if(targetRelativePath.empty())
        return targetRelativePath;
    // Normalize inputs: strip leading slashes.
    std::string target=targetRelativePath;
    while(!target.empty() && target.front()=='/')
        target.erase(target.begin());
    std::string from=fromPage;
    while(!from.empty() && from.front()=='/')
        from.erase(from.begin());
    if(from.empty())
        return target;
    // Split both into path components; last component of "from" is a file
    // name and not a directory level to walk up from.
    auto split=[](const std::string &s, std::vector<std::string> &out){
        out.clear();
        size_t start=0;
        while(start<=s.size())
        {
            size_t pos=s.find('/',start);
            if(pos==std::string::npos)
            {
                if(start<s.size())
                    out.push_back(s.substr(start));
                break;
            }
            out.push_back(s.substr(start,pos-start));
            start=pos+1;
        }
    };
    std::vector<std::string> fromParts,targetParts;
    split(from,fromParts);
    split(target,targetParts);
    // fromParts last component is the file; directory depth is size-1.
    if(!fromParts.empty())
        fromParts.pop_back();
    // Find common prefix length.
    size_t common=0;
    while(common<fromParts.size() && common<targetParts.size()
          && fromParts[common]==targetParts[common])
        common++;
    std::string rel;
    for(size_t i=common;i<fromParts.size();++i)
        rel+="../";
    for(size_t i=common;i<targetParts.size();++i)
    {
        if(i>common)
            rel+="/";
        rel+=targetParts[i];
    }
    if(rel.empty())
        rel="./";
    return rel;
}

// Compute a relative URL from currentPage's directory to targetRelativePath.
std::string relUrl(const std::string &targetRelativePath)
{
    return relUrlFrom(g_currentPage,targetRelativePath);
}

// Build the CONTENT section: just return the page-specific body.
// (Dynamic nav menu is generated by an external tool.)
static std::string buildContent(const std::string &, const std::string &body)
{
    return body;
}

// Rewrite absolute ("/...") links in the template so they become relative
// from the current page. This is a minimal, attribute-oriented rewriter
// that scans for href="/..." and src="/..." and rewrites the values.
static std::string rewriteAbsoluteLinks(std::string html, const std::string &fromPage)
{
    static const char *attrs[]={"href=\"","src=\""};
    for(const char *attr : attrs)
    {
        const std::string needle=attr;
        size_t pos=0;
        while((pos=html.find(needle,pos))!=std::string::npos)
        {
            size_t start=pos+needle.size();
            if(start>=html.size()) break;
            if(html[start]!='/')
            {
                pos=start;
                continue;
            }
            // Skip "//" (protocol-relative) which is not local.
            if(start+1<html.size() && html[start+1]=='/')
            {
                pos=start;
                continue;
            }
            size_t end=html.find('"',start);
            if(end==std::string::npos) break;
            std::string target=html.substr(start+1,end-(start+1)); // strip leading '/'
            // Strip anchor/query before computing relative url, then re-append.
            std::string tail;
            size_t qpos=target.find_first_of("?#");
            if(qpos!=std::string::npos)
            {
                tail=target.substr(qpos);
                target=target.substr(0,qpos);
            }
            // Bare "/" is the site root: map it to index.html.
            if(target.empty())
                target="index.html";
            std::string rel=relUrlFrom(fromPage,target)+tail;
            html.replace(start,end-start,rel);
            pos=start+rel.size();
        }
    }
    return html;
}

void writeHtml(const std::string &relativePath, const std::string &title, const std::string &body)
{
    std::string full=g_localPath+relativePath;
    // Set the current page so any later helpers compute correct relative URLs.
    setCurrentPage(relativePath);

    // Fallback if no template was loaded (should not happen in normal runs).
    std::string tpl=g_template;
    if(tpl.empty())
        tpl="<!DOCTYPE html><html><head><title>${TITLE}</title></head><body>${CONTENT}${AUTOGEN}</body></html>\n";

    const std::string content=buildContent(relativePath,body);
    const std::string autogen="<div id=\"automaticallygen\">Automatically generated from the datapack</div>";

    // Perform substitutions first so template's absolute links can be
    // rewritten alongside the surrounding markup.
    tpl=replaceAll(tpl,"${TITLE}",htmlEscape(title));
    tpl=replaceAll(tpl,"${CONTENT}",content);
    tpl=replaceAll(tpl,"${AUTOGEN}",autogen);

    tpl=rewriteAbsoluteLinks(tpl,relativePath);

    writeFile(full,tpl);
}

std::string relativeFromDatapack(const std::string &absPath)
{
    if(absPath.empty())
        return std::string();
    std::string rel=absPath;
    if(!g_datapackPath.empty() && absPath.compare(0,g_datapackPath.size(),g_datapackPath)==0)
        rel=absPath.substr(g_datapackPath.size());
    // Strip leading slashes that may come from basepath concatenation like
    // "<datapack>/" + "/monsters/..."
    while(!rel.empty() && rel.front()=='/')
        rel.erase(rel.begin());
    return rel;
}

bool copyFile(const std::string &src, const std::string &dst)
{
    std::ifstream in(src,std::ios::binary);
    if(!in.is_open())
        return false;
    size_t pos=dst.find_last_of('/');
    if(pos!=std::string::npos)
    {
        std::string parent=dst.substr(0,pos);
        if(!mkpath(parent))
            return false;
    }
    std::ofstream out(dst,std::ios::binary|std::ios::trunc);
    if(!out.is_open())
        return false;
    out << in.rdbuf();
    return true;
}

std::string publishDatapackFile(const std::string &datapackRelative)
{
    if(datapackRelative.empty())
        return std::string();
    std::string rel=datapackRelative;
    while(!rel.empty() && rel.front()=='/')
        rel.erase(rel.begin());
    // Reference the datapack file relative to the output root directly.
    // The caller is expected to have the datapack content present at
    // g_localPath (either because localpath == datapackpath, or because
    // it was pre-copied). If the destination does not exist but the
    // source does, copy it over so the link is still valid.
    std::string dst=g_localPath+rel;
    if(!fileExists(dst))
    {
        std::string src=g_datapackPath+rel;
        if(fileExists(src))
            copyFile(src,dst);
    }
    return rel;
}

std::string datapackImageUrl(const std::string &absPath)
{
    if(absPath.empty())
        return std::string();
    std::string rel=relativeFromDatapack(absPath);
    std::string published=publishDatapackFile(rel);
    return relUrl(published);
}

} // namespace Helper
