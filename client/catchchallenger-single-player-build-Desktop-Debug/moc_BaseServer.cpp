/****************************************************************************
** Meta object code from reading C++ file 'BaseServer.h'
**
** Created: Fri Apr 26 14:18:59 2013
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.4)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../server/base/BaseServer.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'BaseServer.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.4. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_CatchChallenger__BaseServer[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
      10,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       5,       // signalCount

 // signals: signature, parameters, type, tag, flags
      35,   29,   28,   28, 0x05,
      50,   28,   28,   28, 0x05,
      64,   28,   28,   28, 0x05,
      82,   28,   28,   28, 0x05,
     100,   28,   28,   28, 0x05,

 // slots: signature, parameters, type, tag, flags
     117,   28,   28,   28, 0x09,
     141,   28,   28,   28, 0x09,
     164,   28,   28,   28, 0x09,
     174,   28,   28,   28, 0x09,
     192,   28,   28,   28, 0x09,

       0        // eod
};

static const char qt_meta_stringdata_CatchChallenger__BaseServer[] = {
    "CatchChallenger::BaseServer\0\0error\0"
    "error(QString)\0try_initAll()\0"
    "try_stop_server()\0need_be_started()\0"
    "is_started(bool)\0start_internal_server()\0"
    "stop_internal_server()\0initAll()\0"
    "removeOneClient()\0newConnection()\0"
};

void CatchChallenger::BaseServer::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        BaseServer *_t = static_cast<BaseServer *>(_o);
        switch (_id) {
        case 0: _t->error((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 1: _t->try_initAll(); break;
        case 2: _t->try_stop_server(); break;
        case 3: _t->need_be_started(); break;
        case 4: _t->is_started((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 5: _t->start_internal_server(); break;
        case 6: _t->stop_internal_server(); break;
        case 7: _t->initAll(); break;
        case 8: _t->removeOneClient(); break;
        case 9: _t->newConnection(); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData CatchChallenger::BaseServer::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject CatchChallenger::BaseServer::staticMetaObject = {
    { &QObject::staticMetaObject, qt_meta_stringdata_CatchChallenger__BaseServer,
      qt_meta_data_CatchChallenger__BaseServer, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &CatchChallenger::BaseServer::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *CatchChallenger::BaseServer::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *CatchChallenger::BaseServer::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_CatchChallenger__BaseServer))
        return static_cast<void*>(const_cast< BaseServer*>(this));
    if (!strcmp(_clname, "BaseServerCrafting"))
        return static_cast< BaseServerCrafting*>(const_cast< BaseServer*>(this));
    if (!strcmp(_clname, "BaseServerFight"))
        return static_cast< BaseServerFight*>(const_cast< BaseServer*>(this));
    return QObject::qt_metacast(_clname);
}

int CatchChallenger::BaseServer::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 10)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 10;
    }
    return _id;
}

// SIGNAL 0
void CatchChallenger::BaseServer::error(const QString & _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void CatchChallenger::BaseServer::try_initAll()
{
    QMetaObject::activate(this, &staticMetaObject, 1, 0);
}

// SIGNAL 2
void CatchChallenger::BaseServer::try_stop_server()
{
    QMetaObject::activate(this, &staticMetaObject, 2, 0);
}

// SIGNAL 3
void CatchChallenger::BaseServer::need_be_started()
{
    QMetaObject::activate(this, &staticMetaObject, 3, 0);
}

// SIGNAL 4
void CatchChallenger::BaseServer::is_started(bool _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}
QT_END_MOC_NAMESPACE
