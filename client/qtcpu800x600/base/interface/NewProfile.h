#ifndef NEWPROFILE_H
#define NEWPROFILE_H

#include <QDialog>

#include "../../../../general/base/GeneralStructures.hpp"
#include "../../../libcatchchallenger/DatapackClientLoader.hpp"

namespace Ui {
class NewProfile;
}

class NewProfile : public QDialog
{
    Q_OBJECT
public:
    std::vector<DatapackClientLoader::ProfileText> profileTextList;
    int getProfileIndex();
    int getProfileCount();
    explicit NewProfile(const std::string &datapackPath,QWidget *parent = 0);
    ~NewProfile();
    bool ok();
public slots:
    void on_ok_clicked();
private slots:
    void on_pushButton_2_clicked();
    void on_comboBox_currentIndexChanged(int index);
    void datapackParsed();
private:
    bool mOk;
    Ui::NewProfile *ui;
    std::string datapackPath;
    bool isParsingDatapack;
signals:
    void parseDatapack(const std::string &datapackPath);
};

#endif // NEWPROFILE_H
