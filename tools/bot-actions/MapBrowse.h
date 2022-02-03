#ifndef MAPBROWSE_H
#define MAPBROWSE_H

#include <QDialog>

namespace Ui {
class MapBrowse;
}

class MapBrowse : public QDialog
{
    Q_OBJECT

public:
    explicit MapBrowse(std::vector<std::string> mapList,QWidget *parent = 0);
    ~MapBrowse();
    std::string mapSelected();//empty is not selected
private slots:
    void on_lineEdit_textChanged(const QString &arg1);
    void on_buttonBox_rejected();
    void on_mapList_activated(const QModelIndex &index);
    void on_buttonBox_accepted();
    void on_filter_textEdited(const QString &arg1);

private:
    Ui::MapBrowse *ui;
    void updateFilter();
    std::vector<std::string> mapList;
    std::string mapSelectedString;
};

#endif // MAPBROWSE_H
