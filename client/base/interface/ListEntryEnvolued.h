#ifndef LISTENTRYENVOLUED_H
#define LISTENTRYENVOLUED_H

#include <QLabel>
#include <QTime>

class ListEntryEnvolued : public QLabel
{
    Q_OBJECT
public:
    explicit ListEntryEnvolued();
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

#endif // LISTENTRYENVOLUED_H
