/********************************************************************************
** Form generated from reading UI file 'MainWindow.ui'
**
** Created by: Qt User Interface Compiler version 5.13.1
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QWidget *centralWidget;
    QVBoxLayout *verticalLayout;
    QSpinBox *SampleRate;
    QSpinBox *ChannelCount;
    QSpinBox *SampleSize;
    QLineEdit *Codec;
    QComboBox *ByteOrder;
    QComboBox *SampleType;
    QPushButton *convert;
    QPushButton *playDirectly;
    QLabel *codeclist;
    QStatusBar *statusBar;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName(QString::fromUtf8("MainWindow"));
        MainWindow->resize(400, 300);
        centralWidget = new QWidget(MainWindow);
        centralWidget->setObjectName(QString::fromUtf8("centralWidget"));
        verticalLayout = new QVBoxLayout(centralWidget);
        verticalLayout->setSpacing(6);
        verticalLayout->setContentsMargins(11, 11, 11, 11);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        SampleRate = new QSpinBox(centralWidget);
        SampleRate->setObjectName(QString::fromUtf8("SampleRate"));
        SampleRate->setMaximum(9999999);
        SampleRate->setValue(48000);

        verticalLayout->addWidget(SampleRate);

        ChannelCount = new QSpinBox(centralWidget);
        ChannelCount->setObjectName(QString::fromUtf8("ChannelCount"));
        ChannelCount->setValue(2);

        verticalLayout->addWidget(ChannelCount);

        SampleSize = new QSpinBox(centralWidget);
        SampleSize->setObjectName(QString::fromUtf8("SampleSize"));
        SampleSize->setValue(16);

        verticalLayout->addWidget(SampleSize);

        Codec = new QLineEdit(centralWidget);
        Codec->setObjectName(QString::fromUtf8("Codec"));

        verticalLayout->addWidget(Codec);

        ByteOrder = new QComboBox(centralWidget);
        ByteOrder->addItem(QString());
        ByteOrder->addItem(QString());
        ByteOrder->setObjectName(QString::fromUtf8("ByteOrder"));

        verticalLayout->addWidget(ByteOrder);

        SampleType = new QComboBox(centralWidget);
        SampleType->addItem(QString());
        SampleType->addItem(QString());
        SampleType->addItem(QString());
        SampleType->addItem(QString());
        SampleType->setObjectName(QString::fromUtf8("SampleType"));

        verticalLayout->addWidget(SampleType);

        convert = new QPushButton(centralWidget);
        convert->setObjectName(QString::fromUtf8("convert"));

        verticalLayout->addWidget(convert);

        playDirectly = new QPushButton(centralWidget);
        playDirectly->setObjectName(QString::fromUtf8("playDirectly"));

        verticalLayout->addWidget(playDirectly);

        codeclist = new QLabel(centralWidget);
        codeclist->setObjectName(QString::fromUtf8("codeclist"));

        verticalLayout->addWidget(codeclist);

        MainWindow->setCentralWidget(centralWidget);
        statusBar = new QStatusBar(MainWindow);
        statusBar->setObjectName(QString::fromUtf8("statusBar"));
        MainWindow->setStatusBar(statusBar);

        retranslateUi(MainWindow);

        ByteOrder->setCurrentIndex(1);
        SampleType->setCurrentIndex(1);


        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QCoreApplication::translate("MainWindow", "MainWindow", nullptr));
        Codec->setText(QCoreApplication::translate("MainWindow", "audio/pcm", nullptr));
        ByteOrder->setItemText(0, QCoreApplication::translate("MainWindow", "BigEndian", nullptr));
        ByteOrder->setItemText(1, QCoreApplication::translate("MainWindow", "LittleEndian", nullptr));

        SampleType->setItemText(0, QCoreApplication::translate("MainWindow", "Unknown", nullptr));
        SampleType->setItemText(1, QCoreApplication::translate("MainWindow", "SignedInt", nullptr));
        SampleType->setItemText(2, QCoreApplication::translate("MainWindow", "UnSignedInt", nullptr));
        SampleType->setItemText(3, QCoreApplication::translate("MainWindow", "Float", nullptr));

        convert->setText(QCoreApplication::translate("MainWindow", "Convert", nullptr));
        playDirectly->setText(QCoreApplication::translate("MainWindow", "Play directly", nullptr));
        codeclist->setText(QCoreApplication::translate("MainWindow", "Codec", nullptr));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
