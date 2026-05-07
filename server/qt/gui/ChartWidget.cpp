#include "ChartWidget.hpp"

#include <QPainter>
#include <QPainterPath>
#include <QFontMetrics>
#include <QToolTip>
#include <QPen>
#include <QBrush>
#include <QLinearGradient>
#include <QTime>
#include <algorithm>
#include <cmath>

ChartWidget::ChartWidget(QWidget *parent)
    : QWidget(parent), hoverIndex(-1), maxPoints(17280)
{
    setMinimumSize(300, 150);
    setMouseTracking(true);
}

void ChartWidget::setTitle(const QString &t) { title = t; update(); }

void ChartWidget::setSeries(const QVector<ChartSeries> &s) { series = s; update(); }

void ChartWidget::setUnitSuffix(const QString &suffix) { unitSuffix = suffix; update(); }

void ChartWidget::addDataPoint(int seriesIndex, double value) {
    if (seriesIndex < 0 || seriesIndex >= series.size()) return;
    series[seriesIndex].values.append(value);
    while (series[seriesIndex].values.size() > maxPoints)
        series[seriesIndex].values.remove(0);
    update();
}

void ChartWidget::clearData() {
    for (auto &s : series) s.values.clear();
    hoverIndex = -1;
    update();
}

int ChartWidget::maxDataPoints() const { return maxPoints; }
void ChartWidget::setMaxDataPoints(int max) { maxPoints = max; }

double ChartWidget::maxValue() const {
    double m = 0;
    for (const auto &s : series)
        for (double v : s.values)
            if (v > m) m = v;
    return m > 0 ? m : 1;
}

double ChartWidget::minValue() const {
    double m = 0;
    bool first = true;
    for (const auto &s : series)
        for (double v : s.values) {
            if (first) { m = v; first = false; }
            else if (v < m) m = v;
        }
    return m;
}

QPointF ChartWidget::mapPoint(int index, double value, const QRectF &rect) const {
    int dataCount = 1;
    for (const auto &s : series) { dataCount = std::max(dataCount, static_cast<int>(s.values.size())); break; }
    if (dataCount <= 1) dataCount = 2;
    double x = rect.left() + (static_cast<double>(index) / (dataCount - 1)) * rect.width();
    double minV = 0;
    double maxV = maxValue();
    double range = maxV - minV;
    if (range < 0.001) range = 1;
    double y = rect.bottom() - ((value - minV) / range) * rect.height();
    return QPointF(x, y);
}

void ChartWidget::paintEvent(QPaintEvent *) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QFont labelFont = painter.font();
    labelFont.setPointSize(8);
    QFontMetrics fm(labelFont);
    double maxV = maxValue();
    int nLines = (maxV >= 5) ? 6 : (qMax(static_cast<int>(maxV) + 1, 1));
    int maxW = 0;
    for (int i = 1; i < nLines; ++i)
        maxW = qMax(maxW, fm.horizontalAdvance(formatValue((static_cast<double>(i) / (nLines - 1)) * maxV)));
    int leftMargin = qMax(40, maxW + 20);

    QRectF rect(leftMargin, 20, width() - leftMargin - 10, height() - 40);

    drawBackground(painter, rect);
    drawGrid(painter, rect);
    drawYLabels(painter, rect);

    for (int i = 0; i < series.size(); ++i)
        drawSeries(painter, rect, i);

    drawTimeLabels(painter, rect);
    drawMaxValue(painter, rect);
    drawTooltip(painter, rect);

    painter.setPen(palette().text().color());
    QFont font = painter.font();
    font.setBold(true);
    painter.setFont(font);
    painter.drawText(10, 15, title);

    QRectF legendRect(10, height() - 15, width() - 20, 15);
    painter.setFont(painter.font());
    font.setBold(false);
    painter.setFont(font);
    double x = legendRect.left();
    for (const auto &s : series) {
        painter.setPen(s.color);
        painter.fillRect(QRectF(x, legendRect.top() + 2, 12, 10), s.color);
        painter.setPen(palette().text().color());
        int w = painter.fontMetrics().horizontalAdvance(s.name);
        painter.drawText(x + 16, legendRect.top() + 11, s.name);
        x += 20 + w + 10;
    }
}

void ChartWidget::drawBackground(QPainter &painter, const QRectF &rect) {
    painter.fillRect(rect, QColor(245, 245, 250));
}

void ChartWidget::drawGrid(QPainter &painter, const QRectF &rect) {
    painter.setPen(QPen(QColor(220, 220, 230), 1));
    double max = maxValue();
    int numLines;
    if (max >= 5) {
        numLines = 6;
    } else {
        numLines = static_cast<int>(max) + 1;
    }
    if (numLines < 1) numLines = 1;

    if (numLines == 1) {
        painter.drawLine(QLineF(rect.left(), rect.bottom(), rect.right(), rect.bottom()));
    } else {
        for (int i = 0; i < numLines; ++i) {
            double val = (static_cast<double>(i) / (numLines - 1)) * max;
            double y = rect.bottom() - (val / max) * rect.height();
            painter.drawLine(QLineF(rect.left(), y, rect.right(), y));
        }
    }
}

void ChartWidget::drawYLabels(QPainter &painter, const QRectF &rect) {
    painter.setPen(QColor(150, 150, 160));
    QFont font = painter.font();
    font.setPointSize(8);
    painter.setFont(font);
    double max = maxValue();
    int numLines;
    if (max >= 5) {
        numLines = 6;
    } else {
        numLines = static_cast<int>(max) + 1;
    }
    if (numLines < 1) numLines = 1;
    if (numLines <= 1) return;
    for (int i = 1; i < numLines; ++i) {
        double val = (static_cast<double>(i) / (numLines - 1)) * max;
        double y = rect.bottom() - (val / max) * rect.height();
        QString label = formatValue(val);
        painter.drawText(QRectF(3, y - 6, rect.left() - 6, 12), Qt::AlignRight, label);
    }
}

void ChartWidget::drawSeries(QPainter &painter, const QRectF &rect, int seriesIndex) {
    const auto &s = series[seriesIndex];
    if (s.values.isEmpty()) return;

    QPainterPath linePath;
    QPainterPath fillPath;

    QPointF firstPoint = mapPoint(0, s.values.first(), rect);
    linePath.moveTo(firstPoint);
    fillPath.moveTo(firstPoint.x(), rect.bottom());
    fillPath.lineTo(firstPoint);

    for (int i = 1; i < s.values.size(); ++i) {
        QPointF p = mapPoint(i, s.values[i], rect);
        linePath.lineTo(p);
        fillPath.lineTo(p);
    }

    fillPath.lineTo(fillPath.currentPosition().x(), rect.bottom());
    fillPath.closeSubpath();

    QLinearGradient gradient(rect.topLeft(), QPointF(rect.topLeft().x(), rect.bottom()));
    QColor fillColor = s.fillColor;
    fillColor.setAlpha(60);
    gradient.setColorAt(0, s.color.lighter(150));
    gradient.setColorAt(1, fillColor);

    painter.fillPath(fillPath, gradient);
    painter.setPen(QPen(s.color, 2));
    painter.drawPath(linePath);
}

QString ChartWidget::formatValue(double value) const {
    if (!unitSuffix.isEmpty()) {
        if (unitSuffix == "/s") {
            if (value < 1024) return QString::number(static_cast<int>(value)) + "B" + unitSuffix;
            if (value < 1024 * 1024) return QString::number(value / 1024.0, 'f', 1) + "KB" + unitSuffix;
            if (value < 1024 * 1024 * 1024) return QString::number(value / (1024.0 * 1024.0), 'f', 1) + "MB" + unitSuffix;
            return QString::number(value / (1024.0 * 1024.0 * 1024.0), 'f', 1) + "GB" + unitSuffix;
        } else {
            return QString::number(static_cast<int>(value)) + unitSuffix;
        }
    }
    return QString::number(static_cast<int>(value));
}

void ChartWidget::drawTimeLabels(QPainter &painter, const QRectF &rect) {
    painter.setPen(QColor(150, 150, 160));
    int dataCount = series.isEmpty() ? 0 : series.first().values.size();
    if (dataCount == 0) return;

    int step = qMax(1, dataCount / 6);
    QTime now = QTime::currentTime();
    for (int i = step; i < dataCount; i += step) {
        QPointF p = mapPoint(i, 0, rect);
        int secondsAgo = (dataCount - 1 - i) * 5;
        QTime t = now.addSecs(-secondsAgo);
        painter.drawText(QRectF(p.x() - 15, rect.bottom() + 2, 30, 15), Qt::AlignCenter, t.toString("HH:mm"));
    }
}

void ChartWidget::drawMaxValue(QPainter &painter, const QRectF &rect) {
    double max = maxValue();
    QString maxText = formatValue(max);
    painter.setPen(QColor(100, 100, 120));
    QFont font = painter.font();
    font.setPointSize(9);
    painter.setFont(font);
    painter.drawText(QRectF(rect.right() - 60, rect.top() + 2, 58, 15), Qt::AlignRight, maxText);
}

void ChartWidget::drawTooltip(QPainter &painter, const QRectF &rect) {
    if (hoverIndex < 0 || series.isEmpty() || series.first().values.isEmpty()) return;
    if (hoverIndex >= series.first().values.size()) return;

    QString tooltip;
    for (const auto &s : series) {
        if (hoverIndex < s.values.size()) {
            if (!tooltip.isEmpty()) tooltip += "\n";
            tooltip += s.name + ": " + formatValue(s.values[hoverIndex]);
        }
    }

    if (tooltip.isEmpty()) return;

    QPointF p = mapPoint(hoverIndex, series.first().values[hoverIndex], rect);

    painter.setPen(QPen(QColor(60, 60, 80), 1, Qt::DashLine));
    painter.drawLine(QLineF(p.x(), rect.top(), p.x(), rect.bottom()));

    for (const auto &s : series) {
        if (hoverIndex < s.values.size()) {
            QPointF pt = mapPoint(hoverIndex, s.values[hoverIndex], rect);
            painter.fillRect(QRectF(pt.x() - 5, pt.y() - 5, 10, 10), s.color);
        }
    }

    QFont font = painter.font();
    font.setPointSize(9);
    painter.setFont(font);
    QFontMetrics fm(font);
    QRect textRect = fm.boundingRect(QRect(0, 0, 200, 100), Qt::TextWordWrap, tooltip);
    textRect.adjust(-6, -4, 6, 4);

    double tx = p.x() + 10;
    double ty = rect.top() + 5;
    if (tx + textRect.width() > rect.right()) tx = p.x() - textRect.width() - 10;
    if (ty + textRect.height() > rect.bottom()) ty = rect.bottom() - textRect.height();

    painter.fillRect(QRectF(tx, ty, textRect.width(), textRect.height()), QColor(30, 30, 40, 230));
    painter.setPen(Qt::white);
    painter.drawText(QRectF(tx + 4, ty + 2, textRect.width() - 8, textRect.height() - 4), Qt::TextWordWrap, tooltip);
}

void ChartWidget::mouseMoveEvent(QMouseEvent *event) {
    if (series.isEmpty() || series.first().values.isEmpty()) {
        hoverIndex = -1;
        update();
        return;
    }

    QRectF rect(10, 20, width() - 20, height() - 40);
    int dataCount = series.first().values.size();
    if (dataCount <= 1) dataCount = 2;

    double relX = (event->pos().x() - rect.left()) / rect.width();
    hoverIndex = qRound(relX * (dataCount - 1));
    hoverIndex = qBound(0, hoverIndex, dataCount - 1);
    update();
}

void ChartWidget::leaveEvent(QEvent *) {
    hoverIndex = -1;
    update();
}
