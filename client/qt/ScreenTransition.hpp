#ifndef SCREENTRANSITION_H
#define SCREENTRANSITION_H

#include <QWidget>
#include <QStackedWidget>
#include "../qt/CCBackground.h"
#include "../qt/LoadingScreen.h"
#include "OptionsDialog.h"
#include "MainScreen.h"
#include "Solo.h"
#include "Multi.h"
#include "Login.h"
#include "interface/BaseWindow.h"
#include "ConnexionManager.h"

class ScreenTransition : public QWidget
{
    Q_OBJECT
public:
    explicit ScreenTransition();
    ~ScreenTransition();
    void setBackground(QWidget *widget);
    void setForeground(QWidget *widget);
    void setAbove(QWidget *widget);//first plan popup
protected:
    void paintEvent(QPaintEvent *) override;
    void resizeEvent(QResizeEvent *) override;
    void toMainScreen();
    void openOptions();
    void openSolo();
    void openMulti();
    void backMain();
    void closeOptions();
    void connectToServer(Multi::ConnexionInfo connexionInfo,QString login,QString pass);
    void errorString(std::string error);
    void logged(const std::vector<std::vector<CatchChallenger::CharacterEntry> > &characterEntryList);
    void disconnectedFromServer();
    void toLoading(QString text);
    void goToServerList();
    void goToMap();
private:
    CCBackground b;
    LoadingScreen l;
    QStackedWidget *m_backgroundStack;
    QStackedWidget *m_foregroundStack;
    QStackedWidget *m_aboveStack;
    OptionsDialog *o;
    MainScreen *m;
    Solo *solo;
    Multi *multi;
    Login *login;
    CatchChallenger::BaseWindow *baseWindow;
    ConnexionManager *connexionManager;
};

#endif // SCREENTRANSITION_H
