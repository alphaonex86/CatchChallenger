#ifndef FightEngine_H
#define FightEngine_H

#include <QObject>
#include <QByteArray>

class FightEngine : public QObject
{
    Q_OBJECT
public:
    explicit FightEngine();
    ~FightEngine();
    void resetAll();
    const QByteArray randomSeeds();
    bool canDoRandomStep();
    bool canDoRandomFight();
    enum landFight
    {
        landFight_Water,
        landFight_Grass,
        landFight_Cave
    };
private:
    QByteArray m_randomSeeds;
public slots:
    //fight
    void appendRandomSeeds(const QByteArray &data);
};

#endif // FightEngine_H
