/**************************************************************************
*  File: prefedit.c                                        Part of tbaMUD *
*  Usage: Player-level OLC for setting preferences and toggles            *
*                                                                         *
*  Created by Jamdog for tbaMUD 3.59                                      *
*  All rights reserved.  See license for complete information.            *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
**************************************************************************/

/* Toggle structure held for OLC purposes */
struct prefs_data
{
  struct char_data *ch;            /* Pointer to char being edited   */
  int   pref_flags[PR_ARRAY_MAX];  /* Copy of player's pref flags    */
  int   wimp_level;                /* Copy of player's wimp level    */
  int   page_length;               /* Copy of player's pagelength    */
  int   screen_width;              /* Copy of player's screenwidth   */
};

/* Prefedit utility macros */

/** Get the character being edited */
#define PREFEDIT_GET_CHAR        (OLC_PREFS(d)->ch ? (OLC_PREFS(d)->ch) : (d->character))

/** Get a pointer to the pref toggles of the character being edited   */
#define PREFEDIT_GET_FLAGS       OLC_PREFS(d)->pref_flags

/** Get the wimpy level of the character being edited                 */
#define PREFEDIT_GET_WIMP_LEV    OLC_PREFS(d)->wimp_level

/** Get the page length of the character being edited                 */
#define PREFEDIT_GET_PAGELENGTH  OLC_PREFS(d)->page_length

/** Get the screen width of the character being edited                */
#define PREFEDIT_GET_SCREENWIDTH OLC_PREFS(d)->screen_width

/** Get one of the flag array elements for the character being edited */
#define PREFEDIT_GET_FLAG(i)     OLC_PREFS(d)->pref_flags[(i)]

/** Get the state of a flag (on/off) for the character being edited   */
#define PREFEDIT_FLAGGED(flag)  IS_SET_AR(PREFEDIT_GET_FLAGS, (flag))

/* Prefedit OLC sub-modes */
#define PREFEDIT_MAIN_MENU                0
#define PREFEDIT_PROMPT                   1
#define PREFEDIT_COLOR                    2
#define PREFEDIT_PAGELENGTH               3
#define PREFEDIT_SCREENWIDTH              4
#define PREFEDIT_WIMPY                    5
#define PREFEDIT_CONFIRM_SAVE             6
#define PREFEDIT_SYSLOG                   7
#define PREFEDIT_TOGGLE_MENU              8

/* External Functions in prefedit.c */
void prefedit_Restore_Defaults(struct descriptor_data *d);
void prefedit_parse(struct descriptor_data * d, char *arg);
ACMD(do_oasis_prefedit);

