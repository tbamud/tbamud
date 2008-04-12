/**
* @file genzon.h
* Generic OLC Library - Zones.
* 
* Part of the core tbaMUD source code distribution, which is a derivative
* of, and continuation of, CircleMUD.
* 
* This source code, which was not part of the CircleMUD legacy code,
* is attributed to:
* Copyright 1996 by Harvey Gilpin, 1997-2001 by George Greer.                                                    
*/
#ifndef _GENZON_H_
#define _GENZON_H_

zone_rnum create_new_zone(zone_vnum vzone_num, room_vnum bottom, room_vnum top, const char **error);
void remove_room_zone_commands(zone_rnum zone, room_rnum room_num);
int save_zone(zone_rnum zone_num);
int count_commands(struct reset_com *list);
void add_cmd_to_list(struct reset_com **list, struct reset_com *newcmd, int pos);
int new_command(struct zone_data *zone, int pos);
void delete_zone_command(struct zone_data *zone, int pos);
zone_rnum real_zone_by_thing(room_vnum vznum);
void create_world_index(int znum, const char *type);

#endif /* _GENZON_H_ */
