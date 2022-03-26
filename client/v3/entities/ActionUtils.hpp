// Copyright 2021 CatchChallenger
#ifndef CLIENT_V3_ENTITIES_ACTIONUTILS_HPP_
#define CLIENT_V3_ENTITIES_ACTIONUTILS_HPP_

#include "../action/Sequence.hpp"

#include <functional>

class ActionUtils {
 public:
  static Sequence* WaitAndThen(const int &ms, std::function<void()> callback);
};
#endif  // CLIENT_V3_ENTITIES_ACTIONUTILS_HPP_
