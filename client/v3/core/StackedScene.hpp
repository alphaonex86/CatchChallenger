// Copyright 2021 CatchChallenger
#ifndef CLIENT_QTOPENGL_CORE_STACKEDSCENE_HPP_
#define CLIENT_QTOPENGL_CORE_STACKEDSCENE_HPP_

#include <vector>

#include "Node.hpp"
#include "Scene.hpp"

class StackedScene : public Scene {
 public:
  ~StackedScene();

  void SetBackground(Scene *scene);
  void PushForeground(Scene *scene);
  void ReplaceForeground(Scene *previous, Scene *next);
  Scene *PopForeground();
  Scene *PopForegroundUntilIndex(int index);
  void OnEnter() override;
  void OnExit() override;
  virtual void Restart() = 0;

 protected:
  explicit StackedScene(Node *parent = nullptr);

 private:
  std::vector<Scene *> foreground_;
  Scene * background_;
};
#endif  // CLIENT_QTOPENGL_CORE_STACKEDSCENE_HPP_
