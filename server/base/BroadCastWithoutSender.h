#ifndef POKECRAFT_BROADCASTWITHOUTSENDER_H
#define POKECRAFT_BROADCASTWITHOUTSENDER_H

#include <QObject>

namespace Pokecraft {
class BroadCastWithoutSender : public QObject
{
	Q_OBJECT
public:
	explicit BroadCastWithoutSender(QObject *parent = 0);
	
signals:
	
public slots:
	
};
}

#endif // BROADCASTWITHOUTSENDER_H
