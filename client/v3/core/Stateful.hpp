// Copyright 2021 CatchChallenger
#ifndef CLIENT_QTOPENGL_CORE_STATEFUL_HPP_
#define CLIENT_QTOPENGL_CORE_STATEFUL_HPP_

#include <unordered_map>

class Stateful {
 public:
  ~Stateful();
  void StoreState(int state);
  void UseState(int state);

 protected:
  virtual void* StoreInternal() = 0;
  virtual void UseInternal(void* data) = 0;

 private:
  std::unordered_map<int, void*> states_;
};
#endif  // CLIENT_QTOPENGL_CORE_STATEFUL_HPP_
