#ifndef DATAPACKCLIENTLOADER_H
#define DATAPACKCLIENTLOADER_H

#include <QThread>
#include <QPixmap>
#include <QHash>
#include <QString>
#include <QStringList>

class DatapackClientLoader : public QThread
{
    Q_OBJECT
public:
    void resetAll();

    //static items
    struct item
    {
        QPixmap image;
        QString name;
        QString description;
    };
    QHash<quint32,item> items;
    QStringList maps;
    QPixmap defaultInventoryImage();
    static DatapackClientLoader datapackLoader;
protected:
    void run();
public slots:
    void parseDatapack(const QString &datapackPath);
signals:
    void datapackParsed();
private:
    QString datapackPath;
    QPixmap *mDefaultInventoryImage;
    explicit DatapackClientLoader();
    ~DatapackClientLoader();
private slots:
    void parseItems();
    void parseMaps();
};

#endif // DATAPACKCLIENTLOADER_H
