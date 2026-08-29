/**
* @file dg_olc.h
* This source file is used in extending Oasis OLC for trigedit.
* 
* Part of the core tbaMUD source code distribution, which is a derivative
* of, and continuation of, CircleMUD.
* 
* This source code, which was not part of the CircleMUD legacy code,
* was created by the following people:                                      
* $Author: Mark A. Heilpern/egreen/Welcor $                              
* $Date: 2004/10/11 12:07:00$                                            
* $Revision: 1.0.14 $                                                    
*/
#ifndef _DG_OLC_H_
#define _DG_OLC_H_

#include "dg_scripts.h"

#define NUM_TRIG_TYPE_FLAGS		20

/* Submodes of TRIGEDIT connectedness. */
#define TRIGEDIT_MAIN_MENU              0
#define TRIGEDIT_TRIGTYPE               1
#define TRIGEDIT_CONFIRM_SAVESTRING	2
#define TRIGEDIT_NAME			3
#define TRIGEDIT_INTENDED		4
#define TRIGEDIT_TYPES			5
#define TRIGEDIT_COMMANDS		6
#define TRIGEDIT_NARG			7
#define TRIGEDIT_ARGUMENT		8
#define TRIGEDIT_COPY                   9

#define OLC_SCRIPT_EDIT		    82766  /* arbitrary > highest possible room number */
#define SCRIPT_MAIN_MENU		0
#define SCRIPT_NEW_TRIGGER		1
#define SCRIPT_DEL_TRIGGER		2

#define OLC_SCRIPT_EDIT_MODE(d)	(OLC(d)->script_mode)	/* parse input mode */
#define OLC_SCRIPT(d)           (OLC(d)->script)	/* script editing   */
#define OLC_ITEM_TYPE(d)	(OLC(d)->item_type)	/* mob/obj/room     */

/* prototype exported functions from dg_olc.c */
void script_save_to_disk(FILE *fp, void *item, int type);
void dg_olc_script_copy(struct descriptor_data *d);
void dg_script_menu(struct descriptor_data *d);
int dg_script_edit_parse(struct descriptor_data *d, char *arg);


#endif /* _DG_OLC_H_ */
