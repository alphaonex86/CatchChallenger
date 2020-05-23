#ifndef OPTIONSDIALOG_H
#define OPTIONSDIALOG_H

#include <QWidget>
#include "CCWidget.hpp"

namespace Ui {
class OptionsDialog;
}

class OptionsDialog : public QWidget
{
    Q_OBJECT

public:
    explicit OptionsDialog(QWidget *parent = nullptr);
    ~OptionsDialog();
    void resizeEvent(QResizeEvent * e);
private slots:
    void on_close_clicked();

private:
    Ui::OptionsDialog *ui;
    CCWidget *w;
signals:
    void quitOption();
};

#endif // OPTIONSDIALOG_H
