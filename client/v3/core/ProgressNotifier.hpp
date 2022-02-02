// Copyright 2021 CatchChallenger
#ifndef CLIENT_QTOPENGL_CORE_PROGRESSNOTIFIER_HPP_
#define CLIENT_QTOPENGL_CORE_PROGRESSNOTIFIER_HPP_

#include <QString>
#include <cstdint>
#include <functional>

class ProgressNotifier {
 public:
  void SetOnStatusChange(std::function<void(QString)> callback);
  void SetOnProgress(std::function<void(uint32_t, uint32_t)> callback);

 protected:
  void NotifyProgress(uint32_t size, uint32_t total);
  void NotifyStatus(QString status);

 private:
  std::function<void(uint32_t, uint32_t)> on_progress_;
  std::function<void(QString)> on_status_change_;
};
#endif  // CLIENT_QTOPENGL_CORE_PROGRESSNOTIFIER_HPP_
