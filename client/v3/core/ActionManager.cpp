// Copyright 2021 CatchChallenger
#include "ActionManager.hpp"

#include <chrono>
#include <cstdint>
#include <iostream>
#include <vector>

#include "../action/Action.hpp"
#include "Node.hpp"
#include "uthash.h"

typedef struct HashElement {
  Node *target;
  std::vector<Action *> *actions;
  Action *current_action;
  bool paused;
  bool current_action_salvaged;
  bool deletable;
  UT_hash_handle hh;
} HashElementType;

ActionManager *ActionManager::instance_ = nullptr;

ActionManager::ActionManager() : targets_(nullptr), current_target_(nullptr) {
  last_execution_ = std::chrono::high_resolution_clock::now();
}

ActionManager::~ActionManager() { RemoveAllActions(); }

ActionManager *ActionManager::GetInstance() {
  if (instance_ == nullptr) instance_ = new ActionManager();
  return instance_;
}

void ActionManager::AddAction(Action *action, Node *target, bool paused,
                              bool deletable) {
  if (action == nullptr || target == nullptr) return;

  HashElementType *element = nullptr;
  HASH_FIND_PTR(targets_, &target, element);
  if (!element) {
    element = (HashElementType *)calloc(sizeof(*element), 1);
    element->paused = paused;
    element->target = target;
    element->deletable = deletable;
    element->actions = new std::vector<Action *>();
    HASH_ADD_PTR(targets_, target, element);
  }

  element->actions->push_back(action);
  action->SetTarget(target);
  action->Start();
}

Action *ActionManager::GetActionByTag(int tag, Node *target) {
  HashElementType *element = nullptr;
  HASH_FIND_PTR(targets_, &target, element);

  if (element) {
    if (element->actions != nullptr) {
      auto length = element->actions->size();
      uint index = 0;
      while (index < length) {
        Action *action = element->actions->at(index);
        if (action->Tag() == tag) {
          return action;
        }
        index++;
      }
    }
  }

  return nullptr;
}

std::vector<Action *> *ActionManager::GetAllActionsByTarget(Node *target) {
  HashElementType *element = nullptr;
  HASH_FIND_PTR(targets_, &target, element);

  if (element) {
    if (element->actions != nullptr) {
      return element->actions;
    }
  }

  return nullptr;
}

void ActionManager::RemoveAllActions() {
  for (HashElementType *element = targets_; element != nullptr;) {
    auto target = element->target;
    element = (HashElementType *)element->hh.next;
    RemoveAllActionsFromTarget(target);
  }
}

void ActionManager::RemoveAllActionsFromTarget(Node *target) {
  if (target == nullptr) {
    return;
  }

  HashElementType *element = nullptr;
  HASH_FIND_PTR(targets_, &target, element);
  if (element) {
    auto length = element->actions->size();
    uint index = 0;
    bool contains_element = false;
    while (index < length) {
      if (element->actions->at(index) == element->current_action) {
        contains_element = true;
        break;
      }
      index++;
    }
    if (contains_element && (!element->current_action_salvaged)) {
      // element->current_action->retain();
      element->current_action_salvaged = true;
    }

    if (element->deletable) {
      size_t length = element->actions->size();
      size_t index = 0;
      while (index < length) {
        Action *action = element->actions->at(index);
        delete action;
        index++;
      }
    }

    element->actions->clear();
    if (current_target_ == element) {
      current_target_salvaged_ = true;
    } else {
      DeleteHashElement(element);
    }
  }
}

void ActionManager::RemoveAction(Action *action) {
  // explicit null handling
  if (action == nullptr) {
    return;
  }

  HashElementType *element = nullptr;
  Node *target = action->Target();
  HASH_FIND_PTR(targets_, &target, element);
  if (element) {
    auto length = element->actions->size();
    uint index = 0;
    while (index < length) {
      if (element->actions->at(index) == action) {
        RemoveActionAtIndex(index, element);
        break;
      }
    }
  }
}

void ActionManager::RemoveActionAtIndex(unsigned int index,
                                        HashElementType *element) {
  Action *action = static_cast<Action *>(element->actions->at(index));

  if (action == element->current_action &&
      (!element->current_action_salvaged)) {
    // element->currentAction->retain();
    element->current_action_salvaged = true;
  }
  if (element->deletable) {
    delete action;
  }

  element->actions->erase(element->actions->begin() + index);

  if (element->actions->size() == 0) {
    if (current_target_ == element) {
      current_target_salvaged_ = true;
    } else {
      DeleteHashElement(element);
    }
  }
}

void ActionManager::Update() {
  auto now = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double, std::milli> span = now - last_execution_;
  unsigned int ellapsed = span.count();
  last_execution_ = now;
  for (HashElementType *elt = targets_; elt != nullptr;) {
    current_target_ = elt;
    current_target_salvaged_ = false;

    if (!current_target_->paused) {
      size_t length = elt->actions->size();
      size_t index = 0;
      // for (auto it = elt->actions->begin(); it != elt->actions->end(); ++it)
      // {
      while (index < length) {
        Action *action = elt->actions->at(index);
        // Action *action = (*it);
        current_target_->current_action = action;
        if (current_target_->current_action == nullptr) {
          continue;
        }
        current_target_->current_action_salvaged = false;
        current_target_->current_action->Step(ellapsed);

        if (current_target_->current_action->IsDone()) {
          current_target_->current_action->Stop();

          Action *action = current_target_->current_action;
          current_target_->current_action = nullptr;
          RemoveAction(action);
          length = elt->actions->size();
        }

        current_target_->current_action = nullptr;
        index++;
      }
    }

    elt = (HashElementType *)(elt->hh.next);
    if (current_target_salvaged_ && current_target_->actions->empty()) {
      DeleteHashElement(current_target_);
    }
  }

  current_target_ = nullptr;
}

void ActionManager::DeleteHashElement(HashElementType *element) {
  element->actions->clear();
  delete element->actions;
  HASH_DEL(targets_, element);
  free(element);
}

size_t ActionManager::GetNumberOfRunningActionsInTarget(Node *target) {
  auto actions = GetAllActionsByTarget(target);
  if (actions == nullptr) return 0;
  return actions->size();
}
