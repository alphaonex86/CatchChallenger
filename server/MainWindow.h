#ifndef CATCHCHALLENGER_MAINWINDOW_H
#define CATCHCHALLENGER_MAINWINDOW_H

#include <QMainWindow>
#include <QProcess>
#include <QCoreApplication>
#include <QDir>
#include <QCloseEvent>
#include <QMessageBox>
#include <QTimer>
#include <QTime>
#include <QListWidgetItem>

#include "NormalServer.h"
#include "base/GlobalServerData.h"
#include "../general/base/DebugClass.h"
#include "../general/base/GeneralStructures.h"
#include "../general/base/ChatParsing.h"

namespace Ui {
    class MainWindow;
}
namespace CatchChallenger {
class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
protected:
    void changeEvent(QEvent *e);
private:
    Ui::MainWindow *ui;
    NormalServer server;
    bool need_be_restarted;
    bool need_be_closed;
    void closeEvent(QCloseEvent *event);
    std::unordered_settings *settings;
    void load_settings();
    void send_settings();
    QList<Player_private_and_public_informations> players;
    QTimer timer_update_the_info;
    QTimer check_latency;
    QTime time_latency;
    uint16_t internal_currentLatency;
    QList<Event> events;
    std::unordered_map<std::string,std::unordered_map<std::string,GameServerSettings::ProgrammedEvent> > programmedEventList;
    std::string sizeToString(double size);
    std::string adaptString(float size);
private slots:
    void on_lineEdit_returnPressed();
    void updateActionButton();
    void server_is_started(bool is_started);
    void server_need_be_stopped();
    void server_need_be_restarted();
    void new_player_is_connected(Player_private_and_public_informations player);
    void player_is_disconnected(std::string pseudo);
    void new_chat_message(std::string pseudo,Chat_type type,std::string text);
    void server_error(std::string error);
    void haveQuitForCriticalDatabaseQueryFailed();
    void update_the_info();
    void start_calculate_latency();
    void stop_calculate_latency();
    void benchmark_result(int latency,double TX_speed,double RX_speed,double TX_size,double RX_size,double second);
    void clean_updated_info();
    void update_capture();
    //auto slots
    void on_pushButton_server_start_clicked();
    void on_pushButton_server_stop_clicked();
    void on_pushButton_server_restart_clicked();
    void on_max_player_valueChanged(int arg1);
    void on_server_ip_editingFinished();
    void on_pvp_stateChanged(int arg1);
    void on_server_port_valueChanged(int arg1);
    void on_rates_xp_normal_valueChanged(double arg1);
    void on_rates_gold_normal_valueChanged(double arg1);
    void on_chat_allow_all_toggled(bool checked);
    void on_chat_allow_local_toggled(bool checked);
    void on_chat_allow_private_toggled(bool checked);
    void on_chat_allow_clan_toggled(bool checked);
    void on_db_mysql_host_editingFinished();
    void on_db_mysql_login_editingFinished();
    void on_db_mysql_pass_editingFinished();
    void on_db_mysql_base_editingFinished();
    void on_MapVisibilityAlgorithm_currentIndexChanged(int index);
    void on_MapVisibilityAlgorithmSimpleMax_valueChanged(int arg1);
    void on_MapVisibilityAlgorithmSimpleReshow_editingFinished();
    void on_db_type_currentIndexChanged(int index);
    void updateDbGroupbox();
    void on_sendPlayerNumber_toggled(bool checked);
    void on_db_sqlite_browse_clicked();
    void on_tolerantMode_toggled(bool checked);
    void on_db_fight_sync_currentIndexChanged(int index);
    void on_comboBox_city_capture_frequency_currentIndexChanged(int index);
    void on_comboBox_city_capture_day_currentIndexChanged(int index);
    void on_timeEdit_city_capture_time_editingFinished();
    void on_compression_currentIndexChanged(int index);
    void on_min_character_editingFinished();
    void on_max_character_editingFinished();
    void on_max_pseudo_size_editingFinished();
    void on_character_delete_time_editingFinished();
    void on_automatic_account_creation_clicked();
    void on_anonymous_toggled(bool checked);
    void on_server_message_textChanged();
    void on_proxy_editingFinished();
    void on_proxy_port_editingFinished();
    void on_forceSpeed_toggled(bool checked);
    void on_speed_editingFinished();
    void on_dontSendPseudo_toggled(bool checked);
    void on_dontSendPlayerType_toggled(bool checked);
    void on_rates_xp_pow_normal_valueChanged(double arg1);
    void on_rates_drop_normal_valueChanged(double arg1);
    void on_forceClientToSendAtMapChange_toggled(bool checked);
    void on_MapVisibilityAlgorithmWithBorderMax_editingFinished();
    void on_MapVisibilityAlgorithmWithBorderReshow_editingFinished();
    void on_MapVisibilityAlgorithmWithBorderMaxWithBorder_editingFinished();
    void on_MapVisibilityAlgorithmWithBorderReshowWithBorder_editingFinished();
    void on_httpDatapackMirror_editingFinished();
    void on_datapack_cache_toggled(bool checked);
    void on_datapack_cache_timeout_checkbox_toggled(bool checked);
    void on_datapack_cache_timeout_editingFinished();
    void on_linux_socket_cork_toggled(bool checked);
    void datapack_cache_save();
    void on_MapVisibilityAlgorithmSimpleReemit_toggled(bool checked);
    void on_useSsl_toggled(bool checked);
    void on_useSP_toggled(bool checked);
    void on_autoLearn_toggled(bool checked);
    void on_programmedEventType_currentIndexChanged(int index);
    void on_programmedEventList_itemActivated(QListWidgetItem *item);
    void on_programmedEventAdd_clicked();
    void on_programmedEventEdit_clicked();
    void on_programmedEventRemove_clicked();
    void on_tcpNodelay_toggled(bool checked);
    void on_maxPlayerMonsters_editingFinished();
    void on_maxWarehousePlayerMonsters_editingFinished();
    void on_maxPlayerItems_editingFinished();
    void on_maxWarehousePlayerItems_editingFinished();
    void on_tryInterval_editingFinished();
    void on_considerDownAfterNumberOfTry_editingFinished();
    void on_announce_toggled(bool checked);

    void on_compressionLevel_valueChanged(int value);

signals:
    void record_latency();
};
}

#endif // MAINWINDOW_H
