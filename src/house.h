/**
* @file house.h
* Player house structures, prototypes and defines.
* 
* Part of the core tbaMUD source code distribution, which is a derivative
* of, and continuation of, CircleMUD.
*                                                                        
* All rights reserved.  See license for complete information.                                                                
* Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University 
* CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               
*/
#ifndef _HOUSE_H_
#define _HOUSE_H_

#define MAX_HOUSES	100
#define MAX_GUESTS	10

#define HOUSE_PRIVATE	0

struct house_control_rec {
   room_vnum vnum;		/* vnum of this house		*/
   room_vnum atrium;		/* vnum of atrium		*/
   sh_int exit_num;		/* direction of house's exit	*/
   time_t built_on;		/* date this house was built	*/
   int mode;			/* mode of ownership		*/
   long owner;			/* idnum of house's owner	*/
   int num_of_guests;		/* how many guests for house	*/
   long guests[MAX_GUESTS];	/* idnums of house's guests	*/
   time_t last_payment;		/* date of last house payment   */
   long spare0;
   long spare1;
   long spare2;
   long spare3;
   long spare4;
   long spare5;
   long spare6;
   long spare7;
};

#define TOROOM(room, dir) (world[room].dir_option[dir] ? \
			    world[room].dir_option[dir]->to_room : NOWHERE)

/* Functions in house.c made externally available */
/* Utility Functions */
void	House_boot(void);
void	House_save_all(void);
int	House_can_enter(struct char_data *ch, room_vnum house);
void	House_crashsave(room_vnum vnum);
void	House_list_guests(struct char_data *ch, int i, int quiet);
int House_save(struct obj_data *obj, FILE *fp);
void hcontrol_list_houses(struct char_data *ch, char *arg);
/* In game Commands */
ACMD(do_hcontrol);
ACMD(do_house);


#endif /* _HOUSE_H_ */
