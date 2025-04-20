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
void appear(char_data *ch);
void check_killer(char_data *ch, char_data *vict);
int compute_armor_class(char_data *ch);
int damage(char_data *ch, char_data *victim, int dam, int attacktype);
void death_cry(char_data *ch);
void die(char_data * ch, char_data * killer);
void hit(char_data *ch, char_data *victim, int type);
void perform_violence(void);
void raw_kill(char_data * ch, char_data * killer);
void  set_fighting(char_data *ch, char_data *victim);
int skill_message(int dam, char_data *ch, char_data *vict, int attacktype);
void  stop_fighting(char_data *ch);


/* Global variables */
extern struct attack_hit_type attack_hit_text[];
extern char_data *combat_list;

#endif /* _FIGHT_H_*/
