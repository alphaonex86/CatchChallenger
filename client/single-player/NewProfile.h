#ifndef NEWPROFILE_H
#define NEWPROFILE_H

#include <QDialog>

namespace Ui {
class NewProfile;
}

class NewProfile : public QDialog
{
    Q_OBJECT
public:
    struct Profile
    {
        struct Reputation
        {
            QString type;
            qint8 level;
            qint32 point;
        };
        struct Monster
        {
            quint32 id;
            qint8 level;
            qint32 captured_with;
        };
        struct Item
        {
            quint32 id;
            quint32 quantity;
        };
        QString name;
        QString description;
        QString map;
        quint8 x,y;
        QStringList forcedskin;
        quint64 cash;
        QList<Monster> monsters;
        QList<Reputation> reputation;
        QList<Item> items;
    };
    QList<Profile> profileList;
    bool ok;
    Profile getProfile();
    explicit NewProfile(const QString &datapackPath,QWidget *parent = 0);
    ~NewProfile();
    int profileListSize();
private slots:
    void on_ok_clicked();
    void on_pushButton_2_clicked();
    void on_comboBox_currentIndexChanged(int index);
    void datapackParsed();
private:
    Ui::NewProfile *ui;
    QString datapackPath;
    bool isParsingDatapack;
signals:
    void parseDatapack(const QString &datapackPath);
};

#endif // NEWPROFILE_H
