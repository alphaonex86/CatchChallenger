// Copyright 2021 CatchChallenger
#include "ParallaxForest.hpp"

#include <QDebug>
#include <QPainter>
#include <iostream>

#include "../action/CallFunc.hpp"
#include "../action/Delay.hpp"
#include "../action/RepeatForever.hpp"
#include "../action/Sequence.hpp"

ParallaxForest::ParallaxForest() {
  zoom_ = 0;
  grassMove = 0;
  treebackMove = 0;
  treefrontMove = 0;
  cloudMove = 0;

  grass_updater_ = nullptr;
  treeback_updater_ = nullptr;
  treefront_updater_ = nullptr;
  cloud_updater_ = nullptr;

  StartAnimation();
  setCacheMode(QGraphicsItem::NoCache);
}

ParallaxForest::~ParallaxForest() {
  delete grass_updater_;
  delete treeback_updater_;
  delete treefront_updater_;
  delete cloud_updater_;
}

ParallaxForest* ParallaxForest::Create() {
  ParallaxForest* instance = new (std::nothrow) ParallaxForest();
  return instance;
}

void ParallaxForest::StartAnimation() {
  unsigned int base_time = 20;
  if (grass_updater_ == nullptr) {
    grass_updater_ = RepeatForever::Create(Sequence::Create(
        Delay::Create(base_time),
        CallFunc::Create(std::bind(&ParallaxForest::GrassSlot, this)),
        nullptr));
    treefront_updater_ = RepeatForever::Create(Sequence::Create(
        Delay::Create(base_time * 3),
        CallFunc::Create(std::bind(&ParallaxForest::TreefrontSlot, this)),
        nullptr));
    treeback_updater_ = RepeatForever::Create(Sequence::Create(
        Delay::Create(base_time * 9),
        CallFunc::Create(std::bind(&ParallaxForest::TreebackSlot, this)),
        nullptr));
    cloud_updater_ = RepeatForever::Create(Sequence::Create(
        Delay::Create(base_time * 30),
        CallFunc::Create(std::bind(&ParallaxForest::CloudSlot, this)),
        nullptr));
  }
  RunAction(grass_updater_);
  RunAction(treeback_updater_);
  RunAction(treefront_updater_);
  RunAction(cloud_updater_);
}

void ParallaxForest::StopAnimation() {
  grass_updater_->Stop();
  treeback_updater_->Stop();
  treefront_updater_->Stop();
  cloud_updater_->Stop();
}

void ParallaxForest::ScalePix(QPixmap& pix, unsigned int zoom) {
  pix = pix.scaled(pix.width() * zoom, pix.height() * zoom);
}

void ParallaxForest::GrassSlot() {
  if (grass_.isNull()) return;
  grassMove++;
}

void ParallaxForest::TreebackSlot() {
  if (treeback_.isNull()) return;
  treebackMove++;
}

void ParallaxForest::TreefrontSlot() {
  if (treefront_.isNull()) return;
  treefrontMove++;
}

void ParallaxForest::CloudSlot() {
  if (cloud_.isNull()) return;
  cloudMove++;
}

unsigned int ParallaxForest::GetTargetZoom() {
  unsigned int height = BoundingRect().height() / 120;
  if (height < 1) height = 1;
  if (height > 8) height = 8;
  unsigned int width = BoundingRect().width() / 120;
  if (width < 1) width = 1;
  if (width > 8) width = 8;
  unsigned int target_zoom = height;
  if (width < height) target_zoom = width;
  return target_zoom;
}

void ParallaxForest::Draw(QPainter* painter) {
  treefrontMove %= treefront_.width() * zoom_;
  cloudMove %= cloud_.width() * zoom_;
  treebackMove %= treeback_.width() * zoom_;
  grassMove %= grass_.width() * zoom_;

  int grassOffset = 0;
  int skyOffset = 0;
  int sunOffset = 0;
  unsigned int endOfGrass = 0;
  if (BoundingRect().height() <= 120 * zoom_) {
    endOfGrass = BoundingRect().height();
  } else {
    skyOffset = (BoundingRect().height() - 120 * zoom_) * 2 / 3;
    if (skyOffset > BoundingRect().height() / 4)
      skyOffset = BoundingRect().height() / 4;
    endOfGrass = 120 * zoom_ + (BoundingRect().height() - 120 * zoom_) * 2 / 3;
    painter->fillRect(0, endOfGrass, BoundingRect().width(),
                      BoundingRect().height() - endOfGrass,
                      QColor(131, 203, 83));
    painter->fillRect(0, 0, BoundingRect().width(), skyOffset,
                      QColor(115, 225, 255));
  }
  sunOffset = endOfGrass / 6;

  QRect gradientRect(0, skyOffset, BoundingRect().width(), 120 * zoom_);
  QLinearGradient gradient(
      gradientRect.topLeft(),
      gradientRect
          .bottomLeft());  // diagonal gradient from top-left to bottom-right
  gradient.setColorAt(0, QColor(115, 225, 255));
  gradient.setColorAt(1, QColor(183, 241, 255));
  painter->fillRect(0, skyOffset, BoundingRect().width(),
                    endOfGrass - skyOffset, gradient);

  grassOffset = endOfGrass - grass_.height();
  unsigned int treeFrontOffset = grassOffset + 4 * zoom_ - treefront_.height();
  unsigned int treebackOffset = grassOffset + 4 * zoom_ - treeback_.height();

  int lastTreebackX = -treebackMove;
  while (BoundingRect().width() > lastTreebackX) {
    painter->drawPixmap(lastTreebackX, treebackOffset, treeback_.width(),
                        treeback_.height(), treeback_);
    lastTreebackX += treeback_.width();
  }
  int lastTreefrontX = -treefrontMove;
  while (BoundingRect().width() > lastTreefrontX) {
    painter->drawPixmap(lastTreefrontX, treeFrontOffset, treefront_.width(),
                        treefront_.height(), treefront_);
    lastTreefrontX += treefront_.width();
  }
  int lastGrassX = -grassMove;
  while (BoundingRect().width() > lastGrassX) {
    painter->drawPixmap(lastGrassX, grassOffset, grass_.width(),
                        grass_.height(), grass_);
    lastGrassX += grass_.width();
  }

  painter->drawPixmap(BoundingRect().width() * 2 / 3 - sun_.width() / 2,
                      sunOffset, sun_.width(), sun_.height(), sun_);

  int lastcloudX = -cloudMove;
  while (BoundingRect().width() > lastcloudX) {
    painter->drawPixmap(lastcloudX, skyOffset + (endOfGrass - skyOffset) / 4,
                        cloud_.width(), cloud_.height(), cloud_);
    lastcloudX += cloud_.width();
  }
}

void ParallaxForest::OnScreenSD() {}

void ParallaxForest::OnScreenHD() {}

void ParallaxForest::OnScreenHDR() {}

void ParallaxForest::OnScreenResize() {
  unsigned int target_zoom = GetTargetZoom();
  if (zoom_ != target_zoom) {
    cloud_ = QPixmap(":/CC/images/animatedbackground/cloud.png");
    if (cloud_.isNull()) abort();
    grass_ = QPixmap(":/CC/images/animatedbackground/grass.png");
    if (grass_.isNull()) abort();
    sun_ = QPixmap(":/CC/images/animatedbackground/sun.png");
    if (sun_.isNull()) abort();
    treeback_ = QPixmap(":/CC/images/animatedbackground/treeback.png");
    if (treeback_.isNull()) abort();
    treefront_ = QPixmap(":/CC/images/animatedbackground/treefront.png");
    if (treefront_.isNull()) abort();
    if (target_zoom > 1) {
      ScalePix(cloud_, target_zoom);
      ScalePix(grass_, target_zoom);
      ScalePix(sun_, target_zoom);
      ScalePix(treeback_, target_zoom);
      ScalePix(treefront_, target_zoom);
    }
    grassMove = rand() % grass_.width() * target_zoom;
    treebackMove = rand() % treeback_.width() * target_zoom;
    treefrontMove = rand() % treefront_.width() * target_zoom;
    cloudMove = rand() % cloud_.width();
    zoom_ = target_zoom;
  }
}

QRectF ParallaxForest::boundingRect() const {
  return QRectF();
}
