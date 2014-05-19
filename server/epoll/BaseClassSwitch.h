#ifndef BASECLASSSWITCH_H
#define BASECLASSSWITCH_H

class BaseClassSwitch
{
public:
    enum Type
    {
        Server,
        Client,
        Timer
    };
    virtual Type getType() = 0;
};

#endif // BASECLASSSWITCH_H
