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
private slots:
    void on_ok_clicked();
private:
    Ui::NewGame *ui;
    bool ok;
};

#endif // NEWGAME_H
