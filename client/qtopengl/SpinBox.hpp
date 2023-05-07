#ifndef SpinBox_H
#define SpinBox_H

#include <QGraphicsProxyWidget>
#include <QSpinBox>

class SpinBox : public QGraphicsProxyWidget
{
public:
    SpinBox(QGraphicsItem * parent = 0);
    ~SpinBox();
    void setFixedWidth(int w);
    void setFixedHeight(int h);
    void setFixedSize(const QSize &s);
    void setFixedSize(int w, int h);
    void setValue(int val);
    int value() const;
    int minimum() const;
    void setMinimum(int min);
    int maximum() const;
    void setMaximum(int max);
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
private:
    QSpinBox *m_spinBox;
};

#endif // SpinBox_H
