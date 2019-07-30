#ifndef SCREENTRANSITION_H
#define SCREENTRANSITION_H

#include <QWidget>

class ScreenTransition : public QWidget
{
    Q_OBJECT
public:
    explicit ScreenTransition();
    ~ScreenTransition();
    void setBackground(QWidget *widget);
    void setForeground(QWidget *widget);
protected:
    void paintEvent(QPaintEvent *) override;
    void resizeEvent(QResizeEvent *) override;
private:
    QWidget *m_background;
    QWidget *m_foreground;
};

#endif // SCREENTRANSITION_H
