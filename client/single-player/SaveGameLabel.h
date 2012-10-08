#ifndef QSAVEGAMELABEL_H
#define QSAVEGAMELABEL_H

#include <QLabel>
#include <QTime>

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
    void doubleClicked();
private:
    QTime lastClick;
    bool haveFirstClick;
};

#endif // QSAVEGAMELABEL_H
