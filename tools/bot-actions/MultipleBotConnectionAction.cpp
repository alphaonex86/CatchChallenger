#include "MultipleBotConnectionAction.h"

#include <iostream>
#include <QFile>
#include <QMessageBox>

MultipleBotConnectionAction::MultipleBotConnectionAction()
{
    {
        QFile inputFile("pseudo-not-used.txt");
        if(inputFile.open(QIODevice::ReadOnly))
        {
            QTextStream in(&inputFile);
            while(!in.atEnd())
            {
                QString line = in.readLine();
                if(!line.isEmpty())
                    pseudoNotUsed.push_back(line.toStdString());
            }
            inputFile.close();
        }
    }
    {
        QFile inputFile("pseudo-used.txt");
        if(inputFile.open(QIODevice::ReadOnly))
        {
            QTextStream in(&inputFile);
            while(!in.atEnd())
            {
                QString line = in.readLine();
                if(!line.isEmpty())
                    pseudoUsed.insert(line.toStdString());
            }
            inputFile.close();
        }
    }

}

MultipleBotConnectionAction::~MultipleBotConnectionAction()
{
}

std::string MultipleBotConnectionAction::getNewPseudo()
{
    std::string choosePseudo;
    do
    {
        if(pseudoNotUsed.empty())
        {
            QMessageBox::critical(NULL,tr("Error"),tr("no file pseudo-not-used.txt, empty or too few entries"));
            abort();
            //return std::string("bot")+CatchChallenger::FacilityLibGeneral::randomPassword("abcdefghijklmnopqurstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890",CommonSettingsCommon::commonSettingsCommon.max_pseudo_size-3);
        }
        unsigned int index=rand()%pseudoNotUsed.size();
        choosePseudo=pseudoNotUsed.at(index);
        pseudoNotUsed.erase(pseudoNotUsed.cbegin()+index);
        pseudoUsed.insert(choosePseudo);

        {
            {
                QFile inputFile("pseudo-not-used.txt");
                if(inputFile.open(QIODevice::WriteOnly))
                {
                    inputFile.resize(0);
                    unsigned int index=0;
                    while(index<pseudoNotUsed.size())
                    {
                        inputFile.write((QString::fromStdString(pseudoNotUsed.at(index))+"\n").toUtf8());
                        index++;
                    }
                    inputFile.close();
                }
            }
            {
                QFile inputFile("pseudo-used.txt");
                if(inputFile.open(QIODevice::WriteOnly))
                {
                    inputFile.resize(0);
                    for(std::string pseudo : pseudoUsed) {
                        inputFile.write((QString::fromStdString(pseudo)+"\n").toUtf8());
                    }
                    inputFile.close();
                }
            }
        }
    }
    while(choosePseudo.find(" ")!=std::string::npos);
    return choosePseudo;
}
