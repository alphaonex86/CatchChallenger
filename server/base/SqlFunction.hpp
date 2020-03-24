#ifndef CATCHCHALLENGER_SQLFUNCTION_H
#define CATCHCHALLENGER_SQLFUNCTION_H

#include <string>
#include "DatabaseBase.hpp"
#include "../VariableServer.hpp"

namespace CatchChallenger {
class SqlFunction
{
    #if defined(CATCHCHALLENGER_DB_PREPAREDSTATEMENT)
    #else
public:
    static std::string quoteSqlVariable(const std::string &variable);
protected:
    static std::string text_antislash;
    static std::string text_double_antislash;
    static std::string text_antislash_quote;
    static std::string text_double_antislash_quote;
    static std::string text_single_quote;
    static std::string text_antislash_single_quote;
    #endif
    #ifdef DEBUG_MESSAGE_CLIENT_SQL
public:
    static std::string replaceSQLValues(const std::string &query,const std::vector<std::string> &values);
    #endif
};
}

#endif
