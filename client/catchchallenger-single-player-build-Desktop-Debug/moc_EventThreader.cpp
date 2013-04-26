/****************************************************************************
** Meta object code from reading C++ file 'EventThreader.h'
**
** Created: Fri Apr 26 14:18:59 2013
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.4)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../server/base/EventThreader.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'EventThreader.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.4. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_CatchChallenger__EventThreader[] = {

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
      32,   31,   31,   31, 0x05,

 // slots: signature, parameters, type, tag, flags
      49,   31,   31,   31, 0x0a,

       0        // eod
};

static const char qt_meta_stringdata_CatchChallenger__EventThreader[] = {
    "CatchChallenger::EventThreader\0\0"
    "return_latency()\0probe_latency()\0"
};

void CatchChallenger::EventThreader::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        EventThreader *_t = static_cast<EventThreader *>(_o);
        switch (_id) {
        case 0: _t->return_latency(); break;
        case 1: _t->probe_latency(); break;
        default: ;
        }
    }
    Q_UNUSED(_a);
}

const QMetaObjectExtraData CatchChallenger::EventThreader::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject CatchChallenger::EventThreader::staticMetaObject = {
    { &QThread::staticMetaObject, qt_meta_stringdata_CatchChallenger__EventThreader,
      qt_meta_data_CatchChallenger__EventThreader, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &CatchChallenger::EventThreader::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *CatchChallenger::EventThreader::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *CatchChallenger::EventThreader::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_CatchChallenger__EventThreader))
        return static_cast<void*>(const_cast< EventThreader*>(this));
    return QThread::qt_metacast(_clname);
}

int CatchChallenger::EventThreader::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QThread::qt_metacall(_c, _id, _a);
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
void CatchChallenger::EventThreader::return_latency()
{
    QMetaObject::activate(this, &staticMetaObject, 0, 0);
}
QT_END_MOC_NAMESPACE
