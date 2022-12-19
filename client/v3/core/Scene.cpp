// Copyright 2021 CatchChallenger
#include "Scene.hpp"

#include <QPainter>
#include <QWidget>
#include <cstdarg>
#include <iostream>
#include <string>

#include "EventManager.hpp"
#include "Node.hpp"
#include "../Globals.hpp"
#include "SceneManager.hpp"

Scene::Scene(Node *parent) : Node(parent) {
  class_name_ = __func__;

  int width = SceneManager::GetInstance()->width();
  int height = SceneManager::GetInstance()->height();
  bounding_rect_ = QRectF(0, 0, width, height);
}

Scene::~Scene() {}

void Scene::paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
                  QWidget *widget) {
  Node::paint(painter, option, widget);
}

void Scene::RegisterMouseListener(Node *node) {
  EventManager::GetInstance()->AddMouseListener(node);
}

void Scene::RegisterKeyboardListerner(Node *node) {
  EventManager::GetInstance()->AddKeyboardListener(node);
}

void Scene::Draw(QPainter *painter) { (void)painter; }

void Scene::MousePressEvent(const QPointF &p, bool &press) {
  (void)p;
  (void)press;
}

void Scene::MouseReleaseEvent(const QPointF &p, bool &press) {
  (void)p;
  (void)press;
}

void Scene::MouseMoveEvent(const QPointF &p) { (void)p; }

void Scene::KeyPressEvent(QKeyEvent *event, bool &event_trigger) {
  (void)event;
  (void)event_trigger;
}

void Scene::KeyReleaseEvent(QKeyEvent *event, bool &event_trigger) {
  (void)event;
  (void)event_trigger;
}

void Scene::PrintError(const char *file, int line, const QString &message) {
  std::cout << "[" << file << ":" << line << "]" << message.toStdString()
            << std::endl;
}

void Scene::PrintError(const QString &message) {
  Globals::GetAlertDialog()->Show(message);
}

void Scene::PrintError(const std::string &message) {
  PrintError(QString::fromStdString(message));
}

void Scene::OnScreenSD() {}

void Scene::OnScreenHD() {}

void Scene::OnScreenHDR() {}

void Scene::OnScreenResize() {}

QRectF Scene::boundingRect() const {
  return QRectF();
}

void Scene::OnResize() {
  int width = SceneManager::GetInstance()->width();
  int height = SceneManager::GetInstance()->height();
  bounding_rect_ = QRectF(0, 0, width, height);
  OnScreenResize();
  if (width < 512) {
    OnScreenSD();
  } else if (width < 1024) {
    OnScreenHD();
  } else {
    OnScreenHDR();
  }
}

void Scene::OnEnter() {
  OnResize();
  //Node::OnEnter();
}

void Scene::OnExit() {
  //Node::OnExit();
}
