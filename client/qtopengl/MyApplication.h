#ifndef MYAPPLICATION_H
#define MYAPPLICATION_H

#include <QApplication>
#include <QEvent>

class MyApplication : public QApplication
{
public:
    MyApplication(int& argc, char** argv);
    bool notify(QObject* receiver, QEvent* event);
};

#endif // MYAPPLICATION_H
