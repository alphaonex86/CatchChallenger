#include <QCoreApplication>
#include <QDir>
#include <QRegularExpression>
#include <QDebug>

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
    quint32 price;
};
QHash<QString,Item> itemList;

struct Skill
{
    QString name;
    QString description;
};
QHash<QString,Skill> skillList;

void loadMonsterInformations(const QString &path,const QString &data)
{
    MonsterInformations monsterInformations;

    if(!data.contains("rotation nationale\">"))
        return;
    if(!data.contains(QRegularExpression("<h1 id=\"firstHeading\" class=\"firstHeading\" lang=\"fr\"><span dir=\"auto\">([^<]+)</span></h1>")))
        return;
    if(!data.contains(QRegularExpression("<big><span class=\"explain\" title=\"Numérotation nationale\">. ([0-9]+)</span></big>")))
        return;
    if(!data.contains(QRegularExpression("<tr>[\n\r\t ]*<th colspan=\"2\">[\n\r\t ]*<a [^>]+>Taux de capture</a>[\n\r\t ]*</th>[\n\r\t ]*<td colspan=\"3\">[\n\r\t ]+([0-9]+)[\n\r\t ]+</td></tr>")))
        return;
    if(!data.contains(QRegularExpression("<dt><a [^>]+>Pokémon Émeraude</a>[\n\r\t ]*</dt><dd>[\n\r\t ]+([^\n\r\t<]+)[\n\r\t ]+</dd>")))
        return;
    if(!data.contains(QRegularExpression("<th>[\n\r\t ]*<b>♂</b><br /><br /><i><a [^>]+>Mâle</a></i>[\n\r\t ]*</th>[\n\r\t ]*<td> <a [^>]+><img alt=\"Sprite[^\"]+\" src=\"([^\"]+\\.png)\" width=\"96\" height=\"96\" /></a>[\n\r\t ]*</td>")))
        return;
    if(!data.contains(QRegularExpression("<tr>[\n\r\t ]*<th colspan=\"2\">[\n\r\t ]+Empreinte[\n\r\t ]+</th>[\n\r\t ]*<td colspan=\"3\">[\n\r\t ]*<a [^>]+><img alt=\"[^\"]+\" src=\"([^\"]+)\" width=\"22\" height=\"22\" /></a>[\n\r\t ]*</td></tr>")))
        return;
    if(!data.contains(QRegularExpression("<tr>[\n\r\t ]*<th colspan=\"2\">[\n\r\t ]*<a [^>]+>Forme du corps</a>[\n\r\t ]*</th>[\n\r\t ]*<td colspan=\"3\"> <a href=\".*Miniat_forme_([0-9]+)[^0-9][^\"]+\" class=\"image\"><img alt=\"[^\"]+\" src=\"([^\"]+)\" width=\"32\" height=\"32\" /></a>[\n\r\t ]*</td></tr>")))
        return;


    QString idString=data;
    idString.replace(QRegularExpression(".*<big><span class=\"explain\" title=\"Numérotation nationale\">. ([0-9]+)</span></big>.*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\1");
    quint32 id=idString.toUInt();

    monsterInformations.name=data;
    monsterInformations.name.replace(QRegularExpression(".*<h1 id=\"firstHeading\" class=\"firstHeading\" lang=\"fr\"><span dir=\"auto\">([^<]+)</span></h1>.*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\1");
    monsterInformations.name.replace(QRegularExpression("[ \r\t\n]+$",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"");
    monsterInformations.capture_rate=data;
    monsterInformations.capture_rate.replace(QRegularExpression(".*<tr>[\n\r\t ]*<th colspan=\"2\">[\n\r\t ]*<a [^>]+>Taux de capture</a>[\n\r\t ]*</th>[\n\r\t ]*<td colspan=\"3\">[\n\r\t ]+([0-9]+)[\n\r\t ]+</td></tr>.*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\1");
    monsterInformations.description=data;
    monsterInformations.description.replace(QRegularExpression(".*<dt><a [^>]+>Pokémon Émeraude</a>[\n\r\t ]*</dt><dd>[\n\r\t ]+([^\n\r\t<]+)[\n\r\t ]+</dd>.*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\1");
    monsterInformations.description.replace(QRegularExpression("[ \r\t\n]+$",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"");
    monsterInformations.backPath=data;
    monsterInformations.backPath.replace(QRegularExpression(".*<th>[\n\r\t ]*<b>♂</b><br /><br /><i><a [^>]+>Mâle</a></i>[\n\r\t ]*</th>[\n\r\t ]*<td> <a [^>]+><img alt=\"Sprite[^\"]+\" src=\"([^\"]+\\.png)\" width=\"96\" height=\"96\" /></a>[\n\r\t ]*</td>.*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\1");
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
}

void loadItems(const QString &path,const QString &data)
{
    Item item;
    item.price=0;

    if(!data.contains("<div id=\"mw-content-text\" lang=\"fr\" dir=\"ltr\" class=\"mw-content-ltr\"><table class=\"tableaustandard ficheinfo\">"))
        return;
    if(!data.contains(QRegularExpression("<div id=\"mw-content-text\" lang=\"fr\" dir=\"ltr\" class=\"mw-content-ltr\"><table class=\"tableaustandard ficheinfo\">[\n\r\t ]*<tr>[\n\r\t ]*<th [^>]+> <a [^>]+ class=\"image\"><img alt=\"[^\"]+\" src=\"([^\"]+)\" width=\"24\" height=\"24\" /></a>[\n\r\t ]*</th>[\n\r\t ]*<th class=\"entêtesection\" colspan=\"3\">[\n\r\t ]*([^<]+)<br /><span lang=\"ja\"><span class=\"explain\" title=\"\">[^<]+</span></span>[\n\r\t ]*([^<]+)[\n\r\t ]*</th></tr>[\n\r\t ]*<tr>[\n\r\t ]*<td colspan=\"4\" class=\"illustration\"> <a [^>]+><img [^>]+></a>[\n\r\t ]*</td></tr>[\n\r\t ]*<tr>[\n\r\t ]*<td class=\"précision\" colspan=\"4\">.*</a>[\n\r\t ]*</td></tr>[\n\r\t ]*<tr>[\n\r\t ]*<th>[\n\r\t ]*Nom anglais[\n\r\t ]*</th>[\n\r\t ]*<td> <span lang=\"en\">([^<]+)</span>")))
        return;
    if(!data.contains(QRegularExpression("<th>[\n\r\t ]*<a [^>]+>Achat</a>[\n\r\t ]*</th>[\n\r\t ]*<td>[\n\r\t ]*([0-9]+)[\n\r\t ]*<")))
        return;

    item.name=data;
    item.name.replace(QRegularExpression(".*<h1 id=\"firstHeading\" class=\"firstHeading\" lang=\"fr\"><span dir=\"auto\">([^<]+)</span></h1>.*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\1");
    item.name.replace(QRegularExpression("[ \r\t\n]+$",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"");
    QString priceString=data;
    priceString.replace(QRegularExpression(".*<th>[\n\r\t ]*<a [^>]+>Achat</a>[\n\r\t ]*</th>[\n\r\t ]*<td>[\n\r\t ]*([0-9]+)[\n\r\t ]*<.*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\1");
    item.price=priceString.toUInt();
    item.image=data;
    item.image.replace(QRegularExpression(".*<div id=\"mw-content-text\" lang=\"fr\" dir=\"ltr\" class=\"mw-content-ltr\"><table class=\"tableaustandard ficheinfo\">[\n\r\t ]*<tr>[\n\r\t ]*<th [^>]+> <a [^>]+ class=\"image\"><img alt=\"[^\"]+\" src=\"([^\"]+)\" width=\"24\" height=\"24\" /></a>[\n\r\t ]*</th>[\n\r\t ]*<th class=\"entêtesection\" colspan=\"3\">[\n\r\t ]*([^<]+)<br /><span lang=\"ja\"><span class=\"explain\" title=\"\">[^<]+</span></span>[\n\r\t ]*([^<]+)[\n\r\t ]*</th></tr>[\n\r\t ]*<tr>[\n\r\t ]*<td colspan=\"4\" class=\"illustration\"> <a [^>]+><img [^>]+></a>[\n\r\t ]*</td></tr>[\n\r\t ]*<tr>[\n\r\t ]*<td class=\"précision\" colspan=\"4\">.*</a>[\n\r\t ]*</td></tr>[\n\r\t ]*<tr>[\n\r\t ]*<th>[\n\r\t ]*Nom anglais[\n\r\t ]*</th>[\n\r\t ]*<td> <span lang=\"en\">([^<]+)</span>.*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\1");
    item.image=QFileInfo(QFileInfo(path).absolutePath()+"/"+item.image).absoluteFilePath();
    QString englishName1=data;
    englishName1.replace(QRegularExpression(".*<div id=\"mw-content-text\" lang=\"fr\" dir=\"ltr\" class=\"mw-content-ltr\"><table class=\"tableaustandard ficheinfo\">[\n\r\t ]*<tr>[\n\r\t ]*<th [^>]+> <a [^>]+ class=\"image\"><img alt=\"[^\"]+\" src=\"([^\"]+)\" width=\"24\" height=\"24\" /></a>[\n\r\t ]*</th>[\n\r\t ]*<th class=\"entêtesection\" colspan=\"3\">[\n\r\t ]*([^<]+)<br /><span lang=\"ja\"><span class=\"explain\" title=\"\">[^<]+</span></span>[\n\r\t ]*([^<]+)[\n\r\t ]*</th></tr>[\n\r\t ]*<tr>[\n\r\t ]*<td colspan=\"4\" class=\"illustration\"> <a [^>]+><img [^>]+></a>[\n\r\t ]*</td></tr>[\n\r\t ]*<tr>[\n\r\t ]*<td class=\"précision\" colspan=\"4\">.*</a>[\n\r\t ]*</td></tr>[\n\r\t ]*<tr>[\n\r\t ]*<th>[\n\r\t ]*Nom anglais[\n\r\t ]*</th>[\n\r\t ]*<td> <span lang=\"en\">([^<]+)</span>.*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\3");
    englishName1.replace(QRegularExpression("[ \r\t\n]+$",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"");
    itemList[englishName1]=item;
    QString englishName2=data;
    englishName2.replace(QRegularExpression(".*<div id=\"mw-content-text\" lang=\"fr\" dir=\"ltr\" class=\"mw-content-ltr\"><table class=\"tableaustandard ficheinfo\">[\n\r\t ]*<tr>[\n\r\t ]*<th [^>]+> <a [^>]+ class=\"image\"><img alt=\"[^\"]+\" src=\"([^\"]+)\" width=\"24\" height=\"24\" /></a>[\n\r\t ]*</th>[\n\r\t ]*<th class=\"entêtesection\" colspan=\"3\">[\n\r\t ]*([^<]+)<br /><span lang=\"ja\"><span class=\"explain\" title=\"\">[^<]+</span></span>[\n\r\t ]*([^<]+)[\n\r\t ]*</th></tr>[\n\r\t ]*<tr>[\n\r\t ]*<td colspan=\"4\" class=\"illustration\"> <a [^>]+><img [^>]+></a>[\n\r\t ]*</td></tr>[\n\r\t ]*<tr>[\n\r\t ]*<td class=\"précision\" colspan=\"4\">.*</a>[\n\r\t ]*</td></tr>[\n\r\t ]*<tr>[\n\r\t ]*<th>[\n\r\t ]*Nom anglais[\n\r\t ]*</th>[\n\r\t ]*<td> <span lang=\"en\">([^<]+)</span>.*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\4");
    englishName2.replace(QRegularExpression("[ \r\t\n]+$",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"");
    itemList[englishName2]=item;

    qDebug() << "Item:" << path;
}

void loadSkill(const QString &path,const QString &data)
{
    Skill skill;

    if(!data.contains("<div id=\"mw-content-text\" lang=\"fr\" dir=\"ltr\" class=\"mw-content-ltr\"><table class=\"tableaustandard ficheinfo #ffc631\">"))
        return;
    if(!data.contains(QRegularExpression("<th> Nom anglais[\n\r\t ]*</th>[\n\r\t ]*<td> [\n\r\t ]*([^<]+)[\n\r\t ]*</td>")))
        return;
    if(!data.contains(QRegularExpression("title=\"([^\"]+) \\(move\\)\" lang=\"en\"")))
        return;
    if(!data.contains(QRegularExpression("<h1 id=\"firstHeading\" class=\"firstHeading\" lang=\"fr\"><span dir=\"auto\">([^<]+)</span></h1>")))
        return;
    if(!data.contains(QRegularExpression("<td class=\"précision\" colspan=\"2\">[\n\r\t ]*([^<]+)[\n\r\t ]*</td>")))
        return;

    skill.name=data;
    skill.name.replace(QRegularExpression(".*<h1 id=\"firstHeading\" class=\"firstHeading\" lang=\"fr\"><span dir=\"auto\">([^<]+)</span></h1>.*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\1");
    skill.name.replace(QRegularExpression("[ \r\t\n]+$",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"");
    skill.description=data;
    skill.description.replace(QRegularExpression(".*<td class=\"précision\" colspan=\"2\">[\n\r\t ]*([^<]+)[\n\r\t ]*</td>.*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\1");
    skill.description.replace(QRegularExpression("[ \r\t\n]+$",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"");
    QString englishName1=data;
    englishName1.replace(QRegularExpression(".*<th> Nom anglais[\n\r\t ]*</th>[\n\r\t ]*<td> [\n\r\t ]*([^<]+)[\n\r\t ]*</td>.*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\1");
    englishName1.replace(QRegularExpression("[ \r\t\n]+$",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"");
    skillList[englishName1]=skill;
    QString englishName2=data;
    englishName2.replace(QRegularExpression(".*title=\"([^\"]+) \\(move\\)\" lang=\"en\".*",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"\\1");
    englishName2.replace(QRegularExpression("[ \r\t\n]+$",QRegularExpression::MultilineOption|QRegularExpression::DotMatchesEverythingOption),"");
    skillList[englishName2]=skill;

    qDebug() << "Skill:" << path;
}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
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
                QString data=QString::fromUtf8(file.readAll());
                file.close();
                loadMonsterInformations(fileorFolderList.at(index).absoluteFilePath(),data);
                loadItems(fileorFolderList.at(index).absoluteFilePath(),data);
                loadSkill(fileorFolderList.at(index).absoluteFilePath(),data);
            }
        }
        index++;
    }
    return 0;
}
