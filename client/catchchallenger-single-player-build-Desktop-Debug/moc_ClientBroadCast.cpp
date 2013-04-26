/****************************************************************************
** Meta object code from reading C++ file 'ClientBroadCast.h'
**
** Created: Fri Apr 26 14:18:59 2013
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.4)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../server/base/ClientBroadCast.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'ClientBroadCast.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.4. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_CatchChallenger__ClientBroadCast[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
      21,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       8,       // signalCount

 // signals: signature, parameters, type, tag, flags
      40,   34,   33,   33, 0x05,
      55,   33,   33,   33, 0x05,
      72,   64,   33,   33, 0x05,
      89,   33,   33,   33, 0x05,
     129,  105,   33,   33, 0x05,
     186,  167,   33,   33, 0x25,
     228,  213,   33,   33, 0x05,
     268,  258,   33,   33, 0x25,

 // slots: signature, parameters, type, tag, flags
     299,  287,   33,   33, 0x0a,
     357,  323,   33,   33, 0x0a,
     443,  428,   33,   33, 0x0a,
     480,  475,   33,   33, 0x2a,
     521,  507,   33,   33, 0x0a,
     571,  553,   33,   33, 0x0a,
     609,   33,   33,   33, 0x0a,
     634,  616,   33,   33, 0x0a,
     677,  672,   33,   33, 0x0a,
     700,   33,   33,   33, 0x0a,
     727,   33,   33,   33, 0x0a,
     748,  428,   33,   33, 0x0a,
     780,  475,   33,   33, 0x2a,

       0        // eod
};

static const char qt_meta_stringdata_CatchChallenger__ClientBroadCast[] = {
    "CatchChallenger::ClientBroadCast\0\0"
    "error\0error(QString)\0kicked()\0message\0"
    "message(QString)\0isReadyToStop()\0"
    "mainIdent,subIdent,data\0"
    "sendPacket(quint8,quint16,QByteArray)\0"
    "mainIdent,subIdent\0sendPacket(quint8,quint16)\0"
    "mainIdent,data\0sendPacket(quint8,QByteArray)\0"
    "mainIdent\0sendPacket(quint8)\0text,pseudo\0"
    "sendPM(QString,QString)\0"
    "chatType,text,sender_informations\0"
    "receiveChatText(Chat_type,QString,const Player_internal_informations*)\0"
    "text,important\0receiveSystemText(QString,bool)\0"
    "text\0receiveSystemText(QString)\0"
    "chatType,text\0sendChatText(Chat_type,QString)\0"
    "connected_players\0"
    "receive_instant_player_number(qint32)\0"
    "kick()\0command,extraText\0"
    "sendBroadCastCommand(QString,QString)\0"
    "type\0setRights(Player_type)\0"
    "send_player_informations()\0"
    "askIfIsReadyToStop()\0"
    "sendSystemMessage(QString,bool)\0"
    "sendSystemMessage(QString)\0"
};

void CatchChallenger::ClientBroadCast::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
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
        case 8: _t->sendPM((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2]))); break;
        case 9: _t->receiveChatText((*reinterpret_cast< const Chat_type(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2])),(*reinterpret_cast< const Player_internal_informations*(*)>(_a[3]))); break;
        case 10: _t->receiveSystemText((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< const bool(*)>(_a[2]))); break;
        case 11: _t->receiveSystemText((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 12: _t->sendChatText((*reinterpret_cast< const Chat_type(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2]))); break;
        case 13: _t->receive_instant_player_number((*reinterpret_cast< qint32(*)>(_a[1]))); break;
        case 14: _t->kick(); break;
        case 15: _t->sendBroadCastCommand((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2]))); break;
        case 16: _t->setRights((*reinterpret_cast< const Player_type(*)>(_a[1]))); break;
        case 17: _t->send_player_informations(); break;
        case 18: _t->askIfIsReadyToStop(); break;
        case 19: _t->sendSystemMessage((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< const bool(*)>(_a[2]))); break;
        case 20: _t->sendSystemMessage((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData CatchChallenger::ClientBroadCast::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject CatchChallenger::ClientBroadCast::staticMetaObject = {
    { &QObject::staticMetaObject, qt_meta_stringdata_CatchChallenger__ClientBroadCast,
      qt_meta_data_CatchChallenger__ClientBroadCast, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &CatchChallenger::ClientBroadCast::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *CatchChallenger::ClientBroadCast::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *CatchChallenger::ClientBroadCast::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_CatchChallenger__ClientBroadCast))
        return static_cast<void*>(const_cast< ClientBroadCast*>(this));
    return QObject::qt_metacast(_clname);
}

int CatchChallenger::ClientBroadCast::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 21)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 21;
    }
    return _id;
}

// SIGNAL 0
void CatchChallenger::ClientBroadCast::error(const QString & _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void CatchChallenger::ClientBroadCast::kicked()
{
    QMetaObject::activate(this, &staticMetaObject, 1, 0);
}

// SIGNAL 2
void CatchChallenger::ClientBroadCast::message(const QString & _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void CatchChallenger::ClientBroadCast::isReadyToStop()
{
    QMetaObject::activate(this, &staticMetaObject, 3, 0);
}

// SIGNAL 4
void CatchChallenger::ClientBroadCast::sendPacket(const quint8 & _t1, const quint16 & _t2, const QByteArray & _t3)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)), const_cast<void*>(reinterpret_cast<const void*>(&_t3)) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}

// SIGNAL 6
void CatchChallenger::ClientBroadCast::sendPacket(const quint8 & _t1, const QByteArray & _t2)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 6, _a);
}
QT_END_MOC_NAMESPACE
