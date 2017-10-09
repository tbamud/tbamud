/**
* @file boards.h
* Header file for the bulletin board system (boards.c).
* 
* Part of the core tbaMUD source code distribution, which is a derivative
* of, and continuation of, CircleMUD.
*                                                                        
* All rights reserved.  See license for complete information.                                                                
* Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University 
* CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.
* 
* @todo Utility functions that could easily be moved elsewhere have been
* marked. Suggest a review of all utility functions (aka. non ACMDs) and
* determine if the utility functions should be placed into a lower level
* shared module.               
*
*/
#ifndef _BAN_H_
#define _BAN_H_

/* don't change these */
#define BAN_NOT   0
#define BAN_NEW   1
#define BAN_SELECT  2
#define BAN_ALL   3

#define BANNED_SITE_LENGTH    50
struct ban_list_element {
   char site[BANNED_SITE_LENGTH+1];
   int  type;
   time_t date;
   char name[MAX_NAME_LENGTH+1];
   struct ban_list_element *next;
};

/* Global functions */
/* Utility Functions */
void load_banned(void);
int isbanned(char *hostname);
int valid_name(char *newname);
void read_invalid_list(void);
void free_invalid_list(void);
/* Command functions without subcommands */
ACMD(do_ban);
ACMD(do_unban);

extern struct ban_list_element *ban_list;
extern int num_invalid;

#endif /* _BAN_H_*/
