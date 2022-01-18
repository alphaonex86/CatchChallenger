#ifndef CATCHCHALLENGER_QtTimeRangeEventScan_H
#define CATCHCHALLENGER_QtTimeRangeEventScan_H

#include <QTimer>
#include "../base/TimeRangeEventScanBase.hpp"

#include <stdint.h>

class QtTimeRangeEventScan : public QTimer, public CatchChallenger::TimeRangeEventScanBase
{
    Q_OBJECT
public:
    explicit QtTimeRangeEventScan();
signals:
    void try_initAll() const;
    void timeRangeEventTrigger();
private:
    void exec();
    void initAll();
private:
    QTimer *next_send_timer;
};

#endif // PLAYERUPDATER_H
