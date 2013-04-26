/****************************************************************************
** Meta object code from reading C++ file 'NewGame.h'
**
** Created: Fri Apr 26 14:18:59 2013
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.4)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../single-player/NewGame.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'NewGame.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.4. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_NewGame[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       6,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: signature, parameters, type, tag, flags
       9,    8,    8,    8, 0x08,
      30,   25,    8,    8, 0x08,
      61,    8,    8,    8, 0x08,
      87,    8,    8,    8, 0x08,
     109,    8,    8,    8, 0x08,
     135,   25,    8,    8, 0x08,

       0        // eod
};

static const char qt_meta_stringdata_NewGame[] = {
    "NewGame\0\0on_ok_clicked()\0arg1\0"
    "on_pseudo_textChanged(QString)\0"
    "on_pseudo_returnPressed()\0"
    "on_nextSkin_clicked()\0on_previousSkin_clicked()\0"
    "on_gameName_textChanged(QString)\0"
};

void NewGame::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        NewGame *_t = static_cast<NewGame *>(_o);
        switch (_id) {
        case 0: _t->on_ok_clicked(); break;
        case 1: _t->on_pseudo_textChanged((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 2: _t->on_pseudo_returnPressed(); break;
        case 3: _t->on_nextSkin_clicked(); break;
        case 4: _t->on_previousSkin_clicked(); break;
        case 5: _t->on_gameName_textChanged((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData NewGame::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject NewGame::staticMetaObject = {
    { &QDialog::staticMetaObject, qt_meta_stringdata_NewGame,
      qt_meta_data_NewGame, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &NewGame::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *NewGame::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *NewGame::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_NewGame))
        return static_cast<void*>(const_cast< NewGame*>(this));
    return QDialog::qt_metacast(_clname);
}

int NewGame::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QDialog::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 6)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 6;
    }
    return _id;
}
QT_END_MOC_NAMESPACE
