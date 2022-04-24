/**
* @file kingdom.h
* Header file for kingdom specific functions and variables.
*
* Part of the core tbaMUD source code distribution, which is a derivative
* of, and continuation of, CircleMUD.
*
* All rights reserved.  See license for complete information.
* Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University
* CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.
*
*/
#ifndef _KINGDOM_H_
#define _KINGDOM_H_

/* Functions available through kingdom.c */
bitvector_t find_kingdom_bitvector(const char *arg);
int parse_kingdom(char arg);

/* Global variables */
extern const char *kingdom_abbrevs[];
extern const char *pc_kingdom_types[];
extern const char *kingdom_menu;
extern struct kingdom_info_type kingdom_info[];

#endif /* _KINGDOM_H_ */
