#ifndef Trade_H
#define Trade_H

#include <QObject>
#include <QListWidget>
#include "../ScreenInput.hpp"
#include "../ImagesStrechMiddle.hpp"
#include "../CustomButton.hpp"
#include "../CustomText.hpp"
#include "../SpinBox.hpp"
#include "../../../general/base/GeneralStructures.hpp"

class ConnexionManager;

class Trade : public ScreenInput
{
    Q_OBJECT
public:
    explicit Trade();
    ~Trade();

    enum TradeOtherStat
    {
        TradeOtherStat_InWait,
        TradeOtherStat_Accepted
    };

    QRectF boundingRect() const override;
    void paint(QPainter *p, const QStyleOptionGraphicsItem *, QWidget *widget = nullptr) override;
    void mousePressEventXY(const QPointF &p,bool &pressValidated,bool &callParentClass) override;
    void mouseReleaseEventXY(const QPointF &p, bool &pressValidated,bool &callParentClass) override;
    void keyPressEvent(QKeyEvent * event, bool &eventTriggerGeneral) override;
    void keyReleaseEvent(QKeyEvent *event, bool &eventTriggerGeneral) override;

    void setVar(ConnexionManager *connexionManager, const std::string &otherPseudo, const uint8_t &otherSkinId);

    // called by OverMapLogic when other player actions arrive
    void tradeCanceledByOther();
    void tradeFinishedByOther();
    void tradeValidatedByTheServer();
    void tradeAddTradeCash(const uint64_t &cash);
    void tradeAddTradeObject(const uint16_t &item, const uint32_t &quantity);
    void tradeAddTradeMonster(const CatchChallenger::PlayerMonster &monster);
private slots:
    void newLanguage();
    void onAddItemClicked();
    void onAddMonsterClicked();
    void onValidateClicked();
    void onCancelClicked();
private:
    void updateCurrentItemsDisplay();
    void updateOtherItemsDisplay();

    ImagesStrechMiddle *wdialog;
    CustomText *title;
    QGraphicsPixmapItem label;

    // player side
    QGraphicsTextItem *playerPseudo;
    QGraphicsPixmapItem *playerImage;
    QListWidget *playerItemsList;
    QGraphicsProxyWidget *playerItemsProxy;
    QGraphicsTextItem *playerCashLabel;
    SpinBox *playerCash;
    CustomButton *addItemBtn;
    CustomButton *addMonsterBtn;

    // other player side
    QGraphicsTextItem *otherPseudo;
    QGraphicsPixmapItem *otherImage;
    QListWidget *otherItemsList;
    QGraphicsProxyWidget *otherItemsProxy;
    QGraphicsTextItem *otherCashLabel;
    QGraphicsTextItem *otherStatusText;

    CustomButton *validateBtn;
    CustomButton *cancelBtn;

    ConnexionManager *connexionManager;
    TradeOtherStat tradeOtherStat;
    std::unordered_map<uint16_t,uint32_t> tradeCurrentObjects;
    std::unordered_map<uint16_t,uint32_t> tradeOtherObjects;
    std::vector<CatchChallenger::PlayerMonster> tradeCurrentMonsters;
    std::vector<CatchChallenger::PlayerMonster> tradeOtherMonsters;
    std::vector<uint8_t> tradeCurrentMonstersPosition;
signals:
    void setAbove(ScreenInput *widget);
    void add_to_inventory(const uint16_t &item, const uint32_t &quantity, const bool &showGain);
    void remove_to_inventory(const uint16_t &itemId, const uint32_t &quantity);
    void addCash(const uint32_t &cash);
    void removeCash(const uint32_t &cash);
    void showTip(const std::string &tip);
    void tradeClosed();
    void selectObjectForTrade();
    void selectMonsterForTrade();
};

#endif // Trade_H
