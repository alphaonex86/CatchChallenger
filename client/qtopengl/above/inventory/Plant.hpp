#ifndef Plant_H
#define Plant_H

#include <QObject>
#include <QTreeWidget>
#include "../../ImagesStrechMiddle.hpp"
#include "../../ScreenInput.hpp"
#include "../../CustomButton.hpp"
#include "../../CustomText.hpp"
#include "../../CCGraphicsTextItem.hpp"
#include "../../CCSliderH.hpp"
#include "../../LineEdit.hpp"
#include "../../SpinBox.hpp"

class ConnexionManager;

class Plant : public ScreenInput
{
    Q_OBJECT
public:
    explicit Plant();
    ~Plant();

    void resizeEvent(QResizeEvent * e);
    QRectF boundingRect() const override;
    void paint(QPainter *p, const QStyleOptionGraphicsItem *, QWidget *widget = nullptr) override;
    void mousePressEventXY(const QPointF &p,bool &pressValidated,bool &callParentClass) override;
    void mouseReleaseEventXY(const QPointF &p, bool &pressValidated,bool &callParentClass) override;
    void keyPressEvent(QKeyEvent * event, bool &eventTriggerGeneral) override;
    void keyReleaseEvent(QKeyEvent *event, bool &eventTriggerGeneral) override;
    void removeAbove();
    void setVar(ConnexionManager *connexionManager,const bool inSelection,const int lastItemSelected=-1);

    void on_inventory_itemSelectionChanged();
    void inventoryUse_slot();
private slots:
    void newLanguage();
    void updatePlant();
    static QPixmap setCanvas(const QPixmap &image,unsigned int size);
    static QPixmap setCanvas(const QImage &image,unsigned int size);
private:
    ImagesStrechMiddle *wdialog;
    CustomButton *quit;
    CustomText *title;
    QGraphicsPixmapItem label;

    CustomButton *next;
    CustomButton *back;

    QTreeWidget *plantList;
    QGraphicsProxyWidget *plantListProxy;

    ImagesStrechMiddle *descriptionBack;
    QGraphicsTextItem *inventory_description;
    CustomButton *inventoryUse;

    ConnexionManager *connexionManager;
    bool inSelection;
    uint8_t lastItemSize;
    int lastItemSelected;
signals:
    void setAbove(ScreenInput *widget);//first plan popup
    void sendNext();
    void sendBack();
    void useItem(uint16_t item);
};

#endif // OPTIONSDIALOG_H
