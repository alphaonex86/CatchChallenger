#ifndef QTTIMEREVENTS_H
#define QTTIMEREVENTS_H

#include <QTimer>

class QtTimerEvents : public QObject
{
    Q_OBJECT
public:
    QtTimerEvents(const int &offset,const int &timer,const unsigned char &event,const unsigned char &value);
private:
    const unsigned char event;
    const unsigned char value;
    QTimer offset;
    QTimer timer;
private:
    void offsetFinish();
    void timerFinish();
};

#endif // TIMERDDOS_H
