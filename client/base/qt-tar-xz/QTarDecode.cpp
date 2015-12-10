/** \file QTarDecode.cpp
\brief To read a tar data block
\author alpha_one_x86
\version 0.3
\date 2010
\licence GPL3, see the file COPYING */

#include "QTarDecode.h"

#include <cstring>

QTarDecode::QTarDecode()
{
    error="Unknow error";
}

std::string QTarDecode::errorString()
{
    return error;
}

void QTarDecode::setErrorString(const std::string &error)
{
    this->error=error;
    fileList.clear();
    dataList.clear();
}

bool QTarDecode::stringStartWith(std::string const &fullString, std::string const &starting)
{
    if (fullString.length() >= starting.length()) {
        return (fullString.substr(0,starting.length())==starting);
    } else {
        return false;
    }
}

bool QTarDecode::decodeData(const std::vector<char> &data)
{
    setErrorString("Unknow error");
    if(data.size()<1024)
        return false;
    unsigned int offset=0;
    while(offset<data.size())
    {
        //load the file name from ascii, from 0+offset with size of 100
        std::string fileName(data.data()+offset,100);
        //load the file type
        std::string fileType(data.data()+156+offset,1);
        //load the ustar string
        std::string ustar(data.data()+257+offset,5);
        //load the ustar version
        std::string ustarVersion(data.data()+263+offset,2);
        bool ok=false;
        //load the file size from ascii, from 124+offset with size of 12
        int finalSize=atoi(std::string(data.data()+124+offset,12).c_str());
        //if the size conversion to qulonglong have failed, then qui with error
        if(ok==false)
        {
            //if((offset+1024)==data.size())
            //it's the end of the archive, no more verification
            break;
            /*else
            {
                setErrorString("Unable to convert ascii size at "+std::to_string(124+offset)+" to usigned long long: \""+std::string(data.mid(124+offset,12))+"\"");
                return false;
            }*/
        }
        //if check if the ustar not found
        if(ustar!="ustar")
        {
            setErrorString("\"ustar\" string not found at "+std::to_string(257+offset)+", content: \""+ustar+"\"");
            return false;
        }
        if(ustarVersion!="00")
        {
            setErrorString("ustar version string is wrong, content:\""+ustarVersion+"\"");
            return false;
        }
        //check if the file have the good size for load the data
        if((offset+512+finalSize)>data.size())
        {
            setErrorString("The tar file seem be too short");
            return false;
        }
        if(fileType=="0") //this code manage only the file, then only the file is returned
        {
            std::vector<char> newdata;
            newdata.resize(finalSize);
            memcpy(newdata.data(),data.data()+512+offset,finalSize);
            fileList.push_back(fileName);
            dataList.push_back(newdata);
        }
        //calculate the offset for the next header
        bool retenu=((finalSize%512)!=0);
        //go to the next header
        offset+=512+(finalSize/512+retenu)*512;
    }
    if(fileList.size()>0)
    {
        std::string baseDirToTest=fileList.at(0);
        std::size_t found = baseDirToTest.find_last_of("/");
        if(found!=std::string::npos && found>=baseDirToTest.size())
            baseDirToTest.resize(baseDirToTest.size()-(baseDirToTest.size()-found));
        bool isFoundForAllEntries=true;
        for (unsigned int i = 0; i < fileList.size(); ++i)
            if(!stringStartWith(fileList.at(i),baseDirToTest))
                isFoundForAllEntries=false;
        if(isFoundForAllEntries)
            for (unsigned int i = 0; i < fileList.size(); ++i)
                fileList[i].erase(0,baseDirToTest.size());
    }
    return true;
}

std::vector<std::string> QTarDecode::getFileList()
{
    return fileList;
}

std::vector<std::vector<char> > QTarDecode::getDataList()
{
    return dataList;
}


