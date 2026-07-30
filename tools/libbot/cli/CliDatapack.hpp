#ifndef CATCHCHALLENGER_CLI_CLIDATAPACK_H
#define CATCHCHALLENGER_CLI_CLIDATAPACK_H

#include <string>

#include "../../../client/libcatchchallenger/DatapackClientLoader.hpp"
#include "../../../general/base/GeneralType.hpp"

/** \brief Qt-free DatapackClientLoader.
 *
 * DatapackClientLoader is abstract only through five hooks the GUI client uses
 * to emit Qt signals / build tilesets. A CLI client has nothing to notify and
 * no images to load, so they collapse to a flag and no-ops.
 */
class CliDatapackClientLoader : public DatapackClientLoader
{
public:
    CliDatapackClientLoader();
    ~CliDatapackClientLoader();
    bool checksumError() const;
private:
    void emitdatapackParsed() override;
    void emitdatapackParsedMainSub() override;
    void emitdatapackChecksumError() override;
    void parseTopLib() override;
    std::string getLanguage() override;
    bool mChecksumError;
};

/** \brief process-wide datapack state.
 *
 * CommonDatapack / CommonDatapackServerSpec are singletons, so the datapack is
 * parsed ONCE per process no matter how many bots run. Both loadBase() and
 * loadMainSub() are idempotent: the second and later callers just get true.
 */
class CliDatapack
{
public:
    /// \brief parse the base datapack (also fills CommonDatapack)
    static bool loadBase(const std::string &datapackPath);
    /// \brief parse map/main/<mainCode>/ (+ sub) and CommonDatapackServerSpec
    static bool loadMainSub(const std::string &mainCode,const std::string &subCode);
    /// \brief number of maps the loader indexed; 0 when nothing is loaded
    static CATCHCHALLENGER_TYPE_MAPID mapCount();
    static const std::string &lastError();
private:
    static CliDatapackClientLoader *loader;
    static bool baseLoaded;
    static bool mainSubLoaded;
    static std::string errorString;
};

#endif // CATCHCHALLENGER_CLI_CLIDATAPACK_H
