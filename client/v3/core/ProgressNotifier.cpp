// Copyright 2021 CatchChallenger
#include "ProgressNotifier.hpp"

void ProgressNotifier::SetOnStatusChange(
    std::function<void(QString)> callback) {
  on_status_change_ = callback;
}

void ProgressNotifier::SetOnProgress(
    std::function<void(uint32_t, uint32_t)> callback) {
  on_progress_ = callback;
}

void ProgressNotifier::NotifyProgress(uint32_t size, uint32_t total) {
  if (on_progress_) on_progress_(size, total);
}

void ProgressNotifier::NotifyStatus(QString status) {
  if (on_status_change_) on_status_change_(status);
}
