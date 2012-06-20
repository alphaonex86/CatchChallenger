#ifndef BROADCASTWITHOUTSENDER_H
#define BROADCASTWITHOUTSENDER_H

#include <QObject>

class BroadCastWithoutSender : public QObject
{
	Q_OBJECT
public:
	explicit BroadCastWithoutSender(QObject *parent = 0);
	
signals:
	
public slots:
	
};

#endif // BROADCASTWITHOUTSENDER_H
