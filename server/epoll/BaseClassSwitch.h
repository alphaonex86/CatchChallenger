#ifndef BASECLASSSWITCH_H
#define BASECLASSSWITCH_H

class BaseClassSwitch
{
public:
    virtual ~BaseClassSwitch() {}
    enum Type
    {
        Server=0x00,
        UnixServer=0x01,
        Client=0x02,
        UnixClient=0x03,
        Timer=0x04,
        Database=0x05
    };
    virtual Type getType() const = 0;
};

#endif // BASECLASSSWITCH_H
