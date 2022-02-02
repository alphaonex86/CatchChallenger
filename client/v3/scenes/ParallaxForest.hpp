// Copyright 2021 CatchChallenger
#ifndef CLIENT_QTOPENGL_SCENES_PARALLAXFOREST_HPP_
#define CLIENT_QTOPENGL_SCENES_PARALLAXFOREST_HPP_

#include <QGraphicsWidget>
#include <QPixmap>

#include "../action/RepeatForever.hpp"
#include "../core/Scene.hpp"

class ParallaxForest : public Scene {
 public:
  static ParallaxForest* Create();
  ~ParallaxForest();
  void StartAnimation();
  void StopAnimation();
  void Draw(QPainter* painter) override;
  QRectF boundingRect() const;

 protected:
  void OnScreenSD() override;
  void OnScreenHD() override;
  void OnScreenHDR() override;
  void OnScreenResize() override;

 private:
  ParallaxForest();
  void ScalePix(QPixmap& pix, unsigned int zoom);
  void GrassSlot();
  void TreebackSlot();
  void TreefrontSlot();
  void CloudSlot();
  unsigned int GetTargetZoom();

  QPixmap cloud_;
  QPixmap grass_;
  QPixmap sun_;
  QPixmap treeback_;
  QPixmap treefront_;
  unsigned int zoom_;
  int grassMove, treebackMove, treefrontMove, cloudMove;
  RepeatForever* grass_updater_;
  RepeatForever* treeback_updater_;
  RepeatForever* treefront_updater_;
  RepeatForever* cloud_updater_;
};

#endif  // CLIENT_QTOPENGL_SCENES_PARALLAXFOREST_HPP_
