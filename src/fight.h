/**
* @file fight.h
* Fighting and violence functions and variables.
* 
* Part of the core tbaMUD source code distribution, which is a derivative
* of, and continuation of, CircleMUD.
*                                                                        
* All rights reserved.  See license for complete information.                                                                
* Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University 
* CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.
*
*/
#ifndef _FIGHT_H_
#define _FIGHT_H_

/* Structures and defines */
/* Attacktypes with grammar */
struct attack_hit_type {
   const char *singular;
   const char *plural;
};

/* Functions available in fight.c */
void appear(struct char_data *ch);
void check_killer(struct char_data *ch, struct char_data *vict);
int compute_armor_class(struct char_data *ch);
int damage(struct char_data *ch, struct char_data *victim, int dam, int attacktype);
void death_cry(struct char_data *ch);
void die(struct char_data * ch, struct char_data * killer);
void hit(struct char_data *ch, struct char_data *victim, int type);
void perform_violence(void);
void raw_kill(struct char_data * ch, struct char_data * killer);
void  set_fighting(struct char_data *ch, struct char_data *victim);
int skill_message(int dam, struct char_data *ch, struct char_data *vict,
          int attacktype);
void  stop_fighting(struct char_data *ch);


/* Global variables */
#ifndef __FIGHT_C__
extern struct attack_hit_type attack_hit_text[];
extern struct char_data *combat_list;
#endif /* __FIGHT_C__ */

#endif /* _FIGHT_H_*/
