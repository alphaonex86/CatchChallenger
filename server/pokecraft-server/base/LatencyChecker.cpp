#include "LatencyChecker.h"

LatencyChecker::LatencyChecker()
{
}

LatencyChecker::~LatencyChecker()
{
/*	while(timers.size()>0)
	{
		delete timers.first();
		timers.removeFirst();
	}
	while(times.size()>0)
	{
		delete times.first();
		times.removeFirst();
	}
	while(mutexs.size()>0)
	{
		delete mutexs.first();
		mutexs.removeFirst();
	}*/
}

void LatencyChecker::setLatencyVariable(QList<EventThreader *> threads,QStringList names)
{
/*	if(threads.size()!=names.size())
	{
		DebugClass::debugConsole("Both variable list size not match");
		return;
	}
	this->threads=threads;
	this->names=names;
	int index=0;
	while(index<threads.size())
	{
		timers << new QTimer();
		mutexs << new QMutex();
		times << new QTime();
		latency << 0;
		waitReply << false;
		timers.last()->setSingleShot(true);
		timers.last()->start(100);
		times.last()->start();
		connect(timers.last(),SIGNAL(timeout()),threads.at(index),SLOT(probe_latency()));
		connect(timers.last(),SIGNAL(timeout()),this,SLOT(probe_latency()));
		connect(threads.at(index),SIGNAL(return_latency()),this,SLOT(resolve_latency()));
		index++;
	}*/
}

void LatencyChecker::resolve_latency()
{
/*	EventThreader *thread=qobject_cast<EventThreader *>(QObject::sender());
	int index=0;
	while(index<threads.size())
	{
		if(threads.at(index)==thread)
		{
			{
				QMutexLocker lock(mutexs.at(index));
				latency[index]=times.at(index)->elapsed();
			}
			waitReply[index]=false;
			timers.at(index)->start();
			return;
		}
		index++;
	}*/
}

void LatencyChecker::probe_latency()
{
/*	QTimer *timer=qobject_cast<QTimer *>(QObject::sender());
	int index=0;
	while(index<timers.size())
	{
		if(timers.at(index)==timer)
		{
			{
				QMutexLocker lock(mutexs.at(index));
				times.at(index)->restart();
			}
			waitReply[index]=true;
			return;
		}
		index++;
	}
	*/
}

QStringList LatencyChecker::getLatency()
{
	return QStringList();
	/*QStringList latency_list;
	total_latency=0;
	int index=0;
	while(index<latency.size())
	{
		if(waitReply[index])
		{
			{
				QMutexLocker lock(mutexs.at(index));
				latency[index]=times.at(index)->elapsed();
			}
			total_latency+=latency.at(index);
			latency_list << QString("%1: %2ms").arg(names.at(index)).arg(latency.at(index));
		}
		else
		{
			total_latency+=latency.at(index);
			latency_list << QString("%1: %2ms").arg(names.at(index)).arg(latency.at(index));
		}
		index++;
	}
	return latency_list;*/
}

quint32 LatencyChecker::getTotalLatency()
{
	return 0;
	//return total_latency;
}
