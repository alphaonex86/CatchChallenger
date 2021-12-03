#ifndef PREPAREDSTATEMENTUNIT_H
#define PREPAREDSTATEMENTUNIT_H

#include "DatabaseBase.hpp"
#include "StringWithReplacement.hpp"
#include <string>
#include <vector>
#if defined(CATCHCHALLENGER_DB_PREPAREDSTATEMENT)
#include <unordered_map>
#endif
#if defined(CATCHCHALLENGER_CLASS_QT) || !defined(EPOLLCATCHCHALLENGERSERVER)
#include "QtDatabase.hpp"
#endif

namespace CatchChallenger {
class PreparedStatementUnit
{
public:
    PreparedStatementUnit();
    PreparedStatementUnit(const std::string &query,CatchChallenger::DatabaseBase * const database);
    virtual ~PreparedStatementUnit();

    PreparedStatementUnit(const PreparedStatementUnit& other);// copy constructor
    PreparedStatementUnit(PreparedStatementUnit&& other);// move constructor
    PreparedStatementUnit& operator=(const PreparedStatementUnit& other);// copy assignment
    PreparedStatementUnit& operator=(PreparedStatementUnit&& other);// move assignment

    bool setQuery(const std::string &query);
    virtual DatabaseBase::CallBack * asyncRead(void * returnObject,CallBackDatabase method,const std::vector<std::string> &values);
    virtual bool asyncWrite(const std::vector<std::string> &values);
    bool empty() const;
    std::string queryText() const;
private:
    #ifdef EPOLLCATCHCHALLENGERSERVER
    CatchChallenger::DatabaseBase *database;
    #else
    QtDatabase *database;
    #endif
    #if defined(CATCHCHALLENGER_DB_PREPAREDSTATEMENT)
    //prepared statement
    char uniqueName[3];
    static std::string writeToPrepare(const std::string &query);

    static std::unordered_map<CatchChallenger::DatabaseBase *,uint16_t> queryCount;
    #endif
    StringWithReplacement query;
};
}

#endif // PREPAREDSTATEMENTUNIT_H
