#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QListWidgetItem>
#include <tinyxml2::XMLElement>
#include <QHash>
#include <QString>
#include <QSettings>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
private slots:
    void on_browseMapMetaFile_clicked();
    void on_openMapMetaFile_clicked();
    void on_nameAdd_clicked();
    void on_nameRemove_clicked();
    void on_type_editingFinished();
    void on_backgroundsound_editingFinished();
    void on_zone_editingFinished();
    void on_save_clicked();
    void on_grassAdd_clicked();
    void on_grassRemove_clicked();
    void on_watherAdd_clicked();

    void on_watherRemove_clicked();

    void on_caveAdd_clicked();

    void on_caveRemove_clicked();

private:
    Ui::MainWindow *ui;
    QHash<quint32,tinyxml2::XMLElement> items;
    QSettings settings;
    QDomDocument domDocument;
    bool loadingTheInformations;
    int grassTotalLuck,watherTotalLuck,caveTotalLuck;
};

#endif // MAINWINDOW_H
