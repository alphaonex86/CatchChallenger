/****************************************************************************
** Meta object code from reading C++ file 'ClientNetworkRead.h'
**
** Created: Fri Oct 5 13:40:09 2012
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../server/base/ClientNetworkRead.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'ClientNetworkRead.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.3. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_Pokecraft__ClientNetworkRead[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
      16,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
      13,       // signalCount

 // signals: signature, parameters, type, tag, flags
      60,   30,   29,   29, 0x05,
     123,   98,   29,   29, 0x25,
     168,  150,   29,   29, 0x05,
     211,  198,   29,   29, 0x25,
     247,  230,   29,   29, 0x05,
     276,   29,   29,   29, 0x05,
     312,  292,   29,   29, 0x05,
     374,  348,   29,   29, 0x05,
     450,  422,   29,   29, 0x05,
     494,  482,   29,   29, 0x05,
     532,  518,   29,   29, 0x05,
     582,  564,   29,   29, 0x05,
     620,  564,   29,   29, 0x05,

 // slots: signature, parameters, type, tag, flags
     656,  651,   29,   29, 0x0a,
     686,   29,   29,   29, 0x0a,
     707,   29,   29,   29, 0x0a,

       0        // eod
};

static const char qt_meta_stringdata_Pokecraft__ClientNetworkRead[] = {
    "Pokecraft::ClientNetworkRead\0\0"
    "mainCodeType,subCodeType,data\0"
    "sendPacket(quint8,quint16,QByteArray)\0"
    "mainCodeType,subCodeType\0"
    "sendPacket(quint8,quint16)\0mainCodeType,data\0"
    "sendPacket(quint8,QByteArray)\0"
    "mainCodeType\0sendPacket(quint8)\0"
    "queryNumber,data\0postReply(quint8,QByteArray)\0"
    "isReadyToStop()\0query_id,login,hash\0"
    "askLogin(quint8,QString,QByteArray)\0"
    "query_id,files,timestamps\0"
    "datapackList(quint8,QStringList,QList<quint32>)\0"
    "previousMovedUnit,direction\0"
    "moveThePlayer(quint8,Direction)\0"
    "text,pseudo\0sendPM(QString,QString)\0"
    "chatType,text\0sendChatText(Chat_type,QString)\0"
    "command,extraText\0"
    "sendBroadCastCommand(QString,QString)\0"
    "serverCommand(QString,QString)\0data\0"
    "fake_receive_data(QByteArray)\0"
    "askIfIsReadyToStop()\0stop()\0"
};

void Pokecraft::ClientNetworkRead::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        ClientNetworkRead *_t = static_cast<ClientNetworkRead *>(_o);
        switch (_id) {
        case 0: _t->sendPacket((*reinterpret_cast< const quint8(*)>(_a[1])),(*reinterpret_cast< const quint16(*)>(_a[2])),(*reinterpret_cast< const QByteArray(*)>(_a[3]))); break;
        case 1: _t->sendPacket((*reinterpret_cast< const quint8(*)>(_a[1])),(*reinterpret_cast< const quint16(*)>(_a[2]))); break;
        case 2: _t->sendPacket((*reinterpret_cast< const quint8(*)>(_a[1])),(*reinterpret_cast< const QByteArray(*)>(_a[2]))); break;
        case 3: _t->sendPacket((*reinterpret_cast< const quint8(*)>(_a[1]))); break;
        case 4: _t->postReply((*reinterpret_cast< const quint8(*)>(_a[1])),(*reinterpret_cast< const QByteArray(*)>(_a[2]))); break;
        case 5: _t->isReadyToStop(); break;
        case 6: _t->askLogin((*reinterpret_cast< const quint8(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2])),(*reinterpret_cast< const QByteArray(*)>(_a[3]))); break;
        case 7: _t->datapackList((*reinterpret_cast< const quint8(*)>(_a[1])),(*reinterpret_cast< const QStringList(*)>(_a[2])),(*reinterpret_cast< const QList<quint32>(*)>(_a[3]))); break;
        case 8: _t->moveThePlayer((*reinterpret_cast< const quint8(*)>(_a[1])),(*reinterpret_cast< const Direction(*)>(_a[2]))); break;
        case 9: _t->sendPM((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2]))); break;
        case 10: _t->sendChatText((*reinterpret_cast< const Chat_type(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2]))); break;
        case 11: _t->sendBroadCastCommand((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2]))); break;
        case 12: _t->serverCommand((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2]))); break;
        case 13: _t->fake_receive_data((*reinterpret_cast< const QByteArray(*)>(_a[1]))); break;
        case 14: _t->askIfIsReadyToStop(); break;
        case 15: _t->stop(); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData Pokecraft::ClientNetworkRead::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject Pokecraft::ClientNetworkRead::staticMetaObject = {
    { &ProtocolParsingInput::staticMetaObject, qt_meta_stringdata_Pokecraft__ClientNetworkRead,
      qt_meta_data_Pokecraft__ClientNetworkRead, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &Pokecraft::ClientNetworkRead::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *Pokecraft::ClientNetworkRead::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *Pokecraft::ClientNetworkRead::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_Pokecraft__ClientNetworkRead))
        return static_cast<void*>(const_cast< ClientNetworkRead*>(this));
    return ProtocolParsingInput::qt_metacast(_clname);
}

int Pokecraft::ClientNetworkRead::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = ProtocolParsingInput::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 16)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 16;
    }
    return _id;
}

// SIGNAL 0
void Pokecraft::ClientNetworkRead::sendPacket(const quint8 & _t1, const quint16 & _t2, const QByteArray & _t3)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)), const_cast<void*>(reinterpret_cast<const void*>(&_t3)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 2
void Pokecraft::ClientNetworkRead::sendPacket(const quint8 & _t1, const QByteArray & _t2)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 4
void Pokecraft::ClientNetworkRead::postReply(const quint8 & _t1, const QByteArray & _t2)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}

// SIGNAL 5
void Pokecraft::ClientNetworkRead::isReadyToStop()
{
    QMetaObject::activate(this, &staticMetaObject, 5, 0);
}

// SIGNAL 6
void Pokecraft::ClientNetworkRead::askLogin(const quint8 & _t1, const QString & _t2, const QByteArray & _t3)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)), const_cast<void*>(reinterpret_cast<const void*>(&_t3)) };
    QMetaObject::activate(this, &staticMetaObject, 6, _a);
}

// SIGNAL 7
void Pokecraft::ClientNetworkRead::datapackList(const quint8 & _t1, const QStringList & _t2, const QList<quint32> & _t3)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)), const_cast<void*>(reinterpret_cast<const void*>(&_t3)) };
    QMetaObject::activate(this, &staticMetaObject, 7, _a);
}

// SIGNAL 8
void Pokecraft::ClientNetworkRead::moveThePlayer(const quint8 & _t1, const Direction & _t2)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 8, _a);
}

// SIGNAL 9
void Pokecraft::ClientNetworkRead::sendPM(const QString & _t1, const QString & _t2)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 9, _a);
}

// SIGNAL 10
void Pokecraft::ClientNetworkRead::sendChatText(const Chat_type & _t1, const QString & _t2)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 10, _a);
}

// SIGNAL 11
void Pokecraft::ClientNetworkRead::sendBroadCastCommand(const QString & _t1, const QString & _t2)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 11, _a);
}

// SIGNAL 12
void Pokecraft::ClientNetworkRead::serverCommand(const QString & _t1, const QString & _t2)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 12, _a);
}
QT_END_MOC_NAMESPACE
