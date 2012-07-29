#ifndef MAPVISIBILITYALGORITHM_SIMPLE_H
#define MAPVISIBILITYALGORITHM_SIMPLE_H

#include "ClientMapManagement.h"

class MapVisibilityAlgorithm_Simple : public ClientMapManagement
{
public:
	explicit MapVisibilityAlgorithm_Simple();
	virtual ~MapVisibilityAlgorithm_Simple();
	void reinsertAllClient(const int &loop_size);
	//drop all clients
	virtual void dropAllClients();
protected:
	//add clients linked
	void insertClient();
	void moveClient(const quint8 &movedUnit,const Direction &direction);
	void removeClient();
	void mapVisiblity_unloadFromTheMap();
private:
	static int index;
	static int loop_size;
	static MapVisibilityAlgorithm_Simple *current_client;//static to drop down the memory
	//overwrite
	//remove the move/remove
	#if defined(POKECRAFT_SERVER_VISIBILITY_CLEAR) && defined(POKECRAFT_SERVER_MAP_DROP_OVER_MOVE)
	QHash<SIMPLIFIED_PLAYER_ID_TYPE, MapVisibilityAlgorithm_Simple *>			to_send_over_move;
	void moveAnotherClientWithMap(const SIMPLIFIED_PLAYER_ID_TYPE &player_id,MapVisibilityAlgorithm_Simple *the_another_player,const quint8 &movedUnit,const Direction &direction);
	void send_reinsert();
	#endif
	//for the purge buffer
	void send_insert();
	void send_move();
	void send_remove();
	#ifdef POKECRAFT_SERVER_VISIBILITY_CLEAR
	void insertAnotherClient(const SIMPLIFIED_PLAYER_ID_TYPE &player_id,MapVisibilityAlgorithm_Simple *the_another_player);
	void removeAnotherClient(const SIMPLIFIED_PLAYER_ID_TYPE &player_id);
	#endif
	//map load/unload and change
	virtual void			loadOnTheMap();
	virtual void			unloadFromTheMap();

	//map move
	bool singleMove(const Direction &direction);

	//temp variable for purge buffer
	static QByteArray purgeBuffer_outputData;
	static int purgeBuffer_index;
	static int purgeBuffer_list_size;
	static int purgeBuffer_list_size_internal;
	static int purgeBuffer_indexMovement;
	static bool mapHaveChanged;
	static map_management_move purgeBuffer_move;
	static QHash<SIMPLIFIED_PLAYER_ID_TYPE, QList<map_management_movement> >::const_iterator i_move;
	static QHash<SIMPLIFIED_PLAYER_ID_TYPE, QList<map_management_movement> >::const_iterator i_move_end;
	static QHash<SIMPLIFIED_PLAYER_ID_TYPE, MapVisibilityAlgorithm_Simple *>::const_iterator i_insert;
	static QHash<SIMPLIFIED_PLAYER_ID_TYPE, MapVisibilityAlgorithm_Simple *>::const_iterator i_insert_end;
	static QSet<SIMPLIFIED_PLAYER_ID_TYPE>::const_iterator i_remove;
	static QSet<SIMPLIFIED_PLAYER_ID_TYPE>::const_iterator i_remove_end;
	static Map*			old_map;
	static Map*			new_map;

	//temp variable to move on the map
	static map_management_movement moveClient_tempMov;

	// stuff to send
	QHash<SIMPLIFIED_PLAYER_ID_TYPE, MapVisibilityAlgorithm_Simple *>			to_send_insert;
	QHash<SIMPLIFIED_PLAYER_ID_TYPE, QList<map_management_movement> >	to_send_move;
	QSet<SIMPLIFIED_PLAYER_ID_TYPE>						to_send_remove;
public slots:
	virtual void purgeBuffer();

private slots:
	virtual void extraStop();
};

#endif // MAPVISIBILITYALGORITHM_SIMPLE_H
