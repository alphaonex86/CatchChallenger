#include "QtDatabaseSQLite.hpp"

#include <QFile>
#include <QSqlError>
#include <QSqlQuery>
#include <QStringList>
#include <iostream>

using namespace CatchChallenger;

namespace {
// Initialize an empty SQLite database with the schema embedded in the
// `:/catchchallenger-sqlite.sql` Qt resource. Mirrors what the
// qtcpu800x600 / qtopengl SINGLEPLAYER clients do via SoloDatabaseInit
// — read the .sql blob, split on `;`, exec each non-comment statement.
// Called from syncConnect() on the FIRST connection only (when
// `sqlite_master` has zero user tables); subsequent connections reuse
// the populated file. If the resource is missing (server target was
// built without resources-db-sqlite.qrc linked) the function returns
// false and syncConnect logs the error so the operator sees the missing-
// resource case immediately instead of hitting "no such table" at
// runtime.
bool ensureSqliteSchema(QSqlDatabase &db, std::string &lastErrorOut)
{
    {
        QSqlQuery probe(db);
        if(!probe.exec(QStringLiteral(
                "SELECT name FROM sqlite_master "
                "WHERE type='table' AND name NOT LIKE 'sqlite_%' "
                "LIMIT 1")))
        {
            lastErrorOut="ensureSqliteSchema: sqlite_master probe failed: "
                          +probe.lastError().text().toStdString();
            return false;
        }
        if(probe.next())
            //schema already populated — nothing to do
            return true;
    }
    QFile sqlRes(QStringLiteral(":/catchchallenger-sqlite.sql"));
    if(!sqlRes.open(QIODevice::ReadOnly|QIODevice::Text))
    {
        lastErrorOut="ensureSqliteSchema: :/catchchallenger-sqlite.sql "
                     "resource missing (link resources-db-sqlite.qrc into "
                     "the target)";
        return false;
    }
    const QString sqlBlob=QString::fromUtf8(sqlRes.readAll());
    sqlRes.close();
    //SQLite + Qt's QSqlQuery rejects multi-statement queries — split on
    //the trailing `;` of each statement and exec one at a time. The
    //schema dump uses ASCII semicolons; nothing inside string literals,
    //so a naive split is safe here.
    const QStringList statements=sqlBlob.split(QChar(';'),Qt::SkipEmptyParts);
    int si=0;
    while(si<statements.size())
    {
        const QString stmt=statements.at(si).trimmed();
        si++;
        if(stmt.isEmpty())
            continue;
        if(stmt.startsWith(QStringLiteral("--")))
            continue;
        QSqlQuery q(db);
        if(!q.exec(stmt))
        {
            lastErrorOut="ensureSqliteSchema: stmt failed ("
                          +q.lastError().text().toStdString()
                          +"): "+stmt.toStdString();
            return false;
        }
    }
    return true;
}
}//namespace

QtDatabaseSQLite::QtDatabaseSQLite()
{
    setObjectName("QtDatabaseSQLite");
}

QtDatabaseSQLite::~QtDatabaseSQLite()
{
}

bool QtDatabaseSQLite::syncConnect(const std::string &host, const std::string &dbname, const std::string &user,const std::string &password)
{
    const std::string &file=host;
    (void)dbname;
    (void)user;
    (void)password;
    if(conn!=NULL)
        syncDisconnect();
    conn=findConnexionToOpen(file,std::string(),DatabaseBase::DatabaseType::SQLite);
    if(conn!=NULL)
    {
        databaseTypeVar=DatabaseBase::DatabaseType::SQLite;
        return true;
    }
    conn = new QSqlDatabase();
    *conn = QSqlDatabase::addDatabase("QSQLITE","server"+QString::number(QtDatabase::establishedConnexionCount));
    conn->setDatabaseName(file.c_str());
    if(!conn->open())
    {
        lastErrorMessage=(conn->lastError().driverText()+": "+conn->lastError().databaseText()).toStdString();
        delete conn;
        conn=NULL;
        return false;
    }
    //Auto-initialise the schema when QSQLITE just created an empty file.
    //QSQLITE creates the file on open() if missing, so a fresh deploy
    //(or a wiped test run) lands here with zero user tables. Mirrors the
    //qtcpu800x600 / qtopengl SINGLEPLAYER behaviour where SoloDatabaseInit
    //creates the savegame on first launch — same .sql resource is reused
    //here, linked in via resources-db-sqlite.qrc on the consuming target.
    if(!ensureSqliteSchema(*conn,lastErrorMessage))
    {
        std::cerr << "QtDatabaseSQLite: " << lastErrorMessage << std::endl;
        conn->close();
        delete conn;
        conn=NULL;
        return false;
    }
    databaseTypeVar=DatabaseBase::DatabaseType::SQLite;

    QtDatabase::EstablishedConnexion establishedConnexion;
    establishedConnexion.conn=conn;
    establishedConnexion.host=file;
    establishedConnexion.openCount=1;
    establishedConnexion.type=DatabaseBase::DatabaseType::SQLite;
    QtDatabase::establishedConnexionList.push_back(establishedConnexion);
    QtDatabase::establishedConnexionCount++;

    return true;
}
