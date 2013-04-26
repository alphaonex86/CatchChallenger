/****************************************************************************
** Meta object code from reading C++ file 'Options.h'
**
** Created: Thu Apr 11 17:51:25 2013
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.4)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../base/Options.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'Options.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.4. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_Options[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       7,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       4,       // signalCount

 // signals: signature, parameters, type, tag, flags
      13,    9,    8,    8, 0x05,
      40,   29,    8,    8, 0x05,
      60,    9,    8,    8, 0x05,
      86,   81,    8,    8, 0x05,

 // slots: signature, parameters, type, tag, flags
     107,    9,    8,    8, 0x0a,
     134,  123,    8,    8, 0x0a,
     154,   81,    8,    8, 0x0a,

       0        // eod
};

static const char qt_meta_stringdata_Options[] = {
    "Options\0\0fps\0newFPS(quint16)\0fpsLimited\0"
    "newLimitedFPS(bool)\0newFinalFPS(quint16)\0"
    "zoom\0newZoomEnabled(bool)\0setFPS(quint16)\0"
    "limitedFPS\0setLimitedFPS(bool)\0"
    "setZoomEnabled(bool)\0"
};

void Options::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        Options *_t = static_cast<Options *>(_o);
        switch (_id) {
        case 0: _t->newFPS((*reinterpret_cast< const quint16(*)>(_a[1]))); break;
        case 1: _t->newLimitedFPS((*reinterpret_cast< const bool(*)>(_a[1]))); break;
        case 2: _t->newFinalFPS((*reinterpret_cast< const quint16(*)>(_a[1]))); break;
        case 3: _t->newZoomEnabled((*reinterpret_cast< const bool(*)>(_a[1]))); break;
        case 4: _t->setFPS((*reinterpret_cast< const quint16(*)>(_a[1]))); break;
        case 5: _t->setLimitedFPS((*reinterpret_cast< const bool(*)>(_a[1]))); break;
        case 6: _t->setZoomEnabled((*reinterpret_cast< const bool(*)>(_a[1]))); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData Options::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject Options::staticMetaObject = {
    { &QObject::staticMetaObject, qt_meta_stringdata_Options,
      qt_meta_data_Options, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &Options::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *Options::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *Options::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_Options))
        return static_cast<void*>(const_cast< Options*>(this));
    return QObject::qt_metacast(_clname);
}

int Options::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 7)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 7;
    }
    return _id;
}

// SIGNAL 0
void Options::newFPS(const quint16 & _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void Options::newLimitedFPS(const bool & _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void Options::newFinalFPS(const quint16 & _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void Options::newZoomEnabled(const bool & _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}
QT_END_MOC_NAMESPACE
