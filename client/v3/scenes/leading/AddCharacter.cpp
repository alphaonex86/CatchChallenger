// Copyright 2021 CatchChallenger
#include "AddCharacter.hpp"

#include <iostream>

#include "../../../../general/base/CommonDatapack.hpp"
#include "../../../../general/base/DatapackGeneralLoader.hpp"
#include "../../../../general/base/GeneralVariable.hpp"
#include "../../../../general/tinyXML2/tinyxml2.hpp"
#include "../../../libqtcatchchallenger/Language.hpp"
#include "../../Constants.hpp"

using Scenes::AddCharacter;
using std::placeholders::_1;

AddCharacter::AddCharacter() : UI::Dialog(false) {
  ok = false;

  quit = UI::Button::Create(":/CC/images/interface/cancel.png", this);
  quit->SetOnClick(std::bind(&AddCharacter::OnActionClick, this, _1));
  validate = UI::Button::Create(":/CC/images/interface/validate.png", this);
  validate->SetOnClick(std::bind(&AddCharacter::OnActionClick, this, _1));

  comboBox = UI::Combo::Create(":/CC/images/interface/button.png", this);
  auto buttonSize = Constants::ButtonMediumSize();
  comboBox->SetSize(buttonSize);
  description = UI::Label::Create(this);

  comboBox->SetOnSelectChange(
      std::bind(&AddCharacter::on_comboBox_currentIndexChanged, this, _1));

  AddActionButton(quit);
  AddActionButton(validate);

  newLanguage();
}

AddCharacter::~AddCharacter() {
  delete quit;
  delete validate;
  delete comboBox;
  delete description;
}

AddCharacter *AddCharacter::Create() {
  return new (std::nothrow) AddCharacter();
}

void AddCharacter::OnScreenResize() {
  Dialog::OnScreenResize();

  auto roundSize = Constants::ButtonRoundMediumSize();
  auto textSize = Constants::TextMediumSize();

  quit->SetSize(roundSize);
  validate->SetSize(roundSize);
  description->SetPixelSize(textSize);

  unsigned int nameBackgroundNewHeight = 50;
  unsigned int space = 30;

  auto content = ContentBoundary();

  comboBox->SetWidth(content.width() * 0.5);
  comboBox->SetPos(content.x() + content.width() / 2 - comboBox->Width() / 2,
                   content.y() + 20);
  description->SetPos(
      content.x() + content.width() / 2 - description->Width() / 2,
      content.y() + content.height() / 2 - description->Height() / 2 +
          comboBox->Height());
}

void AddCharacter::setDatapack(std::string path) {
  this->datapackPath = path;
  this->ok = false;

  comboBox->Clear();
  if (CatchChallenger::CommonDatapack::commonDatapack.get_profileList()
          .empty()) {
    description->SetText(tr("No profile selected to start a new game"));
    return;
  }
  loadProfileText();
  unsigned int index = 0;
  while (index < profileTextList.size()) {
    comboBox->AddItem(QString::fromStdString(profileTextList.at(index).name));
    index++;
  }
  if (comboBox->Count() > 0) {
    comboBox->SetCurrentIndex(rand() % comboBox->Count());
    description->SetText(QString::fromStdString(
        profileTextList.at(comboBox->CurrentIndex()).description));
  }
  validate->SetEnabled(comboBox->Count() > 0);
  if (profileTextList.size() == 1) on_ok_clicked();
}

void AddCharacter::newLanguage() { SetTitle(tr("Select your profile")); }

void AddCharacter::on_ok_clicked() {
  if (comboBox->Count() < 1) return;
  ok = true;
  Close();
}

bool AddCharacter::isOk() const { return ok; }

void AddCharacter::on_cancel_clicked() {
  ok = false;
  Close();
}

void AddCharacter::loadProfileText() {
  const std::string &xmlFile =
      datapackPath + DATAPACK_BASE_PATH_PLAYERBASE + "start.xml";
  std::vector<const tinyxml2::XMLElement *> xmlList =
      CatchChallenger::DatapackGeneralLoader::loadProfileList(
          datapackPath, xmlFile,
          CatchChallenger::CommonDatapack::commonDatapack.get_items().item,
          CatchChallenger::CommonDatapack::commonDatapack.get_monsters(),
          CatchChallenger::CommonDatapack::commonDatapack.get_reputation())
          .first;
  profileTextList.clear();
  unsigned int index = 0;
  while (index < xmlList.size()) {
    ProfileText profile;
    const tinyxml2::XMLElement *startItem = xmlList.at(index);
#ifndef CATCHCHALLENGER_BOT
    const std::string &language =
        Language::language.getLanguage().toStdString();
#else
    const std::string language("en");
#endif
    bool found = false;
    const tinyxml2::XMLElement *name = startItem->FirstChildElement("name");
    if (!language.empty() && language != "en")
      while (name != NULL) {
        if (name->Attribute("lang") != NULL &&
            name->Attribute("lang") == language && name->GetText() != NULL) {
          profile.name = name->GetText();
          found = true;
          break;
        }
        name = name->NextSiblingElement("name");
      }
    if (!found) {
      name = startItem->FirstChildElement("name");
      while (name != NULL) {
        if (name->Attribute("lang") == NULL ||
            strcmp(name->Attribute("lang"), "en") == 0) {
          if (name->GetText() != NULL) {
            profile.name = name->GetText();
            break;
          }
        }
        name = name->NextSiblingElement("name");
      }
    }
    if (profile.name.empty()) {
      /*
      if (startItem->GetText() != NULL)
        qDebug() << (QStringLiteral("Unable to open the xml file: %1, name "
                                    "empty or not found: child.tagName(): %2")
                         .arg(QString::fromStdString(xmlFile))
                         .arg(startItem->GetText()));
      else
        qDebug() << (QStringLiteral("Unable to open the xml file: %1, name "
                                    "empty or not found: child.tagName(")
                         .arg(QString::fromStdString(xmlFile)));
      */
      startItem = startItem->NextSiblingElement("start");
      continue;
    }
    found = false;
    const tinyxml2::XMLElement *description =
        startItem->FirstChildElement("description");
    if (!language.empty() && language != "en")
      while (description != NULL) {
        if (description->Attribute("lang") != NULL &&
            description->Attribute("lang") == language &&
            description->GetText() != NULL) {
          profile.description = description->GetText();
          found = true;
          break;
        }
        description = description->NextSiblingElement("description");
      }
    if (!found) {
      description = startItem->FirstChildElement("description");
      while (description != NULL) {
        if (description->Attribute("lang") == NULL ||
            strcmp(description->Attribute("lang"), "en") == 0)
          if (description->GetText() != NULL) {
            profile.description = description->GetText();
            break;
          }
        description = description->NextSiblingElement("description");
      }
    }
    if (profile.description.empty()) {
      /*
      if (description->GetText() != NULL)
        qDebug() << (QStringLiteral(
                         "Unable to open the xml file: %1, description empty "
                         "or not found: child.tagName(): %2")
                         .arg(QString::fromStdString(xmlFile))
                         .arg(startItem->GetText()));
      else
        qDebug() << (QStringLiteral(
                         "Unable to open the xml file: %1, description empty "
                         "or not found: child.tagName()")
                         .arg(QString::fromStdString(xmlFile)));
      */
      startItem = startItem->NextSiblingElement("start");
      continue;
    }
    profileTextList.push_back(profile);
    index++;
  }
}

int AddCharacter::getProfileIndex() {
  if (comboBox->Count() > 0) return comboBox->CurrentIndex();
  return 0;
}

int AddCharacter::getProfileCount() { return comboBox->Count(); }

void AddCharacter::on_comboBox_currentIndexChanged(int index) {
  if (comboBox->Count() > 0)
    description->SetText(
        QString::fromStdString(profileTextList.at(index).description));
}

void AddCharacter::OnActionClick(Node *node) {
  if (node == quit) {
    on_cancel_clicked();
  } else if (node == validate) {
    on_ok_clicked();
  }
}

void AddCharacter::OnEnter() {
  Dialog::OnEnter();
  comboBox->RegisterEvents();
}

void AddCharacter::OnExit() {
  comboBox->UnRegisterEvents();
  Dialog::OnExit();
}
