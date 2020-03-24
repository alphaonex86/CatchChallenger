#ifndef CATCHCHALLENGER_DATAPACKCHECKSUM_H
#define CATCHCHALLENGER_DATAPACKCHECKSUM_H

#include <vector>
#include <string>

namespace CatchChallenger {
class DatapackChecksum
{
public:
    explicit DatapackChecksum();
    ~DatapackChecksum();
    struct FullDatapackChecksumReturn
    {
        std::vector<std::string> datapackFilesList;
        std::vector<char> hash;
        std::vector<uint32_t> partialHashList;
    };
    static std::vector<char> doChecksumBase(const std::string &datapackPath);
    static std::vector<char> doChecksumMain(const std::string &datapackPath);
    static std::vector<char> doChecksumSub(const std::string &datapackPath);
    static FullDatapackChecksumReturn doFullSyncChecksumBase(const std::string &datapackPath);
    static FullDatapackChecksumReturn doFullSyncChecksumMain(const std::string &datapackPath);
    static FullDatapackChecksumReturn doFullSyncChecksumSub(const std::string &datapackPath);
/*    #if ! defined(QT_NO_EMIT) && ! defined(EPOLLCATCHCHALLENGERSERVER) && !defined(NOTHREADS)
    void stopThread();
private:
    static QThread thread;
    #endif
    #if ! defined(QT_NO_EMIT) && ! defined(EPOLLCATCHCHALLENGERSERVER)
public slots:
    void doDifferedChecksumBase(const std::string &datapackPath);
    void doDifferedChecksumMain(const std::string &datapackPath);
    void doDifferedChecksumSub(const std::string &datapackPath);
signals:
    void datapackChecksumDoneBase(const std::vector<std::string> &datapackFilesList,const std::vector<char> &hash,const std::vector<uint32_t> &partialHashList);
    void datapackChecksumDoneMain(const std::vector<std::string> &datapackFilesList,const std::vector<char> &hash,const std::vector<uint32_t> &partialHashList);
    void datapackChecksumDoneSub(const std::vector<std::string> &datapackFilesList,const std::vector<char> &hash,const std::vector<uint32_t> &partialHashList);
    #endif*/
};
}

#endif // CATCHCHALLENGER_DATAPACKCHECKSUM_H
