#ifndef SOLODATABASEINIT_HPP
#define SOLODATABASEINIT_HPP

#include <QString>
#include <QByteArray>

class SoloDatabaseInit
{
public:
    static const QString sqlResourcePath;
    static const QString checksumFileSuffix;

    // xxh32 of the embedded SQL file, computed once then cached.
    // Empty string if the resource could not be read.
    static const QString &cachedSqlChecksum();

    // Create a fresh SQLite database at dbPath from the embedded SQL resource.
    // On success writes dbPath + checksumFileSuffix containing the SQL checksum.
    // Returns false and fills *errorMsg on failure.
    static bool createSavegame(const QString &dbPath, QString *errorMsg);

    // Returns true if dbPath + checksumFileSuffix exists and matches cachedSqlChecksum().
    static bool isSavegameValid(const QString &dbPath);

    // Copy the checksum file alongside a copied savegame database.
    static bool copyChecksumFile(const QString &sourceDbPath, const QString &destDbPath);

private:
    static QByteArray readSqlResource();
    static QString computeChecksum(const QByteArray &data);
};

#endif
