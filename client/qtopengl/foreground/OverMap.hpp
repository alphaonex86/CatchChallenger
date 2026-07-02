#ifndef OverMap_H
#define OverMap_H

#include <QObject>
#include <QTimer>
#include "../ScreenInput.hpp"
#include "../CustomButton.hpp"
#include "../../../general/base/GeneralStructures.hpp"

class MapMonsterPreview;
class ImagesStrechMiddle;
class LineEdit;
class ComboBox;
class CustomText;
class ConnexionManager;
class CCMap;
class QGraphicsPixmapItemClick;
class MonsterDetails;

class OverMap : public ScreenInput
{
    Q_OBJECT
public:
    enum ActionClan
    {
        ActionClan_Create,
        ActionClan_Leave,
        ActionClan_Dissolve,
        ActionClan_Invite,
        ActionClan_Eject
    };

    explicit OverMap();
    ~OverMap();
    virtual void resetAll();
    virtual void setVar(CCMap *ccmap, ConnexionManager *connexionManager);
    void newLanguage();
    QRectF boundingRect() const;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;
    void mousePressEventXY(const QPointF &p,bool &pressValidated,bool &callParentClass) override;
    void mouseReleaseEventXY(const QPointF &p, bool &pressValidated,bool &callParentClass) override;
    void mouseMoveEventXY(const QPointF &p,bool &pressValidated/*if true then don't do action*/,bool &callParentClass);
    void keyPressEvent(QKeyEvent * event, bool &eventTriggerGeneral) override;
    void keyReleaseEvent(QKeyEvent *event, bool &eventTriggerGeneral) override;
    void lineEdit_chat_text_returnPressed();
    void updateShowChat();
    void buyClicked();
    //on-screen D-pad + A/B: pressed/released of any pad button routes here and is
    //turned into a synthetic key press/release fed to the map (same held-key path as
    //a real keyboard). sender() identifies which button -> which Qt::Key.
    void padButtonPressed();
    void padButtonReleased();
    //hit-test the 6 pad buttons for a scene point (nullptr if none / pad inactive);
    //used by ScreenTransition's multitouch dispatch to own a touch point per button.
    CustomButton *padButtonAt(const QPointF &scenePoint);
    //whether the on-screen pad is active: Settings "touchControls" (0=auto,1=on,2=off)
    //with auto => on for android/touch/small-screen. Also gates map click-to-move.
    //CACHED after the first resolution: paint() calls this EVERY FRAME, and the raw
    //resolve (QSettings + QInputDevice::devices() + platform string) is too costly for
    //the render loop (blew the map benchmark budget). OptionsDialog invalidates the
    //cache when the option changes, so it still applies without a restart.
    static bool touchControlsEnabled();
    static void invalidateTouchControlsCache();
    virtual void IG_dialog_close();
    void comboBox_chat_type_currentIndexChanged(int index);
    void new_system_text(const CatchChallenger::Chat_type &chat_type, const std::string &text);
    void new_chat_text(CatchChallenger::Chat_type chat_type,std::string text,std::string pseudo,CatchChallenger::Player_type player_type);
    void removeNumberForFlood();

    void openMonster();
    void updatePlayerNumber(const uint16_t &number,const uint16_t &max_players);
    void have_current_player_info(const CatchChallenger::Player_private_and_public_informations &informations);

    //to call from chat to do test
    virtual void add_to_inventory(const std::vector<std::pair<uint16_t,uint32_t> > &items,const bool &showGain) = 0;
    virtual void showTip(const std::string &tip) = 0;
    virtual void showPlace(const std::string &place) = 0;
protected:
    QTimer stopFlood;
    int numberForFlood;
    std::string lastMessageSend;
    ConnexionManager *connexionManager;
    MonsterDetails *monsterDetails;

    QGraphicsPixmapItemClick *playerBackground;
    bool playerBackgroundBig;
    QGraphicsPixmapItem *playerImage;
    QGraphicsTextItem *name;
    QGraphicsTextItem *cash;
    QList<MapMonsterPreview *> monsters;
    QPointF m_startPress;
    bool wasDragged;
    MapMonsterPreview * monstersDragged;

    QGraphicsPixmapItem *playersCountBack;
    QGraphicsTextItem *playersCount;

    ImagesStrechMiddle *chatBack;
    //chat log: a plain QGraphicsTextItem (pure scene rendering — works on every
    //platform incl. Android/OpenGL ES). setTextWidth() word-wraps it to the chat
    //box width so server text never spills past the right edge.
    QGraphicsTextItem *chatText;
    LineEdit *chatInput;
    ComboBox *chatType;
    CustomButton *chat;
    CustomText *chatOver;

    CustomButton *bag;
    CustomText *bagOver;
    CustomButton *opentolan;
    CustomText *opentolanOver;
    CustomButton *buy;
    CustomText *buyOver;

    //on-screen touch controls (shown only when touchControlsActive). D-pad cross to
    //the LEFT of chat; A/B to the RIGHT of buy. Held to walk; A=action, B=cancel.
    CCMap *ccmap;
    bool touchControlsActive;
    //touchControlsEnabled() cache: -1 unresolved, else 0/1 (see its comment)
    static int touchControlsCachedValue;
    CustomButton *dpadUp;
    CustomButton *dpadDown;
    CustomButton *dpadLeft;
    CustomButton *dpadRight;
    CustomButton *btnA;
    CustomButton *btnB;
    int padKeyForSender();
    void sendKeyToMap(int key,bool pressed);

    ImagesStrechMiddle *gainBack;
    QGraphicsTextItem *gain;
    QString gainString;

    ImagesStrechMiddle *IG_dialog_textBack;
    QGraphicsTextItem *IG_dialog_name;
    //Sign/NPC dialog body: a plain QGraphicsTextItem (pure scene rendering — works
    //on every platform incl. Android/OpenGL ES). setTextWidth() word-wraps it to
    //the dialog width (bounded by the window), so server text never spills out the
    //side; the dialog box height grows to fit the wrapped text, capped to the window.
    QGraphicsTextItem *IG_dialog_text;
    CustomButton *IG_dialog_quit;
    QString IG_dialog_nameString;
    QString IG_dialog_textString;
    QGraphicsPixmapItem *labelSlow;

    ImagesStrechMiddle *tipBack;
    QGraphicsTextItem *tip;
    QString tipString;

    ImagesStrechMiddle *persistant_tipBack;
    QGraphicsTextItem *persistant_tip;
    QString persistant_tipString;

    std::vector<ActionClan> actionClan;
    std::string clanName;
    bool haveClanInformations;
private:
    void updateChat();
};

#endif // MULTI_H
