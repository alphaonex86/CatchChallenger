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
    void on_nameEditLanguageAdd_clicked();
    void on_nameEditLanguageRemove_clicked();
    void on_descriptionEditLanguageAdd_clicked();
    void on_descriptionEditLanguageRemove_clicked();
    void on_levelAdd_clicked();
    void on_levelRemove_clicked();
    void on_levelEdit_clicked();
    void on_level_itemDoubleClicked(QListWidgetItem *item);
    void updateBuffList();
    void updateLifeList();
    void updateLevelList();
    QDomElement getCurrentLevelInfo();
    QDomElement getCurrentBuffInfo();
    QDomElement getCurrentLifeInfo();
    void on_skillLevelBack_clicked();
    void on_life_back_clicked();
    void on_buff_back_clicked();
    void on_skillLifeEffectEdit_clicked();
    void on_skillBuffEffectEdit_clicked();
    void on_level_life_itemDoubleClicked(QListWidgetItem *item);
    void on_level_buff_itemDoubleClicked(QListWidgetItem *item);
    void on_life_quantity_editingFinished();
    void on_life_quantity_type_currentIndexChanged(int index);
    void on_life_success_editingFinished();
    void on_life_apply_on_currentIndexChanged(int index);
    void on_buff_id_editingFinished();
    void on_buff_level_editingFinished();
    void on_buff_success_editingFinished();
    void on_buff_apply_on_currentIndexChanged(int index);
    void on_level_level_editingFinished();
    void on_level_sp_editingFinished();
    void on_level_endurance_editingFinished();
    void on_skillLifeEffectAdd_clicked();
    void on_skillLifeEffectDelete_clicked();
    void on_skillBuffEffectAdd_clicked();
    void on_skillBuffEffectDelete_clicked();
private:
    Ui::MainWindow *ui;
    QHash<quint32,QDomElement> skills;
    QSettings settings;
    QDomDocument domDocument;
    bool loadingTheInformations;
};

#endif // MAINWINDOW_H
