// Copyright 2021 CatchChallenger
#ifndef CLIENT_QTOPENGL_SCENES_LEADING_SOLO_HPP_
#define CLIENT_QTOPENGL_SCENES_LEADING_SOLO_HPP_

#include "../../base/ConnectionInfo.hpp"
#include "../../core/Scene.hpp"
#include "SubServer.hpp"

namespace Scenes {
class Solo : public Scene {
 public:
  static Solo *Create();
  ~Solo();

 private:
  SubServer *subserver_;
  Solo();
  void Initialize();
  void IsStarted(bool started);
  void ConnectToServer(ConnectionInfo connexionInfo, QString login,
                       QString pass);

 protected:
  void OnScreenResize() override;
};
}  // namespace Scenes
#endif  // CLIENT_QTOPENGL_SCENES_LEADING_SOLO_HPP_
