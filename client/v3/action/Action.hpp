// Copyright 2021 CatchChallenger
#ifndef CLIENT_QTOPENGL_ACTION_ACTION_HPP_
#define CLIENT_QTOPENGL_ACTION_ACTION_HPP_

class Node;

class Action {
 public:
  ~Action();
  void SetTarget(Node *node);
  Node *Target();
  void SetTag(int tag);
  int Tag();
  virtual void Step(unsigned int millisecond) = 0;
  virtual void Stop() = 0;
  virtual void Start() = 0;
  virtual bool IsDone() = 0;

 protected:
  Node *node_;

  Action();
  void OnFinish();

 private:
  int tag_;
};
#endif  // CLIENT_QTOPENGL_ACTION_ACTION_HPP_
