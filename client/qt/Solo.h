#ifndef SOLO_H
#define SOLO_H

#include <QWidget>

namespace Ui {
class Solo;
}

class Solo : public QWidget
{
    Q_OBJECT
public:
    explicit Solo(QWidget *parent = nullptr);
    ~Solo();
private:
    Ui::Solo *ui;
signals:
    void backMain();
};

#endif // SOLO_H
