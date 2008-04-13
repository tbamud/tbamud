/**
* @file class.h
* Header file for class specific functions and variables.
* 
* Part of the core tbaMUD source code distribution, which is a derivative
* of, and continuation of, CircleMUD.
*                                                                        
* All rights reserved.  See license for complete information.                                                                
* Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University 
* CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               
*
*/
#ifndef _CLASS_H_
#define _CLASS_H_

/* Functions available through class.c */
int backstab_mult(int level);
void do_start(struct char_data *ch);
bitvector_t find_class_bitvector(const char *arg);
int invalid_class(struct char_data *ch, struct obj_data *obj);
int level_exp(int chclass, int level);
int parse_class(char arg);
void roll_real_abils(struct char_data *ch);
byte saving_throws(int class_num, int type, int level);
int thaco(int class_num, int level);
const char *title_female(int chclass, int level);
const char *title_male(int chclass, int level);

/* Global variables */

#ifndef __CLASS_C__  

extern const char *class_abbrevs[];
extern const char *pc_class_types[];
extern const char *class_menu;
extern int prac_params[][NUM_CLASSES];
extern struct guild_info_type guild_info[];

#endif /* __CLASS_C__ */

#endif /* _CLASS_H_*/
