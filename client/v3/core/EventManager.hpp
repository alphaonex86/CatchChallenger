// Copyright 2021 CatchChallenger
#ifndef CLIENT_QTOPENGL_CORE_EVENTMANAGER_HPP_
#define CLIENT_QTOPENGL_CORE_EVENTMANAGER_HPP_

#include <QKeyEvent>
#include <QPointF>
#include <cstdint>
#include <vector>

class Node;
class HashElementEvent;
class HashScene;

class EventManager {
 public:
  ~EventManager();

  static EventManager *GetInstance();
  void AddMouseListener(Node *listener);
  void AddKeyboardListener(Node *listener);
  void RemoveAllListeners();
  void RemoveListener(Node *listener);
  void Lock();
  void UnLock();
  void PrintData();

  void MousePressEvent(const QPointF &point, bool &press_validated);
  void MouseReleaseEvent(const QPointF &point, bool &prev_validated);
  void MouseMoveEvent(const QPointF &point);
  void KeyPressEvent(QKeyEvent *event, bool &event_trigger);
  void KeyReleaseEvent(QKeyEvent *event, bool &event_trigger);

 private:
  EventManager();

  HashElementEvent *DeleteHashElement(HashElementEvent *item, HashElementEvent *element);
  HashScene *DeleteHashScene(HashScene *item, HashScene *element);
  HashScene *RemoveListener(HashScene *scene, Node *listener);
  HashScene *AddListener(HashScene *scene, Node *listener, bool paused);
  struct HashElementEvent *current_target_;
  bool current_target_salvaged_;

 protected:
  static EventManager *instance_;
  struct HashScene *mouse_listeners_;
  struct HashScene *keyboard_listeners_;
};
#endif  // CLIENT_QTOPENGL_CORE_EVENTMANAGER_HPP_
