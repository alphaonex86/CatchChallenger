// Copyright 2021 CatchChallenger
#include "Menu.hpp"

#include <QDesktopServices>
#include <QWidget>
#include <iostream>
#include <string>

#include "../../../general/base/CommonSettingsCommon.hpp"
#include "../../../general/base/Version.hpp"
#include "../../../libqtcatchchallenger/QtDatapackClientLoader.hpp"
#include "../../../libqtcatchchallenger/Settings.hpp"
#include "../../Ultimate.hpp"
#include "../../core/AssetsLoader.hpp"
#include "../../core/SceneManager.hpp"
#include "../../core/Sprite.hpp"
#include "../../core/StackedScene.hpp"
#include "../../entities/FeedNews.hpp"
#include "../../entities/InternetUpdater.hpp"
#include "../../ui/Button.hpp"
#include "../../ui/Label.hpp"
#include "../../ui/ThemedButton.hpp"
#include "../../ui/ThemedItem.hpp"
#include "../ParallaxForest.hpp"
#include "Loading.hpp"
#include "Multi.hpp"
#ifndef __ANDROID_API__
  #define TEXT_SIZE 10
#else
  #define TEXT_SIZE 30
#endif

#ifndef CATCHCHALLENGER_NOAUDIO
  #include "../../core/AudioPlayer.hpp"
#endif

using Scenes::Menu;
using std::placeholders::_1;
using std::placeholders::_2;

Menu::Menu() {
  is_loaded_ = false;
  have_update_ = false;
  current_news_type_ = 0;

  solo_ = nullptr;

  auto loader = AssetsLoader::GetInstance();
  if (!loader->IsLoaded()) {
    loader->SetOnDataParsed(std::bind(&Menu::OnAssetsDataParsed, this));
    Loading::GetInstance()->SetNotifier(loader);
    AddChild(Loading::GetInstance());
  } else {
    is_loaded_ = true;
    Initialize();
    // Reload events
    OnScreenResize();
    OnEnter();
  }
}

Menu::~Menu() {
#ifndef __EMSCRIPTEN__
  QObject::disconnect(InternetUpdater::GetInstance(),
                      &InternetUpdater::newUpdate, nullptr, nullptr);
#endif
  QObject::disconnect(FeedNews::GetInstance(), &FeedNews::feedEntryList,
                      nullptr, nullptr);

  if (solo_) {
    delete solo_;
    solo_ = nullptr;
  }
  if (multi_) {
    delete multi_;
    multi_ = nullptr;
  }
  if (options_) {
    delete options_;
    options_ = nullptr;
  }
  if (facebook_) {
    delete facebook_;
    facebook_ = nullptr;
  }
  if (website_) {
    delete website_;
    website_ = nullptr;
  }
  if (debug_) {
    delete debug_;
    debug_ = nullptr;
  }
  if (news_wait_) {
    delete news_wait_;
    news_wait_ = nullptr;
  }
  if (news_list_) {
    delete news_list_;
    news_list_ = nullptr;
  }
  if (news_update_) {
    delete news_update_;
    news_update_ = nullptr;
  }
  if (news_) {
    delete news_;
    news_ = nullptr;
  }
  if (warning_) {
    delete warning_;
    warning_ = nullptr;
  }
  if (multi_scene_) {
    delete multi_scene_;
    multi_scene_ = nullptr;
  }
#ifndef NOSINGLEPLAYER
  if (solo_scene_) {
    delete solo_scene_;
    solo_scene_ = nullptr;
  }
#endif
  if (option_scene_) {
    delete option_scene_;
    option_scene_ = nullptr;
  }
  if (debug_scene_) {
    delete debug_scene_;
    debug_scene_ = nullptr;
  }
}

Menu *Menu::Create() { return new (std::nothrow) Menu(); }

void Menu::Initialize() {
  solo_ = UI::Button::Create(":/CC/images/interface/button.png", this);
  multi_ = UI::Button::Create(":/CC/images/interface/button.png", this);
  options_ = UI::Button::Create(":/CC/images/interface/options.png", this);
  debug_ = UI::Button::Create(":/CC/images/interface/debug.png", this);
  facebook_ = UI::Button::Create(":/CC/images/interface/facebook.png", this);
  facebook_->SetOutlineColor(QColor(0, 79, 154));
  facebook_->SetPixelSize(28);
  website_ = UI::Button::Create(":/CC/images/interface/bluetoolbox.png", this);
  website_->SetText("w");
  website_->SetOutlineColor(QColor(0, 79, 154));
  website_->SetPixelSize(28);
  warning_can_be_reset_ = true;

  have_fresh_feed_ = false;
  news_ = Sprite::Create(":/CC/images/interface/message.png", this);
  news_list_ = UI::ListView::Create(news_);

  // work
  UpdateNews();
  news_wait_ = Sprite::Create(news_);
  news_wait_->SetPixmap(":/CC/images/multi/busy.png");

  warning_ = UI::Label::Create(this);
  warning_->SetVisible(false);
  warning_string_ = QString(
      "<div style=\"background-color: rgb(255, 180, 180);border: 1px solid "
      "rgb(255, 221, 50);border-radius:5px;color: rgb(0, 0, "
      "0);\">&nbsp;%1&nbsp;</div>");
  news_update_ =
      UI::Button::Create(":/CC/images/interface/greenbutton.png", news_);
  news_update_->SetOutlineColor(QColor(44, 117, 0));
#ifndef CATCHCHALLENGER_SOLO
#ifndef __EMSCRIPTEN__
  if (!QObject::connect(InternetUpdater::GetInstance(),
                        &InternetUpdater::newUpdate,
                        std::bind(&Menu::NewUpdate, this, _1)))
    abort();
#endif
  /*
  auto feed_news = FeedNews::GetInstance();
  if (!QObject::connect(feed_news, &FeedNews::feedEntryList,
                        std::bind(&Menu::FeedEntryList, this, _1, _2)))
    std::cerr << "connect(RssNews::rssNews,&RssNews::rssEntryList,this,&"
                 "MenuWindow::rssEntryList) failed"
              << std::endl;
  feed_news->checkCache();
  */
  solo_->SetVisible(false);
#endif

  facebook_->SetOnClick(std::bind(&Menu::OpenFacebook, this));
  website_->SetOnClick(std::bind(&Menu::OpenWebsite, this));
  news_update_->SetOnClick(std::bind(&Menu::OpenUpdate, this));
  solo_->SetOnClick(std::bind(&Menu::GoToSolo, this));
  multi_->SetOnClick(std::bind(&Menu::GoToMulti, this));
  options_->SetOnClick(std::bind(&Menu::GoToOptions, this));
  debug_->SetOnClick(std::bind(&Menu::GoToDebug, this));

#ifndef NOSINGLEPLAYER
  solo_scene_ = nullptr;
#endif
  multi_scene_ = nullptr;
  option_scene_ = nullptr;
  debug_scene_ = nullptr;

#ifndef CATCHCHALLENGER_NOAUDIO
  if (!Settings::settings->contains("audioVolume"))
    Settings::settings->setValue("audioVolume", 80);
  AudioPlayer::GetInstance()->SetVolume(
      Settings::settings->value("audioVolume").toUInt());
  const std::string &terr =
      AudioPlayer::GetInstance()->StartAmbiance(":/CC/music/loading.opus");
#endif

  ChangeLanguage();
}

void Menu::setError(const std::string &error) {
  /*
  if (warning_->toHtml().isEmpty() || !warning->isVisible() ||
      warningCanBeReset) {
    warningCanBeReset = false;
    std::cerr << "ScreenTransition::errorString(" << error << ")" << std::endl;
    warning->setVisible(true);
    warning->setHtml(warningString.arg(QString::fromStdString(error)));
  }
  if (error.empty()) {
    warningCanBeReset = true;
    warning->setVisible(false);
  }*/
}

#ifndef __EMSCRIPTEN__
void Menu::NewUpdate(const std::string &version) {
  Q_UNUSED(version);
  have_update_ = true;
  /*
      ui->update->setText(QString::fromStdString(InternetUpdater::getText(version)));
      ui->update->setVisible(true);*/
}
#endif

void Menu::FeedEntryList(const std::vector<FeedNews::FeedEntry> &entryList,
                         std::string error) {
  entry_list_ = entryList;
  if (entry_list_.empty()) {
    if (!error.empty()) setError(error);
    return;
  }
  if (current_news_type_ != 0) UpdateNews();

  news_wait_->SetVisible(false);
  have_fresh_feed_ = true;
}

void Menu::UpdateNews() {
  switch (current_news_type_) {
    case 0:
      if (news_->IsVisible()) news_->SetVisible(false);
      break;
    case 1:
      if (entry_list_.empty()) {
        news_->SetVisible(false);
      } else {
        news_list_->Clear();
        if (entry_list_.size() > 1) {
          auto item = UI::YellowButton::Create();
          item->SetHeight(30);
          item->SetPixelSize(TEXT_SIZE);
          item->SetText(entry_list_.at(0).title);
          item->SetData(99, entry_list_.at(0).link.toStdString());
          item->SetOnClick(std::bind(&Menu::OnClickLink, this, _1));
          news_list_->AddItem(item);
        }
      }
      break;
    default:
    case 2:
      if (entry_list_.empty()) {
        news_->SetVisible(false);
      } else {
        news_list_->Clear();
        auto text = UI::Label::Create();
        text->SetPixelSize(12);
        text->SetText(tr("Latest news:"));
        news_list_->AddItem(text);
        if (entry_list_.size() == 1) {
          auto item = UI::DotItem::Create();
          item->SetPixelSize(TEXT_SIZE);
          item->SetText(entry_list_.at(0).title);
          item->SetData(99, entry_list_.at(0).link.toStdString());
          item->SetOnClick(std::bind(&Menu::OnClickLink, this, _1));
          news_list_->AddItem(item);
        } else {
          unsigned int index = 0;
          while (index < entry_list_.size() && index < 3) {
            auto item = UI::DotItem::Create();
            item->SetPixelSize(TEXT_SIZE);
            item->SetText(entry_list_.at(index).title);
            item->SetData(99, entry_list_.at(index).link.toStdString());
            item->SetOnClick(std::bind(&Menu::OnClickLink, this, _1));
            news_list_->AddItem(item);
            index++;
          }
        }
      }
      break;
  }
}

void Menu::OpenWebsite() {
  if (!QDesktopServices::openUrl(
          QUrl("https://catchchallenger.first-world.info/")))
    std::cerr << "Menu::openWebsite() failed" << std::endl;
}

void Menu::OpenFacebook() {
  if (!QDesktopServices::openUrl(
          QUrl("https://www.facebook.com/CatchChallenger/")))
    std::cerr << "Menu::openFacebook() failed" << std::endl;
}

void Menu::OpenUpdate() {
  if (!QDesktopServices::openUrl(
          QUrl("https://catchchallenger.first-world.info/download.html")))
    std::cerr << "Menu::openFacebook() failed" << std::endl;
}

void Menu::GoToSolo() {
#ifndef NOSINGLEPLAYER
  if (!solo_scene_) solo_scene_ = Solo::Create();
  static_cast<StackedScene *>(Parent())->PushForeground(solo_scene_);
#endif
}

void Menu::GoToMulti() {
  QtDatapackClientLoader::GetInstance();
  if (!multi_scene_) multi_scene_ = Multi::Create();
  static_cast<StackedScene *>(Parent())->PushForeground(multi_scene_);
}

void Menu::GoToOptions() {
  if (!option_scene_) option_scene_ = Options::Create();
  AddChild(option_scene_);
}

void Menu::GoToDebug() {
  if (!debug_scene_) debug_scene_ = Debug::Create();
  AddChild(debug_scene_);
}

void Menu::ChangeLanguage() {
  if (!is_loaded_) return;
  solo_->SetText(tr("Solo"));
  multi_->SetText(tr("Multi"));
  news_update_->SetText(tr("Update!"));
  UpdateNews();
}

void Menu::OnEnter() {
  if (!is_loaded_) return;
  solo_->RegisterEvents();
  multi_->RegisterEvents();
  options_->RegisterEvents();
  facebook_->RegisterEvents();
  website_->RegisterEvents();
  news_list_->RegisterEvents();
  debug_->RegisterEvents();
}

void Menu::OnExit() {
  if (!is_loaded_) return;
  solo_->UnRegisterEvents();
  multi_->UnRegisterEvents();
  options_->UnRegisterEvents();
  facebook_->UnRegisterEvents();
  website_->UnRegisterEvents();
  news_list_->UnRegisterEvents();
  debug_->UnRegisterEvents();
}

void Menu::OnAssetsDataParsed() {
  is_loaded_ = true;
  RemoveChild(Loading::GetInstance());
  SceneManager::GetInstance()->setWindowTitle(
      tr("CatchChallenger %1")
          .arg(QString::fromStdString(CatchChallenger::Version::str)));

  Initialize();

  // Reload events
  OnScreenResize();
  OnEnter();
}

void Menu::OnClickLink(Node *node) {
  if (!QDesktopServices::openUrl(
          QUrl(QString::fromStdString(node->DataStr(99)))))
    std::cerr << "Menu::OnClickLink() failed" << std::endl;
}

#ifndef __ANDROID_API__
void Menu::OnScreenResize() {
  if (!is_loaded_) return;
  int buttonMargin = 10;
  buttonMargin = 30;
  solo_->SetSize(223, 92);
  solo_->SetPixelSize(35);
  multi_->SetSize(223, 92);
  multi_->SetPixelSize(35);
  options_->SetSize(62, 70);
  options_->SetPixelSize(35);
  debug_->SetSize(62, 70);
  debug_->SetPixelSize(35);
  facebook_->SetSize(62, 70);
  facebook_->SetPixelSize(28);
  website_->SetSize(62, 70);
  website_->SetPixelSize(28);
  int verticalMargin =
      BoundingRect().height() / 2 + multi_->Height() / 2 + buttonMargin;
  if (solo_->IsVisible()) {
    solo_->SetPos(BoundingRect().width() / 2 - solo_->Width() / 2,
                  BoundingRect().height() / 2 - multi_->Height() / 2 -
                      solo_->Height() - buttonMargin);
    multi_->SetPos(BoundingRect().width() / 2 - multi_->Width() / 2,
                   BoundingRect().height() / 2 - multi_->Height() / 2);
  } else {
    multi_->SetPos(BoundingRect().width() / 2 - multi_->Width() / 2,
                   BoundingRect().height() / 2 - multi_->Height());
    verticalMargin = BoundingRect().height() / 2 + buttonMargin;
  }
  debug_->SetPos(10, 10);
  const int horizontalMargin = (multi_->Width() - options_->Width() -
                                facebook_->Width() - website_->Width()) /
                               2;
  options_->SetPos(BoundingRect().width() / 2 - facebook_->Width() / 2 -
                       horizontalMargin - options_->Width(),
                   verticalMargin);
  facebook_->SetPos(BoundingRect().width() / 2 - facebook_->Width() / 2,
                    verticalMargin);
  website_->SetPos(
      BoundingRect().width() / 2 + facebook_->Width() / 2 + horizontalMargin,
      verticalMargin);
  warning_->SetPos(BoundingRect().width() / 2 - warning_->Width() / 2, 5);

  if (BoundingRect().height() < 280) {
    news_update_->SetVisible(false);
    if (current_news_type_ != 0) {
      current_news_type_ = 0;
      UpdateNews();
    }
  } else {
    uint8_t border_size = 46;
    news_list_->SetPos(10, 10);
    news_wait_->SetPos(news_->Width() - news_wait_->Width() - border_size - 2,
                       news_->Height() / 2 - news_wait_->Height() / 2);
    news_->SetVisible(true);

    if (BoundingRect().width() < 600 || BoundingRect().height() < 480) {
      int w = BoundingRect().width() - 9 * 2;
      if (w > 300) w = 300;
      news_->Strech(26, 26, 10, w, 40);
      news_wait_->SetVisible(BoundingRect().width() > 450 && !have_fresh_feed_);
      news_update_->SetVisible(false);

      if (current_news_type_ != 1) {
        current_news_type_ = 1;
        UpdateNews();
      }
    } else {
      news_->Strech(26, 26, 10, 600 - 9 * 2, 180);
      news_wait_->SetVisible(!have_fresh_feed_);
      news_update_->SetSize(136, 57);
      news_update_->SetPixelSize(18);
      news_update_->SetPos(news_->Width() - 136 - border_size - 2,
                           news_->Height() / 2 - 57 / 2);
      news_update_->SetVisible(have_update_);

      if (current_news_type_ != 2) {
        current_news_type_ = 2;
        UpdateNews();
      }
    }
    news_->SetPos(BoundingRect().width() / 2 - news_->Width() / 2,
                  BoundingRect().height() - news_->Height() - 5);
    news_list_->SetSize(news_->Width() - 20, news_->Height() - 20);
  }
}
#else
void Menu::OnScreenResize() {
  if (!is_loaded_) return;
  int buttonMargin = 10;
  buttonMargin = 30;
  solo_->SetSize(300, 120);
  solo_->SetPixelSize(45);
  multi_->SetSize(300, 120);
  multi_->SetPixelSize(45);
  options_->SetSize(90, 95);
  options_->SetPixelSize(45);
  debug_->SetSize(90, 95);
  debug_->SetPixelSize(45);
  facebook_->SetSize(90, 95);
  facebook_->SetPixelSize(45);
  website_->SetSize(90, 95);
  website_->SetPixelSize(45);
  int verticalMargin =
      BoundingRect().height() / 2 + multi_->Height() / 2 + buttonMargin;
  if (solo_->IsVisible()) {
    solo_->SetPos(BoundingRect().width() / 2 - solo_->Width() / 2,
                  BoundingRect().height() / 2 - multi_->Height() / 2 -
                      solo_->Height() - buttonMargin);
    multi_->SetPos(BoundingRect().width() / 2 - multi_->Width() / 2,
                   BoundingRect().height() / 2 - multi_->Height() / 2);
  } else {
    multi_->SetPos(BoundingRect().width() / 2 - multi_->Width() / 2,
                   BoundingRect().height() / 2 - multi_->Height());
    verticalMargin = BoundingRect().height() / 2 + buttonMargin;
  }
  debug_->SetPos(10, 10);
  const int horizontalMargin = (multi_->Width() - options_->Width() -
                                facebook_->Width() - website_->Width()) /
                               2;
  options_->SetPos(BoundingRect().width() / 2 - facebook_->Width() / 2 -
                       horizontalMargin - options_->Width(),
                   verticalMargin);
  facebook_->SetPos(BoundingRect().width() / 2 - facebook_->Width() / 2,
                    verticalMargin);
  website_->SetPos(
      BoundingRect().width() / 2 + facebook_->Width() / 2 + horizontalMargin,
      verticalMargin);
  warning_->SetPos(BoundingRect().width() / 2 - warning_->Width() / 2, 5);

  if (BoundingRect().height() < 280) {
    news_update_->SetVisible(false);
    if (current_news_type_ != 0) {
      current_news_type_ = 0;
      UpdateNews();
    }
  } else {
    uint8_t border_size = 46;
    news_list_->SetPos(10, 10);
    news_wait_->SetPos(news_->Width() - news_wait_->Width() - border_size - 2,
                       news_->Height() / 2 - news_wait_->Height() / 2);
    news_->SetVisible(true);

    if (BoundingRect().width() < 600 || BoundingRect().height() < 480) {
      int w = BoundingRect().width() - 9 * 2;
      if (w > 300) w = 300;
      news_->Strech(26, 26, 10, w, 40);
      news_wait_->SetVisible(BoundingRect().width() > 450 && !have_fresh_feed_);
      news_update_->SetVisible(false);

      if (current_news_type_ != 1) {
        current_news_type_ = 1;
        UpdateNews();
      }
    } else {
      news_->Strech(26, 26, 10, 800 - 9 * 2, 280);
      news_wait_->SetVisible(!have_fresh_feed_);
      news_update_->SetSize(136, 57);
      news_update_->SetPixelSize(18);
      news_update_->SetPos(news_->Width() - 136 - border_size - 2,
                           news_->Height() / 2 - 57 / 2);
      news_update_->SetVisible(have_update_);

      if (current_news_type_ != 2) {
        current_news_type_ = 2;
        UpdateNews();
      }
    }
    news_->SetPos(BoundingRect().width() / 2 - news_->Width() / 2,
                  BoundingRect().height() - news_->Height() - 5);
    news_list_->SetSize(news_->Width() - 20, news_->Height() - 20);
  }
}
#endif
