// Copyright 2021 CatchChallenger
#include "Loading.hpp"

#include <iostream>

#include "../../../../general/base/Version.hpp"
#include "../../action/CallFunc.hpp"
#include "../../action/Delay.hpp"
#include "../../action/RepeatForever.hpp"
#include "../../action/Sequence.hpp"
#include "../../core/AssetsLoader.hpp"
#include "../../core/SceneManager.hpp"
#include "../../core/Sprite.hpp"

using Scenes::Loading;
using std::placeholders::_1;

Loading* Loading::instance_ = nullptr;

Loading::Loading(): Scene(nullptr) {
  widget = Sprite::Create(":/CC/images/interface/message.png", this);
  teacher = Sprite::Create(":/CC/images/interface/teacher.png", widget);
  info = UI::Label::Create(widget);
  version = UI::Label::Create(this);
  version->SetText(QString::fromStdString(CatchChallenger::Version::str));
  progressbar = UI::Progressbar::Create(this);

  doTheNext = false;

  // slow down progression
  timer_ = RepeatForever::Create(Sequence::Create(
      Delay::Create(20),
      CallFunc::Create(std::bind(&Loading::UpdateProgression, this)), nullptr));
  Reset();
}

Loading::~Loading() {
  delete widget;
  delete teacher;
  delete info;
  delete progressbar;
  delete timer_;
}

Loading* Loading::GetInstance() {
  if (!instance_) {
    instance_ = new (std::nothrow) Loading();
  }
  return instance_;
}

void Loading::Reset() {
  info->SetText(tr("%1 is loading...").arg("CatchChallenger"));
  progressbar->SetMaximum(100);
  progressbar->SetMinimum(0);
  progressbar->SetValue(0);
  lastProgression = 0;
  timerProgression = 0;
}

void Loading::CanBeChanged() {
  if (doTheNext) {
    // if (on_finish_) on_finish_();
  } else {
    doTheNext = true;
  }
}

void Loading::DataIsParsed() {
  if (doTheNext) {
    // if (on_finish_) on_finish_();
  } else {
    doTheNext = true;
    info->SetText(tr("%1 is loaded").arg("CatchChallenger"));
  }
}

void Loading::UpdateProgression() {
  if (timerProgression < 100) {
    timerProgression += 5;
    if (timerProgression < lastProgression)
      progressbar->SetValue(timerProgression);
    else
      progressbar->SetValue(lastProgression);
  }
  if (timerProgression >= 100) {
    CanBeChanged();
    timer_->Stop();
  }
}

void Loading::Progression(uint32_t size, uint32_t total) {
  if (total == 0) {
    progressbar->SetValue(0);
    return;
  }
  if (size <= total) {
    lastProgression = size * 100 / total;
    if (timerProgression < lastProgression)
      progressbar->SetValue(timerProgression);
    else
      progressbar->SetValue(lastProgression);
  } else {
    progressbar->SetValue(progressbar->Maximum());  // abort();
  }
}

void Loading::SetText(QString text) {
  if (!text.isEmpty())
    info->SetText(text);
  else
    info->SetText(tr("%1 is loading...").arg("CatchChallenger"));
}

void Loading::OnScreenResize() {
  int progressBarHeight = 82;
  uint8_t widget_border = 46;
  if (BoundingRect().height() < 800) {
    progressBarHeight = BoundingRect().height() / 10;
    if (progressBarHeight < 24) progressBarHeight = 24;
  }
  progressbar->SetPos(10, BoundingRect().height() - progressBarHeight - 10);
  progressbar->SetSize(BoundingRect().width() - 10 - 10, progressBarHeight);

  if (BoundingRect().width() < 800 || BoundingRect().height() < 600) {
    int w = BoundingRect().width() - 20;
    if (w > 500) w = 500;
    int h = BoundingRect().height() - 40 - progressBarHeight;
    if (h > 100) h = 100;
    widget->Strech(widget_border, w, h);
    widget->SetPos(BoundingRect().width() / 2 - widget->Width() / 2,
                   (BoundingRect().height() - progressBarHeight) / 2 -
                       widget->Height() / 2);

    teacher->SetVisible(false);
    info->SetPos(widget_border, h / 2 - info->BoundingRect().height() / 2);
  } else {
    int w = 500;
    if (BoundingRect().width() < 500 + 20) w = BoundingRect().width() - 20;
    widget->Strech(widget_border, w, 250);
    widget->SetPos(BoundingRect().width() / 2 - widget->Width() / 2,
                   BoundingRect().height() / 2 - widget->Height() / 2);

    teacher->SetVisible(true);
    teacher->SetPos(24, 19);
    info->SetPos(24 + teacher->Width() + 10, 120);
  }

  version->SetPos(BoundingRect().width() - version->BoundingRect().width() - 10,
                  5);
  QFont font = version->GetFont();
  if (BoundingRect().height() < 500)
    font.setPixelSize(9);
  else if (widget->Height() < 800)
    font.setPixelSize(11);
  else
    font.setPixelSize(14);
  version->SetFont(font);
}

void Loading::SetNotifier(ProgressNotifier *notifier) {
  notifier->SetOnProgress(std::bind(&Loading::Progression, this, _1, _1));
  notifier->SetOnStatusChange(std::bind(&Loading::SetText, this, _1));
}

void Loading::OnEnter() {
  Scene::OnEnter();
  RunAction(timer_);
}

void Loading::OnExit() {
  timer_->Stop();
}
