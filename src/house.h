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

#define MAX_HOUSES	100    /* Maximum number of houses in the MUD */
#define MAX_GUESTS	10     /* Maximum number of guests per house  */

/* House modes/types */
#define HOUSE_PRIVATE	0  /* A Normal Player-Owned House */
#define HOUSE_GOD       1  /* Imm owned house             */
#define HOUSE_SHOP      2  /* Player-Owned Shop           */

#define NUM_HOUSE_TYPES 3

/* House Flags - also see string constants, in constants.c */
#define HOUSE_NOGUESTS   0   /* Owner cannot add guests                       */
#define HOUSE_FREE       1   /* House does not require payments               */
#define HOUSE_NOIMMS     2   /* Imms below level 2 cannot enter               */
#define HOUSE_IMPONLY    3   /* Imms below level 4 cannot enter               */
#define HOUSE_RENTFREE   4   /* No rent is charged on items left here         */
#define HOUSE_SAVENORENT 5   /* NORENT items are crashsaved too               */
#define HOUSE_NOSAVE     6   /* Do not crash save this room - private only    */
#define HOUSE_NOSPEC     7   /* Don't use 'standard' spec proc for house type */

#define HOUSE_NUM_FLAGS  8

/** House flags.
 * @param loc The house_control_data structure. */
#define HOUSE_FLAGS(loc)	((loc)->house_flags)

/** House flagged.
 * @param loc  The house_control_data structure. *
 *        flag The house flag (see above)        */
#define HOUSE_FLAGGED(loc, flag) ((loc) && IS_SET_AR(HOUSE_FLAGS(loc), (flag)))


#define TOROOM(room, dir) (world[room].dir_option[dir] ? \
			    world[room].dir_option[dir]->to_room : NOWHERE)

/* List structure for house guests */
struct guest_data {
   long id;                  /**< The ID number of this guest    */
   struct guest_data *next;  /**< pointer to next guest in list  */
};

/* List structure for houses */
struct house_control_data {
   room_vnum vnum;                  /**< vnum of this house             */
   room_vnum atrium;                /**< vnum of atrium                 */
   sh_int exit_num;                 /**< direction of house's exit      */
   time_t built_on;                 /**< date this house was built      */
   long built_by;                   /**< The ID of the builder (hsedit) */
   long owner;                      /**< idnum of house's owner         */
   int mode;                        /**< mode of ownership              */
   struct guest_data *guests;       /**< idnums of house's guests       */
   time_t last_payment;             /**< date of last house payment     */
   int house_flags[HS_ARRAY_MAX];   /**< House Flags (hsedit)           */
   mob_vnum receptionist;           /**< The Receptionist/Shopkeeper    */
   struct house_control_data *next; /**< pointer to next house in list  */
};

#define TOROOM(room, dir) (world[room].dir_option[dir] ? \
			    world[room].dir_option[dir]->to_room : NOWHERE)

/* Functions in house.c made externally available */
/* Utility Functions */
void House_boot(void);
void House_save_all(void);
int	 House_can_enter(struct char_data *ch, room_vnum house);
void House_crashsave(room_vnum vnum);
void House_list_guests(struct char_data *ch, struct house_control_data *h, int quiet);
int  House_save(struct obj_data *obj, FILE *fp);
void House_save_control(void);
void House_delete_file(room_vnum vnum);
void hcontrol_list_houses(struct char_data *ch, char *arg);
void set_house(struct house_control_data *h);
void free_house_guests(struct house_control_data *h);
void clear_house_control_data(struct house_control_data *h);
void add_house_guest(struct house_control_data *h_data, long guest_id);
int  count_house_guests(struct house_control_data *h);
bool is_house_guest(struct house_control_data *h, long id_num);
bool remove_house_guest(struct house_control_data *h, long id_num);
int  count_houses(void);
void free_house(struct house_control_data *h);
void free_house_list(void);
bool delete_house_control(struct house_control_data *to_del);
struct house_control_data *new_house(void);
struct house_control_data *add_house(struct house_control_data *h_data);
struct house_control_data *find_house(room_vnum vnum);

/* In game Commands */
ACMD(do_hcontrol);
ACMD(do_house);

/* Spec-procs in house.c */
SPECIAL(house_shopkeeper);
SPECIAL(house_receptionist);

#endif /* _HOUSE_H_ */
