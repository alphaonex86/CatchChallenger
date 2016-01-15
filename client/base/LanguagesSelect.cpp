#include "LanguagesSelect.h"
#include "ui_LanguagesSelect.h"
#include "../base/Options.h"

#include <QStringList>
#include <QDir>
#include <QFile>
#include <QDomDocument>
#include <QDomElement>
#include <QByteArray>
#include <QDebug>
#include <QLibraryInfo>

LanguagesSelect *LanguagesSelect::languagesSelect=NULL;

LanguagesSelect::LanguagesSelect() :
    ui(new Ui::LanguagesSelect)
{
    ui->setupUi(this);
    QStringList folderToParse,languageToParse;
    folderToParse << QStringLiteral(":/languages/");
    folderToParse << QCoreApplication::applicationDirPath()+QStringLiteral("/languages/");
    int index;
    index=0;
    while(index<folderToParse.size())
    {
        QDir dir(folderToParse.at(index));
        QFileInfoList fileInfoList=dir.entryInfoList(QDir::AllDirs|QDir::NoDotAndDotDot);
        int sub_index=0;
        while(sub_index<fileInfoList.size())
        {
            const QFileInfo &fileInfo=fileInfoList.at(sub_index);
            if(fileInfo.isDir())
            {
                if(QFile(fileInfo.absoluteFilePath()+QStringLiteral("/flag.png")).exists() && QFile(fileInfo.absoluteFilePath()+QStringLiteral("/informations.xml")).exists() && QFile(fileInfo.absoluteFilePath()+QStringLiteral("/translation.qm")).exists())
                    languageToParse << fileInfo.absoluteFilePath()+QStringLiteral("/");
            }
            sub_index++;
        }
        index++;
    }
    index=0;
    while(index<languageToParse.size())
    {
        //open and quick check the file
        QFile xmlFile(languageToParse.at(index)+QStringLiteral("informations.xml"));
        QByteArray xmlContent;
        if(!xmlFile.open(QIODevice::ReadOnly))
        {
            qDebug() << QStringLiteral("Unable to open the xml monster file: %1, error: %2").arg(xmlFile.fileName()).arg(xmlFile.errorString());
            index++;
            continue;
        }
        xmlContent=xmlFile.readAll();
        xmlFile.close();
        QDomDocument domDocument;
        QString errorStr;
        int errorLine,errorColumn;
        if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
        {
            qDebug() << QStringLiteral("Unable to open the xml file: %1, Parse error at line %2, column %3: %4").arg(xmlFile.fileName()).arg(errorLine).arg(errorColumn).arg(errorStr);
            index++;
            continue;
        }
        QDomElement root = domDocument.documentElement();
        if(root.tagName()!=QStringLiteral("language"))
        {
            qDebug() << QStringLiteral("Unable to open the xml file: %1, \"language\" root balise not found for the xml file").arg(xmlFile.fileName());
            index++;
            continue;
        }

        //load the content
        QDomElement fullName = root.firstChildElement(QStringLiteral("fullName"));
        if(fullName.isNull() || !fullName.isElement())
        {
            qDebug() << QStringLiteral("Unable to open the xml file: %1, \"fullName\" balise not found for the xml file").arg(xmlFile.fileName());
            index++;
            continue;
        }

        QString shortNameMain;
        QStringList shortNameList;
        QDomElement shortName = root.firstChildElement(QStringLiteral("shortName"));
        while(!shortName.isNull())
        {
            if(shortName.isElement())
            {
                if(shortName.hasAttribute(QStringLiteral("mainCode")) && shortName.attribute(QStringLiteral("mainCode"))==QStringLiteral("true"))
                    shortNameMain=shortName.text();
                shortNameList << shortName.text();
            }
            shortName = shortName.nextSiblingElement(QStringLiteral("shortName"));
        }

        if(shortNameMain.isEmpty())
        {
            qDebug() << QStringLiteral("Unable to open the xml file: %1, \"shortName\" balise with mainCode=\"true\" not found for the xml file").arg(xmlFile.fileName());
            index++;
            continue;
        }

        Language language;
        language.fullName=fullName.text();
        language.path=languageToParse.at(index);
        languagesByMainCode[shortNameMain]=language;
        int sub_index=0;
        while(sub_index<shortNameList.size())
        {
            languagesByShortName[shortNameList.at(sub_index)]=shortNameMain;
            sub_index++;
        }
        index++;
    }
    setCurrentLanguage(getTheRightLanguage());
}

LanguagesSelect::~LanguagesSelect()
{
    delete ui;
}

QString LanguagesSelect::getCurrentLanguages()
{
    if(this==NULL)
        return "en";
    return currentLanguage;
}

void LanguagesSelect::show()
{
    updateContent();
    QWidget::show();
}

void LanguagesSelect::updateContent()
{
    ui->listWidget->clear();
    const QString &language=Options::options.getLanguage();
    ui->automatic->setChecked(language.isEmpty() || !languagesByMainCode.contains(language));
    on_automatic_clicked();
    QHash<QString,Language>::const_iterator i = languagesByMainCode.constBegin();
    while (i != languagesByMainCode.constEnd()) {
        QListWidgetItem *item=new QListWidgetItem(QIcon(i.value().path+QStringLiteral("flag.png")),i.value().fullName);
        item->setData(99,i.key());
        ui->listWidget->addItem(item);
        if(language==i.key())
            item->setSelected(true);
        ++i;
    }
}

int LanguagesSelect::exec()
{
    updateContent();
    return QDialog::exec();
}

void LanguagesSelect::on_automatic_clicked()
{
    ui->listWidget->setEnabled(!ui->automatic->isChecked());
}

QString LanguagesSelect::getTheRightLanguage() const
{
    const QString &language=Options::options.getLanguage();
    if(languagesByShortName.size()==0)
        return QLatin1Literal("en");
    else if(language.isEmpty() || !languagesByMainCode.contains(language))
    {
        if(languagesByShortName.contains(QLocale::languageToString(QLocale::system().language())))
            return languagesByShortName.value(QLocale::languageToString(QLocale::system().language()));
        else if(languagesByShortName.contains(QLocale::system().name()))
            return languagesByShortName.value(QLocale::system().name());
        else
            return QLatin1Literal("en");
    }
    else
        return language;
    return QLatin1Literal("en");
}

void LanguagesSelect::setCurrentLanguage(const QString &newLanguage)
{
    //protection for re-set the same language
    if(currentLanguage==newLanguage)
        return;
    //unload the old language
    int indexTranslator=0;
    while(indexTranslator<installedTranslator.size())
    {
        QCoreApplication::removeTranslator(installedTranslator.value(indexTranslator));
        delete installedTranslator.value(indexTranslator);
        indexTranslator++;
    }
    installedTranslator.clear();
    QTranslator *temp;
    QStringList fileToLoad;
    //load the language main
    QDir dir;
    if(newLanguage==QStringLiteral("en"))
        dir=QDir(QStringLiteral(":/Languages/en/"));
    else
        dir=QDir(languagesByMainCode.value(newLanguage).path);
    QFileInfoList fileInfoList=dir.entryInfoList(QDir::Files|QDir::NoDotAndDotDot);
    int index=0;
    while(index<fileInfoList.size())
    {
        const QFileInfo &fileInfo=fileInfoList.at(index);
        if(fileInfo.suffix()==QStringLiteral("qm"))
            fileToLoad << fileInfo.absoluteFilePath();
        index++;
    }

    int indexTranslationFile=0;
    while(indexTranslationFile<fileToLoad.size())
    {
        temp=new QTranslator();
        if(!temp->load(fileToLoad.at(indexTranslationFile)) || temp->isEmpty())
            delete temp;
        else
        {
            QCoreApplication::installTranslator(temp);
            installedTranslator<<temp;
        }
        indexTranslationFile++;
    }
    temp=new QTranslator();
    if(temp->load(QStringLiteral("qt_")+newLanguage, QLibraryInfo::location(QLibraryInfo::TranslationsPath)) && !temp->isEmpty())
    {
        QCoreApplication::installTranslator(temp);
        installedTranslator<<temp;
    }
    currentLanguage=newLanguage;
}


void LanguagesSelect::on_cancel_clicked()
{
    close();
}

void LanguagesSelect::on_ok_clicked()
{
    if(!ui->automatic->isChecked())
    {
        QList<QListWidgetItem *> selectedItems=ui->listWidget->selectedItems();
        if(selectedItems.size()!=1)
            return;
        const QString &language=selectedItems.first()->data(99).toString();
        Options::options.setLanguage(language);
        setCurrentLanguage(language);
    }
    else
    {
        Options::options.setLanguage(QString());
        setCurrentLanguage(getTheRightLanguage());
    }
    updateContent();
    close();
}
