#ifndef Shop_H
#define Shop_H

#include <QObject>
#include <QListWidget>
#include "../ScreenInput.hpp"
#include "../ImagesStrechMiddle.hpp"
#include "../CustomButton.hpp"
#include "../CustomText.hpp"
#include "../../../general/base/GeneralStructures.hpp"

class ConnexionManager;

class Shop : public ScreenInput
{
    Q_OBJECT
public:
    explicit Shop();
    ~Shop();

    QRectF boundingRect() const override;
    void paint(QPainter *p, const QStyleOptionGraphicsItem *, QWidget *widget = nullptr) override;
    void mousePressEventXY(const QPointF &p,bool &pressValidated,bool &callParentClass) override;
    void mouseReleaseEventXY(const QPointF &p, bool &pressValidated,bool &callParentClass) override;
    void keyPressEvent(QKeyEvent * event, bool &eventTriggerGeneral) override;
    void keyReleaseEvent(QKeyEvent *event, bool &eventTriggerGeneral) override;
    void removeAbove();
    void setVar(ConnexionManager *connexionManager, const std::string &sellerName, const uint8_t &skinId);
private slots:
    void newLanguage();
    void on_shopItemList_itemSelectionChanged();
    void onBuyClicked();
    void haveShopList(const std::vector<CatchChallenger::ItemToSellOrBuy> &items);
    void haveBuyObject(const CatchChallenger::BuyStat &stat,const uint32_t &newPrice);
    void haveSellObject(const CatchChallenger::SoldStat &stat,const uint32_t &newPrice);
private:
    ImagesStrechMiddle *wdialog;
    CustomButton *quit;
    CustomText *title;
    QGraphicsPixmapItem label;

    QGraphicsPixmapItem *sellerImage;
    QGraphicsTextItem *sellerNameText;

    QListWidget *shopItemList;
    QGraphicsProxyWidget *shopListProxy;

    ImagesStrechMiddle *descriptionBack;
    QGraphicsTextItem *shopDescription;
    CustomButton *shopBuy;

    ConnexionManager *connexionManager;
    std::unordered_map<QListWidgetItem*,uint16_t> shop_items_graphical;
    std::unordered_map<uint16_t,QListWidgetItem*> shop_items_to_graphical;
    std::unordered_map<uint16_t,CatchChallenger::ItemToSellOrBuy> itemsIntoTheShop;
    std::vector<CatchChallenger::ItemToSellOrBuy> itemsToBuy;
signals:
    void setAbove(ScreenInput *widget);
    void add_to_inventory(const uint16_t &item, const uint32_t &quantity, const bool &showGain);
    void remove_to_inventory(const uint16_t &itemId, const uint32_t &quantity);
    void addCash(const uint32_t &cash);
    void removeCash(const uint32_t &cash);
    void showTip(const std::string &tip);
};

#endif // Shop_H
