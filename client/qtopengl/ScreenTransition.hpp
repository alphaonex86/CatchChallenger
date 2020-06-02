#ifndef SCREENTRANSITION_H
#define SCREENTRANSITION_H

#include <QWidget>
#include <QGraphicsView>
#include "background/CCBackground.hpp"
#include "foreground/LoadingScreen.hpp"
#include "ScreenInput.hpp"
#include "../../general/base/GeneralStructures.hpp"

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

class ScreenTransition : public QGraphicsView
{
    Q_OBJECT
public:
    explicit ScreenTransition();
    ~ScreenTransition();
    void setBackground(ScreenInput *widget);
    void setForeground(ScreenInput *widget);
    void setAbove(ScreenInput *widget);//first plan popup
protected:
    void resizeEvent(QResizeEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
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

    void updateFPS();
    void setTargetFPS(int targetFPS);
    void keyPressEvent(QKeyEvent * event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
signals:
    void newFPSvalue(const unsigned int FPS);
private:
    CCBackground b;
    LoadingScreen l;
    ScreenInput *m_backgroundStack;
    ScreenInput *m_foregroundStack;
    ScreenInput *m_aboveStack;
    OptionsDialog *o;
    DebugDialog *d;
    MainScreen *m;
    //Solo *solo;
    Multi *multi;
    Login *login;
    ConnexionManager *connexionManager;
    QGraphicsScene *mScene;
    QGraphicsPixmapItem *imageText;
    ScreenInput *mousePress;
    SubServer *subserver;
    CharacterList *characterList;
    CCMap *ccmap;
    OverMapLogic *overmap;
    std::vector<std::vector<CatchChallenger::CharacterEntry> > characterEntryList;

    uint8_t waitRenderTime;
    QTimer timerRender;
    QTime timeRender;
    uint16_t frameCounter;
    QTimer timerUpdateFPS;
    QTime timeUpdateFPS;
};

#endif // SCREENTRANSITION_H
