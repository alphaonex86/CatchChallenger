// Copyright 2021 CatchChallenger
#include "Test.hpp"

#include "../../action/CallFunc.hpp"
#include "../../action/MoveTo.hpp"
#include "../../action/Sequence.hpp"
#include "../../ui/ThemedButton.hpp"

#include <iostream>

using Scenes::Test;
using std::placeholders::_1;

Test::Test() : Scene(nullptr) {
  sprite_ = Sprite::Create(":/CC/images/interface/teacher.png", this);
  // sprite_2 = Sprite::Create(":/CC/images/interface/teacher.png", this);
  // sprite_3 = Sprite::Create(":/CC/images/interface/teacher.png", this);
  // sprite_4 = Sprite::Create(":/CC/images/interface/teacher.png", this);
  // sprite_5 = Sprite::Create(":/CC/images/interface/teacher.png", this);
  label_ = UI::Label::Create(this);
  button_ = UI::YellowButton::Create(this);
  button_->SetText("Click me");
  button_->SetOnClick(std::bind(&Test::OnButtonClick, this, _1));
}

Test::~Test() { delete sprite_; }

Test* Test::Create() { return new (std::nothrow) Test(); }

void Test::OnScreenResize() {
  uint8_t widget_border = 46;
  sprite_->Strech(widget_border, 70, 70);
  sprite_->SetPos(-600, 500);
  // sprite_->Strech(widget_border, 70, 70);
  // sprite_->SetPos(600, 500);
  // sprite_->Strech(widget_border, 70, 70);
  // sprite_->SetPos(700, 500);
  // sprite_->Strech(widget_border, 70, 70);
  // sprite_->SetPos(800, 500);
  // sprite_->Strech(widget_border, 70, 70); sprite_->SetPos(900, 500); label_->SetPos(200, 100);
  label_->SetWidth(60);
  label_->SetText(QString::fromStdString("bene gesserit"));
}

void Test::OnEnter() { button_->RegisterEvents(); }

void Test::OnExit() { button_->UnRegisterEvents(); }

void Test::OnButtonClick(Node* node) {
  std::vector<Action*> actions;

  std::cout<< "LAN_[" << __FILE__ << ":" << __LINE__ << "] "<< "asdas" << std::endl;
  actions.push_back(MoveTo::Create(1500, 192, 500));
  actions.push_back(
      CallFunc::Create([this]() { 
        std::cout << "call func" << std::endl; 
        //sprite_->RunAction(Sequence::Create(
            //MoveTo::Create(1000, 500, 500),
            //nullptr
              //), true);
        }));


  sprite_->RunAction(Sequence::Create(actions), true);
}
