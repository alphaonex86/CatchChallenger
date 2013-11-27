#include <QCoreApplication>
#include <QDir>
#include <QRegularExpression>
#include <QDomElement>
#include <QDomDocument>
#include <QDebug>

#define LANG "fr"

QString datapackPath;

struct MonsterInformations
{
    QString name;
    QString capture_rate;
    QString description;
    QString backPath;
    QString empreinte;
    quint32 forme;
};
QHash<quint32,MonsterInformations> monsterList;

struct MonsterForme
{
    QString image;
    QString name;
};
QHash<quint32,MonsterForme> monsterFormeList;

struct Item
{
    QString image;
    QString name;
    QString description;
    quint32 price;
    QString englishName;
};
QHash<QString,Item> itemList;

struct Skill
{
    QString name;
    QString description;
};
QHash<QString,Skill> skillList;

bool loadMonsterInformations(const QString &path,const QString &data)
{
    MonsterInformations monsterInformations;

    if(!data.contains("rotation nationale\">"))
        return false;
    if(!data.contains(QRegularExpression("<h1 id=\"firstHeading\" class=\"firstHeading\" lang=\"fr\"><span dir=\"auto\">([^<]+)</span></h1>")))
        return false;
    if(!data.contains(QRegularExpression("<big><span class=\"explain\" title=\"Numérotation nationale\">. ([0-9]+)</span></big>")))
        return false;
    if(!data.contains(QRegularExpression("<tr>[\n\r\t ]*<th colspan=\"2\">[\n\r\t ]*<a [^>]+>Taux de capture</a>[\n\r\t ]*</th>[\n\r\t ]*<td colspan=\"3\">[\n\r\t ]+([0-9]+)[\n\r\t ]+</td></tr>")))
        return false;
    if(!data.contains(QRegularExpression("<dt><a [^>]+>Pokémon Émeraude</a>[\n\r\t ]*</dt><dd>[\n\r\t ]*([^\n\r\t<]+)[\n\r\t ]*</dd>")))
        return false;
    if(!data.contains(QRegularExpression("<img alt=\"Sprite[^\"]+\" src=\"([^\"]+Sprite_[1-5]_dos_[0-9]{1,3}(_[mf])?\\.png)\" width=\"96\" height=\"96\" />")))
        return false;
    if(!data.contains(QRegularExpression("<tr>[\n\r\t ]*<th colspan=\"2\">[\n\r\t ]+Empreinte[\n\r\t ]+</th>[\n\r\t ]*<td colspan=\"3\">[\n\r\t ]*<a [^>]+><img alt=\"[^\"]+\" src=\"([^\"]+)\" width=\"22\" height=\"22\" /></a>[\n\r\t ]*</td></tr>")))
        return false;
    if(!data.contains(QRegularExpression("<tr>[\n\r\t ]*<th colspan=\"2\">[\n\r\t ]*<a [^>]+>Forme du corps</a>[\n\r\t ]*</th>[\n\r\t ]*<td colspan=\"3\"> <a href=\".*Miniat_forme_([0-9]+)[^0-9][^\"]+\" class=\"image\"><img alt=\"[^\"]+\" src=\"([^\"]+)\" width=\"32\" height=\"32\" /></a>[\n\r\t ]*</td></tr>")))
        return false;


    QString idString=data;
    idString.replace(QRegularExpression(".*<big><span class=\"explain\" title=\"Numérotation nationale\">. ([0-9]+)</span></big>.*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\1");
    quint32 id=idString.toUInt();

    monsterInformations.name=data;
    monsterInformations.name.replace(QRegularExpression(".*<h1 id=\"firstHeading\" class=\"firstHeading\" lang=\"fr\"><span dir=\"auto\">([^<]+)</span></h1>.*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\1");
    monsterInformations.name.replace(QRegularExpression("[ \r\t\n]+$",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"");
    monsterInformations.name.replace(QRegularExpression("^[ \r\t\n]+",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"");
    monsterInformations.capture_rate=data;
    monsterInformations.capture_rate.replace(QRegularExpression(".*<tr>[\n\r\t ]*<th colspan=\"2\">[\n\r\t ]*<a [^>]+>Taux de capture</a>[\n\r\t ]*</th>[\n\r\t ]*<td colspan=\"3\">[\n\r\t ]+([0-9]+)[\n\r\t ]+</td></tr>.*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\1");
    monsterInformations.description=data;
    monsterInformations.description.replace(QRegularExpression(".*<dt><a [^>]+>Pokémon Émeraude</a>[\n\r\t ]*</dt><dd>[\n\r\t ]*([^\n\r\t<]+)[\n\r\t ]*</dd>.*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\1");
    monsterInformations.description.replace(QRegularExpression("[ \r\t\n]+$",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"");
    monsterInformations.description.replace(QRegularExpression("^[ \r\t\n]+",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"");
    monsterInformations.backPath=data;
    monsterInformations.backPath.replace(QRegularExpression(".*<img alt=\"Sprite[^\"]+\" src=\"([^\"]+Sprite_[1-5]_dos_[0-9]{1,3}(_[mf])?\\.png)\" width=\"96\" height=\"96\" />.*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\1");
    monsterInformations.backPath=QFileInfo(QFileInfo(path).absolutePath()+"/"+monsterInformations.backPath).absoluteFilePath();
    monsterInformations.empreinte=data;
    monsterInformations.empreinte.replace(QRegularExpression(".*<tr>[\n\r\t ]*<th colspan=\"2\">[\n\r\t ]+Empreinte[\n\r\t ]+</th>[\n\r\t ]*<td colspan=\"3\">[\n\r\t ]*<a [^>]+><img alt=\"[^\"]+\" src=\"([^\"]+)\" width=\"22\" height=\"22\" /></a>[\n\r\t ]*</td></tr>.*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\1");
    monsterInformations.empreinte=QFileInfo(QFileInfo(path).absolutePath()+"/"+monsterInformations.empreinte).absoluteFilePath();
    QString formeString=data;
    formeString.replace(QRegularExpression(".*<tr>[\n\r\t ]*<th colspan=\"2\">[\n\r\t ]*<a [^>]+>Forme du corps</a>[\n\r\t ]*</th>[\n\r\t ]*<td colspan=\"3\"> <a href=\".*Miniat_forme_([0-9]+)[^0-9][^\"]+\" class=\"image\"><img alt=\"[^\"]+\" src=\"([^\"]+)\" width=\"32\" height=\"32\" /></a>[\n\r\t ]*</td></tr>.*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\1");
    monsterInformations.forme=formeString.toUInt();
    QString formeImage=data;
    formeImage.replace(QRegularExpression(".*<tr>[\n\r\t ]*<th colspan=\"2\">[\n\r\t ]*<a [^>]+>Forme du corps</a>[\n\r\t ]*</th>[\n\r\t ]*<td colspan=\"3\"> <a href=\".*Miniat_forme_([0-9]+)[^0-9][^\"]+\" class=\"image\"><img alt=\"[^\"]+\" src=\"([^\"]+)\" width=\"32\" height=\"32\" /></a>[\n\r\t ]*</td></tr>.*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\2");
    formeImage=QFileInfo(QFileInfo(path).absolutePath()+"/"+formeImage).absoluteFilePath();
    monsterFormeList[monsterInformations.forme].image=formeImage;

    monsterList[id]=monsterInformations;
    qDebug() << "Monster:" << path;
    return true;
}

bool loadItems(const QString &path,const QString &data)
{
    Item item;
    item.price=0;

    if(!data.contains(QRegularExpression("<div id=\"mw-content-text\" lang=\"fr\" dir=\"ltr\" class=\"mw-content-ltr\">[\n\r\t ]*<table class=\"tableaustandard ficheinfo( #[0-9a-f]{6})?\">")))
        return false;
    if(!data.contains(QRegularExpression("<div id=\"mw-content-text\" lang=\"fr\" dir=\"ltr\" class=\"mw-content-ltr\">[\n\r\t ]*<table class=\"tableaustandard ficheinfo( #[0-9a-f]{6})?\">[\n\r\t ]*<tr>[\n\r\t ]*<th [^>]+> <a [^>]+ class=\"image\"><img alt=\"[^\"]+\" src=\"([^\"]+)\" width=\"[0-9]+\" height=\"[0-9]+\" /></a>[\n\r\t ]*</th>[\n\r\t ]*<th class=\"entêtesection\" colspan=\"3\">[\n\r\t ]*([^<]+)<br /><span lang=\"ja\"><span class=\"explain\" title=\"[^\"]*\">[^<]+</span></span>[\n\r\t ]*([^<]+)[\n\r\t ]*</th></tr>[\n\r\t ]*<tr>[\n\r\t ]*(<td colspan=\"4\" class=\"illustration\"> <a [^>]+><img [^>]+></a>[\n\r\t ]*</td></tr>[\n\r\t ]*<tr>[\n\r\t ]*<td class=\"précision\" colspan=\"4\">.*</a>[\n\r\t ]*</td></tr>[\n\r\t ]*<tr>)?[\n\r\t ]*<th>[\n\r\t ]*Nom anglais[\n\r\t ]*</th>[\n\r\t ]*<td> <span lang=\"en\">([^<]+)</span>")))
        return false;
    if(!data.contains(QRegularExpression("<th>[\n\r\t ]*<a [^>]+>Achat</a>[\n\r\t ]*</th>[\n\r\t ]*<td>[\n\r\t ]*([0-9]+)[\n\r\t ]*<")))
        return false;

    item.name=data;
    item.name.replace(QRegularExpression(".*<h1 id=\"firstHeading\" class=\"firstHeading\" lang=\"fr\"><span dir=\"auto\">([^<]+)</span></h1>.*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\1");
    item.name.replace(QRegularExpression("[ \r\t\n]+$",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"");
    item.name.replace(QRegularExpression("^[ \r\t\n]+",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"");
    if(data.contains(QRegularExpression("<dt><a [^>]+>Pokémon Émeraude</a>[\n\r\t ]*</dt><dd>[\n\r\t ]*([^\n\r\t<]+)[\n\r\t ]*</dd>")))
    {
        item.description=data;
        item.description.replace(QRegularExpression(".*<dt><a [^>]+>Pokémon Émeraude</a>[\n\r\t ]*</dt><dd>[\n\r\t ]*([^\n\r\t<]+)[\n\r\t ]*</dd>.*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\1");
    }
    else if(data.contains(QRegularExpression("<dt><a [^>]+><i>Pokémon Rouge Feu</i> et <i>Vert Feuille</i></a>[\n\r\t ]*</dt><dd>[\n\r\t ]*([^<]+)[\n\r\t ]*</dd>")))
    {
        item.description=data;
        item.description.replace(QRegularExpression(".*<dt><a [^>]+><i>Pok..?mon Rouge Feu</i> et <i>Vert Feuille</i></a>[\n\r\t ]*</dt><dd>[\n\r\t ]*([^\n\r\t<]+)[\n\r\t ]*</dd>.*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\1");
    }
    item.description.replace(QRegularExpression("[ \r\t\n]+$",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"");
    item.description.replace(QRegularExpression("^[ \r\t\n]+",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"");
    QString priceString=data;
    priceString.replace(QRegularExpression(".*<th>[\n\r\t ]*<a [^>]+>Achat</a>[\n\r\t ]*</th>[\n\r\t ]*<td>[\n\r\t ]*([0-9]+)[\n\r\t ]*<.*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\1");
    item.price=priceString.toUInt();
    item.image=data;
    item.image.replace(QRegularExpression(".*<div id=\"mw-content-text\" lang=\"fr\" dir=\"ltr\" class=\"mw-content-ltr\">[\n\r\t ]*<table class=\"tableaustandard ficheinfo( #[0-9a-f]{6})?\">[\n\r\t ]*<tr>[\n\r\t ]*<th [^>]+> <a [^>]+ class=\"image\"><img alt=\"[^\"]+\" src=\"([^\"]+)\" width=\"[0-9]+\" height=\"[0-9]+\" /></a>[\n\r\t ]*</th>[\n\r\t ]*<th class=\"entêtesection\" colspan=\"3\">[\n\r\t ]*([^<]+)<br /><span lang=\"ja\"><span class=\"explain\" title=\"[^\"]*\">[^<]+</span></span>[\n\r\t ]*([^<]+)[\n\r\t ]*</th></tr>[\n\r\t ]*<tr>[\n\r\t ]*(<td colspan=\"4\" class=\"illustration\"> <a [^>]+><img [^>]+></a>[\n\r\t ]*</td></tr>[\n\r\t ]*<tr>[\n\r\t ]*<td class=\"précision\" colspan=\"4\">.*</a>[\n\r\t ]*</td></tr>[\n\r\t ]*<tr>)?[\n\r\t ]*<th>[\n\r\t ]*Nom anglais[\n\r\t ]*</th>[\n\r\t ]*<td> <span lang=\"en\">([^<]+)</span>.*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\2");
    item.image=QFileInfo(QFileInfo(path).absolutePath()+"/"+item.image).absoluteFilePath();
    QString englishName1=data;
    englishName1.replace(QRegularExpression(".*<div id=\"mw-content-text\" lang=\"fr\" dir=\"ltr\" class=\"mw-content-ltr\">[\n\r\t ]*<table class=\"tableaustandard ficheinfo( #[0-9a-f]{6})?\">[\n\r\t ]*<tr>[\n\r\t ]*<th [^>]+> <a [^>]+ class=\"image\"><img alt=\"[^\"]+\" src=\"([^\"]+)\" width=\"[0-9]+\" height=\"[0-9]+\" /></a>[\n\r\t ]*</th>[\n\r\t ]*<th class=\"entêtesection\" colspan=\"3\">[\n\r\t ]*([^<]+)<br /><span lang=\"ja\"><span class=\"explain\" title=\"[^\"]*\">[^<]+</span></span>[\n\r\t ]*([^<]+)[\n\r\t ]*</th></tr>[\n\r\t ]*<tr>[\n\r\t ]*(<td colspan=\"4\" class=\"illustration\"> <a [^>]+><img [^>]+></a>[\n\r\t ]*</td></tr>[\n\r\t ]*<tr>[\n\r\t ]*<td class=\"précision\" colspan=\"4\">.*</a>[\n\r\t ]*</td></tr>[\n\r\t ]*<tr>)?[\n\r\t ]*<th>[\n\r\t ]*Nom anglais[\n\r\t ]*</th>[\n\r\t ]*<td> <span lang=\"en\">([^<]+)</span>.*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\4");
    englishName1.replace(QRegularExpression("[ \r\t\n]+$",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"");
    itemList[englishName1]=item;
    QString englishName2=data;
    englishName2.replace(QRegularExpression(".*<div id=\"mw-content-text\" lang=\"fr\" dir=\"ltr\" class=\"mw-content-ltr\">[\n\r\t ]*<table class=\"tableaustandard ficheinfo( #[0-9a-f]{6})?\">[\n\r\t ]*<tr>[\n\r\t ]*<th [^>]+> <a [^>]+ class=\"image\"><img alt=\"[^\"]+\" src=\"([^\"]+)\" width=\"[0-9]+\" height=\"[0-9]+\" /></a>[\n\r\t ]*</th>[\n\r\t ]*<th class=\"entêtesection\" colspan=\"3\">[\n\r\t ]*([^<]+)<br /><span lang=\"ja\"><span class=\"explain\" title=\"[^\"]*\">[^<]+</span></span>[\n\r\t ]*([^<]+)[\n\r\t ]*</th></tr>[\n\r\t ]*<tr>[\n\r\t ]*(<td colspan=\"4\" class=\"illustration\"> <a [^>]+><img [^>]+></a>[\n\r\t ]*</td></tr>[\n\r\t ]*<tr>[\n\r\t ]*<td class=\"précision\" colspan=\"4\">.*</a>[\n\r\t ]*</td></tr>[\n\r\t ]*<tr>)?[\n\r\t ]*<th>[\n\r\t ]*Nom anglais[\n\r\t ]*</th>[\n\r\t ]*<td> <span lang=\"en\">([^<]+)</span>.*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\6");
    englishName2.replace(QRegularExpression("[ \r\t\n]+$",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"");
    itemList[englishName2]=item;
    item.englishName=englishName2;

    qDebug() << "Item:" << path;
    return true;
}

bool loadBerry(const QString &path,const QString &data)
{
    Item item;
    item.price=0;

    if(!data.contains("<div id=\"mw-content-text\" lang=\"fr\" dir=\"ltr\" class=\"mw-content-ltr\"><div style=\"border: 1px solid #88a; background: #f8f8ff; text-align: center; margin: 0.5em 0; padding: 0.5em; clear: both;\">"))
        return false;
    if(!data.contains(QRegularExpression("title=\"([^\"]+)\" lang=\"en\"")))
        return false;
    if(!data.contains(QRegularExpression("<th colspan=\"2\"> Génération IV[\n\r\t ]*</th>[\n\r\t ]*<td>[\n\r\t ]*([^<]+)[\n\r\t ]*</td>")))
        return false;

    item.name=data;
    item.name.replace(QRegularExpression(".*<h1 id=\"firstHeading\" class=\"firstHeading\" lang=\"fr\"><span dir=\"auto\">([^<]+)</span></h1>.*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\1");
    item.name.replace(QRegularExpression("[ \r\t\n]+$",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"");
    item.name.replace(QRegularExpression("^[ \r\t\n]+",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"");
    item.description=data;
    item.description.replace(QRegularExpression(".*<th colspan=\"2\"> Génération IV[\n\r\t ]*</th>[\n\r\t ]*<td>[\n\r\t ]*([^<]+)[\n\r\t ]*</td>.*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\1");
    item.description.replace(QRegularExpression("[ \r\t\n]+$",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"");
    item.description.replace(QRegularExpression("^[ \r\t\n]+",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"");
    QString englishName1=data;
    englishName1.replace(QRegularExpression(".*title=\"([^\"]+)\" lang=\"en\".*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\1");
    englishName1.replace(QRegularExpression("[ \r\t\n]+$",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"");
    itemList[englishName1]=item;

    qDebug() << "Berry:" << path;
    return true;
}

bool loadSkill(const QString &path,const QString &data)
{
    Skill skill;

    if(!data.contains(QRegularExpression("<div id=\"mw-content-text\" lang=\"fr\" dir=\"ltr\" class=\"mw-content-ltr\">[\n\r\t ]*<table class=\"tableaustandard ficheinfo( #[0-9a-f]{6})?\">")))
        return false;
    if(!data.contains(QRegularExpression("<th> Nom anglais[\n\r\t ]*</th>[\n\r\t ]*<td> [\n\r\t ]*([^<]+)[\n\r\t ]*</td>")))
        return false;
    if(!data.contains(QRegularExpression("title=\"([^\"]+) \\(move\\)\" lang=\"en\"")))
        return false;
    if(!data.contains(QRegularExpression("<h1 id=\"firstHeading\" class=\"firstHeading\" lang=\"fr\"><span dir=\"auto\">([^<]+)</span></h1>")))
        return false;
    if(!data.contains(QRegularExpression("<td class=\"précision\" colspan=\"2\">[\n\r\t ]*([^<]+)[\n\r\t ]*</td>")))
        return false;
    if(!data.contains(QRegularExpression("Adversaire")))
        return false;
    if(!data.contains(QRegularExpression("Lanceur")))
        return false;
    if(!data.contains(QRegularExpression("Allié")))
        return false;
    if(!data.contains(QRegularExpression("title=\"Attaque\">Puissance</a>")))
        return false;

    skill.name=data;
    skill.name.replace(QRegularExpression(".*<h1 id=\"firstHeading\" class=\"firstHeading\" lang=\"fr\"><span dir=\"auto\">([^<]+)</span></h1>.*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\1");
    skill.name.replace(QRegularExpression("[ \r\t\n]+$",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"");
    skill.name.replace(QRegularExpression("^[ \r\t\n]+",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"");
    skill.description=data;
    skill.description.replace(QRegularExpression(".*<td class=\"précision\" colspan=\"2\">[\n\r\t ]*([^<]+)[\n\r\t ]*</td>.*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\1");
    skill.description.replace(QRegularExpression("[ \r\t\n]+$",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"");
    skill.description.replace(QRegularExpression("^[ \r\t\n]+",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"");
    QString englishName1=data;
    englishName1.replace(QRegularExpression(".*<th> Nom anglais[\n\r\t ]*</th>[\n\r\t ]*<td> [\n\r\t ]*([^<]+)[\n\r\t ]*</td>.*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\1");
    englishName1.replace(QRegularExpression("[ \r\t\n]+$",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"");
    skillList[englishName1]=skill;
    QString englishName2=data;
    englishName2.replace(QRegularExpression(".*title=\"([^\"]+) \\(move\\)\" lang=\"en\".*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\1");
    englishName2.replace(QRegularExpression("[ \r\t\n]+$",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"");
    skillList[englishName2]=skill;

    qDebug() << "Skill:" << path;
    return true;
}

void parseItemsExtra()
{
    quint32 maxId=0;
    QSet<QString> itemLoaded;
    //open and quick check the file
    QFile itemsFile(datapackPath+"items/items.xml");
    QByteArray xmlContent;
    if(!itemsFile.open(QIODevice::ReadOnly))
    {
        qDebug() << QString("Unable to open the file: %1, error: %2").arg(itemsFile.fileName()).arg(itemsFile.errorString());
        return;
    }
    xmlContent=itemsFile.readAll();
    itemsFile.close();
    QDomDocument domDocument;
    QString errorStr;
    int errorLine,errorColumn;
    if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
    {
        qDebug() << QString("Unable to open the file: %1, Parse error at line %2, column %3: %4").arg(itemsFile.fileName()).arg(errorLine).arg(errorColumn).arg(errorStr);
        return;
    }
    QDomElement root = domDocument.documentElement();
    if(root.tagName()!="items")
    {
        qDebug() << QString("Unable to open the file: %1, \"items\" root balise not found for the xml file").arg(itemsFile.fileName());
        return;
    }

    //load the content
    bool ok;
    QDomElement item = root.firstChildElement("item");
    while(!item.isNull())
    {
        if(item.isElement())
        {
            if(item.hasAttribute("id"))
            {
                quint32 id=item.attribute("id").toULongLong(&ok);
                if(id>maxId)
                    maxId=id;
                if(ok)
                {
                    const QString &language=LANG;

                    //load the name
                    {
                        bool name_found_translated=false;
                        QDomElement name = item.firstChildElement("name");
                        if(!language.isEmpty() && language!="en")
                            while(!name.isNull())
                            {
                                if(name.isElement())
                                {
                                    if(name.hasAttribute("lang") && name.attribute("lang")==language)
                                    {
                                        itemLoaded << name.text();
                                        name_found_translated=true;
                                    }
                                }
                                name = name.nextSiblingElement("name");
                            }
                        name = item.firstChildElement("name");
                        while(!name.isNull())
                        {
                            if(name.isElement())
                            {
                                if(!name.hasAttribute("lang") || name.attribute("lang")=="en")
                                {
                                    if(itemList.contains(name.text()))
                                    {
                                        itemLoaded << name.text();
                                        itemLoaded << itemList[name.text()].name;
                                        if(!name_found_translated)
                                        {
                                            //add the name
                                            {
                                                QDomElement name=domDocument.createElement("name");
                                                name.setAttribute("lang",language);
                                                item.appendChild(name);
                                                QDomText newTextElement=name.ownerDocument().createTextNode(itemList[name.text()].name);
                                                name.appendChild(newTextElement);
                                            }
                                            //add the description
                                            {
                                                QDomElement name=domDocument.createElement("description");
                                                name.setAttribute("lang",language);
                                                item.appendChild(name);
                                                QDomText newTextElement=name.ownerDocument().createTextNode(itemList[name.text()].description);
                                                name.appendChild(newTextElement);
                                            }
                                            name_found_translated=true;
                                        }
                                    }
                                }
                            }
                            name = name.nextSiblingElement("name");
                        }
                    }
                }
                else
                    qDebug() << QString("Unable to open the file: %1, id is not a number: child.tagName(): %2 (at line: %3)").arg(itemsFile.fileName()).arg(item.tagName()).arg(item.lineNumber());
            }
            else
                qDebug() << QString("Unable to open the file: %1, have not the item id: child.tagName(): %2 (at line: %3)").arg(itemsFile.fileName()).arg(item.tagName()).arg(item.lineNumber());
        }
        else
            qDebug() << QString("Unable to open the file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(itemsFile.fileName()).arg(item.tagName()).arg(item.lineNumber());
        item = item.nextSiblingElement("item");
    }

    QHashIterator<QString, Item> i(itemList);
    while (i.hasNext()) {
        i.next();
        if(!itemLoaded.contains(i.key()) && !itemLoaded.contains(i.value().name))
        {
            maxId++;
            QDomElement item=domDocument.createElement("item");
            item.setAttribute("id",maxId);
            QFileInfo fileInfo(i.value().image);
            QString fileName=QString("%1.%2").arg(maxId).arg(fileInfo.suffix());
            QString destinationImage=datapackPath+QString("items/%1").arg(fileName);
            if(QFile::copy(fileInfo.absoluteFilePath(),destinationImage))
                item.setAttribute("image",fileName);
            item.setAttribute("price",i.value().price);
            //add the english name
            {
                QDomElement name=domDocument.createElement("name");
                item.appendChild(name);
                QDomText newTextElement=name.ownerDocument().createTextNode(itemList[name.text()].englishName);
                name.appendChild(newTextElement);
            }
            //add the name
            {
                QDomElement name=domDocument.createElement("name");
                name.setAttribute("lang",LANG);
                item.appendChild(name);
                QDomText newTextElement=name.ownerDocument().createTextNode(itemList[name.text()].name);
                name.appendChild(newTextElement);
            }
            //add the description
            {
                QDomElement name=domDocument.createElement("description");
                name.setAttribute("lang",LANG);
                item.appendChild(name);
                QDomText newTextElement=name.ownerDocument().createTextNode(itemList[name.text()].description);
                name.appendChild(newTextElement);
            }
        }
    }
}

void parseMonstersExtra()
{
/*    //open and quick check the file
    QFile xmlFile(datapackPath+DATAPACK_BASE_PATH_MONSTERS+"monster.xml");
    QByteArray xmlContent;
    if(!xmlFile.open(QIODevice::ReadOnly))
    {
        CatchChallenger::DebugClass::debugConsole(QString("Unable to open the xml monster extra file: %1, error: %2").arg(xmlFile.fileName()).arg(xmlFile.errorString()));
        return;
    }
    xmlContent=xmlFile.readAll();
    xmlFile.close();
    QDomDocument domDocument;
    QString errorStr;
    int errorLine,errorColumn;
    if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
    {
        CatchChallenger::DebugClass::debugConsole(QString("Unable to open the xml file: %1, Parse error at line %2, column %3: %4").arg(xmlFile.fileName()).arg(errorLine).arg(errorColumn).arg(errorStr));
        return;
    }
    QDomElement root = domDocument.documentElement();
    if(root.tagName()!="list")
    {
        CatchChallenger::DebugClass::debugConsole(QString("Unable to open the xml file: %1, \"list\" root balise not found for the xml file").arg(xmlFile.fileName()));
        return;
    }

    const QString &language=LANG;
    //load the content
    bool ok;
    QDomElement item = root.firstChildElement("monster");
    while(!item.isNull())
    {
        if(item.isElement())
        {
            if(item.hasAttribute("id"))
            {
                quint32 id=item.attribute("id").toUInt(&ok);
                if(!ok)
                    CatchChallenger::DebugClass::debugConsole(QString("Unable to open the xml file: %1, id not a number: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                else
                {
                    if(!CatchChallenger::CommonDatapack::commonDatapack.monsters.contains(id))
                        CatchChallenger::DebugClass::debugConsole(QString("Unable to open the xml file: %1, id not into monster list: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                    else
                    {
                        MonsterExtra monsterExtraEntry;
                        #ifdef DEBUG_MESSAGE_MONSTER_LOAD
                        CatchChallenger::DebugClass::debugConsole(QString("monster extra loading: %1").arg(id));
                        #endif
                        {
                            bool found=false;
                            QDomElement name = item.firstChildElement("name");
                            if(!language.isEmpty() && language!="en")
                                while(!name.isNull())
                                {
                                    if(name.isElement())
                                    {
                                        if(name.hasAttribute("lang") && name.attribute("lang")==language)
                                        {
                                            monsterExtraEntry.name=name.text();
                                            found=true;
                                            break;
                                        }
                                    }
                                    else
                                        CatchChallenger::DebugClass::debugConsole(QString("Unable to open the xml file: %1, name balise is not an element: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                                    name = name.nextSiblingElement("name");
                                }
                            if(!found)
                            {
                                name = item.firstChildElement("name");
                                while(!name.isNull())
                                {
                                    if(name.isElement())
                                    {
                                        if(!name.hasAttribute("lang") || name.attribute("lang")=="en")
                                        {
                                            monsterExtraEntry.name=name.text();
                                            break;
                                        }
                                    }
                                    else
                                        CatchChallenger::DebugClass::debugConsole(QString("Unable to open the xml file: %1, name balise is not an element: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                                    name = name.nextSiblingElement("name");
                                }
                            }
                        }
                        {
                            bool found=false;
                            QDomElement description = item.firstChildElement("description");
                            if(!language.isEmpty() && language!="en")
                                while(!description.isNull())
                                {
                                    if(description.isElement())
                                    {
                                        if(description.hasAttribute("lang") && description.attribute("lang")==language)
                                        {
                                                monsterExtraEntry.description=description.text();
                                                found=true;
                                                break;
                                        }
                                    }
                                    else
                                        CatchChallenger::DebugClass::debugConsole(QString("Unable to open the xml file: %1, description balise is not an element: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                                    description = description.nextSiblingElement("description");
                                }
                            if(!found)
                            {
                                description = item.firstChildElement("description");
                                while(!description.isNull())
                                {
                                    if(description.isElement())
                                    {
                                        if(!description.hasAttribute("lang") || description.attribute("lang")=="en")
                                        {
                                                monsterExtraEntry.description=description.text();
                                                break;
                                        }
                                    }
                                    else
                                        CatchChallenger::DebugClass::debugConsole(QString("Unable to open the xml file: %1, description balise is not an element: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                                    description = description.nextSiblingElement("description");
                                }
                            }
                        }
                        //load the kind
                        {
                            bool kind_found=false;
                            QDomElement kind = item.firstChildElement("kind");
                            if(!language.isEmpty() && language!="en")
                                while(!kind.isNull())
                                {
                                    if(kind.isElement())
                                    {
                                        if(kind.hasAttribute("lang") && kind.attribute("lang")==language)
                                        {
                                            monsterExtraEntry.kind=kind.text();
                                            kind_found=true;
                                            break;
                                        }
                                    }
                                    kind = kind.nextSiblingElement("kind");
                                }
                            if(!kind_found)
                            {
                                kind = item.firstChildElement("kind");
                                while(!kind.isNull())
                                {
                                    if(kind.isElement())
                                    {
                                        if(!kind.hasAttribute("lang") || kind.attribute("lang")=="en")
                                        {
                                            monsterExtraEntry.kind=kind.text();
                                            kind_found=true;
                                            break;
                                        }
                                    }
                                    kind = kind.nextSiblingElement("kind");
                                }
                            }
                            if(!kind_found)
                                qDebug() << QString("English kind not found for the item with id: %1").arg(id);
                        }

                        //load the habitat
                        {
                            bool habitat_found=false;
                            QDomElement habitat = item.firstChildElement("habitat");
                            if(!language.isEmpty() && language!="en")
                                while(!habitat.isNull())
                                {
                                    if(habitat.isElement())
                                    {
                                        if(habitat.hasAttribute("lang") && habitat.attribute("lang")==language)
                                        {
                                            monsterExtraEntry.habitat=habitat.text();
                                            habitat_found=true;
                                            break;
                                        }
                                    }
                                    habitat = habitat.nextSiblingElement("habitat");
                                }
                            if(!habitat_found)
                            {
                                habitat = item.firstChildElement("habitat");
                                while(!habitat.isNull())
                                {
                                    if(habitat.isElement())
                                    {
                                        if(!habitat.hasAttribute("lang") || habitat.attribute("lang")=="en")
                                        {
                                            monsterExtraEntry.habitat=habitat.text();
                                            habitat_found=true;
                                            break;
                                        }
                                    }
                                    habitat = habitat.nextSiblingElement("habitat");
                                }
                            }
                            if(!habitat_found)
                                qDebug() << QString("English habitat not found for the item with id: %1").arg(id);
                        }
                        if(monsterExtraEntry.name.isEmpty())
                            monsterExtraEntry.name=tr("Unknown");
                        if(monsterExtraEntry.description.isEmpty())
                            monsterExtraEntry.description=tr("Unknown");
                        monsterExtraEntry.frontPath=QString("%1/%2/%3/%4").arg(datapackPath).arg(DATAPACK_BASE_PATH_MONSTERS).arg(id).arg("front.png");
                        monsterExtraEntry.front=QPixmap(monsterExtraEntry.frontPath);
                        if(monsterExtraEntry.front.isNull())
                        {
                            monsterExtraEntry.frontPath=QString("%1/%2/%3/%4").arg(datapackPath).arg(DATAPACK_BASE_PATH_MONSTERS).arg(id).arg("front.gif");
                            monsterExtraEntry.front=QPixmap(monsterExtraEntry.frontPath);
                            if(monsterExtraEntry.front.isNull())
                            {
                                monsterExtraEntry.frontPath=":/images/monsters/default/front.png";
                                monsterExtraEntry.front=QPixmap(monsterExtraEntry.frontPath);
                            }
                        }
                        monsterExtraEntry.backPath=QString("%1/%2/%3/%4").arg(datapackPath).arg(DATAPACK_BASE_PATH_MONSTERS).arg(id).arg("back.png");
                        monsterExtraEntry.back=QPixmap(monsterExtraEntry.backPath);
                        if(monsterExtraEntry.back.isNull())
                        {
                            monsterExtraEntry.backPath=QString("%1/%2/%3/%4").arg(datapackPath).arg(DATAPACK_BASE_PATH_MONSTERS).arg(id).arg("back.gif");
                            monsterExtraEntry.back=QPixmap(monsterExtraEntry.backPath);
                            if(monsterExtraEntry.back.isNull())
                            {
                                monsterExtraEntry.backPath=":/images/monsters/default/back.png";
                                monsterExtraEntry.back=QPixmap(monsterExtraEntry.backPath);
                            }
                        }
                        monsterExtraEntry.thumb=QPixmap(QString("%1/%2/%3/%4").arg(datapackPath).arg(DATAPACK_BASE_PATH_MONSTERS).arg(id).arg("small.png"));
                        if(monsterExtraEntry.thumb.isNull())
                        {
                            monsterExtraEntry.thumb=QPixmap(QString("%1/%2/%3/%4").arg(datapackPath).arg(DATAPACK_BASE_PATH_MONSTERS).arg(id).arg("small.gif"));
                            if(monsterExtraEntry.thumb.isNull())
                                monsterExtraEntry.thumb=QPixmap(":/images/monsters/default/small.png");
                        }
                        datapackLoader.monsterExtra[id]=monsterExtraEntry;
                    }
                }
            }
            else
                CatchChallenger::DebugClass::debugConsole(QString("Unable to open the xml file: %1, have not the monster id at monster extra: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
        }
        else
            CatchChallenger::DebugClass::debugConsole(QString("Unable to open the xml file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
        item = item.nextSiblingElement("monster");
    }

    QHashIterator<quint32,CatchChallenger::Monster> i(CatchChallenger::CommonDatapack::commonDatapack.monsters);
    while(i.hasNext())
    {
        i.next();
        if(!DatapackClientLoader::datapackLoader.monsterExtra.contains(i.key()))
        {
            CatchChallenger::DebugClass::debugConsole(QString("Strange, have entry into monster list, but not into monster extra for id: %1").arg(i.key()));
            MonsterExtra monsterExtraEntry;
            monsterExtraEntry.name=tr("Unknown");
            monsterExtraEntry.description=tr("Unknown");
            monsterExtraEntry.front=QPixmap(":/images/monsters/default/front.png");
            monsterExtraEntry.back=QPixmap(":/images/monsters/default/back.png");
            monsterExtraEntry.thumb=QPixmap(":/images/monsters/default/small.png");
            datapackLoader.monsterExtra[i.key()]=monsterExtraEntry;
        }
    }

    qDebug() << QString("%1 monster(s) extra loaded").arg(DatapackClientLoader::datapackLoader.monsterExtra.size());*/
}

void parseSkillsExtra()
{
/*    //open and quick check the file
    QFile xmlFile(datapackPath+DATAPACK_BASE_PATH_MONSTERS+"skill.xml");
    QByteArray xmlContent;
    if(!xmlFile.open(QIODevice::ReadOnly))
    {
        CatchChallenger::DebugClass::debugConsole(QString("Unable to open the xml file: %1, error: %2").arg(xmlFile.fileName()).arg(xmlFile.errorString()));
        return;
    }
    xmlContent=xmlFile.readAll();
    xmlFile.close();
    QDomDocument domDocument;
    QString errorStr;
    int errorLine,errorColumn;
    if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
    {
        CatchChallenger::DebugClass::debugConsole(QString("Unable to open the xml file: %1, Parse error at line %2, column %3: %4").arg(xmlFile.fileName()).arg(errorLine).arg(errorColumn).arg(errorStr));
        return;
    }
    QDomElement root = domDocument.documentElement();
    if(root.tagName()!="list")
    {
        CatchChallenger::DebugClass::debugConsole(QString("Unable to open the xml file: %1, \"list\" root balise not found for the xml file").arg(xmlFile.fileName()));
        return;
    }

    const QString &language=LANG;
    bool found;
    //load the content
    bool ok;
    QDomElement item = root.firstChildElement("skill");
    while(!item.isNull())
    {
        if(item.isElement())
        {
            if(item.hasAttribute("id"))
            {
                quint32 id=item.attribute("id").toUInt(&ok);
                if(!ok)
                    CatchChallenger::DebugClass::debugConsole(QString("Unable to open the xml file: %1, id not a number: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                else
                {
                    if(!CatchChallenger::CommonDatapack::commonDatapack.monsterSkills.contains(id))
                        CatchChallenger::DebugClass::debugConsole(QString("Unable to open the xml file: %1, id not into monster list: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                    else
                    {
                        MonsterExtra::Skill monsterSkillExtraEntry;
                        #ifdef DEBUG_MESSAGE_MONSTER_LOAD
                        CatchChallenger::DebugClass::debugConsole(QString("monster extra loading: %1").arg(id));
                        #endif
                        found=false;
                        QDomElement name = item.firstChildElement("name");
                        if(!language.isEmpty() && language!="en")
                            while(!name.isNull())
                            {
                                if(name.isElement())
                                {
                                    if(name.hasAttribute("lang") && name.attribute("lang")==language)
                                    {
                                        monsterSkillExtraEntry.name=name.text();
                                        found=true;
                                        break;
                                    }
                                }
                                else
                                    CatchChallenger::DebugClass::debugConsole(QString("Unable to open the xml file: %1, name balise is not an element: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                                name = name.nextSiblingElement("name");
                            }
                        if(!found)
                        {
                            name = item.firstChildElement("name");
                            while(!name.isNull())
                            {
                                if(name.isElement())
                                {
                                    if(!name.hasAttribute("lang") || name.attribute("lang")=="en")
                                    {
                                        monsterSkillExtraEntry.name=name.text();
                                        break;
                                    }
                                }
                                else
                                    CatchChallenger::DebugClass::debugConsole(QString("Unable to open the xml file: %1, name balise is not an element: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                                name = name.nextSiblingElement("name");
                            }
                        }
                        found=false;
                        QDomElement description = item.firstChildElement("description");
                        if(!language.isEmpty() && language!="en")
                            while(!description.isNull())
                            {
                                if(description.isElement())
                                {
                                    if(description.hasAttribute("lang") && description.attribute("lang")==language)
                                    {
                                        monsterSkillExtraEntry.description=description.text();
                                        found=true;
                                        break;
                                    }
                                }
                                else
                                    CatchChallenger::DebugClass::debugConsole(QString("Unable to open the xml file: %1, description balise is not an element: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                                description = description.nextSiblingElement("description");
                            }
                        if(!found)
                        {
                            description = item.firstChildElement("description");
                            while(!description.isNull())
                            {
                                if(description.isElement())
                                {
                                    if(!description.hasAttribute("lang") || description.attribute("lang")=="en")
                                    {
                                        monsterSkillExtraEntry.description=description.text();
                                        break;
                                    }
                                }
                                else
                                    CatchChallenger::DebugClass::debugConsole(QString("Unable to open the xml file: %1, description balise is not an element: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
                                description = description.nextSiblingElement("description");
                            }
                        }
                        if(monsterSkillExtraEntry.name.isEmpty())
                            monsterSkillExtraEntry.name=tr("Unknown");
                        if(monsterSkillExtraEntry.description.isEmpty())
                            monsterSkillExtraEntry.description=tr("Unknown");
                        monsterSkillsExtra[id]=monsterSkillExtraEntry;
                    }
                }
            }
            else
                CatchChallenger::DebugClass::debugConsole(QString("Unable to open the xml file: %1, have not the skill id: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
        }
        else
            CatchChallenger::DebugClass::debugConsole(QString("Unable to open the xml file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(xmlFile.fileName()).arg(item.tagName()).arg(item.lineNumber()));
        item = item.nextSiblingElement("skill");
    }

    QHashIterator<quint32,CatchChallenger::Skill> i(CatchChallenger::CommonDatapack::commonDatapack.monsterSkills);
    while(i.hasNext())
    {
        i.next();
        if(!monsterSkillsExtra.contains(i.key()))
        {
            CatchChallenger::DebugClass::debugConsole(QString("Strange, have entry into monster list, but not into monster extra for id: %1").arg(i.key()));
            MonsterExtra::Skill monsterSkillExtraEntry;
            monsterSkillExtraEntry.name=tr("Unknown");
            monsterSkillExtraEntry.description=tr("Unknown");
            monsterSkillsExtra[i.key()]=monsterSkillExtraEntry;
        }
    }

    qDebug() << QString("%1 skill(s) extra loaded").arg(monsterSkillsExtra.size());*/
}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    QStringList arguments=QCoreApplication::arguments();
    if(arguments.size()!=2)
    {
        qDebug() << "pass only the datapack path to update as arguements";
        return 1;
    }
    if(!QFile(arguments.last()+"/items/items.xml").exists())
    {
        qDebug() << "missing items/items.xml into the path given";
        return 2;
    }
    if(!QFile(arguments.last()+"/monsters/skill.xml").exists())
    {
        qDebug() << "missing monsters/skill.xml into the path given";
        return 2;
    }
    if(!QFile(arguments.last()+"/monsters/monster.xml").exists())
    {
        qDebug() << "missing monsters/monster.xml into the path given";
        return 2;
    }
    datapackPath=arguments.last()+"/";

    QByteArray endOfString;
    endOfString[0]=0x00;
    QDir dir(QDir::currentPath());
    const QFileInfoList &fileorFolderList=dir.entryInfoList(QDir::Files);
    int index=0;
    while(index<fileorFolderList.size())
    {
        if(fileorFolderList.at(index).isFile())
        {
            QFile file(fileorFolderList.at(index).absoluteFilePath());
            if(file.open(QIODevice::ReadOnly))
            {
                QByteArray rawData=file.readAll();
                rawData.replace(endOfString,QByteArray());
                QString data=QString::fromUtf8(rawData);
                file.close();
                data.replace(QRegularExpression("<span style=\"font-variant:small-caps;\">([^<]+)</span>",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\1");
                data.replace(QRegularExpression("<center><div style=\"background:#E8E6F8; border-width:1px 0px; border-style:dotted; border-color:#111111; padding:1em; width:66%; text-align:center\"><a href=\"/index.php/Fichier:Vieille_Carte.png\" class=\"image\"><img alt=\"Vieille Carte.png\" src=\"/images/d/db/Vieille_Carte.png\" width=\"[0-9]+\" height=\"[0-9]+\" /></a> <span style=\"font-style:italic; padding:0.5em;\"> Cet article est une <a href=\"/index.php/Aide:%C3%89bauche\" title=\"Aide:Ébauche\">ébauche à compléter</a>, vous pouvez la <span class=\"plainlinks noprint\"><b><a class=\"external text\" href=\"[^\"]*\">modifier</a></b></span> et ainsi partager vos connaissances.</span></div></center>"),"");
                bool used=false;
                used|=loadMonsterInformations(fileorFolderList.at(index).absoluteFilePath(),data);
                used|=loadItems(fileorFolderList.at(index).absoluteFilePath(),data);
                used|=loadBerry(fileorFolderList.at(index).absoluteFilePath(),data);
                used|=loadSkill(fileorFolderList.at(index).absoluteFilePath(),data);
                if(!used)
                    qDebug() << "Not used:" << fileorFolderList.at(index).absoluteFilePath();
            }
        }
        index++;
    }

    parseItemsExtra();
    parseSkillsExtra();
    parseMonstersExtra();

    return 0;
}
