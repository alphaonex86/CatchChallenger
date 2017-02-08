#ifndef SOCIALCHAT_H
#define SOCIALCHAT_H

#include <QMainWindow>
#include "../bot/actions/ActionsAction.h"

namespace Ui {
class SocialChat;
}

class SocialChat : public QMainWindow
{
    Q_OBJECT

public:
    explicit SocialChat();
    ~SocialChat();
    static SocialChat *socialChat;
private slots:
    void on_bots_itemSelectionChanged();

private:
    void showEvent(QShowEvent * event);
    Ui::SocialChat *ui;
    QHash<QString,ActionsBotInterface::Player *> pseudoToBot;
};

#endif // SOCIALCHAT_H
