/**************************************************************************
*  File: genolc.h                                          Part of tbaMUD *
*                                                                         *
*  Usage: Generic OLC Library - General.                                  *
*                                                                         *
*  Copyright 1996 by Harvey Gilpin, 1997-2001 by George Greer.            *
**************************************************************************/

#define STRING_TERMINATOR       '~'
#define CONFIG_GENOLC_MOBPROG	0

void smash_tilde(char *str); /* from modify.c */
int genolc_checkstring(struct descriptor_data *d, char *arg);
int remove_from_save_list(zone_vnum, int type);
int add_to_save_list(zone_vnum, int type);
int in_save_list(zone_vnum, int type);
void strip_cr(char *);
int save_all(void);
char *str_udup(const char *);
void copy_ex_descriptions(struct extra_descr_data **to, struct extra_descr_data *from);
void free_ex_descriptions(struct extra_descr_data *head);
int sprintascii(char *out, bitvector_t bits);

struct save_list_data {
  int zone;
  int type;
  struct save_list_data *next;
};

extern struct save_list_data *save_list;
extern int top_shop_offset;

/* save_list_data.type */
#define SL_MOB	0
#define SL_OBJ	1
#define SL_SHP	2
#define SL_WLD	3
#define SL_ZON	4
#define SL_CFG	5
#define SL_ACT  6
#define SL_MAX	6

#ifndef STOCK_CIRCLE
#define SL_ACTION 100 /* must be above MAX */
#endif

#define ZCMD(zon, cmds)	zone_table[(zon)].cmd[(cmds)]

#define LIMIT(var, low, high)	MIN(high, MAX(var, low))

room_vnum genolc_zone_bottom(zone_rnum rznum);
room_vnum genolc_zonep_bottom(struct zone_data *zone);

