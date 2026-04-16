#ifndef Factory_H
#define Factory_H

#include <QObject>
#include <QListWidget>
#include "../ScreenInput.hpp"
#include "../ImagesStrechMiddle.hpp"
#include "../CustomButton.hpp"
#include "../CustomText.hpp"
#include "../../../general/base/GeneralStructures.hpp"

class ConnexionManager;

class Factory : public ScreenInput
{
    Q_OBJECT
public:
    explicit Factory();
    ~Factory();

    QRectF boundingRect() const override;
    void paint(QPainter *p, const QStyleOptionGraphicsItem *, QWidget *widget = nullptr) override;
    void mousePressEventXY(const QPointF &p,bool &pressValidated,bool &callParentClass) override;
    void mouseReleaseEventXY(const QPointF &p, bool &pressValidated,bool &callParentClass) override;
    void keyPressEvent(QKeyEvent * event, bool &eventTriggerGeneral) override;
    void keyReleaseEvent(QKeyEvent *event, bool &eventTriggerGeneral) override;
    void removeAbove();
    void setVar(ConnexionManager *connexionManager, const uint16_t factoryId);
private slots:
    void newLanguage();
    void on_factoryResources_itemSelectionChanged();
    void on_factoryProducts_itemSelectionChanged();
    void onBuyClicked();
    void onSellClicked();
    void haveFactoryList(const uint32_t &remainingProductionTime, const std::vector<CatchChallenger::ItemToSellOrBuy> &resources, const std::vector<CatchChallenger::ItemToSellOrBuy> &products);
    void haveBuyFactoryObject(const CatchChallenger::BuyStat &stat,const uint32_t &newPrice);
    void haveSellFactoryObject(const CatchChallenger::SoldStat &stat,const uint32_t &newPrice);
private:
    void factoryToResourceItem(QListWidgetItem *item);
    void factoryToProductItem(QListWidgetItem *item);

    ImagesStrechMiddle *wdialog;
    CustomButton *quit;
    CustomText *title;
    QGraphicsPixmapItem label;

    QGraphicsTextItem *resourcesTitle;
    QGraphicsTextItem *productsTitle;

    QListWidget *factoryResources;
    QGraphicsProxyWidget *resourcesProxy;
    QListWidget *factoryProducts;
    QGraphicsProxyWidget *productsProxy;

    ImagesStrechMiddle *descriptionBack;
    QGraphicsTextItem *factoryDescription;
    QGraphicsTextItem *factoryStatus;
    CustomButton *buyButton;
    CustomButton *sellButton;

    ConnexionManager *connexionManager;
    uint16_t m_factoryId;
    bool factoryInProduction;
    CatchChallenger::IndustryStatus industryStatus;
    std::vector<CatchChallenger::ItemToSellOrBuy> itemsToBuy;
    std::vector<CatchChallenger::ItemToSellOrBuy> itemsToSell;
signals:
    void setAbove(ScreenInput *widget);
    void add_to_inventory(const uint16_t &item, const uint32_t &quantity, const bool &showGain);
    void remove_to_inventory(const uint16_t &itemId, const uint32_t &quantity);
    void addCash(const uint32_t &cash);
    void removeCash(const uint32_t &cash);
    void showTip(const std::string &tip);
    void appendReputationRewards(const std::vector<CatchChallenger::ReputationRewards> &reputationList);
};

#endif // Factory_H
