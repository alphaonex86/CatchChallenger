/****************************************************************************
** Meta object code from reading C++ file 'MapBasicMove.h'
**
** Created: Fri Apr 26 14:18:59 2013
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.4)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../server/base/ClientMapManagement/MapBasicMove.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'MapBasicMove.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.4. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_CatchChallenger__MapBasicMove[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
      11,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       7,       // signalCount

 // signals: signature, parameters, type, tag, flags
      37,   31,   30,   30, 0x05,
      60,   52,   30,   30, 0x05,
      77,   30,   30,   30, 0x05,
     117,   93,   30,   30, 0x05,
     174,  155,   30,   30, 0x25,
     216,  201,   30,   30, 0x05,
     256,  246,   30,   30, 0x25,

 // slots: signature, parameters, type, tag, flags
     295,  275,   30,   30, 0x0a,
     375,  347,  342,   30, 0x0a,
     407,  275,   30,   30, 0x0a,
     459,   30,   30,   30, 0x0a,

       0        // eod
};

static const char qt_meta_stringdata_CatchChallenger__MapBasicMove[] = {
    "CatchChallenger::MapBasicMove\0\0error\0"
    "error(QString)\0message\0message(QString)\0"
    "isReadyToStop()\0mainIdent,subIdent,data\0"
    "sendPacket(quint8,quint16,QByteArray)\0"
    "mainIdent,subIdent\0sendPacket(quint8,quint16)\0"
    "mainIdent,data\0sendPacket(quint8,QByteArray)\0"
    "mainIdent\0sendPacket(quint8)\0"
    "map,x,y,orientation\0"
    "put_on_the_map(Map*,quint8,quint8,Orientation)\0"
    "bool\0previousMovedUnit,direction\0"
    "moveThePlayer(quint8,Direction)\0"
    "teleportValidatedTo(Map*,quint8,quint8,Orientation)\0"
    "askIfIsReadyToStop()\0"
};

void CatchChallenger::MapBasicMove::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        MapBasicMove *_t = static_cast<MapBasicMove *>(_o);
        switch (_id) {
        case 0: _t->error((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 1: _t->message((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 2: _t->isReadyToStop(); break;
        case 3: _t->sendPacket((*reinterpret_cast< const quint8(*)>(_a[1])),(*reinterpret_cast< const quint16(*)>(_a[2])),(*reinterpret_cast< const QByteArray(*)>(_a[3]))); break;
        case 4: _t->sendPacket((*reinterpret_cast< const quint8(*)>(_a[1])),(*reinterpret_cast< const quint16(*)>(_a[2]))); break;
        case 5: _t->sendPacket((*reinterpret_cast< const quint8(*)>(_a[1])),(*reinterpret_cast< const QByteArray(*)>(_a[2]))); break;
        case 6: _t->sendPacket((*reinterpret_cast< const quint8(*)>(_a[1]))); break;
        case 7: _t->put_on_the_map((*reinterpret_cast< Map*(*)>(_a[1])),(*reinterpret_cast< const quint8(*)>(_a[2])),(*reinterpret_cast< const quint8(*)>(_a[3])),(*reinterpret_cast< const Orientation(*)>(_a[4]))); break;
        case 8: { bool _r = _t->moveThePlayer((*reinterpret_cast< const quint8(*)>(_a[1])),(*reinterpret_cast< const Direction(*)>(_a[2])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = _r; }  break;
        case 9: _t->teleportValidatedTo((*reinterpret_cast< Map*(*)>(_a[1])),(*reinterpret_cast< const quint8(*)>(_a[2])),(*reinterpret_cast< const quint8(*)>(_a[3])),(*reinterpret_cast< const Orientation(*)>(_a[4]))); break;
        case 10: _t->askIfIsReadyToStop(); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData CatchChallenger::MapBasicMove::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject CatchChallenger::MapBasicMove::staticMetaObject = {
    { &QObject::staticMetaObject, qt_meta_stringdata_CatchChallenger__MapBasicMove,
      qt_meta_data_CatchChallenger__MapBasicMove, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &CatchChallenger::MapBasicMove::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *CatchChallenger::MapBasicMove::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *CatchChallenger::MapBasicMove::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_CatchChallenger__MapBasicMove))
        return static_cast<void*>(const_cast< MapBasicMove*>(this));
    return QObject::qt_metacast(_clname);
}

int CatchChallenger::MapBasicMove::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 11)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 11;
    }
    return _id;
}

// SIGNAL 0
void CatchChallenger::MapBasicMove::error(const QString & _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void CatchChallenger::MapBasicMove::message(const QString & _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void CatchChallenger::MapBasicMove::isReadyToStop()
{
    QMetaObject::activate(this, &staticMetaObject, 2, 0);
}

// SIGNAL 3
void CatchChallenger::MapBasicMove::sendPacket(const quint8 & _t1, const quint16 & _t2, const QByteArray & _t3)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)), const_cast<void*>(reinterpret_cast<const void*>(&_t3)) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}

// SIGNAL 5
void CatchChallenger::MapBasicMove::sendPacket(const quint8 & _t1, const QByteArray & _t2)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 5, _a);
}
QT_END_MOC_NAMESPACE
