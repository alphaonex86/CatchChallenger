#ifndef CATCHCHALLENGER_SQLFUNCTION_H
#define CATCHCHALLENGER_SQLFUNCTION_H

#include <string>

namespace CatchChallenger {
class SqlFunction
{
public:
    //do a fake login
    static std::basic_string<char> quoteSqlVariable(std::basic_string<char> variable);
protected:
    static std::basic_string<char> text_antislash;
    static std::basic_string<char> text_double_antislash;
    static std::basic_string<char> text_antislash_quote;
    static std::basic_string<char> text_double_antislash_quote;
    static std::basic_string<char> text_single_quote;
    static std::basic_string<char> text_antislash_single_quote;
};
}

#endif
