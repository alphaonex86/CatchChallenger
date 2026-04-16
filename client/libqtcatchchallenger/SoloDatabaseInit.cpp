#include "SoloDatabaseInit.hpp"

#include <QFile>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QUuid>
#include <xxhash.h>

const QString SoloDatabaseInit::sqlResourcePath = QStringLiteral(":/catchchallenger-sqlite.sql");
const QString SoloDatabaseInit::checksumFileSuffix = QStringLiteral(".xxh32");

QByteArray SoloDatabaseInit::readSqlResource()
{
    QFile f(sqlResourcePath);
    if(!f.open(QIODevice::ReadOnly))
        return QByteArray();
    return f.readAll();
}

QString SoloDatabaseInit::computeChecksum(const QByteArray &data)
{
    XXH32_canonical_t htemp;
    XXH32_canonicalFromHash(&htemp, XXH32(data.constData(), data.size(), 0));
    return QString::fromLatin1(QByteArray(reinterpret_cast<const char *>(htemp.digest),
                                          sizeof(htemp.digest)).toHex());
}

const QString &SoloDatabaseInit::cachedSqlChecksum()
{
    static QString cached;
    static bool computed = false;
    if(!computed)
    {
        const QByteArray data = readSqlResource();
        if(!data.isEmpty())
            cached = computeChecksum(data);
        computed = true;
    }
    return cached;
}

bool SoloDatabaseInit::createSavegame(const QString &dbPath, QString *errorMsg)
{
    const QByteArray sqlBytes = readSqlResource();
    if(sqlBytes.isEmpty())
    {
        if(errorMsg) *errorMsg = QStringLiteral("Embedded SQL resource is missing or empty");
        return false;
    }
    // Ensure we start from a clean file so QSQLITE creates a new database.
    if(QFile::exists(dbPath))
        QFile::remove(dbPath);

    const QString connName = QStringLiteral("savegameinit_") + QUuid::createUuid().toString(QUuid::WithoutBraces);
    bool ok = false;
    {
        QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connName);
        db.setDatabaseName(dbPath);
        if(!db.open())
        {
            if(errorMsg) *errorMsg = QStringLiteral("Unable to open savegame db: ") + db.lastError().text();
        }
        else
        {
            // Split on ';' — the embedded SQL has no semicolons inside string literals.
            const QString sql = QString::fromUtf8(sqlBytes);
            QStringList statements;
            int start = 0;
            for(int i = 0; i < sql.size(); ++i)
            {
                if(sql.at(i) == QLatin1Char(';'))
                {
                    statements.append(sql.mid(start, i - start));
                    start = i + 1;
                }
            }
            if(start < sql.size())
                statements.append(sql.mid(start));

            ok = true;
            for(const QString &raw : statements)
            {
                const QString stmt = raw.trimmed();
                if(stmt.isEmpty())
                    continue;
                const QString upper = stmt.toUpper();
                if(upper == QStringLiteral("BEGIN") || upper == QStringLiteral("BEGIN TRANSACTION")
                        || upper == QStringLiteral("COMMIT") || upper == QStringLiteral("END")
                        || upper == QStringLiteral("END TRANSACTION"))
                    continue;
                QSqlQuery q(db);
                if(!q.exec(stmt))
                {
                    if(errorMsg) *errorMsg = QStringLiteral("SQL error: ") + q.lastError().text()
                            + QStringLiteral(" on: ") + stmt.left(80);
                    ok = false;
                    break;
                }
            }
            db.close();
        }
    }
    QSqlDatabase::removeDatabase(connName);

    if(!ok)
    {
        QFile::remove(dbPath);
        return false;
    }

    QFile checksumOut(dbPath + checksumFileSuffix);
    if(!checksumOut.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        if(errorMsg) *errorMsg = QStringLiteral("Unable to write checksum file: ") + checksumOut.fileName();
        QFile::remove(dbPath);
        return false;
    }
    const QString sum = cachedSqlChecksum();
    if(checksumOut.write(sum.toLatin1()) < 0)
    {
        if(errorMsg) *errorMsg = QStringLiteral("Unable to write checksum file: ") + checksumOut.fileName();
        checksumOut.close();
        QFile::remove(checksumOut.fileName());
        QFile::remove(dbPath);
        return false;
    }
    checksumOut.close();
    return true;
}

bool SoloDatabaseInit::isSavegameValid(const QString &dbPath)
{
    const QString expected = cachedSqlChecksum();
    if(expected.isEmpty())
        return false;
    QFile f(dbPath + checksumFileSuffix);
    if(!f.exists() || !f.open(QIODevice::ReadOnly))
        return false;
    const QByteArray stored = f.readAll().trimmed();
    f.close();
    return QString::fromLatin1(stored).compare(expected, Qt::CaseInsensitive) == 0;
}

bool SoloDatabaseInit::copyChecksumFile(const QString &sourceDbPath, const QString &destDbPath)
{
    const QString src = sourceDbPath + checksumFileSuffix;
    const QString dst = destDbPath + checksumFileSuffix;
    if(!QFile::exists(src))
        return false;
    if(QFile::exists(dst))
        QFile::remove(dst);
    return QFile::copy(src, dst);
}
