#ifndef NEWGAME_H
#define NEWGAME_H

#include <QDialog>

namespace Ui {
class NewGame;
}

class NewGame : public QDialog
{
    Q_OBJECT
    
public:
    explicit NewGame(QWidget *parent = 0);
    ~NewGame();
    bool haveTheInformation();
    QString pseudo();
    QString skin();
    bool haveSkin();
    void updateSkin();
    QString gameName();
private slots:
    void on_ok_clicked();
    void on_pseudo_textChanged(const QString &arg1);
    void on_pseudo_returnPressed();
    void on_nextSkin_clicked();
    void on_previousSkin_clicked();
    void on_gameName_textChanged(const QString &arg1);
private:
    Ui::NewGame *ui;
    bool ok;
    bool skinLoaded;
    QList<int> skinNumber;
    int currentSkin;
    QString skinPath;
    bool okCanBeEnabled();
};

#endif // NEWGAME_H
