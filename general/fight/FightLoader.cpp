#include "FightLoader.h"
#include "../base/GeneralVariable.h"
#include "../base/CommonSettingsCommon.h"
#include "../base/CommonSettingsServer.h"
#include "../base/CommonDatapack.h"

#include <QFile>
#include <vector>
#include <QDomDocument>
#include <QDomElement>
#include <QtCore/qmath.h>
#include <QDir>
#include <iostream>

using namespace CatchChallenger;

std::string FightLoader::text_type="type";
std::string FightLoader::text_name="name";
std::string FightLoader::text_id="id";
std::string FightLoader::text_multiplicator="multiplicator";
std::string FightLoader::text_number="number";
std::string FightLoader::text_to="to";
std::string FightLoader::text_dotcoma=";";
std::string FightLoader::text_list="list";
std::string FightLoader::text_monster="monster";
std::string FightLoader::text_monsters="monsters";
std::string FightLoader::text_dotxml=".xml";
std::string FightLoader::text_skills="skills";
std::string FightLoader::text_buffs="buffs";
std::string FightLoader::text_egg_step="egg_step";
std::string FightLoader::text_xp_for_max_level="xp_for_max_level";
std::string FightLoader::text_xp_max="xp_max";
std::string FightLoader::text_hp="hp";
std::string FightLoader::text_attack="attack";
std::string FightLoader::text_defense="defense";
std::string FightLoader::text_special_attack="special_attack";
std::string FightLoader::text_special_defense="special_defense";
std::string FightLoader::text_speed="speed";
std::string FightLoader::text_give_sp="give_sp";
std::string FightLoader::text_give_xp="give_xp";
std::string FightLoader::text_catch_rate="catch_rate";
std::string FightLoader::text_type2="type2";
std::string FightLoader::text_pow="pow";
std::string FightLoader::text_ratio_gender="ratio_gender";
std::string FightLoader::text_percent="%";
std::string FightLoader::text_attack_list="attack_list";
std::string FightLoader::text_skill="skill";
std::string FightLoader::text_skill_level="skill_level";
std::string FightLoader::text_attack_level="attack_level";
std::string FightLoader::text_level="level";
std::string FightLoader::text_byitem="byitem";
std::string FightLoader::text_evolution="evolution";
std::string FightLoader::text_evolutions="evolutions";
std::string FightLoader::text_trade="trade";
std::string FightLoader::text_evolveTo="evolveTo";
std::string FightLoader::text_item="item";
std::string FightLoader::text_fights="fights";
std::string FightLoader::text_fight="fight";
std::string FightLoader::text_gain="gain";
std::string FightLoader::text_cash="cash";
std::string FightLoader::text_sp="sp";
std::string FightLoader::text_effect="effect";
std::string FightLoader::text_endurance="endurance";
std::string FightLoader::text_life="life";
std::string FightLoader::text_applyOn="applyOn";
std::string FightLoader::text_quantity="quantity";
std::string FightLoader::text_more="+";
std::string FightLoader::text_success="success";
std::string FightLoader::text_buff="buff";
std::string FightLoader::text_capture_bonus="capture_bonus";
std::string FightLoader::text_duration="duration";
std::string FightLoader::text_Always="Always";
std::string FightLoader::text_NumberOfTurn="NumberOfTurn";
std::string FightLoader::text_durationNumberOfTurn="durationNumberOfTurn";
std::string FightLoader::text_ThisFight="ThisFight";
std::string FightLoader::text_inFight="inFight";
std::string FightLoader::text_inWalk="inWalk";
std::string FightLoader::text_steps="steps";

bool CatchChallenger::operator<(const Monster::AttackToLearn &entry1, const Monster::AttackToLearn &entry2)
{
    if(entry1.learnAtLevel!=entry2.learnAtLevel)
        return entry1.learnAtLevel < entry2.learnAtLevel;
    else
        return entry1.learnSkill < entry2.learnSkill;
}

#ifndef CATCHCHALLENGER_CLASS_MASTER
std::vector<Type> FightLoader::loadTypes(const std::string &file)
{
    std::unordered_map<std::string,uint8_t> nameToId;
    std::vector<Type> types;
    QDomDocument domDocument;
    //open and quick check the file
    #ifndef EPOLLCATCHCHALLENGERSERVER
    if(CommonDatapack::commonDatapack.xmlLoadedFile.find(file)!=CommonDatapack::commonDatapack.xmlLoadedFile.cend())
        domDocument=CommonDatapack::commonDatapack.xmlLoadedFile.at(file);
    else
    {
        #endif
        QFile itemsFile(QString::fromStdString(file));
        QByteArray xmlContent;
        if(!itemsFile.open(QIODevice::ReadOnly))
        {
            std::cerr << "Unable to open the file: " << file << ", error: " << itemsFile.errorString().toStdString() << std::endl;
            return types;
        }
        xmlContent=itemsFile.readAll();
        itemsFile.close();
        QString errorStr;
        int errorLine,errorColumn;
        if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
        {
            std::cerr << "Unable to open the file: " << file << ", Parse error at line " << errorLine << ", column " << errorColumn << ": " << errorStr.toStdString() << std::endl;
            return types;
        }
        #ifndef EPOLLCATCHCHALLENGERSERVER
        CommonDatapack::commonDatapack.xmlLoadedFile[file]=domDocument;
    }
    #endif
    QDomElement root = domDocument.documentElement();
    if(root.tagName()!=QLatin1String("types"))
    {
        std::cerr << "Unable to open the file: " << file << ", \"types\" root balise not found for the xml file" << std::endl;
        return types;
    }

    //load the content
    bool ok;
    {
        std::unordered_set<std::string> duplicate;
        QDomElement typeItem = root.firstChildElement(QString::fromStdString("type"));
        while(!typeItem.isNull())
        {
            if(typeItem.isElement())
            {
                if(typeItem.hasAttribute(QString::fromStdString(FightLoader::text_name)))
                {
                    std::string name=typeItem.attribute(QString::fromStdString(FightLoader::text_name)).toStdString();
                    if(duplicate.find(name)==duplicate.cend())
                    {
                        duplicate.insert(name);
                        Type type;
                        type.name=name;
                        nameToId[type.name]=types.size();
                        types.push_back(type);
                    }
                    else
                        std::cerr << "Unable to open the file: " << file << ", name is already set for type: child.tagName(): " << typeItem.tagName().toStdString() << " (at line: " << typeItem.lineNumber() << ")" << std::endl;
                }
                else
                    std::cerr << "Unable to open the file: " << file << ", have not the item id: child.tagName(): " << typeItem.tagName().toStdString() << " (at line: " << typeItem.lineNumber() << ")" << std::endl;
            }
            else
                std::cerr << "Unable to open the file: " << file << ", is not an element: child.tagName(): " << typeItem.tagName().toStdString() << " (at line: " << typeItem.lineNumber() << ")" << std::endl;
            typeItem = typeItem.nextSiblingElement(QString::fromStdString("type"));
        }
    }
    {
        std::unordered_set<std::string> duplicate;
        QDomElement typeItem = root.firstChildElement(QString::fromStdString("type"));
        while(!typeItem.isNull())
        {
            if(typeItem.isElement())
            {
                if(typeItem.hasAttribute(QString::fromStdString(FightLoader::text_name)))
                {
                    std::string name=typeItem.attribute(QString::fromStdString(FightLoader::text_name)).toStdString();
                    if(duplicate.find(name)==duplicate.cend())
                    {
                        duplicate.insert(name);
                        QDomElement multiplicator = typeItem.firstChildElement(QString::fromStdString(FightLoader::text_multiplicator));
                        while(!multiplicator.isNull())
                        {
                            if(multiplicator.isElement())
                            {
                                if(multiplicator.hasAttribute(QString::fromStdString("number")) && multiplicator.hasAttribute(QString::fromStdString(FightLoader::text_to)))
                                {
                                    float number=multiplicator.attribute(QString::fromStdString("number")).toFloat(&ok);
                                    std::vector<std::string> to=stringsplit(multiplicator.attribute(QString::fromStdString(FightLoader::text_to)).toStdString(),';');
                                    if(ok && (number==2.0 || number==0.5 || number==0.0))
                                    {
                                        unsigned int index=0;
                                        while(index<to.size())
                                        {
                                            if(nameToId.find(to.at(index))!=nameToId.cend())
                                            {
                                                const std::string &typeName=to.at(index);
                                                if(number==0)
                                                    types[nameToId.at(name)].multiplicator[nameToId.at(typeName)]=0;
                                                else if(number>1)
                                                    types[nameToId.at(name)].multiplicator[nameToId.at(typeName)]=number;
                                                else
                                                    types[nameToId.at(name)].multiplicator[nameToId.at(typeName)]=-(1.0/number);
                                            }
                                            else
                                                std::cerr << "Unable to open the file: " << file << ", name is not into list: " << to.at(index) << " is not found: child.tagName(): " << multiplicator.tagName().toStdString() << " (at line: " << multiplicator.lineNumber() << ")" << std::endl;
                                            index++;
                                        }
                                    }
                                    else
                                        std::cerr << "Unable to open the file: " << file << ", name is already set for type: child.tagName(): " << multiplicator.tagName().toStdString() << " (at line: " << multiplicator.lineNumber() << ")" << std::endl;
                                }
                                else
                                    std::cerr << "Unable to open the file: " << file << ", have not the item id: child.tagName(): " << multiplicator.tagName().toStdString() << " (at line: " << multiplicator.lineNumber() << ")" << std::endl;
                            }
                            else
                                std::cerr << "Unable to open the file: " << file << ", is not an element: child.tagName(): " << multiplicator.tagName().toStdString() << " (at line: " << multiplicator.lineNumber() << ")" << std::endl;
                            multiplicator = multiplicator.nextSiblingElement(QString::fromStdString(FightLoader::text_multiplicator));
                        }
                    }
                    else
                        std::cerr << "Unable to open the file: " << file << ", name is already set for type: child.tagName(): " << typeItem.tagName().toStdString() << " (at line: " << typeItem.lineNumber() << ")" << std::endl;
                }
                else
                    std::cerr << "Unable to open the file: " << file << ", have not the item id: child.tagName(): " << typeItem.tagName().toStdString() << " (at line: " << typeItem.lineNumber() << ")" << std::endl;
            }
            else
                std::cerr << "Unable to open the file: " << file << ", is not an element: child.tagName(): " << typeItem.tagName().toStdString() << " (at line: " << typeItem.lineNumber() << ")" << std::endl;
            typeItem = typeItem.nextSiblingElement(QString::fromStdString("type"));
        }
    }
    return types;
}
#endif

std::unordered_map<uint16_t,Monster> FightLoader::loadMonster(const std::string &folder, const std::unordered_map<uint16_t, Skill> &monsterSkills
                                                #ifndef CATCHCHALLENGER_CLASS_MASTER
                                                ,const std::vector<Type> &types,const std::unordered_map<uint16_t, Item> &items
                                                #endif
                                                )
{
    std::unordered_map<uint16_t,Monster> monsters;
    QDir dir(QString::fromStdString(folder));
    QFileInfoList fileList=dir.entryInfoList(QDir::Files|QDir::NoDotAndDotDot);
    unsigned int file_index=0;
    while(file_index<(uint32_t)fileList.size())
    {
        if(!fileList.at(file_index).isFile())
        {
            file_index++;
            continue;
        }
        const std::string &file=fileList.at(file_index).absoluteFilePath().toStdString();
        if(!stringEndsWith(file,FightLoader::text_dotxml))
        {
            file_index++;
            continue;
        }
        #ifndef CATCHCHALLENGER_CLASS_MASTER
        std::unordered_map<std::string,uint8_t> typeNameToId;
        {
            unsigned int index=0;
            while(index<types.size())
            {
                typeNameToId[types.at(index).name]=index;
                index++;
            }
        }
        #endif
        QDomDocument domDocument;
        //open and quick check the file
        #ifndef EPOLLCATCHCHALLENGERSERVER
        if(CommonDatapack::commonDatapack.xmlLoadedFile.find(file)!=CommonDatapack::commonDatapack.xmlLoadedFile.cend())
            domDocument=CommonDatapack::commonDatapack.xmlLoadedFile.at(file);
        else
        {
            #endif
            QFile xmlFile(QString::fromStdString(file));
            QByteArray xmlContent;
            if(!xmlFile.open(QIODevice::ReadOnly))
            {
                std::cerr << "Unable to open the xml monster file: " << file << ", error: " << xmlFile.errorString().toStdString() << std::endl;
                file_index++;
                continue;
            }
            xmlContent=xmlFile.readAll();
            xmlFile.close();
            QString errorStr;
            int errorLine,errorColumn;
            if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
            {
                std::cerr << "Unable to open the xml file: " << file << ", Parse error at line " << errorLine << ", column " << errorColumn << ": " << errorStr.toStdString() << std::endl;
                file_index++;
                continue;
            }
            #ifndef EPOLLCATCHCHALLENGERSERVER
            CommonDatapack::commonDatapack.xmlLoadedFile[file]=domDocument;
        }
        #endif
        QDomElement root = domDocument.documentElement();
        if(root.tagName().toStdString()!=FightLoader::text_monsters)
        {
            file_index++;
            continue;
        }

        //load the content
        bool ok,ok2;
        QDomElement item = root.firstChildElement(QString::fromStdString("monster"));
        while(!item.isNull())
        {
            if(item.isElement())
            {
                bool attributeIsOk=true;
                if(!item.hasAttribute(QString::fromStdString("id")))
                {
                    std::cerr << "Unable to open the xml file: " << file << ", have not the monster attribute \"id\": child.tagName(): " << item.tagName().toStdString() << " (at line: " << item.lineNumber() << ")" << std::endl;
                    attributeIsOk=false;
                }
                #ifndef CATCHCHALLENGER_CLASS_MASTER
                if(!item.hasAttribute(QString::fromStdString(FightLoader::text_egg_step)))
                {
                    std::cerr << "Unable to open the xml file: " << file << ", have not the monster attribute \"egg_step\": child.tagName(): " << item.tagName().toStdString() << " (at line: " << item.lineNumber() << ")" << std::endl;
                    attributeIsOk=false;
                }
                if(!item.hasAttribute(QString::fromStdString(FightLoader::text_xp_for_max_level)) && !item.hasAttribute(QString::fromStdString(FightLoader::text_xp_max)))
                {
                    std::cerr << "Unable to open the xml file: " << file << ", have not the monster attribute \"xp_for_max_level\": child.tagName(): " << item.tagName().toStdString() << " (at line: " << item.lineNumber() << ")" << std::endl;
                    attributeIsOk=false;
                }
                else
                {
                    if(!item.hasAttribute(QString::fromStdString(FightLoader::text_xp_for_max_level)))
                        item.setAttribute(QString::fromStdString(FightLoader::text_xp_for_max_level),item.attribute(QString::fromStdString(FightLoader::text_xp_max)));
                }
                if(!item.hasAttribute(QString::fromStdString("hp")))
                {
                    std::cerr << "Unable to open the xml file: " << file << ", have not the monster attribute \"hp\": child.tagName(): " << item.tagName().toStdString() << " (at line: " << item.lineNumber() << ")" << std::endl;
                    attributeIsOk=false;
                }
                if(!item.hasAttribute(QString::fromStdString("attack")))
                {
                    std::cerr << "Unable to open the xml file: " << file << ", have not the monster attribute \"attack\": child.tagName(): " << item.tagName().toStdString() << " (at line: " << item.lineNumber() << ")" << std::endl;
                    attributeIsOk=false;
                }
                if(!item.hasAttribute(QString::fromStdString("defense")))
                {
                    std::cerr << "Unable to open the xml file: " << file << ", have not the monster attribute \"defense\": child.tagName(): " << item.tagName().toStdString() << " (at line: " << item.lineNumber() << ")" << std::endl;
                    attributeIsOk=false;
                }
                if(!item.hasAttribute(QString::fromStdString(FightLoader::text_special_attack)))
                {
                    std::cerr << "Unable to open the xml file: " << file << ", have not the monster attribute \"special_attack\": child.tagName(): " << item.tagName().toStdString() << " (at line: " << item.lineNumber() << ")" << std::endl;
                    attributeIsOk=false;
                }
                if(!item.hasAttribute(QString::fromStdString(FightLoader::text_special_defense)))
                {
                    std::cerr << "Unable to open the xml file: " << file << ", have not the monster attribute \"special_defense\": child.tagName(): " << item.tagName().toStdString() << " (at line: " << item.lineNumber() << ")" << std::endl;
                    attributeIsOk=false;
                }
                if(!item.hasAttribute(QString::fromStdString(FightLoader::text_speed)))
                {
                    std::cerr << "Unable to open the xml file: " << file << ", have not the monster attribute \"speed\": child.tagName(): " << item.tagName().toStdString() << " (at line: " << item.lineNumber() << ")" << std::endl;
                    attributeIsOk=false;
                }
                if(!item.hasAttribute(QString::fromStdString(FightLoader::text_give_sp)))
                {
                    std::cerr << "Unable to open the xml file: " << file << ", have not the monster attribute \"give_sp\": child.tagName(): " << item.tagName().toStdString() << " (at line: " << item.lineNumber() << ")" << std::endl;
                    attributeIsOk=false;
                }
                if(!item.hasAttribute(QString::fromStdString(FightLoader::text_give_xp)))
                {
                    std::cerr << "Unable to open the xml file: " << file << ", have not the monster attribute \"give_xp\": child.tagName(): " << item.tagName().toStdString() << " (at line: " << item.lineNumber() << ")" << std::endl;
                    attributeIsOk=false;
                }
                #endif // CATCHCHALLENGER_CLASS_MASTER
                if(attributeIsOk)
                {
                    Monster monster;
                    monster.catch_rate=100;
                    uint32_t id=item.attribute(QString::fromStdString("id")).toUInt(&ok);
                    if(!ok)
                        std::cerr << "Unable to open the xml file: " << file << ", id not a number: child.tagName(): " << item.tagName().toStdString() << " (at line: " << item.lineNumber() << ")" << std::endl;
                    else if(monsters.find(id)!=monsters.cend())
                        std::cerr << "Unable to open the xml file: " << file << ", id already found: child.tagName(): " << item.tagName().toStdString() << " (at line: " << item.lineNumber() << ")" << std::endl;
                    else
                    {
                        #ifndef CATCHCHALLENGER_CLASS_MASTER
                        if(item.hasAttribute(QString::fromStdString(FightLoader::text_catch_rate)))
                        {
                            bool ok2;
                            uint32_t catch_rate=item.attribute(QString::fromStdString(FightLoader::text_catch_rate)).toUInt(&ok2);
                            if(ok2)
                            {
                                if(catch_rate<=255)
                                    monster.catch_rate=catch_rate;
                                else
                                    std::cerr << "Unable to open the xml file: " << file << ", catch_rate is not a number: " << item.attribute("catch_rate").toStdString() << " child.tagName(): " << item.tagName().toStdString() << " (at line: " << item.lineNumber() << ")" << std::endl;
                            }
                            else
                                std::cerr << "Unable to open the xml file: " << file << ", catch_rate is not a number: " << item.attribute("catch_rate").toStdString() << " child.tagName(): " << item.tagName().toStdString() << " (at line: " << item.lineNumber() << ")" << std::endl;
                        }
                        if(item.hasAttribute(QString::fromStdString("type")))
                        {
                            const std::vector<std::string> &typeList=stringsplit(item.attribute(QString::fromStdString("type")).toStdString(),';');
                            unsigned int index=0;
                            while(index<typeList.size())
                            {
                                if(typeNameToId.find(typeList.at(index))!=typeNameToId.cend())
                                    monster.type.push_back(typeNameToId.at(typeList.at(index)));
                                else
                                    std::cerr << "Unable to open the xml file: " << file << ", type not found into the list: " << item.attribute(QString::fromStdString("type")).toStdString() << " child.tagName(): " << item.tagName().toStdString() << " (at line: " << item.lineNumber() << ")" << std::endl;
                                index++;
                            }
                        }
                        if(item.hasAttribute(QString::fromStdString(FightLoader::text_type2)))
                        {
                            const std::vector<std::string> &typeList=stringsplit(item.attribute(QString::fromStdString(FightLoader::text_type2)).toStdString(),';');
                            unsigned int index=0;
                            while(index<typeList.size())
                            {
                                if(typeNameToId.find(typeList.at(index))!=typeNameToId.cend())
                                    monster.type.push_back(typeNameToId.at(typeList.at(index)));
                                else
                                    std::cerr << "Unable to open the xml file: " << file << ", type not found into the list: " << item.attribute(QString::fromStdString(FightLoader::text_type2)).toStdString() << " child.tagName(): " << item.tagName().toStdString() << " (at line: " << item.lineNumber() << ")" << std::endl;
                                index++;
                            }
                        }
                        qreal pow=1.0;
                        if(ok)
                        {
                            if(item.hasAttribute(QString::fromStdString(FightLoader::text_pow)))
                            {
                                pow=item.attribute(QString::fromStdString(FightLoader::text_pow)).toDouble(&ok);
                                if(!ok)
                                {
                                    pow=1.0;
                                    std::cerr << "Unable to open the xml file: " << file << ", pow is not a double: child.tagName(): " << item.tagName().toStdString() << " (at line: " << item.lineNumber() << ")" << std::endl;
                                    ok=true;
                                }
                                if(pow<=1.0)
                                {
                                    pow=1.0;
                                    std::cerr << "Unable to open the xml file: " << file << ", pow is too low: child.tagName(): " << item.tagName().toStdString() << " (at line: " << item.lineNumber() << ")" << std::endl;
                                }
                                if(pow>=10.0)
                                {
                                    pow=1.0;
                                    std::cerr << "Unable to open the xml file: " << file << ", pow is too hight: child.tagName(): " << item.tagName().toStdString() << " (at line: " << item.lineNumber() << ")" << std::endl;
                                }
                            }
                        }
                        #ifndef EPOLLCATCHCHALLENGERSERVERNOGAMESERVER
                        pow=qPow(pow,CommonSettingsServer::commonSettingsServer.rates_xp_pow);
                        #endif
                        if(ok)
                        {
                            monster.egg_step=item.attribute(QString::fromStdString(FightLoader::text_egg_step)).toUInt(&ok);
                            if(!ok)
                                std::cerr << "Unable to open the xml file: " << file << ", egg_step is not number: child.tagName(): " << item.tagName().toStdString() << " (at line: " << item.lineNumber() << ")" << std::endl;
                        }
                        if(ok)
                        {
                            monster.xp_for_max_level=item.attribute(QString::fromStdString(FightLoader::text_xp_for_max_level)).toUInt(&ok);
                            if(!ok)
                                std::cerr << "Unable to open the xml file: " << file << ", xp_for_max_level is not number: child.tagName(): " << item.tagName().toStdString() << " (at line: " << item.lineNumber() << ")" << std::endl;
                        }
                        #endif
                        if(ok)
                        {
                            monster.stat.hp=item.attribute(QString::fromStdString("hp")).toUInt(&ok);
                            if(!ok)
                                std::cerr << "Unable to open the xml file: " << file << ", hp is not number: child.tagName(): " << item.tagName().toStdString() << " (at line: " << item.lineNumber() << ")" << std::endl;
                        }
                        #ifndef CATCHCHALLENGER_CLASS_MASTER
                        if(ok)
                        {
                            monster.stat.attack=item.attribute(QString::fromStdString("attack")).toUInt(&ok);
                            if(!ok)
                                std::cerr << "Unable to open the xml file: " << file << ", attack is not number: child.tagName(): " << item.tagName().toStdString() << " (at line: " << item.lineNumber() << ")" << std::endl;
                        }
                        if(ok)
                        {
                            monster.stat.defense=item.attribute(QString::fromStdString("defense")).toUInt(&ok);
                            if(!ok)
                                std::cerr << "Unable to open the xml file: " << file << ", defense is not number: child.tagName(): " << item.tagName().toStdString() << " (at line: " << item.lineNumber() << ")" << std::endl;
                        }
                        if(ok)
                        {
                            monster.stat.special_attack=item.attribute(QString::fromStdString(FightLoader::text_special_attack)).toUInt(&ok);
                            if(!ok)
                                std::cerr << "Unable to open the xml file: " << file << ", special_attack is not number: child.tagName(): " << item.tagName().toStdString() << " (at line: " << item.lineNumber() << ")" << std::endl;
                        }
                        if(ok)
                        {
                            monster.stat.special_defense=item.attribute(QString::fromStdString(FightLoader::text_special_defense)).toUInt(&ok);
                            if(!ok)
                                std::cerr << "Unable to open the xml file: " << file << ", special_defense is not number: child.tagName(): " << item.tagName().toStdString() << " (at line: " << item.lineNumber() << ")" << std::endl;
                        }
                        if(ok)
                        {
                            monster.stat.speed=item.attribute(QString::fromStdString(FightLoader::text_speed)).toUInt(&ok);
                            if(!ok)
                                std::cerr << "Unable to open the xml file: " << file << ", speed is not number: child.tagName(): " << item.tagName().toStdString() << " (at line: " << item.lineNumber() << ")" << std::endl;
                        }
                        #ifndef EPOLLCATCHCHALLENGERSERVERNOGAMESERVER
                        if(ok)
                        {
                            monster.give_xp=item.attribute(QString::fromStdString(FightLoader::text_give_xp)).toUInt(&ok)*CommonSettingsServer::commonSettingsServer.rates_xp;
                            if(!ok)
                                std::cerr << "Unable to open the xml file: " << file << ", give_xp is not number: child.tagName(): " << item.tagName().toStdString() << " (at line: " << item.lineNumber() << ")" << std::endl;
                        }
                        if(ok)
                        {
                            monster.give_sp=item.attribute(QString::fromStdString(FightLoader::text_give_sp)).toUInt(&ok)*CommonSettingsServer::commonSettingsServer.rates_xp;
                            if(!ok)
                                std::cerr << "Unable to open the xml file: " << file << ", give_sp is not number: child.tagName(): " << item.tagName().toStdString() << " (at line: " << item.lineNumber() << ")" << std::endl;
                        }
                        #else
                        monster.give_xp=0;
                        monster.give_sp=0;
                        #endif
                        #endif
                        if(ok)
                        {
                            if(item.hasAttribute(QString::fromStdString(FightLoader::text_ratio_gender)))
                            {
                                std::string ratio_gender=item.attribute(QString::fromStdString(FightLoader::text_ratio_gender)).toStdString();
                                stringreplaceOne(ratio_gender,FightLoader::text_percent,"");
                                monster.ratio_gender=stringtouint8(ratio_gender,&ok2);
                                if(!ok2)
                                {
                                    std::cerr << "Unable to open the xml file: " << file << ", ratio_gender is not number: child.tagName(): " << item.tagName().toStdString() << " (at line: " << item.lineNumber() << ")" << std::endl;
                                    monster.ratio_gender=50;
                                }
                                if(monster.ratio_gender<-1 || monster.ratio_gender>100)
                                {
                                    std::cerr << "Unable to open the xml file: " << file << ", ratio_gender is not in range of -1, 100: child.tagName(): " << item.tagName().toStdString() << " (at line: " << item.lineNumber() << ")" << std::endl;
                                    monster.ratio_gender=50;
                                }
                            }
                            monster.ratio_gender=50;
                        }
                        if(ok)
                        {
                            {
                                QDomElement attack_list = item.firstChildElement(QString::fromStdString(FightLoader::text_attack_list));
                                if(!attack_list.isNull())
                                {
                                    if(attack_list.isElement())
                                    {
                                        QDomElement attack = attack_list.firstChildElement(QString::fromStdString("attack"));
                                        while(!attack.isNull())
                                        {
                                            if(attack.isElement())
                                            {
                                                if(attack.hasAttribute(QString::fromStdString("skill")) || attack.hasAttribute(QString::fromStdString("id")))
                                                {
                                                    ok=true;
                                                    if(!attack.hasAttribute(QString::fromStdString("skill")))
                                                        attack.setAttribute(QString::fromStdString("skill"),attack.attribute(QString::fromStdString("id")));
                                                    Monster::AttackToLearn attackVar;
                                                    if(attack.hasAttribute(QString::fromStdString(FightLoader::text_skill_level)) || attack.hasAttribute(QString::fromStdString(FightLoader::text_attack_level)))
                                                    {
                                                        if(!attack.hasAttribute(QString::fromStdString(FightLoader::text_skill_level)))
                                                            attack.setAttribute(QString::fromStdString(FightLoader::text_skill_level),attack.attribute(QString::fromStdString(FightLoader::text_attack_level)));
                                                        attackVar.learnSkillLevel=attack.attribute(QString::fromStdString(FightLoader::text_skill_level)).toUShort(&ok);
                                                        if(!ok)
                                                            std::cerr << "Unable to open the xml file: " << file << ", skill_level is not a number: child.tagName(): " << attack.tagName().toStdString() << " (at line: " << attack.lineNumber() << ")" << std::endl;
                                                    }
                                                    else
                                                        attackVar.learnSkillLevel=1;
                                                    if(ok)
                                                    {
                                                        attackVar.learnSkill=attack.attribute(QString::fromStdString("skill")).toUShort(&ok);
                                                        if(!ok)
                                                            std::cerr << "Unable to open the xml file: " << file << ", skill is not a number: child.tagName(): " << attack.tagName().toStdString() << " (at line: " << attack.lineNumber() << ")" << std::endl;
                                                    }
                                                    if(ok)
                                                    {
                                                        if(monsterSkills.find(attackVar.learnSkill)==monsterSkills.cend())
                                                        {
                                                            std::cerr << "Unable to open the xml file: " << file << ", attack is not into attack loaded: child.tagName(): " << attack.tagName().toStdString() << " (at line: " << attack.lineNumber() << ")" << std::endl;
                                                            ok=false;
                                                        }
                                                    }
                                                    if(ok)
                                                    {
                                                        if(attackVar.learnSkillLevel<=0 || attackVar.learnSkillLevel>(uint32_t)monsterSkills.at(attackVar.learnSkill).level.size())
                                                        {
                                                            std::cerr << "Unable to open the xml file: " << file << ", attack level is not in range 1-" << monsterSkills.at(attackVar.learnSkill).level.size() << ": child.tagName(): " << attack.tagName().toStdString() << " (at line: " << attack.lineNumber() << ")" << std::endl;
                                                            ok=false;
                                                        }
                                                    }
                                                    if(attack.hasAttribute(QString::fromStdString("level")))
                                                    {
                                                        if(ok)
                                                        {
                                                            attackVar.learnAtLevel=attack.attribute(QString::fromStdString("level")).toUShort(&ok);
                                                            if(ok)
                                                            {
                                                                unsigned int index;
                                                                //if it's the first lean don't need previous learn
                                                                if(attackVar.learnSkillLevel>1)
                                                                {
                                                                    index=0;
                                                                    while(index<monster.learn.size())
                                                                    {
                                                                        if(monster.learn.at(index).learnSkillLevel==(attackVar.learnSkillLevel-1) && monster.learn.at(index).learnSkill==attackVar.learnSkill)
                                                                            break;
                                                                        index++;
                                                                    }
                                                                    if(index==monster.learn.size())
                                                                    {
                                                                        std::cerr << "Unable to open the xml file: " << file << ", attack " << attackVar.learnSkill << " with level " << attackVar.learnSkillLevel << " can't be added because not same attack with previous level: child.tagName(): " << attack.tagName().toStdString() << " (at line: " << attack.lineNumber() << ")" << std::endl;
                                                                        ok=false;
                                                                    }
                                                                }

                                                                //check if can learn
                                                                if(ok)
                                                                {
                                                                    index=0;
                                                                    while(index<monster.learn.size())
                                                                    {
                                                                        if(attackVar.learnSkillLevel>1 && (monster.learn.at(index).learnSkillLevel-1)==attackVar.learnSkillLevel)
                                                                            ok=true;
                                                                        if(monster.learn.at(index).learnSkillLevel==attackVar.learnSkillLevel && monster.learn.at(index).learnSkill==attackVar.learnSkill)
                                                                        {
                                                                            std::cerr << "Unable to open the xml file: " << file << ", attack already do for this level for skill " << attackVar.learnSkill << " at level " << attackVar.learnSkillLevel << " for monster " << id << ": child.tagName(): " << attack.tagName().toStdString() << " (at line: " << attack.lineNumber() << ")" << std::endl;
                                                                            ok=false;
                                                                            break;
                                                                        }
                                                                        if(monster.learn.at(index).learnSkill==attackVar.learnSkill && monster.learn.at(index).learnSkillLevel==attackVar.learnSkillLevel)
                                                                        {
                                                                            std::cerr << "Unable to open the xml file: " << file << ", this attack level is already found " << attackVar.learnSkill << ", level: " << attackVar.learnSkillLevel << " for attack: " << index << ": child.tagName(): " << attack.tagName().toStdString() << " (at line: " << attack.lineNumber() << ")" << std::endl;
                                                                            ok=false;
                                                                            break;
                                                                        }
                                                                        index++;
                                                                    }
                                                                    if(ok)
                                                                        monster.learn.push_back(attackVar);
                                                                }
                                                                else
                                                                    std::cerr << "Unable to open the xml file: " << file << ", no way to learn " << attackVar.learnSkill << ", level: " << attackVar.learnSkillLevel << " for attack: " << index << ": child.tagName(): " << attack.tagName().toStdString() << " (at line: " << attack.lineNumber() << ")" << std::endl;
                                                            }
                                                            else
                                                                std::cerr << "Unable to open the xml file: " << file << ", level is not a number: child.tagName(): " << attack.tagName().toStdString() << " (at line: " << attack.lineNumber() << ")" << std::endl;
                                                        }
                                                    }
                                                    else
                                                    {
                                                        #ifndef CATCHCHALLENGER_CLASS_MASTER
                                                        if(attack.hasAttribute(QString::fromStdString(FightLoader::text_byitem)))
                                                        {
                                                            uint32_t itemId;
                                                            if(ok)
                                                            {
                                                                itemId=attack.attribute(QString::fromStdString(FightLoader::text_byitem)).toUShort(&ok);
                                                                if(!ok)
                                                                    std::cerr << "Unable to open the xml file: " << file << ", item to learn is not a number " << attack.attribute("byitem").toStdString() << ": child.tagName(): " << attack.tagName().toStdString() << " (at line: " << attack.lineNumber() << ")" << std::endl;
                                                            }
                                                            if(ok)
                                                            {
                                                                if(items.find(itemId)==items.cend())
                                                                {
                                                                    std::cerr << "Unable to open the xml file: " << file << ", item to learn not found " << itemId << ": child.tagName(): " << attack.tagName().toStdString() << " (at line: " << attack.lineNumber() << ")" << std::endl;
                                                                    ok=false;
                                                                }
                                                            }
                                                            if(ok)
                                                            {
                                                                if(monster.learnByItem.find(itemId)!=monster.learnByItem.cend())
                                                                {
                                                                    std::cerr << "Unable to open the xml file: " << file << ", item to learn is already used " << itemId << ": child.tagName(): " << attack.tagName().toStdString() << " (at line: " << attack.lineNumber() << ")" << std::endl;
                                                                    ok=false;
                                                                }
                                                            }
                                                            if(ok)
                                                            {
                                                                Monster::AttackToLearnByItem tempEntry;
                                                                tempEntry.learnSkill=attackVar.learnSkill;
                                                                tempEntry.learnSkillLevel=attackVar.learnSkillLevel;
                                                                monster.learnByItem[itemId]=tempEntry;
                                                            }
                                                        }
                                                        else
                                                        {
                                                            std::cerr << "Unable to open the xml file: " << file << ", level and byitem is not found: child.tagName(): " << attack.tagName().toStdString() << " (at line: " << attack.lineNumber() << ")" << std::endl;
                                                            ok=false;
                                                        }
                                                        #endif // CATCHCHALLENGER_CLASS_MASTER
                                                    }
                                                }
                                                else
                                                    std::cerr << "Unable to open the xml file: " << file << ", missing arguements (level or skill): child.tagName(): " << attack.tagName().toStdString() << " (at line: " << attack.lineNumber() << ")" << std::endl;
                                            }
                                            else
                                                std::cerr << "Unable to open the xml file: " << file << ", attack_list balise is not an element: child.tagName(): " << attack.tagName().toStdString() << " (at line: " << attack.lineNumber() << ")" << std::endl;
                                            attack = attack.nextSiblingElement(QString::fromStdString("attack"));
                                        }
                                        qSort(monster.learn);
                                    }
                                    else
                                        std::cerr << "Unable to open the xml file: " << file << ", attack_list balise is not an element: child.tagName(): " << item.tagName().toStdString() << " (at line: " << item.lineNumber() << ")" << std::endl;
                                }
                                else
                                    std::cerr << "Unable to open the xml file: " << file << ", have not attack_list: child.tagName(): " << item.tagName().toStdString() << " (at line: " << item.lineNumber() << ")" << std::endl;
                            }
                            #ifndef CATCHCHALLENGER_CLASS_MASTER
                            {
                                QDomElement evolutionsItem = item.firstChildElement(QString::fromStdString(FightLoader::text_evolutions));
                                if(!evolutionsItem.isNull())
                                {
                                    if(evolutionsItem.isElement())
                                    {
                                        QDomElement evolutionItem = evolutionsItem.firstChildElement(QString::fromStdString(FightLoader::text_evolution));
                                        while(!evolutionItem.isNull())
                                        {
                                            if(evolutionItem.isElement())
                                            {
                                                if(evolutionItem.hasAttribute(QString::fromStdString("type")) && (
                                                       evolutionItem.hasAttribute(QString::fromStdString("level")) ||
                                                       (evolutionItem.attribute(QString::fromStdString("type"))=="trade" && evolutionItem.hasAttribute(QString::fromStdString(FightLoader::text_evolveTo))) ||
                                                       (evolutionItem.attribute(QString::fromStdString("type"))=="item" && evolutionItem.hasAttribute(QString::fromStdString("item")))
                                                       )
                                                   )
                                                {
                                                    ok=true;
                                                    Monster::Evolution evolutionVar;
                                                    const std::string &typeText=evolutionItem.attribute(QString::fromStdString("type")).toStdString();
                                                    if(typeText!=FightLoader::text_trade)
                                                    {
                                                        if(typeText==FightLoader::text_item)
                                                            evolutionVar.level=evolutionItem.attribute(QString::fromStdString("item")).toUInt(&ok);
                                                        else
                                                            evolutionVar.level=evolutionItem.attribute(QString::fromStdString("level")).toInt(&ok);
                                                        if(!ok)
                                                            std::cerr << "Unable to open the xml file: " << file << ", level is not a number: child.tagName(): " << evolutionItem.tagName().toStdString() << " (at line: " << evolutionItem.lineNumber() << ")" << std::endl;
                                                    }
                                                    else
                                                        evolutionVar.level=0;
                                                    if(ok)
                                                    {
                                                        evolutionVar.evolveTo=evolutionItem.attribute(QString::fromStdString(FightLoader::text_evolveTo)).toUInt(&ok);
                                                        if(!ok)
                                                            std::cerr << "Unable to open the xml file: " << file << ", evolveTo is not a number: child.tagName(): " << evolutionItem.tagName().toStdString() << " (at line: " << evolutionItem.lineNumber() << ")" << std::endl;
                                                    }
                                                    if(ok)
                                                    {
                                                        if(typeText==FightLoader::text_level)
                                                            evolutionVar.type=Monster::EvolutionType_Level;
                                                        else if(typeText==FightLoader::text_item)
                                                            evolutionVar.type=Monster::EvolutionType_Item;
                                                        else if(typeText==FightLoader::text_trade)
                                                            evolutionVar.type=Monster::EvolutionType_Trade;
                                                        else
                                                        {
                                                            ok=false;
                                                            std::cerr << "Unable to open the xml file: " << file << ", unknown evolution type: " << typeText << " child.tagName(): " << evolutionItem.tagName().toStdString() << " (at line: " << evolutionItem.lineNumber() << ")" << std::endl;
                                                        }
                                                    }
                                                    if(ok)
                                                    {
                                                        if(typeText==FightLoader::text_level && (evolutionVar.level<0 || evolutionVar.level>CATCHCHALLENGER_MONSTER_LEVEL_MAX))
                                                        {
                                                            ok=false;
                                                            std::cerr << "Unable to open the xml file: " << file << ", level out of range: " << evolutionVar.level << " child.tagName(): " << evolutionItem.tagName().toStdString() << " (at line: " << evolutionItem.lineNumber() << ")" << std::endl;
                                                        }
                                                    }
                                                    if(ok)
                                                    {
                                                        if(typeText==FightLoader::text_item)
                                                        {
                                                            if(items.find(evolutionVar.level)==items.cend())
                                                            {
                                                                ok=false;
                                                                std::cerr << "Unable to open the xml file: " << file << ", unknown evolution item: " << evolutionVar.level << " child.tagName(): " << evolutionItem.tagName().toStdString() << " (at line: " << evolutionItem.lineNumber() << ")" << std::endl;
                                                            }
                                                        }
                                                    }
                                                    if(ok)
                                                        monster.evolutions.push_back(evolutionVar);
                                                }
                                                else
                                                    std::cerr << "Unable to open the xml file: " << file << ", missing arguements (level or skill): child.tagName(): " << evolutionItem.tagName().toStdString() << " (at line: " << evolutionItem.lineNumber() << ")" << std::endl;
                                            }
                                            else
                                                std::cerr << "Unable to open the xml file: " << file << ", attack_list balise is not an element: child.tagName(): " << evolutionItem.tagName().toStdString() << " (at line: " << evolutionItem.lineNumber() << ")" << std::endl;
                                            evolutionItem = evolutionItem.nextSiblingElement(QString::fromStdString(FightLoader::text_evolution));
                                        }
                                    }
                                    else
                                        std::cerr << "Unable to open the xml file: " << file << ", attack_list balise is not an element: child.tagName(): " << item.tagName().toStdString() << " (at line: " << item.lineNumber() << ")" << std::endl;
                                }
                            }
                            int index=0;
                            while(index<CATCHCHALLENGER_MONSTER_LEVEL_MAX)
                            {
                                uint64_t xp_for_this_level=qPow(index+1,pow);
                                uint64_t xp_for_max_level=monster.xp_for_max_level;
                                uint64_t max_xp=qPow(CATCHCHALLENGER_MONSTER_LEVEL_MAX,pow);
                                uint64_t tempXp=xp_for_this_level*xp_for_max_level/max_xp;
                                if(tempXp<1)
                                    tempXp=1;
                                monster.level_to_xp.push_back(tempXp);
                                index++;
                            }
                            #ifdef DEBUG_MESSAGE_MONSTER_XP_LOAD
                            index=0;
                            while(index<CATCHCHALLENGER_MONSTER_LEVEL_MAX)
                            {
                                int give_xp=(monster.give_xp*(index+1))/CATCHCHALLENGER_MONSTER_LEVEL_MAX;
                                std::cerr << "monster: " << id << ", xp " << index+1 << " for the level: " << monster.level_to_xp.at(index) << ", monster for this level: " << monster.level_to_xp.at(index)/give_xp << std::endl;
                                index++;
                            }
                            std::cerr << "monster.level_to_xp.size(): " << monster.level_to_xp.size() << std::endl;
                            #endif

                            #ifndef EPOLLCATCHCHALLENGERSERVERNOGAMESERVER
                            #ifdef CATCHCHALLENGER_EXTRA_CHECK
                            if(monster.give_xp!=0)
                                if((monster.xp_for_max_level*CommonSettingsServer::commonSettingsServer.rates_xp/monster.give_xp)>150)
                                    std::cerr << "Warning: you need more than " << monster.xp_for_max_level/monster.give_xp << " monster(s) to pass the last level, prefer do that's with the rate for the monster id: " << id << std::endl;
                            #endif
                            #endif

                            #endif // CATCHCHALLENGER_CLASS_MASTER
                            monsters[id]=monster;
                        }
                        else
                            std::cerr << "Unable to open the xml file: " << file << ", one of the attribute is wrong or is not a number: child.tagName(): " << item.tagName().toStdString() << " (at line: " << item.lineNumber() << ")" << std::endl;
                    }
                }
                else
                    std::cerr << "Unable to open the xml file: " << file << ", have not the monster id: child.tagName(): " << item.tagName().toStdString() << " (at line: " << item.lineNumber() << ")" << std::endl;
            }
            else
                std::cerr << "Unable to open the xml file: " << file << ", is not an element: child.tagName(): " << item.tagName().toStdString() << " (at line: " << item.lineNumber() << ")" << std::endl;
            item = item.nextSiblingElement(QString::fromStdString("monster"));
        }
        //check the evolveTo
        auto i = monsters.begin();
        while(i!=monsters.cend())
        {
            unsigned int index=0;
            bool evolutionByLevel=false,evolutionByTrade=false;
            while(index<i->second.evolutions.size())
            {
                std::unordered_set<uint32_t> itemUse;
                if(i->second.evolutions.at(index).type==Monster::EvolutionType_Level)
                {
                    if(evolutionByLevel)
                    {
                        std::cerr << "Unable to open the xml file: " << file << ", the monster " << i->first << " have already evolution by level: child.tagName(): " << item.tagName().toStdString() << " (at line: " << item.lineNumber() << ")" << std::endl;
                        i->second.evolutions.erase(i->second.evolutions.cbegin()+index);
                        continue;
                    }
                    evolutionByLevel=true;
                }
                if(i->second.evolutions.at(index).type==Monster::EvolutionType_Trade)
                {
                    if(evolutionByTrade)
                    {
                        std::cerr << "Unable to open the xml file: " << file << ", the monster " << i->first << " have already evolution by trade: child.tagName(): " << item.tagName().toStdString() << " (at line: " << item.lineNumber() << ")" << std::endl;
                        i->second.evolutions.erase(i->second.evolutions.cbegin()+index);
                        continue;
                    }
                    evolutionByTrade=true;
                }
                if(i->second.evolutions.at(index).type==Monster::EvolutionType_Item)
                {
                    if(itemUse.find(i->second.evolutions.at(index).level)!=itemUse.cend())
                    {
                        std::cerr << "Unable to open the xml file: " << file << ", the monster " << i->first << " have already evolution with this item: child.tagName(): " << item.tagName().toStdString() << " (at line: " << item.lineNumber() << ")" << std::endl;
                        i->second.evolutions.erase(i->second.evolutions.cbegin()+index);
                        continue;
                    }
                    itemUse.insert(i->second.evolutions.at(index).level);
                }
                if(i->second.evolutions.at(index).evolveTo==i->first)
                {
                    std::cerr << "Unable to open the xml file: " << file << ", the monster " << i->first << " can't evolve into them self: child.tagName(): " << item.tagName().toStdString() << " (at line: " << item.lineNumber() << ")" << std::endl;
                    i->second.evolutions.erase(i->second.evolutions.cbegin()+index);
                    continue;
                }
                else if(monsters.find(i->second.evolutions.at(index).evolveTo)==monsters.cend())
                {
                    std::cerr << "Unable to open the xml file: " << file << ", the monster " << i->second.evolutions.at(index).evolveTo << " for the evolution of " << i->first << " can't be found: child.tagName(): " << item.tagName().toStdString() << " (at line: " << item.lineNumber() << ")" << std::endl;
                    i->second.evolutions.erase(i->second.evolutions.cbegin()+index);
                    continue;
                }
                index++;
            }
            ++i;
        }
        file_index++;
    }
    return monsters;
}

#ifndef EPOLLCATCHCHALLENGERSERVERNOGAMESERVER
std::unordered_map<uint16_t/*item*/, std::unordered_map<uint16_t/*monster*/,uint16_t/*evolveTo*/> > FightLoader::loadMonsterEvolutionItems(const std::unordered_map<uint16_t,Monster> &monsters)
{
    std::unordered_map<uint16_t, std::unordered_map<uint16_t,uint16_t> > evolutionItem;
    auto i=monsters.begin();
    while(i!=monsters.cend())
    {
        unsigned int index=0;
        while(index<i->second.evolutions.size())
        {
            if(i->second.evolutions.at(index).type==Monster::EvolutionType_Item)
                evolutionItem[i->second.evolutions.at(index).level][i->first]=i->second.evolutions.at(index).evolveTo;
            index++;
        }
        ++i;
    }
    return evolutionItem;
}
#endif

#ifndef EPOLLCATCHCHALLENGERSERVERNOGAMESERVER
std::unordered_map<uint16_t/*item*/, std::unordered_set<uint16_t/*monster*/> > FightLoader::loadMonsterItemToLearn(const std::unordered_map<uint16_t,Monster> &monsters,const std::unordered_map<uint16_t/*item*/, std::unordered_map<uint16_t/*monster*/,uint16_t/*evolveTo*/> > &evolutionItem)
{
    std::unordered_map<uint16_t/*item*/, std::unordered_set<uint16_t/*monster*/> > learnItem;
    auto i=monsters.cbegin();
    while(i!=monsters.cend())
    {
        auto j=i->second.learnByItem.cbegin();
        while(j!=i->second.learnByItem.cend())
        {
            if(evolutionItem.find(j->first)==evolutionItem.cend())
                learnItem[j->first].insert(i->first);
            else
                std::cerr << "The item " << j->first << " can't be used to learn because already used to evolv" << std::endl;
            ++j;
        }
        ++i;
    }
    return learnItem;
}
#endif

std::vector<PlayerMonster::PlayerSkill> FightLoader::loadDefaultAttack(const uint16_t &monsterId,const uint8_t &level, const std::unordered_map<uint16_t,Monster> &monsters, const std::unordered_map<uint16_t, Skill> &monsterSkills)
{
    std::vector<CatchChallenger::PlayerMonster::PlayerSkill> skills;
    std::vector<CatchChallenger::Monster::AttackToLearn> attack=monsters.at(monsterId).learn;
    unsigned int index=0;
    while(index<attack.size())
    {
        if(attack.at(index).learnAtLevel<=level)
        {
            CatchChallenger::PlayerMonster::PlayerSkill temp;
            temp.level=attack.at(index).learnSkillLevel;
            temp.skill=attack.at(index).learnSkill;
            temp.endurance=0;
            if(monsterSkills.find(temp.skill)!=monsterSkills.cend())
                if(temp.level<=monsterSkills.at(temp.skill).level.size() && temp.level>0)
                    temp.endurance=monsterSkills.at(temp.skill).level.at(temp.level-1).endurance;
            skills.push_back(temp);
        }
        index++;
    }
    while(skills.size()>4)
        skills.erase(skills.begin());
    return skills;
}

#ifndef EPOLLCATCHCHALLENGERSERVERNOGAMESERVER
std::unordered_map<uint16_t,BotFight> FightLoader::loadFight(const std::string &folder, const std::unordered_map<uint16_t,Monster> &monsters, const std::unordered_map<uint16_t, Skill> &monsterSkills, const std::unordered_map<uint16_t, Item> &items)
{
    std::unordered_map<uint16_t,BotFight> botFightList;
    QDir dir(QString::fromStdString(folder));
    QFileInfoList list=dir.entryInfoList(QStringList(),QDir::NoDotAndDotDot|QDir::Files);
    int index_file=0;
    while(index_file<list.size())
    {
        if(list.at(index_file).isFile())
        {
            const std::string &file=list.at(index_file).absoluteFilePath().toStdString();
            QDomDocument domDocument;
            //open and quick check the file
            #ifndef EPOLLCATCHCHALLENGERSERVER
            if(CommonDatapack::commonDatapack.xmlLoadedFile.find(file)!=CommonDatapack::commonDatapack.xmlLoadedFile.cend())
                domDocument=CommonDatapack::commonDatapack.xmlLoadedFile.at(file);
            else
            {
                #endif
                QFile xmlFile(QString::fromStdString(file));
                QByteArray xmlContent;
                if(!xmlFile.open(QIODevice::ReadOnly))
                {
                    std::cerr << "Unable to open the xml fight file: " << file << ", error: " << xmlFile.errorString().toStdString() << std::endl;
                    index_file++;
                    continue;
                }
                xmlContent=xmlFile.readAll();
                xmlFile.close();
                QString errorStr;
                int errorLine,errorColumn;
                if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
                {
                    std::cerr << "Unable to open the xml file: " << file << ", Parse error at line " << errorLine << ", column " << errorColumn << ": " << errorStr.toStdString() << std::endl;
                    index_file++;
                    continue;
                }
                #ifndef EPOLLCATCHCHALLENGERSERVER
                CommonDatapack::commonDatapack.xmlLoadedFile[file]=domDocument;
            }
            #endif
            QDomElement root = domDocument.documentElement();
            if(root.tagName()!="fights")
            {
                std::cerr << "Unable to open the xml file: " << file << ", \"fights\" root balise not found for the xml file" << std::endl;
                index_file++;
                continue;
            }

            //load the content
            bool ok;
            QDomElement item = root.firstChildElement("fight");
            while(!item.isNull())
            {
                if(item.isElement())
                {
                    if(item.hasAttribute("id"))
                    {
                        uint32_t id=item.attribute("id").toUInt(&ok);
                        if(ok)
                        {
                            bool entryValid=true;
                            CatchChallenger::BotFight botFight;
                            botFight.cash=0;
                            {
                                QDomElement monster = item.firstChildElement("monster");
                                while(entryValid && !monster.isNull())
                                {
                                    if(!monster.hasAttribute("id"))
                                        std::cerr << "Has not attribute \"id\": tagName(): " << monster.tagName().toStdString() << " (at line: " << monster.lineNumber() << ")" << std::endl;
                                    else if(!monster.isElement())
                                        std::cerr << "Is not an element: type: " << monster.attribute("type").toStdString() << " tagName(): " << monster.tagName().toStdString() << " (at line: " << monster.lineNumber() << ")" << std::endl;
                                    else
                                    {
                                        CatchChallenger::BotFight::BotFightMonster botFightMonster;
                                        botFightMonster.level=1;
                                        botFightMonster.id=monster.attribute("id").toUInt(&ok);
                                        if(ok)
                                        {
                                            if(monsters.find(botFightMonster.id)==monsters.cend())
                                            {
                                                entryValid=false;
                                                std::cerr << "Monster not found into the monster list: " << botFightMonster.id << " into the tagName(): " << monster.tagName().toStdString() << " (at line: " << monster.lineNumber() << ")" << std::endl;
                                                break;
                                            }
                                            if(monster.hasAttribute("level"))
                                            {
                                                botFightMonster.level=monster.attribute("level").toUShort(&ok);
                                                if(!ok)
                                                {
                                                    std::cerr << "The level is not a number: type: " << monster.attribute("type").toStdString() << " tagName(): " << monster.tagName().toStdString() << " (at line: " << monster.lineNumber() << ")" << std::endl;
                                                    botFightMonster.level=1;
                                                }
                                                if(botFightMonster.level<1)
                                                {
                                                    std::cerr << "Can't be 0 or negative: type: " << monster.attribute("type").toStdString() << " tagName(): " << monster.tagName().toStdString() << " (at line: " << monster.lineNumber() << ")" << std::endl;
                                                    botFightMonster.level=1;
                                                }
                                            }
                                            QDomElement attack = monster.firstChildElement("attack");
                                            while(entryValid && !attack.isNull())
                                            {
                                                uint8_t attackLevel=1;
                                                if(!attack.hasAttribute("id"))
                                                    std::cerr << "Has not attribute \"type\": tagName(): " << attack.tagName().toStdString() << " (at line: " << attack.lineNumber() << ")" << std::endl;
                                                else if(!attack.isElement())
                                                    std::cerr << "Is not an element: tagName(): " << attack.tagName().toStdString() << " (at line: " << attack.lineNumber() << ")" << std::endl;
                                                else
                                                {
                                                    uint32_t attackId=attack.attribute("id").toUInt(&ok);
                                                    if(ok)
                                                    {
                                                        if(monsterSkills.find(attackId)==monsterSkills.cend())
                                                        {
                                                            entryValid=false;
                                                            std::cerr << "Monster attack not found: %1 into the tagName(): " << attack.tagName().toStdString() << " (at line: " << attack.lineNumber() << ")" << std::endl;
                                                            break;
                                                        }
                                                        if(attack.hasAttribute("level"))
                                                        {
                                                            attackLevel=attack.attribute("level").toUShort(&ok);
                                                            if(!ok)
                                                            {
                                                                std::cerr << "The level is not a number: tagName(): " << attack.tagName().toStdString() << " (at line: " << attack.lineNumber() << ")" << std::endl;
                                                                entryValid=false;
                                                                break;
                                                            }
                                                            if(attackLevel<1)
                                                            {
                                                                std::cerr << "Can't be 0 or negative: tagName(): " << attack.tagName().toStdString() << " (at line: " << attack.lineNumber() << ")" << std::endl;
                                                                entryValid=false;
                                                                break;
                                                            }
                                                        }
                                                        if(attackLevel>monsterSkills.at(attackId).level.size())
                                                        {
                                                            std::cerr << "Level out of range: tagName(): " << attack.tagName().toStdString() << " (at line: " << attack.lineNumber() << ")" << std::endl;
                                                            entryValid=false;
                                                            break;
                                                        }
                                                        CatchChallenger::PlayerMonster::PlayerSkill botFightAttack;
                                                        botFightAttack.skill=attackId;
                                                        botFightAttack.level=attackLevel;
                                                        botFightMonster.attacks.push_back(botFightAttack);
                                                    }
                                                }
                                                attack = attack.nextSiblingElement("attack");
                                            }
                                            if(botFightMonster.attacks.empty())
                                                botFightMonster.attacks=loadDefaultAttack(botFightMonster.id,botFightMonster.level,monsters,monsterSkills);
                                            if(botFightMonster.attacks.empty())
                                            {
                                                std::cerr << "Empty attack list: tagName(): " << monster.tagName().toStdString() << " (at line: " << monster.lineNumber() << ")" << std::endl;
                                                entryValid=false;
                                                break;
                                            }
                                            botFight.monsters.push_back(botFightMonster);
                                        }
                                    }
                                    monster = monster.nextSiblingElement("monster");
                                }
                            }
                            {
                                QDomElement gain = item.firstChildElement("gain");
                                while(entryValid && !gain.isNull())
                                {
                                    if(gain.isElement())
                                    {
                                        if(gain.hasAttribute("cash"))
                                        {
                                            const uint32_t &cash=gain.attribute("cash").toUInt(&ok)*CommonSettingsServer::commonSettingsServer.rates_gold;
                                            if(ok)
                                                botFight.cash+=cash;
                                            else
                                                std::cerr << "Unable to open the xml file: " << file << ", unknow cash text: child.tagName(): " << item.tagName().toStdString() << " (at line: " << item.lineNumber() << ")" << std::endl;
                                        }
                                        else if(gain.hasAttribute("item"))
                                        {
                                            BotFight::Item itemVar;
                                            itemVar.quantity=1;
                                            itemVar.id=gain.attribute("item").toUInt(&ok);
                                            if(ok)
                                            {
                                                if(items.find(itemVar.id)!=items.cend())
                                                {
                                                    if(gain.hasAttribute("quantity"))
                                                    {
                                                        itemVar.quantity=gain.attribute("quantity").toUInt(&ok);
                                                        if(!ok || itemVar.quantity<1)
                                                        {
                                                            itemVar.quantity=1;
                                                            std::cerr << "Unable to open the xml file: " << file << ", quantity value is wrong: child.tagName(): " << item.tagName().toStdString() << " (at line: " << item.lineNumber() << ")" << std::endl;
                                                        }
                                                    }
                                                    botFight.items.push_back(itemVar);
                                                }
                                                else
                                                    std::cerr << "Unable to open the xml file: " << file << ", item not found: child.tagName(): " << item.tagName().toStdString() << " (at line: " << item.lineNumber() << ")" << std::endl;
                                            }
                                            else
                                                std::cerr << "Unable to open the xml file: " << file << ", unknow item id text: child.tagName(): " << item.tagName().toStdString() << " (at line: " << item.lineNumber() << ")" << std::endl;
                                        }
                                        else
                                            std::cerr << "unknown fight gain: file: " << file << " child.tagName(): " << gain.tagName().toStdString() << " (at line: " << gain.lineNumber() << ")" << std::endl;
                                    }
                                    else
                                        std::cerr << "Is not an element: file: " << file << ", type: " << gain.attribute("type").toStdString() << " child.tagName(): " << gain.tagName().toStdString() << " (at line: " << gain.lineNumber() << ")" << std::endl;
                                    gain = gain.nextSiblingElement("gain");
                                }
                            }
                            if(entryValid)
                            {
                                if(botFightList.find(id)==botFightList.cend())
                                {
                                    if(!botFight.monsters.empty())
                                        botFightList[id]=botFight;
                                    else
                                        std::cerr << "Monster list is empty to open the xml file: " << file << ", id already found: child.tagName(): " << item.tagName().toStdString() << " (at line: " << item.lineNumber() << ")" << std::endl;
                                }
                                else
                                    std::cerr << "Unable to open the xml file: " << file << ", id already found: child.tagName(): " << item.tagName().toStdString() << " (at line: " << item.lineNumber() << ")" << std::endl;
                            }
                        }
                        else
                            std::cerr << "Unable to open the xml file: " << file << ", id is not a number: child.tagName(): " << item.tagName().toStdString() << " (at line: " << item.lineNumber() << ")" << std::endl;
                    }
                }
                else
                    std::cerr << "Unable to open the xml file: " << file << ", is not an element: child.tagName(): " << item.tagName().toStdString() << " (at line: " << item.lineNumber() << ")" << std::endl;
                item = item.nextSiblingElement("fight");
            }
            index_file++;
        }
    }
    return botFightList;
}
#endif

std::unordered_map<uint16_t,Skill> FightLoader::loadMonsterSkill(const std::string &folder
                                                   #ifndef CATCHCHALLENGER_CLASS_MASTER
                                                   , const std::unordered_map<uint8_t, Buff> &monsterBuffs
                                                   , const std::vector<Type> &types
                                                   #endif
                                                   )
{
    std::unordered_map<uint16_t,Skill> monsterSkills;
    QDir dir(QString::fromStdString(folder));
    QFileInfoList fileList=dir.entryInfoList(QDir::Files|QDir::NoDotAndDotDot);
    int file_index=0;
    while(file_index<fileList.size())
    {
        if(!fileList.at(file_index).isFile())
        {
            file_index++;
            continue;
        }
        const std::string &file=fileList.at(file_index).absoluteFilePath().toStdString();
        if(!stringEndsWith(file,FightLoader::text_dotxml))
        {
            file_index++;
            continue;
        }
        #ifndef CATCHCHALLENGER_CLASS_MASTER
        std::unordered_map<std::string,uint8_t> typeNameToId;
        {
            unsigned int index=0;
            while(index<types.size())
            {
                typeNameToId[types.at(index).name]=index;
                index++;
            }
        }
        #endif // CATCHCHALLENGER_CLASS_MASTER
        QDomDocument domDocument;
        //open and quick check the file
        #ifndef EPOLLCATCHCHALLENGERSERVER
        if(CommonDatapack::commonDatapack.xmlLoadedFile.find(file)!=CommonDatapack::commonDatapack.xmlLoadedFile.cend())
            domDocument=CommonDatapack::commonDatapack.xmlLoadedFile.at(file);
        else
        {
            #endif
            QFile xmlFile(QString::fromStdString(file));
            QByteArray xmlContent;
            if(!xmlFile.open(QIODevice::ReadOnly))
            {
                std::cerr << "Unable to open the xml skill monster " << file << ", error: " << xmlFile.errorString().toStdString() << std::endl;
                file_index++;
                continue;
            }
            xmlContent=xmlFile.readAll();
            xmlFile.close();
            QString errorStr;
            int errorLine,errorColumn;
            if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
            {
                std::cerr << "Unable to open the xml file: " << file << ", Parse error at line " << errorLine << ", column " << errorColumn << ": " << errorStr.toStdString() << std::endl;
                file_index++;
                continue;
            }
            #ifndef EPOLLCATCHCHALLENGERSERVER
            CommonDatapack::commonDatapack.xmlLoadedFile[file]=domDocument;
        }
        #endif
        QDomElement root = domDocument.documentElement();
        if(root.tagName()!="skills")
        {
            std::cerr << "Unable to open the xml file: " << file << ", \"list\" root balise not found for the xml file" << std::endl;
            file_index++;
            continue;
        }

        //load the content
        bool ok;
        #ifndef CATCHCHALLENGER_CLASS_MASTER
        bool ok2;
        #endif
        QDomElement item = root.firstChildElement("skill");
        while(!item.isNull())
        {
            if(item.isElement())
            {
                if(item.hasAttribute("id"))
                {
                    uint32_t id=item.attribute("id").toUInt(&ok);
                    if(ok && monsterSkills.find(id)!=monsterSkills.cend())
                        std::cerr << "Unable to open the xml file: " << file << ", id already found: child.tagName(): " << item.tagName().toStdString() << " (at line: " << item.lineNumber() << ")" << std::endl;
                    else if(ok)
                    {
                        std::unordered_map<uint8_t,Skill::SkillList> levelDef;
                        QDomElement effect = item.firstChildElement("effect");
                        if(!effect.isNull())
                        {
                            if(effect.isElement())
                            {
                                QDomElement level = effect.firstChildElement("level");
                                while(!level.isNull())
                                {
                                    if(level.isElement())
                                    {
                                        if(level.hasAttribute("number"))
                                        {
                                            uint32_t sp=0;
                                            if(level.hasAttribute("sp"))
                                            {
                                                sp=level.attribute("sp").toUShort(&ok);
                                                if(!ok)
                                                {
                                                    std::cerr << "Unable to open the xml file: " << file << ", sp is not number: child.tagName(): " << level.tagName().toStdString() << " (at line: " << level.lineNumber() << ")" << std::endl;
                                                    sp=0;
                                                }
                                            }
                                            uint8_t endurance=40;
                                            if(level.hasAttribute("endurance"))
                                            {
                                                endurance=level.attribute("endurance").toUShort(&ok);
                                                if(!ok)
                                                {
                                                    std::cerr << "Unable to open the xml file: " << file << ", endurance is not number: child.tagName(): " << level.tagName().toStdString() << " (at line: " << level.lineNumber() << ")" << std::endl;
                                                    endurance=40;
                                                }
                                                if(endurance<1)
                                                {
                                                    std::cerr << "Unable to open the xml file: " << file << ", endurance lower than 1: child.tagName(): " << level.tagName().toStdString() << " (at line: " << level.lineNumber() << ")" << std::endl;
                                                    endurance=40;
                                                }
                                            }
                                            uint8_t number;
                                            if(ok)
                                                number=level.attribute("number").toUShort(&ok);
                                            if(ok)
                                            {
                                                levelDef[number].sp_to_learn=sp;
                                                levelDef[number].endurance=endurance;
                                                if(number>0)
                                                {
                                                    #ifndef CATCHCHALLENGER_CLASS_MASTER
                                                    {
                                                        QDomElement life = level.firstChildElement("life");
                                                        while(!life.isNull())
                                                        {
                                                            if(life.isElement())
                                                            {
                                                                Skill::Life effect;
                                                                if(life.hasAttribute("applyOn"))
                                                                {
                                                                    if(life.attribute("applyOn")=="aloneEnemy")
                                                                        effect.effect.on=ApplyOn_AloneEnemy;
                                                                    else if(life.attribute("applyOn")=="themself")
                                                                        effect.effect.on=ApplyOn_Themself;
                                                                    else if(life.attribute("applyOn")=="allEnemy")
                                                                        effect.effect.on=ApplyOn_AllEnemy;
                                                                    else if(life.attribute("applyOn")=="allAlly")
                                                                        effect.effect.on=ApplyOn_AllAlly;
                                                                    else if(life.attribute("applyOn")=="nobody")
                                                                        effect.effect.on=ApplyOn_Nobody;
                                                                    else
                                                                    {
                                                                        std::cerr << "Unable to open the xml file: " << file << ", applyOn tag wrong " << life.attribute("applyOn").toStdString() << ": child.tagName(): " << item.tagName().toStdString() << " (at line: " << item.lineNumber() << ")" << std::endl;
                                                                        effect.effect.on=ApplyOn_AloneEnemy;
                                                                    }
                                                                }
                                                                else
                                                                    effect.effect.on=ApplyOn_AloneEnemy;
                                                                std::string text;
                                                                if(life.hasAttribute("quantity"))
                                                                    text=life.attribute("quantity").toStdString();
                                                                if(stringEndsWith(text,"%"))
                                                                    effect.effect.type=QuantityType_Percent;
                                                                else
                                                                    effect.effect.type=QuantityType_Quantity;
                                                                stringreplaceOne(text,"%","");
                                                                stringreplaceOne(text,"+","");
                                                                effect.effect.quantity=stringtoint32(text,&ok);
                                                                effect.success=100;
                                                                if(life.hasAttribute("success"))
                                                                {
                                                                    std::string success=life.attribute("success").toStdString();
                                                                    stringreplaceOne(success,"%","");
                                                                    effect.success=stringtouint8(success,&ok2);
                                                                    if(!ok2)
                                                                    {
                                                                        std::cerr << "Unable to open the xml file: " << file << ", success wrong corrected to 100%: child.tagName(): " << item.tagName().toStdString() << " (at line: " << item.lineNumber() << ")" << std::endl;
                                                                        effect.success=100;
                                                                    }
                                                                }
                                                                if(ok)
                                                                {
                                                                    if(effect.effect.quantity!=0)
                                                                        levelDef[number].life.push_back(effect);
                                                                }
                                                                else
                                                                    std::cerr << "Unable to open the xml file: " << file << ", " << text << " is not a number: child.tagName(): " << item.tagName().toStdString() << " (at line: " << item.lineNumber() << ")" << std::endl;
                                                            }
                                                            life = life.nextSiblingElement("life");
                                                        }
                                                    }
                                                    #endif // CATCHCHALLENGER_CLASS_MASTER
                                                    #ifndef CATCHCHALLENGER_CLASS_MASTER
                                                    {
                                                        QDomElement buff = level.firstChildElement("buff");
                                                        while(!buff.isNull())
                                                        {
                                                            if(buff.isElement())
                                                            {
                                                                if(buff.hasAttribute("id"))
                                                                {
                                                                    uint32_t idBuff=buff.attribute("id").toUInt(&ok);
                                                                    if(ok)
                                                                    {
                                                                        Skill::Buff effect;
                                                                        if(buff.hasAttribute("applyOn"))
                                                                        {
                                                                            if(buff.attribute("applyOn")=="aloneEnemy")
                                                                                effect.effect.on=ApplyOn_AloneEnemy;
                                                                            else if(buff.attribute("applyOn")=="themself")
                                                                                effect.effect.on=ApplyOn_Themself;
                                                                            else if(buff.attribute("applyOn")=="allEnemy")
                                                                                effect.effect.on=ApplyOn_AllEnemy;
                                                                            else if(buff.attribute("applyOn")=="allAlly")
                                                                                effect.effect.on=ApplyOn_AllAlly;
                                                                            else if(buff.attribute("applyOn")=="nobody")
                                                                                effect.effect.on=ApplyOn_Nobody;
                                                                            else
                                                                            {
                                                                                std::cerr << "Unable to open the xml file: " << file << ", applyOn tag wrong " << buff.attribute("applyOn").toStdString() << ": child.tagName(): " << item.tagName().toStdString() << " (at line: " << item.lineNumber() << ")" << std::endl;
                                                                                effect.effect.on=ApplyOn_AloneEnemy;
                                                                            }
                                                                        }
                                                                        else
                                                                            effect.effect.on=ApplyOn_AloneEnemy;
                                                                        if(monsterBuffs.find(idBuff)==monsterBuffs.cend())
                                                                            std::cerr << "Unable to open the xml file: " << file << ", this buff id is not found: " << idBuff << ": child.tagName(): " << item.tagName().toStdString() << " (at line: " << item.lineNumber() << ")" << std::endl;
                                                                        else
                                                                        {
                                                                            effect.effect.level=1;
                                                                            ok2=true;
                                                                            if(buff.hasAttribute("level"))
                                                                            {
                                                                                std::string level=buff.attribute("level").toStdString();
                                                                                effect.effect.level=stringtouint8(level,&ok2);
                                                                                if(!ok2)
                                                                                    std::cerr << "Unable to open the xml file: " << file << ", level wrong: " << buff.attribute("level").toStdString() << " child.tagName(): " << item.tagName().toStdString() << " (at line: " << item.lineNumber() << ")" << std::endl;
                                                                                if(effect.effect.level<=0)
                                                                                {
                                                                                    ok2=false;
                                                                                    std::cerr << "Unable to open the xml file: " << file << ", level need be egal or greater than 1: child.tagName(): " << item.tagName().toStdString() << " (at line: " << item.lineNumber() << ")" << std::endl;
                                                                                }
                                                                            }
                                                                            if(ok2)
                                                                            {
                                                                                if(monsterBuffs.at(idBuff).level.size()<effect.effect.level)
                                                                                    std::cerr << "Unable to open the xml file: " << file << ", level needed: " << effect.effect.level << ", level max found: " << monsterBuffs.at(idBuff).level.size() << ": child.tagName(): " << item.tagName().toStdString() << " (at line: " << item.lineNumber() << ")" << std::endl;
                                                                                else
                                                                                {
                                                                                    effect.effect.buff=idBuff;
                                                                                    effect.success=100;
                                                                                    if(buff.hasAttribute("success"))
                                                                                    {
                                                                                        std::string success=buff.attribute("success").toStdString();
                                                                                        stringreplaceOne(success,"%","");
                                                                                        effect.success=stringtouint8(success,&ok2);
                                                                                        if(!ok2)
                                                                                        {
                                                                                            std::cerr << "Unable to open the xml file: " << file << ", success wrong corrected to 100%: child.tagName(): " << item.tagName().toStdString() << " (at line: " << item.lineNumber() << ")" << std::endl;
                                                                                            effect.success=100;
                                                                                        }
                                                                                    }
                                                                                    levelDef[number].buff.push_back(effect);
                                                                                }
                                                                            }
                                                                        }
                                                                    }
                                                                    else
                                                                        std::cerr << "Unable to open the xml file: " << file << ", have not tag id: child.tagName(): " << item.tagName().toStdString() << " (at line: " << item.lineNumber() << ")" << std::endl;
                                                                }
                                                                else
                                                                    std::cerr << "Unable to open the xml file: " << file << ", have not tag id: child.tagName(): " << item.tagName().toStdString() << " (at line: " << item.lineNumber() << ")" << std::endl;
                                                            }
                                                            buff = buff.nextSiblingElement("buff");
                                                        }
                                                    }
                                                    #endif // CATCHCHALLENGER_CLASS_MASTER
                                                }
                                                else
                                                    std::cerr << "Unable to open the xml file: " << file << ", level need be egal or greater than 1: child.tagName(): " << item.tagName().toStdString() << " (at line: " << item.lineNumber() << ")" << std::endl;
                                            }
                                            else
                                                std::cerr << "Unable to open the xml file: " << file << ", number tag is not a number: child.tagName(): " << item.tagName().toStdString() << " (at line: " << item.lineNumber() << ")" << std::endl;
                                        }
                                    }
                                    else
                                        std::cerr << "Unable to open the xml file: " << file << ", level balise is not an element: child.tagName(): " << item.tagName().toStdString() << " (at line: " << item.lineNumber() << ")" << std::endl;
                                    level = level.nextSiblingElement("level");
                                }
                                if(levelDef.size()==0)
                                    std::cerr << "Unable to open the xml file: " << file << ", 0 level found: child.tagName(): " << item.tagName().toStdString() << " (at line: " << item.lineNumber() << ")" << std::endl;
                                else
                                {
                                    monsterSkills[id].type=255;
                                    #ifndef CATCHCHALLENGER_CLASS_MASTER
                                    if(item.hasAttribute("type"))
                                    {
                                        if(typeNameToId.find(item.attribute("type").toStdString())!=typeNameToId.cend())
                                            monsterSkills[id].type=typeNameToId.at(item.attribute("type").toStdString());
                                        else
                                            std::cerr << "Unable to open the xml file: " << file << ", type not found: " << item.attribute("type").toStdString() << ": child.tagName(): " << item.tagName().toStdString() << " (at line: " << item.lineNumber() << ")" << std::endl;
                                    }
                                    #endif // CATCHCHALLENGER_CLASS_MASTER
                                }
                                //order by level to learn
                                uint8_t index=1;
                                while(levelDef.find(index)!=levelDef.cend())
                                {
                                    /*if(levelDef.value(index).buff.empty() && levelDef.value(index).life.empty())
                                        std::cerr << "Unable to open the xml file: " << file << ", no effect loaded for skill %4 at level %5, missing level to continue: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()).arg(id).arg(index));*/
                                    monsterSkills[id].level.push_back(levelDef.at(index));
                                    levelDef.erase(index);
                                    index++;
                                }
                                if(levelDef.size()>0)
                                    std::cerr << "Unable to open the xml file: " << file << ", level up to " << index << " loaded, missing level to continue: child.tagName(): " << item.tagName().toStdString() << " (at line: " << item.lineNumber() << ")" << std::endl;
                            }
                            else
                                std::cerr << "Unable to open the xml file: " << file << ", effect balise is not an element: child.tagName(): " << item.tagName().toStdString() << " (at line: " << item.lineNumber() << ")" << std::endl;
                        }
                        /*else
                            std::cerr << "Unable to open the xml file: " << file << ", have not effect balise: child.tagName(): " << item.tagName().toStdString() << " (at line: " << item.lineNumber() << ")" << std::endl;*/
                    }
                    else
                        std::cerr << "Unable to open the xml file: " << file << ", id is not a number: child.tagName(): " << item.tagName().toStdString() << " (at line: " << item.lineNumber() << ")" << std::endl;
                }
                else
                    std::cerr << "Unable to open the xml file: " << file << ", have not the skill id: child.tagName(): " << item.tagName().toStdString() << " (at line: " << item.lineNumber() << ")" << std::endl;
            }
            else
                std::cerr << "Unable to open the xml file: " << file << ", is not an element: child.tagName(): " << item.tagName().toStdString() << " (at line: " << item.lineNumber() << ")" << std::endl;
            item = item.nextSiblingElement("skill");
        }
        //check the default attack
        if(monsterSkills.find(0)==monsterSkills.cend())
            std::cerr << "Warning: no default monster attack if no more attack" << std::endl;
        else if(monsterSkills.at(0).level.empty())
        {
            monsterSkills.erase(0);
            std::cerr << "Warning: no level for default monster attack if no more attack" << std::endl;
        }
        else
        {
            #ifndef CATCHCHALLENGER_CLASS_MASTER
            if(monsterSkills.at(0).level.front().life.empty())
            {
                monsterSkills.erase(0);
                std::cerr << "Warning: no life effect for the default attack" << std::endl;
            }
            else
            {
                const unsigned int &list_size=monsterSkills.at(0).level.front().life.size();
                unsigned int index=0;
                while(index<list_size)
                {
                    const Skill::Life &life=monsterSkills.at(0).level.front().life.at(index);
                    if(life.success==100 && life.effect.on==ApplyOn_AloneEnemy && life.effect.quantity<0)
                        break;
                    index++;
                }
                if(index==list_size)
                {
                    const Skill::Life &life=monsterSkills.at(0).level.front().life.front();
                    monsterSkills.erase(0);
                    std::cerr << "Warning: no valid life effect for the default attack (id: 0): success=100%: " << life.success << ", on=ApplyOn_AloneEnemy: " << life.effect.on << ", quantity<0: " << life.effect.quantity << " for skill" << std::endl;
                }
            }
            #endif
        }
        file_index++;
    }
    return monsterSkills;
}

#ifndef EPOLLCATCHCHALLENGERSERVERNOGAMESERVER
std::unordered_map<uint8_t,Buff> FightLoader::loadMonsterBuff(const std::string &folder)
{
    std::unordered_map<uint8_t,Buff> monsterBuffs;
    QDir dir(QString::fromStdString(folder));
    QFileInfoList fileList=dir.entryInfoList(QDir::Files|QDir::NoDotAndDotDot);
    int file_index=0;
    while(file_index<fileList.size())
    {
        if(!fileList.at(file_index).isFile())
        {
            file_index++;
            continue;
        }
        const std::string &file=fileList.at(file_index).absoluteFilePath().toStdString();
        if(!stringEndsWith(file,FightLoader::text_dotxml))
        {
            file_index++;
            continue;
        }
        QDomDocument domDocument;
        //open and quick check the file
        #ifndef EPOLLCATCHCHALLENGERSERVER
        if(CommonDatapack::commonDatapack.xmlLoadedFile.find(file)!=CommonDatapack::commonDatapack.xmlLoadedFile.cend())
            domDocument=CommonDatapack::commonDatapack.xmlLoadedFile.at(file);
        else
        {
            #endif
            QFile xmlFile(QString::fromStdString(file));
            QByteArray xmlContent;
            if(!xmlFile.open(QIODevice::ReadOnly))
            {
                std::cerr << "Unable to open the xml buff monster file: " << file << ", error: " << xmlFile.errorString().toStdString() << std::endl;
                file_index++;
                continue;
            }
            xmlContent=xmlFile.readAll();
            xmlFile.close();
            QString errorStr;
            int errorLine,errorColumn;
            if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
            {
                std::cerr << "Unable to open the xml file: " << file << ", Parse error at line " << errorLine << ", column " << errorColumn << ": " << errorStr.toStdString() << std::endl;
                file_index++;
                continue;
            }
            #ifndef EPOLLCATCHCHALLENGERSERVER
            CommonDatapack::commonDatapack.xmlLoadedFile[file]=domDocument;
        }
        #endif
        QDomElement root = domDocument.documentElement();
        if(root.tagName()!="buffs")
        {
            std::cerr << "Unable to open the xml file: " << file << ", \"list\" root balise not found for the xml file" << std::endl;
            file_index++;
            continue;
        }

        //load the content
        bool ok;
        QDomElement item = root.firstChildElement("buff");
        while(!item.isNull())
        {
            if(item.isElement())
            {
                if(item.hasAttribute("id"))
                {
                    uint32_t id=item.attribute("id").toUInt(&ok);
                    if(ok && monsterBuffs.find(id)!=monsterBuffs.cend())
                        std::cerr << "Unable to open the xml file: " << file << ", id already found: child.tagName(): " << item.tagName().toStdString() << " (at line: " << item.lineNumber() << ")" << std::endl;
                    else if(ok)
                    {
                        Buff::Duration general_duration=Buff::Duration_ThisFight;
                        uint8_t general_durationNumberOfTurn=0;
                        float general_capture_bonus=1.0;
                        if(item.hasAttribute("capture_bonus"))
                        {
                           general_capture_bonus=item.attribute("capture_bonus").toFloat(&ok);
                            if(!ok)
                            {
                                std::cerr << "Unable to open the xml file: " << file << ", capture_bonus is not a number: child.tagName(): " << item.tagName().toStdString() << " (at line: " << item.lineNumber() << ")" << std::endl;
                                general_capture_bonus=1.0;
                            }
                        }
                        if(item.hasAttribute("duration"))
                        {
                            if(item.attribute("duration")=="Always")
                                general_duration=Buff::Duration_Always;
                            else if(item.attribute("duration")=="NumberOfTurn")
                            {
                                if(item.hasAttribute("durationNumberOfTurn"))
                                {
                                    general_durationNumberOfTurn=item.attribute("durationNumberOfTurn").toUShort(&ok);
                                    if(!ok)
                                    {
                                        std::cerr << "Unable to open the xml file: " << file << ", durationNumberOfTurn is not a number: child.tagName(): " << item.tagName().toStdString() << " (at line: " << item.lineNumber() << ")" << std::endl;
                                        general_durationNumberOfTurn=3;
                                    }
                                    if(general_durationNumberOfTurn<=0)
                                    {
                                        std::cerr << "Unable to open the xml file: " << file << ", durationNumberOfTurn is egal to 0: child.tagName(): " << item.tagName().toStdString() << " (at line: " << item.lineNumber() << ")" << std::endl;
                                        general_durationNumberOfTurn=3;
                                    }
                                }
                                else
                                    general_durationNumberOfTurn=3;
                                general_duration=Buff::Duration_NumberOfTurn;
                            }
                            else if(item.attribute("duration")=="ThisFight")
                                general_duration=Buff::Duration_ThisFight;
                            else
                            {
                                std::cerr << "Unable to open the xml file: " << file << ", attribute duration have wrong value \"" << item.attribute("duration").toStdString() << "\" is not a number: child.tagName(): " << item.tagName().toStdString() << " (at line: " << item.lineNumber() << ")" << std::endl;
                                general_duration=Buff::Duration_ThisFight;
                            }
                        }
                        std::unordered_map<uint8_t,Buff::GeneralEffect> levelDef;
                        QDomElement effect = item.firstChildElement("effect");
                        if(!effect.isNull())
                        {
                            if(effect.isElement())
                            {
                                QDomElement level = effect.firstChildElement("level");
                                while(!level.isNull())
                                {
                                    if(level.isElement())
                                    {
                                        if(level.hasAttribute("number"))
                                        {
                                            uint8_t number=level.attribute("number").toUShort(&ok);
                                            if(ok)
                                            {
                                                if(number>0)
                                                {
                                                    Buff::Duration duration=general_duration;
                                                    uint8_t durationNumberOfTurn=general_durationNumberOfTurn;
                                                    float capture_bonus=general_capture_bonus;
                                                    if(item.hasAttribute("capture_bonus"))
                                                    {
                                                       capture_bonus=item.attribute("capture_bonus").toFloat(&ok);
                                                        if(!ok)
                                                        {
                                                            std::cerr << "Unable to open the xml file: " << file << ", capture_bonus is not a number: child.tagName(): " << item.tagName().toStdString() << " (at line: " << item.lineNumber() << ")" << std::endl;
                                                            capture_bonus=general_capture_bonus;
                                                        }
                                                    }
                                                    if(item.hasAttribute("duration"))
                                                    {
                                                        if(item.attribute("duration")=="Always")
                                                            duration=Buff::Duration_Always;
                                                        else if(item.attribute("duration")=="NumberOfTurn")
                                                        {
                                                            if(item.hasAttribute("durationNumberOfTurn"))
                                                            {
                                                                durationNumberOfTurn=item.attribute("durationNumberOfTurn").toUShort(&ok);
                                                                if(!ok)
                                                                {
                                                                    std::cerr << "Unable to open the xml file: " << file << ", durationNumberOfTurn is not a number: child.tagName(): " << item.tagName().toStdString() << " (at line: " << item.lineNumber() << ")" << std::endl;
                                                                    durationNumberOfTurn=general_durationNumberOfTurn;
                                                                }
                                                            }
                                                            else
                                                                durationNumberOfTurn=general_durationNumberOfTurn;
                                                            duration=Buff::Duration_NumberOfTurn;
                                                        }
                                                        else if(item.attribute("duration")=="ThisFight")
                                                            duration=Buff::Duration_ThisFight;
                                                        else
                                                        {
                                                            std::cerr << "Unable to open the xml file: " << file << ", attribute duration have wrong value \"" << item.attribute("duration").toStdString() << "\" is not a number: child.tagName(): " << item.tagName().toStdString() << " (at line: " << item.lineNumber() << ")" << std::endl;
                                                            duration=general_duration;
                                                        }
                                                    }
                                                    levelDef[number].duration=duration;
                                                    levelDef[number].durationNumberOfTurn=durationNumberOfTurn;
                                                    levelDef[number].capture_bonus=capture_bonus;



                                                    QDomElement inFight = level.firstChildElement("inFight");
                                                    while(!inFight.isNull())
                                                    {
                                                        if(inFight.isElement())
                                                        {
                                                            ok=true;
                                                            Buff::Effect effect;
                                                            std::string text;
                                                            if(inFight.hasAttribute("hp"))
                                                            {
                                                                text=inFight.attribute("hp").toStdString();
                                                                effect.on=Buff::Effect::EffectOn_HP;
                                                            }
                                                            else if(inFight.hasAttribute("defense"))
                                                            {
                                                                text=inFight.attribute("defense").toStdString();
                                                                effect.on=Buff::Effect::EffectOn_Defense;
                                                            }
                                                            else if(inFight.hasAttribute("attack"))
                                                            {
                                                                text=inFight.attribute("attack").toStdString();
                                                                effect.on=Buff::Effect::EffectOn_Attack;
                                                            }
                                                            else
                                                            {
                                                                ok=false;
                                                                std::cerr << "Unable to open the xml file: " << file << ", not know attribute balise: child.tagName(): " << inFight.tagName().toStdString() << " (at line: " << inFight.lineNumber() << ")" << std::endl;
                                                            }
                                                            if(ok)
                                                            {
                                                                if(stringEndsWith(text,"%"))
                                                                    effect.type=QuantityType_Percent;
                                                                else
                                                                    effect.type=QuantityType_Quantity;
                                                                stringreplaceOne(text,"%","");
                                                                stringreplaceOne(text,"+","");
                                                                effect.quantity=stringtoint32(text,&ok);
                                                                if(ok)
                                                                    levelDef[number].fight.push_back(effect);
                                                                else
                                                                    std::cerr << "Unable to open the xml file: " << file << ", \"" << text << "\" something is wrong, or is not a number, or not into hp or defense balise: child.tagName(): " << inFight.tagName().toStdString() << " (at line: " << inFight.lineNumber() << ")" << std::endl;
                                                            }
                                                        }
                                                        inFight = inFight.nextSiblingElement("inFight");
                                                    }
                                                    QDomElement inWalk = level.firstChildElement("inWalk");
                                                    while(!inWalk.isNull())
                                                    {
                                                        if(inWalk.isElement())
                                                        {
                                                            if(inWalk.hasAttribute("steps"))
                                                            {
                                                                uint32_t steps=inWalk.attribute("steps").toUInt(&ok);
                                                                if(ok)
                                                                {
                                                                    Buff::EffectInWalk effect;
                                                                    effect.steps=steps;
                                                                    std::string text;
                                                                    if(inWalk.hasAttribute("hp"))
                                                                    {
                                                                        text=inWalk.attribute("hp").toStdString();
                                                                        effect.effect.on=Buff::Effect::EffectOn_HP;
                                                                    }
                                                                    else if(inWalk.hasAttribute("defense"))
                                                                    {
                                                                        text=inWalk.attribute("defense").toStdString();
                                                                        effect.effect.on=Buff::Effect::EffectOn_Defense;
                                                                    }
                                                                    else
                                                                        std::cerr << "Unable to open the xml file: " << file << ", not action found: child.tagName(): " << item.tagName().toStdString() << " (at line: " << item.lineNumber() << ")" << std::endl;
                                                                    if(stringEndsWith(text,"%"))
                                                                        effect.effect.type=QuantityType_Percent;
                                                                    else
                                                                        effect.effect.type=QuantityType_Quantity;
                                                                    stringreplaceOne(text,"%","");
                                                                    stringreplaceOne(text,"+","");
                                                                    effect.effect.quantity=stringtoint32(text,&ok);
                                                                    if(ok)
                                                                        levelDef[number].walk.push_back(effect);
                                                                    else
                                                                        std::cerr << "Unable to open the xml file: " << file << ", " << text << " is not a number: child.tagName(): " << item.tagName().toStdString() << " (at line: " << item.lineNumber() << ")" << std::endl;
                                                                }
                                                                else
                                                                    std::cerr << "Unable to open the xml file: " << file << ", have not tag steps: child.tagName(): " << item.tagName().toStdString() << " (at line: " << item.lineNumber() << ")" << std::endl;
                                                            }
                                                            else
                                                                std::cerr << "Unable to open the xml file: " << file << ", have not tag steps: child.tagName(): " << item.tagName().toStdString() << " (at line: " << item.lineNumber() << ")" << std::endl;
                                                        }
                                                        inWalk = inWalk.nextSiblingElement("inWalk");
                                                    }
                                                }
                                                else
                                                    std::cerr << "Unable to open the xml file: " << file << ", level need be egal or greater than 1: child.tagName(): " << item.tagName().toStdString() << " (at line: " << item.lineNumber() << ")" << std::endl;
                                            }
                                            else
                                                std::cerr << "Unable to open the xml file: " << file << ", number tag is not a number: child.tagName(): " << item.tagName().toStdString() << " (at line: " << item.lineNumber() << ")" << std::endl;
                                        }
                                    }
                                    else
                                        std::cerr << "Unable to open the xml file: " << file << ", level balise is not an element: child.tagName(): " << item.tagName().toStdString() << " (at line: " << item.lineNumber() << ")" << std::endl;
                                    level = level.nextSiblingElement("level");
                                }
                                uint8_t index=1;
                                while(levelDef.find(index)!=levelDef.cend())
                                {
                                    /*if(levelDef.value(index).fight.empty() && levelDef.value(index).walk.empty())
                                        std::cerr << "Unable to open the xml file: " << file << ", no effect loaded for buff %4 at level %5, missing level to continue: child.tagName(): %2 (at line: %3)").arg(file).arg(item.tagName()).arg(item.lineNumber()).arg(id).arg(index));*/
                                    monsterBuffs[id].level.push_back(levelDef.at(index));
                                    levelDef.erase(index);
                                    index++;
                                }
                                if(levelDef.size()>0)
                                    std::cerr << "Unable to open the xml file: " << file << ", level up to " << index << " loaded, missing level to continue: child.tagName(): " << item.tagName().toStdString() << " (at line: " << item.lineNumber() << ")" << std::endl;
                            }
                            else
                                std::cerr << "Unable to open the xml file: " << file << ", effect balise is not an element: child.tagName(): " << item.tagName().toStdString() << " (at line: " << item.lineNumber() << ")" << std::endl;
                        }
                        else
                            std::cerr << "Unable to open the xml file: " << file << ", have not effet balise: child.tagName(): " << item.tagName().toStdString() << " (at line: " << item.lineNumber() << ")" << std::endl;
                    }
                    else
                        std::cerr << "Unable to open the xml file: " << file << ", id is not a number: child.tagName(): " << item.tagName().toStdString() << " (at line: " << item.lineNumber() << ")" << std::endl;
                }
                else
                    std::cerr << "Unable to open the xml file: " << file << ", have not the buff id: child.tagName(): " << item.tagName().toStdString() << " (at line: " << item.lineNumber() << ")" << std::endl;
            }
            else
                std::cerr << "Unable to open the xml file: " << file << ", is not an element: child.tagName(): " << item.tagName().toStdString() << " (at line: " << item.lineNumber() << ")" << std::endl;
            item = item.nextSiblingElement("buff");
        }
        file_index++;
    }
    return monsterBuffs;
}
#endif
