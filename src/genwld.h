/************************************************************************
 * Generic OLC Library - Rooms / genwld.h			v1.0	*
 * Original author: Levork						*
 * Copyright 1996 by Harvey Gilpin					*
 * Copyright 1997-2001 by George Greer (greerga@circlemud.org)		*
 ************************************************************************/

room_rnum add_room(struct room_data *);
int delete_room(room_rnum);
int save_rooms(zone_rnum);
int copy_room(struct room_data *to, struct room_data *from);
room_rnum duplicate_room(room_vnum to, room_rnum from);
int copy_room_strings(struct room_data *dest, struct room_data *source);
int free_room_strings(struct room_data *);
