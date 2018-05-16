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
    //permanent bot on client, temp to parse on the server
    struct Bot
    {
        QHash<quint8,tinyxml2::XMLElement> step;
        quint32 botId;
    };
private slots:
    void updateTextDisplayed();
private:
    Ui::MainWindow *ui;
    QHash<quint8,Bot> botFiles;
    quint8 selectedBot;
    quint8 selectedStep;
    QHash<QString,QString> allowedType;
    QHash<QString,QString> reverseAllowedType;
    QSettings settings;
    QDomDocument domDocument;
private:
    void updateBotList();
    void updateStepList();
    void updateType();
    void editStep(quint8 id, bool newStep=false);
private slots:
    void on_browseBotFile_clicked();
    void on_openBotFile_clicked();
    void on_botListAdd_clicked();
    void on_botListDelete_clicked();
    void on_botListEdit_clicked();
    void on_stepListAdd_clicked();
    void on_stepListDelete_clicked();
    void on_stepListEdit_clicked();
    void on_stepListBack_clicked();
    void on_stepEditBack_clicked();
    void on_plainTextEdit_textChanged();
    void on_stepEditLanguageRemove_clicked();
    void on_stepEditLanguageAdd_clicked();
    void on_stepEditShop_editingFinished();
    void on_stepEditSell_editingFinished();
    void on_stepEditFight_editingFinished();
    void on_botFileSave_clicked();
    void on_botList_itemDoubleClicked(QListWidgetItem *item);
    void on_stepList_itemDoubleClicked(QListWidgetItem *item);
    void on_stepEditZoneCapture_editingFinished();
    void on_stepEditIndustry_editingFinished();
};

#endif // MAINWINDOW_H
