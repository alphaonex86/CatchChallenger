#ifndef DATAPACKCLIENTLOADER_H
#define DATAPACKCLIENTLOADER_H

#include <QThread>
#include <QPixmap>
#include <QHash>
#include <QString>

class DatapackClientLoader : public QThread
{
    Q_OBJECT
public:
    explicit DatapackClientLoader();
    void resetAll();

    //static items
    static QHash<quint32,QPixmap> items;
    static QPixmap defaultInventoryImage;
protected:
    void run();
public slots:
    void parseDatapack(const QString &datapackPath);
signals:
    void datapackParsed();
private:
    QString datapackPath;
private slots:
    void parseItems();
};

#endif // DATAPACKCLIENTLOADER_H
