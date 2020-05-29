#ifndef OPTIONSDIALOG_H
#define OPTIONSDIALOG_H

#include <QWidget>
#include <QComboBox>
#include "../ImagesStrechMiddle.hpp"
#include "../ScreenInput.hpp"
#include "../CustomButton.hpp"
#include "../CustomText.hpp"
#include "../CCGraphicsTextItem.hpp"
#include "../CCSliderH.hpp"
#include "../LineEdit.hpp"
#include "../ComboBox.hpp"

class OptionsDialog : public QObject, public ScreenInput
{
    Q_OBJECT
public:
    explicit OptionsDialog();
    ~OptionsDialog();
    void resizeEvent(QResizeEvent * e);
    QRectF boundingRect() const override;
    void paint(QPainter *p, const QStyleOptionGraphicsItem *, QWidget *widget = nullptr) override;
    void mousePressEventXY(const QPointF &p,bool &pressValidated,bool &callParentClass) override;
    void mouseReleaseEventXY(const QPointF &p, bool &pressValidated,bool &callParentClass) override;
    void keyPressEvent(QKeyEvent * event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
private slots:
    void volumeSliderChange();
    void productKeyChange();
    void languagesChange(int);
    void openBuy();

    void newLanguage();
private:
    ImagesStrechMiddle *wdialog;
    CustomButton *quit;
    CustomText *title;
    QGraphicsPixmapItem label;

    CCGraphicsTextItem *volumeText;
    CCSliderH *volumeSlider;
    QGraphicsTextItem *productKeyText;
    LineEdit *productKeyInput;
    CustomButton *buy;

    QGraphicsTextItem *languagesText;
    ComboBox *languagesList;

    int x,y;
    QString previousKey;
signals:
    void removeAbove();
};

#endif // OPTIONSDIALOG_H
