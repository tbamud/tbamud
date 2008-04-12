/**
* @file genolc.h
* Generic OLC Library - General.
* 
* Part of the core tbaMUD source code distribution, which is a derivative
* of, and continuation of, CircleMUD.
* 
* This source code, which was not part of the CircleMUD legacy code,
* is attributed to:
* Copyright 1996 by Harvey Gilpin, 1997-2001 by George Greer.                                                    
*/
#ifndef _GENOLC_H_
#define _GENOLC_H_

#define STRING_TERMINATOR       '~'
#define CONFIG_GENOLC_MOBPROG	0

int genolc_checkstring(struct descriptor_data *d, char *arg);
int remove_from_save_list(zone_vnum, int type);
int add_to_save_list(zone_vnum, int type);
int in_save_list(zone_vnum, int type);
void strip_cr(char *);
int save_all(void);
char *str_udup(const char *);
char *str_udupnl(const char *);
void copy_ex_descriptions(struct extra_descr_data **to, struct extra_descr_data *from);
void free_ex_descriptions(struct extra_descr_data *head);
int sprintascii(char *out, bitvector_t bits);
ACMD(do_export_zone);
ACMD(do_show_save_list);

struct save_list_data {
  int zone;
  int type;
  struct save_list_data *next;
};

extern struct save_list_data *save_list;

/* save_list_data.type */
#define SL_MOB	0
#define SL_OBJ	1
#define SL_SHP	2
#define SL_WLD	3
#define SL_ZON	4
#define SL_CFG	5
#define SL_QST  6
#define SL_MAX  6	
#define SL_ACT SL_MAX + 1 /* must be above MAX */ 
#define SL_HLP SL_MAX + 2

#define ZCMD(zon, cmds)	zone_table[(zon)].cmd[(cmds)]

#define LIMIT(var, low, high)	MIN(high, MAX(var, low))

room_vnum genolc_zone_bottom(zone_rnum rznum);
room_vnum genolc_zonep_bottom(struct zone_data *zone);
extern void free_save_list(void);

#endif /* _GENOLC_H_ */
