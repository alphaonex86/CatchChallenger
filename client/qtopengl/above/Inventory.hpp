#ifndef Inventory_H
#define Inventory_H

#include <QObject>
#include <QListWidget>
#include "../ImagesStrechMiddle.hpp"
#include "../ScreenInput.hpp"
#include "../CustomButton.hpp"
#include "../CustomText.hpp"
#include "../CCGraphicsTextItem.hpp"
#include "../CCSliderH.hpp"
#include "../LineEdit.hpp"
#include "../SpinBox.hpp"

class ConnexionManager;

class Inventory : public ScreenInput
{
    Q_OBJECT
public:
    explicit Inventory();
    ~Inventory();

    enum ObjectType
    {
        ObjectType_All,
        ObjectType_Seed,
        ObjectType_Sell,
        ObjectType_SellToMarket,
        ObjectType_Trade,
        ObjectType_MonsterToTrade,
        ObjectType_MonsterToTradeToMarket,
        ObjectType_MonsterToLearn,
        ObjectType_MonsterToFight,
        ObjectType_MonsterToFightKO,
        ObjectType_ItemOnMonsterOutOfFight,
        ObjectType_ItemOnMonster,
        ObjectType_ItemEvolutionOnMonster,
        ObjectType_ItemLearnOnMonster,
        ObjectType_UseInFight
    };

    void resizeEvent(QResizeEvent * e);
    QRectF boundingRect() const override;
    void paint(QPainter *p, const QStyleOptionGraphicsItem *, QWidget *widget = nullptr) override;
    void mousePressEventXY(const QPointF &p,bool &pressValidated,bool &callParentClass) override;
    void mouseReleaseEventXY(const QPointF &p, bool &pressValidated,bool &callParentClass) override;
    void keyPressEvent(QKeyEvent * event, bool &eventTriggerGeneral) override;
    void keyReleaseEvent(QKeyEvent *event, bool &eventTriggerGeneral) override;
    void removeAbove();
    void setVar(ConnexionManager *connexionManager,const ObjectType &waitedObjectType,const bool inSelection,const int lastItemSelected=-1);

    void on_inventory_itemSelectionChanged();
    void inventoryUse_slot();
    void inventoryDestroy_slot();
private slots:
    void newLanguage();
    void updateInventory(uint8_t targetSize);
private:
    ImagesStrechMiddle *wdialog;
    CustomButton *quit;
    CustomText *title;
    QGraphicsPixmapItem label;

    CustomButton *next;
    CustomButton *back;

    QGraphicsProxyWidget *proxy;
    QListWidget *inventory;

    ImagesStrechMiddle *descriptionBack;
    QGraphicsTextItem *inventory_description;
    CustomButton *inventoryDestroy;
    CustomButton *inventoryUse;

    ConnexionManager *connexionManager;
    Inventory::ObjectType waitedObjectType;
    bool inSelection;
    uint8_t lastItemSize;
    uint8_t itemCount;
    int lastItemSelected;
signals:
    void setAbove(ScreenInput *widget);//first plan popup
    void sendNext();
    void sendBack();
    void useItem(uint16_t item);
    void deleteItem(uint16_t item);
};

#endif // OPTIONSDIALOG_H
