/****************************************************************************
** Meta object code from reading C++ file 'Api_client_virtual.h'
**
** Created: Fri Oct 5 13:40:08 2012
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../base/Api_client_virtual.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'Api_client_virtual.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.3. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_Pokecraft__Api_client_virtual[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       0,    0, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

       0        // eod
};

static const char qt_meta_stringdata_Pokecraft__Api_client_virtual[] = {
    "Pokecraft::Api_client_virtual\0"
};

void Pokecraft::Api_client_virtual::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    Q_UNUSED(_o);
    Q_UNUSED(_id);
    Q_UNUSED(_c);
    Q_UNUSED(_a);
}

const QMetaObjectExtraData Pokecraft::Api_client_virtual::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject Pokecraft::Api_client_virtual::staticMetaObject = {
    { &Api_protocol::staticMetaObject, qt_meta_stringdata_Pokecraft__Api_client_virtual,
      qt_meta_data_Pokecraft__Api_client_virtual, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &Pokecraft::Api_client_virtual::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *Pokecraft::Api_client_virtual::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *Pokecraft::Api_client_virtual::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_Pokecraft__Api_client_virtual))
        return static_cast<void*>(const_cast< Api_client_virtual*>(this));
    return Api_protocol::qt_metacast(_clname);
}

int Pokecraft::Api_client_virtual::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = Api_protocol::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    return _id;
}
QT_END_MOC_NAMESPACE
