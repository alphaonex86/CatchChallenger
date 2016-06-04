#include <QCoreApplication>
#include <QDomDocument>
#include <QFile>
#include <QByteArray>
#include <QStringList>
#include <iostream>
#include <cstdint>

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    if(argc!=2)
    {
        std::cerr << "Only accept one arguement, the tmx to compress" << std::endl;
        return -1;
    }

    QDomDocument domDocument;

    //open and quick check the file
    QFile mapFile(argv[1]);
    if(!mapFile.open(QIODevice::ReadOnly))
    {
        std::cerr << mapFile.fileName().toStdString() << ": " << mapFile.errorString().toStdString() << std::endl;
        return -1;
    }
    const QByteArray &xmlContent=mapFile.readAll();
    mapFile.close();
    QString errorStr;
    int errorLine,errorColumn;
    if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
    {
        std::cerr << mapFile.fileName().toStdString() << ", Parse error at line " << std::to_string(errorLine) << " column " << std::to_string(errorColumn) << ": " << errorStr.toStdString() << std::endl;
        return -1;
    }

    QFile xmlFile(argv[1]);
    if(!xmlFile.open(QIODevice::WriteOnly))
    {
        std::cerr << "Unable to open the file in write: " << argv[1] << std::endl;
        return -1;
    }
    xmlFile.write(domDocument.toByteArray(1));
    xmlFile.close();

    return 0;
}
