/**
* @file modify.h
* Header file for the modify module.
* 
* Part of the core tbaMUD source code distribution, which is a derivative
* of, and continuation of, CircleMUD.
*                                                                        
* All rights reserved.  See license for complete information.                                                                
* Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University 
* CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.
* 
* @todo This module is originally defined as 'Run-time modification of game 
* variables.' I believe it has digressed from the original intent. This
* header file is created to help redefine (over time) what the heck modify.h
* is. For example, publicly functions declared in comm.h but defined in modify.c
* should have their declarations moved here.  
*
*/
#ifndef _MODIFY_H_
#define _MODIFY_H_

/* Public functions */
void show_string(struct descriptor_data *d, char *input);
void smash_tilde(char *str);
void parse_at(char *str);
void parse_tab(char *str);
void paginate_string(char *str, struct descriptor_data *d);
/** @todo should this really be in modify.c? */
ACMD(do_skillset);
/* Following function prototypes moved here from comm.h */
void  string_write(struct descriptor_data *d, char **txt, size_t len, long mailto, void *data);
void  string_add(struct descriptor_data *d, char *str);
void  page_string(struct descriptor_data *d, char *str, int keep_internal);
/* page string function & defines */
#define PAGE_LENGTH 22
#define PAGE_WIDTH  80

#endif /* _MODIFY_H_*/
