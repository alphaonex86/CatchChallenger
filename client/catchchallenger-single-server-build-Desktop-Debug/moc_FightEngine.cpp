/****************************************************************************
** Meta object code from reading C++ file 'FightEngine.h'
**
** Created: Thu Apr 11 17:51:26 2013
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.4)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../fight/interface/FightEngine.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'FightEngine.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.4. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_CatchChallenger__FightEngine[] = {

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
      35,   30,   29,   29, 0x0a,
      93,   79,   65,   29, 0x0a,

       0        // eod
};

static const char qt_meta_stringdata_CatchChallenger__FightEngine[] = {
    "CatchChallenger::FightEngine\0\0data\0"
    "appendRandomSeeds(QByteArray)\0"
    "Monster::Stat\0monster,level\0"
    "getStat(Monster,quint8)\0"
};

void CatchChallenger::FightEngine::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        FightEngine *_t = static_cast<FightEngine *>(_o);
        switch (_id) {
        case 0: _t->appendRandomSeeds((*reinterpret_cast< const QByteArray(*)>(_a[1]))); break;
        case 1: { Monster::Stat _r = _t->getStat((*reinterpret_cast< const Monster(*)>(_a[1])),(*reinterpret_cast< const quint8(*)>(_a[2])));
            if (_a[0]) *reinterpret_cast< Monster::Stat*>(_a[0]) = _r; }  break;
        default: ;
        }
    }
}

const QMetaObjectExtraData CatchChallenger::FightEngine::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject CatchChallenger::FightEngine::staticMetaObject = {
    { &QObject::staticMetaObject, qt_meta_stringdata_CatchChallenger__FightEngine,
      qt_meta_data_CatchChallenger__FightEngine, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &CatchChallenger::FightEngine::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *CatchChallenger::FightEngine::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *CatchChallenger::FightEngine::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_CatchChallenger__FightEngine))
        return static_cast<void*>(const_cast< FightEngine*>(this));
    return QObject::qt_metacast(_clname);
}

int CatchChallenger::FightEngine::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
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
QT_END_MOC_NAMESPACE
