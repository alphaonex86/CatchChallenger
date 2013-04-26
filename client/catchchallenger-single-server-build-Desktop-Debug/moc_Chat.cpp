/****************************************************************************
** Meta object code from reading C++ file 'Chat.h'
**
** Created: Thu Apr 11 17:51:26 2013
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.4)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../base/interface/Chat.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'Chat.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.4. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_CatchChallenger__Chat[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       9,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: signature, parameters, type, tag, flags
      38,   23,   22,   22, 0x0a,
     117,   90,   22,   22, 0x0a,
     212,  204,   22,   22, 0x08,
     244,   22,   22,   22, 0x08,
     279,   22,   22,   22, 0x08,
     302,   22,   22,   22, 0x08,
     329,  324,  316,   22, 0x08,
     353,  324,  316,   22, 0x08,
     378,  372,   22,   22, 0x08,

       0        // eod
};

static const char qt_meta_stringdata_CatchChallenger__Chat[] = {
    "CatchChallenger::Chat\0\0chat_type,text\0"
    "new_system_text(CatchChallenger::Chat_type,QString)\0"
    "chat_type,text,pseudo,type\0"
    "new_chat_text(CatchChallenger::Chat_type,QString,QString,CatchChalleng"
    "er::Player_type)\0"
    "checked\0on_pushButtonChat_toggled(bool)\0"
    "lineEdit_chat_text_returnPressed()\0"
    "removeNumberForFlood()\0update_chat()\0"
    "QString\0text\0toHtmlEntities(QString)\0"
    "toSmilies(QString)\0index\0"
    "comboBox_chat_type_currentIndexChanged(int)\0"
};

void CatchChallenger::Chat::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        Chat *_t = static_cast<Chat *>(_o);
        switch (_id) {
        case 0: _t->new_system_text((*reinterpret_cast< CatchChallenger::Chat_type(*)>(_a[1])),(*reinterpret_cast< QString(*)>(_a[2]))); break;
        case 1: _t->new_chat_text((*reinterpret_cast< CatchChallenger::Chat_type(*)>(_a[1])),(*reinterpret_cast< QString(*)>(_a[2])),(*reinterpret_cast< QString(*)>(_a[3])),(*reinterpret_cast< CatchChallenger::Player_type(*)>(_a[4]))); break;
        case 2: _t->on_pushButtonChat_toggled((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 3: _t->lineEdit_chat_text_returnPressed(); break;
        case 4: _t->removeNumberForFlood(); break;
        case 5: _t->update_chat(); break;
        case 6: { QString _r = _t->toHtmlEntities((*reinterpret_cast< QString(*)>(_a[1])));
            if (_a[0]) *reinterpret_cast< QString*>(_a[0]) = _r; }  break;
        case 7: { QString _r = _t->toSmilies((*reinterpret_cast< QString(*)>(_a[1])));
            if (_a[0]) *reinterpret_cast< QString*>(_a[0]) = _r; }  break;
        case 8: _t->comboBox_chat_type_currentIndexChanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData CatchChallenger::Chat::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject CatchChallenger::Chat::staticMetaObject = {
    { &QWidget::staticMetaObject, qt_meta_stringdata_CatchChallenger__Chat,
      qt_meta_data_CatchChallenger__Chat, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &CatchChallenger::Chat::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *CatchChallenger::Chat::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *CatchChallenger::Chat::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_CatchChallenger__Chat))
        return static_cast<void*>(const_cast< Chat*>(this));
    return QWidget::qt_metacast(_clname);
}

int CatchChallenger::Chat::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 9)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 9;
    }
    return _id;
}
QT_END_MOC_NAMESPACE
