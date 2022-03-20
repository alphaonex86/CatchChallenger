#ifndef ASKKEY_H
#define ASKKEY_H

#include <QDialog>

namespace Ui {
class AskKey;
}

class AskKey : public QDialog
{
    Q_OBJECT

public:
    explicit AskKey(QWidget *parent = 0);
    ~AskKey();
    QString key() const;
private slots:
    void on_pushButton_clicked();
private:
    Ui::AskKey *ui;
};

#endif // ASKKEY_H
