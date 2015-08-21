#ifndef CATCHCHALLENGER_FACILITYLIB_GENERAL_H
#define CATCHCHALLENGER_FACILITYLIB_GENERAL_H

#include <QByteArray>
#include <string>
#include <vector>
#include <QRect>
#include <QDir>
#include <QDateTime>

namespace CatchChallenger {
class FacilityLibGeneral
{
public:
    static QByteArray toUTF8WithHeader(const std::string &text);
    static int toUTF8WithHeader(const std::string &text,char * const data);
    static int toUTF8With16BitsHeader(const std::string &text,char * const data);
    static std::vector<std::string > listFolder(const std::string& folder,const std::string& suffix=std::string());
    static std::string randomPassword(const std::string& string,const quint8& length);
    static std::vector<std::string > skinIdList(const std::string& skinPath);
    static std::string secondsToString(const quint64 &seconds);
    static bool rectTouch(QRect r1,QRect r2);
    static bool rmpath(const QDir &dir);
    static std::string timeToString(const quint32 &time);
private:
    static QByteArray UTF8EmptyData;
    static std::string text_slash;
    static std::string text_male;
    static std::string text_female;
    static std::string text_unknown;
    static std::string text_clan;
    static std::string text_dotcomma;
};
}

#endif
