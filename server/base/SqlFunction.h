#include <QString>

#ifndef CATCHCHALLENGER_SQLFUNCTION_H
#define CATCHCHALLENGER_SQLFUNCTION_H

namespace CatchChallenger {
class SqlFunction
{
public:
    //do a fake login
    static QString quoteSqlVariable(QString variable);
protected:
    static QString text_antislash;
    static QString text_double_antislash;
    static QString text_antislash_quote;
    static QString text_double_antislash_quote;
    static QString text_single_quote;
    static QString text_antislash_single_quote;
};
}

#endif
