#ifndef QUESTJS_H
#define QUESTJS_H

#include <QObject>

class Quest : public QObject
{
    Q_OBJECT
public:
    friend class BaseWindow;
    explicit Quest(const uint32_t &quest,void *baseWindow);
    Q_SCRIPTABLE int currentQuestStep() const;
    Q_SCRIPTABLE int currentBot() const;
    Q_SCRIPTABLE bool finishOneTime() const;
    Q_SCRIPTABLE bool haveQuestStepRequirements() const;
private:
    const uint32_t quest;
    void *baseWindow;
};

#endif // QUESTJS_H
