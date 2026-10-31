#ifndef EVOLUTIONSCREEN_H
#define EVOLUTIONSCREEN_H

#include <QElapsedTimer>
#include <QLabel>
#include <QPixmap>
#include <QPushButton>
#include <QTimer>
#include <QWidget>
#include "../../../../general/base/GeneralType.hpp"

//"X is evolving!" blinking between the two monsters, Cancel while it runs, then OK
class EvolutionScreen : public QWidget
{
    Q_OBJECT
public:
    explicit EvolutionScreen(QWidget *parent);
    //canCancel false for an item evolution: the item is already used
    void start(const CATCHCHALLENGER_TYPE_MONSTER &from,const CATCHCHALLENGER_TYPE_MONSTER &to,const bool &canCancel);
signals:
    //the animation went to the end: confirm it to the server; never emitted when the player cancels
    void evolved();
    void closed();
private slots:
    void tick();
    void buttonClicked();
private:
    enum State : uint8_t
    {
        State_Evolving,
        State_Evolved,
        State_Canceled
    };
    static QString monsterName(const CATCHCHALLENGER_TYPE_MONSTER &monster);
    void showEnd(const State &state);

    QLabel *image_;
    QLabel *text_;
    QPushButton *button_;
    QTimer timer_;
    QElapsedTimer elapsed_;
    QPixmap fromImage_;
    QPixmap toImage_;
    QString fromName_;
    QString toName_;
    State state_;
};

#endif // EVOLUTIONSCREEN_H
