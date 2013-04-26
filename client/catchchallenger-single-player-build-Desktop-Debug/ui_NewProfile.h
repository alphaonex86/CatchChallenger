/********************************************************************************
** Form generated from reading UI file 'NewProfile.ui'
**
** Created: Fri Apr 26 14:18:41 2013
**      by: Qt User Interface Compiler version 4.8.4
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_NEWPROFILE_H
#define UI_NEWPROFILE_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QComboBox>
#include <QtGui/QDialog>
#include <QtGui/QGroupBox>
#include <QtGui/QHBoxLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QPushButton>
#include <QtGui/QSpacerItem>
#include <QtGui/QVBoxLayout>

QT_BEGIN_NAMESPACE

class Ui_NewProfile
{
public:
    QVBoxLayout *verticalLayout;
    QHBoxLayout *horizontalLayout_2;
    QSpacerItem *horizontalSpacer_3;
    QComboBox *comboBox;
    QSpacerItem *horizontalSpacer_4;
    QGroupBox *groupBox;
    QVBoxLayout *verticalLayout_2;
    QLabel *description;
    QSpacerItem *verticalSpacer;
    QHBoxLayout *horizontalLayout;
    QSpacerItem *horizontalSpacer;
    QPushButton *ok;
    QPushButton *pushButton_2;
    QSpacerItem *horizontalSpacer_2;

    void setupUi(QDialog *NewProfile)
    {
        if (NewProfile->objectName().isEmpty())
            NewProfile->setObjectName(QString::fromUtf8("NewProfile"));
        NewProfile->resize(334, 222);
        NewProfile->setStyleSheet(QString::fromUtf8("#NewProfile{background-image: url(:/images/background.png);}\n"
""));
        verticalLayout = new QVBoxLayout(NewProfile);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        horizontalLayout_2 = new QHBoxLayout();
        horizontalLayout_2->setObjectName(QString::fromUtf8("horizontalLayout_2"));
        horizontalSpacer_3 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_2->addItem(horizontalSpacer_3);

        comboBox = new QComboBox(NewProfile);
        comboBox->setObjectName(QString::fromUtf8("comboBox"));

        horizontalLayout_2->addWidget(comboBox);

        horizontalSpacer_4 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_2->addItem(horizontalSpacer_4);


        verticalLayout->addLayout(horizontalLayout_2);

        groupBox = new QGroupBox(NewProfile);
        groupBox->setObjectName(QString::fromUtf8("groupBox"));
        groupBox->setStyleSheet(QString::fromUtf8("#groupBox{background-image: url(:/images/light-white.png);}\n"
""));
        verticalLayout_2 = new QVBoxLayout(groupBox);
        verticalLayout_2->setObjectName(QString::fromUtf8("verticalLayout_2"));
        description = new QLabel(groupBox);
        description->setObjectName(QString::fromUtf8("description"));

        verticalLayout_2->addWidget(description);

        verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout_2->addItem(verticalSpacer);


        verticalLayout->addWidget(groupBox);

        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout->addItem(horizontalSpacer);

        ok = new QPushButton(NewProfile);
        ok->setObjectName(QString::fromUtf8("ok"));
        ok->setEnabled(false);

        horizontalLayout->addWidget(ok);

        pushButton_2 = new QPushButton(NewProfile);
        pushButton_2->setObjectName(QString::fromUtf8("pushButton_2"));

        horizontalLayout->addWidget(pushButton_2);

        horizontalSpacer_2 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout->addItem(horizontalSpacer_2);


        verticalLayout->addLayout(horizontalLayout);


        retranslateUi(NewProfile);

        QMetaObject::connectSlotsByName(NewProfile);
    } // setupUi

    void retranslateUi(QDialog *NewProfile)
    {
        NewProfile->setWindowTitle(QApplication::translate("NewProfile", "New profile", 0, QApplication::UnicodeUTF8));
        groupBox->setTitle(QApplication::translate("NewProfile", "Description", 0, QApplication::UnicodeUTF8));
        ok->setText(QApplication::translate("NewProfile", "ok", 0, QApplication::UnicodeUTF8));
        pushButton_2->setText(QApplication::translate("NewProfile", "cancel", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class NewProfile: public Ui_NewProfile {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_NEWPROFILE_H
