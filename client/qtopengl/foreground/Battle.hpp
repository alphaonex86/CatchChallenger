#ifndef Battle_H
#define Battle_H

#include "../ScreenInput.hpp"
#include "../ConnexionManager.hpp"
#include <QObject>
#include <QTimer>
#include <QElapsedTimer>
class CCprogressbar;
class ImagesStrechMiddle;
class CustomButton;
class QGraphicsProxyWidget;
class QListWidget;
class QListWidgetItem;

class Battle : public ScreenInput
{
    Q_OBJECT
public:
    enum BattleType
    {
        BattleType_Wild,
        BattleType_Bot,
        BattleType_OtherPlayer
    };
    enum BattleStep
    {
        BattleStep_Presentation,
        BattleStep_PresentationMonster,
        BattleStep_Middle,
        BattleStep_Final
    };
    enum MoveType
    {
        MoveType_Enter,
        MoveType_Leave,
        MoveType_Dead
    };
    enum DoNextActionStep
    {
        DoNextActionStep_Start,
        DoNextActionStep_Loose,
        DoNextActionStep_Win
    };

    explicit Battle();
    ~Battle();
    static Battle *battle;
    void newLanguage();
    QRectF boundingRect() const;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;
    void mousePressEventXY(const QPointF &p,bool &pressValidated,bool &callParentClass) override;
    void mouseReleaseEventXY(const QPointF &p, bool &pressValidated,bool &callParentClass) override;
    void stackedWidgetFightBottomBarsetCurrentWidget(int index);

    void on_monsterList_itemActivated(uint8_t monsterPosition);
    bool check_monsters();
    void init_environement_display(const CATCHCHALLENGER_TYPE_MAPID &mapIndex, const COORD_TYPE &x, const COORD_TYPE &y);
    void setVar(ConnexionManager *connexionManager);
    void init_current_monster_display(CatchChallenger::PlayerMonster *fightMonster=nullptr);
    void updateOtherMonsterInformation();
    void updateCurrentMonsterInformation();
    void updateAttackList();
    void resetPosition(bool both=true, bool top=true, bool bottom=true);

    void startWildBattle(const CATCHCHALLENGER_TYPE_MAPID &mapIndex, const COORD_TYPE &x, const COORD_TYPE &y);
    void startBotBattle(const CATCHCHALLENGER_TYPE_MAPID &mapIndex, const COORD_TYPE &x, const COORD_TYPE &y);

    BattleType battleType;
    BattleStep battleStep;
public slots:
    void on_pushButtonFightEnterNext_clicked();
    void on_pushButtonFightAttack_clicked();
    void on_pushButtonFightBag_clicked();
    void on_pushButtonFightMonster_clicked();
    void on_toolButtonFightQuit_clicked();
    void on_pushButtonFightAttackConfirmed_clicked();
    void on_pushButtonFightReturn_clicked();
    void on_listWidgetFightAttack_itemSelectionChanged();
    void moveFightMonsterBottom();
    void moveFightMonsterTop();
    void moveFightMonsterBoth();
    void doNextAction();
    void win();
    void loose();
    void sendBattleReturn(const std::vector<CatchChallenger::Skill::AttackReturn> &attackReturn);
signals:
    void battleWin();
    void battleLose();
    void error(const std::string &error);
    void openBagForBattle();
    void openMonsterListForBattle();
    void setForeground(ScreenInput *widget);
private:
    //place an arena element: design center (cx,cy) in the 800x440 space scaled by
    //(ax,ay), art scaled uniformly by k (anchor-responsive: positions stretch,
    //sprites keep their aspect). Rescales the pixmap only when the target size
    //changed; set a null pixmap on the item to force a refresh from src.
    void placeArenaPixmap(QGraphicsPixmapItem *item,const QPixmap &src,
                          const qreal &cx,const qreal &cy,const qreal &dw,const qreal &dh,
                          const qreal &ax,const qreal &ay,const qreal &k);
    void layoutInfoFrame(ImagesStrechMiddle *frame,QGraphicsTextItem *name,QGraphicsTextItem *level,
                         CCprogressbar *hp,CCprogressbar *exp,const qreal &k);

    ImagesStrechMiddle *frameFightBottom;
    QList<QGraphicsPixmapItem *> bottomBuff;
    QGraphicsTextItem *labelFightBottomName;
    QGraphicsTextItem *labelFightBottomLevel;
    CCprogressbar *progressBarFightBottomHP;
    CCprogressbar *progressBarFightBottomExp;

    ImagesStrechMiddle *frameFightTop;
    QList<QGraphicsPixmapItem *> topBuff;
    QGraphicsTextItem *labelFightTopName;
    QGraphicsTextItem *labelFightTopLevel;
    CCprogressbar *progressBarFightTopHP;

    QGraphicsPixmapItem *labelFightBackground;
    QPixmap labelFightBackgroundPix;
    QGraphicsPixmapItem *labelFightForeground;
    QPixmap labelFightForegroundPix;
    QGraphicsPixmapItem *labelFightPlateformBottom;
    QPixmap labelFightPlateformBottomPix;
    QGraphicsPixmapItem *labelFightPlateformTop;
    QPixmap labelFightPlateformTopPix;

    QGraphicsPixmapItem *labelFightMonsterAttackBottom;
    QGraphicsPixmapItem *labelFightMonsterAttackTop;
    QGraphicsPixmapItem *labelFightMonsterBottom;
    QGraphicsPixmapItem *labelFightMonsterTop;
    QGraphicsPixmapItem *labelFightTrap;
    //unscaled monster sprites; paint() scales them to the current screen
    QPixmap monsterBottomPixSrc;
    QPixmap monsterTopPixSrc;
    //monster positions in the 800x440 design space (animated by the move timers,
    //mapped to screen coordinates by paint())
    QPointF monsterBottomDesignPos;
    QPointF monsterTopDesignPos;

    ImagesStrechMiddle *stackedWidgetFightBottomBar;

    QGraphicsTextItem *labelFightEnter;
    CustomButton *pushButtonFightEnterNext;

    CustomButton *pushButtonFightAttack;
    CustomButton *pushButtonFightBag;
    CustomButton *pushButtonFightMonster;
    CustomButton *toolButtonFightQuit;

    CustomButton *pushButtonFightAttackConfirmed;
    CustomButton *pushButtonFightReturn;
    QGraphicsTextItem *labelFightAttackDetails;
    QGraphicsProxyWidget *listWidgetFightAttackProxy;
    QListWidget *listWidgetFightAttack;

    int m_stackedWidgetFightBottomBarIndex=0;

    bool escape;
    bool escapeSuccess;
    ConnexionManager *connexionManager;

    MoveType moveType;
    DoNextActionStep doNextActionStep;
    QTimer moveFightMonsterBottomTimer;
    QTimer moveFightMonsterTopTimer;
    QTimer moveFightMonsterBothTimer;

    std::vector<CatchChallenger::Skill::AttackReturn> attackReturnList;
    std::unordered_map<QListWidgetItem*,uint16_t> fight_attacks_graphical;
};

#endif // Battle_H
