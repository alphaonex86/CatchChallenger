// Copyright 2021 CatchChallenger
#include "EventManager.hpp"

#include <cstdint>
#include <iostream>
#include <vector>

#include "../entities/Utils.hpp"
#include "Logger.hpp"
#include "Node.hpp"
#include "VirtualInput.hpp"
#include "uthash.h"

typedef struct HashElement {
  Node *target;
  int z;
  int8_t paused;
  bool must_remove;
  UT_hash_handle hh;
} HashElementType;

typedef struct HashScene {
  Node *scene;
  int z;
  HashElementType *listeners;
  UT_hash_handle hh;
} HashSceneType;

EventManager *EventManager::instance_ = nullptr;

EventManager::EventManager()
    : mouse_listeners_(nullptr), keyboard_listeners_(nullptr) {
  current_target_ = nullptr;
  current_target_salvaged_ = false;
}

EventManager::~EventManager() {}

EventManager *EventManager::GetInstance() {
  if (instance_ == nullptr) instance_ = new EventManager();
  return instance_;
}

void EventManager::AddMouseListener(Node *listener) {
  mouse_listeners_ = AddListener(mouse_listeners_, listener, false);
}

void EventManager::AddKeyboardListener(Node *listener) {
  keyboard_listeners_ = AddListener(keyboard_listeners_, listener, false);
}

int scene_sort(HashSceneType *a, HashSceneType *b) { return (b->z - a->z); }

int listener_sort(HashElementType *a, HashElementType *b) {
  return (b->z - a->z);
}

HashSceneType *EventManager::AddListener(HashSceneType *scene, Node *listener,
                                         bool paused) {
  // Verify if the listener added it will be delete, if true the prevent the
  // delete
  if (current_target_ != nullptr && current_target_salvaged_ &&
      listener == current_target_->target) {
    current_target_->paused = 0;
    current_target_salvaged_ = 0;
  }

  HashSceneType *root_scene = nullptr;
  auto *root = Utils::GetCurrentScene(listener);
  if (root == nullptr) {
    return nullptr;
  }
  HASH_FIND_PTR(scene, &root, root_scene);
  if (!root_scene) {
    root_scene = (HashSceneType *)calloc(sizeof(*root_scene), 1);
    root_scene->scene = root;
    root_scene->listeners = nullptr;
    root_scene->z = root->ZValue();
    HASH_ADD_PTR(scene, scene, root_scene);
    HASH_SORT(scene, scene_sort);
  }

  HashElementType *element = nullptr;
  HASH_FIND_PTR(root_scene->listeners, &listener, element);
  if (!element) {
    element = (HashElementType *)calloc(sizeof(*element), 1);
    element->paused = paused ? 1 : 0;
    element->target = listener;
    element->z = listener->ZValue();
    HASH_ADD_PTR(root_scene->listeners, target, element);
    HASH_SORT(root_scene->listeners, listener_sort);
  }
  return scene;
}

HashElementType *EventManager::DeleteHashElement(HashElementType *item,
                                                 HashElementType *element) {
  HASH_DEL(item, element);
  free(element);
  return item;
}

HashSceneType *EventManager::DeleteHashScene(HashSceneType *item,
                                             HashSceneType *element) {
  HASH_DEL(item, element);
  free(element);
  return item;
}

void EventManager::RemoveListener(Node *listener) {
  mouse_listeners_ = RemoveListener(mouse_listeners_, listener);
  keyboard_listeners_ = RemoveListener(keyboard_listeners_, listener);
}

HashSceneType *EventManager::RemoveListener(HashSceneType *scene,
                                            Node *listener) {
  if (listener == nullptr) {
    return scene;
  }
  HashSceneType *root_scene = nullptr;
  auto *root = Utils::GetCurrentScene(listener);
  HASH_FIND_PTR(scene, &root, root_scene);

  if (!root_scene) {
    return scene;
  }

  HashElementType *element = nullptr;
  HASH_FIND_PTR(root_scene->listeners, &listener, element);
  if (element) {
    if (current_target_ == element) {
      current_target_salvaged_ = true;
    } else {
      root_scene->listeners = DeleteHashElement(root_scene->listeners, element);
      if (HASH_COUNT(root_scene->listeners) == 0) {
        scene = DeleteHashScene(scene, root_scene);
      }
    }
  }
  return scene;
}

void EventManager::RemoveAllListeners() {
  for (HashSceneType *scene = mouse_listeners_; scene != nullptr;) {
    for (HashElementType *elt = scene->listeners; elt != nullptr;) {
      scene->listeners = DeleteHashElement(scene->listeners, elt);
      elt = (HashElementType *)(elt->hh.next);
    }
    auto *current_scene = scene;
    scene = (HashSceneType *)(scene->hh.next);
    if (HASH_COUNT(current_scene->listeners) == 0) {
      mouse_listeners_ = DeleteHashScene(mouse_listeners_, current_scene);
    }
  }
  for (HashSceneType *scene = keyboard_listeners_; scene != nullptr;) {
    for (HashElementType *elt = scene->listeners; elt != nullptr;) {
      scene->listeners = DeleteHashElement(scene->listeners, elt);
      elt = (HashElementType *)(elt->hh.next);
    }
    auto *current_scene = scene;
    scene = (HashSceneType *)(scene->hh.next);
    if (HASH_COUNT(current_scene->listeners) == 0) {
      keyboard_listeners_ = DeleteHashScene(keyboard_listeners_, current_scene);
    }
  }
}

void EventManager::MousePressEvent(const QPointF &point,
                                   bool &press_validated) {
  for (HashSceneType *scene = mouse_listeners_; scene != nullptr;) {
    auto *current_scene = scene;
    for (HashElementType *elt = scene->listeners; elt != nullptr;) {
      current_target_ = elt;
      current_target_salvaged_ = false;

      if (!current_target_->paused) {
        current_target_->target->MousePressEvent(point, press_validated);
      }

      elt = (HashElementType *)(elt->hh.next);
      if (current_target_salvaged_) {
        scene->listeners = DeleteHashElement(scene->listeners, current_target_);
        if (HASH_COUNT(scene->listeners) == 0) {
          scene = DeleteHashScene(mouse_listeners_, scene);
        }
      }
    }
    scene = (HashSceneType *)(scene->hh.next);
    if (HASH_COUNT(current_scene->listeners) == 0) {
      mouse_listeners_ = DeleteHashScene(mouse_listeners_, current_scene);
    }
  }

  current_target_ = nullptr;
}

void EventManager::MouseReleaseEvent(const QPointF &point,
                                     bool &prev_validated) {
  for (HashSceneType *scene = mouse_listeners_; scene != nullptr;) {
    auto *current_scene = scene;
    for (HashElementType *elt = scene->listeners; elt != nullptr;) {
      current_target_ = elt;
      current_target_salvaged_ = false;

      if (current_target_->paused == 0) {
        current_target_->target->MouseReleaseEvent(point, prev_validated);
      }

      elt = (HashElementType *)(elt->hh.next);
      if (current_target_salvaged_) {
        scene->listeners = DeleteHashElement(scene->listeners, current_target_);
      }
    }
    scene = (HashSceneType *)(scene->hh.next);
    if (HASH_COUNT(current_scene->listeners) == 0) {
      mouse_listeners_ = DeleteHashScene(mouse_listeners_, current_scene);
    }
  }

  current_target_ = nullptr;
}

void EventManager::MouseMoveEvent(const QPointF &point) {
  for (HashSceneType *scene = mouse_listeners_; scene != nullptr;) {
    auto *current_scene = scene;
    for (HashElementType *elt = scene->listeners; elt != nullptr;) {
      current_target_ = elt;
      current_target_salvaged_ = false;

      if (current_target_->paused == 0) {
        current_target_->target->MouseMoveEvent(point);
      }

      elt = (HashElementType *)(elt->hh.next);
      if (current_target_salvaged_) {
        scene->listeners = DeleteHashElement(scene->listeners, current_target_);
      }
    }
    scene = (HashSceneType *)(scene->hh.next);
    if (HASH_COUNT(current_scene->listeners) == 0) {
      mouse_listeners_ = DeleteHashScene(mouse_listeners_, current_scene);
    }
  }

  current_target_ = nullptr;
}

void EventManager::KeyPressEvent(QKeyEvent *event, bool &event_trigger) {
  for (HashSceneType *scene = keyboard_listeners_; scene != nullptr;) {
    auto *current_scene = scene;
    for (HashElementType *elt = scene->listeners; elt != nullptr;) {
      current_target_ = elt;
      current_target_salvaged_ = false;

      if (current_target_->paused == 0) {
        current_target_->target->KeyPressEvent(event, event_trigger);
      }

      elt = (HashElementType *)(elt->hh.next);
      if (current_target_salvaged_) {
        scene->listeners = DeleteHashElement(scene->listeners, current_target_);
      }
    }
    scene = (HashSceneType *)(scene->hh.next);
    if (HASH_COUNT(current_scene->listeners) == 0) {
      keyboard_listeners_ = DeleteHashScene(keyboard_listeners_, current_scene);
    }
  }

  current_target_ = nullptr;
}

void EventManager::KeyReleaseEvent(QKeyEvent *event, bool &event_trigger) {
  for (HashSceneType *scene = keyboard_listeners_; scene != nullptr;) {
    auto *current_scene = scene;
    for (HashElementType *elt = scene->listeners; elt != nullptr;) {
      current_target_ = elt;
      current_target_salvaged_ = false;

      if (current_target_->paused == 0) {
        current_target_->target->KeyReleaseEvent(event, event_trigger);
      }

      elt = (HashElementType *)(elt->hh.next);
      if (current_target_salvaged_) {
        scene->listeners = DeleteHashElement(scene->listeners, current_target_);
        if (HASH_COUNT(scene->listeners) == 0) {
          scene = DeleteHashScene(keyboard_listeners_, scene);
        }
      }
    }
    scene = (HashSceneType *)(scene->hh.next);
    if (HASH_COUNT(current_scene->listeners) == 0) {
      keyboard_listeners_ = DeleteHashScene(keyboard_listeners_, current_scene);
    }
  }

  current_target_ = nullptr;
}

void EventManager::Lock() {
  for (HashSceneType *scene = mouse_listeners_; scene != nullptr;) {
    for (HashElementType *elt = scene->listeners; elt != nullptr;) {
      elt->paused++;

      elt = (HashElementType *)(elt->hh.next);
    }
    scene = (HashSceneType *)(scene->hh.next);
  }
  for (HashSceneType *scene = keyboard_listeners_; scene != nullptr;) {
    for (HashElementType *elt = scene->listeners; elt != nullptr;) {
      elt->paused++;

      elt = (HashElementType *)(elt->hh.next);
    }
    scene = (HashSceneType *)(scene->hh.next);
  }
}

void EventManager::UnLock() {
  for (HashSceneType *scene = mouse_listeners_; scene != nullptr;) {
    for (HashElementType *elt = scene->listeners; elt != nullptr;) {
      elt->paused--;
      if (elt->paused < 0) {
        elt->paused = 0;
      }

      elt = (HashElementType *)(elt->hh.next);
    }
    scene = (HashSceneType *)(scene->hh.next);
  }
  for (HashSceneType *scene = keyboard_listeners_; scene != nullptr;) {
    for (HashElementType *elt = scene->listeners; elt != nullptr;) {
      elt->paused--;
      if (elt->paused < 0) {
        elt->paused = 0;
      }

      elt = (HashElementType *)(elt->hh.next);
    }
    scene = (HashSceneType *)(scene->hh.next);
  }
}

void EventManager::PrintData() {
  Logger::Log(QString("Events:"));
  for (HashSceneType *scene = mouse_listeners_; scene != nullptr;) {
    intptr_t point = reinterpret_cast<intptr_t>(scene->scene);
    Logger::Log(QString("Scene %1 z: %2").arg(point).arg(scene->z));
    for (HashElementType *elt = scene->listeners; elt != nullptr;) {
      intptr_t point_1 = reinterpret_cast<intptr_t>(elt->target);
      Logger::Log(QString("Listener %1 z: %2 paused: %3")
                      .arg(point_1)
                      .arg(elt->z)
                      .arg(elt->paused));
      elt = (HashElementType *)(elt->hh.next);
    }

    scene = (HashSceneType *)(scene->hh.next);
  }
}
