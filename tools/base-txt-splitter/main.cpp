#include <QCoreApplication>
#include <QFile>
#include <iostream>

std::vector<std::string> stringsplit(const std::string &s, char delim)
{
    std::vector<std::string> elems;

    std::string::size_type i = 0;
    std::string::size_type j = s.find(delim);

    if(j == std::string::npos)
    {
        if(!s.empty())
            elems.push_back(s);
        return elems;
    }
    else
    {
        while (j != std::string::npos) {
           elems.push_back(s.substr(i, j-i));
           i = ++j;
           j = s.find(delim, j);

           if (j == std::string::npos)
              elems.push_back(s.substr(i, s.length()));
        }
        return elems;
    }
}

int main(int argc, char *argv[])
{
    QFile file("base.txt");
    if(!file.open(QFile::ReadOnly))
        return -1;

    std::vector<char> data;
    QByteArray olddata=file.readAll();
    data.resize(olddata.size());
    memcpy(data.data(),olddata.constData(),olddata.size());

    size_t endOfText;
    {
        std::string text(data.data(),data.size());
        endOfText=text.find("\n-\n");
    }
    if(endOfText==std::string::npos)
    {
        std::cerr << "no text delimitor into file list: " << std::endl;
        return -1;
    }

    QFile head("head.txt");
    head.remove();
    if(!head.open(QFile::WriteOnly))
        return -1;
    head.write(data.data(),endOfText);

    QFile body("body.txt");
    body.remove();
    if(!body.open(QFile::WriteOnly))
        return -1;
    body.write(data.data()+endOfText+3,data.size()-(endOfText+3));

    std::vector<std::string> content;
    std::vector<char> partialHashListRaw(data.cbegin()+endOfText+3,data.cend());
    {
        if(partialHashListRaw.size()%4!=0)
        {
            std::cerr << "partialHashList not divisible by 4 (base), check: " << std::endl;
            return -1;
        }
        {
            std::string text(data.data(),endOfText);
            content=stringsplit(text,'\n');
        }
        if(partialHashListRaw.size()/4!=content.size())
        {
            std::cerr << "partialHashList/4!=content.size()" << std::endl;
            return -1;
        }
    }
}
