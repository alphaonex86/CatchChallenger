/****************************************************************************
** Meta object code from reading C++ file 'ClientMapManagement.h'
**
** Created: Fri Oct 5 13:40:09 2012
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../server/base/ClientMapManagement/ClientMapManagement.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'ClientMapManagement.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.3. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_Pokecraft__ClientMapManagement[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       3,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: signature, parameters, type, tag, flags
      65,   37,   32,   31, 0x0a,
      97,   31,   31,   31, 0x0a,
     111,   31,   31,   31, 0x08,

       0        // eod
};

static const char qt_meta_stringdata_Pokecraft__ClientMapManagement[] = {
    "Pokecraft::ClientMapManagement\0\0bool\0"
    "previousMovedUnit,direction\0"
    "moveThePlayer(quint8,Direction)\0"
    "purgeBuffer()\0extraStop()\0"
};

void Pokecraft::ClientMapManagement::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        ClientMapManagement *_t = static_cast<ClientMapManagement *>(_o);
        switch (_id) {
        case 0: { bool _r = _t->moveThePlayer((*reinterpret_cast< const quint8(*)>(_a[1])),(*reinterpret_cast< const Direction(*)>(_a[2])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = _r; }  break;
        case 1: _t->purgeBuffer(); break;
        case 2: _t->extraStop(); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData Pokecraft::ClientMapManagement::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject Pokecraft::ClientMapManagement::staticMetaObject = {
    { &MapBasicMove::staticMetaObject, qt_meta_stringdata_Pokecraft__ClientMapManagement,
      qt_meta_data_Pokecraft__ClientMapManagement, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &Pokecraft::ClientMapManagement::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *Pokecraft::ClientMapManagement::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *Pokecraft::ClientMapManagement::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_Pokecraft__ClientMapManagement))
        return static_cast<void*>(const_cast< ClientMapManagement*>(this));
    return MapBasicMove::qt_metacast(_clname);
}

int Pokecraft::ClientMapManagement::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = MapBasicMove::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 3)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 3;
    }
    return _id;
}
QT_END_MOC_NAMESPACE
