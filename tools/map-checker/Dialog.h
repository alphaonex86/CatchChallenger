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
    void scanFolder(const QDir &dir);
    QString loadOtherMap(const QString &fileName);
private:
    Ui::Dialog *ui;
    QHash<QString,Pokecraft::Map_client> other_map;
};

#endif // DIALOG_H
