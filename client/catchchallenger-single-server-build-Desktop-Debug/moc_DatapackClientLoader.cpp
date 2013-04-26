/****************************************************************************
** Meta object code from reading C++ file 'DatapackClientLoader.h'
**
** Created: Thu Apr 11 17:51:25 2013
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.4)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../base/interface/DatapackClientLoader.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'DatapackClientLoader.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.4. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_DatapackClientLoader[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
      16,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: signature, parameters, type, tag, flags
      22,   21,   21,   21, 0x05,

 // slots: signature, parameters, type, tag, flags
      52,   39,   21,   21, 0x0a,
      75,   21,   21,   21, 0x08,
      88,   21,   21,   21, 0x08,
     102,   21,   21,   21, 0x08,
     114,   21,   21,   21, 0x08,
     128,   21,   21,   21, 0x08,
     151,   21,   21,   21, 0x08,
     163,   21,   21,   21, 0x08,
     177,   21,   21,   21, 0x08,
     193,   21,   21,   21, 0x08,
     211,   21,   21,   21, 0x08,
     232,   21,   21,   21, 0x08,
     249,   21,   21,   21, 0x08,
     268,   21,   21,   21, 0x08,
     287,   21,   21,   21, 0x08,

       0        // eod
};

static const char qt_meta_stringdata_DatapackClientLoader[] = {
    "DatapackClientLoader\0\0datapackParsed()\0"
    "datapackPath\0parseDatapack(QString)\0"
    "parseItems()\0parseQuests()\0parseMaps()\0"
    "parsePlants()\0parseCraftingRecipes()\0"
    "parseBuff()\0parseSkills()\0parseMonsters()\0"
    "parseReputation()\0parseMonstersExtra()\0"
    "parseBuffExtra()\0parseSkillsExtra()\0"
    "parseQuestsExtra()\0parseQuestsLink()\0"
};

void DatapackClientLoader::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        DatapackClientLoader *_t = static_cast<DatapackClientLoader *>(_o);
        switch (_id) {
        case 0: _t->datapackParsed(); break;
        case 1: _t->parseDatapack((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 2: _t->parseItems(); break;
        case 3: _t->parseQuests(); break;
        case 4: _t->parseMaps(); break;
        case 5: _t->parsePlants(); break;
        case 6: _t->parseCraftingRecipes(); break;
        case 7: _t->parseBuff(); break;
        case 8: _t->parseSkills(); break;
        case 9: _t->parseMonsters(); break;
        case 10: _t->parseReputation(); break;
        case 11: _t->parseMonstersExtra(); break;
        case 12: _t->parseBuffExtra(); break;
        case 13: _t->parseSkillsExtra(); break;
        case 14: _t->parseQuestsExtra(); break;
        case 15: _t->parseQuestsLink(); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData DatapackClientLoader::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject DatapackClientLoader::staticMetaObject = {
    { &QThread::staticMetaObject, qt_meta_stringdata_DatapackClientLoader,
      qt_meta_data_DatapackClientLoader, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &DatapackClientLoader::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *DatapackClientLoader::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *DatapackClientLoader::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_DatapackClientLoader))
        return static_cast<void*>(const_cast< DatapackClientLoader*>(this));
    return QThread::qt_metacast(_clname);
}

int DatapackClientLoader::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QThread::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 16)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 16;
    }
    return _id;
}

// SIGNAL 0
void DatapackClientLoader::datapackParsed()
{
    QMetaObject::activate(this, &staticMetaObject, 0, 0);
}
QT_END_MOC_NAMESPACE
