#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QListWidgetItem>
#include <QDomElement>
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
    void updateItemList();
private slots:
    void on_browseItemFile_clicked();
    void on_openItemFile_clicked();
    void on_itemList_itemDoubleClicked(QListWidgetItem *item);
    void on_itemListAdd_clicked();
    void on_itemListEdit_clicked();
    void on_itemListDelete_clicked();
    void on_itemListSave_clicked();
    void on_nameEditLanguageList_currentIndexChanged(int index);
    void on_descriptionEditLanguageList_currentIndexChanged(int index);
    void on_stepEditBack_clicked();
    void on_namePlainTextEdit_textChanged();
    void on_descriptionPlainTextEdit_textChanged();
    void on_price_editingFinished();
    void on_imageBrowse_clicked();
private:
    Ui::MainWindow *ui;
    QHash<quint32,QDomElement> items;
    QSettings settings;
    QDomDocument domDocument;
    bool loadingTheInformations;
};

#endif // MAINWINDOW_H
