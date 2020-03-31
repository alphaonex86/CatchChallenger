#ifndef OPTIONSDIALOG_H
#define OPTIONSDIALOG_H

#include <QWidget>
#include <QComboBox>
#include "CCWidget.hpp"
#include "ScreenInput.hpp"
#include "CustomButton.hpp"
#include "CCDialogTitle.hpp"
#include "CCGraphicsTextItem.hpp"
#include "CCSliderH.hpp"

class OptionsDialog : public QObject, public ScreenInput
{
    Q_OBJECT
public:
    explicit OptionsDialog();
    ~OptionsDialog();
    void resizeEvent(QResizeEvent * e);
    QRectF boundingRect() const override;
    void paint(QPainter *p, const QStyleOptionGraphicsItem *, QWidget *widget = nullptr) override;
    void mousePressEventXY(const QPointF &p,bool &pressValidated) override;
    void mouseReleaseEventXY(const QPointF &p, bool &pressValidated) override;
    void mouseMoveEventXY(const QPointF &p, bool &pressValidated) override;

private slots:
    void volumeSliderChange();
    void productKeyChange();
    void languagesChange(int);
private:
    CCWidget *wdialog;
    CustomButton *quit;
    CCDialogTitle *title;
    QGraphicsPixmapItem label;

    CCGraphicsTextItem *volumeText;
    CCSliderH *volumeSlider;
    QGraphicsTextItem *productKeyText;
    QGraphicsPixmapItem *productKeyBackground;
    CCGraphicsTextItem *productKeyInput;

    QGraphicsTextItem *languagesText;
    QComboBox *languagesList;
    QGraphicsProxyWidget *languagesListProxy;

    int x,y;
    QString previousKey;
signals:
    void quitOption();
};

#endif // OPTIONSDIALOG_H
