#ifndef CATCHCHALLENGER_CHATINPUTGUARD_H
#define CATCHCHALLENGER_CHATINPUTGUARD_H

#include <QObject>
#include <QString>
#include <QTimer>
#include <string>

#include "../../general/base/lib.h"

namespace CatchChallenger {
/// \brief what every client checks before sending a chat line: cleanup, empty,
/// spaces only, too long, same as the previous one, flood (more than 3 lines
/// per 1.5s decay). Slash commands bypass the repeat and flood checks.
class DLL_PUBLIC ChatInputGuard : public QObject
{
    Q_OBJECT
public:
    enum Verdict : uint8_t
    {
        Verdict_Nothing,//nothing to send, leave the input as it is
        Verdict_Refused,//clear the input and show refusal() as system text
        Verdict_Send//clear the input and send text
    };
    ChatInputGuard();
    /// \brief strips \\n \\r \\t from text, then decides; Verdict_Send counts it for the flood check
    Verdict check(QString &text);
    const std::string &refusal() const;
    /// \brief new session: forget the last line and the flood count
    void reset();
    /// \brief one flood decay step, called by the internal 1.5s timer
    void decayFlood();
private:
    QTimer floodTimer;
    int floodCount;
    std::string lastMessageSent;
    std::string refusalText;
};
}

#endif // CATCHCHALLENGER_CHATINPUTGUARD_H
