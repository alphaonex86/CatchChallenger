#include <QCoreApplication>
#include <QDir>
#include <QRegularExpression>
#include <tinyxml2::XMLElement>
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

bool loadMonsterInformations(const QString &path,const QString &data,const QString &dataClean)
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
    if(!data.contains(QRegularExpression("<img alt=\"Sprite[^\"]+\" src=\"([^\"]+Sprite_[1-5]_dos_[0-9]{1,3}(_[mf])?\\.png)\" width=\"[0-9]+\" height=\"[0-9]+\" />")))
        return false;
    if(!data.contains(QRegularExpression("<tr>[\n\r\t ]*<th colspan=\"2\">[\n\r\t ]+Empreinte[\n\r\t ]+</th>[\n\r\t ]*<td colspan=\"3\">[\n\r\t ]*<a [^>]+><img alt=\"[^\"]+\" src=\"([^\"]+)\" width=\"[0-9]+\" height=\"[0-9]+\" /></a>[\n\r\t ]*</td></tr>")))
        return false;
    if(!data.contains(QRegularExpression("<tr>[\n\r\t ]*<th colspan=\"2\">[\n\r\t ]*<a [^>]+>Forme du corps</a>[\n\r\t ]*</th>[\n\r\t ]*<td colspan=\"3\"> <a href=\".*Miniat_forme_([0-9]+)[^0-9][^\"]+\" class=\"image\"><img alt=\"[^\"]+\" src=\"([^\"]+)\" width=\"[0-9]+\" height=\"[0-9]+\" /></a>[\n\r\t ]*</td></tr>")))
        return false;


    QString idString=data;
    idString.replace(QRegularExpression(".*<big><span class=\"explain\" title=\"Numérotation nationale\">. ([0-9]+)</span></big>.*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\1");
    quint32 id=idString.toUInt();

    monsterInformations.name=data;
    monsterInformations.name.replace(QRegularExpression(".*<h1 id=\"firstHeading\" class=\"firstHeading\" lang=\"fr\"><span dir=\"auto\">([^<]+)</span></h1>.*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\1");
    monsterInformations.name.replace(QRegularExpression("[ \r\t\n]+$",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"");
    monsterInformations.name.replace(QRegularExpression("^[ \r\t\n]+",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"");
    monsterInformations.name.replace(QRegularExpression("^[^:]+:",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"");
    monsterInformations.capture_rate=data;
    monsterInformations.capture_rate.replace(QRegularExpression(".*<tr>[\n\r\t ]*<th colspan=\"2\">[\n\r\t ]*<a [^>]+>Taux de capture</a>[\n\r\t ]*</th>[\n\r\t ]*<td colspan=\"3\">[\n\r\t ]+([0-9]+)[\n\r\t ]+</td></tr>.*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\1");
    if(dataClean.contains(QRegularExpression("<dt>Pokémon Émeraude[\n\r\t ]*</dt><dd>[\n\r\t ]*([^\n\r\t<]+)[\n\r\t ]*</dd>")))
    {
        monsterInformations.description=dataClean;
        monsterInformations.description.replace(QRegularExpression(".*<dt>Pokémon Émeraude[\n\r\t ]*</dt><dd>[\n\r\t ]*([^\n\r\t<]+)[\n\r\t ]*</dd>.*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\1");
    }
    else if(dataClean.contains(QRegularExpression("<dt>Pokémon Rouge Feu et Vert Feuille[\n\r\t ]*</dt><dd>[\n\r\t ]*([^<]+)[\n\r\t ]*</dd>")))
    {
        monsterInformations.description=dataClean;
        monsterInformations.description.replace(QRegularExpression(".*<dt>Pok..?mon Rouge Feu et Vert Feuille[\n\r\t ]*</dt><dd>[\n\r\t ]*([^\n\r\t<]+)[\n\r\t ]*</dd>.*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\1");
    }
    else if(dataClean.contains(QRegularExpression("<dt>Pokémon Noir et Blanc[\n\r\t ]*</dt><dd>[\n\r\t ]*([^<]+)[\n\r\t ]*</dd>")))
    {
        monsterInformations.description=dataClean;
        monsterInformations.description.replace(QRegularExpression(".*<dt>Pokémon Noir et Blanc[\n\r\t ]*</dt><dd>[\n\r\t ]*([^\n\r\t<]+)[\n\r\t ]*</dd>.*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\1");
    }
    else if(dataClean.contains(QRegularExpression("<dt>Pokémon Noir 2 et Blanc 2[\n\r\t ]*</dt><dd>[\n\r\t ]*([^<]+)[\n\r\t ]*</dd>")))
    {
        monsterInformations.description=dataClean;
        monsterInformations.description.replace(QRegularExpression(".*<dt>Pokémon Noir 2 et Blanc 2[\n\r\t ]*</dt><dd>[\n\r\t ]*([^\n\r\t<]+)[\n\r\t ]*</dd>.*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\1");
    }
    else if(dataClean.contains(QRegularExpression("<dt>Pokémon Noir et Blanc 2[\n\r\t ]*</dt><dd>[\n\r\t ]*([^<]+)[\n\r\t ]*</dd>")))
    {
        monsterInformations.description=dataClean;
        monsterInformations.description.replace(QRegularExpression(".*<dt>Pokémon Noir et Blanc 2[\n\r\t ]*</dt><dd>[\n\r\t ]*([^\n\r\t<]+)[\n\r\t ]*</dd>.*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\1");
    }
    else if(dataClean.contains(QRegularExpression("<dt>Pokémon Or HeartGold[\n\r\t ]*</dt><dd>[\n\r\t ]*([^<]+)[\n\r\t ]*</dd>")))
    {
        monsterInformations.description=dataClean;
        monsterInformations.description.replace(QRegularExpression(".*<dt>Pokémon Or HeartGold[\n\r\t ]*</dt><dd>[\n\r\t ]*([^\n\r\t<]+)[\n\r\t ]*</dd>.*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\1");
    }
    else if(dataClean.contains(QRegularExpression("<dt>Pokémon Argent SoulSilver[\n\r\t ]*</dt><dd>[\n\r\t ]*([^<]+)[\n\r\t ]*</dd>")))
    {
        monsterInformations.description=dataClean;
        monsterInformations.description.replace(QRegularExpression(".*<dt>Pokémon Argent SoulSilver[\n\r\t ]*</dt><dd>[\n\r\t ]*([^\n\r\t<]+)[\n\r\t ]*</dd>.*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\1");
    }
    else if(dataClean.contains(QRegularExpression("<dt>Pokémon X[\n\r\t ]*</dt><dd>[\n\r\t ]*([^<]+)[\n\r\t ]*</dd>")))
    {
        monsterInformations.description=dataClean;
        monsterInformations.description.replace(QRegularExpression(".*<dt>Pokémon X[\n\r\t ]*</dt><dd>[\n\r\t ]*([^\n\r\t<]+)[\n\r\t ]*</dd>.*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\1");
    }
    else if(dataClean.contains(QRegularExpression("<dt>Pokémon Y[\n\r\t ]*</dt><dd>[\n\r\t ]*([^<]+)[\n\r\t ]*</dd>")))
    {
        monsterInformations.description=dataClean;
        monsterInformations.description.replace(QRegularExpression(".*<dt>Pokémon Y[\n\r\t ]*</dt><dd>[\n\r\t ]*([^\n\r\t<]+)[\n\r\t ]*</dd>.*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\1");
    }
    else if(dataClean.contains(QRegularExpression("<dt>Pokémon Rouge Feu[\n\r\t ]*</dt><dd>[\n\r\t ]*([^<]+)[\n\r\t ]*</dd>")))
    {
        monsterInformations.description=dataClean;
        monsterInformations.description.replace(QRegularExpression(".*<dt>Pokémon Rouge Feu[\n\r\t ]*</dt><dd>[\n\r\t ]*([^\n\r\t<]+)[\n\r\t ]*</dd>.*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\1");
    }
    else if(dataClean.contains(QRegularExpression("<dt>Pokémon Vert Feuille[\n\r\t ]*</dt><dd>[\n\r\t ]*([^<]+)[\n\r\t ]*</dd>")))
    {
        monsterInformations.description=dataClean;
        monsterInformations.description.replace(QRegularExpression(".*<dt>Pokémon Vert Feuille[\n\r\t ]*</dt><dd>[\n\r\t ]*([^\n\r\t<]+)[\n\r\t ]*</dd>.*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\1");
    }
    else if(dataClean.contains(QRegularExpression("<dt>Pokémon Diamant[\n\r\t ]*</dt><dd>[\n\r\t ]*([^<]+)[\n\r\t ]*</dd>")))
    {
        monsterInformations.description=dataClean;
        monsterInformations.description.replace(QRegularExpression(".*<dt>Pokémon Diamant[\n\r\t ]*</dt><dd>[\n\r\t ]*([^\n\r\t<]+)[\n\r\t ]*</dd>.*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\1");
    }
    else if(dataClean.contains(QRegularExpression("<dt>Pokémon Cristal[\n\r\t ]*</dt><dd>[\n\r\t ]*([^<]+)[\n\r\t ]*</dd>")))
    {
        monsterInformations.description=dataClean;
        monsterInformations.description.replace(QRegularExpression(".*<dt>Pokémon Cristal[\n\r\t ]*</dt><dd>[\n\r\t ]*([^\n\r\t<]+)[\n\r\t ]*</dd>.*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\1");
    }
    else if(dataClean.contains(QRegularExpression("<dt>Pokémon Or[\n\r\t ]*</dt><dd>[\n\r\t ]*([^<]+)[\n\r\t ]*</dd>")))
    {
        monsterInformations.description=dataClean;
        monsterInformations.description.replace(QRegularExpression(".*<dt>Pokémon Or[\n\r\t ]*</dt><dd>[\n\r\t ]*([^\n\r\t<]+)[\n\r\t ]*</dd>.*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\1");
    }
    else if(dataClean.contains(QRegularExpression("<dt>Pokémon Argent[\n\r\t ]*</dt><dd>[\n\r\t ]*([^<]+)[\n\r\t ]*</dd>")))
    {
        monsterInformations.description=dataClean;
        monsterInformations.description.replace(QRegularExpression(".*<dt>Pokémon Argent[\n\r\t ]*</dt><dd>[\n\r\t ]*([^\n\r\t<]+)[\n\r\t ]*</dd>.*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\1");
    }
    else if(dataClean.contains(QRegularExpression("<dt>Pokémon Jaune[\n\r\t ]*</dt><dd>[\n\r\t ]*([^<]+)[\n\r\t ]*</dd>")))
    {
        monsterInformations.description=dataClean;
        monsterInformations.description.replace(QRegularExpression(".*<dt>Pokémon Jaune[\n\r\t ]*</dt><dd>[\n\r\t ]*([^\n\r\t<]+)[\n\r\t ]*</dd>.*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\1");
    }
    else if(dataClean.contains(QRegularExpression("<dt>Pokémon Rouge et Bleu[\n\r\t ]*</dt><dd>[\n\r\t ]*([^<]+)[\n\r\t ]*</dd>")))
    {
        monsterInformations.description=dataClean;
        monsterInformations.description.replace(QRegularExpression(".*<dt>Pokémon Rouge et Bleu[\n\r\t ]*</dt><dd>[\n\r\t ]*([^\n\r\t<]+)[\n\r\t ]*</dd>.*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\1");
    }
    else if(dataClean.contains(QRegularExpression("<dt>Pokémon Or, Argent et Cristal[\n\r\t ]*</dt><dd>[\n\r\t ]*([^<]+)[\n\r\t ]*</dd>")))
    {
        monsterInformations.description=dataClean;
        monsterInformations.description.replace(QRegularExpression(".*<dt>Pokémon Or, Argent et Cristal[\n\r\t ]*</dt><dd>[\n\r\t ]*([^\n\r\t<]+)[\n\r\t ]*</dd>.*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\1");
    }
    else if(dataClean.contains(QRegularExpression("<dt>Pokémon Rubis, Saphir et Émeraude[\n\r\t ]*</dt><dd>[\n\r\t ]*([^<]+)[\n\r\t ]*</dd>")))
    {
        monsterInformations.description=dataClean;
        monsterInformations.description.replace(QRegularExpression(".*<dt>Pokémon Rubis, Saphir et Émeraude[\n\r\t ]*</dt><dd>[\n\r\t ]*([^\n\r\t<]+)[\n\r\t ]*</dd>.*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\1");
    }
    else if(dataClean.contains(QRegularExpression("<dt>Pokémon Rouge Feu et Vert Feuille[\n\r\t ]*</dt><dd>[\n\r\t ]*([^<]+)[\n\r\t ]*</dd>")))
    {
        monsterInformations.description=dataClean;
        monsterInformations.description.replace(QRegularExpression(".*<dt>Pokémon Rouge Feu et Vert Feuille[\n\r\t ]*</dt><dd>[\n\r\t ]*([^\n\r\t<]+)[\n\r\t ]*</dd>.*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\1");
    }
    else if(dataClean.contains(QRegularExpression("<dt>Pokémon Diamant, Perle et Platine[\n\r\t ]*</dt><dd>[\n\r\t ]*([^<]+)[\n\r\t ]*</dd>")))
    {
        monsterInformations.description=dataClean;
        monsterInformations.description.replace(QRegularExpression(".*<dt>Pokémon Diamant, Perle et Platine[\n\r\t ]*</dt><dd>[\n\r\t ]*([^\n\r\t<]+)[\n\r\t ]*</dd>.*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\1");
    }
    else if(dataClean.contains(QRegularExpression("<dt>Pokémon Or HeartGold et Argent SoulSilver[\n\r\t ]*</dt><dd>[\n\r\t ]*([^<]+)[\n\r\t ]*</dd>")))
    {
        monsterInformations.description=dataClean;
        monsterInformations.description.replace(QRegularExpression(".*<dt>Pokémon Or HeartGold et Argent SoulSilver[\n\r\t ]*</dt><dd>[\n\r\t ]*([^\n\r\t<]+)[\n\r\t ]*</dd>.*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\1");
    }
    else if(dataClean.contains(QRegularExpression("<th colspan=\"2\">[\n\r\t ]*Génération IV[\n\r\t ]*</th>[\n\r\t ]*<td>[\n\r\t ]*([^<]+)[\n\r\t ]*</td>")))
    {
        monsterInformations.description=dataClean;
        monsterInformations.description.replace(QRegularExpression(".*<th colspan=\"2\">[\n\r\t ]*Génération IV[\n\r\t ]*</th>[\n\r\t ]*<td>[\n\r\t ]*([^<]+)[\n\r\t ]*</td>.*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\1");
    }
    else if(dataClean.contains(QRegularExpression("<th colspan=\"2\">[\n\r\t ]*Génération III[\n\r\t ]*</th>[\n\r\t ]*<td>[\n\r\t ]*([^<]+)[\n\r\t ]*</td>")))
    {
        monsterInformations.description=dataClean;
        monsterInformations.description.replace(QRegularExpression(".*<th colspan=\"2\">[\n\r\t ]*Génération III[\n\r\t ]*</th>[\n\r\t ]*<td>[\n\r\t ]*([^<]+)[\n\r\t ]*</td>.*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\1");
    }
    monsterInformations.description.replace(QRegularExpression("[ \r\t\n]+$",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"");
    monsterInformations.description.replace(QRegularExpression("^[ \r\t\n]+",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"");
    monsterInformations.backPath=data;
    monsterInformations.backPath.replace(QRegularExpression(".*<img alt=\"Sprite[^\"]+\" src=\"([^\"]+Sprite_[1-5]_dos_[0-9]{1,3}(_[mf])?\\.png)\" width=\"[0-9]+\" height=\"[0-9]+\" />.*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\1");
    monsterInformations.backPath=QFileInfo(QFileInfo(path).absolutePath()+"/"+monsterInformations.backPath).absoluteFilePath();
    monsterInformations.empreinte=data;
    monsterInformations.empreinte.replace(QRegularExpression(".*<tr>[\n\r\t ]*<th colspan=\"2\">[\n\r\t ]+Empreinte[\n\r\t ]+</th>[\n\r\t ]*<td colspan=\"3\">[\n\r\t ]*<a [^>]+><img alt=\"[^\"]+\" src=\"([^\"]+)\" width=\"[0-9]+\" height=\"[0-9]+\" /></a>[\n\r\t ]*</td></tr>.*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\1");
    monsterInformations.empreinte=QFileInfo(QFileInfo(path).absolutePath()+"/"+monsterInformations.empreinte).absoluteFilePath();
    QString formeString=data;
    formeString.replace(QRegularExpression(".*<tr>[\n\r\t ]*<th colspan=\"2\">[\n\r\t ]*<a [^>]+>Forme du corps</a>[\n\r\t ]*</th>[\n\r\t ]*<td colspan=\"3\"> <a href=\".*Miniat_forme_([0-9]+)[^0-9][^\"]+\" class=\"image\"><img alt=\"[^\"]+\" src=\"([^\"]+)\" width=\"[0-9]+\" height=\"[0-9]+\" /></a>[\n\r\t ]*</td></tr>.*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\1");
    monsterInformations.forme=formeString.toUInt();
    QString formeImage=data;
    formeImage.replace(QRegularExpression(".*<tr>[\n\r\t ]*<th colspan=\"2\">[\n\r\t ]*<a [^>]+>Forme du corps</a>[\n\r\t ]*</th>[\n\r\t ]*<td colspan=\"3\"> <a href=\".*Miniat_forme_([0-9]+)[^0-9][^\"]+\" class=\"image\"><img alt=\"[^\"]+\" src=\"([^\"]+)\" width=\"[0-9]+\" height=\"[0-9]+\" /></a>[\n\r\t ]*</td></tr>.*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\2");
    formeImage=QFileInfo(QFileInfo(path).absolutePath()+"/"+formeImage).absoluteFilePath();
    monsterFormeList[monsterInformations.forme].image=formeImage;

    monsterList[id]=monsterInformations;
    qDebug() << "Monster:" << path;
    return true;
}

bool loadItems(const QString &path,const QString &data,const QString &dataClean)
{
    Item item;
    item.price=0;

    if(!data.contains(QRegularExpression("<div id=\"mw-content-text\" lang=\"fr\" dir=\"ltr\" class=\"mw-content-ltr\">[\n\r\t ]*<table class=\"tableaustandard ficheinfo( #[0-9a-f]{6})?\">")))
        return false;
    if(!data.contains(QRegularExpression("<div id=\"mw-content-text\" lang=\"fr\" dir=\"ltr\" class=\"mw-content-ltr\">[\n\r\t ]*<table class=\"tableaustandard ficheinfo( #[0-9a-f]{6})?\">[\n\r\t ]*<tr>[\n\r\t ]*<th [^>]+> <a [^>]+ class=\"image\"><img alt=\"[^\"]+\" src=\"([^\"]+)\" width=\"[0-9]+\" height=\"[0-9]+\" /></a>[\n\r\t ]*</th>[\n\r\t ]*<th class=\"entêtesection\" colspan=\"3\">[\n\r\t ]*([^<]+)<br /><span lang=\"ja\"><span class=\"explain\" title=\"[^\"]*\">[^<]+</span></span>[\n\r\t ]*([^<]+)[\n\r\t ]*</th></tr>[\n\r\t ]*<tr>[\n\r\t ]*(<td colspan=\"4\" class=\"illustration\"> <a [^>]+><img [^>]+></a>[\n\r\t ]*</td></tr>[\n\r\t ]*<tr>[\n\r\t ]*<td class=\"précision\" colspan=\"4\">.*</a>[\n\r\t ]*</td></tr>[\n\r\t ]*<tr>)?[\n\r\t ]*<th>[\n\r\t ]*Nom anglais[\n\r\t ]*</th>[\n\r\t ]*<td> <span lang=\"en\">([^<]+)</span>")))
        return false;

    item.name=data;
    item.name.replace(QRegularExpression(".*<h1 id=\"firstHeading\" class=\"firstHeading\" lang=\"fr\"><span dir=\"auto\">([^<]+)</span></h1>.*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\1");
    item.name.replace(QRegularExpression("[ \r\t\n]+$",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"");
    item.name.replace(QRegularExpression("^[ \r\t\n]+",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"");
    item.name.replace(QRegularExpression("^[^:]+:",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"");
    if(dataClean.contains(QRegularExpression("<dt>Pokémon Émeraude[\n\r\t ]*</dt><dd>[\n\r\t ]*([^\n\r\t<]+)[\n\r\t ]*</dd>")))
    {
        item.description=dataClean;
        item.description.replace(QRegularExpression(".*<dt>Pokémon Émeraude[\n\r\t ]*</dt><dd>[\n\r\t ]*([^\n\r\t<]+)[\n\r\t ]*</dd>.*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\1");
    }
    else if(dataClean.contains(QRegularExpression("<dt>Pokémon Rouge Feu et Vert Feuille[\n\r\t ]*</dt><dd>[\n\r\t ]*([^<]+)[\n\r\t ]*</dd>")))
    {
        item.description=dataClean;
        item.description.replace(QRegularExpression(".*<dt>Pok..?mon Rouge Feu et Vert Feuille[\n\r\t ]*</dt><dd>[\n\r\t ]*([^\n\r\t<]+)[\n\r\t ]*</dd>.*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\1");
    }
    else if(dataClean.contains(QRegularExpression("<dt>Pokémon Noir et Blanc[\n\r\t ]*</dt><dd>[\n\r\t ]*([^<]+)[\n\r\t ]*</dd>")))
    {
        item.description=dataClean;
        item.description.replace(QRegularExpression(".*<dt>Pokémon Noir et Blanc[\n\r\t ]*</dt><dd>[\n\r\t ]*([^\n\r\t<]+)[\n\r\t ]*</dd>.*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\1");
    }
    else if(dataClean.contains(QRegularExpression("<dt>Pokémon Noir 2 et Blanc 2[\n\r\t ]*</dt><dd>[\n\r\t ]*([^<]+)[\n\r\t ]*</dd>")))
    {
        item.description=dataClean;
        item.description.replace(QRegularExpression(".*<dt>Pokémon Noir 2 et Blanc 2[\n\r\t ]*</dt><dd>[\n\r\t ]*([^\n\r\t<]+)[\n\r\t ]*</dd>.*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\1");
    }
    else if(dataClean.contains(QRegularExpression("<dt>Pokémon Noir et Blanc 2[\n\r\t ]*</dt><dd>[\n\r\t ]*([^<]+)[\n\r\t ]*</dd>")))
    {
        item.description=dataClean;
        item.description.replace(QRegularExpression(".*<dt>Pokémon Noir et Blanc 2[\n\r\t ]*</dt><dd>[\n\r\t ]*([^\n\r\t<]+)[\n\r\t ]*</dd>.*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\1");
    }
    else if(dataClean.contains(QRegularExpression("<dt>Pokémon Or HeartGold[\n\r\t ]*</dt><dd>[\n\r\t ]*([^<]+)[\n\r\t ]*</dd>")))
    {
        item.description=dataClean;
        item.description.replace(QRegularExpression(".*<dt>Pokémon Or HeartGold[\n\r\t ]*</dt><dd>[\n\r\t ]*([^\n\r\t<]+)[\n\r\t ]*</dd>.*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\1");
    }
    else if(dataClean.contains(QRegularExpression("<dt>Pokémon Argent SoulSilver[\n\r\t ]*</dt><dd>[\n\r\t ]*([^<]+)[\n\r\t ]*</dd>")))
    {
        item.description=dataClean;
        item.description.replace(QRegularExpression(".*<dt>Pokémon Argent SoulSilver[\n\r\t ]*</dt><dd>[\n\r\t ]*([^\n\r\t<]+)[\n\r\t ]*</dd>.*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\1");
    }
    else if(dataClean.contains(QRegularExpression("<dt>Pokémon X[\n\r\t ]*</dt><dd>[\n\r\t ]*([^<]+)[\n\r\t ]*</dd>")))
    {
        item.description=dataClean;
        item.description.replace(QRegularExpression(".*<dt>Pokémon X[\n\r\t ]*</dt><dd>[\n\r\t ]*([^\n\r\t<]+)[\n\r\t ]*</dd>.*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\1");
    }
    else if(dataClean.contains(QRegularExpression("<dt>Pokémon Y[\n\r\t ]*</dt><dd>[\n\r\t ]*([^<]+)[\n\r\t ]*</dd>")))
    {
        item.description=dataClean;
        item.description.replace(QRegularExpression(".*<dt>Pokémon Y[\n\r\t ]*</dt><dd>[\n\r\t ]*([^\n\r\t<]+)[\n\r\t ]*</dd>.*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\1");
    }
    else if(dataClean.contains(QRegularExpression("<dt>Pokémon Rouge Feu[\n\r\t ]*</dt><dd>[\n\r\t ]*([^<]+)[\n\r\t ]*</dd>")))
    {
        item.description=dataClean;
        item.description.replace(QRegularExpression(".*<dt>Pokémon Rouge Feu[\n\r\t ]*</dt><dd>[\n\r\t ]*([^\n\r\t<]+)[\n\r\t ]*</dd>.*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\1");
    }
    else if(dataClean.contains(QRegularExpression("<dt>Pokémon Vert Feuille[\n\r\t ]*</dt><dd>[\n\r\t ]*([^<]+)[\n\r\t ]*</dd>")))
    {
        item.description=dataClean;
        item.description.replace(QRegularExpression(".*<dt>Pokémon Vert Feuille[\n\r\t ]*</dt><dd>[\n\r\t ]*([^\n\r\t<]+)[\n\r\t ]*</dd>.*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\1");
    }
    else if(dataClean.contains(QRegularExpression("<dt>Pokémon Diamant[\n\r\t ]*<[\n\r\t ]*</dt><dd>[\n\r\t ]*([^<]+)[\n\r\t ]*</dd>")))
    {
        item.description=dataClean;
        item.description.replace(QRegularExpression(".*<dt>Pokémon Diamant[\n\r\t ]*</dt><dd>[\n\r\t ]*([^\n\r\t<]+)[\n\r\t ]*</dd>.*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\1");
    }
    else if(dataClean.contains(QRegularExpression("<dt>Pokémon Cristal[\n\r\t ]*</dt><dd>[\n\r\t ]*([^<]+)[\n\r\t ]*</dd>")))
    {
        item.description=dataClean;
        item.description.replace(QRegularExpression(".*<dt>Pokémon Cristal[\n\r\t ]*</dt><dd>[\n\r\t ]*([^\n\r\t<]+)[\n\r\t ]*</dd>.*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\1");
    }
    else if(dataClean.contains(QRegularExpression("<dt>Pokémon Or[\n\r\t ]*</dt><dd>[\n\r\t ]*([^<]+)[\n\r\t ]*</dd>")))
    {
        item.description=dataClean;
        item.description.replace(QRegularExpression(".*<dt>Pokémon Or[\n\r\t ]*</dt><dd>[\n\r\t ]*([^\n\r\t<]+)[\n\r\t ]*</dd>.*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\1");
    }
    else if(dataClean.contains(QRegularExpression("<dt>Pokémon Argent[\n\r\t ]*</dt><dd>[\n\r\t ]*([^<]+)[\n\r\t ]*</dd>")))
    {
        item.description=dataClean;
        item.description.replace(QRegularExpression(".*<dt>Pokémon Argent[\n\r\t ]*</dt><dd>[\n\r\t ]*([^\n\r\t<]+)[\n\r\t ]*</dd>.*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\1");
    }
    else if(dataClean.contains(QRegularExpression("<dt>Pokémon Jaune[\n\r\t ]*</dt><dd>[\n\r\t ]*([^<]+)[\n\r\t ]*</dd>")))
    {
        item.description=dataClean;
        item.description.replace(QRegularExpression(".*<dt>Pokémon Jaune[\n\r\t ]*</dt><dd>[\n\r\t ]*([^\n\r\t<]+)[\n\r\t ]*</dd>.*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\1");
    }
    else if(dataClean.contains(QRegularExpression("<dt>Pokémon Rouge et Bleu[\n\r\t ]*</dt><dd>[\n\r\t ]*([^<]+)[\n\r\t ]*</dd>")))
    {
        item.description=dataClean;
        item.description.replace(QRegularExpression(".*<dt>Pokémon Rouge et Bleu[\n\r\t ]*</dt><dd>[\n\r\t ]*([^\n\r\t<]+)[\n\r\t ]*</dd>.*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\1");
    }
    else if(dataClean.contains(QRegularExpression("<dt>Pokémon Or, Argent et Cristal[\n\r\t ]*</dt><dd>[\n\r\t ]*([^<]+)[\n\r\t ]*</dd>")))
    {
        item.description=dataClean;
        item.description.replace(QRegularExpression(".*<dt>Pokémon Or, Argent et Cristal[\n\r\t ]*</dt><dd>[\n\r\t ]*([^\n\r\t<]+)[\n\r\t ]*</dd>.*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\1");
    }
    else if(dataClean.contains(QRegularExpression("<dt>Pokémon Rubis, Saphir et Émeraude[\n\r\t ]*</dt><dd>[\n\r\t ]*([^<]+)[\n\r\t ]*</dd>")))
    {
        item.description=dataClean;
        item.description.replace(QRegularExpression(".*<dt>Pokémon Rubis, Saphir et Émeraude[\n\r\t ]*</dt><dd>[\n\r\t ]*([^\n\r\t<]+)[\n\r\t ]*</dd>.*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\1");
    }
    else if(dataClean.contains(QRegularExpression("<dt>Pokémon Rouge Feu et Vert Feuille[\n\r\t ]*</dt><dd>[\n\r\t ]*([^<]+)[\n\r\t ]*</dd>")))
    {
        item.description=dataClean;
        item.description.replace(QRegularExpression(".*<dt>Pokémon Rouge Feu et Vert Feuille[\n\r\t ]*</dt><dd>[\n\r\t ]*([^\n\r\t<]+)[\n\r\t ]*</dd>.*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\1");
    }
    else if(dataClean.contains(QRegularExpression("<dt>Pokémon Diamant, Perle et Platine[\n\r\t ]*</dt><dd>[\n\r\t ]*([^<]+)[\n\r\t ]*</dd>")))
    {
        item.description=dataClean;
        item.description.replace(QRegularExpression(".*<dt>Pokémon Diamant, Perle et Platine[\n\r\t ]*</dt><dd>[\n\r\t ]*([^\n\r\t<]+)[\n\r\t ]*</dd>.*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\1");
    }
    else if(dataClean.contains(QRegularExpression("<dt>Pokémon Or HeartGold et Argent SoulSilver[\n\r\t ]*</dt><dd>[\n\r\t ]*([^<]+)[\n\r\t ]*</dd>")))
    {
        item.description=dataClean;
        item.description.replace(QRegularExpression(".*<dt>Pokémon Or HeartGold et Argent SoulSilver[\n\r\t ]*</dt><dd>[\n\r\t ]*([^\n\r\t<]+)[\n\r\t ]*</dd>.*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\1");
    }
    else if(dataClean.contains(QRegularExpression("<th colspan=\"2\">[\n\r\t ]*Génération IV[\n\r\t ]*</th>[\n\r\t ]*<td>[\n\r\t ]*([^<]+)[\n\r\t ]*</td>")))
    {
        item.description=dataClean;
        item.description.replace(QRegularExpression(".*<th colspan=\"2\">[\n\r\t ]*Génération IV[\n\r\t ]*</th>[\n\r\t ]*<td>[\n\r\t ]*([^<]+)[\n\r\t ]*</td>.*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\1");
    }
    else if(dataClean.contains(QRegularExpression("<th colspan=\"2\">[\n\r\t ]*Génération III[\n\r\t ]*</th>[\n\r\t ]*<td>[\n\r\t ]*([^<]+)[\n\r\t ]*</td>")))
    {
        item.description=dataClean;
        item.description.replace(QRegularExpression(".*<th colspan=\"2\">[\n\r\t ]*Génération III[\n\r\t ]*</th>[\n\r\t ]*<td>[\n\r\t ]*([^<]+)[\n\r\t ]*</td>.*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\1");
    }
    item.description.replace(QRegularExpression("[ \r\t\n]+$",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"");
    item.description.replace(QRegularExpression("^[ \r\t\n]+",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"");
    if(data.contains(QRegularExpression("<th>[\n\r\t ]*<a [^>]+>Achat</a>[\n\r\t ]*</th>[\n\r\t ]*<td>[\n\r\t ]*([0-9]+)[\n\r\t ]*<")))
    {
        QString priceString=data;
        priceString.replace(QRegularExpression(".*<th>[\n\r\t ]*<a [^>]+>Achat</a>[\n\r\t ]*</th>[\n\r\t ]*<td>[\n\r\t ]*([0-9]+)[\n\r\t ]*<.*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\1");
        item.price=priceString.toUInt();
    }
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
    QString englishName3=data;
    englishName3.replace(QRegularExpression(".*title=\"([^\"]+)\" lang=\"en\".*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\1");
    englishName3.replace(QRegularExpression("[ \r\t\n]+$",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"");
    itemList[englishName3]=item;

    qDebug() << "Item:" << path;
    return true;
}

bool loadBerry(const QString &path,const QString &data,const QString &dataClean)
{
    Item item;
    item.price=0;

    if(!path.contains("Baie"))
        return false;
    if(!data.contains(QRegularExpression("title=\"([^\"]+)\" lang=\"en\"")))
        return false;

    item.name=data;
    item.name.replace(QRegularExpression(".*<h1 id=\"firstHeading\" class=\"firstHeading\" lang=\"fr\"><span dir=\"auto\">([^<]+)</span></h1>.*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\1");
    item.name.replace(QRegularExpression("[ \r\t\n]+$",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"");
    item.name.replace(QRegularExpression("^[ \r\t\n]+",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"");
    item.name.replace(QRegularExpression("^[^:]+:",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"");
    if(dataClean.contains(QRegularExpression("<dt>Pokémon Émeraude[\n\r\t ]*</dt><dd>[\n\r\t ]*([^\n\r\t<]+)[\n\r\t ]*</dd>")))
    {
        item.description=dataClean;
        item.description.replace(QRegularExpression(".*<dt>Pokémon Émeraude[\n\r\t ]*</dt><dd>[\n\r\t ]*([^\n\r\t<]+)[\n\r\t ]*</dd>.*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\1");
    }
    else if(dataClean.contains(QRegularExpression("<dt>Pokémon Rouge Feu et Vert Feuille[\n\r\t ]*</dt><dd>[\n\r\t ]*([^<]+)[\n\r\t ]*</dd>")))
    {
        item.description=dataClean;
        item.description.replace(QRegularExpression(".*<dt>Pok..?mon Rouge Feu et Vert Feuille[\n\r\t ]*</dt><dd>[\n\r\t ]*([^\n\r\t<]+)[\n\r\t ]*</dd>.*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\1");
    }
    else if(dataClean.contains(QRegularExpression("<dt>Pokémon Noir et Blanc[\n\r\t ]*</dt><dd>[\n\r\t ]*([^<]+)[\n\r\t ]*</dd>")))
    {
        item.description=dataClean;
        item.description.replace(QRegularExpression(".*<dt>Pokémon Noir et Blanc[\n\r\t ]*</dt><dd>[\n\r\t ]*([^\n\r\t<]+)[\n\r\t ]*</dd>.*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\1");
    }
    else if(dataClean.contains(QRegularExpression("<dt>Pokémon Noir 2 et Blanc 2[\n\r\t ]*</dt><dd>[\n\r\t ]*([^<]+)[\n\r\t ]*</dd>")))
    {
        item.description=dataClean;
        item.description.replace(QRegularExpression(".*<dt>Pokémon Noir 2 et Blanc 2[\n\r\t ]*</dt><dd>[\n\r\t ]*([^\n\r\t<]+)[\n\r\t ]*</dd>.*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\1");
    }
    else if(dataClean.contains(QRegularExpression("<dt>Pokémon Noir et Blanc 2[\n\r\t ]*</dt><dd>[\n\r\t ]*([^<]+)[\n\r\t ]*</dd>")))
    {
        item.description=dataClean;
        item.description.replace(QRegularExpression(".*<dt>Pokémon Noir et Blanc 2[\n\r\t ]*</dt><dd>[\n\r\t ]*([^\n\r\t<]+)[\n\r\t ]*</dd>.*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\1");
    }
    else if(dataClean.contains(QRegularExpression("<dt>Pokémon Or HeartGold[\n\r\t ]*</dt><dd>[\n\r\t ]*([^<]+)[\n\r\t ]*</dd>")))
    {
        item.description=dataClean;
        item.description.replace(QRegularExpression(".*<dt>Pokémon Or HeartGold[\n\r\t ]*</dt><dd>[\n\r\t ]*([^\n\r\t<]+)[\n\r\t ]*</dd>.*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\1");
    }
    else if(dataClean.contains(QRegularExpression("<dt>Pokémon Argent SoulSilver[\n\r\t ]*</dt><dd>[\n\r\t ]*([^<]+)[\n\r\t ]*</dd>")))
    {
        item.description=dataClean;
        item.description.replace(QRegularExpression(".*<dt>Pokémon Argent SoulSilver[\n\r\t ]*</dt><dd>[\n\r\t ]*([^\n\r\t<]+)[\n\r\t ]*</dd>.*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\1");
    }
    else if(dataClean.contains(QRegularExpression("<dt>Pokémon X[\n\r\t ]*</dt><dd>[\n\r\t ]*([^<]+)[\n\r\t ]*</dd>")))
    {
        item.description=dataClean;
        item.description.replace(QRegularExpression(".*<dt>Pokémon X[\n\r\t ]*</dt><dd>[\n\r\t ]*([^\n\r\t<]+)[\n\r\t ]*</dd>.*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\1");
    }
    else if(dataClean.contains(QRegularExpression("<dt>Pokémon Y[\n\r\t ]*</dt><dd>[\n\r\t ]*([^<]+)[\n\r\t ]*</dd>")))
    {
        item.description=dataClean;
        item.description.replace(QRegularExpression(".*<dt>Pokémon Y[\n\r\t ]*</dt><dd>[\n\r\t ]*([^\n\r\t<]+)[\n\r\t ]*</dd>.*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\1");
    }
    else if(dataClean.contains(QRegularExpression("<dt>Pokémon Rouge Feu[\n\r\t ]*</dt><dd>[\n\r\t ]*([^<]+)[\n\r\t ]*</dd>")))
    {
        item.description=dataClean;
        item.description.replace(QRegularExpression(".*<dt>Pokémon Rouge Feu[\n\r\t ]*</dt><dd>[\n\r\t ]*([^\n\r\t<]+)[\n\r\t ]*</dd>.*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\1");
    }
    else if(dataClean.contains(QRegularExpression("<dt>Pokémon Vert Feuille[\n\r\t ]*</dt><dd>[\n\r\t ]*([^<]+)[\n\r\t ]*</dd>")))
    {
        item.description=dataClean;
        item.description.replace(QRegularExpression(".*<dt>Pokémon Vert Feuille[\n\r\t ]*</dt><dd>[\n\r\t ]*([^\n\r\t<]+)[\n\r\t ]*</dd>.*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\1");
    }
    else if(dataClean.contains(QRegularExpression("<dt>Pokémon Diamant[\n\r\t ]*</dt><dd>[\n\r\t ]*([^<]+)[\n\r\t ]*</dd>")))
    {
        item.description=dataClean;
        item.description.replace(QRegularExpression(".*<dt>Pokémon Diamant[\n\r\t ]*</dt><dd>[\n\r\t ]*([^\n\r\t<]+)[\n\r\t ]*</dd>.*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\1");
    }
    else if(dataClean.contains(QRegularExpression("<dt>Pokémon Cristal[\n\r\t ]*</dt><dd>[\n\r\t ]*([^<]+)[\n\r\t ]*</dd>")))
    {
        item.description=dataClean;
        item.description.replace(QRegularExpression(".*<dt>Pokémon Cristal[\n\r\t ]*</dt><dd>[\n\r\t ]*([^\n\r\t<]+)[\n\r\t ]*</dd>.*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\1");
    }
    else if(dataClean.contains(QRegularExpression("<dt>Pokémon Or[\n\r\t ]*</dt><dd>[\n\r\t ]*([^<]+)[\n\r\t ]*</dd>")))
    {
        item.description=dataClean;
        item.description.replace(QRegularExpression(".*<dt>Pokémon Or[\n\r\t ]*</dt><dd>[\n\r\t ]*([^\n\r\t<]+)[\n\r\t ]*</dd>.*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\1");
    }
    else if(dataClean.contains(QRegularExpression("<dt>Pokémon Argent[\n\r\t ]*</dt><dd>[\n\r\t ]*([^<]+)[\n\r\t ]*</dd>")))
    {
        item.description=dataClean;
        item.description.replace(QRegularExpression(".*<dt>Pokémon Argent[\n\r\t ]*</dt><dd>[\n\r\t ]*([^\n\r\t<]+)[\n\r\t ]*</dd>.*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\1");
    }
    else if(dataClean.contains(QRegularExpression("<dt>Pokémon Jaune[\n\r\t ]*</dt><dd>[\n\r\t ]*([^<]+)[\n\r\t ]*</dd>")))
    {
        item.description=dataClean;
        item.description.replace(QRegularExpression(".*<dt>Pokémon Jaune[\n\r\t ]*</dt><dd>[\n\r\t ]*([^\n\r\t<]+)[\n\r\t ]*</dd>.*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\1");
    }
    else if(dataClean.contains(QRegularExpression("<dt>Pokémon Rouge et Bleu[\n\r\t ]*</dt><dd>[\n\r\t ]*([^<]+)[\n\r\t ]*</dd>")))
    {
        item.description=dataClean;
        item.description.replace(QRegularExpression(".*<dt>Pokémon Rouge et Bleu[\n\r\t ]*</dt><dd>[\n\r\t ]*([^\n\r\t<]+)[\n\r\t ]*</dd>.*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\1");
    }
    else if(dataClean.contains(QRegularExpression("<dt>Pokémon Or, Argent et Cristal[\n\r\t ]*</dt><dd>[\n\r\t ]*([^<]+)[\n\r\t ]*</dd>")))
    {
        item.description=dataClean;
        item.description.replace(QRegularExpression(".*<dt>Pokémon Or, Argent et Cristal[\n\r\t ]*</dt><dd>[\n\r\t ]*([^\n\r\t<]+)[\n\r\t ]*</dd>.*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\1");
    }
    else if(dataClean.contains(QRegularExpression("<dt>Pokémon Rubis, Saphir et Émeraude[\n\r\t ]*</dt><dd>[\n\r\t ]*([^<]+)[\n\r\t ]*</dd>")))
    {
        item.description=dataClean;
        item.description.replace(QRegularExpression(".*<dt>Pokémon Rubis, Saphir et Émeraude[\n\r\t ]*</dt><dd>[\n\r\t ]*([^\n\r\t<]+)[\n\r\t ]*</dd>.*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\1");
    }
    else if(dataClean.contains(QRegularExpression("<dt>Pokémon Rouge Feu et Vert Feuille[\n\r\t ]*</dt><dd>[\n\r\t ]*([^<]+)[\n\r\t ]*</dd>")))
    {
        item.description=dataClean;
        item.description.replace(QRegularExpression(".*<dt>Pokémon Rouge Feu et Vert Feuille[\n\r\t ]*</dt><dd>[\n\r\t ]*([^\n\r\t<]+)[\n\r\t ]*</dd>.*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\1");
    }
    else if(dataClean.contains(QRegularExpression("<dt>Pokémon Diamant, Perle et Platine[\n\r\t ]*</dt><dd>[\n\r\t ]*([^<]+)[\n\r\t ]*</dd>")))
    {
        item.description=dataClean;
        item.description.replace(QRegularExpression(".*<dt>Pokémon Diamant, Perle et Platine[\n\r\t ]*</dt><dd>[\n\r\t ]*([^\n\r\t<]+)[\n\r\t ]*</dd>.*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\1");
    }
    else if(dataClean.contains(QRegularExpression("<dt>Pokémon Or HeartGold et Argent SoulSilver[\n\r\t ]*</dt><dd>[\n\r\t ]*([^<]+)[\n\r\t ]*</dd>")))
    {
        item.description=dataClean;
        item.description.replace(QRegularExpression(".*<dt>Pokémon Or HeartGold et Argent SoulSilver[\n\r\t ]*</dt><dd>[\n\r\t ]*([^\n\r\t<]+)[\n\r\t ]*</dd>.*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\1");
    }
    else if(dataClean.contains(QRegularExpression("<th colspan=\"2\">[\n\r\t ]*Génération IV[\n\r\t ]*</th>[\n\r\t ]*<td>[\n\r\t ]*([^<]+)[\n\r\t ]*</td>")))
    {
        item.description=dataClean;
        item.description.replace(QRegularExpression(".*<th colspan=\"2\">[\n\r\t ]*Génération IV[\n\r\t ]*</th>[\n\r\t ]*<td>[\n\r\t ]*([^<]+)[\n\r\t ]*</td>.*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\1");
    }
    else if(dataClean.contains(QRegularExpression("<th colspan=\"2\">[\n\r\t ]*Génération III[\n\r\t ]*</th>[\n\r\t ]*<td>[\n\r\t ]*([^<]+)[\n\r\t ]*</td>")))
    {
        item.description=dataClean;
        item.description.replace(QRegularExpression(".*<th colspan=\"2\">[\n\r\t ]*Génération III[\n\r\t ]*</th>[\n\r\t ]*<td>[\n\r\t ]*([^<]+)[\n\r\t ]*</td>.*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\1");
    }
    item.description.replace(QRegularExpression("[ \r\t\n]+$",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"");
    item.description.replace(QRegularExpression("^[ \r\t\n]+",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"");
    QString englishName1=data;
    englishName1.replace(QRegularExpression(".*title=\"([^\"]+)\" lang=\"en\".*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\1");
    englishName1.replace(QRegularExpression("[ \r\t\n]+$",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"");
    itemList[englishName1]=item;
    QString englishName2=data;
    englishName2.replace(QRegularExpression(".*title=\"([^\"]+)\" lang=\"en\".*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\1");
    englishName2.replace(QRegularExpression("[ \r\t\n]+$",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"");
    itemList[englishName2]=item;

    qDebug() << "Berry:" << path;
    return true;
}

bool loadSkill(const QString &path,const QString &data,const QString &dataClean)
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
    if(!itemsFile.open(QIODevice::ReadWrite))
    {
        qDebug() << QStringLiteral("Unable to open the file: %1, error: %2").arg(itemsFile.fileName()).arg(itemsFile.errorString());
        return;
    }
    xmlContent=itemsFile.readAll();
    QDomDocument domDocument;
    QString errorStr;
    int errorLine,errorColumn;
    if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
    {
        qDebug() << QStringLiteral("Unable to open the file: %1, Parse error at line %2, column %3: %4").arg(itemsFile.fileName()).arg(errorLine).arg(errorColumn).arg(errorStr);
        return;
    }
    tinyxml2::XMLElement root = domDocument.RootElement();
    if(root.tagName()!="items")
    {
        qDebug() << QStringLiteral("Unable to open the file: %1, \"items\" root balise not found for the xml file").arg(itemsFile.fileName());
        return;
    }

    //load the content
    bool ok;
    tinyxml2::XMLElement item = root.FirstChildElement("item");
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
                        tinyxml2::XMLElement name = item.FirstChildElement("name");
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
                                name = name.NextSiblingElement("name");
                            }
                        name = item.FirstChildElement("name");
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
                                            QString stringIndex=name.text();
                                            //add the name
                                            {
                                                tinyxml2::XMLElement name=domDocument.createElement("name");
                                                name.setAttribute("lang",language);
                                                item.appendChild(name);
                                                QDomText newTextElement=name.ownerDocument().createTextNode(itemList[stringIndex].name);
                                                name.appendChild(newTextElement);
                                            }
                                            //add the description
                                            {
                                                tinyxml2::XMLElement name=domDocument.createElement("description");
                                                name.setAttribute("lang",language);
                                                item.appendChild(name);
                                                QDomText newTextElement=name.ownerDocument().createTextNode(itemList[stringIndex].description);
                                                name.appendChild(newTextElement);
                                            }
                                            name_found_translated=true;
                                        }
                                    }
                                }
                            }
                            name = name.NextSiblingElement("name");
                        }
                    }
                }
                else
                    qDebug() << QStringLiteral("Unable to open the file: %1, id is not a number: child.tagName(): %2 (at line: %3)").arg(itemsFile.fileName()).arg(item.tagName()).arg(item.lineNumber());
            }
            else
                qDebug() << QStringLiteral("Unable to open the file: %1, have not the item id: child.tagName(): %2 (at line: %3)").arg(itemsFile.fileName()).arg(item.tagName()).arg(item.lineNumber());
        }
        else
            qDebug() << QStringLiteral("Unable to open the file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(itemsFile.fileName()).arg(item.tagName()).arg(item.lineNumber());
        item = item.NextSiblingElement("item");
    }

    QHashIterator<QString, Item> i(itemList);
    while (i.hasNext()) {
        i.next();
        if(!itemLoaded.contains(i.key()) && !itemLoaded.contains(i.value().name))
        {
            maxId++;
            tinyxml2::XMLElement item=domDocument.createElement("item");
            item.setAttribute("id",maxId);
            QFileInfo fileInfo(i.value().image);
            QString fileName=QStringLiteral("%1.%2").arg(maxId).arg(fileInfo.suffix());
            QString destinationImage=datapackPath+QStringLiteral("items/%1").arg(fileName);
            if(!QFile(destinationImage).exists())
            {
                if(fileInfo.exists())
                    if(QFile::copy(fileInfo.absoluteFilePath(),destinationImage))
                        item.setAttribute("image",fileName);
            }
            else
                item.setAttribute("image",fileName);
            item.setAttribute("price",i.value().price);
            //add the english name
            {
                tinyxml2::XMLElement name=domDocument.createElement("name");
                item.appendChild(name);
                QDomText newTextElement=name.ownerDocument().createTextNode(itemList[name.text()].englishName);
                name.appendChild(newTextElement);
            }
            //add the name
            {
                tinyxml2::XMLElement name=domDocument.createElement("name");
                name.setAttribute("lang",LANG);
                item.appendChild(name);
                QDomText newTextElement=name.ownerDocument().createTextNode(itemList[name.text()].name);
                name.appendChild(newTextElement);
            }
            //add the description
            {
                tinyxml2::XMLElement name=domDocument.createElement("description");
                name.setAttribute("lang",LANG);
                item.appendChild(name);
                QDomText newTextElement=name.ownerDocument().createTextNode(itemList[name.text()].description);
                name.appendChild(newTextElement);
            }
        }
    }
    itemsFile.seek(0);
    itemsFile.resize(0);
    itemsFile.write(domDocument.toByteArray(4));
    itemsFile.close();
}

void parseMonstersExtra()
{
    quint32 maxId=0;
    //open and quick check the file
    QFile itemsFile(datapackPath+"monsters/monster.xml");
    QByteArray xmlContent;
    if(!itemsFile.open(QIODevice::ReadWrite))
    {
        qDebug() << QStringLiteral("Unable to open the file: %1, error: %2").arg(itemsFile.fileName()).arg(itemsFile.errorString());
        return;
    }
    xmlContent=itemsFile.readAll();
    QDomDocument domDocument;
    QString errorStr;
    int errorLine,errorColumn;
    if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
    {
        qDebug() << QStringLiteral("Unable to open the file: %1, Parse error at line %2, column %3: %4").arg(itemsFile.fileName()).arg(errorLine).arg(errorColumn).arg(errorStr);
        return;
    }
    tinyxml2::XMLElement root = domDocument.RootElement();
    if(root.tagName()!="list")
    {
        qDebug() << QStringLiteral("Unable to open the file: %1, \"items\" root balise not found for the xml file").arg(itemsFile.fileName());
        return;
    }

    //load the content
    bool ok;
    tinyxml2::XMLElement item = root.FirstChildElement("monster");
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
                        if(!item.hasAttribute("catch_rate") && monsterList.contains(id))
                            item.setAttribute("catch_rate",monsterList[id].capture_rate);
                        bool name_found_translated=false;
                        tinyxml2::XMLElement name = item.FirstChildElement("name");
                        if(!language.isEmpty() && language!="en")
                        {
                            while(!name.isNull())
                            {
                                if(name.isElement())
                                {
                                    if(name.hasAttribute("lang") && name.attribute("lang")==language)
                                        name_found_translated=true;
                                }
                                name = name.NextSiblingElement("name");
                            }
                            if(!name_found_translated && monsterList.contains(id))
                            {
                                //add the name
                                {
                                    tinyxml2::XMLElement name=domDocument.createElement("name");
                                    name.setAttribute("lang",language);
                                    item.appendChild(name);
                                    QDomText newTextElement=name.ownerDocument().createTextNode(monsterList[id].name);
                                    name.appendChild(newTextElement);
                                }
                                //add the description
                                {
                                    tinyxml2::XMLElement name=domDocument.createElement("description");
                                    name.setAttribute("lang",language);
                                    item.appendChild(name);
                                    QDomText newTextElement=name.ownerDocument().createTextNode(monsterList[id].description);
                                    name.appendChild(newTextElement);
                                }
                                name_found_translated=true;
                            }
                        }
                    }
                }
                else
                    qDebug() << QStringLiteral("Unable to open the file: %1, id is not a number: child.tagName(): %2 (at line: %3)").arg(itemsFile.fileName()).arg(item.tagName()).arg(item.lineNumber());
            }
            else
                qDebug() << QStringLiteral("Unable to open the file: %1, have not the item id: child.tagName(): %2 (at line: %3)").arg(itemsFile.fileName()).arg(item.tagName()).arg(item.lineNumber());
        }
        else
            qDebug() << QStringLiteral("Unable to open the file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(itemsFile.fileName()).arg(item.tagName()).arg(item.lineNumber());
        item = item.NextSiblingElement("monster");
    }
    itemsFile.seek(0);
    itemsFile.resize(0);
    itemsFile.write(domDocument.toByteArray(4));
    itemsFile.close();
}

void parseSkillsExtra()
{
    quint32 maxId=0;
    //open and quick check the file
    QFile itemsFile(datapackPath+"monsters/skill.xml");
    QByteArray xmlContent;
    if(!itemsFile.open(QIODevice::ReadWrite))
    {
        qDebug() << QStringLiteral("Unable to open the file: %1, error: %2").arg(itemsFile.fileName()).arg(itemsFile.errorString());
        return;
    }
    xmlContent=itemsFile.readAll();
    QDomDocument domDocument;
    QString errorStr;
    int errorLine,errorColumn;
    if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
    {
        qDebug() << QStringLiteral("Unable to open the file: %1, Parse error at line %2, column %3: %4").arg(itemsFile.fileName()).arg(errorLine).arg(errorColumn).arg(errorStr);
        return;
    }
    tinyxml2::XMLElement root = domDocument.RootElement();
    if(root.tagName()!="list")
    {
        qDebug() << QStringLiteral("Unable to open the file: %1, \"items\" root balise not found for the xml file").arg(itemsFile.fileName());
        return;
    }

    //load the content
    bool ok;
    tinyxml2::XMLElement item = root.FirstChildElement("skill");
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
                        tinyxml2::XMLElement name = item.FirstChildElement("name");
                        if(!language.isEmpty() && language!="en")
                        {
                            while(!name.isNull())
                            {
                                if(name.isElement())
                                {
                                    if(name.hasAttribute("lang") && name.attribute("lang")==language)
                                        name_found_translated=true;
                                }
                                name = name.NextSiblingElement("name");
                            }
                            name = item.FirstChildElement("name");
                            while(!name.isNull())
                            {
                                if(name.isElement())
                                {
                                    if(!name.hasAttribute("lang") || name.attribute("lang")=="en")
                                    {
                                        if(skillList.contains(name.text()))
                                        {
                                            if(!name_found_translated)
                                            {
                                                QString stringIndex=name.text();
                                                //add the name
                                                {
                                                    tinyxml2::XMLElement name=domDocument.createElement("name");
                                                    name.setAttribute("lang",language);
                                                    item.appendChild(name);
                                                    QString newText=skillList[stringIndex].name;
                                                    QDomText newTextElement=name.ownerDocument().createTextNode(newText);
                                                    name.appendChild(newTextElement);
                                                }
                                                //add the description
                                                {
                                                    tinyxml2::XMLElement name=domDocument.createElement("description");
                                                    name.setAttribute("lang",language);
                                                    item.appendChild(name);
                                                    QDomText newTextElement=name.ownerDocument().createTextNode(skillList[stringIndex].description);
                                                    name.appendChild(newTextElement);
                                                }
                                                name_found_translated=true;
                                            }
                                        }
                                    }
                                }
                                name = name.NextSiblingElement("name");
                            }
                        }
                    }
                }
                else
                    qDebug() << QStringLiteral("Unable to open the file: %1, id is not a number: child.tagName(): %2 (at line: %3)").arg(itemsFile.fileName()).arg(item.tagName()).arg(item.lineNumber());
            }
            else
                qDebug() << QStringLiteral("Unable to open the file: %1, have not the item id: child.tagName(): %2 (at line: %3)").arg(itemsFile.fileName()).arg(item.tagName()).arg(item.lineNumber());
        }
        else
            qDebug() << QStringLiteral("Unable to open the file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(itemsFile.fileName()).arg(item.tagName()).arg(item.lineNumber());
        item = item.NextSiblingElement("skill");
    }
    itemsFile.seek(0);
    itemsFile.resize(0);
    itemsFile.write(domDocument.toByteArray(4));
    itemsFile.close();
}

int main(int argc, char *argv[])
{
    QByteArray endOfString;
    endOfString[0]=0x00;
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

    /*QFile file("Mimigalhtml.html.tmp");
    if(file.open(QIODevice::ReadOnly))
    {
        QByteArray rawData=file.readAll();
        rawData.replace(endOfString,QByteArray());
        QString data=QString::fromUtf8(rawData);
        file.close();
        data.replace(QRegularExpression("<span style=\"font-variant:small-caps;\">([^<]+)</span>",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\1");
        data.replace(QRegularExpression("<center><div style=\"background:#E8E6F8; border-width:1px 0px; border-style:dotted; border-color:#111111; padding:1em; width:66%; text-align:center\"><a href=\"/index.php/Fichier:Vieille_Carte.png\" class=\"image\"><img alt=\"Vieille Carte.png\" src=\"/images/d/db/Vieille_Carte.png\" width=\"[0-9]+\" height=\"[0-9]+\" /></a> <span style=\"font-style:italic; padding:0.5em;\"> Cet article est une <a href=\"/index.php/Aide:%C3%89bauche\" title=\"Aide:Ébauche\">ébauche à compléter</a>, vous pouvez la <span class=\"plainlinks noprint\"><b><a class=\"external text\" href=\"[^\"]*\">modifier</a></b></span> et ainsi partager vos connaissances.</span></div></center>"),"");
        data.replace("<i>","");
        data.replace("</i>","");
        QString dataClean=data;
        dataClean.replace(QRegularExpression("<a [^>]+>"),"");
        dataClean.replace("</a>","");
        loadMonsterInformations("Mimigalhtml.html.tmp",data,dataClean);
    }
    return 0;*/

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
                data.replace("<i>","");
                data.replace("</i>","");
                QString dataClean=data;
                dataClean.replace(QRegularExpression("<a [^>]+>"),"");
                dataClean.replace("</a>","");
                bool used=false;
                used=used || loadMonsterInformations(fileorFolderList.at(index).absoluteFilePath(),data,dataClean);
                used=used || loadItems(fileorFolderList.at(index).absoluteFilePath(),data,dataClean);
                used=used || loadBerry(fileorFolderList.at(index).absoluteFilePath(),data,dataClean);
                used=used || loadSkill(fileorFolderList.at(index).absoluteFilePath(),data,dataClean);
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
