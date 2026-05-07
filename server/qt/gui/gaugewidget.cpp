#include "gaugewidget.h"
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QConicalGradient>

GaugeWidget::GaugeWidget(QWidget *parent)
    : QWidget(parent), value(0), maximum(100), barColor(74, 144, 217)
{
    setMinimumSize(40, 40);
    setMaximumSize(40, 40);
}

void GaugeWidget::setValue(int val) {
    value = qBound(0, val, maximum);
    update();
}

void GaugeWidget::setMaximum(int max) {
    maximum = max > 0 ? max : 100;
    update();
}

void GaugeWidget::setBarColor(const QColor &color) {
    barColor = color;
    update();
}

void GaugeWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    int side = qMin(width(), height());
    QRectF rect(2, 2, side - 4, side - 4);
    int startAngle = 90 * 16;
    int spanAngle = -static_cast<int>(static_cast<double>(value) / maximum * 360 * 16);

    QPen pen(barColor, 4);
    pen.setCapStyle(Qt::RoundCap);
    painter.setPen(pen);
    painter.drawArc(rect, startAngle, spanAngle);

    QPen penBg(QColor(230, 230, 235), 4);
    penBg.setCapStyle(Qt::RoundCap);
    painter.setPen(penBg);
    painter.drawArc(rect, startAngle + spanAngle, -360 * 16 - spanAngle);
}
