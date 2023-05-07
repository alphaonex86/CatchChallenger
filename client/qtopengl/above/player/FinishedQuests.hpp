#ifndef FinishedQuests_H
#define FinishedQuests_H

#include <QObject>
#include <QListWidget>
#include "../../ImagesStrechMiddle.hpp"
#include "../../ScreenInput.hpp"
#include "../../CustomButton.hpp"
#include "../../CustomText.hpp"
#include "../../CCGraphicsTextItem.hpp"
#include "../../CCSliderH.hpp"
#include "../../LineEdit.hpp"
#include "../../SpinBox.hpp"

class ConnexionManager;

class FinishedQuests : public ScreenInput
{
    Q_OBJECT
public:
    explicit FinishedQuests();
    ~FinishedQuests();

    void resizeEvent(QResizeEvent * e);
    QRectF boundingRect() const override;
    void paint(QPainter *p, const QStyleOptionGraphicsItem *, QWidget *widget = nullptr) override;
    void mousePressEventXY(const QPointF &p,bool &pressValidated,bool &callParentClass) override;
    void mouseReleaseEventXY(const QPointF &p, bool &pressValidated,bool &callParentClass) override;
    void keyPressEvent(QKeyEvent * event, bool &eventTriggerGeneral) override;
    void keyReleaseEvent(QKeyEvent *event, bool &eventTriggerGeneral) override;
    void removeAbove();
    void setVar(ConnexionManager *connexionManager);
private slots:
    void newLanguage();
private:
    ImagesStrechMiddle *wdialog;
    CustomButton *quit;
    CustomText *title;
    QGraphicsPixmapItem label;

    CustomButton *next;
    CustomButton *back;

    QGraphicsTextItem *finishedQuests;

    ConnexionManager *connexionManager;
    std::string imagesInterfaceRepeatableString;
    std::string imagesInterfaceInProgressString;
signals:
    void setAbove(ScreenInput *widget);//first plan popup
    void sendNext();
    void sendBack();
    void useItem(uint16_t item);
    void deleteItem(uint16_t item);
};

#endif // OPTIONSDIALOG_H
