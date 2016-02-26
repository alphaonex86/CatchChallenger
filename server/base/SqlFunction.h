#ifndef CATCHCHALLENGER_SQLFUNCTION_H
#define CATCHCHALLENGER_SQLFUNCTION_H

#include <string>

namespace CatchChallenger {
class SqlFunction
{
public:
    static std::string quoteSqlVariable(const std::string &variable);
protected:
    static std::string text_antislash;
    static std::string text_double_antislash;
    static std::string text_antislash_quote;
    static std::string text_double_antislash_quote;
    static std::string text_single_quote;
    static std::string text_antislash_single_quote;
};
}

#endif
