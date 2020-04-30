#ifndef SCREENTRANSITION_H
#define SCREENTRANSITION_H

#include <QWidget>
#include "CCBackground.hpp"
#include "LoadingScreen.hpp"
#include "above/OptionsDialog.hpp"
#include "foreground/MainScreen.hpp"
//#include "Solo.h"
#include "foreground/Multi.hpp"
//#include "Login.h"
//#include "interface/BaseWindow.h"
#include "ConnexionManager.hpp"
#include "ScreenInput.hpp"

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
    void openSolo();
    void openMulti();
    void backMain();
    void closeOptions();
    void connectToServer(ConnexionInfo connexionInfo,QString login,QString pass);
    void errorString(std::string error);
    void logged(const std::vector<std::vector<CatchChallenger::CharacterEntry> > &characterEntryList);
    void disconnectedFromServer();
    void toLoading(QString text);
    void goToServerList();
    void goToMap();
    void render();
    void paintEvent(QPaintEvent * event) override;

    void updateFPS();
    void setTargetFPS(int targetFPS);
signals:
    void newFPSvalue(const unsigned int FPS);
private:
    CCBackground b;
    LoadingScreen l;
    ScreenInput *m_backgroundStack;
    ScreenInput *m_foregroundStack;
    ScreenInput *m_aboveStack;
    OptionsDialog *o;
    MainScreen *m;
    //Solo *solo;
    Multi *multi;
    Login *login;
    CatchChallenger::BaseWindow *baseWindow;
    ConnexionManager *connexionManager;
    QGraphicsScene *mScene;
    QGraphicsPixmapItem *imageText;
    ScreenInput *mousePress;

    uint8_t waitRenderTime;
    QTimer timerRender;
    QTime timeRender;
    uint16_t frameCounter;
    QTimer timerUpdateFPS;
    QTime timeUpdateFPS;
};

#endif // SCREENTRANSITION_H
