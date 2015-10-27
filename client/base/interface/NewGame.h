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
    explicit NewGame(const QString &skinPath, const std::vector<uint8_t> &forcedSkin, QWidget *parent = 0);
    ~NewGame();
    bool haveTheInformation();
    QString pseudo();
    QString skin();
    uint32_t skinId();
    bool haveSkin();
    void updateSkin();
private slots:
    void on_ok_clicked();
    void on_pseudo_textChanged(const QString &arg1);
    void on_pseudo_returnPressed();
    void on_nextSkin_clicked();
    void on_previousSkin_clicked();
private:
    std::vector<uint8_t> forcedSkin;
    Ui::NewGame *ui;
    bool ok;
    bool skinLoaded;
    std::vector<std::string> skinList;
    std::vector<uint8_t> skinListId;
    unsigned int currentSkin;
    std::string skinPath;
    bool okCanBeEnabled();
};

#endif // NEWGAME_H
