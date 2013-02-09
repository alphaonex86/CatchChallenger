#define DEBUG_MESSAGE_MAP_TP
#define DEBUG_MESSAGE_MAP
#define DEBUG_MESSAGE_MAP_BORDER
#define DEBUG_MESSAGE_MAP_OBJECT

#ifndef DIALOG_H
#define DIALOG_H

#include <QDialog>
#include <QString>
#include <QDir>

#include "../../client/base/Map_client.h"
#include "mapreader.h"

namespace Ui {
class Dialog;
}

class Dialog : public QDialog
{
    Q_OBJECT
    
public:
    explicit Dialog(QWidget *parent = 0);
    ~Dialog();

    void scanMaps(const QString &folderName);
    void scanFolderMap(const QDir &dir);
    void scanFolderPosition(const QDir &dir);
    QString loadMap(const QString &fileName);
    void loadPosition(const QString &fileName);
private:
    struct Map_full
    {
        CatchChallenger::Map_client map;
        QStringList log;
        qint32 x,y;
        bool position_set;
    };
    Ui::Dialog *ui;
    QHash<QString,Map_full> other_map;
    Tiled::MapReader reader;
};

#endif // DIALOG_H
