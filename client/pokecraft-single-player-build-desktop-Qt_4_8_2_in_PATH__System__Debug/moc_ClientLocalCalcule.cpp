/****************************************************************************
** Meta object code from reading C++ file 'ClientLocalCalcule.h'
**
** Created: Fri Oct 5 13:40:09 2012
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../server/base/ClientLocalCalcule.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'ClientLocalCalcule.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.3. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_Pokecraft__ClientLocalCalcule[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       6,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       2,       // signalCount

 // signals: signature, parameters, type, tag, flags
      40,   31,   30,   30, 0x05,
      57,   30,   30,   30, 0x05,

 // slots: signature, parameters, type, tag, flags
      95,   75,   30,   30, 0x0a,
     183,  155,  150,   30, 0x0a,
     226,  215,   30,   30, 0x0a,
     254,   30,   30,   30, 0x08,

       0        // eod
};

static const char qt_meta_stringdata_Pokecraft__ClientLocalCalcule[] = {
    "Pokecraft::ClientLocalCalcule\0\0sqlQuery\0"
    "dbQuery(QString)\0askRandomNumber()\0"
    "map,x,y,orientation\0"
    "put_on_the_map(Map*,COORD_TYPE,COORD_TYPE,Orientation)\0"
    "bool\0previousMovedUnit,direction\0"
    "moveThePlayer(quint8,Direction)\0"
    "randomData\0newRandomNumber(QByteArray)\0"
    "extraStop()\0"
};

void Pokecraft::ClientLocalCalcule::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        ClientLocalCalcule *_t = static_cast<ClientLocalCalcule *>(_o);
        switch (_id) {
        case 0: _t->dbQuery((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 1: _t->askRandomNumber(); break;
        case 2: _t->put_on_the_map((*reinterpret_cast< Map*(*)>(_a[1])),(*reinterpret_cast< const COORD_TYPE(*)>(_a[2])),(*reinterpret_cast< const COORD_TYPE(*)>(_a[3])),(*reinterpret_cast< const Orientation(*)>(_a[4]))); break;
        case 3: { bool _r = _t->moveThePlayer((*reinterpret_cast< const quint8(*)>(_a[1])),(*reinterpret_cast< const Direction(*)>(_a[2])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = _r; }  break;
        case 4: _t->newRandomNumber((*reinterpret_cast< const QByteArray(*)>(_a[1]))); break;
        case 5: _t->extraStop(); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData Pokecraft::ClientLocalCalcule::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject Pokecraft::ClientLocalCalcule::staticMetaObject = {
    { &MapBasicMove::staticMetaObject, qt_meta_stringdata_Pokecraft__ClientLocalCalcule,
      qt_meta_data_Pokecraft__ClientLocalCalcule, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &Pokecraft::ClientLocalCalcule::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *Pokecraft::ClientLocalCalcule::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *Pokecraft::ClientLocalCalcule::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_Pokecraft__ClientLocalCalcule))
        return static_cast<void*>(const_cast< ClientLocalCalcule*>(this));
    return MapBasicMove::qt_metacast(_clname);
}

int Pokecraft::ClientLocalCalcule::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = MapBasicMove::qt_metacall(_c, _id, _a);
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
void Pokecraft::ClientLocalCalcule::dbQuery(const QString & _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void Pokecraft::ClientLocalCalcule::askRandomNumber()
{
    QMetaObject::activate(this, &staticMetaObject, 1, 0);
}
QT_END_MOC_NAMESPACE
