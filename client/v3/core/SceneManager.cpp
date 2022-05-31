// Copyright 2021 CatchChallenger
#include "SceneManager.hpp"

#ifdef CATCHCHALLENGER_SOLO
  #include <QStandardPaths>
#endif
#include "../../../general/base/CommonSettingsCommon.hpp"
#include "../../../general/base/Version.hpp"
#include "../Globals.hpp"
#include "../Options.hpp"
#ifndef CATCHCHALLENGER_NOAUDIO
  #include "AudioPlayer.hpp"
#endif
#include <QGLWidget>
#include <QOpenGLWidget>
#include <QSurfaceFormat>
#include <iostream>

using std::placeholders::_1;

SceneManager *SceneManager::instance_ = nullptr;

#ifdef __ANDROID_API__
  #include <QtAndroidExtras>

void keep_screen_on(bool on) {
  QtAndroid::runOnAndroidThread([on] {
    QAndroidJniObject activity = QtAndroid::androidActivity();
    if (activity.isValid()) {
      QAndroidJniObject window =
          activity.callObjectMethod("getWindow", "()Landroid/view/Window;");

      if (window.isValid()) {
        const int FLAG_KEEP_SCREEN_ON = 128;
        if (on) {
          window.callMethod<void>("addFlags", "(I)V", FLAG_KEEP_SCREEN_ON);
        } else {
          window.callMethod<void>("clearFlags", "(I)V", FLAG_KEEP_SCREEN_ON);
        }
      }
    }
    QAndroidJniEnvironment env;
    if (env->ExceptionCheck()) {
      env->ExceptionClear();
    }
  });
}
#endif

SceneManager::SceneManager() : graphic_scene_(new QGraphicsScene(this)) {
  {
    //QGLWidget *context = new QGLWidget(QGLFormat(QGL::SampleBuffers));
    QOpenGLWidget *context = new QOpenGLWidget();
    // if OpenGL is present, use it
    if (context->isValid()) {
       QSurfaceFormat format;
       format.setSamples(4);
       format.setSwapInterval(1);
       context->setFormat(format);

      setViewport(context);
      setRenderHint(QPainter::Antialiasing, true);
      setRenderHint(QPainter::TextAntialiasing, true);
      setRenderHint(QPainter::HighQualityAntialiasing, true);
      setRenderHint(QPainter::SmoothPixmapTransform, false);
      setRenderHint(QPainter::NonCosmeticDefaultPen, true);

      setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    }
    // else use the CPU only
  }

  should_shutdown_ = false;
  should_resize_ = false;
  overlay_ = nullptr;
  draw_times_ = 0;
  action_manager_ = ActionManager::GetInstance();
  event_manager_ = EventManager::GetInstance();
#ifdef CATCHCHALLENGER_SOLO
  Globals::InternalServer = nullptr;
#endif
#ifndef NOTHREADS
  Globals::ThreadSolo = new QThread(this);
#endif

  timerUpdateFPS.setSingleShot(true);
  timerUpdateFPS.setInterval(1000);
  timeUpdateFPS.restart();
  frame_counter_ = 0;
  time_elapsed_.restart();
  action_elapsed_.restart();
  wait_render_time_ = 40;
  timer_render_.setSingleShot(true);
  action_render_.setSingleShot(true);
  if (!connect(&timer_render_, &QTimer::timeout, this, &SceneManager::Render))
    abort();
  if (!connect(&action_render_, &QTimer::timeout, this,
               &SceneManager::RenderActions))
    abort();
  if (!connect(&timerUpdateFPS, &QTimer::timeout, this,
               &SceneManager::UpdateFPS))
    abort();
  timerUpdateFPS.start();

  setScene(graphic_scene_);
  graphic_scene_->setSceneRect(QRectF(0, 0, width(), height()));
  setOptimizationFlags(QGraphicsView::DontAdjustForAntialiasing |
                       QGraphicsView::DontSavePainterState |
                       QGraphicsView::IndirectPainting);
  setBackgroundBrush(Qt::black);
  setFrameStyle(0);
  viewport()->setAttribute(Qt::WA_StaticContents);
  viewport()->setAttribute(Qt::WA_TranslucentBackground);
  viewport()->setAttribute(Qt::WA_NoSystemBackground);
  setViewportUpdateMode(QGraphicsView::NoViewportUpdate);

#ifdef __ANDROID_API__
  keep_screen_on(true);
#endif

  connection_ = ConnectionManager::GetInstance();
  connection_->SetOnError(std::bind(&SceneManager::OnError, this, _1));
  connection_->SetOnDisconnect(std::bind(&SceneManager::OnDisconnect, this));

  imageText = new QGraphicsPixmapItem();
  graphic_scene_->addItem(imageText);
  imageText->setPos(0, 0);
  imageText->setZValue(999);

  action_render_.start(1);
  Render();

  SetTargetFPS(Options::GetInstance()->getFPS());
}

SceneManager::~SceneManager() {
#ifndef NOTHREADS
  Globals::ThreadSolo->exit();
  Globals::ThreadSolo->wait();
  Globals::ThreadSolo->deleteLater();
#endif
  delete action_manager_;
  delete event_manager_;
}

SceneManager *SceneManager::GetInstance() {
  if (!instance_) {
    instance_ = new SceneManager();
  }
  return instance_;
}

void SceneManager::resizeEvent(QResizeEvent *event) {
  (void)event;
  graphic_scene_->setSceneRect(QRectF(0, 0, width(), height()));
  should_resize_ = true;
}

void SceneManager::OnResize() {
  auto items = graphic_scene_->items();
  QList<QGraphicsItem *>::iterator i;
  for (i = items.begin(); i != items.end(); ++i) {
    auto scene = dynamic_cast<Scene *>(*i);
    if (scene == nullptr) {
      continue;
    }
    // Verify that node is not nullptr
    try {
      scene->SetSize(width(), height());
    } catch (const std::bad_alloc &) {
      continue;
    }
  }
}

void SceneManager::mousePressEvent(QMouseEvent *event) {
  bool call_parent_class = false;
  const QPointF &p = mapToScene(event->pos());
  bool temp = false;

  event_manager_->MousePressEvent(p, temp);

  if (!temp || call_parent_class) QGraphicsView::mousePressEvent(event);
}

void SceneManager::mouseReleaseEvent(QMouseEvent *event) {
  bool call_parent_class = false;
  const QPointF &p = mapToScene(event->pos());
  bool press_validated = false;

  event_manager_->MouseReleaseEvent(p, press_validated);

  if (!press_validated || call_parent_class)
    QGraphicsView::mouseReleaseEvent(event);
}

void SceneManager::mouseMoveEvent(QMouseEvent *event) {
  const QPointF &p = mapToScene(event->pos());
  bool press_validated = false;

  event_manager_->MouseMoveEvent(p);

  if (!press_validated) QGraphicsView::mouseMoveEvent(event);
}

void SceneManager::closeEvent(QCloseEvent *event) {
  should_shutdown_ = true;
#ifdef CATCHCHALLENGER_SOLO
  if (Globals::InternalServer != nullptr) {
    hide();
    if (connection_ != nullptr)
      if (connection_->client != nullptr)
        connection_->client->disconnectFromHost();
    // Globals::InternalServer->Stopped();
    event->accept();
    return;
  }
#endif
  QWidget::closeEvent(event);
  QCoreApplication::quit();
}

void SceneManager::Render() {
  draw_times_++;
  if (should_resize_) {
    graphic_scene_->setSceneRect(QRectF(0, 0, width(), height()));
    OnResize();

    should_resize_ = false;
  }
  viewport()->update();
}

void SceneManager::RenderActions() {
  action_manager_->Update();

  action_render_.start(1);
}

void SceneManager::paintEvent(QPaintEvent *event) {
  // graphic_scene_->setSceneRect(QRectF(0, 0, width(), height()));
  time_elapsed_.restart();

  QGraphicsView::paintEvent(event);

  uint32_t elapsed = time_elapsed_.elapsed();
  if (wait_render_time_ <= elapsed)
    timer_render_.start(1);
  else
    timer_render_.start(wait_render_time_ - elapsed);

  if (frame_counter_ < 65535) frame_counter_++;
}

void SceneManager::keyPressEvent(QKeyEvent *event) {
  bool event_trigger = false;
  event->setAccepted(false);
  event_manager_->KeyPressEvent(event, event_trigger);
  if (event_trigger) {
    QGraphicsView::keyPressEvent(event);
    return;
  }
  if (!event->isAccepted()) QGraphicsView::keyPressEvent(event);
}

void SceneManager::keyReleaseEvent(QKeyEvent *event) {
  bool event_trigger = false;
  event->setAccepted(false);
  event_manager_->KeyReleaseEvent(event, event_trigger);
  if (event_trigger) {
    QGraphicsView::keyPressEvent(event);
    return;
  }
  if (!event->isAccepted()) QGraphicsView::keyReleaseEvent(event);
}

void SceneManager::PushScene(Scene *scene) {
  if (!scene_stack_.empty()) {
    if (scene_stack_.back() == scene) {
      return;
    }
    Scene *tmp = scene_stack_.back();
    tmp->OnExit();
    graphic_scene_->removeItem(tmp);
  }
  scene_stack_.push_back(scene);
  // renderer_->SetScene(scene);
  graphic_scene_->addItem(scene);
  scene->OnEnter();
}

Scene *SceneManager::PopScene() {
  Scene *tmp = scene_stack_.back();
  tmp->OnExit();
  graphic_scene_->removeItem(tmp);
  scene_stack_.pop_back();
  auto scene = scene_stack_.back();
  // renderer_->SetScene(scene);
  graphic_scene_->addItem(scene);
  scene->OnEnter();
  return tmp;
}

void SceneManager::UpdateFPS() {
  draw_times_ = 0;
  const unsigned int FPS =
      (int)(((float)frame_counter_) * 1000) / timeUpdateFPS.elapsed();
  QImage pix(50, 40, QImage::Format_ARGB32_Premultiplied);
  pix.fill(Qt::transparent);
  QPainter p(&pix);
  p.setFont(QFont("Times", 12, QFont::Bold));

  const int offset = 1;
  p.setPen(QPen(Qt::black));
  p.drawText(pix.rect().x() + offset, pix.rect().y() + offset,
             pix.rect().width(), pix.rect().height(), Qt::AlignCenter,
             QString::number(FPS));
  p.drawText(pix.rect().x() - offset, pix.rect().y() + offset,
             pix.rect().width(), pix.rect().height(), Qt::AlignCenter,
             QString::number(FPS));
  p.drawText(pix.rect().x() - offset, pix.rect().y() - offset,
             pix.rect().width(), pix.rect().height(), Qt::AlignCenter,
             QString::number(FPS));
  p.drawText(pix.rect().x() + offset, pix.rect().y() - offset,
             pix.rect().width(), pix.rect().height(), Qt::AlignCenter,
             QString::number(FPS));

  p.setPen(QPen(Qt::white));
  p.drawText(pix.rect(), Qt::AlignCenter, QString::number(FPS));
  imageText->setPixmap(QPixmap::fromImage(pix));

  frame_counter_ = 0;
  timeUpdateFPS.restart();
  timerUpdateFPS.start();
}

void SceneManager::SetTargetFPS(int targetFPS) {
  std::cout<< "LAN_[" << __FILE__ << ":" << __LINE__ << "] "<< targetFPS << std::endl;
  if (targetFPS == 0) {
    wait_render_time_ = 0;
  } else {
    wait_render_time_ = static_cast<uint8_t>(static_cast<float>(1000.0) /
                                             static_cast<float>(targetFPS));
    if (wait_render_time_ < 1) wait_render_time_ = 1;
  }
}

void SceneManager::OnError(std::string error) {
  Globals::GetAlertDialog()->Show(QString::fromStdString(error));
}

void SceneManager::OnDisconnect() {
  Restart();
#ifdef CATCHCHALLENGER_SOLO
  // if (Globals::InternalServer != nullptr) Globals::InternalServer->Stopped();
#endif
}

void SceneManager::ShowOverlay(Node *overlay) {
  auto current = scene_stack_.back();
  if (overlay_ && overlay_ != overlay) {
    current->RemoveChild(overlay_);
  }
  overlay_ = overlay;
  current->AddChild(overlay_);
}

void SceneManager::RemoveOverlay() {
  if (overlay_) {
    overlay_->Parent()->RemoveChild(overlay_);
  }
  overlay_ = nullptr;
}

Scene *SceneManager::CurrentScene() { return scene_stack_.back(); }

QGraphicsItem *SceneManager::GetRenderer() { return renderer_; }

void SceneManager::Restart() {
  auto current = scene_stack_.back();
  graphic_scene_->removeItem(current);
  scene_stack_.clear();
  EventManager::GetInstance()->RemoveAllListeners();
  ActionManager::GetInstance()->RemoveAllActions();
  Globals::ClearAllScenes();
  PushScene(Globals::GetLeadingScene());
}

#ifdef CATCHCHALLENGER_SOLO
void SceneManager::RegisterInternalServerEvents() {
  connect(Globals::InternalServer,
    &CatchChallenger::InternalServer::is_started,
                   this, &SceneManager::IsStarted);
  connect(Globals::InternalServer,
                   &CatchChallenger::InternalServer::error,
                   this, &SceneManager::OnError);
}

void SceneManager::IsStarted(bool started) {
  if (!started) {
    if (Globals::InternalServer != nullptr) {
      delete Globals::InternalServer;
      Globals::InternalServer = nullptr;
    }
    QCoreApplication::quit();
  }
}
#endif
