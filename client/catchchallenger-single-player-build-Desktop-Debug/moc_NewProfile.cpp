/****************************************************************************
** Meta object code from reading C++ file 'NewProfile.h'
**
** Created: Fri Apr 26 14:18:59 2013
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.4)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../single-player/NewProfile.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'NewProfile.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.4. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_NewProfile[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       5,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: signature, parameters, type, tag, flags
      25,   12,   11,   11, 0x05,

 // slots: signature, parameters, type, tag, flags
      48,   11,   11,   11, 0x08,
      64,   11,   11,   11, 0x08,
      96,   90,   11,   11, 0x08,
     133,   11,   11,   11, 0x08,

       0        // eod
};

static const char qt_meta_stringdata_NewProfile[] = {
    "NewProfile\0\0datapackPath\0"
    "parseDatapack(QString)\0on_ok_clicked()\0"
    "on_pushButton_2_clicked()\0index\0"
    "on_comboBox_currentIndexChanged(int)\0"
    "datapackParsed()\0"
};

void NewProfile::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        NewProfile *_t = static_cast<NewProfile *>(_o);
        switch (_id) {
        case 0: _t->parseDatapack((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 1: _t->on_ok_clicked(); break;
        case 2: _t->on_pushButton_2_clicked(); break;
        case 3: _t->on_comboBox_currentIndexChanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 4: _t->datapackParsed(); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData NewProfile::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject NewProfile::staticMetaObject = {
    { &QDialog::staticMetaObject, qt_meta_stringdata_NewProfile,
      qt_meta_data_NewProfile, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &NewProfile::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *NewProfile::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *NewProfile::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_NewProfile))
        return static_cast<void*>(const_cast< NewProfile*>(this));
    return QDialog::qt_metacast(_clname);
}

int NewProfile::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QDialog::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 5)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 5;
    }
    return _id;
}

// SIGNAL 0
void NewProfile::parseDatapack(const QString & _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}
QT_END_MOC_NAMESPACE
