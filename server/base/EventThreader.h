#ifndef POKECRAFT_EVENTTHREADER_H
#define POKECRAFT_EVENTTHREADER_H

#include <QThread>

namespace Pokecraft {
class EventThreader : public QThread
{
	Q_OBJECT
public:
	explicit EventThreader();
	~EventThreader();
private:
	void run();
public slots:
	void probe_latency();
signals:
	void return_latency();
};
}

#endif // EVENTTHREADER_H
