#ifndef NEWPROFILE_H
#define NEWPROFILE_H

#include <QDialog>

#include "../../../general/base/GeneralStructures.h"

namespace Ui {
class NewProfile;
}

class NewProfile : public QDialog
{
    Q_OBJECT
public:
    struct ProfileText
    {
        QString name;
        QString description;
    };
    QList<ProfileText> profileTextList;
    int getProfileIndex();
    explicit NewProfile(const QString &datapackPath,QWidget *parent = 0);
    ~NewProfile();
    void loadProfileText();
    bool ok();
private slots:
    void on_ok_clicked();
    void on_pushButton_2_clicked();
    void on_comboBox_currentIndexChanged(int index);
    void datapackParsed();
private:
    bool mOk;
    Ui::NewProfile *ui;
    QString datapackPath;
    bool isParsingDatapack;
signals:
    void parseDatapack(const QString &datapackPath);
};

#endif // NEWPROFILE_H
