#ifndef QUESTJS_H
#define QUESTJS_H

#include <QObject>

class Quest : public QObject
{
    Q_OBJECT
public:
    friend class BaseWindow;
    explicit Quest(const quint32 &quest);
    Q_SCRIPTABLE int currentQuestStep() const;
    Q_SCRIPTABLE int currentBot() const;
    Q_SCRIPTABLE bool finishOneTime() const;
    Q_SCRIPTABLE bool haveQuestStepRequirements() const;
private:
    const quint32 quest;
};

#endif // QUESTJS_H
