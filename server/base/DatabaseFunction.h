#ifndef CATCHCHALLENGER_DATABASEFUNCTION_H
#define CATCHCHALLENGER_DATABASEFUNCTION_H

#include <string>
#include <vector>

namespace CatchChallenger {
class DatabaseFunction
{
public:
    virtual uint8_t stringtouint8(const std::string &string,bool *ok=NULL);
    virtual uint16_t stringtouint16(const std::string &string,bool *ok=NULL);
    virtual uint32_t stringtouint32(const std::string &string,bool *ok=NULL);
    virtual bool stringtobool(const std::string &string,bool *ok=NULL);
    virtual uint64_t stringtouint64(const std::string &string,bool *ok=NULL);
    virtual int8_t stringtoint8(const std::string &string,bool *ok=NULL);
    virtual int16_t stringtoint16(const std::string &string,bool *ok=NULL);
    virtual int32_t stringtoint32(const std::string &string,bool *ok=NULL);
    virtual int64_t stringtoint64(const std::string &string,bool *ok=NULL);
    virtual float stringtofloat(const std::string &string,bool *ok=NULL);
    virtual double stringtodouble(const std::string &string,bool *ok=NULL);
    virtual std::vector<char> hexatoBinary(const std::string &data);
};
}

#endif
