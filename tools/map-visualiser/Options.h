#ifndef OPTIONS_H
#define OPTIONS_H

#include <QDialog>

#include "../../client/base/render/MapVisualiser.h"

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
    MapVisualiser *mapVisualiser;
};

#endif // OPTIONS_H
