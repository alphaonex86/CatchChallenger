/********************************************************************************
** Form generated from reading UI file 'mainwindow.ui'
**
** Created: Thu Apr 11 17:51:14 2013
**      by: Qt User Interface Compiler version 4.8.4
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QCheckBox>
#include <QtGui/QFrame>
#include <QtGui/QHBoxLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QLineEdit>
#include <QtGui/QMainWindow>
#include <QtGui/QPushButton>
#include <QtGui/QSpacerItem>
#include <QtGui/QStackedWidget>
#include <QtGui/QVBoxLayout>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QWidget *centralWidget;
    QHBoxLayout *horizontalLayout;
    QStackedWidget *stackedWidget;
    QWidget *page_login;
    QVBoxLayout *verticalLayout_3;
    QSpacerItem *verticalSpacer_login_2;
    QHBoxLayout *horizontalLayout_login_1;
    QSpacerItem *horizontalSpacer_login_8;
    QFrame *frame_login_2;
    QHBoxLayout *horizontalLayout_6;
    QFrame *frame_login;
    QVBoxLayout *verticalLayout;
    QHBoxLayout *horizontalLayout_login_2;
    QLabel *label_login_login;
    QSpacerItem *horizontalSpacer_login_7;
    QLineEdit *lineEditLogin;
    QHBoxLayout *horizontalLayout_login_3;
    QLabel *label_login_pass;
    QSpacerItem *horizontalSpacer_login_2;
    QLineEdit *lineEditPass;
    QCheckBox *checkBoxRememberPassword;
    QSpacerItem *verticalSpacer_login;
    QHBoxLayout *horizontalLayout_login_4;
    QSpacerItem *horizontalSpacer_login_3;
    QPushButton *pushButtonTryLogin;
    QSpacerItem *horizontalSpacer_login_5;
    QLabel *label_login_register;
    QSpacerItem *horizontalSpacer_login_6;
    QLabel *label_login_website;
    QSpacerItem *horizontalSpacer_login_4;
    QSpacerItem *horizontalSpacer_login_9;
    QSpacerItem *verticalSpacer_login_3;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName(QString::fromUtf8("MainWindow"));
        MainWindow->resize(800, 600);
        MainWindow->setMinimumSize(QSize(800, 600));
        MainWindow->setMaximumSize(QSize(800, 600));
        QIcon icon;
        icon.addFile(QString::fromUtf8(":/images/catchchallenger.png"), QSize(), QIcon::Normal, QIcon::Off);
        MainWindow->setWindowIcon(icon);
        centralWidget = new QWidget(MainWindow);
        centralWidget->setObjectName(QString::fromUtf8("centralWidget"));
        horizontalLayout = new QHBoxLayout(centralWidget);
        horizontalLayout->setSpacing(0);
        horizontalLayout->setContentsMargins(0, 0, 0, 0);
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        stackedWidget = new QStackedWidget(centralWidget);
        stackedWidget->setObjectName(QString::fromUtf8("stackedWidget"));
        page_login = new QWidget();
        page_login->setObjectName(QString::fromUtf8("page_login"));
        page_login->setStyleSheet(QString::fromUtf8("#page_login{background-image: url(:/images/background.png);}"));
        verticalLayout_3 = new QVBoxLayout(page_login);
        verticalLayout_3->setSpacing(0);
        verticalLayout_3->setContentsMargins(0, 0, 0, 0);
        verticalLayout_3->setObjectName(QString::fromUtf8("verticalLayout_3"));
        verticalSpacer_login_2 = new QSpacerItem(20, 197, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout_3->addItem(verticalSpacer_login_2);

        horizontalLayout_login_1 = new QHBoxLayout();
        horizontalLayout_login_1->setSpacing(6);
        horizontalLayout_login_1->setObjectName(QString::fromUtf8("horizontalLayout_login_1"));
        horizontalSpacer_login_8 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_login_1->addItem(horizontalSpacer_login_8);

        frame_login_2 = new QFrame(page_login);
        frame_login_2->setObjectName(QString::fromUtf8("frame_login_2"));
        frame_login_2->setMinimumSize(QSize(431, 191));
        frame_login_2->setMaximumSize(QSize(431, 191));
        frame_login_2->setStyleSheet(QString::fromUtf8("#frame_login_2{background-image: url(:/images/background-dialog.png);}"));
        horizontalLayout_6 = new QHBoxLayout(frame_login_2);
        horizontalLayout_6->setSpacing(0);
        horizontalLayout_6->setContentsMargins(0, 0, 0, 0);
        horizontalLayout_6->setObjectName(QString::fromUtf8("horizontalLayout_6"));
        frame_login = new QFrame(frame_login_2);
        frame_login->setObjectName(QString::fromUtf8("frame_login"));
        frame_login->setMinimumSize(QSize(411, 161));
        frame_login->setMaximumSize(QSize(411, 161));
        frame_login->setFrameShape(QFrame::NoFrame);
        frame_login->setFrameShadow(QFrame::Raised);
        verticalLayout = new QVBoxLayout(frame_login);
        verticalLayout->setSpacing(6);
        verticalLayout->setContentsMargins(11, 11, 11, 11);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        horizontalLayout_login_2 = new QHBoxLayout();
        horizontalLayout_login_2->setSpacing(6);
        horizontalLayout_login_2->setObjectName(QString::fromUtf8("horizontalLayout_login_2"));
        label_login_login = new QLabel(frame_login);
        label_login_login->setObjectName(QString::fromUtf8("label_login_login"));

        horizontalLayout_login_2->addWidget(label_login_login);

        horizontalSpacer_login_7 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_login_2->addItem(horizontalSpacer_login_7);

        lineEditLogin = new QLineEdit(frame_login);
        lineEditLogin->setObjectName(QString::fromUtf8("lineEditLogin"));
        lineEditLogin->setMinimumSize(QSize(180, 0));
        lineEditLogin->setMaximumSize(QSize(180, 16777215));
        lineEditLogin->setStyleSheet(QString::fromUtf8("background-image: url(:/images/light-white.png);\n"
"border-image: url(:/images/empty.png);"));

        horizontalLayout_login_2->addWidget(lineEditLogin);


        verticalLayout->addLayout(horizontalLayout_login_2);

        horizontalLayout_login_3 = new QHBoxLayout();
        horizontalLayout_login_3->setSpacing(6);
        horizontalLayout_login_3->setObjectName(QString::fromUtf8("horizontalLayout_login_3"));
        label_login_pass = new QLabel(frame_login);
        label_login_pass->setObjectName(QString::fromUtf8("label_login_pass"));

        horizontalLayout_login_3->addWidget(label_login_pass);

        horizontalSpacer_login_2 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_login_3->addItem(horizontalSpacer_login_2);

        lineEditPass = new QLineEdit(frame_login);
        lineEditPass->setObjectName(QString::fromUtf8("lineEditPass"));
        lineEditPass->setMinimumSize(QSize(180, 0));
        lineEditPass->setMaximumSize(QSize(180, 16777215));
        lineEditPass->setStyleSheet(QString::fromUtf8("background-image: url(:/images/light-white.png);\n"
"border-image: url(:/images/empty.png);"));
        lineEditPass->setEchoMode(QLineEdit::Password);

        horizontalLayout_login_3->addWidget(lineEditPass);


        verticalLayout->addLayout(horizontalLayout_login_3);

        checkBoxRememberPassword = new QCheckBox(frame_login);
        checkBoxRememberPassword->setObjectName(QString::fromUtf8("checkBoxRememberPassword"));

        verticalLayout->addWidget(checkBoxRememberPassword);

        verticalSpacer_login = new QSpacerItem(20, 72, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout->addItem(verticalSpacer_login);

        horizontalLayout_login_4 = new QHBoxLayout();
        horizontalLayout_login_4->setSpacing(6);
        horizontalLayout_login_4->setObjectName(QString::fromUtf8("horizontalLayout_login_4"));
        horizontalSpacer_login_3 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_login_4->addItem(horizontalSpacer_login_3);

        pushButtonTryLogin = new QPushButton(frame_login);
        pushButtonTryLogin->setObjectName(QString::fromUtf8("pushButtonTryLogin"));

        horizontalLayout_login_4->addWidget(pushButtonTryLogin);

        horizontalSpacer_login_5 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_login_4->addItem(horizontalSpacer_login_5);

        label_login_register = new QLabel(frame_login);
        label_login_register->setObjectName(QString::fromUtf8("label_login_register"));
        label_login_register->setStyleSheet(QString::fromUtf8("background-image: url(:/images/light-white.png);\n"
"padding: 0 5px;"));
        label_login_register->setOpenExternalLinks(true);

        horizontalLayout_login_4->addWidget(label_login_register);

        horizontalSpacer_login_6 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_login_4->addItem(horizontalSpacer_login_6);

        label_login_website = new QLabel(frame_login);
        label_login_website->setObjectName(QString::fromUtf8("label_login_website"));
        label_login_website->setStyleSheet(QString::fromUtf8("background-image: url(:/images/light-white.png);\n"
"padding: 0 5px;"));
        label_login_website->setOpenExternalLinks(true);

        horizontalLayout_login_4->addWidget(label_login_website);

        horizontalSpacer_login_4 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_login_4->addItem(horizontalSpacer_login_4);


        verticalLayout->addLayout(horizontalLayout_login_4);


        horizontalLayout_6->addWidget(frame_login);


        horizontalLayout_login_1->addWidget(frame_login_2);

        horizontalSpacer_login_9 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_login_1->addItem(horizontalSpacer_login_9);


        verticalLayout_3->addLayout(horizontalLayout_login_1);

        verticalSpacer_login_3 = new QSpacerItem(20, 197, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout_3->addItem(verticalSpacer_login_3);

        stackedWidget->addWidget(page_login);

        horizontalLayout->addWidget(stackedWidget);

        MainWindow->setCentralWidget(centralWidget);

        retranslateUi(MainWindow);

        stackedWidget->setCurrentIndex(0);


        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QApplication::translate("MainWindow", "CatchChallenger", 0, QApplication::UnicodeUTF8));
        label_login_login->setText(QApplication::translate("MainWindow", "Login", 0, QApplication::UnicodeUTF8));
        label_login_pass->setText(QApplication::translate("MainWindow", "Password:", 0, QApplication::UnicodeUTF8));
        checkBoxRememberPassword->setText(QApplication::translate("MainWindow", "Remember the password", 0, QApplication::UnicodeUTF8));
        pushButtonTryLogin->setText(QApplication::translate("MainWindow", " Ok ", 0, QApplication::UnicodeUTF8));
        label_login_register->setText(QApplication::translate("MainWindow", "<a href=\"http://catchchallenger.first-world.info/register.html\"><span style=\"text-decoration:underline;color:#0057ae;\">Register</span></a>", 0, QApplication::UnicodeUTF8));
        label_login_website->setText(QApplication::translate("MainWindow", "<a href=\"http://catchchallenger.first-world.info/\"><span style=\"text-decoration:underline;color:#0057ae;\">Web site</span></a>", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
