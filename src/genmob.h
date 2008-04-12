/**
* @file genmob.h
* Generic OLC Library - Mobiles.
* 
* Part of the core tbaMUD source code distribution, which is a derivative
* of, and continuation of, CircleMUD.
* 
* This source code, which was not part of the CircleMUD legacy code,
* is attributed to:
* Copyright 1996 by Harvey Gilpin, 1997-2001 by George Greer.                                                    
*/
#ifndef _GENMOB_H_
#define _GENMOB_H_

int delete_mobile(mob_rnum);
int copy_mobile(struct char_data *to, struct char_data *from);
int add_mobile(struct char_data *, mob_vnum);
int copy_mob_strings(struct char_data *to, struct char_data *from);
int free_mob_strings(struct char_data *);
int free_mobile(struct char_data *mob);
int save_mobiles(zone_rnum rznum);
int update_mobile_strings(struct char_data *t, struct char_data *f);
void check_mobile_strings(struct char_data *mob);
void check_mobile_string(mob_vnum i, char **string, const char *desc);
int write_mobile_record(mob_vnum mvnum, struct char_data *mob, FILE *fd);
int write_mobile_espec(mob_vnum mvnum, struct char_data *mob, FILE *fd);
int free_mobile_strings(struct char_data *mob);
int copy_mobile_strings(struct char_data *t, struct char_data *f);
#if CONFIG_GENOLC_MOBPROG
int write_mobile_mobprog(mob_vnum mvnum, struct char_data *mob, FILE *fd);
#endif

/* Handy macros. */
#define GET_NDD(mob)	((mob)->mob_specials.damnodice)
#define GET_SDD(mob)	((mob)->mob_specials.damsizedice)
#define GET_ALIAS(mob)	((mob)->player.name)
#define GET_SDESC(mob)	((mob)->player.short_descr)
#define GET_LDESC(mob)	((mob)->player.long_descr)
#define GET_DDESC(mob)	((mob)->player.description)
#define GET_ATTACK(mob)	((mob)->mob_specials.attack_type)

#endif /* _GENMOB_H_ */
