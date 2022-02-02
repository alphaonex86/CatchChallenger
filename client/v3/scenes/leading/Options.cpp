// Copyright 2021 CatchChallenger
#include "Options.hpp"

#include <QDesktopServices>

#include "../../../libqtcatchchallenger/Language.hpp"
#include "../../../libqtcatchchallenger/Settings.hpp"
#include "../../Ultimate.hpp"
#ifndef CATCHCHALLENGER_NOAUDIO
  #include "../../core/AudioPlayer.hpp"
#endif

using Scenes::Options;
using std::placeholders::_1;

Options::Options() : UI::Dialog() {
  volumeText = UI::Label::Create(this);
  volumeSlider = UI::Slider::Create(this);
  volumeSlider->SetValue(Settings::settings->value("audioVolume").toUInt());
  volumeSlider->SetMaximum(100);
  productKeyText = UI::Label::Create(this);
  productKeyInput = UI::Input::Create(this);
  buy = UI::Button::Create(":/CC/images/interface/buy.png", this);
  buy->SetSize(42, 46);

  languagesText = UI::Label::Create(this);
  languagesList = UI::Combo::Create(":/CC/images/interface/button.png", this);
  languagesList->AddItem("English");
  languagesList->AddItem("French");
  languagesList->AddItem("Spanish");
  languagesList->SetPos(100, 200);

  {
    QString key = Settings::settings->value("key").toString();
    productKeyInput->SetValue(key);
    // if (Ultimate::ultimate.isUltimate())
    // productKeyInput->setStyleSheet("");
    // else
    // productKeyInput->setStyleSheet("color:red");
    previousKey = key;
    QString language = Settings::settings->value("language").toString();
    if (language == "fr")
      languagesList->SetCurrentIndex(1);
    else if (language == "es")
      languagesList->SetCurrentIndex(2);
    else
      languagesList->SetCurrentIndex(0);
  }

  volumeSlider->SetOnValueChange(std::bind(&Options::volumeSliderChange, this));
  productKeyInput->SetOnTextChange(std::bind(&Options::productKeyChange, this));
  languagesList->SetOnSelectChange(
      std::bind(&Options::languagesChange, this, _1));
  buy->SetOnClick(std::bind(&Options::openBuy, this));

  newLanguage();
}

Options::~Options() { delete buy; }

Options* Options::Create() { return new (std::nothrow) Options(); }

void Options::OnScreenResize() {
  UI::Dialog::OnScreenResize();

  auto font = volumeText->GetFont();
  volumeText->SetFont(font);
  productKeyText->SetFont(font);
  languagesText->SetFont(font);
  productKeyInput->SetFont(font);

  auto content = ContentBoundary();
  int x = content.x() + 20;
  int x_1 = content.x() + 300;
  int y = content.y() + 20;
  int width = content.x() + content.width();

  {
    volumeText->SetPos(x, y);
    volumeSlider->SetPos(x_1, y);
    volumeSlider->SetSize(width - x_1, volumeText->Height());
    y += volumeText->Height() + 10;
  }
  {
    productKeyText->SetPos(x, y);
    productKeyInput->SetPos(x_1, y);
    productKeyInput->SetSize(width - x_1, productKeyText->Height());

    buy->SetPos(width - buy->Width(), y);
    y += productKeyText->Height() + 10;
  }
  {
    languagesText->SetPos(x, y);
    languagesList->SetPos(x_1, y);
    languagesList->SetWidth(content.width() - x_1);
  }
}

void Options::volumeSliderChange() {
#ifndef CATCHCHALLENGER_NOAUDIO
  AudioPlayer::GetInstance()->SetVolume((int)volumeSlider->Value());
#endif
  Settings::settings->setValue("audioVolume", (int)volumeSlider->Value());
}

void Options::productKeyChange() {
  QString key = productKeyInput->Value();
  if (key == previousKey) return;
  previousKey = key;
  Settings::settings->setValue("key", key);
  Ultimate::ultimate.setKey(key.toStdString());
  // if (Ultimate::ultimate.setKey(key.toStdString()))
  // productKeyInput->setStyleSheet("");
  // else
  // productKeyInput->setStyleSheet("color:red");
}

void Options::languagesChange(int) {
  switch (languagesList->CurrentIndex()) {
    default:
    case 0:
      Settings::settings->setValue("language", "en");
      break;
    case 1:
      Settings::settings->setValue("language", "fr");
      break;
    case 2:
      Settings::settings->setValue("language", "es");
      break;
  }
  Language::language.setLanguage(
      Settings::settings->value("language").toString());
}

void Options::newLanguage() {
  languagesText->SetText(tr("Language: "));
  productKeyText->SetText(tr("Product key: "));
  volumeText->SetText(tr("Volume: "));
}

void Options::openBuy() {
  QDesktopServices::openUrl(QUrl(tr("https://shop.first-world.info/en/")));
}

void Options::OnEnter() {
  UI::Dialog::OnEnter();
  productKeyInput->RegisterEvents();
  volumeSlider->RegisterEvents();
  buy->RegisterEvents();
  languagesList->RegisterEvents();
}

void Options::OnExit() {
  productKeyInput->UnRegisterEvents();
  volumeSlider->UnRegisterEvents();
  buy->UnRegisterEvents();
  languagesList->UnRegisterEvents();
  UI::Dialog::OnExit();
}
