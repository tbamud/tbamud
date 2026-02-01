/**
* @file py_olc.c
* 
* This set of code was not originally part of the circlemud distribution.
*/

#ifndef _PY_OLC_H_
#define _PY_OLC_H_

#include "py_triggers.h"

#define NUM_TRIG_TYPE_FLAGS		21

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


#endif /* _PY_OLC_H_ */
