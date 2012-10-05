/****************************************************************************
** Meta object code from reading C++ file 'QFakeServer.h'
**
** Created: Fri Oct 5 13:40:08 2012
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../general/base/QFakeServer.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'QFakeServer.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.3. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_Pokecraft__QFakeServer[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       2,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: signature, parameters, type, tag, flags
      24,   23,   23,   23, 0x05,

 // slots: signature, parameters, type, tag, flags
      40,   23,   23,   23, 0x08,

       0        // eod
};

static const char qt_meta_stringdata_Pokecraft__QFakeServer[] = {
    "Pokecraft::QFakeServer\0\0newConnection()\0"
    "disconnectedSocket()\0"
};

void Pokecraft::QFakeServer::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        QFakeServer *_t = static_cast<QFakeServer *>(_o);
        switch (_id) {
        case 0: _t->newConnection(); break;
        case 1: _t->disconnectedSocket(); break;
        default: ;
        }
    }
    Q_UNUSED(_a);
}

const QMetaObjectExtraData Pokecraft::QFakeServer::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject Pokecraft::QFakeServer::staticMetaObject = {
    { &QObject::staticMetaObject, qt_meta_stringdata_Pokecraft__QFakeServer,
      qt_meta_data_Pokecraft__QFakeServer, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &Pokecraft::QFakeServer::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *Pokecraft::QFakeServer::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *Pokecraft::QFakeServer::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_Pokecraft__QFakeServer))
        return static_cast<void*>(const_cast< QFakeServer*>(this));
    return QObject::qt_metacast(_clname);
}

int Pokecraft::QFakeServer::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 2)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 2;
    }
    return _id;
}

// SIGNAL 0
void Pokecraft::QFakeServer::newConnection()
{
    QMetaObject::activate(this, &staticMetaObject, 0, 0);
}
QT_END_MOC_NAMESPACE
