#ifndef OPTIONSDIALOG_H
#define OPTIONSDIALOG_H

#include <QWidget>
#include "CCWidget.h"

class OptionsDialog : public QObject, public QGraphicsItem
{
    Q_OBJECT
public:
    explicit OptionsDialog(QGraphicsItem *parent = nullptr);
    ~OptionsDialog();
/*    void resizeEvent(QResizeEvent * e);
private slots:
    void on_close_clicked();

private:
    CCWidget *w;
signals:
    void quitOption();*/
};

#endif // OPTIONSDIALOG_H
