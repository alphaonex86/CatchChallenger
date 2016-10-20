#ifndef PREPAREDSTATEMENTUNIT_H
#define PREPAREDSTATEMENTUNIT_H

#include "DatabaseBase.h"
#include "StringWithReplacement.h"
#include <string>
#include <vector>

namespace CatchChallenger {
class PreparedStatementUnit
{
public:
    PreparedStatementUnit();
    PreparedStatementUnit(const std::string &query,CatchChallenger::DatabaseBase * const database);

    PreparedStatementUnit(const PreparedStatementUnit& other);// copy constructor
    PreparedStatementUnit(PreparedStatementUnit&& other);// move constructor
    PreparedStatementUnit& operator=(const PreparedStatementUnit& other);// copy assignment
    PreparedStatementUnit& operator=(PreparedStatementUnit&& other);// move assignment

    bool setQuery(const std::string &query);
    virtual DatabaseBase::CallBack * asyncRead(void * returnObject,CallBackDatabase method,const std::vector<std::string> &values);
    virtual bool asyncWrite(const std::vector<std::string> &values);
    bool empty() const;
    const std::string &queryText() const;
private:
    #ifdef EPOLLCATCHCHALLENGERSERVER
    CatchChallenger::DatabaseBase *database;
    #else
    QtDatabase *database;
    #endif
    #if defined(CATCHCHALLENGER_DB_POSTGRESQL) && defined(EPOLLCATCHCHALLENGERSERVER)
    //prepared statement
    char uniqueName[3];
    static uint16_t queryCount;
    #endif
    StringWithReplacement query;
};
}

#endif // PREPAREDSTATEMENTUNIT_H
