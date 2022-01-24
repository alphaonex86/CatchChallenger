#ifndef QtDATAPACKCLIENTLOADER_H
#define QtDATAPACKCLIENTLOADER_H

#ifndef NOTHREADS
#include <QThread>
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
#include "tiled/tiled_tileset.hpp"

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
    static QtDatapackClientLoader *datapackLoader;//pointer to control the init
    explicit QtDatapackClientLoader();
    std::string getFrontSkinPath(const uint32_t &skinId) const;
    std::string getSkinPath(const std::string &skinName,const std::string &type) const;
    ~QtDatapackClientLoader();
    struct QtItemExtra
    {
        QPixmap image;
    };
    struct QtMonsterExtra
    {
        QPixmap front;
        QPixmap back;
        QPixmap thumb;
        struct QtBuff
        {
            QIcon icon;
        };
    };
    struct QtPlantExtra
    {
        Tiled::Tileset * tileset;
    };
    std::unordered_map<uint16_t,QtItemExtra> QtitemsExtra;
    std::unordered_map<uint16_t,QtMonsterExtra> QtmonsterExtra;
    std::unordered_map<uint8_t,QtMonsterExtra::QtBuff> QtmonsterBuffsExtra;
    std::unordered_map<uint8_t,QtPlantExtra> QtplantExtra;
    QPixmap defaultInventoryImage();
    void resetAll();
    QImage imagesInterfaceFightLabelBottom,imagesInterfaceFightLabelTop;
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
private:
    QPixmap *mDefaultInventoryImage;
    std::string getLanguage() override;
private slots:
    void parsePlantsExtra();
    void parseItemsExtra() override;
    void parseMonstersExtra();
    void parseBuffExtra();
    void parseTileset();
};

#endif // DATAPACKCLIENTLOADER_H
