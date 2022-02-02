// Copyright 2021 CatchChallenger
#ifndef CLIENT_V3_ULTIMATE_HPP_
#define CLIENT_V3_ULTIMATE_HPP_

#include <string>

class Ultimate {
 public:
  Ultimate();
  bool setKey(const std::string &key);
  bool isUltimate() const;

  static Ultimate ultimate;

 private:
  bool m_ultimate;
};

#endif  // CLIENT_V3_ULTIMATE_HPP_
