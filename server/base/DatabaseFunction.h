#ifndef CATCHCHALLENGER_DATABASEFUNCTION_H
#define CATCHCHALLENGER_DATABASEFUNCTION_H

#include <string>

namespace CatchChallenger {
class DatabaseFunction
{
public:
    static uint8_t stringtouint8(const std::string &string,bool *ok=NULL);
    static uint16_t stringtouint16(const std::string &string,bool *ok=NULL);
    static uint32_t stringtouint32(const std::string &string,bool *ok=NULL);
    static bool stringtobool(const std::string &string,bool *ok=NULL);
    static uint64_t stringtouint64(const std::string &string,bool *ok=NULL);
    static int8_t stringtoint8(const std::string &string,bool *ok=NULL);
    static int16_t stringtoint16(const std::string &string,bool *ok=NULL);
    static int32_t stringtoint32(const std::string &string,bool *ok=NULL);
    static int64_t stringtoint64(const std::string &string,bool *ok=NULL);
    static float stringtofloat(const std::string &string,bool *ok=NULL);
    static double stringtodouble(const std::string &string,bool *ok=NULL);
};
}

#endif
