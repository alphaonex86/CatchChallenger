#include "MainWindow.h"
#include "ui_MainWindow.h"

#include <QFileDialog>
#include <QDirIterator>
#include <QDebug>
#include <QProcess>
#include <QSqlQuery>
#include <QVariant>
#include <QSqlError>
#include <QStringList>
#include <QtAlgorithms>   // for qSort()
#include <QStringList>

#include <iostream>

#include "../../general/base/Map_loader.h"
#include "../../general/base/FacilityLibGeneral.h"

//CREATE TABLE maps(file text primary key, region text, zone text, subzone text, name text, type text, finished int, officialzone int);

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    canUpdate=false;
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

    QDir dir2("/tmp/map-metadata/");
    dir2.setFilter(QDir::Files | QDir::Dirs | QDir::Hidden | QDir::NoSymLinks | QDir::NoDotAndDotDot);
    QFileInfoList list = dir2.entryInfoList();
    if(!QDir("/tmp/map-metadata/").exists() || list.size()==0)
    {
        process.setArguments(QStringList() << "/home/user/Desktop/CatchChallenger/datapack-pkmn/map/main/gen4/map/");
        process.setProgram("/home/user/Desktop/CatchChallenger/tools/build-map2png-Desktop-Debug/map2png");
        process.setWorkingDirectory("/tmp/map-metadata/");
        process.start();
        process.waitForFinished(999999999);
        std::cerr << process.errorString().toStdString() << std::endl;
        std::cout << process.readAll().toStdString() << std::endl;
        if(process.exitCode()!=0)
            std::cerr << "Process exit code: " << process.exitCode() << std::endl;
    }

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
        if(mapContent.region.isEmpty())
            abort();
        mapContent.officialzone=query.value(7).toInt()>0;
        if(query.value(6).toInt()>0)
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
                        if(mapContent.region.isEmpty())
                            abort();
                        mapContent.zone=elementList.at(1);
                        mapContent.zone.replace(".tmx","");
                        if(elementList.size()==3)
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

                            const tinyxml2::XMLElement root = domDocument.RootElement();
                            if(root.tagName()=="map")
                            {
                                //load the content
                                const tinyxml2::XMLElement nameItem = root.FirstChildElement("name");
                                if(!nameItem.isNull())
                                    mapContent.name=nameItem.text();

                                if(root.hasAttribute("type"))
                                    mapContent.type=root.attribute("type");
                                if(root.hasAttribute("zone"))
                                {
                                    mapContent.officialzone=true;
                                    mapContent.zone=root.attribute("zone");
                                }
                                else
                                {
                                    if(mapContent.name.startsWith("Route "))
                                    {
                                        mapContent.officialzone=false;
                                        mapContent.zone="route";
                                    }
                                    else
                                        mapContent.officialzone=true;
                                }
                            }
                        }

                        not_finishedFile[file]=mapContent;
                        //insert into database
                        QSqlQuery query;
                        if(!query.prepare("INSERT INTO maps (file, region, zone, subzone, name, type, finished) "
                                      "VALUES (:file, :region, :zone, :subzone, :name, :type, :finished)"))
                        {
                            qDebug() << query.lastError().text();
                            abort();
                        }
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
    for(auto& n:map_list)
    {
        CatchChallenger::MapServer * map=n.second;
        unsigned int index=0;
        while(index<map->linked_map.size())
        {
            CatchChallenger::CommonMap * const newMap=map->linked_map.at(index);
            if(!vectorcontainsAtLeastOne(newMap->linked_map,static_cast<CatchChallenger::CommonMap *>(map)))
                newMap->linked_map.push_back(map);
            index++;
        }
    }

    displayNewNotFinishedMap();
    updateProgressLabel();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_getAnotherUnfinishedMap_clicked()
{
    displayNewNotFinishedMap();
}

void MainWindow::displayNewNotFinishedMap(const bool useNearMap)
{
    canUpdate=false;
    QString key;

    bool havePreselectedMap=true;
    MapContent mapContent;
    if(not_finishedFile.contains(selectedMap))
        mapContent=not_finishedFile.value(selectedMap);
    else if(finishedFile.contains(selectedMap))
        mapContent=finishedFile.value(selectedMap);
    else
        havePreselectedMap=false;
    if(havePreselectedMap && useNearMap)
    {
        selectedMap.remove(".tmx");
        const CatchChallenger::MapServer * const mapPointer=map_list.at(selectedMap.toStdString());
        unsigned int mapIndex=0;
        while(mapIndex<mapPointer->linked_map.size())
        {
            const CatchChallenger::CommonMap * const map=mapPointer->linked_map.at(mapIndex);
            if(not_finishedFile.contains(QString::fromStdString(map->map_file)+".tmx"))
            {
                key=QString::fromStdString(map->map_file)+".tmx";
                if(!not_finishedFile.contains(key))
                    abort();
                break;
            }
            mapIndex++;
        }
        if(mapIndex>=mapPointer->linked_map.size())
        {
            QStringList keys=not_finishedFile.keys();
            if(!keys.empty())
            {
                qSort(keys.begin(),keys.end());
                //key=keys.at(rand()%keys.size());
                key=keys.first();
            }
            else if(!finishedFile.isEmpty())
                key=finishedFile.keys().first();
            else
                return;
        }
    }
    else
    {
        QStringList keys=not_finishedFile.keys();
        if(!keys.empty())
        {
            qSort(keys.begin(),keys.end());
            //key=keys.at(rand()%keys.size());
            key=keys.first();
        }
        else if(!finishedFile.isEmpty())
            key=finishedFile.keys().first();
        else
            return;
    }

    selectedMap.clear();
    displayMap(key);
    canUpdate=true;
}

void MainWindow::displayMap(const QString key)
{
    {
        const QString &file=key;
        if(!not_finishedFile.contains(key) && !finishedFile.contains(key))
            abort();
        MapContent mapContent;
        if(not_finishedFile.contains(key))
            mapContent=not_finishedFile.value(key);
        else if(finishedFile.contains(key))
            mapContent=finishedFile.value(key);
        else
            abort();
        if(mapContent.region.isEmpty())
            abort();
        ui->region->setText(mapContent.region);
        ui->zone->setText(mapContent.zone);
        ui->subzone->setText(mapContent.subzone);
        ui->name->setText(mapContent.name);
        ui->type->setCurrentIndex(ui->type->findText(mapContent.type));
        ui->officialZone->setChecked(mapContent.officialzone);
        ui->finished->setChecked(finishedFile.contains(key));

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

                QScrollArea *scrollArea = new QScrollArea(ui->groupBox);
                scrollArea->setMinimumSize(QSize(350, 0));
                scrollArea->setWidgetResizable(true);
                QWidget * scrollAreaWidgetContents = new QWidget();
                scrollAreaWidgetContents->setGeometry(QRect(0, 0, 892, 337));
                QHBoxLayout * horizontalLayout_2 = new QHBoxLayout(scrollAreaWidgetContents);
                horizontalLayout_2->setSpacing(6);
                horizontalLayout_2->setContentsMargins(11, 11, 11, 11);

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
                        abort();//haveContent=false;

                    if(haveContent)
                    {
                        QGroupBox * groupBox_4 = new QGroupBox(scrollAreaWidgetContents);
                        if(finishedFile.contains(QString::fromStdString(map->map_file)+".tmx"))
                        {
                            if(mapContent.type=="outdoor")
                                groupBox_4->setStyleSheet("background-color: rgb(60, 230, 108);");
                            else if(mapContent.type=="city")
                                groupBox_4->setStyleSheet("background-color: #e7ff43;");
                            else if(mapContent.type=="indoor")
                                groupBox_4->setStyleSheet("background-color: #ff4343;");
                            else if(mapContent.type=="cave")
                                groupBox_4->setStyleSheet("background-color: #9b6c14;");
                            else
                                groupBox_4->setStyleSheet("background-color: #43d2ff;");
                        }
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
                        QLabel * label_10 = new QLabel(groupBox_4);
                        verticalLayout->addWidget(label_10);
                        horizontalLayout_2->addWidget(groupBox_4);
                        scrollArea->setWidget(scrollAreaWidgetContents);

                        groupBox_4->setTitle(QString("%1 (%2)").arg(mapContent.name).arg(mapContent.type));
                        label_7->setText("Region: <a href=\""+mapContent.region+"\">"+mapContent.region+"</a>");
                        label_7->setToolTip(mapContent.region);
                        label_8->setText("Zone: <a href=\""+mapContent.zone+"\">"+mapContent.zone+"</a>");
                        label_8->setToolTip(mapContent.zone);
                        label_9->setText("Sub zone: <a href=\""+mapContent.subzone+"\">"+mapContent.subzone+"</a>");
                        label_9->setToolTip(mapContent.subzone);
                        label_10->setText("<a href=\""+QString::fromStdString(map->map_file)+"\">[Edit]</a>");
                        label_10->setToolTip(QString::fromStdString(map->map_file));

                        connect(label_7,&QLabel::linkActivated,this,&MainWindow::on_label_7_linkActivated);
                        connect(label_8,&QLabel::linkActivated,this,&MainWindow::on_label_8_linkActivated);
                        connect(label_9,&QLabel::linkActivated,this,&MainWindow::on_label_9_linkActivated);
                        connect(label_10,&QLabel::linkActivated,this,&MainWindow::on_label_10_linkActivated);

                    }

                    mapIndex++;
                }

                ui->horizontalLayout_3->addWidget(scrollArea);
                ui->scrollArea=scrollArea;
            }
        }

        selectedMap=file;
    }
}

void MainWindow::on_region_textChanged(const QString &)
{
    ui->region->setText(simplifiedName(ui->region->text()));
    if(!canUpdate || (!finishedFile.contains(selectedMap) && !not_finishedFile.contains(selectedMap)))
        return;
    if(finishedFile.contains(selectedMap))
    {
        MapContent &mapContent=finishedFile[selectedMap];
        mapContent.region=ui->region->text();
    }
    if(not_finishedFile.contains(selectedMap))
    {
        MapContent &mapContent=not_finishedFile[selectedMap];
        mapContent.region=ui->region->text();
    }
    QSqlQuery query;
    if(!query.prepare("UPDATE maps SET region=:region WHERE file=:file;"))
    {
        qDebug() << query.lastError().text();
        abort();
    }
    query.bindValue(":file", selectedMap);
    query.bindValue(":region", ui->region->text());
    if(!query.exec())
    {
        qDebug() << query.lastError().text();
        abort();
    }
}

void MainWindow::on_zone_textChanged(const QString &)
{
    ui->zone->setText(simplifiedName(ui->zone->text()));
    if(!canUpdate || (!finishedFile.contains(selectedMap) && !not_finishedFile.contains(selectedMap)))
        return;
    if(finishedFile.contains(selectedMap))
    {
        MapContent &mapContent=finishedFile[selectedMap];
        mapContent.zone=ui->zone->text();
    }
    if(not_finishedFile.contains(selectedMap))
    {
        MapContent &mapContent=not_finishedFile[selectedMap];
        mapContent.zone=ui->zone->text();
    }
    QSqlQuery query;
    if(!query.prepare("UPDATE maps SET zone=:zone WHERE file=:file;"))
    {
        qDebug() << query.lastError().text();
        abort();
    }
    query.bindValue(":file", selectedMap);
    query.bindValue(":zone", ui->zone->text());
    if(!query.exec())
    {
        qDebug() << query.lastError().text();
        abort();
    }
}

void MainWindow::on_subzone_textChanged(const QString &)
{
    ui->subzone->setText(simplifiedName(ui->subzone->text()));
    if(!canUpdate || (!finishedFile.contains(selectedMap) && !not_finishedFile.contains(selectedMap)))
        return;
    if(finishedFile.contains(selectedMap))
    {
        MapContent &mapContent=finishedFile[selectedMap];
        mapContent.subzone=ui->subzone->text();
    }
    if(not_finishedFile.contains(selectedMap))
    {
        MapContent &mapContent=not_finishedFile[selectedMap];
        mapContent.subzone=ui->subzone->text();
    }
    QSqlQuery query;
    if(!query.prepare("UPDATE maps SET subzone=:subzone WHERE file=:file;"))
    {
        qDebug() << query.lastError().text();
        abort();
    }
    query.bindValue(":file", selectedMap);
    query.bindValue(":subzone", ui->subzone->text());
    if(!query.exec())
    {
        qDebug() << query.lastError().text();
        abort();
    }
}

void MainWindow::on_name_textChanged(const QString &)
{
    if(!canUpdate || (!finishedFile.contains(selectedMap) && !not_finishedFile.contains(selectedMap)))
        return;
    if(finishedFile.contains(selectedMap))
    {
        MapContent &mapContent=finishedFile[selectedMap];
        mapContent.name=ui->name->text();
    }
    if(not_finishedFile.contains(selectedMap))
    {
        MapContent &mapContent=not_finishedFile[selectedMap];
        mapContent.name=ui->name->text();
    }
    QSqlQuery query;
    if(!query.prepare("UPDATE maps SET name=:name WHERE file=:file;"))
    {
        qDebug() << query.lastError().text();
        abort();
    }
    query.bindValue(":file", selectedMap);
    query.bindValue(":name", ui->name->text());
    if(!query.exec())
    {
        qDebug() << query.lastError().text();
        abort();
    }
}

void MainWindow::on_type_currentIndexChanged(const QString &arg1)
{
    if(!canUpdate || (!finishedFile.contains(selectedMap) && !not_finishedFile.contains(selectedMap)))
        return;
    if(finishedFile.contains(selectedMap))
    {
        MapContent &mapContent=finishedFile[selectedMap];
        mapContent.type=arg1;
    }
    if(not_finishedFile.contains(selectedMap))
    {
        MapContent &mapContent=not_finishedFile[selectedMap];
        mapContent.type=arg1;
    }
    QSqlQuery query;
    if(!query.prepare("UPDATE maps SET type=:type WHERE file=:file;"))
    {
        qDebug() << query.lastError().text();
        abort();
    }
    query.bindValue(":file", selectedMap);
    query.bindValue(":type", arg1);
    if(!query.exec())
    {
        qDebug() << query.lastError().text();
        abort();
    }
}

void MainWindow::on_officialZone_toggled(bool checked)
{
    if(!canUpdate || (!finishedFile.contains(selectedMap) && !not_finishedFile.contains(selectedMap)))
        return;
    if(finishedFile.contains(selectedMap))
    {
        MapContent &mapContent=finishedFile[selectedMap];
        mapContent.officialzone=checked;
    }
    if(not_finishedFile.contains(selectedMap))
    {
        MapContent &mapContent=not_finishedFile[selectedMap];
        mapContent.officialzone=checked;
    }
    QSqlQuery query;
    if(!query.prepare("UPDATE maps SET officialzone=:officialzone WHERE file=:file;"))
    {
        qDebug() << query.lastError().text();
        abort();
    }
    query.bindValue(":file", selectedMap);
    query.bindValue(":officialzone", (int)checked);
    if(!query.exec())
    {
        qDebug() << query.lastError().text();
        abort();
    }
}

QString MainWindow::simplifiedName(QString name)
{
    name=name.toLower();
    name.replace(QRegularExpression("[^0-9a-z_./-]"), "-");
    name.replace(QRegularExpression("-+"), "-");
    return name;
}

void MainWindow::on_label_7_linkActivated(const QString &link)
{
    ui->region->setText(simplifiedName(link));
}

void MainWindow::on_label_8_linkActivated(const QString &link)
{
    ui->zone->setText(simplifiedName(link));
    ui->officialZone->setChecked(true);
}

void MainWindow::on_label_9_linkActivated(const QString &link)
{
    ui->subzone->setText(simplifiedName(link));
}

void MainWindow::on_label_10_linkActivated(const QString &link)
{
    canUpdate=false;
    selectedMap.clear();
    displayMap(link+".tmx");
    canUpdate=true;
}

void MainWindow::on_finished_toggled(bool checked)
{
    if(!canUpdate || (!finishedFile.contains(selectedMap) && !not_finishedFile.contains(selectedMap)))
        return;
    if(checked)
    {
        if(not_finishedFile.contains(selectedMap))
        {
            MapContent mapContent=not_finishedFile[selectedMap];
            finishedFile[selectedMap]=mapContent;
            not_finishedFile.remove(selectedMap);
        }
    }
    else
    {
        if(finishedFile.contains(selectedMap))
        {
            MapContent mapContent=finishedFile[selectedMap];
            not_finishedFile[selectedMap]=mapContent;
            finishedFile.remove(selectedMap);
        }
    }
    QSqlQuery query;
    if(!query.prepare("UPDATE maps SET finished=:finished WHERE file=:file;"))
    {
        qDebug() << query.lastError().text();
        abort();
    }
    query.bindValue(":file", selectedMap);
    query.bindValue(":finished", checked);
    if(!query.exec())
    {
        qDebug() << query.lastError().text();
        abort();
    }
    /*if(checked)
        displayNewNotFinishedMap(true);*/
    updateProgressLabel();
}

void MainWindow::updateProgressLabel()
{
    ui->progressLabel->setText(tr("Finished map: %1/%2").arg(finishedFile.size()).arg(finishedFile.size()+not_finishedFile.size()));
}
