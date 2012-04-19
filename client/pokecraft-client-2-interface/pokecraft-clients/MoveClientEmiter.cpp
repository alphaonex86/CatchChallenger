#include "MoveClientEmiter.h"

MoveClientEmiter::MoveClientEmiter(Direction original_direction)
{
	last_direction=original_direction;
	last_step=0;
}

void MoveClientEmiter::send_player_move(quint8 moved_unit,Direction the_new_direction)
{
	emit new_player_movement(moved_unit,the_new_direction);
}
