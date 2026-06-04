#ifndef GBA2CC_DATAPACKBASE_HPP
#define GBA2CC_DATAPACKBASE_HPP

// Loads the base datapack through the CatchChallenger client datapack loader
// (client/libcatchchallenger, the non-Qt base that libqtcatchchallenger builds
// on).  We only need the skin list for bot mapping, but loading via the real
// loader keeps us consistent with how the client reads the datapack.

#include "../../client/libcatchchallenger/DatapackClientLoader.hpp"

#include <string>
#include <vector>

class DatapackBase : public DatapackClientLoader {
public:
    DatapackBase();

    // Parse the datapack at datapackPath.  Returns the skin list (also reused
    // from disk by the caller for pixel-level matching).
    bool load(const std::string &datapackPath);

private:
    // DatapackClientLoader pure virtuals — no GUI side effects needed here.
    void emitdatapackParsed() override;
    void emitdatapackParsedMainSub() override;
    void emitdatapackChecksumError() override;
    void parseTopLib() override;
    std::string getLanguage() override;
};

#endif // GBA2CC_DATAPACKBASE_HPP
