#ifndef LATENCYCHECKER_H
#define LATENCYCHECKER_H

#include <QObject>
#include <QList>
#include <QTimer>
#include <QTime>
#include <QStringList>
#include <QMutex>
#include <QMutexLocker>

#include "../../pokecraft-general/base/DebugClass.h"
#include "EventThreader.h"

class LatencyChecker : public QObject
{
	Q_OBJECT
public:
	explicit LatencyChecker();
	~LatencyChecker();
	void setLatencyVariable(QList<EventThreader *> threads,QStringList names);
	QStringList getLatency();
	quint32 getTotalLatency();
private:
	QList<EventThreader *> threads;
	QStringList names;
	QList<QTimer *> timers;
	QList<QTime *> times;
	QList<QMutex *> mutexs;
	QList<quint32> latency;
	QList<bool> waitReply;
	quint32 total_latency;
private slots:
	void resolve_latency();
	void probe_latency();
};

#endif // LATENCYCHECKER_H
