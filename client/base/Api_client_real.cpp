#include "Api_client_real.h"

using namespace Pokecraft;

#ifdef Q_CC_GNU
//this next header is needed to change file time/date under gcc
#include <utime.h>
#include <sys/stat.h>
#endif

//need host + port here to have datapack base

Api_client_real::Api_client_real() :
	Api_protocol(&socket)
{
	host="localhost";
	port=42489;
	datapack_base_name=QString("%1/%2-%3/").arg(QApplication::applicationDirPath()).arg(host).arg(port);
	connect(&socket,SIGNAL(stateChanged(QAbstractSocket::SocketState)),	this,SIGNAL(stateChanged(QAbstractSocket::SocketState)));
	connect(&socket,SIGNAL(error(QAbstractSocket::SocketError)),		this,SIGNAL(error(QAbstractSocket::SocketError)));
	connect(&socket,SIGNAL(readyRead()),					this,SLOT(readyRead()));
	connect(&socket,SIGNAL(disconnected()),					this,SLOT(disconnected()));
	disconnected();
	dataClear();
}

Api_client_real::~Api_client_real()
{
	socket.abort();
	socket.disconnectFromHost();
	if(socket.state()!=QAbstractSocket::UnconnectedState)
		socket.waitForDisconnected();
}

void Api_client_real::parseReplyData(const quint8 &mainCodeType,const quint16 &subCodeType,const quint8 &queryNumber,const QByteArray &data)
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
					QByteArray rawData=qUncompress(data);
					QDataStream in(rawData);
					in.setVersion(QDataStream::Qt_4_4);
					if((in.device()->size()-in.device()->pos())!=((int)sizeof(quint8))*datapackFilesList.size())
					{
						emit newError(tr("Procotol wrong or corrupted"),QString("wrong size to return file list"));
						return;
					}
					quint8 reply_code;
					int index=0;
					while(index<datapackFilesList.size())
					{
						in >> reply_code;
						if(reply_code==0x02)
						{
							DebugClass::debugConsole(QString("remove the file: %1").arg(datapackFilesList.at(index)));
							emit removeFile(datapackFilesList.at(index));
						}
						index++;
					}
					datapackFilesList.clear();
					emit haveTheDatapack();
				}
				break;
				default:
					Api_protocol::parseReplyData(mainCodeType,subCodeType,queryNumber,data);
					return;
				break;
			}
		}
		break;
		default:
			Api_protocol::parseReplyData(mainCodeType,subCodeType,queryNumber,data);
			return;
		break;
	}
	if((in.device()->size()-in.device()->pos())!=0)
	{
		emit newError(tr("Procotol wrong or corrupted"),QString("remaining data: Api_client_real::parseReplyData(%1,%2,%3)").arg(mainCodeType).arg(subCodeType).arg(queryNumber));
		return;
	}
}

void Api_client_real::resetAll()
{
	query_files_list.clear();
	wait_datapack_content=false;

	Api_protocol::resetAll();
}

void Api_client_real::tryConnect(QString host,quint16 port)
{
	DebugClass::debugConsole(QString("Try connect on: %1:%2").arg(host).arg(port));
	this->host=host;
	this->port=port;
	socket.connectToHost(host,port);
	datapack_base_name=QString("%1/%2-%3/").arg(QApplication::applicationDirPath()).arg(host).arg(port);
}

void Api_client_real::disconnected()
{
	wait_datapack_content=false;
	haveData=false;
	dataClear();
	resetAll();
}

/*void Api_client_real::errorOutput(QString error,QString detailedError)
{
	error_string=error;
	emit haveNewError();
	DebugClass::debugConsole("User message: "+error);
	socket.disconnectFromHost();
	if(!detailedError.isEmpty())
		DebugClass::debugConsole(detailedError);
}*/

void Api_client_real::tryDisconnect()
{
	socket.disconnectFromHost();
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
	quint8 datapack_content_query_number=queryNumber();
	datapackFilesList=listDatapack("");
	QByteArray outputData;
	QDataStream out(&outputData, QIODevice::WriteOnly);
	out.setVersion(QDataStream::Qt_4_4);
	out << datapack_content_query_number;
	out << (quint32)datapackFilesList.size();
	int index=0;
	while(index<datapackFilesList.size())
	{
		out << datapackFilesList.at(index);
		struct stat info;
		stat(QString(datapack_base_name+datapackFilesList.at(index)).toLatin1().data(),&info);
		out << (quint32)info.st_mtime;
		index++;
	}
	output->packOutcommingQuery(0x02,0x000C,datapack_content_query_number,qCompress(outputData,9));
}

const QStringList Api_client_real::listDatapack(QString suffix)
{
	QStringList returnFile;
	QDir finalDatapackFolder(datapack_base_name+suffix);
	QString fileName;
	QFileInfoList entryList=finalDatapackFolder.entryInfoList(QDir::AllEntries|QDir::NoDotAndDotDot|QDir::Hidden|QDir::System,QDir::DirsFirst);//possible wait time here
	int sizeEntryList=entryList.size();
	for (int index=0;index<sizeEntryList;++index)
	{
		QFileInfo fileInfo=entryList.at(index);
		if(fileInfo.isDir())
			returnFile << listDatapack(suffix+fileInfo.fileName()+"/");//put unix separator because it's transformed into that's under windows too
		else
			returnFile << suffix+fileInfo.fileName();
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

QString Api_client_real::get_datapack_base_name()
{
	return datapack_base_name;
}
