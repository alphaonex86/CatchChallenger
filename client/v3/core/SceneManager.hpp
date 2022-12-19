// Copyright 2021 CatchChallenger
#ifndef CLIENT_QTOPENGL_CORE_SCENEMANAGER_HPP_
#define CLIENT_QTOPENGL_CORE_SCENEMANAGER_HPP_

#include <QGraphicsView>
#include <QTime>
#include <QTimer>
#include <QWidget>
#include <vector>
#include <QGraphicsScene>

#include "../../../general/base/GeneralStructures.hpp"
#include "../base/ConnectionManager.hpp"
#include "ActionManager.hpp"
#include "EventManager.hpp"
#include "GraphicRenderer.hpp"
#include "Scene.hpp"

namespace CatchChallenger {
class InternalServer;
}

class SceneManager : public QGraphicsView {
  Q_OBJECT

 public:
  static SceneManager *GetInstance();
  ~SceneManager();

  void PushScene(Scene *scene);
  Scene *PopScene();
  Scene *CurrentScene();
  void ShowOverlay(Node *overlay);
  void RemoveOverlay();
  bool IsOverlay(Node* overlay);
  QGraphicsItem *GetRenderer();
  void Restart();

#ifdef CATCHCHALLENGER_SOLO
  void RegisterInternalServerEvents();
  void IsStarted(bool started);
#endif

 protected:
  void resizeEvent(QResizeEvent *event) override;
  void mousePressEvent(QMouseEvent *event) override;
  void mouseReleaseEvent(QMouseEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;
  void closeEvent(QCloseEvent *event) override;
  void paintEvent(QPaintEvent *event) override;
  void keyPressEvent(QKeyEvent *event) override;
  void keyReleaseEvent(QKeyEvent *event) override;

 private:
  static SceneManager *instance_;
  SceneManager();
  GraphicRenderer *renderer_;
  ConnectionManager *connection_;
  QGraphicsScene *graphic_scene_;
  std::vector<Scene *> scene_stack_;
  Node *overlay_;
  bool should_resize_;
  bool should_shutdown_;

  uint8_t wait_render_time_;
  QTimer action_render_;
  QTime action_elapsed_;
  QTimer timer_render_;
  QTime time_elapsed_;
  uint16_t frame_counter_;
  QTimer timerUpdateFPS;
  QTime timeUpdateFPS;
  QGraphicsPixmapItem *imageText;
  int draw_times_;

  ActionManager *action_manager_;
  EventManager *event_manager_;

  void Render();
  void RenderActions();
  void UpdateFPS();
  void SetTargetFPS(int targetFPS);
  void OnError(std::string error);
  void OnDisconnect();
  void OnGoToMap();
  void OnResize();
};

#endif  // CLIENT_QTOPENGL_CORE_SCENEMANAGER_HPP_
