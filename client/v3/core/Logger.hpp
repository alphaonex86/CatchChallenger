// Copyright 2021 CatchChallenger
#ifndef CLIENT_V3_CORE_LOGGER_HPP_
#define CLIENT_V3_CORE_LOGGER_HPP_

#include <QString>
#include <string>

class Logger {
 public:
  static void Log(const char *file, int line, const QString &message);
  static void Log(const char *file, int line, const std::string &message);
  static void Log(const QString &message);
  static void Log(const char *message);
};
#endif  // CLIENT_V3_CORE_LOGGER_HPP_
