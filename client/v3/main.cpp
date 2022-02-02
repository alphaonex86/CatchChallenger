// Copyright 2021 CatchChallenger
#include <QApplication>
#include <QFontDatabase>
#include <QScreen>
#include <QStandardPaths>
#include <QStyleFactory>
#include <iostream>

#include "../general/base/FacilityLibGeneral.hpp"
#include "../libqtcatchchallenger/Language.hpp"
#include "../libqtcatchchallenger/QtDatapackChecksum.hpp"
#include "../libqtcatchchallenger/QtDatapackClientLoader.hpp"
#include "../libqtcatchchallenger/Settings.hpp"
#include "Globals.hpp"
#include "MyApplication.h"
#include "Options.hpp"
#include "base/LocalListener.hpp"
#include "core/SceneManager.hpp"
#include "scenes/test/Test.hpp"
#ifndef CATCHCHALLENGER_NOAUDIO
  #include "core/AudioPlayer.hpp"
#endif

#ifdef __ANDROID_API__
  #include <android/log.h>

class androidbuf : public std::streambuf {
 public:
  enum { bufsize = 4096 };  // ... or some other suitable buffer size
  androidbuf() { this->setp(buffer, buffer + bufsize - 1); }

 private:
  int overflow(int c) {
    if (c == traits_type::eof()) {
      *this->pptr() = traits_type::to_char_type(c);
      this->sbumpc();
    }
    return this->sync() ? traits_type::eof() : traits_type::not_eof(c);
  }

  int sync() {
    int rc = 0;
    if (this->pbase() != this->pptr()) {
      char writebuf[bufsize + 1];
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
  enum { bufsize = 4096 };  // ... or some other suitable buffer size
  androidbuferror() { this->setp(buffer, buffer + bufsize - 1); }

 private:
  int overflow(int c) {
    if (c == traits_type::eof()) {
      *this->pptr() = traits_type::to_char_type(c);
      this->sbumpc();
    }
    return this->sync() ? traits_type::eof() : traits_type::not_eof(c);
  }

  int sync() {
    int rc = 0;
    if (this->pbase() != this->pptr()) {
      char writebuf[bufsize + 1];
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

int main(int argc, char *argv[]) {
#ifdef __ANDROID_API__
  std::cout.rdbuf(new androidbuf);
  std::cerr.rdbuf(new androidbuferror);
#endif
  // work around for android
  QtDatapackClientLoader::datapackLoader = nullptr;
#ifndef CATCHCHALLENGER_NOAUDIO
  AudioPlayer::GetInstance();
#endif

  // QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
  MyApplication a(argc, argv);
  a.setApplicationName("client");
  a.setOrganizationName("CatchChallenger");
  a.setStyle(QStyleFactory::create("Fusion"));
  a.setQuitOnLastWindowClosed(false);

  qRegisterMetaType<std::vector<std::string>>("std::vector<std::string>");
  qRegisterMetaType<QList<QUrl>>("QList<QUrl>");
  qRegisterMetaType<std::vector<std::vector<CatchChallenger::CharacterEntry>>>(
      "std::vector<std::vector<CatchChallenger::CharacterEntry>>");
  qRegisterMetaType<std::vector<std::vector<CatchChallenger::CharacterEntry>>>(
      "std::vector<std::vector<CharacterEntry>>");
  qRegisterMetaType<std::vector<char>>("std::vector<char>");
  qRegisterMetaType<std::vector<uint32_t>>("std::vector<uint32_t>");
  qRegisterMetaType<std::string>("std::string");
  qRegisterMetaType<uint64_t>("uint64_t");
  qRegisterMetaType<uint32_t>("uint32_t");
  qRegisterMetaType<uint16_t>("uint16_t");
  qRegisterMetaType<uint8_t>("uint8_t");
  qRegisterMetaType<std::unordered_map<uint16_t, uint32_t>>(
      "std::unordered_map<uint16_t,uint32_t>");
  // qRegisterMetaType<std::vector<Map_full> >("std::vector<Map_full>");

  /*
  qRegisterMetaType<CatchChallenger::Chat_type>("CatchChallenger::Chat_type");
  qRegisterMetaType<CatchChallenger::Player_type>("CatchChallenger::Player_type");
  qRegisterMetaType<CatchChallenger::Player_private_and_public_informations>("CatchChallenger::Player_private_and_public_informations");
  qRegisterMetaType<std::vector<CatchChallenger::ServerFromPoolForDisplay *>
  >("std::vector<ServerFromPoolForDisplay *>");
  //qRegisterMetaType<MapVisualiserPlayer::BlockedOn>("MapVisualiserPlayer::BlockedOn");

  qRegisterMetaType<CatchChallenger::Chat_type>("Chat_type");
  qRegisterMetaType<CatchChallenger::Player_type>("Player_type");
  qRegisterMetaType<CatchChallenger::Player_private_and_public_informations>("Player_private_and_public_informations");
  */

  qRegisterMetaType<std::unordered_map<uint32_t, uint32_t>>(
      "std::unordered_map<uint32_t,uint32_t>");
  // qRegisterMetaType<std::unordered_map<uint32_t,uint32_t>
  // >("CatchChallenger::Plant_collect");
  // qRegisterMetaType<std::vector<CatchChallenger::ItemToSellOrBuy>
  // >("std::vector<ItemToSell>");
  qRegisterMetaType<std::vector<std::pair<uint8_t, uint8_t>>>(
      "std::vector<std::pair<uint8_t,uint8_t> >");
  // qRegisterMetaType<CatchChallenger::Skill::AttackReturn>("Skill::AttackReturn");
  qRegisterMetaType<std::vector<uint32_t>>("std::vector<uint32_t>");
  // qRegisterMetaType<std::vector<CatchChallenger::MarketMonster>
  // >("std::vector<MarketMonster>");

  qRegisterMetaType<std::unordered_map<uint16_t, uint16_t>>(
      "std::unordered_map<uint16_t,uint16_t>");
  qRegisterMetaType<std::unordered_map<uint16_t, uint32_t>>(
      "std::unordered_map<uint16_t,uint32_t>");
  qRegisterMetaType<std::unordered_map<uint8_t, uint32_t>>(
      "std::unordered_map<uint8_t,uint32_t>");
  qRegisterMetaType<std::unordered_map<uint8_t, uint16_t>>(
      "std::unordered_map<uint8_t,uint16_t>");
  qRegisterMetaType<std::unordered_map<uint8_t, uint32_t>>(
      "std::unordered_map<uint8_t,uint32_t>");

  qRegisterMetaType<std::map<uint16_t, uint16_t>>(
      "std::map<uint16_t,uint16_t>");
  qRegisterMetaType<std::map<uint16_t, uint32_t>>(
      "std::map<uint16_t,uint32_t>");
  qRegisterMetaType<std::map<uint8_t, uint32_t>>("std::map<uint8_t,uint32_t>");
  qRegisterMetaType<std::map<uint8_t, uint16_t>>("std::map<uint8_t,uint16_t>");
  qRegisterMetaType<std::map<uint8_t, uint32_t>>("std::map<uint8_t,uint32_t>");
  qRegisterMetaType<std::vector<std::string>>("std::vector<std::string>");
  qRegisterMetaType<std::vector<char>>("std::vector<char>");
  qRegisterMetaType<std::vector<uint32_t>>("std::vector<uint32_t>");

  if (argc < 1) {
    std::cerr << "argc<1: wrong arg count" << std::endl;
    return EXIT_FAILURE;
  }
  CatchChallenger::FacilityLibGeneral::applicationDirPath = argv[0];
  Settings::init();

  QFontDatabase::addApplicationFont(":/CC/other/comicbd.ttf");
  QFontDatabase::addApplicationFont(":/CC/other/aristotelica.ttf");
  QFontDatabase::addApplicationFont(":/CC/other/calibri.ttf");
  QFont font("Comic Sans MS");
  font.setStyleHint(QFont::Monospace);
  // font.setBold(true);
  font.setPixelSize(15);
  QApplication::setFont(font);

  LocalListener localListener;
  const QStringList &arguments = QCoreApplication::arguments();
  if (arguments.size() == 2 && arguments.last() == "quit") {
    localListener.quitAllRunningInstance();
    return 0;
  } else {
    if (!localListener.tryListen()) return 0;
  }

  if (Settings::settings->contains("language"))
    Language::language.setLanguage(
        Settings::settings->value("language").toString());
  Options::options.loadVar();

  // ScreenTransition s;
  SceneManager *s = SceneManager::GetInstance();

  s->PushScene(Globals::GetLeadingScene());
  s->setWindowTitle(QObject::tr("CatchChallenger loading..."));
/*QScreen *screen = QApplication::screens().at(0);
s.setMinimumSize(QSize(screen->availableSize().width(),
                       screen->availableSize().height()));*/
#ifdef __ANDROID_API__
  s->setMinimumSize(QSize(640, 480));
#endif
  s->move(0, 0);
  QIcon icon;
  icon.addFile(":/CC/images/catchchallenger.png", QSize(), QIcon::Normal,
               QIcon::Off);
  s->setWindowIcon(icon);
#ifdef __ANDROID_API__
  s->setMinimumSize(QSize(640, 480));
  s->showFullScreen();
#else
  // s->show();
  s->showMaximized();
#endif
  QtDatapackClientLoader::datapackLoader = nullptr;
  const auto returnCode = a.exec();
  if (QtDatapackClientLoader::datapackLoader != nullptr) {
    delete QtDatapackClientLoader::datapackLoader;
    QtDatapackClientLoader::datapackLoader = nullptr;
  }
#if !defined(QT_NO_EMIT) && !defined(EPOLLCATCHCHALLENGERSERVER) && \
    !defined(NOTHREADS)
  CatchChallenger::QtDatapackChecksum::thread.quit();
  CatchChallenger::QtDatapackChecksum::thread.wait();
#endif
  return returnCode;
}
