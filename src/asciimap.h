/**************************************************************************
*  File: asciimap.h                                        Part of tbaMUD *
*  Usage: Generates an ASCII map of the player's surroundings.            *
*                                                                         *
*  All rights reserved.  See license for complete information.            *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
**************************************************************************/
#ifndef ASCIIMAP_H_
#define ASCIIMAP_H_

/* Map options (settable in cedit) */
#define MAP_OFF      0
#define MAP_ON       1
#define MAP_IMM_ONLY 2

/* Exported function prototypes */
bool can_see_map(struct char_data *ch);
void str_and_map(char *str, struct char_data *ch, room_vnum target_room );
ACMD(do_map);

#endif /* ASCIIMAP_H_*/
