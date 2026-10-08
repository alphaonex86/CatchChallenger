#if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
#ifndef PREPAREDSTATEMENTUNIT_H
#define PREPAREDSTATEMENTUNIT_H

#include "DatabaseBase.hpp"
#include "StringWithReplacement.hpp"
#include <string>
#include <vector>
#if defined(CATCHCHALLENGER_DB_PREPAREDSTATEMENT)
#include <unordered_map>
#include <limits>
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
    virtual DatabaseBaseCallBack * asyncRead(void * returnObject,CallBackDatabase method,const std::vector<std::string> &values);
    virtual bool asyncWrite(const std::vector<std::string> &values);
    bool empty() const;
    std::string queryText() const;
private:
    //convert asyncPrepared to async, then need database
    CatchChallenger::DatabaseBase * database;
    #if defined(CATCHCHALLENGER_DB_PREPAREDSTATEMENT)
    //prepared statement name = the per-connection counter in decimal: sized from the
    //counter type at compile time, so every value fits (max digits + NUL)
    typedef uint16_t StatementNumber;
    char uniqueName[std::numeric_limits<StatementNumber>::digits10+2];
    static std::string writeToPrepare(const std::string &query);

    static std::unordered_map<CatchChallenger::DatabaseBase *,StatementNumber> queryCount;
    #endif
    StringWithReplacement query;
};
}

#endif // PREPAREDSTATEMENTUNIT_H
#elif CATCHCHALLENGER_DB_BLACKHOLE
#elif CATCHCHALLENGER_DB_FILE
#else
#error Define what do here
#endif
