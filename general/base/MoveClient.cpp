#include "MoveClient.h"

MoveClient::MoveClient()
{
	last_direction=Direction_look_at_bottom;
	last_step=0;
}

void MoveClient::newDirection(const Direction &the_new_direction)
{
	if(last_direction!=the_new_direction)
	{
		send_player_move(last_step,the_new_direction);
		last_step=0;
		last_direction=the_new_direction;
	}
	else
	{
		switch(the_new_direction)
		{
			case Direction_look_at_top:
			case Direction_look_at_right:
			case Direction_look_at_bottom:
			case Direction_look_at_left:
			//to drop the dual same trame
			return;
			break;
			default:
			break;
		}
	}
	switch(the_new_direction)
	{
		case Direction_move_at_top:
		case Direction_move_at_right:
		case Direction_move_at_bottom:
		case Direction_move_at_left:
			last_step++;
		break;
		default:
		break;
	}
}
