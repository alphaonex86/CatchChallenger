#ifndef QtDATAPACKCLIENTLOADER_H
#define QtDATAPACKCLIENTLOADER_H

#ifndef NOTHREADS
#include <QThread>
#include <QMutex>
#else
#include <QObject>
#endif
#include <QPixmap>
#include <QHash>
#include <string>
#include <vector>
#include <unordered_map>
#include <QIcon>
#include <QColor>
#include <utility>

#include "../../general/base/GeneralStructures.hpp"
#include "../libcatchchallenger/DatapackClientLoader.hpp"
#include "../tiled/tiled_tileset.hpp"

class QtDatapackClientLoaderThread;

class QtDatapackClientLoader
        #ifndef NOTHREADS
        : public QThread
        #else
        : public QObject
        #endif
        , public DatapackClientLoader
{
    Q_OBJECT
public:
    friend class QtDatapackClientLoaderThread;
    static QtDatapackClientLoader *datapackLoader;//pointer to control the init
    explicit QtDatapackClientLoader();
    ~QtDatapackClientLoader();
    struct ImageItemExtra
    {
        QImage image;
    };
    struct ImageMonsterExtra
    {
        QImage front;
        QImage back;
        QImage thumb;
    };
    QMutex mutex;

    struct QtItemExtra
    {
        QPixmap image;
    };
    struct QtMonsterExtra
    {
        QPixmap front;
        QPixmap back;
        QPixmap thumb;
    };
    struct QtPlantExtra
    {
        Tiled::Tileset * tileset;
    };
    struct QtBuffExtra
    {
        QPixmap icon;
    };
    const QtMonsterExtra &getMonsterExtra(const uint16_t &id);
    const QtItemExtra &getItemExtra(const uint16_t &id);
    const QtBuffExtra &getMonsterBuffExtra(const uint8_t &id);
    const QtPlantExtra &getPlantExtra(const uint8_t &id);

    QPixmap defaultInventoryImage();
    void resetAll();
    QImage imagesInterfaceFightLabelBottom,imagesInterfaceFightLabelTop;

    std::string getSkinPath(const std::string &skinName,const std::string &type) const;
    std::string getFrontSkinPath(const uint32_t &skinId) const;
protected:
    #ifndef NOTHREADS
    void run() override;
    #endif
    void emitdatapackParsed() override;
    void emitdatapackParsedMainSub() override;
    void emitdatapackChecksumError() override;
    void parseTopLib() override;
public slots:
    void parseDatapack(const std::string &datapackPath);
    void parseDatapackMainSub(const std::string &mainDatapackCode, const std::string &subDatapackCode);
signals:
    void datapackParsed();
    void datapackParsedMainSub();
    void datapackChecksumError();

    void newItemImage(uint16_t id);
    void newMonsterImage(uint16_t id);
private:
    QPixmap *mDefaultInventoryImage;
    std::string getLanguage() override;

    std::unordered_map<uint16_t,QtItemExtra> QtitemsExtra;
    std::unordered_map<uint16_t,QtMonsterExtra> QtmonsterExtra;
    std::unordered_map<uint8_t,QtBuffExtra> QtmonsterBuffsExtra;
    std::unordered_map<uint8_t,QtPlantExtra> QtplantExtra;
    std::unordered_set<QtDatapackClientLoaderThread *> threads;
    QtItemExtra emptyItem;
    QtMonsterExtra emptyMonster;
protected:
    std::unordered_map<uint16_t,ImageItemExtra> ImageitemsExtra;
    std::unordered_map<uint16_t,ImageMonsterExtra> ImagemonsterExtra;
    std::vector<uint16_t> ImageitemsToLoad;
    std::vector<uint16_t> ImagemonsterToLoad;
private slots:
    void parsePlantsExtra();
    void parseItemsExtra() override;
    void parseMonstersExtra();
    void parseBuffExtra();
    void parseTileset();

    void startThread();
    void threadFinished();
    void loadItemImage(uint16_t id,void *v);
    void loadMonsterImage(uint16_t id,void *v);
};

#endif // DATAPACKCLIENTLOADER_H
