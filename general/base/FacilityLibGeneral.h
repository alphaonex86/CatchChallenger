#ifndef CATCHCHALLENGER_FACILITYLIB_H
#define CATCHCHALLENGER_FACILITYLIB_H

#include <QByteArray>
#include <QString>
#include <QStringList>
#include <QRect>
#include <QDir>
#include <QDateTime>

namespace CatchChallenger {
class FacilityLibGeneral
{
public:
    static QByteArray toUTF8WithHeader(const QString &text);
    static int toUTF8WithHeader(const QString &text,char * const data);
    static int toUTF8With16BitsHeader(const QString &text,char * const data);
    static QStringList listFolder(const QString& folder,const QString& suffix=QStringLiteral(""));
    static QString randomPassword(const QString& string,const quint8& length);
    static QStringList skinIdList(const QString& skinPath);
    static QString secondsToString(const quint64 &seconds);
    static bool rectTouch(QRect r1,QRect r2);
    static bool rmpath(const QDir &dir);
    static QString timeToString(const quint32 &time);
private:
    static QByteArray UTF8EmptyData;
    static QString text_slash;
    static QString text_male;
    static QString text_female;
    static QString text_unknown;
    static QString text_clan;
    static QString text_dotcomma;
};
}

#endif
