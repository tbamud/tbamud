/**
* @file improved-edit.h
* The basic and improved editor.
* 
* Part of the core tbaMUD source code distribution, which is a derivative
* of, and continuation of, CircleMUD.
* 
* This set of code was not originally part of the circlemud distribution.                                                                                       
*/
#ifndef _IMPROVED_EDIT_H_
#define _IMPROVED_EDIT_H_

/* This is here to allow different code for the basic and improved editor. If 
 * you do not wish to use the improved editor, put #if 0 below, otherwise you 
 * should leave the setting at #if 1. */
#define CONFIG_IMPROVED_EDITOR	1

#define using_improved_editor	CONFIG_IMPROVED_EDITOR

void parse_edit_action(int command, char *string, struct descriptor_data *d);
int improved_editor_execute(struct descriptor_data *d, char *string);
int format_text(char **ptr_string, int mode, struct descriptor_data *d, unsigned int maxlen, int low, int high);
int replace_str(char **string, char *pattern, char *replacement, int rep_all, unsigned int max_size);
void send_editor_help(struct descriptor_data *d);

/* Action modes for parse_edit_action(). */
#define PARSE_FORMAT        0
#define PARSE_REPLACE       1
#define PARSE_HELP          2
#define PARSE_DELETE        3
#define PARSE_INSERT        4
#define PARSE_LIST_NORM	    5
#define PARSE_LIST_NUM      6
#define PARSE_EDIT          7
#define PARSE_TOGGLE        8

/* Defines for the action variable. */
#define STRINGADD_OK		0	/* Just keep adding text.		*/
#define STRINGADD_SAVE		1	/* Save current text.			*/
#define STRINGADD_ABORT		2	/* Abort edit, restore old text.	*/
#define STRINGADD_ACTION	4	/* Editor action, don't append \r\n.	*/

/* Settings for formatter. */
#define FORMAT_INDENT	(1 << 0)

#endif /* _IMPROVED_EDIT_H_ */
