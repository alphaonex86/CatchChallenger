/****************************************************************************
** Meta object code from reading C++ file 'Api_client_real.h'
**
** Created: Fri Apr 26 14:18:58 2013
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.4)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../base/Api_client_real.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'Api_client_real.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.4. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_CatchChallenger__Api_client_real[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       2,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: signature, parameters, type, tag, flags
      34,   33,   33,   33, 0x08,
      69,   49,   33,   33, 0x08,

       0        // eod
};

static const char qt_meta_stringdata_CatchChallenger__Api_client_real[] = {
    "CatchChallenger::Api_client_real\0\0"
    "disconnected()\0fileName,data,mtime\0"
    "writeNewFile(QString,QByteArray,quint64)\0"
};

void CatchChallenger::Api_client_real::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        Api_client_real *_t = static_cast<Api_client_real *>(_o);
        switch (_id) {
        case 0: _t->disconnected(); break;
        case 1: _t->writeNewFile((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< const QByteArray(*)>(_a[2])),(*reinterpret_cast< const quint64(*)>(_a[3]))); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData CatchChallenger::Api_client_real::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject CatchChallenger::Api_client_real::staticMetaObject = {
    { &Api_protocol::staticMetaObject, qt_meta_stringdata_CatchChallenger__Api_client_real,
      qt_meta_data_CatchChallenger__Api_client_real, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &CatchChallenger::Api_client_real::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *CatchChallenger::Api_client_real::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *CatchChallenger::Api_client_real::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_CatchChallenger__Api_client_real))
        return static_cast<void*>(const_cast< Api_client_real*>(this));
    return Api_protocol::qt_metacast(_clname);
}

int CatchChallenger::Api_client_real::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = Api_protocol::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 2)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 2;
    }
    return _id;
}
QT_END_MOC_NAMESPACE
