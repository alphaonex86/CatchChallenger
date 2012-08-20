#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QProcess>
#include <QCoreApplication>
#include <QDir>
#include <QCloseEvent>
#include <QMessageBox>
#include <QTimer>
#include <QTime>

#include "base/EventDispatcher.h"
#include "../general/base/DebugClass.h"
#include "../general/base/GeneralStructures.h"
#include "../general/base/ChatParsing.h"

namespace Ui {
	class MainWindow;
}
namespace Pokecraft {
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
	EventDispatcher eventDispatcher;
	bool need_be_restarted;
	bool need_be_closed;
	void closeEvent(QCloseEvent *event);
	QSettings *settings;
	void load_settings();
	void send_settings();
	QList<Player_internal_informations> players;
	QTimer timer_update_the_info;
	QTimer check_latency;
	QTime time_latency;
	quint16 internal_currentLatency;
	QString sizeToString(double size);
	QString adaptString(float size);
private slots:
	void on_lineEdit_returnPressed();
	void updateActionButton();
	void server_is_started(bool is_started);
	void server_need_be_stopped();
	void server_need_be_restarted();
	void new_player_is_connected(Player_internal_informations player);
	void player_is_disconnected(QString pseudo);
	void new_chat_message(QString pseudo,Chat_type type,QString text);
	void server_error(QString error);
	void update_the_info();
	void start_calculate_latency();
	void stop_calculate_latency();
	void benchmark_result(int latency,double TX_speed,double RX_speed,double TX_size,double RX_size,double second);
	void clean_updated_info();
	//auto slots
	void on_pushButton_server_start_clicked();
	void on_pushButton_server_stop_clicked();
	void on_pushButton_server_restart_clicked();
	void on_max_player_valueChanged(int arg1);
	void on_server_ip_editingFinished();
	void on_pvp_stateChanged(int arg1);
	void on_server_port_valueChanged(int arg1);
	void on_rates_xp_normal_valueChanged(double arg1);
	void on_rates_xp_premium_valueChanged(double arg1);
	void on_rates_gold_normal_valueChanged(double arg1);
	void on_rates_gold_premium_valueChanged(double arg1);
	void on_rates_shiny_normal_valueChanged(double arg1);
	void on_rates_shiny_premium_valueChanged(double arg1);
	void on_chat_allow_all_toggled(bool checked);
	void on_chat_allow_local_toggled(bool checked);
	void on_chat_allow_private_toggled(bool checked);
	void on_chat_allow_aliance_toggled(bool checked);
	void on_chat_allow_clan_toggled(bool checked);
	void on_db_mysql_host_editingFinished();
	void on_db_mysql_login_editingFinished();
	void on_db_mysql_pass_editingFinished();
	void on_db_mysql_base_editingFinished();
	void on_pushButton_server_benchmark_clicked();
	void on_MapVisibilityAlgorithm_currentIndexChanged(int index);
	void on_MapVisibilityAlgorithmSimpleMax_valueChanged(int arg1);
	void on_MapVisibilityAlgorithmSimpleReshow_editingFinished();
	void on_benchmark_benchmarkMap_clicked();

	void on_benchmark_seconds_valueChanged(int arg1);

	void on_benchmark_clients_valueChanged(int arg1);

	void on_db_type_currentIndexChanged(int index);
	void updateDbGroupbox();
signals:
	void record_latency();
};
}

#endif // MAINWINDOW_H
