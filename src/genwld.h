/**
* @file genwld.h
* Generic OLC Library - Rooms.
* 
* Part of the core tbaMUD source code distribution, which is a derivative
* of, and continuation of, CircleMUD.
* 
* This source code, which was not part of the CircleMUD legacy code,
* is attributed to:
* By Levork. Copyright 1996 by Harvey Gilpin, 1997-2001 by George Greer.                                                    
*/
#ifndef _GENWLD_H_
#define _GENWLD_H_

room_rnum add_room(struct room_data *);
int delete_room(room_rnum);
int save_rooms(zone_rnum);
int copy_room(struct room_data *to, struct room_data *from);
room_rnum duplicate_room(room_vnum to, room_rnum from);
int copy_room_strings(struct room_data *dest, struct room_data *source);
int free_room_strings(struct room_data *);

#endif /* _GENWLD_H_ */
