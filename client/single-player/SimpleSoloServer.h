#ifndef SIMPLESOLOSERVER_H
#define SIMPLESOLOSERVER_H

#include <QDialog>
#include "../base/solo/SoloWindow.h"

namespace Ui {
class SimpleSoloServer;
}

class SimpleSoloServer : public QDialog
{
    Q_OBJECT
public:
    explicit SimpleSoloServer(QWidget *parent = 0);
    ~SimpleSoloServer();
public slots:
    void play(const QString &savegamesPath);
    void back();
private:
    Ui::SimpleSoloServer *ui;
    SoloWindow *solowindow;
    CatchChallenger::ConnectedSocket *socket;
};

#endif // SIMPLESOLOSERVER_H
