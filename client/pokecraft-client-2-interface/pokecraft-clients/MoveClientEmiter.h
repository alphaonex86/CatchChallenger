#ifndef MOVECLIENTEMITER_H
#define MOVECLIENTEMITER_H

#include <QObject>

#include "../pokecraft-general/GeneralStructures.h"
#include "../pokecraft-general/MoveClient.h"

class MoveClientEmiter : public QObject, public MoveClient
{
	Q_OBJECT
public:
	MoveClientEmiter(Direction original_direction);
private:
	void send_player_move(quint8 moved_unit,Direction the_new_direction);
signals:
	void new_player_movement(quint8 moved_unit,Direction the_new_direction);
};

#endif // MOVECLIENTEMITER_H
