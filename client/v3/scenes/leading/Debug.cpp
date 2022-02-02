// Copyright 2021 CatchChallenger
#include "Debug.hpp"

#include <QPainter>
#include <QPaintEngine>

#include "../../action/CallFunc.hpp"
#include "../../action/Delay.hpp"
#include "../../action/Sequence.hpp"
#include "../../core/SceneManager.hpp"

using Scenes::Debug;

Debug::Debug() : UI::Dialog() {
  debugIsShow = false;
  debugText = UI::Label::Create(this);
  updater_ = RepeatForever::Create(Sequence::Create(
      Delay::Create(1000), CallFunc::Create(std::bind(&Node::ReDraw, this)), nullptr));
  SetTitle("Debug");
}

Debug::~Debug() {
  delete debugText;
  debugText = nullptr;
}

Debug* Debug::Create() {
  return new (std::nothrow) Debug();
}

void Debug::OnEnter() { 
  UI::Dialog::OnEnter();
  RunAction(updater_); }

void Debug::OnExit() {
  updater_->Stop(); 
  UI::Dialog::OnExit();
}

void Debug::OnScreenResize() {
  UI::Dialog::OnScreenResize();
  auto content = ContentBoundary();
  debugText->SetPos(content.x() + 10, content.y() + 10);
}

void Debug::Draw(QPainter *painter) {
  UI::Dialog::Draw(painter);
  const QPaintEngine::Type type = painter->paintEngine()->type();
  QString OpenGL = "2D acceleration: CPU (" + QString::number(type) + ")";
  switch (type) {
    case QPaintEngine::X11:
      OpenGL = "2D acceleration: X11";
      break;
    case QPaintEngine::Windows:
      OpenGL = "2D acceleration: Windows";
      break;
    case QPaintEngine::QuickDraw:
      OpenGL = "2D acceleration: QuickDraw";
      break;
    case QPaintEngine::CoreGraphics:
      OpenGL = "2D acceleration: CoreGraphics";
      break;
    case QPaintEngine::MacPrinter:
      OpenGL = "2D acceleration: MacPrinter";
      break;
    case QPaintEngine::QWindowSystem:
      OpenGL = "2D acceleration: QWindowSystem";
      break;
    case QPaintEngine::OpenGL:
      OpenGL = "2D acceleration: OpenGL";
      break;
    case QPaintEngine::Picture:
      OpenGL = "2D acceleration: Picture";
      break;
    case QPaintEngine::SVG:
      OpenGL = "2D acceleration: SVG";
      break;
    case QPaintEngine::Raster:
      OpenGL = "2D acceleration: Raster";
      break;
    case QPaintEngine::Direct3D:
      OpenGL = "2D acceleration: Direct3D";
      break;
    case QPaintEngine::Pdf:
      OpenGL = "2D acceleration: Pdf";
      break;
    case QPaintEngine::OpenVG:
      OpenGL = "2D acceleration: OpenVG";
      break;
    case QPaintEngine::OpenGL2:
      OpenGL = "2D acceleration: OpenGL2";
      break;
    case QPaintEngine::PaintBuffer:
      OpenGL = "2D acceleration: PaintBuffer";
      break;
    case QPaintEngine::Blitter:
      OpenGL = "2D acceleration: Blitter";
      break;
    case QPaintEngine::Direct2D:
      OpenGL = "2D acceleration: Direct2D";
      break;
    default:
      break;
  }
  debugIsShow = true;
  auto widget = SceneManager::GetInstance();
  debugText->SetText(
      QString("logicalDpiX: ") + QString::number(widget->logicalDpiX()) + ", " +
      "logicalDpiY: " + QString::number(widget->logicalDpiY()) + "\n" +
      "physicalDpiX: " + QString::number(widget->physicalDpiX()) + ", " +
      "physicalDpiY: " + QString::number(widget->physicalDpiY()) + "\n" +
      "width: " + QString::number(widget->width()) + ", " +
      "height: " + QString::number(widget->height()) + "\n" + "thread: " +
      QString::number(QThread::idealThreadCount()) + "\n" + OpenGL);
}
