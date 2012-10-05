/****************************************************************************
** Meta object code from reading C++ file 'InternalServer.h'
**
** Created: Fri Oct 5 13:40:09 2012
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../single-player/InternalServer.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'InternalServer.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.3. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_Pokecraft__InternalServer[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
      11,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       5,       // signalCount

 // signals: signature, parameters, type, tag, flags
      33,   27,   26,   26, 0x05,
      48,   26,   26,   26, 0x05,
      62,   26,   26,   26, 0x05,
      80,   26,   26,   26, 0x05,
      98,   26,   26,   26, 0x05,

 // slots: signature, parameters, type, tag, flags
     108,   26,   26,   26, 0x08,
     124,   26,   26,   26, 0x08,
     142,   26,   26,   26, 0x08,
     152,   26,   26,   26, 0x08,
     175,   26,   26,   26, 0x08,
     198,   26,   26,   26, 0x08,

       0        // eod
};

static const char qt_meta_stringdata_Pokecraft__InternalServer[] = {
    "Pokecraft::InternalServer\0\0error\0"
    "error(QString)\0try_initAll()\0"
    "try_stop_server()\0need_be_started()\0"
    "isReady()\0newConnection()\0removeOneClient()\0"
    "initAll()\0stop_internal_server()\0"
    "check_if_now_stopped()\0start_internal_server()\0"
};

void Pokecraft::InternalServer::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        InternalServer *_t = static_cast<InternalServer *>(_o);
        switch (_id) {
        case 0: _t->error((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 1: _t->try_initAll(); break;
        case 2: _t->try_stop_server(); break;
        case 3: _t->need_be_started(); break;
        case 4: _t->isReady(); break;
        case 5: _t->newConnection(); break;
        case 6: _t->removeOneClient(); break;
        case 7: _t->initAll(); break;
        case 8: _t->stop_internal_server(); break;
        case 9: _t->check_if_now_stopped(); break;
        case 10: _t->start_internal_server(); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData Pokecraft::InternalServer::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject Pokecraft::InternalServer::staticMetaObject = {
    { &QObject::staticMetaObject, qt_meta_stringdata_Pokecraft__InternalServer,
      qt_meta_data_Pokecraft__InternalServer, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &Pokecraft::InternalServer::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *Pokecraft::InternalServer::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *Pokecraft::InternalServer::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_Pokecraft__InternalServer))
        return static_cast<void*>(const_cast< InternalServer*>(this));
    return QObject::qt_metacast(_clname);
}

int Pokecraft::InternalServer::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
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
void Pokecraft::InternalServer::error(const QString & _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void Pokecraft::InternalServer::try_initAll()
{
    QMetaObject::activate(this, &staticMetaObject, 1, 0);
}

// SIGNAL 2
void Pokecraft::InternalServer::try_stop_server()
{
    QMetaObject::activate(this, &staticMetaObject, 2, 0);
}

// SIGNAL 3
void Pokecraft::InternalServer::need_be_started()
{
    QMetaObject::activate(this, &staticMetaObject, 3, 0);
}

// SIGNAL 4
void Pokecraft::InternalServer::isReady()
{
    QMetaObject::activate(this, &staticMetaObject, 4, 0);
}
QT_END_MOC_NAMESPACE
