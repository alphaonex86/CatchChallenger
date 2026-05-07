#ifndef CHARTWIDGET_H
#define CHARTWIDGET_H

#include <QWidget>
#include <QVector>
#include <QColor>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QFontMetrics>

struct ChartSeries {
    QString name;
    QColor color;
    QColor fillColor;
    QVector<double> values;
};

class ChartWidget : public QWidget {
    Q_OBJECT
public:
    explicit ChartWidget(QWidget *parent = nullptr);

    void setTitle(const QString &title);
    void setSeries(const QVector<ChartSeries> &series);
    void setUnitSuffix(const QString &suffix);
    void addDataPoint(int seriesIndex, double value);
    void clearData();
    int maxDataPoints() const;
    void setMaxDataPoints(int max);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    QString title;
    QString unitSuffix;
    QVector<ChartSeries> series;
    int hoverIndex;
    int maxPoints;

    void drawBackground(QPainter &painter, const QRectF &rect);
    void drawGrid(QPainter &painter, const QRectF &rect);
    void drawYLabels(QPainter &painter, const QRectF &rect);
    void drawSeries(QPainter &painter, const QRectF &rect, int seriesIndex);
    void drawTooltip(QPainter &painter, const QRectF &rect);
    void drawTimeLabels(QPainter &painter, const QRectF &rect);
    void drawMaxValue(QPainter &painter, const QRectF &rect);
    QPointF mapPoint(int index, double value, const QRectF &rect) const;
    double maxValue() const;
    double minValue() const;
    QString formatValue(double value) const;
};

#endif // CHARTWIDGET_H
