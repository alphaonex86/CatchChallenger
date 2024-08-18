#include <QCoreApplication>
#include <QFile>
#include <QStringList>
#include <QByteArray>
#include <QDomDocument>
#include <QDebug>
#include <QDir>
#include <QDirIterator>
#include <tinyxml2.h>
#include <iostream>
#include <unordered_map>
#include <unordered_set>

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    //qDebug() << "current path: " << QDir::currentPath();
    QDirIterator it(QDir::currentPath(), QStringList() << "*.tmx", QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext())
    {
        const QString pathTMX=it.next();
        if(!pathTMX.endsWith("tmx"))
            continue;
        QString pathXML=pathTMX.mid(0,pathTMX.size()-4)+".xml";

        unsigned int tempID=1;
        std::unordered_map<std::string,std::string> listOldId;
        std::unordered_set<std::string> intoTMX;
        bool errorFound=false;

        tinyxml2::XMLDocument domDocumentXML;
        auto loadOkay = domDocumentXML.LoadFile(pathXML.toStdString().c_str());
        if(loadOkay!=0)
        {
            std::cerr << pathXML.toStdString() << " error" << std::endl;
            continue;
        }
        tinyxml2::XMLElement *rootXML = domDocumentXML.RootElement();
        if(rootXML==NULL || rootXML->Name()==NULL || strcmp(rootXML->Name(),"map")!=0)
        {
            std::cerr << pathXML.toStdString() << " root is not map" << std::endl;
            continue;
        }
        tinyxml2::XMLElement *botXML = rootXML->FirstChildElement("bot");
        while(botXML!=NULL)
        {
            if(botXML->Attribute("id")!=NULL)
            {
                if(listOldId.find(botXML->Attribute("id"))==listOldId.cend())
                {
                    listOldId[botXML->Attribute("id")]=std::to_string(tempID);
                    tempID++;
                    botXML = botXML->NextSiblingElement("bot");
                }
                else
                {
                    rootXML->DeleteChild(botXML);
                    botXML = rootXML->FirstChildElement("bot");
                    listOldId.clear();
                    //std::cerr << pathXML.toStdString() << " duplicate: " << botXML->Attribute("id") << std::endl;
                }
            }
            else
                botXML = botXML->NextSiblingElement("bot");
        }
        if(errorFound)
            continue;

        tinyxml2::XMLDocument domDocumentTMX;
        loadOkay = domDocumentTMX.LoadFile(pathTMX.toStdString().c_str());
        if(loadOkay!=0)
        {
            std::cerr << pathTMX.toStdString() << " error" << std::endl;
            continue;
        }
        tinyxml2::XMLElement *rootTMX = domDocumentTMX.RootElement();
        if(rootTMX==NULL || rootTMX->Name()==NULL || strcmp(rootTMX->Name(),"map")!=0)
        {
            std::cerr << pathTMX.toStdString() << " root is not map" << std::endl;
            continue;
        }
        tinyxml2::XMLElement *objectgroupTMX = rootTMX->FirstChildElement("objectgroup");
        while(objectgroupTMX!=NULL)
        {
            const tinyxml2::XMLElement *objectTMX = objectgroupTMX->FirstChildElement("object");
            while(objectTMX!=NULL)
            {
                if(objectTMX->Attribute("type")!=NULL)
                {
                    const tinyxml2::XMLElement *propertiesTMX = objectTMX->FirstChildElement("properties");
                    while(propertiesTMX!=NULL)
                    {
                        const tinyxml2::XMLElement *propertyTMX = propertiesTMX->FirstChildElement("property");
                        while(propertyTMX!=NULL)
                        {
                            if(propertyTMX->Attribute("name")!=NULL && propertyTMX->Attribute("value")!=NULL && strcmp(propertyTMX->Attribute("name"),"id")==0)
                            {
                                //propertyTMX->SetAttribute("type","int");
                                if(listOldId.find(propertyTMX->Attribute("value"))!=listOldId.cend())
                                    intoTMX.insert(propertyTMX->Attribute("value"));
                                else
                                {
                                    //errorFound=true;
                                    std::cerr << pathTMX.toStdString() << " not found: " << propertyTMX->Attribute("value") << std::endl;
                                }
                            }
                            propertyTMX = propertyTMX->NextSiblingElement("property");
                        }
                        propertiesTMX = propertiesTMX->NextSiblingElement("properties");
                    }
                }
                objectTMX = objectTMX->NextSiblingElement("object");
            }
            objectgroupTMX = objectgroupTMX->NextSiblingElement("objectgroup");
        }
        if(errorFound)
            continue;

        for (const std::pair<std::string,std::string>& n : listOldId)
        {
            if(intoTMX.find(n.first)==intoTMX.cend())
            {
                errorFound=true;
                std::cerr << pathTMX.toStdString() << " found into XML but not into TMX: " << n.first << std::endl;
            }
        }
        if(errorFound)
            continue;

        botXML = rootXML->FirstChildElement("bot");
        while(botXML!=NULL)
        {
            if(botXML->Attribute("id")!=NULL)
                botXML->SetAttribute("id",listOldId.at(botXML->Attribute("id")).c_str());
            botXML = botXML->NextSiblingElement("bot");
        }
        objectgroupTMX = rootTMX->FirstChildElement("objectgroup");
        while(objectgroupTMX!=NULL)
        {
            tinyxml2::XMLElement *objectTMX = objectgroupTMX->FirstChildElement("object");
            while(objectTMX!=NULL)
            {
                if(objectTMX->Attribute("type")!=NULL)
                {
                    tinyxml2::XMLElement *propertiesTMX = objectTMX->FirstChildElement("properties");
                    while(propertiesTMX!=NULL)
                    {
                        tinyxml2::XMLElement *propertyTMX = propertiesTMX->FirstChildElement("property");
                        while(propertyTMX!=NULL)
                        {
                            if(propertyTMX->Attribute("name")!=NULL && propertyTMX->Attribute("value")!=NULL && strcmp(propertyTMX->Attribute("name"),"id")==0)
                            {
                                propertyTMX->SetAttribute("type","int");
                                if(listOldId.find(propertyTMX->Attribute("value"))!=listOldId.cend())
                                    propertyTMX->SetAttribute("value",listOldId.at(propertyTMX->Attribute("value")).c_str());
                                else
                                    propertyTMX->SetAttribute("value","9999");
                            }
                            propertyTMX = propertyTMX->NextSiblingElement("property");
                        }
                        propertiesTMX = propertiesTMX->NextSiblingElement("properties");
                    }
                }
                objectTMX = objectTMX->NextSiblingElement("object");
            }
            objectgroupTMX = objectgroupTMX->NextSiblingElement("objectgroup");
        }

        auto returnVar=domDocumentXML.SaveFile(pathXML.toStdString().c_str());
        if(returnVar!=tinyxml2::XML_SUCCESS)
        {
            std::cerr << "Unable to save the file: " << pathXML.toStdString() << ", returnVar: " << std::to_string(returnVar) << std::endl;
            abort();
        }
        else
            std::cerr << "Saved: " << pathXML.toStdString() << std::endl;
        returnVar=domDocumentTMX.SaveFile(pathTMX.toStdString().c_str());
        if(returnVar!=tinyxml2::XML_SUCCESS)
        {
            std::cerr << "Unable to save the file: " << pathTMX.toStdString() << ", returnVar: " << std::to_string(returnVar) << std::endl;
            abort();
        }
        else
            std::cerr << "Saved: " << pathTMX.toStdString() << std::endl;
    }

    return 0;
}
