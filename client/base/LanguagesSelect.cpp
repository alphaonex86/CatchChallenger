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
    folderToParse << ":/languages/";
    folderToParse << QCoreApplication::applicationDirPath()+"/languages/";
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
                if(QFile(fileInfo.absoluteFilePath()+"/flag.png").exists() && QFile(fileInfo.absoluteFilePath()+"/informations.xml").exists() && QFile(fileInfo.absoluteFilePath()+"/translation.qm").exists())
                    languageToParse << fileInfo.absoluteFilePath()+"/";
            }
            sub_index++;
        }
        index++;
    }
    index=0;
    while(index<languageToParse.size())
    {
        //open and quick check the file
        QFile xmlFile(languageToParse.at(index)+"informations.xml");
        QByteArray xmlContent;
        if(!xmlFile.open(QIODevice::ReadOnly))
        {
            qDebug() << QString("Unable to open the xml monster file: %1, error: %2").arg(xmlFile.fileName()).arg(xmlFile.errorString());
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
            qDebug() << QString("Unable to open the xml file: %1, Parse error at line %2, column %3: %4").arg(xmlFile.fileName()).arg(errorLine).arg(errorColumn).arg(errorStr);
            index++;
            continue;
        }
        QDomElement root = domDocument.documentElement();
        if(root.tagName()!="language")
        {
            qDebug() << QString("Unable to open the xml file: %1, \"language\" root balise not found for the xml file").arg(xmlFile.fileName());
            index++;
            continue;
        }

        //load the content
        QDomElement fullName = root.firstChildElement("fullName");
        if(fullName.isNull() || !fullName.isElement())
        {
            qDebug() << QString("Unable to open the xml file: %1, \"fullName\" balise not found for the xml file").arg(xmlFile.fileName());
            index++;
            continue;
        }

        QString shortNameMain;
        QStringList shortNameList;
        QDomElement shortName = root.firstChildElement("shortName");
        while(!shortName.isNull())
        {
            if(shortName.isElement())
            {
                if(shortName.hasAttribute("mainCode") && shortName.attribute("mainCode")=="true")
                    shortNameMain=shortName.text();
                shortNameList << shortName.text();
            }
            shortName = shortName.nextSiblingElement("shortName");
        }

        if(shortNameMain.isEmpty())
        {
            qDebug() << QString("Unable to open the xml file: %1, \"shortName\" balise with mainCode=\"true\" not found for the xml file").arg(xmlFile.fileName());
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
        QListWidgetItem *item=new QListWidgetItem(QIcon(i.value().path+"flag.png"),i.value().fullName);
        item->setData(99,i.key());
        if(language==i.key())
            item->setSelected(true);
        ui->listWidget->addItem(item);
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
        return "en";
    else if(language.isEmpty() || !languagesByMainCode.contains(language))
    {
        if(languagesByShortName.contains(QLocale::languageToString(QLocale::system().language())))
            return languagesByShortName[QLocale::languageToString(QLocale::system().language())];
        else if(languagesByShortName.contains(QLocale::system().name()))
            return languagesByShortName[QLocale::system().name()];
        else
            return "en";
    }
    else
        return language;
    return "en";
}

void LanguagesSelect::setCurrentLanguage(const QString &newLanguage)
{
    //protection for re-set the same language
    if(currentLanguage==newLanguage)
        return;
    //unload the old language
    if(currentLanguage!="en")
    {
        int indexTranslator=0;
        while(indexTranslator<installedTranslator.size())
        {
            QCoreApplication::removeTranslator(installedTranslator[indexTranslator]);
            delete installedTranslator[indexTranslator];
            indexTranslator++;
        }
        installedTranslator.clear();
    }
    QTranslator *temp;
    QStringList fileToLoad;
    //load the language main
    if(newLanguage=="en")
        fileToLoad<<":/Languages/en/translation.qm";
    else
        fileToLoad<<languagesByMainCode[newLanguage].path+"translation.qm";
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
    if(temp->load(QString("qt_")+newLanguage, QLibraryInfo::location(QLibraryInfo::TranslationsPath)) && !temp->isEmpty())
    {
        QCoreApplication::installTranslator(temp);
        installedTranslator<<temp;
    }
    else
    {
        if(!temp->load(languagesByMainCode[newLanguage].path+"qt.qm") || temp->isEmpty())
            delete temp;
        else
        {
            QCoreApplication::installTranslator(temp);
            installedTranslator<<temp;
        }
    }
    currentLanguage=newLanguage;
}


void LanguagesSelect::on_cancel_clicked()
{
    close();
}

void LanguagesSelect::on_ok_clicked()
{
    QList<QListWidgetItem *> selectedItems=ui->listWidget->selectedItems();
    if(selectedItems.size()!=1)
        return;
    const QString &language=selectedItems.first()->data(99).toString();
    Options::options.setLanguage(language);
    setCurrentLanguage(language);
    close();
    updateContent();
}
