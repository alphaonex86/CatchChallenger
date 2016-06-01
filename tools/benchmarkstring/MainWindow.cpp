#include "MainWindow.h"
#include "ui_MainWindow.h"
#include <QTime>
#include <QDebug>
#include "../../general/base/cpp11addition.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_pushButton_clicked()
{
    QStringList times;
    QString testString="Test string é!";

    {
        QTime time;
        time.restart();
        quint64 index=0;
        while(index<6553500)
        {
            if(testString!=QStringLiteral("test bis"))
            {}
            if(testString==QStringLiteral("test bis"))
            {}
            index++;
        }
        times << QString("Condition with != and ==: %1ms").arg(time.elapsed());
    }
    {
        QTime time;
        time.restart();
        quint64 index=0;
        while(index<6553500)
        {
            item.hasAttribute(QStringLiteral("test bis"));
            index++;
        }
        times << QString("QDomElement::hasAttribute(): %1ms").arg(time.elapsed());
    }
    {
        QTime time;
        time.restart();
        quint64 index=0;
        while(index<6553500)
        {
            testString+QStringLiteral("test bis");
            index++;
        }
        times << QString("concat by +: %1ms").arg(time.elapsed());
    }
    {
        QTime time;
        time.restart();
        quint64 index=0;
        while(index<6553500)
        {
            testString.replace(QStringLiteral("test bis"),testString);
            index++;
        }
        times << QString("replace format to QString: %1ms").arg(time.elapsed());
    }
    {
        QTime time;
        time.restart();
        quint64 index=0;
        while(index<6553500)
        {
            testString.replace(QStringLiteral("test bis"),QStringLiteral("test bis2"));
            index++;
        }
        times << QString("replace format to format: %1ms").arg(time.elapsed());
    }
    {
        QRegularExpression s("a$");
        QTime time;
        time.restart();
        quint64 index=0;
        while(index<6553500)
        {
            testString.replace(s,QStringLiteral("test bis2"));
            index++;
        }
        times << QString("search remplace string to format: %1ms").arg(time.elapsed());
    }
    {
        QRegularExpression s("a$");
        QTime time;
        time.restart();
        quint64 index=0;
        while(index<6553500)
        {
            QStringLiteral("test bis2").replace(s,testString);
            index++;
        }
        times << QString("search remplace format to string: %1ms").arg(time.elapsed());
    }

    qDebug() << QString("QStringLiteral: \n%1\n\n").arg(times.join("\n"));
}

void MainWindow::on_pushButton_2_clicked()
{
    QStringList times;
    QString testString="Test string é!";

    {
        QTime time;
        time.restart();
        quint64 index=0;
        while(index<6553500)
        {
            if(testString!=QLatin1String("test bis"))
            {}
            if(testString==QLatin1String("test bis"))
            {}
            index++;
        }
        times << QString("Condition with != and ==: %1ms").arg(time.elapsed());
    }
    {
        QTime time;
        time.restart();
        quint64 index=0;
        while(index<6553500)
        {
            item.hasAttribute(QLatin1String("test bis"));
            index++;
        }
        times << QString("QDomElement::hasAttribute(): %1ms").arg(time.elapsed());
    }
    {
        QTime time;
        time.restart();
        quint64 index=0;
        while(index<6553500)
        {
            testString+QLatin1String("test bis");
            index++;
        }
        times << QString("concat by +: %1ms").arg(time.elapsed());
    }
    {
        QTime time;
        time.restart();
        quint64 index=0;
        while(index<6553500)
        {
            testString.replace(QLatin1String("test bis"),testString);
            index++;
        }
        times << QString("replace format to QString: %1ms").arg(time.elapsed());
    }
    {
        QTime time;
        time.restart();
        quint64 index=0;
        while(index<6553500)
        {
            testString.replace(QLatin1String("test bis"),QLatin1String("test bis2"));
            index++;
        }
        times << QString("replace format to format: %1ms").arg(time.elapsed());
    }
    {
        QRegularExpression s("a$");
        QTime time;
        time.restart();
        quint64 index=0;
        while(index<6553500)
        {
            testString.replace(s,QLatin1String("test bis2"));
            index++;
        }
        times << QString("search remplace string to format: %1ms").arg(time.elapsed());
    }
    {
        times << QString("search remplace format to string: NA");
    }

    qDebug() << QString("QLatin1String: \n%1").arg(times.join("\n"));
}

void MainWindow::on_pushButton_3_clicked()
{
    QStringList times;
    QString testString="Test string é!";

    {
        QTime time;
        time.restart();
        quint64 index=0;
        while(index<6553500)
        {
            if(testString!=QString("test bis"))
            {}
            if(testString==QString("test bis"))
            {}
            index++;
        }
        times << QString("Condition with != and ==: %1ms").arg(time.elapsed());
    }
    {
        QTime time;
        time.restart();
        quint64 index=0;
        while(index<6553500)
        {
            item.hasAttribute(QString("test bis"));
            index++;
        }
        times << QString("QDomElement::hasAttribute(): %1ms").arg(time.elapsed());
    }
    {
        QTime time;
        time.restart();
        quint64 index=0;
        while(index<6553500)
        {
            testString+QString("test bis");
            index++;
        }
        times << QString("concat by +: %1ms").arg(time.elapsed());
    }
    {
        QTime time;
        time.restart();
        quint64 index=0;
        while(index<6553500)
        {
            testString.replace(QString("test bis"),testString);
            index++;
        }
        times << QString("replace format to QString: %1ms").arg(time.elapsed());
    }
    {
        QTime time;
        time.restart();
        quint64 index=0;
        while(index<6553500)
        {
            testString.replace(QString("test bis"),QString("test bis2"));
            index++;
        }
        times << QString("replace format to format: %1ms").arg(time.elapsed());
    }
    {
        QRegularExpression s("a$");
        QTime time;
        time.restart();
        quint64 index=0;
        while(index<6553500)
        {
            testString.replace(s,QString("test bis2"));
            index++;
        }
        times << QString("search remplace string to format: %1ms").arg(time.elapsed());
    }
    {
        QRegularExpression s("a$");
        QTime time;
        time.restart();
        quint64 index=0;
        while(index<6553500)
        {
            QString("test bis2").replace(s,testString);
            index++;
        }
        times << QString("search remplace format to string: %1ms").arg(time.elapsed());
    }

    qDebug() << QString("QString: \n%1").arg(times.join("\n"));
}

void MainWindow::on_pushButton_4_clicked()
{
    QStringList times;
    QString testString="Test string é!";

    {
        QTime time;
        time.restart();
        quint64 index=0;
        while(index<6553500)
        {
            if(testString!=QLatin1Literal("test bis"))
            {}
            if(testString==QLatin1Literal("test bis"))
            {}
            index++;
        }
        times << QString("Condition with != and ==: %1ms").arg(time.elapsed());
    }
    {
        QTime time;
        time.restart();
        quint64 index=0;
        while(index<6553500)
        {
            item.hasAttribute(QLatin1Literal("test bis"));
            index++;
        }
        times << QString("QDomElement::hasAttribute(): %1ms").arg(time.elapsed());
    }
    {
        QTime time;
        time.restart();
        quint64 index=0;
        while(index<6553500)
        {
            testString+QLatin1Literal("test bis");
            index++;
        }
        times << QString("concat by +: %1ms").arg(time.elapsed());
    }
    {
        QTime time;
        time.restart();
        quint64 index=0;
        while(index<6553500)
        {
            testString.replace(QLatin1Literal("test bis"),testString);
            index++;
        }
        times << QString("replace format to QString: %1ms").arg(time.elapsed());
    }
    {
        QTime time;
        time.restart();
        quint64 index=0;
        while(index<6553500)
        {
            testString.replace(QLatin1Literal("test bis"),QLatin1Literal("test bis2"));
            index++;
        }
        times << QString("replace format to format: %1ms").arg(time.elapsed());
    }
    {
        QRegularExpression s("a$");
        QTime time;
        time.restart();
        quint64 index=0;
        while(index<6553500)
        {
            testString.replace(s,QLatin1Literal("test bis2"));
            index++;
        }
        times << QString("search remplace string to format: %1ms").arg(time.elapsed());
    }
    {
        times << QString("search remplace format to string: NA");
    }

    qDebug() << QString("QLatin1Literal: \n%1").arg(times.join("\n"));
}

void MainWindow::on_pushButton_5_clicked()
{
    QStringList times;
    QString testString="Test string é!";

    {
        QTime time;
        time.restart();
        quint64 index=0;
        while(index<6553500)
        {
            if(testString!="test bis")
            {}
            if(testString=="test bis")
            {}
            index++;
        }
        times << QString("Condition with != and ==: %1ms").arg(time.elapsed());
    }
    {
        QTime time;
        time.restart();
        quint64 index=0;
        while(index<6553500)
        {
            item.hasAttribute("test bis");
            index++;
        }
        times << QString("QDomElement::hasAttribute(): %1ms").arg(time.elapsed());
    }
    {
        QTime time;
        time.restart();
        quint64 index=0;
        while(index<6553500)
        {
            testString+"test bis";
            index++;
        }
        times << QString("concat by +: %1ms").arg(time.elapsed());
    }
    {
        QTime time;
        time.restart();
        quint64 index=0;
        while(index<6553500)
        {
            testString.replace("test bis",testString);
            index++;
        }
        times << QString("replace format to QString: %1ms").arg(time.elapsed());
    }
    {
        QTime time;
        time.restart();
        quint64 index=0;
        while(index<6553500)
        {
            testString.replace("test bis","test bis2");
            index++;
        }
        times << QString("replace format to format: %1ms").arg(time.elapsed());
    }
    {
        QRegularExpression s("a$");
        QTime time;
        time.restart();
        quint64 index=0;
        while(index<6553500)
        {
            testString.replace(s,"test bis2");
            index++;
        }
        times << QString("search remplace string to format: %1ms").arg(time.elapsed());
    }
    {
        times << QString("search remplace format to string: NA");
    }

    qDebug() << QString("Char*: \n%1").arg(times.join("\n"));
}

void MainWindow::on_pushButton_6_clicked()
{
    QStringList times;
    QString testString="Test string é!";
    QString preparedString="sdflgsdfgpml$ùù";

    {
        QTime time;
        time.restart();
        quint64 index=0;
        while(index<6553500)
        {
            if(testString!=preparedString)
            {}
            if(testString==preparedString)
            {}
            index++;
        }
        times << QString("Condition with != and ==: %1ms").arg(time.elapsed());
    }
    {
        QTime time;
        time.restart();
        quint64 index=0;
        while(index<6553500)
        {
            item.hasAttribute(preparedString);
            index++;
        }
        times << QString("QDomElement::hasAttribute(): %1ms").arg(time.elapsed());
    }
    {
        QTime time;
        time.restart();
        quint64 index=0;
        while(index<6553500)
        {
            testString+preparedString;
            index++;
        }
        times << QString("concat by +: %1ms").arg(time.elapsed());
    }
    {
        QTime time;
        time.restart();
        quint64 index=0;
        while(index<6553500)
        {
            testString.replace(preparedString,testString);
            index++;
        }
        times << QString("replace format to QString: %1ms").arg(time.elapsed());
    }
    {
        QTime time;
        time.restart();
        quint64 index=0;
        while(index<6553500)
        {
            testString.replace(preparedString,preparedString);
            index++;
        }
        times << QString("replace format to format: %1ms").arg(time.elapsed());
    }
    {
        QRegularExpression s("a$");
        QTime time;
        time.restart();
        quint64 index=0;
        while(index<6553500)
        {
            testString.replace(s,preparedString);
            index++;
        }
        times << QString("search remplace string to format: %1ms").arg(time.elapsed());
    }
    {
        QRegularExpression s("a$");
        QTime time;
        time.restart();
        quint64 index=0;
        while(index<6553500)
        {
            preparedString.replace(s,testString);
            index++;
        }
        times << QString("search remplace format to string: %1ms").arg(time.elapsed());
    }

    qDebug() << QString("Prepared: \n%1").arg(times.join("\n"));
}

void MainWindow::on_pushButton_7_clicked()
{
    QStringList times;
    std::string testString="Test string é!";

    {
        QTime time;
        time.restart();
        quint64 index=0;
        while(index<6553500)
        {
            if(testString!="test bis")
            {}
            if(testString=="test bis")
            {}
            index++;
        }
        times << QString("Condition with != and ==: %1ms").arg(time.elapsed());
    }
    {
        QTime time;
        time.restart();
        quint64 index=0;
        while(index<6553500)
        {
            item.hasAttribute("test bis");
            index++;
        }
        times << QString("QDomElement::hasAttribute(): %1ms").arg(time.elapsed());
    }
    {
        QTime time;
        time.restart();
        quint64 index=0;
        while(index<6553500)
        {
            testString+"test bis";
            index++;
        }
        times << QString("concat by +: %1ms").arg(time.elapsed());
    }
    {
        QTime time;
        time.restart();
        quint64 index=0;
        while(index<6553500)
        {
            stringreplaceAll(testString,"test bis",testString);
            index++;
        }
        times << QString("replace format to QString: %1ms").arg(time.elapsed());
    }
    {
        QTime time;
        time.restart();
        quint64 index=0;
        while(index<6553500)
        {
            stringreplaceAll(testString,"test bis","test bis");
            index++;
        }
        times << QString("replace format to format: %1ms").arg(time.elapsed());
    }

    qDebug() << QString("char * c++11: \n%1").arg(times.join("\n"));
}

void MainWindow::on_pushButton_8_clicked()
{
    QStringList times;
    std::string testString="Test string é!";

    {
        QTime time;
        time.restart();
        quint64 index=0;
        while(index<6553500)
        {
            if(testString!=std::string("test bis"))
            {}
            if(testString==std::string("test bis"))
            {}
            index++;
        }
        times << QString("Condition with != and ==: %1ms").arg(time.elapsed());
    }
    {
        QTime time;
        time.restart();
        quint64 index=0;
        while(index<6553500)
        {
            testString+std::string("test bis");
            index++;
        }
        times << QString("concat by +: %1ms").arg(time.elapsed());
    }
    {
        QTime time;
        time.restart();
        quint64 index=0;
        while(index<6553500)
        {
            stringreplaceAll(testString,std::string("test bis"),testString);
            index++;
        }
        times << QString("replace format to QString: %1ms").arg(time.elapsed());
    }
    {
        QTime time;
        time.restart();
        quint64 index=0;
        while(index<6553500)
        {
            stringreplaceAll(testString,std::string("test bis"),std::string("test bis"));
            index++;
        }
        times << QString("replace format to format: %1ms").arg(time.elapsed());
    }

    qDebug() << QString("dyna string c++11: \n%1").arg(times.join("\n"));
}

void MainWindow::on_pushButton_9_clicked()
{
    QStringList times;
    std::string testString="Test string é!";
    std::string preparedString="sdflgsdfgpml$ùù";

    {
        QTime time;
        time.restart();
        quint64 index=0;
        while(index<6553500)
        {
            if(testString!=preparedString)
            {}
            if(testString==preparedString)
            {}
            index++;
        }
        times << QString("Condition with != and ==: %1ms").arg(time.elapsed());
    }
    {
        QTime time;
        time.restart();
        quint64 index=0;
        while(index<6553500)
        {
            testString+preparedString;
            index++;
        }
        times << QString("concat by +: %1ms").arg(time.elapsed());
    }
    {
        QTime time;
        time.restart();
        quint64 index=0;
        while(index<6553500)
        {
            stringreplaceAll(testString,preparedString,testString);
            index++;
        }
        times << QString("replace format to QString: %1ms").arg(time.elapsed());
    }
    {
        QTime time;
        time.restart();
        quint64 index=0;
        while(index<6553500)
        {
            stringreplaceAll(testString,preparedString,preparedString);
            index++;
        }
        times << QString("replace format to format: %1ms").arg(time.elapsed());
    }

    qDebug() << QString("Prepared c++11: \n%1").arg(times.join("\n"));
}
