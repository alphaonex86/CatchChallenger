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
    struct item
    {
        QPixmap image;
        QString name;
        QString description;
    };
    static QHash<quint32,item> items;
    QPixmap defaultInventoryImage();
protected:
    void run();
public slots:
    void parseDatapack(const QString &datapackPath);
signals:
    void datapackParsed();
private:
    QString datapackPath;
    QPixmap mDefaultInventoryImage;
private slots:
    void parseItems();
};

#endif // DATAPACKCLIENTLOADER_H
