#ifndef MULTIPLEBOTCONNECTIONACTION_H
#define MULTIPLEBOTCONNECTIONACTION_H

#include "../libbot/MultipleBotConnectionImplForGui.h"

#include <string>

class MultipleBotConnectionAction : public MultipleBotConnectionImplForGui
{
public:
    explicit MultipleBotConnectionAction();
    ~MultipleBotConnectionAction();
public:
    std::string getNewPseudo();
protected:
    /// Widget flavour of the library hook: the modal dialog the GUI bot used to
    /// show from inside tools/libbot.
    virtual void displayCriticalError(const std::string &errorString);
private:
    std::vector<std::string> pseudoNotUsed;
    std::unordered_set<std::string> pseudoUsed;
};

#endif // MULTIPLEBOTCONNECTIONACTION_H
