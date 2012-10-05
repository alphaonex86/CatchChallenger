/****************************************************************************
** Meta object code from reading C++ file 'Client.h'
**
** Created: Fri Oct 5 13:40:09 2012
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../server/base/Client.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'Client.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.3. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_Pokecraft__Client[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
      21,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
      10,       // signalCount

 // signals: signature, parameters, type, tag, flags
      19,   18,   18,   18, 0x05,
      40,   18,   18,   18, 0x05,
      65,   58,   18,   18, 0x05,
     126,  119,   18,   18, 0x05,
     175,  158,   18,   18, 0x05,
     264,  219,   18,   18, 0x05,
     362,  357,   18,   18, 0x05,
     389,  357,   18,   18, 0x05,
     425,   18,   18,   18, 0x05,
     442,  440,   18,   18, 0x05,

 // slots: signature, parameters, type, tag, flags
     478,   18,   18,   18, 0x08,
     536,  524,   18,   18, 0x08,
     557,   18,   18,   18, 0x08,
     574,  566,   18,   18, 0x08,
     596,   18,   18,   18, 0x08,
     623,   18,   18,   18, 0x08,
     662,  644,   18,   18, 0x08,
     705,  693,   18,   18, 0x08,
     749,  735,   18,   18, 0x08,
     787,   18,   18,   18, 0x0a,
     806,  357,   18,   18, 0x0a,

       0        // eod
};

static const char qt_meta_stringdata_Pokecraft__Client[] = {
    "Pokecraft::Client\0\0askIfIsReadyToStop()\0"
    "isReadyToDelete()\0player\0"
    "new_player_is_connected(Player_internal_informations)\0"
    "pseudo\0player_is_disconnected(QString)\0"
    "pseudo,type,text\0"
    "new_chat_message(QString,Chat_type,QString)\0"
    "last_fake_player_id,x,y,map,orientation,skin\0"
    "send_fakeLogin(quint32,quint16,quint16,Map_server_MapVisibility_simple"
    "*,Orientation,QString)\0"
    "data\0fake_send_data(QByteArray)\0"
    "fake_send_received_data(QByteArray)\0"
    "try_ask_stop()\0,\0emit_serverCommand(QString,QString)\0"
    "connectionError(QAbstractSocket::SocketError)\0"
    "errorString\0errorOutput(QString)\0"
    "kicked()\0message\0normalOutput(QString)\0"
    "send_player_informations()\0"
    "disconnectNextStep()\0command,extraText\0"
    "serverCommand(QString,QString)\0"
    "text,pseudo\0local_sendPM(QString,QString)\0"
    "chatType,text\0local_sendChatText(Chat_type,QString)\0"
    "disconnectClient()\0fake_receive_data(QByteArray)\0"
};

void Pokecraft::Client::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        Client *_t = static_cast<Client *>(_o);
        switch (_id) {
        case 0: _t->askIfIsReadyToStop(); break;
        case 1: _t->isReadyToDelete(); break;
        case 2: _t->new_player_is_connected((*reinterpret_cast< const Player_internal_informations(*)>(_a[1]))); break;
        case 3: _t->player_is_disconnected((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 4: _t->new_chat_message((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< const Chat_type(*)>(_a[2])),(*reinterpret_cast< const QString(*)>(_a[3]))); break;
        case 5: _t->send_fakeLogin((*reinterpret_cast< quint32(*)>(_a[1])),(*reinterpret_cast< quint16(*)>(_a[2])),(*reinterpret_cast< quint16(*)>(_a[3])),(*reinterpret_cast< Map_server_MapVisibility_simple*(*)>(_a[4])),(*reinterpret_cast< Orientation(*)>(_a[5])),(*reinterpret_cast< QString(*)>(_a[6]))); break;
        case 6: _t->fake_send_data((*reinterpret_cast< const QByteArray(*)>(_a[1]))); break;
        case 7: _t->fake_send_received_data((*reinterpret_cast< const QByteArray(*)>(_a[1]))); break;
        case 8: _t->try_ask_stop(); break;
        case 9: _t->emit_serverCommand((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2]))); break;
        case 10: _t->connectionError((*reinterpret_cast< QAbstractSocket::SocketError(*)>(_a[1]))); break;
        case 11: _t->errorOutput((*reinterpret_cast< QString(*)>(_a[1]))); break;
        case 12: _t->kicked(); break;
        case 13: _t->normalOutput((*reinterpret_cast< QString(*)>(_a[1]))); break;
        case 14: _t->send_player_informations(); break;
        case 15: _t->disconnectNextStep(); break;
        case 16: _t->serverCommand((*reinterpret_cast< QString(*)>(_a[1])),(*reinterpret_cast< QString(*)>(_a[2]))); break;
        case 17: _t->local_sendPM((*reinterpret_cast< QString(*)>(_a[1])),(*reinterpret_cast< QString(*)>(_a[2]))); break;
        case 18: _t->local_sendChatText((*reinterpret_cast< Chat_type(*)>(_a[1])),(*reinterpret_cast< QString(*)>(_a[2]))); break;
        case 19: _t->disconnectClient(); break;
        case 20: _t->fake_receive_data((*reinterpret_cast< QByteArray(*)>(_a[1]))); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData Pokecraft::Client::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject Pokecraft::Client::staticMetaObject = {
    { &QObject::staticMetaObject, qt_meta_stringdata_Pokecraft__Client,
      qt_meta_data_Pokecraft__Client, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &Pokecraft::Client::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *Pokecraft::Client::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *Pokecraft::Client::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_Pokecraft__Client))
        return static_cast<void*>(const_cast< Client*>(this));
    return QObject::qt_metacast(_clname);
}

int Pokecraft::Client::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
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
void Pokecraft::Client::askIfIsReadyToStop()
{
    QMetaObject::activate(this, &staticMetaObject, 0, 0);
}

// SIGNAL 1
void Pokecraft::Client::isReadyToDelete()
{
    QMetaObject::activate(this, &staticMetaObject, 1, 0);
}

// SIGNAL 2
void Pokecraft::Client::new_player_is_connected(const Player_internal_informations & _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void Pokecraft::Client::player_is_disconnected(const QString & _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}

// SIGNAL 4
void Pokecraft::Client::new_chat_message(const QString & _t1, const Chat_type & _t2, const QString & _t3)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)), const_cast<void*>(reinterpret_cast<const void*>(&_t3)) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}

// SIGNAL 5
void Pokecraft::Client::send_fakeLogin(quint32 _t1, quint16 _t2, quint16 _t3, Map_server_MapVisibility_simple * _t4, Orientation _t5, QString _t6)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)), const_cast<void*>(reinterpret_cast<const void*>(&_t3)), const_cast<void*>(reinterpret_cast<const void*>(&_t4)), const_cast<void*>(reinterpret_cast<const void*>(&_t5)), const_cast<void*>(reinterpret_cast<const void*>(&_t6)) };
    QMetaObject::activate(this, &staticMetaObject, 5, _a);
}

// SIGNAL 6
void Pokecraft::Client::fake_send_data(const QByteArray & _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 6, _a);
}

// SIGNAL 7
void Pokecraft::Client::fake_send_received_data(const QByteArray & _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 7, _a);
}

// SIGNAL 8
void Pokecraft::Client::try_ask_stop()
{
    QMetaObject::activate(this, &staticMetaObject, 8, 0);
}

// SIGNAL 9
void Pokecraft::Client::emit_serverCommand(const QString & _t1, const QString & _t2)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 9, _a);
}
QT_END_MOC_NAMESPACE
