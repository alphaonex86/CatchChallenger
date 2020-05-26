#include <QApplication>
#include "../qt/LanguagesSelect.hpp"
#include "ScreenTransition.hpp"
#include "render/MapVisualiserOrder.hpp"
#include "../qt/LocalListener.hpp"
#include "../qt/GameLoader.hpp"
#include "../qt/QtDatapackClientLoader.hpp"
#include "../general/base/FacilityLibGeneral.hpp"
//#include "mainwindow.h"
#include <iostream>
#include <QFontDatabase>
#include <QStyleFactory>
#include "../qt/Options.hpp"
#include "../qt/QtDatapackChecksum.hpp"
#include <QStandardPaths>
#include <QScreen>
#include "Language.hpp"
#include "MyApplication.h"
#include "../qt/Settings.hpp"
#ifndef CATCHCHALLENGER_NOAUDIO
#include "../qt/Audio.hpp"
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
    GameLoader::gameLoader=nullptr;
    #ifndef CATCHCHALLENGER_NOAUDIO
    Audio::audio=nullptr;
    #endif

    //QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    MyApplication a(argc, argv);
    a.setApplicationName("client");
    a.setOrganizationName("CatchChallenger");
    a.setStyle(QStyleFactory::create("Fusion"));

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
    qRegisterMetaType<std::vector<Map_full> >("std::vector<Map_full>");

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

    if(Settings::settings->contains("language"))
        Language::language.setLanguage(Settings::settings->value("language").toString());
    Options::options.loadVar();

    ScreenTransition s;
    s.setWindowTitle(QObject::tr("CatchChallenger loading..."));
    /*QScreen *screen = QApplication::screens().at(0);
    s.setMinimumSize(QSize(screen->availableSize().width(),
                           screen->availableSize().height()));*/
    #ifndef  Q_OS_ANDROID
    s.setMinimumSize(QSize(640,480));
    #endif
    s.move(0,0);
    s.show();
    QIcon icon;
    icon.addFile(":/CC/images/catchchallenger.png", QSize(), QIcon::Normal, QIcon::Off);
    s.setWindowIcon(icon);
#ifdef  Q_OS_ANDROID
    s.showFullScreen();
#else
    //s.showMaximized();
#endif
    QtDatapackClientLoader::datapackLoader=nullptr;
    const auto returnCode=a.exec();
    if(QtDatapackClientLoader::datapackLoader!=nullptr)
        delete QtDatapackClientLoader::datapackLoader;
    #if ! defined(QT_NO_EMIT) && ! defined(EPOLLCATCHCHALLENGERSERVER) && !defined(NOTHREADS)
    CatchChallenger::QtDatapackChecksum::thread.quit();
    CatchChallenger::QtDatapackChecksum::thread.wait();
    #endif
    return returnCode;
}
