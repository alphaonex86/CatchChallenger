/****************************************************************************
** Meta object code from reading C++ file 'InternalServer.h'
**
** Created: Fri Apr 26 14:18:59 2013
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.4)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../single-player/InternalServer.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'InternalServer.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.4. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_CatchChallenger__InternalServer[] = {

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
      33,   32,   32,   32, 0x08,
      57,   32,   32,   32, 0x09,

       0        // eod
};

static const char qt_meta_stringdata_CatchChallenger__InternalServer[] = {
    "CatchChallenger::InternalServer\0\0"
    "start_internal_server()\0removeOneClient()\0"
};

void CatchChallenger::InternalServer::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        InternalServer *_t = static_cast<InternalServer *>(_o);
        switch (_id) {
        case 0: _t->start_internal_server(); break;
        case 1: _t->removeOneClient(); break;
        default: ;
        }
    }
    Q_UNUSED(_a);
}

const QMetaObjectExtraData CatchChallenger::InternalServer::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject CatchChallenger::InternalServer::staticMetaObject = {
    { &BaseServer::staticMetaObject, qt_meta_stringdata_CatchChallenger__InternalServer,
      qt_meta_data_CatchChallenger__InternalServer, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &CatchChallenger::InternalServer::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *CatchChallenger::InternalServer::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *CatchChallenger::InternalServer::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_CatchChallenger__InternalServer))
        return static_cast<void*>(const_cast< InternalServer*>(this));
    return BaseServer::qt_metacast(_clname);
}

int CatchChallenger::InternalServer::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = BaseServer::qt_metacall(_c, _id, _a);
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
