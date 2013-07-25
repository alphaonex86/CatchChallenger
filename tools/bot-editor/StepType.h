#ifndef STEPTYPE_H
#define STEPTYPE_H

#include <QDialog>

namespace Ui {
class StepType;
}

class StepType : public QDialog
{
    Q_OBJECT

public:
    explicit StepType(const QHash<QString,QString> &allowedType,QWidget *parent = 0);
    ~StepType();
    bool validated();
    QString type();
private slots:
    void on_cancel_clicked();
    void on_ok_clicked();
private:
    Ui::StepType *ui;
    bool mValidated;
};

#endif // STEPTYPE_H
