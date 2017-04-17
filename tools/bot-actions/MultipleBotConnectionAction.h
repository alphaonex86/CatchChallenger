#ifndef MULTIPLEBOTCONNECTIONACTION_H
#define MULTIPLEBOTCONNECTIONACTION_H

#include "../bot/MultipleBotConnectionImplForGui.h"

#include <string>

class MultipleBotConnectionAction : public MultipleBotConnectionImplForGui
{
public:
    explicit MultipleBotConnectionAction();
    ~MultipleBotConnectionAction();
public:
    std::string getNewPseudo();
private:
    std::vector<std::string> pseudoNotUsed;
    std::unordered_set<std::string> pseudoUsed;
};

#endif // MULTIPLEBOTCONNECTIONACTION_H
