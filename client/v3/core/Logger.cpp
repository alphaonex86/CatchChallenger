// Copyright 2021 CatchChallenger
#include "Logger.hpp"

#include <iostream>

void Logger::Log(const char *file, int line, const std::string &message) {
  std::cout << "[" << file << ":" << line << "]" << message << std::endl;
}

void Logger::Log(const char *file, int line, const QString &message) {
  Log(file, line, message.toStdString());
}

void Logger::Log(const QString &message) {
  std::cout << message.toStdString() << std::endl;
}

void Logger::Log(const char *message) {
  std::cout << message << std::endl;
}
