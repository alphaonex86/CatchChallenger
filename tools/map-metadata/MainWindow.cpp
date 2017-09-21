#include "MainWindow.h"
#include "ui_MainWindow.h"

#include <QFileDialog>
#include <QDirIterator>
#include <QDebug>
#include <QProcess>
#include <QSqlQuery>
#include <QVariant>
#include <QSqlError>

#include <iostream>

#include "../../general/base/Map_loader.h"
#include "../../general/base/FacilityLibGeneral.h"

//CREATE TABLE maps(file text primary key, region text, zone text, subzone text, name text, type text, finished int, officialzone int);

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    QSettings settings;
    QString dir=QFileDialog::getExistingDirectory(NULL,"Folder containing region directory",settings.value("mapfolder").toString());
    if(dir.isEmpty())
        abort();
    if(!QString(TMPDATA).endsWith("/"))
        abort();
    if(!dir.endsWith("/"))
        dir+="/";
    settings.setValue("mapfolder",dir);
    QDir().mkpath(TMPDATA);

    /*process.setArguments(QStringList() << "/home/user/Desktop/CatchChallenger/datapack-pkmn/map/main/gen4/map/");
    process.setProgram("/home/user/Desktop/CatchChallenger/tools/build-map2png-Desktop-Debug/map2png");
    process.setWorkingDirectory("/tmp/map-metadata/");
    process.start();
    process.waitForFinished(999999999);
    std::cerr << process.errorString().toStdString() << std::endl;
    std::cout << process.readAll().toStdString() << std::endl;
    if(process.exitCode()!=0)
        std::cerr << "Process exit code: " << process.exitCode() << std::endl;*/

    QString path=QCoreApplication::applicationDirPath()+"/changes.db";
    QFile destinationFile(path);
    if(!destinationFile.exists())
    {
        if(QFile::copy(":/changes.db",path))
        {
            if(!destinationFile.setPermissions(destinationFile.permissions() | QFileDevice::WriteOwner | QFileDevice::WriteUser))
                std::cerr << "Unable to set db permissions" << std::endl;
        }
        else
            std::cerr << "Unable to copy the :/changes.db" << std::endl;
    }
    m_db = QSqlDatabase::addDatabase("QSQLITE");
    m_db.setDatabaseName(path);

    if (!m_db.open())
    {
        qDebug() << "Error: connection with database fail";
        abort();
    }

    QHash<QString,MapContent> db_finishedFile;
    QHash<QString,MapContent> db_not_finishedFile;
    QSqlQuery query;
    if(!query.exec("SELECT file, region, zone, subzone, name, type, finished FROM maps"))
    {
        qDebug() << query.lastError().text();
        abort();
    }
    while(query.next())
    {
        QString file = query.value(0).toString();
        MapContent mapContent;
        mapContent.region=query.value(1).toString();
        mapContent.zone=query.value(2).toString();
        mapContent.subzone=query.value(3).toString();
        mapContent.name=query.value(4).toString();
        mapContent.type=query.value(5).toString();
        mapContent.officialzone=true;
        if(query.value(1).toInt()>0)
            db_finishedFile[file]=mapContent;
        else
            db_not_finishedFile[file]=mapContent;
    }

    {
        QDirIterator it(dir,QDirIterator::Subdirectories);
        QRegularExpression regex("\\.tmx$");
        QRegularExpression regexRemove("^/");
        while (it.hasNext()) {
            QString element=it.next();
            if(element.contains(regex))
            {
                QString sortPath=element;
                sortPath.remove(0,dir.size());
                sortPath.remove(regexRemove);
                QString pngDest=TMPDATA+sortPath;
                pngDest.replace(".tmx",".png");
                //std::cout << sortPath.toStdString() << " -> " << pngDest.toStdString() << std::endl;

                const QStringList elementList=sortPath.split("/");
                if(elementList.size()>=2 && elementList.size()<=3)
                {
                    QString file = sortPath;
                    if(db_finishedFile.contains(file))
                        finishedFile[file]=db_finishedFile.value(file);
                    else if(db_not_finishedFile.contains(file))
                        not_finishedFile[file]=db_not_finishedFile.value(file);
                    else
                    {
                        MapContent mapContent;
                        mapContent.region=elementList.at(0);
                        mapContent.zone=elementList.at(1);
                        mapContent.zone.replace(".tmx","");
                        if(element.size()==3)
                        {
                            mapContent.subzone=elementList.at(2);
                            mapContent.subzone.replace(".tmx","");
                        }
                        mapContent.officialzone=true;
                        //get from xml
                        QDomDocument domDocument;
                        QString xmlpath=element;
                        xmlpath.replace(".tmx",".xml");
                        QFile xmlfile(xmlpath);
                        if (xmlfile.open(QIODevice::ReadOnly))
                        {
                            if (!domDocument.setContent(&xmlfile)) {
                                xmlfile.close();
                                return;
                            }
                            xmlfile.close();

                            const QDomElement root = domDocument.documentElement();
                            if(root.tagName()=="map")
                            {
                                //load the content
                                const QDomElement nameItem = root.firstChildElement("name");
                                if(!nameItem.isNull())
                                    mapContent.name=nameItem.text();

                                if(root.hasAttribute("type"))
                                    mapContent.type=root.attribute("type");
                                mapContent.officialzone=root.hasAttribute("zone");
                            }
                        }

                        not_finishedFile[file]=mapContent;
                        //insert into database
                        QSqlQuery query;
                        query.prepare("INSERT INTO maps (file, region, zone, subzone, name, type, finished) "
                                      "VALUES (:file, :region, :zone, :subzone, :name, :type, :finished)");
                        query.bindValue(":file", file);
                        query.bindValue(":region", mapContent.region);
                        query.bindValue(":zone", mapContent.zone);
                        query.bindValue(":subzone", mapContent.subzone);
                        query.bindValue(":name", mapContent.name);
                        query.bindValue(":type", mapContent.type);
                        query.bindValue(":finished", 0);
                        if(!query.exec())
                        {
                            qDebug() << query.lastError().text();
                            abort();
                        }
                    }
                }
            }
        }
    }
    preload_the_map(dir.toStdString());

    displayNewNotFinishedMap();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_getAnotherUnfinishedMap_clicked()
{
    displayNewNotFinishedMap();
}

void MainWindow::displayNewNotFinishedMap()
{
    const QList<QString> keys=not_finishedFile.keys();
    QString key=keys.at(rand()%keys.size());
    {
        const QString &file=key;
        const MapContent &mapContent=not_finishedFile.value(key);
        ui->region->setText(mapContent.region);
        ui->zone->setText(mapContent.zone);
        ui->subzone->setText(mapContent.subzone);
        ui->name->setText(mapContent.name);
        ui->type->setCurrentIndex(ui->type->findText(mapContent.type));
        ui->checkBox->setChecked(mapContent.officialzone);

        QString pngDest=TMPDATA+file;
        pngDest.replace(".tmx",".png");
        ui->currentMapPreview->setPixmap(QPixmap(pngDest));

        {
            QLayoutItem *item = NULL;
            while ((item = ui->horizontalLayout_3->takeAt(0)) != 0) {
            delete item->widget();
            }
            //ui->horizontalLayout_3->widget()->deleteLater();
            //delete ui->scrollArea;
            QString fileSort=file;
            fileSort.replace(".tmx","");
            const std::string &fileSortStd=fileSort.toStdString();
            if(map_list.find(fileSortStd)!=map_list.cend())
            {
                const CatchChallenger::MapServer * const mapPointer=map_list.at(fileSortStd);

                unsigned int mapIndex=0;
                while(mapIndex<mapPointer->linked_map.size())
                {
                    const CatchChallenger::CommonMap * const map=mapPointer->linked_map.at(mapIndex);
                    bool haveContent=true;
                    MapContent mapContent;
                    if(not_finishedFile.contains(QString::fromStdString(map->map_file)+".tmx"))
                        mapContent=not_finishedFile.value(QString::fromStdString(map->map_file)+".tmx");
                    else if(finishedFile.contains(QString::fromStdString(map->map_file)+".tmx"))
                        mapContent=finishedFile.value(QString::fromStdString(map->map_file)+".tmx");
                    else
                        haveContent=false;

                    if(haveContent)
                    {
                        QScrollArea *scrollArea = new QScrollArea(ui->groupBox);
                        scrollArea->setMinimumSize(QSize(350, 0));
                        scrollArea->setWidgetResizable(true);
                        QWidget * scrollAreaWidgetContents = new QWidget();
                        scrollAreaWidgetContents->setGeometry(QRect(0, 0, 892, 337));
                        QHBoxLayout * horizontalLayout_2 = new QHBoxLayout(scrollAreaWidgetContents);
                        horizontalLayout_2->setSpacing(6);
                        horizontalLayout_2->setContentsMargins(11, 11, 11, 11);
                        QGroupBox * groupBox_4 = new QGroupBox(scrollAreaWidgetContents);
                        groupBox_4->setMaximumSize(QSize(300, 300));
                        QVBoxLayout * verticalLayout = new QVBoxLayout(groupBox_4);
                        verticalLayout->setSpacing(6);
                        verticalLayout->setContentsMargins(11, 11, 11, 11);
                        QLabel * label_6 = new QLabel(groupBox_4);

                        label_6->setPixmap(QPixmap(TMPDATA+QString::fromStdString(map->map_file)+".png"));

                        label_6->setScaledContents(true);
                        verticalLayout->addWidget(label_6);
                        QLabel * label_7 = new QLabel(groupBox_4);
                        verticalLayout->addWidget(label_7);
                        QLabel * label_8 = new QLabel(groupBox_4);
                        verticalLayout->addWidget(label_8);
                        QLabel * label_9 = new QLabel(groupBox_4);
                        verticalLayout->addWidget(label_9);
                        horizontalLayout_2->addWidget(groupBox_4);
                        scrollArea->setWidget(scrollAreaWidgetContents);

                        groupBox_4->setTitle(mapContent.name);
                        label_7->setText("Region: <a href=\""+mapContent.region+"\">"+mapContent.region+"</a>");
                        label_8->setText("Zone: <a href=\""+mapContent.zone+"\">"+mapContent.zone+"</a>");
                        label_9->setText("Sub zone: <a href=\""+mapContent.subzone+"\">"+mapContent.subzone+"</a>");

                        ui->horizontalLayout_3->addWidget(scrollArea);
                        ui->scrollArea=scrollArea;
                    }

                    mapIndex++;
                }
            }
        }

        selectedMap=file;
    }
}
