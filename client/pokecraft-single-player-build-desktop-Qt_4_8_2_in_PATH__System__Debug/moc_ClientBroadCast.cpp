/****************************************************************************
** Meta object code from reading C++ file 'ClientBroadCast.h'
**
** Created: Fri Oct 5 13:40:09 2012
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../server/base/ClientBroadCast.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'ClientBroadCast.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.3. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_Pokecraft__ClientBroadCast[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
      23,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       8,       // signalCount

 // signals: signature, parameters, type, tag, flags
      34,   28,   27,   27, 0x05,
      49,   27,   27,   27, 0x05,
      66,   58,   27,   27, 0x05,
      83,   27,   27,   27, 0x05,
     123,   99,   27,   27, 0x05,
     180,  161,   27,   27, 0x25,
     222,  207,   27,   27, 0x05,
     262,  252,   27,   27, 0x25,

 // slots: signature, parameters, type, tag, flags
     293,  281,   27,   27, 0x0a,
     320,  281,   27,   27, 0x0a,
     391,  344,   27,   27, 0x0a,
     461,  446,   27,   27, 0x0a,
     498,  493,   27,   27, 0x2a,
     539,  525,   27,   27, 0x0a,
     589,  571,   27,   27, 0x0a,
     627,   27,   27,   27, 0x0a,
     652,  634,   27,   27, 0x0a,
     695,  690,   27,   27, 0x0a,
     718,   27,   27,   27, 0x0a,
     745,   27,   27,   27, 0x0a,
     766,   27,   27,   27, 0x0a,
     773,  446,   27,   27, 0x0a,
     805,  493,   27,   27, 0x2a,

       0        // eod
};

static const char qt_meta_stringdata_Pokecraft__ClientBroadCast[] = {
    "Pokecraft::ClientBroadCast\0\0error\0"
    "error(QString)\0kicked()\0message\0"
    "message(QString)\0isReadyToStop()\0"
    "mainIdent,subIdent,data\0"
    "sendPacket(quint8,quint16,QByteArray)\0"
    "mainIdent,subIdent\0sendPacket(quint8,quint16)\0"
    "mainIdent,data\0sendPacket(quint8,QByteArray)\0"
    "mainIdent\0sendPacket(quint8)\0text,pseudo\0"
    "receivePM(QString,QString)\0"
    "sendPM(QString,QString)\0"
    "chatType,text,sender_pseudo,sender_player_type\0"
    "receiveChatText(Chat_type,QString,QString,Player_type)\0"
    "text,important\0receiveSystemText(QString,bool)\0"
    "text\0receiveSystemText(QString)\0"
    "chatType,text\0sendChatText(Chat_type,QString)\0"
    "connected_players\0"
    "receive_instant_player_number(qint32)\0"
    "kick()\0command,extraText\0"
    "sendBroadCastCommand(QString,QString)\0"
    "type\0setRights(Player_type)\0"
    "send_player_informations()\0"
    "askIfIsReadyToStop()\0stop()\0"
    "sendSystemMessage(QString,bool)\0"
    "sendSystemMessage(QString)\0"
};

void Pokecraft::ClientBroadCast::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        ClientBroadCast *_t = static_cast<ClientBroadCast *>(_o);
        switch (_id) {
        case 0: _t->error((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 1: _t->kicked(); break;
        case 2: _t->message((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 3: _t->isReadyToStop(); break;
        case 4: _t->sendPacket((*reinterpret_cast< const quint8(*)>(_a[1])),(*reinterpret_cast< const quint16(*)>(_a[2])),(*reinterpret_cast< const QByteArray(*)>(_a[3]))); break;
        case 5: _t->sendPacket((*reinterpret_cast< const quint8(*)>(_a[1])),(*reinterpret_cast< const quint16(*)>(_a[2]))); break;
        case 6: _t->sendPacket((*reinterpret_cast< const quint8(*)>(_a[1])),(*reinterpret_cast< const QByteArray(*)>(_a[2]))); break;
        case 7: _t->sendPacket((*reinterpret_cast< const quint8(*)>(_a[1]))); break;
        case 8: _t->receivePM((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2]))); break;
        case 9: _t->sendPM((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2]))); break;
        case 10: _t->receiveChatText((*reinterpret_cast< const Chat_type(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2])),(*reinterpret_cast< const QString(*)>(_a[3])),(*reinterpret_cast< const Player_type(*)>(_a[4]))); break;
        case 11: _t->receiveSystemText((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< const bool(*)>(_a[2]))); break;
        case 12: _t->receiveSystemText((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 13: _t->sendChatText((*reinterpret_cast< const Chat_type(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2]))); break;
        case 14: _t->receive_instant_player_number((*reinterpret_cast< qint32(*)>(_a[1]))); break;
        case 15: _t->kick(); break;
        case 16: _t->sendBroadCastCommand((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2]))); break;
        case 17: _t->setRights((*reinterpret_cast< const Player_type(*)>(_a[1]))); break;
        case 18: _t->send_player_informations(); break;
        case 19: _t->askIfIsReadyToStop(); break;
        case 20: _t->stop(); break;
        case 21: _t->sendSystemMessage((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< const bool(*)>(_a[2]))); break;
        case 22: _t->sendSystemMessage((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData Pokecraft::ClientBroadCast::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject Pokecraft::ClientBroadCast::staticMetaObject = {
    { &QObject::staticMetaObject, qt_meta_stringdata_Pokecraft__ClientBroadCast,
      qt_meta_data_Pokecraft__ClientBroadCast, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &Pokecraft::ClientBroadCast::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *Pokecraft::ClientBroadCast::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *Pokecraft::ClientBroadCast::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_Pokecraft__ClientBroadCast))
        return static_cast<void*>(const_cast< ClientBroadCast*>(this));
    return QObject::qt_metacast(_clname);
}

int Pokecraft::ClientBroadCast::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 23)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 23;
    }
    return _id;
}

// SIGNAL 0
void Pokecraft::ClientBroadCast::error(const QString & _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void Pokecraft::ClientBroadCast::kicked()
{
    QMetaObject::activate(this, &staticMetaObject, 1, 0);
}

// SIGNAL 2
void Pokecraft::ClientBroadCast::message(const QString & _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void Pokecraft::ClientBroadCast::isReadyToStop()
{
    QMetaObject::activate(this, &staticMetaObject, 3, 0);
}

// SIGNAL 4
void Pokecraft::ClientBroadCast::sendPacket(const quint8 & _t1, const quint16 & _t2, const QByteArray & _t3)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)), const_cast<void*>(reinterpret_cast<const void*>(&_t3)) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}

// SIGNAL 6
void Pokecraft::ClientBroadCast::sendPacket(const quint8 & _t1, const QByteArray & _t2)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 6, _a);
}
QT_END_MOC_NAMESPACE
