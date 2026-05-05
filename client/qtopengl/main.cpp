#include <QApplication>
#include "LanguagesSelect.hpp"
#include "ScreenTransition.hpp"
#include "../libqtcatchchallenger/maprender/MapVisualiserOrder.hpp"
#include "../libqtcatchchallenger/maprender/MapVisualiserPlayer.hpp"
#include "../libqtcatchchallenger/maprender/QMap_client.hpp"
#include "../libqtcatchchallenger/LocalListener.hpp"
#include "../libqtcatchchallenger/QtDatapackClientLoader.hpp"
#include "../../general/base/FacilityLibGeneral.hpp"
//#include "mainwindow.h"
#include <iostream>
#include <QFontDatabase>
#include <QStyleFactory>
#include "../libqtcatchchallenger/QtDatapackChecksum.hpp"
#include <QStandardPaths>
#include <QScreen>
#include "../libqtcatchchallenger/Language.hpp"
#include "MyApplication.h"
#include "../libqtcatchchallenger/CliClientOptions.hpp"
#include "../libqtcatchchallenger/Settings.hpp"
#ifndef CATCHCHALLENGER_NOAUDIO
#include "../libqtcatchchallenger/Audio.hpp"
#endif

#ifdef Q_OS_ANDROID
#include <android/log.h>

class androidbuf : public std::streambuf {
public:
    enum { bufsize = 4096 }; // ... or some other suitable buffer size
    androidbuf() { this->setp(buffer, buffer + bufsize - 1); }

private:
    int overflow(int c)
    {
        if (c == traits_type::eof()) {
            *this->pptr() = traits_type::to_char_type(c);
            this->sbumpc();
        }
        return this->sync()? traits_type::eof(): traits_type::not_eof(c);
    }

    int sync()
    {
        int rc = 0;
        if (this->pbase() != this->pptr()) {
            char writebuf[bufsize+1];
            memcpy(writebuf, this->pbase(), this->pptr() - this->pbase());
            writebuf[this->pptr() - this->pbase()] = '\0';

            rc = __android_log_write(ANDROID_LOG_INFO, "std", writebuf) > 0;
            this->setp(buffer, buffer + bufsize - 1);
        }
        return rc;
    }

    char buffer[bufsize];
};
class androidbuferror : public std::streambuf {
public:
    enum { bufsize = 4096 }; // ... or some other suitable buffer size
    androidbuferror() { this->setp(buffer, buffer + bufsize - 1); }

private:
    int overflow(int c)
    {
        if (c == traits_type::eof()) {
            *this->pptr() = traits_type::to_char_type(c);
            this->sbumpc();
        }
        return this->sync()? traits_type::eof(): traits_type::not_eof(c);
    }

    int sync()
    {
        int rc = 0;
        if (this->pbase() != this->pptr()) {
            char writebuf[bufsize+1];
            memcpy(writebuf, this->pbase(), this->pptr() - this->pbase());
            writebuf[this->pptr() - this->pbase()] = '\0';

            rc = __android_log_write(ANDROID_LOG_ERROR, "std", writebuf) > 0;
            this->setp(buffer, buffer + bufsize - 1);
        }
        return rc;
    }

    char buffer[bufsize];
};
#endif

int main(int argc, char *argv[])
{
#ifdef Q_OS_ANDROID
    std::cout.rdbuf(new androidbuf);
    std::cerr.rdbuf(new androidbuferror);
#endif
    //work around for android
    QtDatapackClientLoader::datapackLoader=nullptr;
    #ifndef CATCHCHALLENGER_NOAUDIO
    Audio::audio=nullptr;
    #endif

    //QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    MyApplication a(argc, argv);
    a.setApplicationName("client");
    a.setOrganizationName("CatchChallenger");
    a.setStyle(QStyleFactory::create("Fusion"));
    a.setQuitOnLastWindowClosed(false);

    qRegisterMetaType<std::vector<std::string> >("std::vector<std::string>");
    qRegisterMetaType<QList<QUrl> >("QList<QUrl>");
    qRegisterMetaType<std::vector<std::vector<CatchChallenger::CharacterEntry> > >("std::vector<std::vector<CatchChallenger::CharacterEntry> >");
    qRegisterMetaType<std::vector<std::vector<CatchChallenger::CharacterEntry> > >("std::vector<std::vector<CharacterEntry> >");
    qRegisterMetaType<std::vector<char> >("std::vector<char>");
    qRegisterMetaType<std::vector<uint32_t> >("std::vector<uint32_t>");
    qRegisterMetaType<std::string>("std::string");
    qRegisterMetaType<uint64_t>("uint64_t");
    qRegisterMetaType<uint32_t>("uint32_t");
    qRegisterMetaType<uint16_t>("uint16_t");
    qRegisterMetaType<uint8_t>("uint8_t");
    qRegisterMetaType<std::unordered_map<uint16_t,uint32_t> >("std::unordered_map<uint16_t,uint32_t>");
    qRegisterMetaType<std::vector<CatchChallenger::QMap_client> >("std::vector<QMap_client>");

    qRegisterMetaType<CatchChallenger::Chat_type>("CatchChallenger::Chat_type");
    qRegisterMetaType<CatchChallenger::Player_type>("CatchChallenger::Player_type");
    qRegisterMetaType<CatchChallenger::Player_private_and_public_informations>("CatchChallenger::Player_private_and_public_informations");
    qRegisterMetaType<std::vector<CatchChallenger::ServerFromPoolForDisplay *> >("std::vector<ServerFromPoolForDisplay *>");
    qRegisterMetaType<MapVisualiserPlayer::BlockedOn>("MapVisualiserPlayer::BlockedOn");

    qRegisterMetaType<CatchChallenger::Chat_type>("Chat_type");
    qRegisterMetaType<CatchChallenger::Player_type>("Player_type");
    qRegisterMetaType<CatchChallenger::Player_private_and_public_informations>("Player_private_and_public_informations");

    qRegisterMetaType<std::unordered_map<uint32_t,uint32_t> >("std::unordered_map<uint32_t,uint32_t>");
    qRegisterMetaType<std::unordered_map<uint32_t,uint32_t> >("CatchChallenger::Plant_collect");
    qRegisterMetaType<std::vector<CatchChallenger::ItemToSellOrBuy> >("std::vector<ItemToSell>");
    qRegisterMetaType<std::vector<std::pair<uint8_t,uint8_t> > >("std::vector<std::pair<uint8_t,uint8_t> >");
    qRegisterMetaType<CatchChallenger::Skill::AttackReturn>("Skill::AttackReturn");
    qRegisterMetaType<std::vector<uint32_t> >("std::vector<uint32_t>");

    qRegisterMetaType<std::unordered_map<uint16_t,uint16_t> >("std::unordered_map<uint16_t,uint16_t>");
    qRegisterMetaType<std::unordered_map<uint16_t,uint32_t> >("std::unordered_map<uint16_t,uint32_t>");
    qRegisterMetaType<std::unordered_map<uint8_t,uint32_t> >("std::unordered_map<uint8_t,uint32_t>");
    qRegisterMetaType<std::unordered_map<uint8_t,uint16_t> >("std::unordered_map<uint8_t,uint16_t>");
    qRegisterMetaType<std::unordered_map<uint8_t,uint32_t> >("std::unordered_map<uint8_t,uint32_t>");

    qRegisterMetaType<std::map<uint16_t,uint16_t> >("std::map<uint16_t,uint16_t>");
    qRegisterMetaType<std::map<uint16_t,uint32_t> >("std::map<uint16_t,uint32_t>");
    qRegisterMetaType<std::map<uint8_t,uint32_t> >("std::map<uint8_t,uint32_t>");
    qRegisterMetaType<std::map<uint8_t,uint16_t> >("std::map<uint8_t,uint16_t>");
    qRegisterMetaType<std::map<uint8_t,uint32_t> >("std::map<uint8_t,uint32_t>");
    qRegisterMetaType<std::vector<std::string> >("std::vector<std::string>");
    qRegisterMetaType<std::vector<char> >("std::vector<char>");
    qRegisterMetaType<std::vector<uint32_t> >("std::vector<uint32_t>");

    if(argc<1)
    {
        std::cerr << "argc<1: wrong arg count" << std::endl;
        return EXIT_FAILURE;
    }
    CatchChallenger::FacilityLibGeneral::applicationDirPath=argv[0];
    Settings::init();

    QFontDatabase::addApplicationFont(":/other/comicbd.ttf");
    QFont font("Comic Sans MS");
    font.setStyleHint(QFont::Monospace);
    //font.setBold(true);
    font.setPixelSize(15);
    QApplication::setFont(font);

    LocalListener localListener;
    const QStringList &arguments=QCoreApplication::arguments();
    if(arguments.size()==2 && arguments.last()=="quit")
    {
        localListener.quitAllRunningInstance();
        return 0;
    }
    else
    {
        if(!localListener.tryListen())
            return 0;
    }

    //parse the command line arguments
    {
        int index=1;
        while(index<arguments.size())
        {
            const QString &arg=arguments.at(index);
            if(arg==QStringLiteral("--help") || arg==QStringLiteral("-h"))
            {
                const QByteArray &progBytes=arguments.at(0).toLocal8Bit();
                const char *name=progBytes.isEmpty()?"client-qtopengl":progBytes.constData();
                std::cout
                    << "Usage: " << name << " [options]\n"
                    << "\n"
                    << "Options:\n"
                    << "  --help, -h                 Show this help and exit.\n"
                    << "  --server NAME              Select an existing server-list entry by name (TCP or WS).\n"
#ifndef CATCHCHALLENGER_NO_TCPSOCKET
                    << "  --host HOST                TCP direct-connect host/IP (requires --port).\n"
                    << "  --port PORT                TCP direct-connect port (requires --host).\n"
#endif
#ifndef CATCHCHALLENGER_NO_WEBSOCKET
                    << "  --url URL                  WebSocket direct-connect ws:// or wss:// URL.\n"
#endif
                    << "  --autologin                Automatically press the login button.\n"
                    << "  --character NAME           Auto-select a character by pseudo.\n"
                    << "  --closewhenonmap           Quit 1s after the first map is loaded.\n"
                    << "  --closewhenonmapafter=N    On map: toggle direction each 1s, quit after N seconds.\n"
                    << "  --dropsenddataafteronmap   Drop outgoing traffic after the first map is loaded.\n"
                    << "  --autosolo                 Load the first savegame and enter the game.\n"
                    << "  --take-screenshot=PATH     On map: write rendered viewport to PATH and exit\n"
                    << "                             (pins srand(42) for reproducible tile/animation rng).\n"
                    << "  --main-datapack-code=CODE  Override the autosolo maincode under\n"
                    << "                             datapack/internal/map/main/ (default: first dir).\n";
                std::cout.flush();
                return 0;
            }
            else if(arg==QStringLiteral("--server"))
            {
                if((index+1)<arguments.size())
                {
                    CliClientOptions::serverName=arguments.at(index+1);
                    index++;
                }
                else
                    std::cerr << "--server require a name argument" << std::endl;
            }
            else if(arg==QStringLiteral("--autologin"))
                CliClientOptions::autologin=true;
            else if(arg==QStringLiteral("--character"))
            {
                if((index+1)<arguments.size())
                {
                    CliClientOptions::characterName=arguments.at(index+1);
                    index++;
                }
                else
                    std::cerr << "--character require a name argument" << std::endl;
            }
            else if(arg==QStringLiteral("--closewhenonmap"))
                CliClientOptions::closeWhenOnMap=true;
            else if(arg.startsWith(QStringLiteral("--closewhenonmapafter=")))
            {
                CliClientOptions::closeWhenOnMapAfter=arg.mid(22).toInt();
                if(CliClientOptions::closeWhenOnMapAfter<1)
                    CliClientOptions::closeWhenOnMapAfter=1;
            }
            else if(arg==QStringLiteral("--dropsenddataafteronmap"))
                CliClientOptions::dropSendDataAfterOnMap=true;
            else if(arg==QStringLiteral("--autosolo"))
                CliClientOptions::autosolo=true;
            else if(arg.startsWith(QStringLiteral("--take-screenshot=")))
                CliClientOptions::takeScreenshotPath=arg.mid(18);
            else if(arg==QStringLiteral("--take-screenshot"))
            {
                if((index+1)<arguments.size())
                {
                    CliClientOptions::takeScreenshotPath=arguments.at(index+1);
                    index++;
                }
                else
                    std::cerr << "--take-screenshot requires a path argument" << std::endl;
            }
#ifndef CATCHCHALLENGER_NO_TCPSOCKET
            else if(arg==QStringLiteral("--host"))
            {
                if((index+1)<arguments.size())
                {
                    CliClientOptions::host=arguments.at(index+1);
                    index++;
                }
                else
                    std::cerr << "--host requires a host/ip argument" << std::endl;
            }
            else if(arg==QStringLiteral("--port"))
            {
                if((index+1)<arguments.size())
                {
                    CliClientOptions::port=static_cast<uint16_t>(arguments.at(index+1).toUShort());
                    index++;
                }
                else
                    std::cerr << "--port requires a number argument" << std::endl;
            }
#endif
#ifndef CATCHCHALLENGER_NO_WEBSOCKET
            else if(arg==QStringLiteral("--url"))
            {
                if((index+1)<arguments.size())
                {
                    CliClientOptions::url=arguments.at(index+1);
                    index++;
                }
                else
                    std::cerr << "--url requires a ws:// or wss:// URL argument" << std::endl;
            }
#endif
            else if(arg.startsWith(QStringLiteral("--main-datapack-code=")))
                CliClientOptions::mainDatapackCodeOverride=arg.mid(21);
            else if(arg==QStringLiteral("--main-datapack-code"))
            {
                if((index+1)<arguments.size())
                {
                    CliClientOptions::mainDatapackCodeOverride=arguments.at(index+1);
                    index++;
                }
                else
                    std::cerr << "--main-datapack-code requires a code argument" << std::endl;
            }
            index++;
        }
    }

    // Enforce: --server, --host/--port, and --url are mutually exclusive
    // groups (see CLAUDE.md "Client server-selection CLI flags — pick
    // exactly ONE"). When two or more are set, drop everything and log a
    // clear error so the user fixes the launch instead of silently
    // hitting whichever wins the priority race in ScreenTransition.
    {
        const bool group_list = !CliClientOptions::serverName.isEmpty();
        const bool group_tcp  = !CliClientOptions::host.isEmpty()
                                || CliClientOptions::port!=0;
        const bool group_ws   = !CliClientOptions::url.isEmpty();
        const int set_groups  = (group_list?1:0)+(group_tcp?1:0)+(group_ws?1:0);
        if(set_groups>1)
        {
            std::cerr << "ERROR: --server, --host/--port, and --url are mutually "
                         "exclusive — pass exactly one group. Ignoring all three."
                      << std::endl;
            CliClientOptions::serverName.clear();
            CliClientOptions::host.clear();
            CliClientOptions::port=0;
            CliClientOptions::url.clear();
        }
    }

    // Deterministic RNG for --take-screenshot: tile "random" properties
    // (e.g. lava variants in MapVisualiserThread) and "random-offset"
    // animation starts both call rand(). Pinning srand here makes the
    // rendered first frame reproducible for visual-regression tests.
    if(!CliClientOptions::takeScreenshotPath.isEmpty())
        srand(42);

    if(Settings::settings->contains("language"))
        Language::language.setLanguage(Settings::settings->value("language").toString());
    //Options::options.loadVar();

    ScreenTransition s;
    s.setWindowTitle(QObject::tr("CatchChallenger loading..."));
    /*QScreen *screen = QApplication::screens().at(0);
    s.setMinimumSize(QSize(screen->availableSize().width(),
                           screen->availableSize().height()));*/
    QIcon icon;
    icon.addFile(":/CC/images/catchchallenger.png", QSize(), QIcon::Normal, QIcon::Off);
    s.setWindowIcon(icon);
    if(!CliClientOptions::host.isEmpty() || !CliClientOptions::serverName.isEmpty() ||
       !CliClientOptions::url.isEmpty() ||
       CliClientOptions::closeWhenOnMap || CliClientOptions::dropSendDataAfterOnMap ||
       CliClientOptions::autosolo || !CliClientOptions::takeScreenshotPath.isEmpty())
    {
        //s.setWindowFlags(s.windowFlags() | Qt::WindowStaysOnBottomHint);
        s.show();
    }
    else
    {
        #ifndef  Q_OS_ANDROID
        s.setMinimumSize(QSize(640,480));
        #endif
        s.move(0,0);
        #ifdef  Q_OS_ANDROID
        s.showFullScreen();
        #else
        s.showMaximized();
        #endif
    }
    QtDatapackClientLoader::datapackLoader=nullptr;
    const int returnCode=a.exec();
    if(QtDatapackClientLoader::datapackLoader!=nullptr)
    {
        delete QtDatapackClientLoader::datapackLoader;
        QtDatapackClientLoader::datapackLoader=nullptr;
    }
    #if ! defined(QT_NO_EMIT) && ! defined(CATCHCHALLENGER_SERVER) && !defined(NOTHREADS)
    CatchChallenger::QtDatapackChecksum::thread.quit();
    CatchChallenger::QtDatapackChecksum::thread.wait();
    #endif
    return returnCode;
}
