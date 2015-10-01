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
    static unsigned int toUTF8WithHeader(const std::string &text,char * const data);
    static unsigned int toUTF8With16BitsHeader(const std::string &text,char * const data);
    static std::vector<std::string > listFolder(const std::string& folder,const std::string& suffix=std::string());
    static std::string randomPassword(const std::string& string,const uint8_t& length);
    static std::vector<std::string > skinIdList(const std::string& skinPath);
    //static std::string secondsToString(const uint64_t &seconds);
    static bool rectTouch(QRect r1,QRect r2);
    static bool rmpath(const std::string &dirPath);
    //static std::string timeToString(const uint32_t &time);
private:
    static std::string text_slash;
    static std::string text_male;
    static std::string text_female;
    static std::string text_unknown;
    static std::string text_clan;
    static std::string text_dotcomma;
};
}

#endif
