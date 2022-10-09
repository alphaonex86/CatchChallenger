// Copyright 2021 CatchChallenger
#include "StackedScene.hpp"

#include <iostream>

#include "Node.hpp"
#include "Scene.hpp"

StackedScene::StackedScene(Node *parent) : Scene(parent) {
  background_ = nullptr;
  foreground_.clear();
}

StackedScene::~StackedScene() {}

void StackedScene::SetBackground(Scene *scene) {
  scene->SetZValue(-1);
  RemoveChild(background_);
  background_ = scene;
  AddChild(background_);
}

void StackedScene::PushForeground(Scene *scene) {
  if (!foreground_.empty()) {
    auto tmp = foreground_.back();
    if (tmp == scene) {
      return;
    }
    RemoveChild(tmp);
  }

  scene->SetZValue(0);
  foreground_.push_back(scene);
  AddChild(scene);
}

void StackedScene::ReplaceForeground(Scene *previous, Scene *next) {
  if (foreground_.empty()) return;
  int iter = 0;
  int index = -1;
  while (iter < foreground_.size()) {
    if (foreground_.at(iter) == previous) {
      index = iter;
      break;
    }
    iter++;
  }
  if (index >= 0) {
    std::replace(foreground_.begin(), foreground_.end(), previous, next);
    if (index == foreground_.size() - 1) {
      RemoveChild(previous);
      AddChild(next);
    }
  }
}

Scene *StackedScene::PopForeground() {
  if (foreground_.empty()) return nullptr;
  auto tmp = foreground_.back();

  RemoveChild(tmp);
  foreground_.pop_back();

  if (!foreground_.empty()) AddChild(foreground_.back());

  return tmp;
}

Scene *StackedScene::PopForegroundUntilIndex(int index) {
  int iter = foreground_.size() - 1;
  Scene* scene = nullptr;

  while (iter > index) {
    scene = PopForeground();
    iter--;
  }

  return scene;
}

void StackedScene::OnEnter() {
  Scene::OnEnter();

  if (background_ != nullptr) {
    background_->OnEnter();
  }
  if (!foreground_.empty()) {
    foreground_.back()->OnEnter();
  }
}

void StackedScene::OnExit() {
  if (background_ != nullptr) {
    background_->OnExit();
  }
  if (!foreground_.empty()) {
    foreground_.back()->OnExit();
  }

  Scene::OnExit();
}
