#include "Api_client_real.h"

using namespace CatchChallenger;

#ifdef Q_CC_GNU
//this next header is needed to change file time/date under gcc
#include <utime.h>
#include <sys/stat.h>
#endif

#include <QApplication>
#include <QRegularExpression>
#include <QNetworkReply>

//need host + port here to have datapack base

Api_protocol* Api_client_real::client=NULL;

QString Api_client_real::text_slash=QLatin1Literal("/");

Api_client_real::Api_client_real(ConnectedSocket *socket,bool tolerantMode) :
    Api_protocol(socket,tolerantMode)
{
    host=QLatin1Literal("localhost");
    port=42489;
    connect(socket, &ConnectedSocket::disconnected,	this,&Api_client_real::disconnected);
    connect(this,   &Api_client_real::newFile,      this,&Api_client_real::writeNewFile);
    connect(this,   &Api_client_real::newHttpFile,  this,&Api_client_real::getHttpFile);
    disconnected();
    //dataClear();do into disconnected()
}

Api_client_real::~Api_client_real()
{
    if(socket!=NULL)
    {
        socket->abort();
        socket->disconnectFromHost();
        if(socket->state()!=QAbstractSocket::UnconnectedState)
            socket->waitForDisconnected();
    }
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
                    {
                        if(datapackFilesList.isEmpty() && data.size()==1)
                        {
                            if(!httpMode)
                                emit haveTheDatapack();
                            return;
                        }
                        QList<bool> boolList;
                        while((in.device()->size()-in.device()->pos())>0)
                        {
                            quint8 returnCode;
                            in >> returnCode;
                            boolList.append(returnCode&0x01);
                            boolList.append(returnCode&0x02);
                            boolList.append(returnCode&0x04);
                            boolList.append(returnCode&0x08);
                            boolList.append(returnCode&0x10);
                            boolList.append(returnCode&0x20);
                            boolList.append(returnCode&0x40);
                            boolList.append(returnCode&0x80);
                        }
                        if(boolList.size()<datapackFilesList.size())
                        {
                            emit newError(tr("Procotol wrong or corrupted"),QStringLiteral("bool list too small with main ident: %1, subCodeType:%2, and queryNumber: %3, type: query_type_protocol").arg(mainCodeType).arg(subCodeType).arg(queryNumber));
                            return;
                        }
                        int index=0;
                        while(index<datapackFilesList.size())
                        {
                            if(boolList.first())
                            {
                                DebugClass::debugConsole(QStringLiteral("remove the file: %1").arg(mDatapack+text_slash+datapackFilesList.at(index)));
                                QFile file(mDatapack+text_slash+datapackFilesList.at(index));
                                if(!file.remove())
                                    DebugClass::debugConsole(QStringLiteral("unable to remove the file: %1: %2").arg(datapackFilesList.at(index)).arg(file.errorString()));
                                //emit removeFile(datapackFilesList.at(index));
                            }
                            boolList.removeFirst();
                            index++;
                        }
                        datapackFilesList.clear();
                        cleanDatapack(QString());
                        if(boolList.size()>=8)
                        {
                            emit newError(tr("Procotol wrong or corrupted"),QStringLiteral("bool list too big with main ident: %1, subCodeType:%2, and queryNumber: %3, type: query_type_protocol").arg(mainCodeType).arg(subCodeType).arg(queryNumber));
                            return;
                        }
                        if(!httpMode)
                            emit haveTheDatapack();
                    }
                    return;
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
        parseError(tr("Procotol wrong or corrupted"),QStringLiteral("error: remaining data: Api_client_real::parseReplyData(%1,%2,%3): %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(QString(data_remaining.toHex())));
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
    httpMode=false;
    httpError=false;
    RXSize=0;
    TXSize=0;
    query_files_list.clear();
    urlInWaitingList.clear();
    wait_datapack_content=false;

    Api_protocol::resetAll();
}

void Api_client_real::tryConnect(QString host,quint16 port)
{
    if(socket==NULL)
        return;
    DebugClass::debugConsole(QStringLiteral("Try connect on: %1:%2").arg(host).arg(port));
    this->host=host;
    this->port=port;
    socket->connectToHost(host,port);
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
    const QString &fullPath=mDatapack+text_slash+fileName;
    //to be sure the QFile is destroyed
    {
        QFile file(fullPath);
        QFileInfo fileInfo(file);

        QDir(fileInfo.absolutePath()).mkpath(fileInfo.absolutePath());

        if(file.exists())
            if(!file.remove())
            {
                DebugClass::debugConsole(QStringLiteral("Can't remove: %1: %2").arg(fileName).arg(file.errorString()));
                return;
            }
        if(!file.open(QIODevice::WriteOnly))
        {
            DebugClass::debugConsole(QStringLiteral("Can't open: %1: %2").arg(fileName).arg(file.errorString()));
            return;
        }
        if(file.write(data)!=data.size())
        {
            file.close();
            DebugClass::debugConsole(QStringLiteral("Can't write: %1: %2").arg(fileName).arg(file.errorString()));
            return;
        }
        file.flush();
        file.close();
    }

    //set the modification time
    {
        time_t actime=QFileInfo(fullPath).lastRead().toTime_t();
        //protect to wrong actime
        if(actime<0)
            actime=0;
        time_t modtime=mtime;
        if(modtime<0)
        {
            DebugClass::debugConsole(QStringLiteral("Last modified date is wrong: %1: %2").arg(fileName).arg(mtime));
            return;
        }
        emit newDatapackFile(data.size());
        #ifdef Q_CC_GNU
            //this function avalaible on unix and mingw
            utimbuf butime;
            butime.actime=actime;
            butime.modtime=modtime;
            int returnVal=utime(fullPath.toLocal8Bit().data(),&butime);
            if(returnVal==0)
                return;
            else
            {
                DebugClass::debugConsole(QStringLiteral("Can't set time: %1").arg(fileName));
                return;
            }
        #else
            #error "Not supported on this platform"
        #endif
    }
}

void Api_client_real::getHttpFile(const QString &url,const QString &fileName,const quint64 &mtime)
{
    if(httpError)
        return;
    if(!httpMode)
        httpMode=true;
    QNetworkRequest networkRequest(url);
    QNetworkReply *reply = qnam.get(networkRequest);
    UrlInWaiting urlInWaiting;
    urlInWaiting.fileName=fileName;
    urlInWaiting.mtime=mtime;
    urlInWaitingList[reply]=urlInWaiting;
    connect(reply, &QNetworkReply::finished, this, &Api_client_real::httpFinished);
}

void Api_client_real::httpFinished()
{
    if(httpError)
        return;
    if(urlInWaitingList.isEmpty())
    {
        httpError=true;
        emit error(QStringLiteral("no more reply in waiting"));
        socket->disconnectFromHost();
        return;
    }
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    if(reply==NULL)
    {
        httpError=true;
        emit error(QStringLiteral("reply for http is NULL"));
        socket->disconnectFromHost();
        reply->deleteLater();
        return;
    }
    QVariant redirectionTarget = reply->attribute(QNetworkRequest::RedirectionTargetAttribute);
    if (!reply->isFinished())
    {
        httpError=true;
        emit newError(tr("Unable to download the datapack"),QStringLiteral("get the new update failed: not finished"));
        socket->disconnectFromHost();
        reply->deleteLater();
        return;
    }
    else if (reply->error())
    {
        httpError=true;
        emit newError(tr("Unable to download the datapack"),QStringLiteral("get the new update failed: %1").arg(reply->errorString()));
        socket->disconnectFromHost();
        reply->deleteLater();
        return;
    } else if (!redirectionTarget.isNull()) {
        httpError=true;
        emit newError(tr("Unable to download the datapack"),QStringLiteral("redirection denied to: %1").arg(redirectionTarget.toUrl().toString()));
        socket->disconnectFromHost();
        reply->deleteLater();
        return;
    }
    if(!urlInWaitingList.contains(reply))
    {
        httpError=true;
        emit error(QStringLiteral("reply of unknown query"));
        socket->disconnectFromHost();
        reply->deleteLater();
        return;
    }

    const UrlInWaiting &urlInWaiting=urlInWaitingList.value(reply);
    writeNewFile(urlInWaiting.fileName,reply->readAll(),urlInWaiting.mtime);

    if(urlInWaitingList.remove(reply)!=1)
        DebugClass::debugConsole(QStringLiteral("[Bug] Remain %1 file to download").arg(urlInWaitingList.size()));
    reply->deleteLater();
    if(urlInWaitingList.isEmpty())
        emit haveTheDatapack();
}

void Api_client_real::tryDisconnect()
{
    if(socket!=NULL)
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
        DebugClass::debugConsole(QStringLiteral("already in wait of datapack content"));
        return;
    }
    wait_datapack_content=true;
    quint8 datapack_content_query_number=queryNumber();
    datapackFilesList=listDatapack(QString());
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << (quint32)datapackFilesList.size();
    int index=0;
    while(index<datapackFilesList.size())
    {
        out << datapackFilesList.at(index);
        struct stat info;
        stat(QString(mDatapack+datapackFilesList.at(index)).toLatin1().data(),&info);
        out << (quint64)info.st_mtime;
        index++;
    }
    if(output==NULL)
        return;
    output->packFullOutcommingQuery(0x02,0x000C,datapack_content_query_number,outputData);
}

const QStringList Api_client_real::listDatapack(QString suffix)
{
    QStringList returnFile;
    QDir finalDatapackFolder(mDatapack+suffix);
    QFileInfoList entryList=finalDatapackFolder.entryInfoList(QDir::AllEntries|QDir::NoDotAndDotDot|QDir::Hidden|QDir::System,QDir::DirsFirst);//possible wait time here
    int sizeEntryList=entryList.size();
    for (int index=0;index<sizeEntryList;++index)
    {
        QFileInfo fileInfo=entryList.at(index);
        if(fileInfo.isDir())
            returnFile << listDatapack(suffix+fileInfo.fileName()+text_slash);//put unix separator because it's transformed into that's under windows too
        else
        {
            //if match with correct file name, considere as valid
            if(fileInfo.fileName().contains(QRegularExpression(DATAPACK_FILE_REGEX)) && extensionAllowed.contains(fileInfo.suffix()))
                returnFile << suffix+fileInfo.fileName();
            //is invalid
            else
            {
                DebugClass::debugConsole(QStringLiteral("listDatapack(): remove invalid file: %1").arg(suffix+fileInfo.fileName()));
                QFile file(mDatapack+suffix+fileInfo.fileName());
                if(!file.remove())
                    DebugClass::debugConsole(QStringLiteral("listDatapack(): unable remove invalid file: %1: %2").arg(suffix+fileInfo.fileName()).arg(file.errorString()));
            }
        }
    }
    return returnFile;
}

void Api_client_real::cleanDatapack(QString suffix)
{
    QDir finalDatapackFolder(mDatapack+suffix);
    QFileInfoList entryList=finalDatapackFolder.entryInfoList(QDir::AllEntries|QDir::NoDotAndDotDot|QDir::Hidden|QDir::System,QDir::DirsFirst);//possible wait time here
    int sizeEntryList=entryList.size();
    for (int index=0;index<sizeEntryList;++index)
    {
        QFileInfo fileInfo=entryList.at(index);
        if(fileInfo.isDir())
            cleanDatapack(suffix+fileInfo.fileName()+text_slash);//put unix separator because it's transformed into that's under windows too
        else
            return;
    }
    entryList=finalDatapackFolder.entryInfoList(QDir::AllEntries|QDir::NoDotAndDotDot|QDir::Hidden|QDir::System,QDir::DirsFirst);//possible wait time here
    if(entryList.size()==0)
        finalDatapackFolder.rmpath(mDatapack+suffix);
}
