// Copyright 2021 CatchChallenger
#include "CharacterSelector.hpp"

#include <iostream>
#include <string>

#include "../../../../general/base/CommonDatapack.hpp"
#include "../../../../general/base/CommonSettingsCommon.hpp"
#include "../../../libqtcatchchallenger/QtDatapackClientLoader.hpp"
#include "../../Constants.hpp"
#include "../../FacilityLibClient.hpp"
#include "../../Globals.hpp"
#include "../../base/ConnectionManager.hpp"
#include "../../core/SceneManager.hpp"
#include "../../core/StackedScene.hpp"
#include "CharacterItem.hpp"
#include "Loading.hpp"

using Scenes::CharacterSelector;
using std::placeholders::_1;
using std::placeholders::_2;

CharacterSelector::CharacterSelector() {
  serverSelected = 0;
  profileIndex = 0;

  add = UI::Button::Create(":/CC/images/interface/greenbutton.png", this);
  add->SetOutlineColor(QColor(44, 117, 0));
  add->SetEnabled(false);
  remove = UI::Button::Create(":/CC/images/interface/delete.png", this);
  remove->SetOutlineColor(QColor(125, 0, 0));
  remove->SetEnabled(false);

  select = UI::Button::Create(":/CC/images/interface/next.png", this);
  back = UI::Button::Create(":/CC/images/interface/back.png", this);
  select->SetEnabled(false);

  wdialog = Sprite::Create(":/CC/images/interface/message.png", this);

  characterEntryList = UI::ListView::Create(this);
  connection_ = ConnectionManager::GetInstance();

  selected_character_ = nullptr;

  add->SetOnClick(std::bind(&CharacterSelector::add_clicked, this));
  remove->SetOnClick(std::bind(&CharacterSelector::remove_clicked, this));
  select->SetOnClick(std::bind(&CharacterSelector::select_clicked, this));
  back->SetOnClick(std::bind(&CharacterSelector::backSubServer, this));

  newLanguage();

  // need be the last
  addCharacter = nullptr;
  newGame = nullptr;
}

CharacterSelector::~CharacterSelector() {
  if (addCharacter) {
    delete addCharacter;
    addCharacter = nullptr;
  }
  if (newGame) {
    delete newGame;
    newGame = nullptr;
  }
}

CharacterSelector *CharacterSelector::Create() {
  return new (std::nothrow) CharacterSelector();
}

void CharacterSelector::OnCharacterClick(Node *node) {
  selected_character_ = node;
  select->SetEnabled(true);
  remove->SetEnabled(characterEntryList->Count() >
                     CommonSettingsCommon::commonSettingsCommon.min_character);
}

void CharacterSelector::add_clicked() {
  profileIndex = 0;
  const std::vector<CatchChallenger::ServerFromPoolForDisplay>
      &serverOrdenedList = connection_->client->getServerOrdenedList();
  if ((characterEntryListInWaiting.size() +
       characterListForSelection
           .at(serverOrdenedList.at(serverSelected).charactersGroupIndex)
           .size()) >=
      CommonSettingsCommon::commonSettingsCommon.max_character) {
    Globals::GetAlertDialog()->Show(
        tr("You have already the max characters count"));
    return;
  }
  if (addCharacter == nullptr) {
    addCharacter = AddCharacter::Create();
    addCharacter->SetOnClose(std::bind(&CharacterSelector::add_finished, this));
  }
  AddChild(addCharacter);
  addCharacter->setDatapack(connection_->client->datapackPathBase());
}

void CharacterSelector::add_finished() {
  RemoveChild(addCharacter);
  if (!addCharacter->isOk()) {
    return;
  }
  const std::string &datapackPath = connection_->client->datapackPathBase();
  if (CatchChallenger::CommonDatapack::commonDatapack.get_profileList().size() >
      1)
    profileIndex = addCharacter->getProfileIndex();
  if (profileIndex >=
      CatchChallenger::CommonDatapack::commonDatapack.get_profileList().size())
    return;
  CatchChallenger::Profile profile =
      CatchChallenger::CommonDatapack::commonDatapack.get_profileList().at(
          profileIndex);
  if (newGame == nullptr) {
    newGame = NewGame::Create();
    newGame->SetOnClose(std::bind(&CharacterSelector::newGame_finished, this));
  }
  newGame->setDatapack(datapackPath + DATAPACK_BASE_PATH_SKIN,
                       datapackPath + DATAPACK_BASE_PATH_MONSTERS,
                       profile.monstergroup, profile.forcedskin);
  if (!newGame->haveSkin()) {
    Globals::GetAlertDialog()->Show(
        tr("Sorry but no skin found into: %1")
            .arg(QFileInfo(QString::fromStdString(datapackPath) +
                           DATAPACK_BASE_PATH_SKIN)
                     .absoluteFilePath()));
    return;
  }
  AddChild(newGame);
}

void CharacterSelector::newGame_finished() {
  if (!newGame->isOk()) {
    RemoveChild(newGame);
    return;
  }
  const std::vector<CatchChallenger::ServerFromPoolForDisplay>
      &serverOrdenedList = connection_->client->getServerOrdenedList();
  CatchChallenger::CharacterEntry characterEntry;
  characterEntry.character_id = 0;
  characterEntry.delete_time_left = 0;
  characterEntry.last_connect = QDateTime::currentMSecsSinceEpoch() / 1000;
  // characterEntry.mapId=QtDatapackClientLoader::GetInstance()->mapToId.value(profile.map);
  characterEntry.played_time = 0;
  characterEntry.pseudo = newGame->pseudo();
  characterEntry.charactersGroupIndex = newGame->monsterGroupId();
  characterEntry.skinId = newGame->skinId();
  connection_->client->addCharacter(
      serverOrdenedList.at(serverSelected).charactersGroupIndex,
      static_cast<uint8_t>(profileIndex), characterEntry.pseudo,
      characterEntry.charactersGroupIndex, characterEntry.skinId);
  characterEntryListInWaiting.push_back(characterEntry);
  if ((characterEntryListInWaiting.size() +
       characterListForSelection
           .at(serverOrdenedList.at(serverSelected).charactersGroupIndex)
           .size()) >= CommonSettingsCommon::commonSettingsCommon.max_character)
    add->SetEnabled(false);
  Globals::GetAlertDialog()->Show(tr("Creating your new character"));
}

void CharacterSelector::remove_clicked() {
  const std::vector<CatchChallenger::ServerFromPoolForDisplay>
      &serverOrdenedList = connection_->client->getServerOrdenedList();

  const uint32_t &character_id = selected_character_->Data(99);
  const uint32_t &delete_time_left = selected_character_->Data(98);
  if (delete_time_left > 0) {
    Globals::GetAlertDialog()->Show(tr("Deleting already planned"));
    return;
  }
  connection_->client->removeCharacter(
      serverOrdenedList.at(serverSelected).charactersGroupIndex, character_id);
  unsigned int index = 0;
  while (index <
         characterListForSelection
             .at(serverOrdenedList.at(serverSelected).charactersGroupIndex)
             .size()) {
    const CatchChallenger::CharacterEntry &characterEntry =
        characterListForSelection
            .at(serverOrdenedList.at(serverSelected).charactersGroupIndex)
            .at(index);
    if (characterEntry.character_id == character_id) {
      characterListForSelection[serverOrdenedList.at(serverSelected)
                                    .charactersGroupIndex][index]
          .delete_time_left =
          CommonSettingsCommon::commonSettingsCommon.character_delete_time;
      break;
    }
    index++;
  }
  updateCharacterList();
  Globals::GetAlertDialog()->Show(
      tr("Your charater will be deleted into %1")
          .arg(QString::fromStdString(
              CatchChallenger::FacilityLibClient::timeToString(
                  CommonSettingsCommon::commonSettingsCommon
                      .character_delete_time))));
}

void CharacterSelector::select_clicked() {
  if (selected_character_ == nullptr) {
    return;
  }

  connection_->SetOnGoToMap(std::bind(&CharacterSelector::GoToMap, this));
  connection_->selectCharacter(serverSelected, selected_character_->Data(99));
  auto loading = Loading::GetInstance();
  loading->Reset();
  loading->Progression(0, 100);
  loading->SetText(tr("Selecting your character..."));
  static_cast<StackedScene *>(Parent())->PushForeground(loading);
}

void CharacterSelector::newLanguage() {
  add->SetText(tr("Add"));
  // remove->setText(tr("Remove"));
}

void CharacterSelector::OnScreenResize() {
  auto space = Constants::ItemMediumSpacing();
  auto textSize = Constants::TextMediumSize();
  auto rectSize = Constants::ButtonMediumSize();
  auto roundSize = Constants::ButtonRoundMediumSize();

  unsigned int multiItemH = 100;
  add->SetSize(rectSize);
  add->SetPixelSize(textSize);
  remove->SetSize(roundSize);
  select->SetSize(roundSize);
  back->SetSize(roundSize);
  multiItemH = 50;

  unsigned int tWidthTButton = add->Width() + space + remove->Width();
  unsigned int tXTButton = bounding_rect_.width() / 2 - tWidthTButton / 2;
  unsigned int tWidthTButtonOffset = 0;
  unsigned int y = bounding_rect_.height() - space - select->Height() - space -
                   add->Height();
  remove->SetPos(tXTButton + tWidthTButtonOffset, y);
  tWidthTButtonOffset += remove->Width() + space;
  add->SetPos(tXTButton + tWidthTButtonOffset, y);
  tWidthTButtonOffset += add->Width() + space;

  tWidthTButton = select->Width() + space + back->Width();
  y = bounding_rect_.height() - space - select->Height();
  tXTButton = bounding_rect_.width() / 2 - tWidthTButton / 2;
  back->SetPos(tXTButton, y);
  select->SetPos(tXTButton + back->Width() + space, y);

  y = bounding_rect_.height() - space - select->Height() - space -
      add->Height();
  int height = bounding_rect_.height() - space - select->Height() - space -
               add->Height() - space - space;
  int width = bounding_rect_.width() * 0.5;
  wdialog->SetPos(bounding_rect_.width() / 2 - width / 2, space);

  if (width != wdialog->Width()) {
    auto border = width * 0.025;
    auto new_border = wdialog->Strech(46, 46, border, width, height);
    characterEntryList->SetPos(wdialog->X() + new_border.x(),
                               wdialog->Y() + new_border.y());
    characterEntryList->SetSize(wdialog->Width() - new_border.x() * 2,
                                wdialog->Height() - new_border.y() * 2);
  }
}

void CharacterSelector::connectToSubServer(
    const int indexSubServer,
    const std::vector<std::vector<CatchChallenger::CharacterEntry> >
        &characterEntryList) {
  Globals::GetMapScene();
  if (!QObject::connect(
          ConnectionManager::GetInstance()->client,
          &CatchChallenger::Api_protocol_Qt::QtnewCharacterId,
          std::bind(&CharacterSelector::newCharacterId, this, _1, _2)))
    abort();
  // detect character group to display only characters in this group
  this->serverSelected = indexSubServer;
  this->characterListForSelection = characterEntryList;
  updateCharacterList();
}

void CharacterSelector::updateCharacterList(bool prevent_auto_select) {
  const std::vector<CatchChallenger::ServerFromPoolForDisplay>
      &serverOrdenedList = connection_->client->getServerOrdenedList();
  // do a crash due to reference
  // const std::vector<ServerFromPoolForDisplay>
  // serverOrdenedList=connexionManager->client->getServerOrdenedList(); do the
  // grouping for characterGroup count
  {
    serverByCharacterGroup.clear();
    unsigned int index = 0;
    uint8_t serverByCharacterGroupTempIndexToDisplay = 1;
    while (index < serverOrdenedList.size()) {
      const CatchChallenger::ServerFromPoolForDisplay &server =
          serverOrdenedList.at(index);
      if (server.charactersGroupIndex > serverOrdenedList.size()) abort();
      if (serverByCharacterGroup.find(server.charactersGroupIndex) !=
          serverByCharacterGroup.cend()) {
        serverByCharacterGroup[server.charactersGroupIndex].first++;
      } else {
        serverByCharacterGroup[server.charactersGroupIndex].first = 1;
        serverByCharacterGroup[server.charactersGroupIndex].second =
            serverByCharacterGroupTempIndexToDisplay;
        serverByCharacterGroupTempIndexToDisplay++;
      }
      index++;
    }
  }
  characterEntryList->Clear();
  unsigned int index = 0;
  const uint8_t charactersGroupIndex =
      serverOrdenedList.at(serverSelected).charactersGroupIndex;
  int charaterCount = 0;
  if (characterListForSelection.size() > 0) {
    while (index < characterListForSelection.at(charactersGroupIndex).size()) {
      const CatchChallenger::CharacterEntry &characterEntry =
          characterListForSelection.at(charactersGroupIndex).at(index);

      auto item = CharacterItem::Create();
      item->SetData(99, characterEntry.character_id);
      item->SetData(98, characterEntry.delete_time_left);
      item->SetTitle(QString::fromStdString(characterEntry.pseudo));
      std::string text;
      if (characterEntry.played_time > 0)
        text += tr("%1 played")
                    .arg(QString::fromStdString(
                        CatchChallenger::FacilityLibClient::timeToString(
                            characterEntry.played_time)))
                    .toStdString();
      else
        text += tr("Never played").toStdString();
      if (characterEntry.delete_time_left > 0)
        text += "\n" + tr("%1 to be deleted")
                           .arg(QString::fromStdString(
                               CatchChallenger::FacilityLibClient::timeToString(
                                   characterEntry.delete_time_left)))
                           .toStdString();
      item->SetSubTitle(QString::fromStdString(text));
      if (characterEntry.skinId <
          QtDatapackClientLoader::GetInstance()->get_skins().size())
        item->SetIcon(
            QString::fromStdString(connection_->client->datapackPathBase()) +
            DATAPACK_BASE_PATH_SKIN +
            QString::fromStdString(
                QtDatapackClientLoader::GetInstance()->get_skins().at(
                    characterEntry.skinId)) +
            "/front.png");
      else
        item->SetIcon(QString(":/CC/images/player_default/front.png"));
      item->SetOnClick(
          std::bind(&CharacterSelector::OnCharacterClick, this, _1));
      characterEntryList->AddItem(item);
      index++;
    }
    charaterCount = characterListForSelection.at(charactersGroupIndex).size();
  }
  add->SetEnabled(charaterCount <
                  CommonSettingsCommon::commonSettingsCommon.max_character);
  if (charaterCount <
          CommonSettingsCommon::commonSettingsCommon.min_character &&
      charaterCount <
          CommonSettingsCommon::commonSettingsCommon.max_character) {
    add_clicked();
  } else if (charaterCount ==
                 CommonSettingsCommon::commonSettingsCommon.min_character &&
             charaterCount ==
                 CommonSettingsCommon::commonSettingsCommon.max_character) {
    selected_character_ = characterEntryList->Items().at(0);
    if (!prevent_auto_select) select_clicked();
  }
}

void CharacterSelector::newCharacterId(const uint8_t &returnCode,
                                       const uint32_t &characterId) {
  const std::vector<CatchChallenger::ServerFromPoolForDisplay>
      &serverOrdenedList = connection_->client->getServerOrdenedList();
  CatchChallenger::CharacterEntry characterEntry =
      characterEntryListInWaiting.front();
  characterEntryListInWaiting.erase(characterEntryListInWaiting.cbegin());
  if (returnCode == 0x00) {
    characterEntry.character_id = characterId;
    const uint8_t charactersGroupIndex =
        serverOrdenedList.at(serverSelected).charactersGroupIndex;
    characterListForSelection[charactersGroupIndex].push_back(characterEntry);
    updateCharacterList(true);
    if (characterEntryList->Count() >=
            CommonSettingsCommon::commonSettingsCommon.min_character &&
        characterEntryList->Count() <=
            CommonSettingsCommon::commonSettingsCommon.max_character) {
      select_clicked();
    }
  } else {
    if (returnCode == 0x01)
      Globals::GetAlertDialog()->Show(tr("This pseudo is already taken"));
    else if (returnCode == 0x02)
      Globals::GetAlertDialog()->Show(
          tr("Have already the max caraters taken"));
    else
      Globals::GetAlertDialog()->Show(tr("Unable to create the character"));
  }
}

void CharacterSelector::backSubServer() {
  // If is solo returns to menu
  if (Globals::IsSolo()) {
    static_cast<StackedScene *>(Parent())->PopForegroundUntilIndex(0);
    return;
  }
  static_cast<StackedScene *>(Parent())->PopForeground();
}

void CharacterSelector::OnEnter() {
  Scene::OnEnter();

  select->SetEnabled(false);
  selected_character_ = nullptr;
  add->RegisterEvents();
  remove->RegisterEvents();
  select->RegisterEvents();
  back->RegisterEvents();
  characterEntryList->RegisterEvents();
}

void CharacterSelector::OnExit() {
  add->UnRegisterEvents();
  remove->UnRegisterEvents();
  select->UnRegisterEvents();
  back->UnRegisterEvents();
  characterEntryList->UnRegisterEvents();

  Scene::OnExit();
}

void CharacterSelector::GoToMap() {
  SceneManager::GetInstance()->PushScene(Globals::GetMapScene());
}
