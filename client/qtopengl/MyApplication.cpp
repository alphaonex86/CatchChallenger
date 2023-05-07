#include "MyApplication.h"
#include <exception>
#include <iostream>

MyApplication::MyApplication(int& argc, char** argv) :
  QApplication(argc, argv) {}

bool MyApplication::notify(QObject* receiver, QEvent* event)
{
  bool done = true;
  try {
    done = QApplication::notify(receiver, event);
  } catch (const std::exception& ex) {
    std::cerr << "exception " << ex.what() << std::endl;
  } catch (...) {
    std::cerr << "exception ..." << std::endl;
  }
  return done;
}
