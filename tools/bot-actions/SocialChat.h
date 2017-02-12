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
    void loadPlayerInformation();
    QPixmap getFrontSkin(const QString &skinName);
    QPixmap getFrontSkin(const uint32_t &skinId);
    QString getFrontSkinPath(const QString &skin);
    QPixmap getBackSkin(const QString &skinName);
    QPixmap getBackSkin(const uint32_t &skinId);
    QString getBackSkinPath(const QString &skin);
    QPixmap getTrainerSkin(const QString &skinName);
    QPixmap getTrainerSkin(const uint32_t &skinId);
    QString getTrainerSkinPath(const QString &skin);
    QString getSkinPath(const QString &skinName,const QString &type);
private:
    void showEvent(QShowEvent * event);
    Ui::SocialChat *ui;
    QHash<QString,ActionsBotInterface::Player *> pseudoToBot;

    //skin
    QHash<QString,QPixmap> frontSkinCacheString;
    QHash<QString,QPixmap> backSkinCacheString;
    QHash<QString,QPixmap> trainerSkinCacheString;
};

#endif // SOCIALCHAT_H
