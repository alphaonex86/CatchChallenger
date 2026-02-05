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
#include "../../general/base/lib.h"
#include "../libcatchchallenger/DatapackClientLoader.hpp"

class QtDatapackClientLoaderThread;

/* just load all, more simple and no load time (frame perfect but RAM intensive)
 * can be loaded ondemand but generate load time and reduce RAM needed
 * to be correctly loaded, the ideal is async + prefetch (prefetch monster/buff/item linked with the map), but need hurge rework
 * QPixmap only can be loaded into the main thread, QImage -> QPixmap have cost, then to prevent slowdown and have good experience, all QPixmap is loaded */
class DLL_PUBLIC QtDatapackClientLoader
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
    #ifndef NOTHREADS
    QMutex mutex;
    #endif

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
        //Tiled::Tileset * tileset;
        //Tiled::SharedTileset tileset;
        QImage tiles;
    };
    struct QtBuffExtra
    {
        QPixmap icon;
    };
    const QtMonsterExtra &getMonsterExtra(const CATCHCHALLENGER_TYPE_MONSTER &id);
    const QtItemExtra &getItemExtra(const CATCHCHALLENGER_TYPE_ITEM &id);
    const QtBuffExtra &getMonsterBuffExtra(const CATCHCHALLENGER_TYPE_BUFF &id);
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

    void newItemImage(CATCHCHALLENGER_TYPE_ITEM id);
    void newMonsterImage(CATCHCHALLENGER_TYPE_MONSTER id);
private:
    QPixmap *mDefaultInventoryImage;
    std::string getLanguage() override;

    std::unordered_map<CATCHCHALLENGER_TYPE_ITEM,QtItemExtra> QtitemsExtra;
    std::unordered_map<CATCHCHALLENGER_TYPE_MONSTER,QtMonsterExtra> QtmonsterExtra;
    std::unordered_map<CATCHCHALLENGER_TYPE_BUFF,QtBuffExtra> QtmonsterBuffsExtra;
    std::unordered_map<uint8_t,QtPlantExtra> QtplantExtra;
    std::unordered_set<QtDatapackClientLoaderThread *> threads;
    QtItemExtra emptyItem;
    QtMonsterExtra emptyMonster;
protected:
    std::unordered_map<CATCHCHALLENGER_TYPE_ITEM,ImageItemExtra> ImageitemsExtra;
    std::unordered_map<CATCHCHALLENGER_TYPE_MONSTER,ImageMonsterExtra> ImagemonsterExtra;
    std::vector<CATCHCHALLENGER_TYPE_ITEM> ImageitemsToLoad;
    std::vector<CATCHCHALLENGER_TYPE_MONSTER> ImagemonsterToLoad;
private slots:
    void parsePlantsExtra();
    void parseItemsExtra() override;
    void parseMonstersExtra();
    void parseBuffExtra();
    void parseTileset();

    void startThread();
    void threadFinished();
    void loadItemImage(CATCHCHALLENGER_TYPE_ITEM id,void *v);
    void loadMonsterImage(CATCHCHALLENGER_TYPE_MONSTER id,void *v);
};

#endif // DATAPACKCLIENTLOADER_H
