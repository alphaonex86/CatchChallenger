#include "ProcessControler.h"

ProcessControler::ProcessControler()
{
	qRegisterMetaType<Chat_type>("Chat_type");
	connect(&eventDispatcher,SIGNAL(is_started(bool)),this,SLOT(server_is_started(bool)));
	connect(&eventDispatcher,SIGNAL(need_be_stopped()),this,SLOT(server_need_be_stopped()));
	connect(&eventDispatcher,SIGNAL(need_be_restarted()),this,SLOT(server_need_be_restarted()));
	connect(&eventDispatcher,SIGNAL(benchmark_result(int,double,double,double,double,double)),this,SLOT(benchmark_result(int,double,double,double,double,double)));
	need_be_restarted=false;
	need_be_closed=false;
	eventDispatcher.start_server();
}

ProcessControler::~ProcessControler()
{
}

void ProcessControler::server_is_started(bool is_started)
{
	if(need_be_closed)
	{
		QCoreApplication::exit();
		return;
	}
	if(!is_started)
	{
		if(need_be_restarted)
		{
			need_be_restarted=false;
			eventDispatcher.start_server();
		}
	}
}

void ProcessControler::server_need_be_stopped()
{
	eventDispatcher.stop_server();
}

void ProcessControler::server_need_be_restarted()
{
	need_be_restarted=true;
	eventDispatcher.stop_server();
}


QString ProcessControler::sizeToString(double size)
{
	if(size<1024)
		return QString::number(size)+tr("B");
	if((size=size/1024)<1024)
		return adaptString(size)+tr("KB");
	if((size=size/1024)<1024)
		return adaptString(size)+tr("MB");
	if((size=size/1024)<1024)
		return adaptString(size)+tr("GB");
	if((size=size/1024)<1024)
		return adaptString(size)+tr("TB");
	if((size=size/1024)<1024)
		return adaptString(size)+tr("PB");
	if((size=size/1024)<1024)
		return adaptString(size)+tr("EB");
	if((size=size/1024)<1024)
		return adaptString(size)+tr("ZB");
	if((size=size/1024)<1024)
		return adaptString(size)+tr("YB");
	return tr("Too big");
}

QString ProcessControler::adaptString(float size)
{
	if(size>=100)
		return QString::number(size,'f',0);
	else
		return QString::number(size,'g',3);
}


void ProcessControler::benchmark_result(int latency,double TX_speed,double RX_speed,double TX_size,double RX_size,double second)
{
	DebugClass::debugConsole(QString("The latency of the benchmark: %1\nTX_speed: %2/s, RX_speed %3/s\nTX_size: %4, RX_size: %5, in %6s\nThis latency is cumulated latency of different point. That's not show the database performance.")
				 .arg(latency)
				 .arg(sizeToString(TX_speed))
				 .arg(sizeToString(RX_speed))
				 .arg(sizeToString(TX_size))
				 .arg(sizeToString(RX_size))
				 .arg(second)
				 );
}
