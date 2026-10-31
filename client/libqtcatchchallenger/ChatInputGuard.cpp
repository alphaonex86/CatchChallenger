#include "ChatInputGuard.hpp"

#include <QRegularExpression>
#include <iostream>

using namespace CatchChallenger;

ChatInputGuard::ChatInputGuard() :
    floodCount(0)
{
    if(!connect(&floodTimer,&QTimer::timeout,this,&ChatInputGuard::decayFlood,Qt::QueuedConnection))
        std::cerr << "ChatInputGuard: unable to connect the flood timer, flood protection never decays" << std::endl;
    floodTimer.setSingleShot(false);
    floodTimer.start(1500);
}

ChatInputGuard::Verdict ChatInputGuard::check(QString &text)
{
    text.remove("\n");
    text.remove("\r");
    text.remove("\t");
    if(text.isEmpty())
        return Verdict_Nothing;
    if(text.contains(QRegularExpression("^ +$")))
    {
        refusalText="Space text not allowed";
        return Verdict_Refused;
    }
    if(text.size()>256)
    {
        refusalText="Message too long";
        return Verdict_Refused;
    }
    if(!text.startsWith('/'))
    {
        if(text.toStdString()==lastMessageSent)
        {
            refusalText="Send message like as previous";
            return Verdict_Refused;
        }
        if(floodCount>2)
        {
            refusalText="Stop flood";
            return Verdict_Refused;
        }
    }
    floodCount++;
    lastMessageSent=text.toStdString();
    return Verdict_Send;
}

const std::string &ChatInputGuard::refusal() const
{
    return refusalText;
}

void ChatInputGuard::reset()
{
    floodCount=0;
    lastMessageSent.clear();
}

void ChatInputGuard::decayFlood()
{
    if(floodCount>0)
        floodCount--;
}
