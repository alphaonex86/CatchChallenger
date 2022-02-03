#include <QCoreApplication>
#include <QFile>
#include <QStringList>
#include <QByteArray>
#include <QDomDocument>
#include <QDebug>

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    if(argc==2)
    {
        QFile xmlFile(a.arguments().last());
        if(!xmlFile.open(QIODevice::ReadWrite))
        {
            qDebug() << QString("Unable to open the file %1:\n%2").arg(xmlFile.fileName()).arg(xmlFile.errorString());
            return -2;
        }
        QDomDocument domDocument;
        QByteArray xmlContent=xmlFile.readAll();
        QString errorStr;
        int errorLine,errorColumn;
        if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
        {
            qDebug() << QString("Unable to open the file %1:\nParse error at line %2, column %3: %4").arg(xmlFile.fileName()).arg(errorLine).arg(errorColumn).arg(errorStr);
            return -1;
        }
        xmlFile.seek(0);
        xmlFile.resize(0);
        QByteArray newData=domDocument.toByteArray(4);
        newData.replace(QString("\t").toUtf8(),QString("    ").toUtf8());
        xmlFile.write(newData);
        xmlFile.close();
        return 0;
    }
    else
        return 1;
}
