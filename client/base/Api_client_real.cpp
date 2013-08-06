#include "Api_client_real.h"

using namespace CatchChallenger;

#ifdef Q_CC_GNU
//this next header is needed to change file time/date under gcc
#include <utime.h>
#include <sys/stat.h>
#endif

#include <QApplication>
#include <QRegularExpression>

//need host + port here to have datapack base

Api_protocol* Api_client_real::client=NULL;

Api_client_real::Api_client_real(ConnectedSocket *socket,bool tolerantMode) :
    Api_protocol(socket,tolerantMode)
{
    host="localhost";
    port=42489;
    datapack_base_name=QString("%1/%2-%3/").arg(QApplication::applicationDirPath()).arg(host).arg(port);
    connect(socket, &ConnectedSocket::disconnected,	this,&Api_client_real::disconnected);
    connect(this,   &Api_client_real::newFile,      this,&Api_client_real::writeNewFile);
    disconnected();
    dataClear();
}

Api_client_real::~Api_client_real()
{
    socket->abort();
    socket->disconnectFromHost();
    if(socket->state()!=QAbstractSocket::UnconnectedState)
        socket->waitForDisconnected();
}

void Api_client_real::parseFullReplyData(const quint8 &mainCodeType,const quint16 &subCodeType,const quint8 &queryNumber,const QByteArray &data)
{
    QDataStream in(data);
    in.setVersion(QDataStream::Qt_4_4);
    switch(mainCodeType)
    {
        case 0x02:
        {
            //local the query number to get the type
            switch(subCodeType)
            {
                //Send datapack file list
                case 0x000C:
                    if((in.device()->size()-in.device()->pos())!=((int)sizeof(quint8))*datapackFilesList.size())
                    {
                        parseError(tr("Procotol wrong or corrupted"),QString("wrong size to return file list"));
                        return;
                    }
                    quint8 reply_code;
                    {
                        int index=0;
                        while(index<datapackFilesList.size())
                        {
                            in >> reply_code;
                            if(reply_code==0x02)
                            {
                                DebugClass::debugConsole(QString("remove the file: %1").arg(datapack_base_name+"/"+datapackFilesList.at(index)));
                                QFile file(datapack_base_name+"/"+datapackFilesList.at(index));
                                if(!file.remove())
                                    DebugClass::debugConsole(QString("unable to remove the file: %1: %2").arg(datapackFilesList.at(index)).arg(file.errorString()));
                                //emit removeFile(datapackFilesList.at(index));
                            }
                            index++;
                        }
                        datapackFilesList.clear();
                        cleanDatapack("");
                        emit haveTheDatapack();
                    }
                break;
                default:
                    Api_protocol::parseFullReplyData(mainCodeType,subCodeType,queryNumber,data);
                    return;
                break;
            }
        }
        break;
        default:
            Api_protocol::parseFullReplyData(mainCodeType,subCodeType,queryNumber,data);
            return;
        break;
    }
    if((in.device()->size()-in.device()->pos())!=0)
    {
        QByteArray data_remaining=data.right(data.size()-in.device()->pos());
        parseError(tr("Procotol wrong or corrupted"),QString("error: remaining data: Api_client_real::parseReplyData(%1,%2,%3): %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(QString(data_remaining.toHex())));
        return;
    }
}

//general data
void Api_client_real::defineMaxPlayers(const quint16 &maxPlayers)
{
    ProtocolParsing::setMaxPlayers(maxPlayers);
}

void Api_client_real::resetAll()
{
    RXSize=0;
    TXSize=0;
    query_files_list.clear();
    wait_datapack_content=false;

    Api_protocol::resetAll();
}

void Api_client_real::tryConnect(QString host,quint16 port)
{
    DebugClass::debugConsole(QString("Try connect on: %1:%2").arg(host).arg(port));
    this->host=host;
    this->port=port;
    socket->connectToHost(host,port);
    datapack_base_name=QString("%1/%2-%3/").arg(QApplication::applicationDirPath()).arg(host).arg(port);
}

void Api_client_real::disconnected()
{
    wait_datapack_content=false;
    haveData=false;
    dataClear();
    resetAll();
}

void Api_client_real::writeNewFile(const QString &fileName,const QByteArray &data,const quint64 &mtime)
{
    QFile file(datapack_base_name+"/"+fileName);
    QFileInfo fileInfo(file);

    QDir(fileInfo.absolutePath()).mkpath(fileInfo.absolutePath());

    if(!file.open(QIODevice::WriteOnly))
    {
        DebugClass::debugConsole(QString("Can't open: %1: %2").arg(fileName).arg(file.errorString()));
        return;
    }
    if(file.write(data)!=data.size())
    {
        file.close();
        DebugClass::debugConsole(QString("Can't write: %1: %2").arg(fileName).arg(file.errorString()));
        return;
    }
    file.close();

    time_t actime=QFileInfo(file).lastRead().toTime_t();
    //protect to wrong actime
    if(actime<0)
        actime=0;
    time_t modtime=mtime;
    if(modtime<0)
    {
        DebugClass::debugConsole(QString("Last modified date is wrong: %1: %2").arg(fileName).arg(mtime));
        return;
    }
    #ifdef Q_CC_GNU
        //this function avalaible on unix and mingw
        utimbuf butime;
        butime.actime=actime;
        butime.modtime=modtime;
        int returnVal=utime(file.fileName().toLocal8Bit().data(),&butime);
        if(returnVal==0)
            return;
        else
        {
            DebugClass::debugConsole(QString("Can't set time: %1").arg(fileName));
            return;
        }
    #else
        #error "Not supported on this platform"
    #endif
}

/*void Api_client_real::errorOutput(QString error,QString detailedError)
{
    error_string=error;
    emit haveNewError();
    DebugClass::debugConsole("User message: "+error);
    socket->disconnectFromHost();
    if(!detailedError.isEmpty())
        DebugClass::debugConsole(detailedError);
}*/

void Api_client_real::tryDisconnect()
{
    socket->disconnectFromHost();
}

QString Api_client_real::getHost()
{
    return host;
}

quint16 Api_client_real::getPort()
{
    return port;
}

void Api_client_real::sendDatapackContent()
{
    if(wait_datapack_content)
    {
        DebugClass::debugConsole(QString("already in wait of datapack content"));
        return;
    }
    wait_datapack_content=true;
    datapack_base_name=QString("%1/%2-%3/").arg(QApplication::applicationDirPath()).arg(host).arg(port);
    QDir(datapack_base_name).mkpath(datapack_base_name);
    quint8 datapack_content_query_number=queryNumber();
    datapackFilesList=listDatapack("");
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << (quint32)datapackFilesList.size();
    int index=0;
    while(index<datapackFilesList.size())
    {
        out << datapackFilesList.at(index);
        struct stat info;
        stat(QString(datapack_base_name+datapackFilesList.at(index)).toLatin1().data(),&info);
        out << (quint64)info.st_mtime;
        index++;
    }
    output->packFullOutcommingQuery(0x02,0x000C,datapack_content_query_number,outputData);
}

const QStringList Api_client_real::listDatapack(QString suffix)
{
    QStringList returnFile;
    QDir finalDatapackFolder(datapack_base_name+suffix);
    QFileInfoList entryList=finalDatapackFolder.entryInfoList(QDir::AllEntries|QDir::NoDotAndDotDot|QDir::Hidden|QDir::System,QDir::DirsFirst);//possible wait time here
    int sizeEntryList=entryList.size();
    for (int index=0;index<sizeEntryList;++index)
    {
        QFileInfo fileInfo=entryList.at(index);
        if(fileInfo.isDir())
            returnFile << listDatapack(suffix+fileInfo.fileName()+"/");//put unix separator because it's transformed into that's under windows too
        else
        {
            //if match with correct file name, considere as valid
            if(fileInfo.fileName().contains(QRegularExpression(DATAPACK_FILE_REGEX)))
                returnFile << suffix+fileInfo.fileName();
            //is invalid
            else
            {
                DebugClass::debugConsole(QString("listDatapack(): remove invalid file: %1").arg(suffix+fileInfo.fileName()));
                QFile file(datapack_base_name+suffix+fileInfo.fileName());
                if(!file.remove())
                    DebugClass::debugConsole(QString("listDatapack(): unable remove invalid file: %1: %2").arg(suffix+fileInfo.fileName()).arg(file.errorString()));
            }
        }
    }
    return returnFile;
}

void Api_client_real::cleanDatapack(QString suffix)
{
    QDir finalDatapackFolder(datapack_base_name+suffix);
    QFileInfoList entryList=finalDatapackFolder.entryInfoList(QDir::AllEntries|QDir::NoDotAndDotDot|QDir::Hidden|QDir::System,QDir::DirsFirst);//possible wait time here
    int sizeEntryList=entryList.size();
    for (int index=0;index<sizeEntryList;++index)
    {
        QFileInfo fileInfo=entryList.at(index);
        if(fileInfo.isDir())
            cleanDatapack(suffix+fileInfo.fileName()+"/");//put unix separator because it's transformed into that's under windows too
        else
            return;
    }
    entryList=finalDatapackFolder.entryInfoList(QDir::AllEntries|QDir::NoDotAndDotDot|QDir::Hidden|QDir::System,QDir::DirsFirst);//possible wait time here
    if(entryList.size()==0)
        finalDatapackFolder.rmpath(datapack_base_name+suffix);
}
