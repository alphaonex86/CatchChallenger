#include <QCoreApplication>
#include <QGuiApplication>
#include <QStringList>
#include <QString>
#include <QSettings>
#include <QFileInfo>
#include <QDir>
#include <QDebug>

#include <iostream>
#include <string>
#include <sstream>

#include "Helper.hpp"
#include "Generator.hpp"
#include "MapStore.hpp"

#include "../../client/libqtcatchchallenger/QtDatapackClientLoader.hpp"
#include "../../client/libqtcatchchallenger/Settings.hpp"
#include "../../client/libqtcatchchallenger/Language.hpp"
#include "../../general/base/CommonDatapack.hpp"
#include "../../general/base/CommonSettingsServer.hpp"

#include <sys/types.h>
#include <dirent.h>

// Stop all background image-loading threads so exit() doesn't segfault
static void stopLoaderThreads()
{
    if(QtDatapackClientLoader::datapackLoader!=nullptr)
        QtDatapackClientLoader::datapackLoader->stopAllThreads();
}

static std::string argValue(const QStringList &args, const QString &name)
{
    const QString prefix=name+"=";
    for(const QString &a : args)
    {
        if(a.startsWith(prefix))
            return a.mid(prefix.size()).toStdString();
        if(a==name)
            return std::string();
    }
    return std::string();
}

static bool hasArg(const QStringList &args, const QString &name)
{
    const QString prefix=name+"=";
    for(const QString &a : args)
        if(a==name || a.startsWith(prefix))
            return true;
    return false;
}

// Capture stderr output during a call to detect error messages from the loader
class StderrCapture {
public:
    StderrCapture() : oldBuf(std::cerr.rdbuf()) {
        std::cerr.rdbuf(captured.rdbuf());
    }
    ~StderrCapture() {
        std::cerr.rdbuf(oldBuf);
    }
    std::string str() const { return captured.str(); }
private:
    std::streambuf *oldBuf;
    std::ostringstream captured;
};

// Capture stdout similarly (qDebug goes to stderr on Qt6, but some messages use cout)
class StdoutCapture {
public:
    StdoutCapture() : oldBuf(std::cout.rdbuf()) {
        std::cout.rdbuf(captured.rdbuf());
    }
    ~StdoutCapture() {
        std::cout.rdbuf(oldBuf);
    }
    std::string str() const { return captured.str(); }
private:
    std::streambuf *oldBuf;
    std::ostringstream captured;
};

static bool checkForErrors(const std::string &output, const std::string &phase)
{
    bool hasError=false;

    // Check each line: if it starts with "0 " then the count should be >0
    {
        std::istringstream stream(output);
        std::string line;
        while(std::getline(stream,line))
        {
            if(line.size()>=2 && line[0]=='0' && line[1]==' '
               && line.find("bot linked with quest")==std::string::npos)
            {
                std::cerr << "ERROR during " << phase << ": \"" << line << "\" should be >0" << std::endl;
                hasError=true;
            }
        }
    }

    // Check for "Unable to open:" directory errors (path issues, not individual file parse failures)
    {
        const char *dirPatterns[]={"Unable to open: ", nullptr};
        for(int p=0;dirPatterns[p];p++)
        {
            size_t pos=0;
            while((pos=output.find(dirPatterns[p],pos))!=std::string::npos)
            {
                size_t eol=output.find('\n',pos);
                std::string line=(eol!=std::string::npos)?output.substr(pos,eol-pos):output.substr(pos);
                std::cerr << "ERROR during " << phase << ": " << line << std::endl;
                hasError=true;
                pos=(eol!=std::string::npos)?eol+1:output.size();
            }
        }
    }

    // Check for "Parse error" messages (warn, don't block - individual map XML errors)
    {
        size_t pos=0;
        while((pos=output.find("Parse error",pos))!=std::string::npos)
        {
            size_t lineStart=output.rfind('\n',pos);
            lineStart=(lineStart!=std::string::npos)?lineStart+1:0;
            size_t lineEnd=output.find('\n',pos);
            std::string line=(lineEnd!=std::string::npos)?output.substr(lineStart,lineEnd-lineStart):output.substr(lineStart);
            std::cerr << "WARNING during " << phase << ": " << line << std::endl;
            pos=(lineEnd!=std::string::npos)?lineEnd+1:output.size();
        }
    }

    return hasError;
}

int main(int argc, char *argv[])
{
    // QGuiApplication needed because QPixmap is used in QtDatapackClientLoader
    // (default image is loaded via resources).
    QGuiApplication a(argc, argv);

    a.setOrganizationDomain(QStringLiteral("catchchallenger"));
    a.setOrganizationName(QStringLiteral("catchchallenger"));
    a.setApplicationName(QStringLiteral("datapack-explorer-generator"));
    a.setApplicationVersion(QStringLiteral("1.0"));

    Settings::init();

    QStringList args=QCoreApplication::arguments();

    if(hasArg(args,"--help") || hasArg(args,"-h"))
    {
        std::cout << "Usage: datapack-explorer-generator [options]" << std::endl;
        std::cout << std::endl;
        std::cout << "Options:" << std::endl;
        std::cout << "  --datapack=PATH        Path to the CatchChallenger datapack folder (required)" << std::endl;
        std::cout << "  --localpath=PATH       Output directory for generated HTML pages (required)" << std::endl;
        std::cout << "  --map2png=PATH         Path to the map2png binary for map image generation" << std::endl;
        std::cout << "                         Called with -platform offscreen. If not set or empty," << std::endl;
        std::cout << "                         map preview/overview images are not generated" << std::endl;
        std::cout << "  --help, -h             Show this help message" << std::endl;
        return 0;
    }

    if(!hasArg(args,"--localpath") || !hasArg(args,"--datapack"))
    {
        std::cerr << "Usage: datapack-explorer-generator --datapack=PATH --localpath=PATH [options]" << std::endl;
        std::cerr << "Try '--help' for more information." << std::endl;
        return 1;
    }

    std::string datapackPath=argValue(args,"--datapack");
    std::string localPath=argValue(args,"--localpath");
    std::string map2pngPath;
    if(hasArg(args,"--map2png"))
        map2pngPath=argValue(args,"--map2png");

    if(datapackPath.empty() || localPath.empty())
    {
        std::cerr << "--datapack and --localpath are required" << std::endl;
        return 1;
    }
    if(datapackPath.back()!='/')
        datapackPath.push_back('/');
    if(localPath.back()!='/')
        localPath.push_back('/');

    Helper::setLocalPath(localPath);
    Helper::setDatapackPath(datapackPath);
    Helper::setMap2PngPath(map2pngPath);

    // Load template.html next to the binary / source dir. It must contain
    // ${TITLE}, ${CONTENT} and ${AUTOGEN} placeholders which are replaced
    // by Helper::writeHtml() for each generated page.
    {
        std::string tpl;
        std::vector<std::string> candidates;
        candidates.push_back("./template.html");
        candidates.push_back(QCoreApplication::applicationDirPath().toStdString()+"/template.html");
        candidates.push_back(std::string(SRC_DIR)+"/template.html");
        for(const std::string &c : candidates)
            if(Helper::readFile(c,tpl) && !tpl.empty())
                break;
        if(tpl.empty())
        {
            std::cerr << "Unable to locate template.html (looked in ./, app dir, "
                      << SRC_DIR << ")" << std::endl;
            return 1;
        }
        Helper::setTemplate(tpl);
    }

    if(!Helper::mkpath(localPath))
    {
        std::cerr << "Failed to create output directory: " << localPath << std::endl;
        return 1;
    }

    // Allocate loader
    if(QtDatapackClientLoader::datapackLoader==nullptr)
        QtDatapackClientLoader::datapackLoader=new QtDatapackClientLoader();

    Language::language.setLanguage(QStringLiteral("en"));

    // Parse base datapack (monsters, items, buffs, plants, skills, types, etc.)
    std::cout << "Parsing datapack: " << datapackPath << std::endl;
    {
        std::string capturedOut, capturedErr;
        {
            StdoutCapture soc;
            StderrCapture sec;
            QtDatapackClientLoader::datapackLoader->parseDatapack(datapackPath);
            capturedOut=soc.str();
            capturedErr=sec.str();
        }
        // Re-emit all output so user can still see it
        if(!capturedOut.empty()) std::cout << capturedOut;
        if(!capturedErr.empty()) std::cerr << capturedErr;

        std::string combined=capturedOut+"\n"+capturedErr;
        if(checkForErrors(combined,"parseDatapack"))
        { stopLoaderThreads(); exit(113); }
    }

    // Verify base data was loaded
    if(CatchChallenger::CommonDatapack::commonDatapack.get_monsters_size()==0)
    {
        std::cerr << "ERROR: monsters list is empty after parseDatapack" << std::endl;
        stopLoaderThreads(); exit(113);
    }
    if(CatchChallenger::CommonDatapack::commonDatapack.get_items_size()==0)
    {
        std::cerr << "ERROR: items list is empty after parseDatapack" << std::endl;
        stopLoaderThreads(); exit(113);
    }

    // Discover all main datapack codes under <datapack>/map/main/
    std::vector<std::string> mainCodes;
    {
        const std::string mainRoot=datapackPath+"map/main/";
        DIR *d=::opendir(mainRoot.c_str());
        if(d!=nullptr)
        {
            struct dirent *ent;
            while((ent=::readdir(d))!=nullptr)
            {
                std::string n=ent->d_name;
                if(n=="." || n=="..")
                    continue;
                if(!Helper::isDir(mainRoot+n))
                    continue;
                mainCodes.push_back(n);
            }
            ::closedir(d);
        }
        if(mainCodes.empty())
            mainCodes.push_back("official");
    }
    std::sort(mainCodes.begin(),mainCodes.end());

    // Helper to list sub-datapack codes under map/main/{code}/sub/
    std::function<std::vector<std::string>(const std::string &)> discoverSubCodes=[&](const std::string &mainCode) -> std::vector<std::string> {
        std::vector<std::string> subs;
        subs.push_back(std::string()); // empty = base (no sub)
        const std::string subRoot=datapackPath+"map/main/"+mainCode+"/sub/";
        DIR *d=::opendir(subRoot.c_str());
        if(d!=nullptr)
        {
            struct dirent *ent;
            while((ent=::readdir(d))!=nullptr)
            {
                std::string n=ent->d_name;
                if(n=="." || n=="..")
                    continue;
                if(!Helper::isDir(subRoot+n))
                    continue;
                subs.push_back(n);
            }
            ::closedir(d);
        }
        std::sort(subs.begin(),subs.end());
        return subs;
    };

    for(const std::string &code : mainCodes)
    {
        std::vector<std::string> subCodes=discoverSubCodes(code);
        for(const std::string &sub : subCodes)
        {
            std::cout << "Parsing main/sub: " << code << " / " << (sub.empty()?"(none)":sub) << std::endl;
            Helper::setMainDatapackCode(code);
            Helper::setSubDatapackCode(sub);

            // Set CommonSettingsServer so internal loader paths are correct
            CommonSettingsServer::commonSettingsServer.mainDatapackCode=code;
            CommonSettingsServer::commonSettingsServer.subDatapackCode=sub;

            {
                std::string capturedOut, capturedErr;
                {
                    StdoutCapture soc;
                    StderrCapture sec;
                    QtDatapackClientLoader::datapackLoader->parseDatapackMainSub(code,sub);
                    capturedOut=soc.str();
                    capturedErr=sec.str();
                }
                if(!capturedOut.empty()) std::cout << capturedOut;
                if(!capturedErr.empty()) std::cerr << capturedErr;

                std::string combined=capturedOut+"\n"+capturedErr;
                if(checkForErrors(combined,"parseDatapackMainSub("+code+","+sub+")"))
                { stopLoaderThreads(); exit(113); }
            }

            // Verify maps were loaded
            if(QtDatapackClientLoader::datapackLoader->get_maps().empty())
            {
                std::cerr << "ERROR: maps list is empty after parseDatapackMainSub(" << code << "," << sub << ")" << std::endl;
                stopLoaderThreads(); exit(113);
            }

            MapStore::addFromCurrentLoader(code);
        }
    }

    Generator::generateAll();

    return 0;
}
