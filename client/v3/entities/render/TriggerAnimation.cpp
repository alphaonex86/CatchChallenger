// Copyright 2021 CatchChallenger
#include "TriggerAnimation.hpp"

#include <QApplication>

TriggerAnimation::TriggerAnimation(Tiled::MapObject *object,
                                   const uint8_t &framesCountEnter,
                                   const uint16_t &msEnter,
                                   const uint8_t &framesCountLeave,
                                   const uint16_t &msLeave,
                                   const uint8_t &framesCountAgain,
                                   const uint16_t &msAgain, bool over)
    : object(object),
      objectOver(NULL),
      cell(object->cell()),
      baseTile(object->cell().tile),
      baseTileOver(NULL),
      framesCountEnter(framesCountEnter),
      msEnter(msEnter),
      framesCountLeave(framesCountLeave),
      msLeave(msLeave),
      framesCountAgain(framesCountAgain),
      msAgain(msAgain),
      over(over),
      playerOnThisTile(0),
      firstTime(true) {
  moveToThread(QApplication::instance()->thread());
}

TriggerAnimation::TriggerAnimation(Tiled::MapObject *object,
                                   const uint8_t &framesCountEnter,
                                   const uint16_t &msEnter,
                                   const uint8_t &framesCountLeave,
                                   const uint16_t &msLeave, bool over)
    : object(object),
      objectOver(NULL),
      cell(object->cell()),
      baseTile(object->cell().tile),
      baseTileOver(NULL),
      framesCountEnter(framesCountEnter),
      msEnter(msEnter),
      framesCountLeave(framesCountLeave),
      msLeave(msLeave),
      framesCountAgain(0),
      msAgain(0),
      over(over),
      playerOnThisTile(0),
      firstTime(true) {
  moveToThread(QApplication::instance()->thread());
}

TriggerAnimation::TriggerAnimation(
    Tiled::MapObject *object, Tiled::MapObject *objectOver,
    const uint8_t &framesCountEnter, const uint16_t &msEnter,
    const uint8_t &framesCountLeave, const uint16_t &msLeave,
    const uint8_t &framesCountAgain, const uint16_t &msAgain, bool over)
    : object(object),
      objectOver(objectOver),
      cell(object->cell()),
      baseTile(object->cell().tile),
      framesCountEnter(framesCountEnter),
      msEnter(msEnter),
      framesCountLeave(framesCountLeave),
      msLeave(msLeave),
      framesCountAgain(framesCountAgain),
      msAgain(msAgain),
      over(over),
      playerOnThisTile(0),
      firstTime(true) {
  moveToThread(QApplication::instance()->thread());
  if (objectOver != NULL) {
    baseTileOver = objectOver->cell().tile;
    cellOver = objectOver->cell();
  }
}

TriggerAnimation::TriggerAnimation(Tiled::MapObject *object,
                                   Tiled::MapObject *objectOver,
                                   const uint8_t &framesCountEnter,
                                   const uint16_t &msEnter,
                                   const uint8_t &framesCountLeave,
                                   const uint16_t &msLeave, bool over)
    : object(object),
      objectOver(objectOver),
      cell(object->cell()),
      baseTile(object->cell().tile),
      framesCountEnter(framesCountEnter),
      msEnter(msEnter),
      framesCountLeave(framesCountLeave),
      msLeave(msLeave),
      framesCountAgain(0),
      msAgain(0),
      over(over),
      playerOnThisTile(0),
      firstTime(true) {
  moveToThread(QApplication::instance()->thread());
  if (objectOver != NULL) {
    baseTileOver = objectOver->cell().tile;
    cellOver = objectOver->cell();
  }
}

TriggerAnimation::~TriggerAnimation() {}

void TriggerAnimation::startEnter() {
  if (framesCountAgain == 0 || firstTime) {
    firstTime = false;
    EnterEventCall eventCall;
    eventCall.timer = new QTimer(this);
    eventCall.timer->start(msEnter);
    eventCall.frame = 0;
    if (!connect(eventCall.timer, &QTimer::timeout, this,
                 &TriggerAnimation::timerFinishEnter, Qt::QueuedConnection))
      abort();
    enterEvents << eventCall;
  } else {
    firstTime = false;
    EnterEventCall eventCall;
    eventCall.timer = new QTimer(this);
    eventCall.timer->start(msAgain);
    eventCall.frame = framesCountEnter + framesCountLeave - 1;
    if (!connect(eventCall.timer, &QTimer::timeout, this,
                 &TriggerAnimation::timerFinishEnter, Qt::QueuedConnection))
      abort();
    enterEvents << eventCall;
  }
}

void TriggerAnimation::startLeave() {
  playerOnThisTile--;
  LeaveEventCall eventCall;
  eventCall.timer = new QTimer(this);
  eventCall.timer->start(msLeave);
  eventCall.frame = framesCountEnter - 1;
  if (!connect(eventCall.timer, &QTimer::timeout, this,
               &TriggerAnimation::timerFinishLeave, Qt::QueuedConnection))
    abort();
  leaveEvents << eventCall;
}

void TriggerAnimation::setTileOffset(const uint8_t &offset) {
  cell.tile = baseTile->tileset()->tileAt(baseTile->id() + offset);
  object->setCell(cell);
  if (objectOver != NULL) {
    cellOver.tile =
        baseTileOver->tileset()->tileAt(baseTileOver->id() + offset);
    objectOver->setCell(cellOver);
  }
}

void TriggerAnimation::timerFinishEnter() {
  QTimer *timer = qobject_cast<QTimer *>(sender());
  uint8_t lowerFrame =
      framesCountEnter + framesCountLeave + framesCountAgain - 1;
  int index = 0;
  while (index < enterEvents.size()) {
    const EnterEventCall &eventCall = enterEvents.at(index);
    if (eventCall.timer == timer) enterEvents[index].frame++;
    if (eventCall.frame == (framesCountEnter - 1) ||
        (eventCall.frame >=
             (framesCountEnter + framesCountLeave + framesCountAgain) &&
         framesCountAgain > 0)) {
      eventCall.timer->stop();
      delete eventCall.timer;
      enterEvents.removeAt(index);
      index--;
      playerOnThisTile++;
    } else {
      if (eventCall.frame < lowerFrame) lowerFrame = eventCall.frame;
    }
    index++;
  }
  // set if one player is on it
  if (playerOnThisTile > 0) {
    if ((framesCountEnter - 1) < lowerFrame) lowerFrame = framesCountEnter - 1;
  } else {
    // no body on it and no event
    if (enterEvents.isEmpty() && leaveEvents.isEmpty()) {
      if (framesCountAgain == 0)
        lowerFrame = 0;
      else
        lowerFrame = framesCountEnter - 1;
    }
  }
  setTileOffset(lowerFrame);
}

void TriggerAnimation::timerFinishLeave() {
  QTimer *timer = qobject_cast<QTimer *>(sender());
  uint8_t lowerFrame =
      framesCountEnter + framesCountLeave + framesCountAgain - 1;
  int index = 0;
  while (index < leaveEvents.size()) {
    const LeaveEventCall &eventCall = leaveEvents.at(index);
    if (eventCall.timer == timer) leaveEvents[index].frame++;
    if (eventCall.frame >= (framesCountEnter + framesCountLeave - 1)) {
      eventCall.timer->stop();
      delete eventCall.timer;
      leaveEvents.removeAt(index);
      index--;
    } else {
      if (eventCall.frame < lowerFrame) lowerFrame = eventCall.frame;
    }
    index++;
  }
  index = 0;
  while (index < enterEvents.size()) {
    const EnterEventCall &eventCall = enterEvents.at(index);
    if (eventCall.frame < lowerFrame) lowerFrame = eventCall.frame;
    index++;
  }
  // set if one player is on it
  if (playerOnThisTile > 0) {
    if ((framesCountEnter - 1) < lowerFrame) lowerFrame = framesCountEnter - 1;
  } else {
    // no body on it and no event
    if (enterEvents.isEmpty() && leaveEvents.isEmpty()) {
      if (framesCountAgain == 0)
        lowerFrame = 0;
      else
        lowerFrame = framesCountEnter + framesCountLeave - 1;
    }
  }
  setTileOffset(lowerFrame);
}
