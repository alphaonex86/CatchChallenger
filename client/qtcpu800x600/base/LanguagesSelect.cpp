#include "LanguagesSelect.h"
#include "ui_LanguagesSelect.h"
#include "../base/Options.h"
#include "../../general/base/tinyXML2/tinyxml2.h"

#include <QStringList>
#include <QDir>
#include <QFile>
#include <QByteArray>
#include <QDebug>
#include <QLibraryInfo>
#include <iostream>

LanguagesSelect *LanguagesSelect::languagesSelect=NULL;

LanguagesSelect::LanguagesSelect() :
    ui(new Ui::LanguagesSelect)
{
    ui->setupUi(this);
    QStringList folderToParse,languageToParse;
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
        tinyxml2::XMLDocument domDocument;
        //open and quick check the file
        const std::string &fileName=languageToParse.at(index).toStdString()+"informations.xml";
        const auto loadOkay = domDocument.LoadFile(fileName.c_str());
        if(loadOkay!=0)
        {
            std::cerr << fileName+", "+domDocument.ErrorName() << std::endl;
            index++;
            continue;
        }

        const tinyxml2::XMLElement *root = domDocument.RootElement();
        if(strcmp(root->Name(),"language")!=0)
        {
            qDebug() << QStringLiteral("Unable to open the xml file: %1, \"language\" root balise not found for the xml file")
                        .arg(QString::fromStdString(fileName));
            index++;
            continue;
        }

        //load the content
        const tinyxml2::XMLElement *fullName = root->FirstChildElement("fullName");
        if(fullName==NULL)
        {
            qDebug() << QStringLiteral("Unable to open the xml file: %1, \"fullName\" balise not found for the xml file")
                        .arg(QString::fromStdString(fileName));
            index++;
            continue;
        }

        QString shortNameMain;
        QStringList shortNameList;
        const tinyxml2::XMLElement *shortName = root->FirstChildElement("shortName");
        while(shortName!=NULL)
        {
            if(shortName->GetText()!=NULL)
            {
                if(shortName->Attribute("mainCode")!=NULL && strcmp(shortName->Attribute("mainCode"),"true")==0)
                    shortNameMain=shortName->GetText();
                shortNameList << shortName->GetText();
            }
            shortName = shortName->NextSiblingElement("shortName");
        }

        if(shortNameMain.isEmpty())
        {
            qDebug() << QStringLiteral("Unable to open the xml file: %1, \"shortName\" balise with mainCode=\"true\" not found for the xml file")
                        .arg(QString::fromStdString(fileName));
            index++;
            continue;
        }

        Language language;
        if(fullName->GetText()!=NULL)
            language.fullName=fullName->GetText();
        language.path=languageToParse.at(index).toStdString();
        languagesByMainCode[shortNameMain.toStdString()]=language;
        int sub_index=0;
        while(sub_index<shortNameList.size())
        {
            languagesByShortName[shortNameList.at(sub_index).toStdString()]=shortNameMain.toStdString();
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

std::string LanguagesSelect::getCurrentLanguages()
{
    /*if(this==NULL)
        return "en";*/
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
    const std::string &language=Options::options.getLanguage();
    ui->automatic->setChecked(language.empty() || languagesByMainCode.find(language)==languagesByMainCode.cend());
    on_automatic_clicked();
    for ( const auto &n : languagesByMainCode ) {
        QListWidgetItem *item=new QListWidgetItem(QIcon(
                                                      QString::fromStdString(n.second.path)+"flag.png"),
                                                  QString::fromStdString(n.second.fullName)
                                                  );
        item->setData(99,QString::fromStdString(n.first));
        ui->listWidget->addItem(item);
        if(language==n.first)
            item->setSelected(true);
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

std::string LanguagesSelect::getTheRightLanguage() const
{
    const std::string &language=Options::options.getLanguage();
    if(languagesByShortName.size()==0)
        return "en";
    else if(language.empty() || languagesByMainCode.find(language)==languagesByMainCode.cend())
    {
        if(languagesByShortName.find(QLocale::languageToString(QLocale::system().language()).toStdString())!=
                languagesByShortName.cend())
            return languagesByShortName.at(QLocale::languageToString(QLocale::system().language()).toStdString());
        else if(languagesByShortName.find(QLocale::system().name().toStdString())!=
                languagesByShortName.cend())
            return languagesByShortName.at(QLocale::system().name().toStdString());
        else
            return "en";
    }
    else
        return language;
    return "en";
}

void LanguagesSelect::setCurrentLanguage(const std::string &newLanguage)
{
    //protection for re-set the same language
    if(currentLanguage==newLanguage)
        return;
    //unload the old language
    unsigned int indexTranslator=0;
    while(indexTranslator<installedTranslator.size())
    {
        QCoreApplication::removeTranslator(installedTranslator.at(indexTranslator));
        delete installedTranslator.at(indexTranslator);
        indexTranslator++;
    }
    installedTranslator.clear();
    QTranslator *temp;
    QStringList fileToLoad;
    //load the language main
    QDir dir;
    if(newLanguage=="en")
        dir=QDir(":/Languages/en/");
    else
        dir=QDir(QString::fromStdString(languagesByMainCode.at(newLanguage).path));
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
            installedTranslator.push_back(temp);
        }
        indexTranslationFile++;
    }
    temp=new QTranslator();
    if(temp->load(QStringLiteral("qt_")+QString::fromStdString(newLanguage), QLibraryInfo::location(QLibraryInfo::TranslationsPath)) && !temp->isEmpty())
    {
        QCoreApplication::installTranslator(temp);
        installedTranslator.push_back(temp);
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
        const std::string &language=selectedItems.first()->data(99).toString().toStdString();
        Options::options.setLanguage(language);
        setCurrentLanguage(language);
    }
    else
    {
        Options::options.setLanguage(std::string());
        setCurrentLanguage(getTheRightLanguage());
    }
    updateContent();
    close();
}
