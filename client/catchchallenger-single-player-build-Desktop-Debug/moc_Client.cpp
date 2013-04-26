/****************************************************************************
** Meta object code from reading C++ file 'Client.h'
**
** Created: Fri Apr 26 14:18:59 2013
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.4)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../server/base/Client.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'Client.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.4. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_CatchChallenger__Client[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
      14,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       6,       // signalCount

 // signals: signature, parameters, type, tag, flags
      25,   24,   24,   24, 0x05,
      46,   24,   24,   24, 0x05,
     109,   64,   24,   24, 0x05,
     207,  202,   24,   24, 0x05,
     234,  202,   24,   24, 0x05,
     270,   24,   24,   24, 0x05,

 // slots: signature, parameters, type, tag, flags
     285,   24,   24,   24, 0x08,
     343,  331,   24,   24, 0x08,
     364,   24,   24,   24, 0x08,
     381,  373,   24,   24, 0x08,
     403,   24,   24,   24, 0x08,
     430,   24,   24,   24, 0x08,
     451,   24,   24,   24, 0x0a,
     470,  202,   24,   24, 0x0a,

       0        // eod
};

static const char qt_meta_stringdata_CatchChallenger__Client[] = {
    "CatchChallenger::Client\0\0askIfIsReadyToStop()\0"
    "isReadyToDelete()\0"
    "last_fake_player_id,x,y,map,orientation,skin\0"
    "send_fakeLogin(quint32,quint16,quint16,Map_server_MapVisibility_simple"
    "*,Orientation,QString)\0"
    "data\0fake_send_data(QByteArray)\0"
    "fake_send_received_data(QByteArray)\0"
    "try_ask_stop()\0"
    "connectionError(QAbstractSocket::SocketError)\0"
    "errorString\0errorOutput(QString)\0"
    "kicked()\0message\0normalOutput(QString)\0"
    "send_player_informations()\0"
    "disconnectNextStep()\0disconnectClient()\0"
    "fake_receive_data(QByteArray)\0"
};

void CatchChallenger::Client::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        Client *_t = static_cast<Client *>(_o);
        switch (_id) {
        case 0: _t->askIfIsReadyToStop(); break;
        case 1: _t->isReadyToDelete(); break;
        case 2: _t->send_fakeLogin((*reinterpret_cast< quint32(*)>(_a[1])),(*reinterpret_cast< quint16(*)>(_a[2])),(*reinterpret_cast< quint16(*)>(_a[3])),(*reinterpret_cast< Map_server_MapVisibility_simple*(*)>(_a[4])),(*reinterpret_cast< Orientation(*)>(_a[5])),(*reinterpret_cast< QString(*)>(_a[6]))); break;
        case 3: _t->fake_send_data((*reinterpret_cast< const QByteArray(*)>(_a[1]))); break;
        case 4: _t->fake_send_received_data((*reinterpret_cast< const QByteArray(*)>(_a[1]))); break;
        case 5: _t->try_ask_stop(); break;
        case 6: _t->connectionError((*reinterpret_cast< QAbstractSocket::SocketError(*)>(_a[1]))); break;
        case 7: _t->errorOutput((*reinterpret_cast< QString(*)>(_a[1]))); break;
        case 8: _t->kicked(); break;
        case 9: _t->normalOutput((*reinterpret_cast< QString(*)>(_a[1]))); break;
        case 10: _t->send_player_informations(); break;
        case 11: _t->disconnectNextStep(); break;
        case 12: _t->disconnectClient(); break;
        case 13: _t->fake_receive_data((*reinterpret_cast< QByteArray(*)>(_a[1]))); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData CatchChallenger::Client::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject CatchChallenger::Client::staticMetaObject = {
    { &QObject::staticMetaObject, qt_meta_stringdata_CatchChallenger__Client,
      qt_meta_data_CatchChallenger__Client, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &CatchChallenger::Client::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *CatchChallenger::Client::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *CatchChallenger::Client::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_CatchChallenger__Client))
        return static_cast<void*>(const_cast< Client*>(this));
    return QObject::qt_metacast(_clname);
}

int CatchChallenger::Client::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 14)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 14;
    }
    return _id;
}

// SIGNAL 0
void CatchChallenger::Client::askIfIsReadyToStop()
{
    QMetaObject::activate(this, &staticMetaObject, 0, 0);
}

// SIGNAL 1
void CatchChallenger::Client::isReadyToDelete()
{
    QMetaObject::activate(this, &staticMetaObject, 1, 0);
}

// SIGNAL 2
void CatchChallenger::Client::send_fakeLogin(quint32 _t1, quint16 _t2, quint16 _t3, Map_server_MapVisibility_simple * _t4, Orientation _t5, QString _t6)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)), const_cast<void*>(reinterpret_cast<const void*>(&_t3)), const_cast<void*>(reinterpret_cast<const void*>(&_t4)), const_cast<void*>(reinterpret_cast<const void*>(&_t5)), const_cast<void*>(reinterpret_cast<const void*>(&_t6)) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void CatchChallenger::Client::fake_send_data(const QByteArray & _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}

// SIGNAL 4
void CatchChallenger::Client::fake_send_received_data(const QByteArray & _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}

// SIGNAL 5
void CatchChallenger::Client::try_ask_stop()
{
    QMetaObject::activate(this, &staticMetaObject, 5, 0);
}
QT_END_MOC_NAMESPACE
