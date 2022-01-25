#ifndef EvolutionControl_H
#define EvolutionControl_H

#include <QObject>
#include "../../../general/base/CommonDatapack.hpp"
#include "../../libcatchchallenger/DatapackClientLoader.hpp"

class EvolutionControl : public QObject
{
    Q_OBJECT
public:
    EvolutionControl(const CatchChallenger::Monster &fromMonsterInformations,const DatapackClientLoader::MonsterExtra &fromMonsterInformationsExtra,const CatchChallenger::Monster &toMonsterInformations,const DatapackClientLoader::MonsterExtra &toMonsterInformationsExtra);
public slots:
    Q_INVOKABLE QString evolutionText();
signals:
    Q_INVOKABLE void cancel();
private:
    const CatchChallenger::Monster &fromMonsterInformations;
    const DatapackClientLoader::MonsterExtra &fromMonsterInformationsExtra;
    const CatchChallenger::Monster &toMonsterInformations;
    const DatapackClientLoader::MonsterExtra &toMonsterInformationsExtra;
};

#endif // EvolutionControl_H
