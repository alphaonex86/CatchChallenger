#ifndef TARGETFILTER_H
#define TARGETFILTER_H

#include <QDialog>
#include <QAbstractButton>

namespace Ui {
class TargetFilter;
}

class TargetFilter : public QDialog
{
    Q_OBJECT

public:
    explicit TargetFilter(QWidget *parent,bool dirt,bool itemOnMap,bool fight,bool shop,bool heal,bool wildMonster);
    ~TargetFilter();
    bool get_dirt();
    bool get_itemOnMap();
    bool get_fight();
    bool get_shop();
    bool get_heal();
    bool get_wildMonster();
private slots:
    void on_buttonBox_clicked(QAbstractButton *button);
private:
    Ui::TargetFilter *ui;
};

#endif // TARGETFILTER_H
