/****************************************************************************
** Meta object code from reading C++ file 'ClientNetworkWrite.h'
**
** Created: Fri Apr 26 14:18:59 2013
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.4)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../server/base/ClientNetworkWrite.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'ClientNetworkWrite.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.4. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_CatchChallenger__ClientNetworkWrite[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       6,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: signature, parameters, type, tag, flags
      37,   36,   36,   36, 0x05,

 // slots: signature, parameters, type, tag, flags
      77,   53,   36,   36, 0x0a,
     130,  115,   36,   36, 0x0a,
     196,  160,   36,   36, 0x0a,
     257,  240,   36,   36, 0x0a,
     286,   36,   36,   36, 0x0a,

       0        // eod
};

static const char qt_meta_stringdata_CatchChallenger__ClientNetworkWrite[] = {
    "CatchChallenger::ClientNetworkWrite\0"
    "\0isReadyToStop()\0mainIdent,subIdent,data\0"
    "sendPacket(quint8,quint16,QByteArray)\0"
    "mainIdent,data\0sendPacket(quint8,QByteArray)\0"
    "mainIdent,subIdent,queryNumber,data\0"
    "sendQuery(quint8,quint16,quint8,QByteArray)\0"
    "queryNumber,data\0postReply(quint8,QByteArray)\0"
    "askIfIsReadyToStop()\0"
};

void CatchChallenger::ClientNetworkWrite::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        ClientNetworkWrite *_t = static_cast<ClientNetworkWrite *>(_o);
        switch (_id) {
        case 0: _t->isReadyToStop(); break;
        case 1: _t->sendPacket((*reinterpret_cast< const quint8(*)>(_a[1])),(*reinterpret_cast< const quint16(*)>(_a[2])),(*reinterpret_cast< const QByteArray(*)>(_a[3]))); break;
        case 2: _t->sendPacket((*reinterpret_cast< const quint8(*)>(_a[1])),(*reinterpret_cast< const QByteArray(*)>(_a[2]))); break;
        case 3: _t->sendQuery((*reinterpret_cast< const quint8(*)>(_a[1])),(*reinterpret_cast< const quint16(*)>(_a[2])),(*reinterpret_cast< const quint8(*)>(_a[3])),(*reinterpret_cast< const QByteArray(*)>(_a[4]))); break;
        case 4: _t->postReply((*reinterpret_cast< const quint8(*)>(_a[1])),(*reinterpret_cast< const QByteArray(*)>(_a[2]))); break;
        case 5: _t->askIfIsReadyToStop(); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData CatchChallenger::ClientNetworkWrite::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject CatchChallenger::ClientNetworkWrite::staticMetaObject = {
    { &ProtocolParsingOutput::staticMetaObject, qt_meta_stringdata_CatchChallenger__ClientNetworkWrite,
      qt_meta_data_CatchChallenger__ClientNetworkWrite, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &CatchChallenger::ClientNetworkWrite::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *CatchChallenger::ClientNetworkWrite::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *CatchChallenger::ClientNetworkWrite::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_CatchChallenger__ClientNetworkWrite))
        return static_cast<void*>(const_cast< ClientNetworkWrite*>(this));
    return ProtocolParsingOutput::qt_metacast(_clname);
}

int CatchChallenger::ClientNetworkWrite::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = ProtocolParsingOutput::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 6)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 6;
    }
    return _id;
}

// SIGNAL 0
void CatchChallenger::ClientNetworkWrite::isReadyToStop()
{
    QMetaObject::activate(this, &staticMetaObject, 0, 0);
}
QT_END_MOC_NAMESPACE
