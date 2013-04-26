/****************************************************************************
** Meta object code from reading C++ file 'ProtocolParsing.h'
**
** Created: Fri Apr 26 14:18:59 2013
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.4)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../general/base/ProtocolParsing.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'ProtocolParsing.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.4. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_CatchChallenger__ProtocolParsing[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       3,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       2,       // signalCount

 // signals: signature, parameters, type, tag, flags
      40,   34,   33,   33, 0x05,
      63,   55,   33,   33, 0x05,

 // slots: signature, parameters, type, tag, flags
      80,   33,   33,   33, 0x08,

       0        // eod
};

static const char qt_meta_stringdata_CatchChallenger__ProtocolParsing[] = {
    "CatchChallenger::ProtocolParsing\0\0"
    "error\0error(QString)\0message\0"
    "message(QString)\0reset()\0"
};

void CatchChallenger::ProtocolParsing::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        ProtocolParsing *_t = static_cast<ProtocolParsing *>(_o);
        switch (_id) {
        case 0: _t->error((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 1: _t->message((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 2: _t->reset(); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData CatchChallenger::ProtocolParsing::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject CatchChallenger::ProtocolParsing::staticMetaObject = {
    { &QObject::staticMetaObject, qt_meta_stringdata_CatchChallenger__ProtocolParsing,
      qt_meta_data_CatchChallenger__ProtocolParsing, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &CatchChallenger::ProtocolParsing::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *CatchChallenger::ProtocolParsing::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *CatchChallenger::ProtocolParsing::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_CatchChallenger__ProtocolParsing))
        return static_cast<void*>(const_cast< ProtocolParsing*>(this));
    return QObject::qt_metacast(_clname);
}

int CatchChallenger::ProtocolParsing::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 3)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 3;
    }
    return _id;
}

// SIGNAL 0
void CatchChallenger::ProtocolParsing::error(const QString & _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void CatchChallenger::ProtocolParsing::message(const QString & _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}
static const uint qt_meta_data_CatchChallenger__ProtocolParsingInput[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       6,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       2,       // signalCount

 // signals: signature, parameters, type, tag, flags
      64,   39,   38,   38, 0x05,
     130,   93,   38,   38, 0x05,

 // slots: signature, parameters, type, tag, flags
     167,   38,   38,   38, 0x09,
     188,   38,   38,   38, 0x08,
     196,   39,   38,   38, 0x0a,
     226,   93,   38,   38, 0x0a,

       0        // eod
};

static const char qt_meta_stringdata_CatchChallenger__ProtocolParsingInput[] = {
    "CatchChallenger::ProtocolParsingInput\0"
    "\0mainCodeType,queryNumber\0"
    "newInputQuery(quint8,quint8)\0"
    "mainCodeType,subCodeType,queryNumber\0"
    "newInputQuery(quint8,quint16,quint8)\0"
    "parseIncommingData()\0reset()\0"
    "newOutputQuery(quint8,quint8)\0"
    "newOutputQuery(quint8,quint16,quint8)\0"
};

void CatchChallenger::ProtocolParsingInput::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        ProtocolParsingInput *_t = static_cast<ProtocolParsingInput *>(_o);
        switch (_id) {
        case 0: _t->newInputQuery((*reinterpret_cast< const quint8(*)>(_a[1])),(*reinterpret_cast< const quint8(*)>(_a[2]))); break;
        case 1: _t->newInputQuery((*reinterpret_cast< const quint8(*)>(_a[1])),(*reinterpret_cast< const quint16(*)>(_a[2])),(*reinterpret_cast< const quint8(*)>(_a[3]))); break;
        case 2: _t->parseIncommingData(); break;
        case 3: _t->reset(); break;
        case 4: _t->newOutputQuery((*reinterpret_cast< const quint8(*)>(_a[1])),(*reinterpret_cast< const quint8(*)>(_a[2]))); break;
        case 5: _t->newOutputQuery((*reinterpret_cast< const quint8(*)>(_a[1])),(*reinterpret_cast< const quint16(*)>(_a[2])),(*reinterpret_cast< const quint8(*)>(_a[3]))); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData CatchChallenger::ProtocolParsingInput::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject CatchChallenger::ProtocolParsingInput::staticMetaObject = {
    { &ProtocolParsing::staticMetaObject, qt_meta_stringdata_CatchChallenger__ProtocolParsingInput,
      qt_meta_data_CatchChallenger__ProtocolParsingInput, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &CatchChallenger::ProtocolParsingInput::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *CatchChallenger::ProtocolParsingInput::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *CatchChallenger::ProtocolParsingInput::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_CatchChallenger__ProtocolParsingInput))
        return static_cast<void*>(const_cast< ProtocolParsingInput*>(this));
    return ProtocolParsing::qt_metacast(_clname);
}

int CatchChallenger::ProtocolParsingInput::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = ProtocolParsing::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 6)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 6;
    }
    return _id;
}

// SIGNAL 0
void CatchChallenger::ProtocolParsingInput::newInputQuery(const quint8 & _t1, const quint8 & _t2)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void CatchChallenger::ProtocolParsingInput::newInputQuery(const quint8 & _t1, const quint16 & _t2, const quint8 & _t3)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)), const_cast<void*>(reinterpret_cast<const void*>(&_t3)) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}
static const uint qt_meta_data_CatchChallenger__ProtocolParsingOutput[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       5,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       2,       // signalCount

 // signals: signature, parameters, type, tag, flags
      65,   40,   39,   39, 0x05,
     132,   95,   39,   39, 0x05,

 // slots: signature, parameters, type, tag, flags
     170,   40,   39,   39, 0x0a,
     199,   95,   39,   39, 0x0a,
     236,   39,   39,   39, 0x08,

       0        // eod
};

static const char qt_meta_stringdata_CatchChallenger__ProtocolParsingOutput[] = {
    "CatchChallenger::ProtocolParsingOutput\0"
    "\0mainCodeType,queryNumber\0"
    "newOutputQuery(quint8,quint8)\0"
    "mainCodeType,subCodeType,queryNumber\0"
    "newOutputQuery(quint8,quint16,quint8)\0"
    "newInputQuery(quint8,quint8)\0"
    "newInputQuery(quint8,quint16,quint8)\0"
    "reset()\0"
};

void CatchChallenger::ProtocolParsingOutput::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        ProtocolParsingOutput *_t = static_cast<ProtocolParsingOutput *>(_o);
        switch (_id) {
        case 0: _t->newOutputQuery((*reinterpret_cast< const quint8(*)>(_a[1])),(*reinterpret_cast< const quint8(*)>(_a[2]))); break;
        case 1: _t->newOutputQuery((*reinterpret_cast< const quint8(*)>(_a[1])),(*reinterpret_cast< const quint16(*)>(_a[2])),(*reinterpret_cast< const quint8(*)>(_a[3]))); break;
        case 2: _t->newInputQuery((*reinterpret_cast< const quint8(*)>(_a[1])),(*reinterpret_cast< const quint8(*)>(_a[2]))); break;
        case 3: _t->newInputQuery((*reinterpret_cast< const quint8(*)>(_a[1])),(*reinterpret_cast< const quint16(*)>(_a[2])),(*reinterpret_cast< const quint8(*)>(_a[3]))); break;
        case 4: _t->reset(); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData CatchChallenger::ProtocolParsingOutput::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject CatchChallenger::ProtocolParsingOutput::staticMetaObject = {
    { &ProtocolParsing::staticMetaObject, qt_meta_stringdata_CatchChallenger__ProtocolParsingOutput,
      qt_meta_data_CatchChallenger__ProtocolParsingOutput, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &CatchChallenger::ProtocolParsingOutput::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *CatchChallenger::ProtocolParsingOutput::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *CatchChallenger::ProtocolParsingOutput::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_CatchChallenger__ProtocolParsingOutput))
        return static_cast<void*>(const_cast< ProtocolParsingOutput*>(this));
    return ProtocolParsing::qt_metacast(_clname);
}

int CatchChallenger::ProtocolParsingOutput::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = ProtocolParsing::qt_metacall(_c, _id, _a);
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
void CatchChallenger::ProtocolParsingOutput::newOutputQuery(const quint8 & _t1, const quint8 & _t2)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void CatchChallenger::ProtocolParsingOutput::newOutputQuery(const quint8 & _t1, const quint16 & _t2, const quint8 & _t3)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)), const_cast<void*>(reinterpret_cast<const void*>(&_t3)) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}
QT_END_MOC_NAMESPACE
