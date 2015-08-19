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
    static QByteArray toUTF8WithHeader(const std::basic_string<char> &text);
    static int toUTF8WithHeader(const std::basic_string<char> &text,char * const data);
    static int toUTF8With16BitsHeader(const std::basic_string<char> &text,char * const data);
    static std::vector<std::basic_string<char> > listFolder(const std::basic_string<char>& folder,const std::basic_string<char>& suffix=std::basic_string<char>());
    static std::basic_string<char> randomPassword(const std::basic_string<char>& string,const quint8& length);
    static std::vector<std::basic_string<char> > skinIdList(const std::basic_string<char>& skinPath);
    static std::basic_string<char> secondsToString(const quint64 &seconds);
    static bool rectTouch(QRect r1,QRect r2);
    static bool rmpath(const QDir &dir);
    static std::basic_string<char> timeToString(const quint32 &time);
private:
    static QByteArray UTF8EmptyData;
    static std::basic_string<char> text_slash;
    static std::basic_string<char> text_male;
    static std::basic_string<char> text_female;
    static std::basic_string<char> text_unknown;
    static std::basic_string<char> text_clan;
    static std::basic_string<char> text_dotcomma;
};
}

#endif
