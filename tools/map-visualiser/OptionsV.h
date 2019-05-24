#ifndef OPTIONS_H
#define OPTIONS_H

#include <QDialog>

#include "MapControllerV.h"

namespace Ui {
class OptionsV;
}

class OptionsV : public QDialog
{
    Q_OBJECT

public:
    explicit OptionsV(QWidget *parent = 0);
    ~OptionsV();

private slots:
    void on_browse_clicked();
    void on_load_clicked();
private:
    Ui::OptionsV *ui;
    MapControllerV *mapController;
};

#endif // OPTIONS_H
