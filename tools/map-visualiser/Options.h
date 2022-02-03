#ifndef OPTIONS_H
#define OPTIONS_H

#include <QDialog>

#include "MapController.h"

namespace Ui {
class Options;
}

class Options : public QDialog
{
    Q_OBJECT
    
public:
    explicit Options(QWidget *parent = 0);
    ~Options();
    
private slots:
    void on_browse_clicked();

    void on_load_clicked();

private:
    Ui::Options *ui;
    MapController *mapController;
};

#endif // OPTIONS_H
