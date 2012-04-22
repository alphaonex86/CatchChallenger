#ifndef MAP_LOADER_H
#define MAP_LOADER_H

#include <QObject>
#include <QStringList>
#include <QString>
#include <QDomDocument>
#include <QDomElement>
#include <QFile>
#include <QCoreApplication>
#include <QVariant>
#include <QMutex>
#include <QTime>
#include <QMutexLocker>

#include "../pokecraft-general/DebugClass.h"
#include "../VariableServer.h"
#include "ServerStructures.h"

class Map_loader : public QObject
{
	Q_OBJECT
public:
	explicit Map_loader();
	~Map_loader();

	Map_final_temp map_to_send;
	QString errorString();
	bool tryLoadMap(const QString &fileName);
private:
	QByteArray decompress(const QByteArray &data, int expectedSize);
	QString error;
};

#endif // MAP_LOADER_H
