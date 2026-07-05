#ifndef SCREENTRANSITION_H
#define SCREENTRANSITION_H

#include <QWidget>
#include <QGraphicsView>
#include <QElapsedTimer>
#include <QTimer>
#include <QHash>
#include <QPointF>
#include "background/CCBackground.hpp"
#include "foreground/LoadingScreen.hpp"
#include "ScreenInput.hpp"
#include "../../general/base/GeneralStructures.hpp"

namespace CatchChallenger {
class InternalServer;
}

class OptionsDialog;
class DebugDialog;
class MainScreen;
class Multi;
class Login;
class ConnexionManager;
class SubServer;
class CharacterList;
class ConnexionInfo;
class CCMap;
class OverMapLogic;
class InternalServer;
class LocalListener;
class CustomButton;
class QEventPoint;

class ScreenTransition : public QGraphicsView
{
    Q_OBJECT
public:
    explicit ScreenTransition();
    ~ScreenTransition();
    void setBackground(ScreenInput *widget);
    void setForeground(ScreenInput *widget);
    void setForegroundInGame(ScreenInput *widget);
    void setAbove(ScreenInput *widget);//first plan popup
    //Remember the per-instance QLocalServer automation channel so every (re)created
    //ccmap gets its mapController re-wired to it in goToMap().
    void setRemoteControl(LocalListener *localListener);
protected:
    void resizeEvent(QResizeEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    //multitouch: route each touch point so the on-screen D-pad + A/B can be held
    //together (one finger per pad button) while a second finger still works.
    bool viewportEvent(QEvent *event) override;
    void handleTouchPoint(const QEventPoint &tp);
    //press/release/move dispatch to the above->foreground->background stack, factored
    //so the touch path reuses the exact mouse dispatch without touching the mouse
    //handlers. p is a SCENE point.
    void dispatchPressAt(const QPointF &p);
    void dispatchReleaseAt(const QPointF &p);
    void dispatchMoveAt(const QPointF &p);
    void closeEvent(QCloseEvent *event) override;
protected:
//    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
    void toMainScreen();
    void openOptions();
    void openDebug();
    void openSolo();
    void openMulti();
    void backMain();
    void backSubServer();
    void removeAbove();
    void loadingFinished();
    void toSubServer();
    void toInGame();
    void connectToServer(ConnexionInfo connexionInfo,QString login,QString pass);
    void errorString(std::string error);
    void logged(const std::vector<std::vector<CatchChallenger::CharacterEntry> > &characterEntryList);
    void connectToSubServer(const int indexSubServer);
    void selectCharacter(const int indexSubServer,const int indexCharacter);
    void disconnectedFromServer();
    void toLoading(QString text);
    void goToServerList();
    void goToMap();
    void render();
    void paintEvent(QPaintEvent * event) override;
    void is_started(bool started);

    void updateFPS();
    void setTargetFPS(int targetFPS);
    //SCREENSHOT automation verb: grab the whole rendered viewport (map + any
    //above dialog) to a PNG and answer on the channel. Routed here from the map
    //controller because only this top-level view composes the above-screens.
    void doRemoteScreenshot(const QString &path);
    void keyPressEvent(QKeyEvent * event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void closeWhenOnMapAfterToggle();
signals:
    void newFPSvalue(const unsigned int FPS);
private:
    QTimer closeWhenOnMapAfterTimer_;
    int closeWhenOnMapAfterRemaining_=0;
    CCBackground b;
    LoadingScreen l;
    ScreenInput *m_backgroundStack;
    ScreenInput *m_foregroundStack;
    ScreenInput *m_aboveStack;
    OptionsDialog *o;
    DebugDialog *d;
    MainScreen *m;
    #if defined(CATCHCHALLENGER_SOLO) && defined(CATCHCHALLENGER_SOLO)
    CatchChallenger::InternalServer *internalServer;
    #endif
    #ifndef NOTHREADS
    QThread *threadSolo;
    #endif
    //Solo *solo;
    Multi *multi;
    Login *login;
    ConnexionManager *connexionManager;
    QGraphicsScene *mScene;
    QGraphicsPixmapItem *imageText;
    ScreenInput *mousePress;
    //on-screen pad ownership: which pad button a mouse press or each touch id owns, so
    //the release goes to the SAME button (no cross-cancel between fingers).
    CustomButton *m_mousePadButton;
    QHash<int,CustomButton *> m_touchButton;
    //the single touch id currently driving the generic (non-pad) dispatch (dialog /
    //map / slider are single-touch); -1 when none.
    int m_genericTouchId;
    SubServer *subserver;
    CharacterList *characterList;
    CCMap *ccmap;
    OverMapLogic *overmap;
    //Per-instance automation channel (owned by main()); re-wired to each fresh
    //ccmap->mapController in goToMap(). nullptr when no controller attached.
    LocalListener *remoteControlListener;
    std::vector<std::vector<CatchChallenger::CharacterEntry> > characterEntryList;

    uint8_t waitRenderTime;
    QTimer timerRender;
    QElapsedTimer timeRender;
    uint16_t frameCounter;
    QTimer timerUpdateFPS;
    QElapsedTimer timeUpdateFPS;

    bool multiplaySelected;
    bool inGame;
};

#endif // SCREENTRANSITION_H
