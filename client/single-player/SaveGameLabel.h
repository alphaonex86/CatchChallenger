#ifndef QSAVEGAMELABEL_H
#define QSAVEGAMELABEL_H

#include <QLabel>

class SaveGameLabel : public QLabel
{
    Q_OBJECT
public:
    explicit SaveGameLabel();
protected:
    void mousePressEvent(QMouseEvent *);
    void mouseReleaseEvent(QMouseEvent *);
signals:
    void clicked();
};

#endif // QSAVEGAMELABEL_H
