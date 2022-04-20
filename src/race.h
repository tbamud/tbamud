/*
* @file race.h
* Header file for race specific functions and variables.
*
* Part of the core tbaMUD source code distribution, which is a derivative
* of, and continuation of, CircleMUD.
*
* All rights reserved.  See license for complete information.
* Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University
* CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.
*/
#ifndef _RACE_H_
#define _RACE_H_

/* Functions available through race.c. */
bitvector_t find_race_bitvector(const char *arg);
int parse_race(char arg);

/* Global variables */
extern const char *race_abbrevs[];
extern const char *pc_race_types[];
extern const char *race_menu;

#endif /* _RACE_H_ */
