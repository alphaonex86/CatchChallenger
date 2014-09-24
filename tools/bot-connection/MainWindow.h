#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QObject>
#include <QTimer>
#include <QList>
#include <QSemaphore>
#include <QByteArray>
#include <QSettings>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "../bot/MultipleBotConnectionImplFoprGui.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
private:
    QSettings settings;
    QTimer slowDownTimer;

    MultipleBotConnectionImplFoprGui multipleBotConnexion;
public slots:
    void detectSlowDown(QString text);
private slots:
    void lastReplyTime(const quint32 &time);
    void on_connect_clicked();
    void on_characterSelect_clicked();
    void logged(const QList<CatchChallenger::CharacterEntry> &characterEntryList,bool haveTheDatapack);
    void statusError(QString error);
    void display_numberOfBotConnected(quint16 numberOfBotConnected);
    void display_numberOfSelectedCharacter(quint16 numberOfSelectedCharacter);
    void on_move_toggled(bool checked);
    void on_randomText_toggled(bool checked);
    void on_chatRandomReply_toggled(bool checked);
    void on_bugInDirection_toggled(bool checked);
signals:
    void isDisconnected();
private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
