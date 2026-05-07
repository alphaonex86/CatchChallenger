#ifndef GAUGEWIDGET_H
#define GAUGEWIDGET_H

#include <QWidget>
#include <QColor>

class GaugeWidget : public QWidget {
    Q_OBJECT
public:
    explicit GaugeWidget(QWidget *parent = nullptr);

    void setValue(int value);
    void setMaximum(int max);
    void setBarColor(const QColor &color);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    int value;
    int maximum;
    QColor barColor;
};

#endif // GAUGEWIDGET_H
