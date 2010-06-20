/**************************************************************************
*  File: ibt.h                                             Part of tbaMUD *
*  Usage: Loading/saving/editing of Ideas, Bugs and Typos lists           *
*                                                                         *
*  All rights reserved.  See license for complete information.            *
*                                                                         *
*  Created by Vash (aka Frenze) for Trigun MUD and the tbaMUD codebase    *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
**************************************************************************/

typedef struct  ibt_data               IBT_DATA;

#define MAX_IBT_LENGTH       2048
#define MAX_IBTNOTE_LENGTH   2048

/* Subcommands, also used for 'modes' */
#define SCMD_BUG  0
#define SCMD_IDEA 1
#define SCMD_TYPO 2

/* Flag array size (min = 4) */
#define IBT_ARRAY_MAX 4

/* List of flags for Ideas, Bugs and Typos */
#define IBT_RESOLVED    0
#define IBT_IMPORTANT   1
#define IBT_INPROGRESS  2

#define NUM_IBT_FLAGS 3

/* IBT Type returns 'Idea', 'Bug' or 'Typo' when in OLC */
#define IBT_TYPE         (ibt_types[(OLC_VAL(d))])

#define IBT_FLAGS(x)     ((x)->flags)
#define IBT_FLAGGED(x,y) (IS_SET_AR(((x)->flags), (y)))

/* IBT Editor OLC modes */
#define IBTEDIT_CONFIRM_SAVESTRING 1
#define IBTEDIT_MAIN_MENU          2
#define IBTEDIT_NAME               3
#define IBTEDIT_ROOM               4
#define IBTEDIT_TEXT               5
#define IBTEDIT_FLAGS              6
#define IBTEDIT_BODY               7
#define IBTEDIT_NOTES              8

#ifdef KEY
#undef KEY
#endif
#define KEY( literal, field, value )                                    \
                                if ( !str_cmp( word, literal ) )        \
                                {                                       \
                                    field  = value;                     \
                                    fMatch = TRUE;                      \
                                    break;                              \
                                }

/* TXT_KEY should be used with fread_line, as it uses a static string, so should be copied */
#ifdef TXT_KEY
#undef TXT_KEY
#endif
#define TXT_KEY( literal, field, value )                                \
                                if ( !str_cmp( word, literal ) )        \
                                {                                       \
                                    if (field) STRFREE(field);          \
                                    field  = STRALLOC(value);           \
                                    fMatch = TRUE;                      \
                                    break;                              \
                                }

struct ibt_data
{
  IBT_DATA   *next;                 /**< Pointer to next IBT in the list           */
  IBT_DATA   *prev;                 /**< Pointer to previous IBT in the list       */
  char       *text;                 /**< Header Text for this IBT                  */
  char       *body;                 /**< Body Text for this IBT                    */
  char       *name;                 /**< Name of the person who reported this IBT  */
  char       *notes;                /**< Resolution Notes added by Administrators  */
  int        level;                 /**< Level of the person who reported this IBT */
  room_vnum  room;                  /**< Room in which this IBT was reported       */
  int        flags[IBT_ARRAY_MAX];  /**< IBT flags                                 */
};

extern  IBT_DATA       *first_bug;
extern  IBT_DATA       *last_bug;
extern  IBT_DATA       *first_idea;
extern  IBT_DATA       *last_idea;
extern  IBT_DATA       *first_typo;
extern  IBT_DATA       *last_typo;

/* Functions in ibt.c that are used externally */
ACMD(do_ibt);
ACMD(do_oasis_ibtedit);
void save_ibt_file(int mode);
void load_ibt_file(int mode);
void ibtedit_parse(struct descriptor_data *d, char *arg);
void ibtedit_string_cleanup(struct descriptor_data *d, int terminator);
void free_ibt_lists();
