// Copyright 2021 CatchChallenger
#include "NewGame.hpp"

#include <iostream>
#include <vector>

#include "../../../../general/base/CommonDatapack.hpp"
#include "../../../../general/base/CommonSettingsCommon.hpp"
#include "../../../../general/base/GeneralStructures.hpp"

using Scenes::NewGame;
using std::placeholders::_1;

NewGame::NewGame() : UI::Dialog(false) {
  ok = false;

  quit = UI::Button::Create(":/CC/images/interface/cancel.png", this);
  validate = UI::Button::Create(":/CC/images/interface/validate.png", this);

  previous = UI::Button::Create(":/CC/images/interface/back.png", this);
  next = UI::Button::Create(":/CC/images/interface/next.png", this);

  row_ = UI::Row::Create(this);

  uipseudo = UI::Input::Create(this);
  uipseudo->SetSize(200, 40);

  warning = UI::Label::Create(this);
  warning->SetVisible(false);

  quit->SetOnClick(std::bind(&NewGame::OnActionClick, this, _1));
  previous->SetOnClick(std::bind(&NewGame::OnActionClick, this, _1));
  next->SetOnClick(std::bind(&NewGame::OnActionClick, this, _1));
  validate->SetOnClick(std::bind(&NewGame::OnActionClick, this, _1));

  AddActionButton(quit);
  AddActionButton(previous);
  AddActionButton(next);
  AddActionButton(validate);

  newLanguage();
}

NewGame::~NewGame() {
  delete quit;
  delete validate;
  delete previous;
  delete next;
  delete uipseudo;
}

NewGame *NewGame::Create() { return new (std::nothrow) NewGame(); }

void NewGame::OnScreenResize() {
  UI::Dialog::OnScreenResize();
  if (bounding_rect_.width() < 800 || bounding_rect_.height() < 600) {
    quit->SetSize(83 / 2, 94 / 2);
    validate->SetSize(83 / 2, 94 / 2);
    previous->SetSize(83, 94);
    next->SetSize(83, 94);
  } else {
    quit->SetSize(83, 94);
    validate->SetSize(83, 94);
    previous->SetSize(83, 94);
    next->SetSize(83, 94);
  }

  unsigned int space = 30;
  if (bounding_rect_.width() < 600 || bounding_rect_.height() < 480) {
    space = 10;
  }

  uipseudo->SetPos(BoundingRect().width() / 2 - uipseudo->Width() / 2,
                   y_ + background_->Height() - uipseudo->Height() - space -
                       validate->Height() / 2);

  row_->SetPos(BoundingRect().width() / 2 - row_->Width() / 2,
               uipseudo->Y() - row_->Height() - 30);
}

void NewGame::newLanguage() { SetTitle(tr("Select")); }

bool NewGame::isOk() const { return ok; }

bool NewGame::haveSkin() const { return skinList.size() > 0; }

void NewGame::OnActionClick(Node *node) {
  if (node == quit) {
    Close();
  } else if (node == validate) {
    on_ok_clicked();
  } else if (node == previous) {
    on_previous_clicked();
  } else if (node == next) {
    on_next_clicked();
  }
}

void NewGame::setDatapack(
    const std::string &skinPath, const std::string &monsterPath,
    std::vector<std::vector<CatchChallenger::Profile::Monster>> monstergroup,
    const std::vector<uint8_t> &forcedSkin) {
  this->forcedSkin.clear();
  this->monstergroup.clear();
  this->step = Step1;
  this->skinLoaded = false;
  this->skinList.clear();
  this->skinListId.clear();

  this->forcedSkin = forcedSkin;
  this->monsterPath = monsterPath;
  this->monstergroup = monstergroup;
  ok = true;
  step = Step1;
  currentMonsterGroup = 0;
  if (!monstergroup.empty()) {
    currentMonsterGroup = static_cast<uint8_t>(rand() % monstergroup.size());
  }
  this->skinPath = skinPath;
  uint8_t index = 0;
  while (index < CatchChallenger::CommonDatapack::commonDatapack.skins.size()) {
    if (forcedSkin.empty() ||
        vectorcontainsAtLeastOne(forcedSkin, (uint8_t)index)) {
      const std::string &currentPath =
          skinPath +
          CatchChallenger::CommonDatapack::commonDatapack.skins.at(index);
      if (QFile::exists(QString::fromStdString(currentPath + "/back.png")) &&
          QFile::exists(QString::fromStdString(currentPath + "/front.png")) &&
          QFile::exists(QString::fromStdString(currentPath + "/trainer.png"))) {
        skinList.push_back(
            CatchChallenger::CommonDatapack::commonDatapack.skins.at(index));
        skinListId.push_back(index);
      }
    }
    index++;
  }

  uipseudo->SetMaxLength(
      CommonSettingsCommon::commonSettingsCommon.max_pseudo_size);
  previous->SetVisible(skinList.size() >= 2);
  next->SetVisible(skinList.size() >= 2);

  currentSkin = 0;
  if (!skinList.empty())
    currentSkin = static_cast<uint8_t>(rand() % skinList.size());
  updateSkin();
  uipseudo->SetFocus(true);
  if (skinList.empty()) {
    warning->SetText(tr("No skin to select!"));
    warning->SetVisible(true);
    return;
  }
}

void NewGame::updateSkin() {
  skinLoaded = false;

  std::vector<std::string> paths;
  if (step == Step1) {
    if (currentSkin >= skinList.size()) return;
    previous->SetEnabled(currentSkin > 0);
    next->SetEnabled(currentSkin < (skinList.size() - 1));
    paths.push_back(skinPath + skinList.at(currentSkin) + "/front.png");
  } else if (step == Step2) {
    if (currentMonsterGroup >= monstergroup.size()) return;
    previous->SetEnabled(currentMonsterGroup > 0);
    next->SetEnabled(currentMonsterGroup < (monstergroup.size() - 1));
    const std::vector<CatchChallenger::Profile::Monster> &monsters =
        monstergroup.at(currentMonsterGroup);
    unsigned int index = 0;
    while (index < monsters.size()) {
      const CatchChallenger::Profile::Monster &monster = monsters.at(index);
      paths.push_back(monsterPath + std::to_string(monster.id) + "/front.png");
      index++;
    }
  } else {
    return;
  }

  row_->RemoveAllChildrens();

  if (!paths.empty()) {
    unsigned int index = 0;
    while (index < paths.size()) {
      const std::string &path = paths.at(index);

      QImage skin = QImage(QString::fromStdString(path));
      if (skin.isNull()) {
        warning->SetText(tr("But the skin can't be loaded: %1")
                             .arg(QString::fromStdString(path)));
        warning->SetVisible(true);
        return;
      }
      QImage scaledSkin = skin.scaled(160, 160, Qt::IgnoreAspectRatio);
      QPixmap pixmap;
      pixmap.convertFromImage(scaledSkin);
      auto item = Sprite::Create(this);
      item->SetPixmap(pixmap);
      row_->AddChild(item);
      skinLoaded = true;

      index++;
    }
    row_->SetX(BoundingRect().width() / 2 - row_->Width() / 2);
  } else {
    skinLoaded = false;
  }
}

bool NewGame::haveTheInformation() {
  return okCanBeEnabled() && step == StepOk;
}

bool NewGame::okCanBeEnabled() {
  return !uipseudo->Value().isEmpty() && skinLoaded;
}

std::string NewGame::pseudo() { return uipseudo->Value().toStdString(); }

std::string NewGame::skin() { return skinList.at(currentSkin); }

uint8_t NewGame::skinId() { return skinListId.at(currentSkin); }

uint8_t NewGame::monsterGroupId() { return currentMonsterGroup; }

void NewGame::on_ok_clicked() {
  if (step == Step1) {
    if (uipseudo->Value().isEmpty()) {
      warning->SetText(tr("Your pseudo can't be empty"));
      warning->SetVisible(true);
      return;
    }
    if (uipseudo->Value().size() >
        CommonSettingsCommon::commonSettingsCommon.max_pseudo_size) {
      warning->SetText(
          tr("Your pseudo can't be greater than %1")
              .arg(CommonSettingsCommon::commonSettingsCommon.max_pseudo_size));
      warning->SetVisible(true);
      return;
    }
    step = Step2;
    uipseudo->SetVisible(false);
    updateSkin();
    if (monstergroup.size() < 2) on_ok_clicked();
    if (uipseudo->Value().contains(" ")) {
      warning->SetText(tr("Your pseudo can't contains space"));
      warning->SetVisible(true);
      return;
    }
  } else if (step == Step2) {
    step = StepOk;
    ok = true;
    Close();
  } else {
    return;
  }
}

void NewGame::on_pseudo_textChanged(const QString &) {
  validate->SetEnabled(okCanBeEnabled());
}

void NewGame::on_pseudo_returnPressed() { on_ok_clicked(); }

void NewGame::on_next_clicked() {
  if (step == Step1) {
    if (currentSkin < (skinList.size() - 1))
      currentSkin++;
    else
      return;
    updateSkin();
  } else if (step == Step2) {
    if (currentMonsterGroup < (monstergroup.size() - 1))
      currentMonsterGroup++;
    else
      return;
    updateSkin();
  } else
    return;
}

void NewGame::on_previous_clicked() {
  if (step == Step1) {
    if (currentSkin == 0) return;
    currentSkin--;
    updateSkin();
  } else if (step == Step2) {
    if (currentMonsterGroup == 0) return;
    currentMonsterGroup--;
    updateSkin();
  } else
    return;
}

void NewGame::OnEnter() {
  UI::Dialog::OnEnter();
  uipseudo->RegisterEvents();
}

void NewGame::OnExit() {
  uipseudo->UnRegisterEvents();
  UI::Dialog::OnExit();
}
