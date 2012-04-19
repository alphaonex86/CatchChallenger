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

#include "pokecraft-server/EventDispatcher.h"
#include "pokecraft-general/DebugClass.h"
#include "pokecraft-general/GeneralStructures.h"
#include "pokecraft-client/pokecraft-client-chat.h"

namespace Ui {
	class MainWindow;
}

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
	QList<Player_private_and_public_informations> players;
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
	void new_player_is_connected(Player_private_and_public_informations player);
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
	void on_instant_player_number_stateChanged(int arg1);
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
	void on_db_host_editingFinished();
	void on_db_login_editingFinished();
	void on_db_pass_editingFinished();
	void on_db_base_editingFinished();
	void on_pushButton_server_benchmark_clicked();

signals:
	void record_latency();
};

#endif // MAINWINDOW_H
