/**************************************************************************
*  File: ibt.c                                             Part of tbaMUD *
*  Usage: Loading/saving/editing of Ideas, Bugs and Typos lists           *
*                                                                         *
*  All rights reserved.  See license for complete information.            *
*                                                                         *
*  Created by Vash (aka Frenze) for Trigun MUD and the tbaMUD codebase    *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
**************************************************************************/

//#if defined(macintosh)
//#include <types.h>
//#include <time.h>
//#else
//#include <sys/types.h>
//#include <sys/time.h>
//#endif
#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "handler.h"
#include "interpreter.h"
//#include "genobj.h"
#include "constants.h"
//#include "fight.h"
#include "screen.h"
#include "act.h"
#include "utils.h"
#include "ibt.h"
#include "oasis.h"
#include "improved-edit.h"
#include "modify.h"

IBT_DATA * first_bug  = NULL;
IBT_DATA * last_bug   = NULL;
IBT_DATA * first_idea = NULL;
IBT_DATA * last_idea  = NULL;
IBT_DATA * first_typo = NULL;
IBT_DATA * last_typo  = NULL;

const char *ibt_types[]={
  "Bug",
  "Idea",
  "Typo",
  "\n"
};

/* Internal (static) functions */
static IBT_DATA *new_ibt(void);
static void free_ibt_list(IBT_DATA *first_ibt, IBT_DATA *last_ibt);
static IBT_DATA *read_ibt(char *filename, FILE *fp);
static IBT_DATA *get_first_ibt(int mode);
static IBT_DATA *get_last_ibt(int mode);
static bool is_ibt_logger(IBT_DATA *ibtData, struct char_data *ch);
/* Internal (static) OLC functions */
static void ibtedit_setup(struct descriptor_data *d);
static void ibtedit_save(struct descriptor_data *d);
static void ibtedit_disp_main_menu(struct descriptor_data *d);
static void ibtedit_disp_flags(struct descriptor_data *d);

static IBT_DATA *new_ibt(void)
{
   int i;
   IBT_DATA *ibtData;

   CREATE( ibtData, IBT_DATA, 1);

   ibtData->next       = NULL;
   ibtData->prev       = NULL;
   ibtData->name       = NULL;
   ibtData->text       = NULL;
   ibtData->body       = NULL;
   ibtData->notes      = NULL;
   ibtData->level      = 0;
   ibtData->id_num     = NOBODY;
   ibtData->room       = NOWHERE;
   ibtData->dated      = 0;

   for (i=0; i<IBT_ARRAY_MAX; i++)
     ibtData->flags[i]   = 0;

   return ibtData;
}

static void free_ibt_list(IBT_DATA *first_ibt, IBT_DATA *last_ibt)
{
  IBT_DATA **top_ptr, **bot_ptr, *this_ibt;

  if ((first_ibt == NULL) || (last_ibt == NULL)) return;

  top_ptr = &(first_ibt);
  bot_ptr = &(last_ibt);

  while (first_ibt) {
    this_ibt = first_ibt;

    /* Disconnect from the list */
    UNLINK(this_ibt, first_ibt, last_ibt, next, prev);

    /* Free the strings first */
    if (this_ibt->body)  STRFREE(this_ibt->body);
    if (this_ibt->name)  STRFREE(this_ibt->name);
    if (this_ibt->text)  STRFREE(this_ibt->text);
    if (this_ibt->notes) STRFREE(this_ibt->notes);

    /* And free the struct */
    DISPOSE(this_ibt);
  }

  *(top_ptr) = NULL;
  *(bot_ptr) = NULL;
}
static IBT_DATA *read_ibt( char *filename, FILE *fp )
{
   IBT_DATA *ibtData;
   char *word, *id_num=NULL, *dated=NULL;
   char buf[MAX_STRING_LENGTH];
   bool fMatch, flgCheck;
   char letter;

   do
   {
      letter = getc( fp );
      if( feof(fp) )
      {
         fclose( fp );
         return NULL;
      }
   }
   while( isspace(letter) );
   ungetc( letter, fp );

   ibtData = new_ibt();
   ibtData->name       = STRALLOC("");
   ibtData->text       = STRALLOC("");
   ibtData->body       = STRALLOC("");

   for ( ; ; )
   {
        word   = feof( fp ) ? "End" : fread_word( fp );
        fMatch = FALSE;

        switch ( UPPER(word[0]) )
        {
          case 'B':
            if (!str_cmp(word, "Body"))  STRFREE(ibtData->body);
            KEY("Body",     ibtData->body,   fread_clean_string( fp, buf ));
            break;
          case 'D':
            TXT_KEY("Dated", dated, fread_line(fp));          
            break;
          case 'E':
            if (!str_cmp(word, "End"))
            {
              if ( id_num ) {
                ibtData->id_num = atol(id_num);
                STRFREE( id_num );
              }  
              if ( dated ) {
                ibtData->dated = atol(dated);
                STRFREE( dated );
              }  
              if ( !ibtData->name )
                ibtData->name = STRALLOC("");
              if ( !ibtData->text )
                ibtData->text = STRALLOC("");
              if ( !ibtData->body )
                ibtData->body = STRALLOC("");
              if ( !ibtData->notes )
                ibtData->notes = STRALLOC("");
              return ibtData;
            }
            break;

          case 'F':
            KEY("Flags",    flgCheck,         fread_flags(fp, ibtData->flags, IBT_ARRAY_MAX));
            break;

          case 'I':
            TXT_KEY("IdNum", id_num, fread_line(fp));
            break;

          case 'L':
            KEY("Level",    ibtData->level,   fread_number(fp));
            break;

          case 'N':
            if (!str_cmp(word, "Name") && ibtData->name)   STRFREE(ibtData->name);
            if (!str_cmp(word, "Notes") && ibtData->notes)  STRFREE(ibtData->notes);
            TXT_KEY("Name",     ibtData->name,    fread_line( fp ));
            KEY("Notes",        ibtData->notes,   fread_clean_string( fp, buf ));
            break;

          case 'R':
            KEY("Room",     ibtData->room,    fread_number(fp));
            break;

          case 'T':
            if (!str_cmp(word, "Text"))  STRFREE(ibtData->text);
            TXT_KEY("Text",     ibtData->text,    fread_line( fp ));
            break;

          case '*':  /* Handle comments */
            fread_to_eol(fp);
            fMatch = TRUE;
            break;

          default:
            log("SYSERR: Invalid keyword (%s) in IBT file", word);
            break;
        }
        if ( !fMatch )
        {
          fread_to_eol(fp);
          log( "read_ibt (%s): no match: %s - aborting line!", filename, word );
        }
   }
   log ("read_ibt: Aborted! Returning NULL");
   if ( ibtData->name)
         STRFREE( ibtData->name);
   if ( ibtData->text)
         STRFREE( ibtData->text);
   if ( ibtData->body)
         STRFREE( ibtData->body);
   if ( id_num )
		 STRFREE( id_num );
   if ( dated )
		 STRFREE( dated );
		 
   DISPOSE( ibtData);
   return NULL;
}

void load_ibt_file(int mode)
{
   char filename[MAX_INPUT_LENGTH];
   FILE *fp;
   IBT_DATA *ibtData, *first_ibt, *last_ibt;

   switch(mode) {
     case SCMD_BUG : sprintf( filename, "%s",BUGS_FILE );
                     first_ibt = first_bug;
                     last_ibt  = last_bug;
                     break;
     case SCMD_IDEA: sprintf( filename, "%s",IDEAS_FILE );
                     first_ibt = first_idea;
                     last_ibt  = last_idea;
                     break;
     case SCMD_TYPO: sprintf( filename, "%s",TYPOS_FILE );
                     first_ibt = first_typo;
                     last_ibt  = last_typo;
                     break;
     default       : log("SYSERR: Invalid mode (%d) in load_ibt_file", mode);
                     return;
   }

   /* Remove and free the old list */
   if ((first_ibt) && (last_ibt)) {
     free_ibt_list(first_ibt, last_ibt);
     first_ibt = NULL;
     last_ibt = NULL;
   }

   if( ( fp = fopen( filename, "r" ) ) == NULL ) {
      log("No File: %s", filename);
      return;
   }

   while( ( ibtData = read_ibt( filename, fp ) ) != NULL ) {
     switch(mode) {
       case SCMD_BUG : LINK( ibtData, first_bug, last_bug, next, prev );
                       break;
       case SCMD_IDEA: LINK( ibtData, first_idea, last_idea, next, prev );
                       break;
       case SCMD_TYPO: LINK( ibtData, first_typo, last_typo, next, prev );
                       break;
     }
   }

   return;
}

void save_ibt_file(int mode)
{
   IBT_DATA *ibtData, *first_ibt, *last_ibt;
   FILE *fp;
   char filename[256];

   switch(mode) {
     case SCMD_BUG : sprintf( filename, "%s",BUGS_FILE );
                     first_ibt = first_bug;
                     last_ibt  = last_bug;
                     break;
     case SCMD_IDEA: sprintf( filename, "%s",IDEAS_FILE );
                     first_ibt = first_idea;
                     last_ibt  = last_idea;
                     break;
     case SCMD_TYPO: sprintf( filename, "%s",TYPOS_FILE );
                     first_ibt = first_typo;
                     last_ibt  = last_typo;
                     break;
     default       : log("SYSERR: Invalid mode (%d) in save_ibt_file", mode);
                     return;
   }

   if (( fp = fopen(filename,"w")) == NULL)
   {
     log("SYSERR: Unable to open IBT file for writing in save_ibt_file");
     log("        IBT File: %s", filename);
     return;
   }
   else
   {
     for (ibtData = first_ibt;ibtData;ibtData=ibtData->next)
     {
       if (ibtData->text && *(ibtData->text))
         fprintf(fp,"Text      %s~\n",ibtData->text);
       if (ibtData->body && *(ibtData->body))
         fprintf(fp,"Body      %s~\n",ibtData->body);
       if (ibtData->name && *(ibtData->name))
         fprintf(fp,"Name      %s~\n",ibtData->name);
       if (ibtData->notes && *(ibtData->notes))
         fprintf(fp,"Notes     %s~\n",ibtData->notes);
       if (ibtData->id_num != NOBODY)
         fprintf(fp,"IdNum     %ld\n",ibtData->id_num);
       if (ibtData->dated != 0)
         fprintf(fp,"Dated     %ld\n",ibtData->dated);
       fprintf(fp,"Level     %d\n",ibtData->level);
       fprintf(fp,"Room      %d\n",ibtData->room);
       fprintf(fp,"Flags     %d %d %d %d\n",ibtData->flags[0],ibtData->flags[1],
                                            ibtData->flags[2],ibtData->flags[3]);
       fprintf(fp,"End\n");
     }
     fclose(fp);
     return;
   }
}

static IBT_DATA *get_first_ibt(int mode)
{
  IBT_DATA *first_ibt = NULL;

  switch(mode) {
    case SCMD_BUG : first_ibt = first_bug;
                    break;
    case SCMD_IDEA: first_ibt = first_idea;
                    break;
    case SCMD_TYPO: first_ibt = first_typo;
                    break;
    default       : log("SYSERR: Invalid mode (%d) in get_first_ibt", mode);
                    break;
  }
  return (first_ibt);
}

static IBT_DATA *get_last_ibt(int mode)
{
  IBT_DATA *last_ibt = NULL;

  switch(mode) {
    case SCMD_BUG : last_ibt = last_bug;
                    break;
    case SCMD_IDEA: last_ibt = last_idea;
                    break;
    case SCMD_TYPO: last_ibt = last_typo;
                    break;
    default       : log("SYSERR: Invalid mode (%d) in get_last_ibt", mode);
                    break;
  }
  return (last_ibt);
}
IBT_DATA *get_ibt_by_num(int mode, int target_num)
{
  int no=0;
  IBT_DATA *target_ibt, *first_ibt;

  if ((first_ibt = get_first_ibt(mode)) == NULL) return NULL;

  for (target_ibt=first_ibt;target_ibt;target_ibt=target_ibt->next) {
    no++;
    if (no==target_num) {
      return target_ibt;
    }
  }
  return NULL;
}

/* Search the IBT list, and return true if ibt is found there */
bool ibt_in_list(int mode, IBT_DATA *ibt)
{
  IBT_DATA *target_ibt, *first_ibt;

  if ((first_ibt = get_first_ibt(mode)) == NULL) return FALSE;

  for (target_ibt=first_ibt;target_ibt;target_ibt=target_ibt->next) {
    if (target_ibt == ibt) {
      return TRUE;
    }
  }
  return FALSE;
}

/* Free up an IBT struct, removing it from the list if necessary */
/* returns TRUE on success                                       */
bool free_ibt(int mode, IBT_DATA *ibtData)
{
  if (ibtData == NULL) return FALSE;

  /* If this IBT is in the IBT list, take it out */
  if (ibt_in_list(mode, ibtData)) {
	switch(mode) {
      case SCMD_BUG : UNLINK(ibtData,first_bug,last_bug,next,prev);
                      break;
      case SCMD_IDEA: UNLINK(ibtData,first_idea,last_idea,next,prev);
                      break;
      case SCMD_TYPO: UNLINK(ibtData,first_typo,last_typo,next,prev);
                      break;
    }
  }

  /* Free the strings first */
  if (ibtData->body) STRFREE(ibtData->body);
  if (ibtData->name) STRFREE(ibtData->name);
  if (ibtData->text) STRFREE(ibtData->text);

  /* And free the struct */
  DISPOSE(ibtData);

  return TRUE;
}

/* Return TRUE if 'ch' is the person who logged the IBT */
static bool is_ibt_logger(IBT_DATA *ibtData, struct char_data *ch)
{
  if ( ch && !IS_NPC(ch) && ibtData ) {

    /* Check the ID number first (in case of name change)   */
    if ((ibtData->id_num != NOBODY) && (ibtData->id_num == GET_IDNUM(ch)))
      return TRUE;

    /* Check the name next (in case of deletion/recreation) */
    if (strcmp(ibtData->name, GET_NAME(ch)) == 0)
      return TRUE;
  }
  return FALSE;
}

ACMD(do_ibt)
{
  char arg[MAX_STRING_LENGTH], arg2[MAX_STRING_LENGTH];
  char buf[MAX_STRING_LENGTH], *arg_text, imp[30];
  int  i, num_res, num_unres;
  IBT_DATA *ibtData, *first_ibt, *last_ibt;
  int ano=0;

  if (IS_NPC(ch))
    return;

  arg_text  = one_argument(argument, arg);
  argument  = two_arguments(argument, arg, arg2);

  first_ibt = get_first_ibt(subcmd);
  last_ibt  = get_last_ibt(subcmd);

  if ((!*arg)){
    if (GET_LEVEL(ch) >= LVL_GRGOD){
      send_to_char(ch, "Usage: %s%s submit <header>%s\r\n"
                       "       %s%s list%s\r\n"
                       "       %s%s show <num>%s\r\n"
                       "       %s%s remove <num>%s\r\n"
                       "       %s%s edit <num>%s\r\n"
                       "       %s%s resolve <num>%s\r\n",
                               QYEL, CMD_NAME, QNRM,
                               QYEL, CMD_NAME, QNRM,
                               QYEL, CMD_NAME, QNRM,
                               QYEL, CMD_NAME, QNRM,
                               QYEL, CMD_NAME, QNRM,
                               QYEL, CMD_NAME, QNRM);
      return;
    } else if (GET_LEVEL(ch) >= LVL_IMMORT) {
      send_to_char(ch, "Usage: %s%s submit <header>%s\r\n"
                       "       %s%s list%s\r\n"
                       "       %s%s show <num>%s\r\n",
                               QYEL, CMD_NAME, QNRM,
                               QYEL, CMD_NAME, QNRM,
                               QYEL, CMD_NAME, QNRM);
      return;
    } else {
      send_to_char(ch, "Usage: %s%s submit <header>%s\r\n"
                       "       %s%s list%s\r\n"
                       "       %s%s show <num>%s\r\n",
                               QYEL, CMD_NAME, QNRM,
                               QYEL, CMD_NAME, QNRM,
                               QYEL, CMD_NAME, QNRM);
      send_to_char(ch, "Note: Only %ss logged by you will be listed or shown.\r\n", CMD_NAME);
      return;
    }
  }
  else if(is_abbrev(arg,"show"))
  {
    if (!is_number(arg2)) {
      send_to_char(ch, "Show which %s?\r\n", CMD_NAME);
      return;
    }
    ano = atoi(arg2);

    if ((ibtData = get_ibt_by_num(subcmd, ano)) == NULL) {
      send_to_char(ch, "That %s doesn't exist.\r\n", CMD_NAME);
      return;
    } else {
      if ((GET_LEVEL(ch) < LVL_IMMORT) && (!is_ibt_logger(ibtData, ch))) {
        send_to_char(ch, "Sorry but you may only view %ss you have posted yourself.\n\r", ibt_types[subcmd]);
      } else {

        send_to_char(ch, "%s%s by %s%s\r\n",QCYN, ibt_types[subcmd], QYEL, ibtData->name);
        send_to_char(ch, "%sSubmitted: %s%s", QCYN, QYEL, ibtData->dated ? ctime(&ibtData->dated) : "Unknown\r\n");
        if (GET_LEVEL(ch) >= LVL_IMMORT) {
          send_to_char(ch, "%sLevel: %s%d\r\n",QCYN, QYEL, ibtData->level);
          send_to_char(ch, "%sRoom : %s%d\r\n",QCYN, QYEL, ibtData->room);
        }
        send_to_char(ch, "%sTitle: %s%s\r\n",QCYN, QYEL, ibtData->text);
        send_to_char(ch, "%s%s Details%s\r\n%s\r\n",QCYN, ibt_types[subcmd], QYEL, ibtData->body);
        if (ibtData->notes && *(ibtData->notes))
          send_to_char(ch, "%s%s Notes%s\r\n%s\r\n",QCYN, ibt_types[subcmd], QYEL, ibtData->notes);
        send_to_char(ch, "%s%s Status: %s%s%s%s\r\n",QCYN, ibt_types[subcmd],
                                       IBT_FLAGGED(ibtData, IBT_RESOLVED) ? QBGRN : QBRED,
                                       IBT_FLAGGED(ibtData, IBT_RESOLVED) ? "Resolved" : "Unresolved",
                                       IBT_FLAGGED(ibtData, IBT_INPROGRESS) ? " (In Progress)" : "",
                                       QNRM);
      }
    }

    return;
  }
  else if(is_abbrev(arg,"list"))
  {

    if (first_ibt)
    {
      if (GET_LEVEL(ch) < LVL_IMMORT) {
        send_to_char(ch,"%s No %s|%s Description\r\n", QCYN, QGRN, QCYN);
        send_to_char(ch,"%s ---|--------------------------------------------------%s\r\n", QGRN, QNRM);
      } else {
        send_to_char(ch,"%s No %s|%sName        %s|%sRoom  %s|%sLevel%s|%s Description\r\n", QCYN, QGRN, QCYN, QGRN, QCYN, QGRN, QCYN, QGRN, QCYN);
        send_to_char(ch,"%s ---|------------|------|-----|--------------------------------------------------%s\r\n", QGRN, QNRM);
      }
      i=num_res=num_unres=0;
      for (ibtData=first_ibt;ibtData;ibtData = ibtData->next) {
        i++;

        /* For mortals, skip IBT's that they didn't log */
        if ((GET_LEVEL(ch) < LVL_IMMORT) && !is_ibt_logger(ibtData,ch))
          continue;

        /* Set up the 'important' flag */
        if (IBT_FLAGGED(ibtData, IBT_IMPORTANT))
          sprintf(imp, "%s!%s", QBFRED, QNRM);
        else
          sprintf(imp, "%c", ' ');

        if (IBT_FLAGGED(ibtData, IBT_RESOLVED)) {
          if (GET_LEVEL(ch) < LVL_IMMORT) {
            send_to_char(ch, "%s%s%3d|%s%s\r\n",
                                  imp, QGRN, i, ibtData->text, QNRM);
          } else {
            send_to_char(ch, "%s%s%3d%s|%s%-12s%s|%s%6d%s|%s%5d%s|%s%s%s\r\n",
                                  imp, QGRN, i, QGRN,
                                  QGRN, ibtData->name, QGRN,
                                  QGRN, ibtData->room, QGRN,
                                  QGRN, ibtData->level, QGRN,
                                  QGRN, ibtData->text, QNRM);
          }
          num_res++;
        } else if (IBT_FLAGGED(ibtData, IBT_INPROGRESS)) {
          if (GET_LEVEL(ch) < LVL_IMMORT) {
            send_to_char(ch, "%s%s%3d%s|%s%s%s\r\n",
                                  imp, QYEL, i, QGRN,
                                  QYEL, ibtData->text, QNRM);
          } else {
            send_to_char(ch, "%s%s%3d%s|%s%-12s%s|%s%6d%s|%s%5d%s|%s%s%s\r\n",
                                  imp, QYEL, i, QGRN,
                                  QYEL, ibtData->name, QGRN,
                                  QYEL, ibtData->room, QGRN,
                                  QYEL, ibtData->level, QGRN,
                                  QYEL, ibtData->text, QNRM);
          }
          num_unres++;
        } else {
          if (GET_LEVEL(ch) < LVL_IMMORT) {
            send_to_char(ch, "%s%s%3d%s|%s%s%s\r\n",
                                  imp, QRED, i, QGRN,
                                  QRED, ibtData->text, QNRM);
          } else {
            send_to_char(ch, "%s%s%3d%s|%s%-12s%s|%s%6d%s|%s%5d%s|%s%s%s\r\n",
                                  imp, QRED, i, QGRN,
                                  QRED, ibtData->name, QGRN,
                                  QRED, ibtData->room, QGRN,
                                  QRED, ibtData->level, QGRN,
                                  QRED, ibtData->text, QNRM);
          }
          num_unres++;
        }
      }
      if ((num_res + num_unres) > 0) {
        send_to_char(ch,"\n\r%s%d %ss in file. %s%d%s resolved, %s%d%s unresolved%s\r\n",QCYN, i, CMD_NAME, QBGRN, num_res, QCYN, QBRED, num_unres, QCYN, QNRM);
        send_to_char(ch,"%s%ss in %sRED%s are unresolved %ss.\r\n", QCYN, ibt_types[subcmd], QRED, QCYN, CMD_NAME);
        send_to_char(ch,"%s%ss in %sYELLOW%s are in-progress %ss.\r\n", QCYN, ibt_types[subcmd], QYEL, QCYN, CMD_NAME);
        send_to_char(ch,"%s%ss in %sGREEN%s are resolved %ss.\r\n", QCYN, ibt_types[subcmd], QGRN, QCYN, CMD_NAME);
      } else {
        send_to_char(ch,"No %ss have been found that were reported by you!\r\n", CMD_NAME);
      }
      if (GET_LEVEL(ch) >= LVL_GRGOD) {
        send_to_char(ch,"%sYou may use %s remove, resolve or edit to change the list..%s\r\n", QCYN, CMD_NAME, QNRM);
      }
      send_to_char(ch,"%sYou may use %s%s show <number>%s to see more indepth about the %s.%s\r\n", QCYN, QYEL, CMD_NAME, QCYN, CMD_NAME, QNRM);
    } else {
      send_to_char(ch,"No %ss have been reported!\r\n", CMD_NAME);
    }
    return;
  }
  else if (is_abbrev(arg,"submit"))
  {
    if (!*arg_text) {
      send_to_char(ch, "You need to add a heading!\r\n");
      return;
    }
    switch (subcmd) {
      case SCMD_IDEA: SET_BIT_AR(PLR_FLAGS(ch), PLR_IDEA);
                      break;
      case SCMD_BUG : SET_BIT_AR(PLR_FLAGS(ch), PLR_BUG);
                      break;
      case SCMD_TYPO: SET_BIT_AR(PLR_FLAGS(ch), PLR_TYPO);
                      break;
      default       : log("Invalid subcmd (%d) in do_ibt", subcmd);
                      return;
	}
    SET_BIT_AR(PLR_FLAGS(ch), PLR_WRITING);

    ibtData = new_ibt();
    send_to_char(ch, "Write your %s.\r\n", CMD_NAME);
    send_editor_help(ch->desc);

    sprintf(buf, "$n starts to give %s %s.", TANA(CMD_NAME), CMD_NAME);
    act(buf, TRUE, ch, 0, 0, TO_ROOM);

    string_write(ch->desc, &(ibtData->body),MAX_IBT_LENGTH, 0, NULL);

    ibtData->room   = GET_ROOM_VNUM(IN_ROOM(ch));
    ibtData->level  = GET_LEVEL(ch);
    ibtData->text   = STRALLOC(arg_text);
    ibtData->name   = STRALLOC(GET_NAME(ch));
    ibtData->id_num = GET_IDNUM(ch);
    ibtData->dated  = time(0);
    
    switch(subcmd) {
       case SCMD_BUG : LINK( ibtData, first_bug, last_bug, next, prev );
                       break;
       case SCMD_IDEA: LINK( ibtData, first_idea, last_idea, next, prev );
                       break;
       case SCMD_TYPO: LINK( ibtData, first_typo, last_typo, next, prev );
                       break;
    }
    mudlog(NRM,LVL_IMMORT, FALSE, "%s has posted %s %s!", GET_NAME(ch), TANA(CMD_NAME), CMD_NAME);
    return;
  }
  else if (is_abbrev(arg,"resolve"))
  {
    if (GET_LEVEL(ch) < LVL_GRGOD){
      send_to_char(ch, "%s what?\r\n", ibt_types[subcmd]);
      return;
    }

    if (!is_number(arg2)) {
      send_to_char(ch, "Resolve which %s?\r\n", CMD_NAME);
      return;
    }
    ano = atoi(arg2);

    if ((ibtData = get_ibt_by_num(subcmd, ano)) == NULL) {
      send_to_char(ch, "%s not found\r\n", ibt_types[subcmd]);
      return;
    } else {
      if (IBT_FLAGGED(ibtData, IBT_RESOLVED)){
        send_to_char(ch, "That %s has already been resolved!\r\n", CMD_NAME);
      } else {
        send_to_char(ch,"%s %d resolved!\r\n", ibt_types[subcmd], ano);
        SET_BIT_AR(IBT_FLAGS(ibtData), IBT_RESOLVED);
        if (CONFIG_IBT_AUTOSAVE) {
		  save_ibt_file(subcmd);
		}
      }
    }
    return;
  } else if (is_abbrev(arg,"remove")) {
    if (GET_LEVEL(ch) < LVL_GRGOD){
      send_to_char(ch, "%s what?\r\n", ibt_types[subcmd]);
      return;
    }

    if (!is_number(arg2)) {
      send_to_char(ch, "Remove which %s?\r\n", CMD_NAME);
      return;
    }
    ano = atoi(arg2);

    if ((ibtData = get_ibt_by_num(subcmd, ano)) == NULL) {
      send_to_char(ch, "%s not found\r\n", ibt_types[subcmd]);
      return;
    } else {
      if (free_ibt(subcmd, ibtData)) {
        send_to_char(ch,"%s%s Number %d removed.%s\r\n", QCYN, ibt_types[subcmd], ano, QNRM);
        if (CONFIG_IBT_AUTOSAVE) {
		  save_ibt_file(subcmd);
		}
      } else {
        send_to_char(ch,"%sUnable to remove %s %d!%s\r\n", QRED, CMD_NAME, ano, QNRM);
      }
    }
    return;
  } else if (is_abbrev(arg,"save")) {
    if (GET_LEVEL(ch) < LVL_GRGOD){
      send_to_char(ch, "%s what?\r\n", ibt_types[subcmd]);
      return;
    }
    save_ibt_file(subcmd);
    send_to_char(ch,"%s list saved.\r\n", ibt_types[subcmd]);
  } else if (is_abbrev(arg,"edit")) {
    if (GET_LEVEL(ch) < LVL_GRGOD){
      send_to_char(ch, "%s what?\r\n", ibt_types[subcmd]);
      return;
    }
    /* Pass control to the OLC without the 'edit' arg */
    do_oasis_ibtedit(ch, arg_text, cmd, subcmd);
  } else {
    if (GET_LEVEL(ch) < LVL_GRGOD){
      send_to_char(ch, "%s what?\r\n", ibt_types[subcmd]);
      send_to_char(ch, "Usage: %s submit <text>\r\n", ibt_types[subcmd]);
      return;
    } else {
      send_to_char(ch, "Usage:  %s (submit/list/show/remove/resolve)\r\n", CMD_NAME);
      return;
    }
  }
}

/* IBT Editor OLC */
/* OLC_VAL(d)  - The IBT 'mode' - Idea, Bug or Typo */
/* OLC_NUM(d)  - The IBT number (shown in 'list')   */
/* OLC_ZNUM(d) - Used as 'has changed' flag         */
ACMD(do_oasis_ibtedit)
{
  int number = NOTHING;
  struct descriptor_data *d;
  char buf1[MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH], *buf3;

  /* No editing as a mob or while being forced. */
  if (IS_NPC(ch) || !ch->desc || STATE(ch->desc) != CON_PLAYING)
    return;

  /* Parse any arguments */
  buf3 = two_arguments(argument, buf1, buf2);

  if (!*buf1) {
    send_to_char(ch, "Specify a %s number to edit.\r\n", ibt_types[subcmd]);
    return;
  } else if (!isdigit(*buf1)) {
    send_to_char(ch, "Yikes!  Stop that, someone will get hurt!\r\n");
    return;
  }

  /* If a numeric argument was given (like a room number), get it. */
  if (number == NOTHING)
    number = atoi(buf1);

  /* Validate that the IBT number exists */
  if (get_ibt_by_num(subcmd, number) == NULL) {
    send_to_char(ch, "That %s number doesn't exist!\r\n", ibt_types[subcmd]);
    return;
  }

  /* Check that whatever it is isn't already being edited. */
  for (d = descriptor_list; d; d = d->next) {
    if (STATE(d) == CON_IBTEDIT) {
      if (d->olc && OLC_NUM(d) == number) {
        send_to_char(ch, "That %s is currently being edited by %s.\r\n",
          ibt_types[subcmd], GET_NAME(d->character));
        return;
      }
    }
  }

  d = ch->desc;

  /* Give descriptor an OLC structure. */
  if (d->olc) {
    mudlog(BRF, LVL_IMMORT, TRUE, "SYSERR: do_oasis_ibtedit: Player already had olc structure.");
    free(d->olc);
  }

  CREATE(d->olc, struct oasis_olc_data, 1);

  /* Set OLC variables. */
  OLC_NUM(d)  = number;
  OLC_VAL(d)  = subcmd;
  OLC_ZNUM(d) = 0;

  /* setup the OLC structure. */
  ibtedit_setup(d);

  /* Show the main IBT edit menu                              */
  ibtedit_disp_main_menu(d);
  STATE(d) = CON_IBTEDIT;

  /* Display the OLC messages to the players in the same room as the
     editor and also log it. */
  act("$n starts using OLC.", TRUE, d->character, 0, 0, TO_ROOM);
  SET_BIT_AR(PLR_FLAGS(ch), PLR_WRITING);

  mudlog(CMP, LVL_IMMORT, TRUE,"OLC: %s starts editing %s %d",
    GET_NAME(ch), IBT_TYPE, OLC_NUM(d));
}

/*-------------------------------------------------------------------*/
/* Copy IBT data into the OLC structure                              */
static void ibtedit_setup(struct descriptor_data *d)
{
  IBT_DATA *ibtData;
  int i;

  /* Set up a blank IBT struct */
  OLC_IBT(d) = new_ibt();

  /* Should always be the case, but just to make sure */
  if ((ibtData = get_ibt_by_num(OLC_VAL(d), OLC_NUM(d))) != NULL) {
    if ((ibtData->text) && *(ibtData->text)) {
      OLC_IBT(d)->text  = STRALLOC(ibtData->text);
    }
    if ((ibtData->body) && *(ibtData->body)) {
      OLC_IBT(d)->body  = STRALLOC(ibtData->body);
    }
    if ((ibtData->name) && *(ibtData->name)) {
      OLC_IBT(d)->name  = STRALLOC(ibtData->name);
    }
    if ((ibtData->notes) && *(ibtData->notes)) {
      OLC_IBT(d)->notes = STRALLOC(ibtData->notes);
    }
    OLC_IBT(d)->level = ibtData->level;
    OLC_IBT(d)->room  = ibtData->room;
    for (i=0; i<IBT_ARRAY_MAX; i++) {
      OLC_IBT(d)->flags[i] = ibtData->flags[i];
    }
  }
}

/*-------------------------------------------------------------------*/
/* Copy IBT data back to the correct list                            */
static void ibtedit_save(struct descriptor_data *d)
{
  IBT_DATA *ibtData;
  int i;

  /* Should always be the case, but just to make sure */
  if ((ibtData = get_ibt_by_num(OLC_VAL(d), OLC_NUM(d))) != NULL) {
    if ((OLC_IBT(d)->text) && *(OLC_IBT(d)->text)) {
      if (ibtData->text) STRFREE(ibtData->text);
      ibtData->text = STRALLOC(OLC_IBT(d)->text);
      STRFREE(OLC_IBT(d)->text);
    }
    if ((OLC_IBT(d)->body) && *(OLC_IBT(d)->body)) {
      if (ibtData->body) STRFREE(ibtData->body);
      ibtData->body = STRALLOC(OLC_IBT(d)->body);
      STRFREE(OLC_IBT(d)->body);
    }
    if ((OLC_IBT(d)->name) && *(OLC_IBT(d)->name)) {
      if (ibtData->name) STRFREE(ibtData->name);
      ibtData->name = STRALLOC(OLC_IBT(d)->name);
      STRFREE(OLC_IBT(d)->name);
    }
    if ((OLC_IBT(d)->notes) && *(OLC_IBT(d)->notes)) {
      if (ibtData->notes) STRFREE(ibtData->notes);
      ibtData->notes = STRALLOC(OLC_IBT(d)->notes);
      STRFREE(OLC_IBT(d)->notes);
    }
    ibtData->level = OLC_IBT(d)->level;
    ibtData->room  = OLC_IBT(d)->room;
    for (i=0; i<IBT_ARRAY_MAX; i++) {
      ibtData->flags[i] = OLC_IBT(d)->flags[i];
    }
  } else {
    log("SYSERR: ibtedit_save: Invalid IBT vnum (%d) in OLC struct", OLC_NUM(d));
    log("        IBT possibly removed while being edited");
    return;
  }

  save_ibt_file(OLC_VAL(d));
}

void free_olc_ibt(IBT_DATA *toFree) {
	
	if (!toFree)
	  return;
 
	if (toFree->text) {
	  STRFREE(toFree->text);
	}

	if (toFree->body) {
	  STRFREE(toFree->body);
	}

	if (toFree->name) {
	  STRFREE(toFree->name);
	}

	if (toFree->notes) {
	  STRFREE(toFree->notes);
	}
	
	free(toFree);
}

/*-------------------------------------------------------------------*/
/* main ibtedit menu function...                                     */
static void ibtedit_disp_main_menu(struct descriptor_data *d)
{
  struct char_data *ch = d->character;
  char flg_text[MAX_STRING_LENGTH];
  room_rnum rr;

  get_char_colors(ch);
  clear_screen(d);

  rr = real_room(OLC_IBT(d)->room);
  sprintbitarray(OLC_IBT(d)->flags, ibt_bits, IBT_ARRAY_MAX, flg_text);

  send_to_char(ch, "%s-- Edit %s Number %s[%s%d%s]\r\n"
                   "%s1%s) Reported By: %s%-12s\r\n"
                   "%s2%s) Reported In: %s[%s%-5d%s]%s - %s%s\r\n"
                   "%s3%s) Header Text: %s%s\r\n"
                   "%s4%s) Flags      : %s%s\r\n"
                   "%s5%s) Details:\r\n%s%s\r\n"
                   "%s6%s) Admin Notes:\r\n%s%s\r\n"
                   "%sQ%s) Quit %s Editor\r\n",
                   QBGRN, IBT_TYPE, cyn, yel, OLC_NUM(d), cyn,
                   yel, nrm, yel, OLC_IBT(d)->name,
                   yel, nrm, cyn, yel, OLC_IBT(d)->room, cyn, nrm, (rr == NOWHERE) ? CCRED(d->character, C_NRM) : cyn, (rr == NOWHERE) ? "<Invalid Room!>" : world[rr].name,
                   yel, nrm, yel, OLC_IBT(d)->text,
                   yel, nrm, cyn, flg_text,
                   yel, nrm, yel, OLC_IBT(d)->body ? OLC_IBT(d)->body : "<Not Set!>",
                   yel, nrm, yel, OLC_IBT(d)->notes ? OLC_IBT(d)->notes : "<Not Set!>",
                   yel, nrm, IBT_TYPE);

  OLC_MODE(d) = IBTEDIT_MAIN_MENU;

}
/*-------------------------------------------------------------------*/
/* Display IBT-flags menu. */
static void ibtedit_disp_flags(struct descriptor_data *d)
{
  char buf[MAX_STRING_LENGTH];

  get_char_colors(d->character);
  clear_screen(d);

  column_list(d->character, 2, ibt_bits, NUM_IBT_FLAGS, TRUE);

  sprintbitarray(OLC_IBT(d)->flags, ibt_bits, IBT_ARRAY_MAX, buf);
  write_to_output(d, "\r\nCurrent flags : %s%s%s\r\nEnter flags (0 to quit) : ",
		  cyn, buf, nrm);

  OLC_MODE(d) = IBTEDIT_FLAGS;
}
/*-------------------------------------------------------------------*/
/* main clanedit parser function... interpreter throws all input to here. */
void ibtedit_parse(struct descriptor_data *d, char *arg)
{
  int i;
  char *oldtext = NULL;

  switch (OLC_MODE(d)) {
    case IBTEDIT_CONFIRM_SAVESTRING:
      switch (*arg) {
      case 'y':
      case 'Y':
        /* Save the IBT in memory and to disk. */
        ibtedit_save(d);
        mudlog(CMP, MAX(LVL_BUILDER, GET_INVIS_LEV(d->character)), TRUE, "OLC: %s edits %s %d", GET_NAME(d->character), IBT_TYPE, OLC_NUM(d));
        cleanup_olc(d, CLEANUP_ALL);
        return;
      case 'n':
      case 'N':
        /* If not saving, we must free the OLC memory. */
        cleanup_olc(d, CLEANUP_ALL);
        return;
      default:
        write_to_output(d, "Invalid choice!\r\n");
        write_to_output(d, "Do you wish to save your changes? : ");
        return;
      }
      break;

    case IBTEDIT_MAIN_MENU:
      switch (*arg) {
        case 'q':
        case 'Q':
          if (OLC_ZNUM(d)) {	/* Anything been changed? */
            write_to_output(d, "Do you wish to save your changes? : ");
            OLC_MODE(d) = IBTEDIT_CONFIRM_SAVESTRING;
          } else
            cleanup_olc(d, CLEANUP_ALL);
          return;

        case '1':
          OLC_MODE(d) = IBTEDIT_NAME;
          write_to_output(d, "Who reported this %s? : ", IBT_TYPE);
          return;

        case '2':
          OLC_MODE(d) = IBTEDIT_ROOM;
          write_to_output(d, "Enter the room VNUM for this %s? : ", IBT_TYPE);
          return;

        case '3':
          OLC_MODE(d) = IBTEDIT_TEXT;
          write_to_output(d, "Enter the header text for this %s? : ", IBT_TYPE);
          return;

        case '4':
          ibtedit_disp_flags(d);
          return;

        case '5':
          OLC_MODE(d) = IBTEDIT_BODY;
          send_editor_help(d);
          write_to_output(d, "Enter %s description:\r\n\r\n", IBT_TYPE);
          if (OLC_IBT(d)->body) {
            write_to_output(d, "%s", OLC_IBT(d)->body);
            if (oldtext) STRFREE(oldtext);
            oldtext = STRALLOC(OLC_IBT(d)->body);
          }
          string_write(d, &OLC_IBT(d)->body, MAX_IBT_LENGTH, 0, oldtext);
          OLC_ZNUM(d) = 1;
          return;

        case '6':
          OLC_MODE(d) = IBTEDIT_NOTES;
          send_editor_help(d);
          write_to_output(d, "Enter %s notes:\r\n\r\n", IBT_TYPE);
          if (OLC_IBT(d)->notes) {
            write_to_output(d, "%s", OLC_IBT(d)->notes);
            if (oldtext) STRFREE(oldtext);
            oldtext = STRALLOC(OLC_IBT(d)->notes);
          }
          string_write(d, &OLC_IBT(d)->notes, MAX_IBT_LENGTH, 0, oldtext);
          OLC_ZNUM(d) = 1;
          return;

        default:
          ibtedit_disp_main_menu(d);
          return;
      }
      break;

    case IBTEDIT_NAME:
      smash_tilde(arg);
      if (OLC_IBT(d)->name) STRFREE(OLC_IBT(d)->name);
      OLC_IBT(d)->name = STRALLOC(arg);
      break;

    case IBTEDIT_ROOM:
      i = atoi(arg);
      if (i == 0) { /* Cancel option */
        ibtedit_disp_main_menu(d);
        return;
      }
      if (real_room(i) == NOWHERE) {
        write_to_output(d, "That room does not exist!\r\n"
                           "Enter room VNUM (0 to cancel) : ");
        return;
      }
      OLC_IBT(d)->room = i;
      /* Drop through to re-show main menu and set 'edited' flag */
      break;

    case IBTEDIT_TEXT:
      smash_tilde(arg);
      if (OLC_IBT(d)->text) STRFREE(OLC_IBT(d)->text);
      OLC_IBT(d)->text = STRALLOC(arg);
      break;

    case IBTEDIT_FLAGS:
      if ((i = atoi(arg)) <= 0)
        break;
      else if (i <= NUM_IBT_FLAGS)
        TOGGLE_BIT_AR(IBT_FLAGS(OLC_IBT(d)), (i - 1));
      ibtedit_disp_flags(d);
      return;

    case IBTEDIT_BODY:
      /* We should never get here, modify.c throws user through ibtedit_string_cleanup. */
      cleanup_olc(d, CLEANUP_ALL);
      mudlog(BRF, LVL_BUILDER, TRUE, "SYSERR: OLC: ibtedit_parse(): Reached BODY case!");
      write_to_output(d, "Oops...\r\n");
      break;

    case IBTEDIT_NOTES:
      /* We should never get here, modify.c throws user through ibtedit_string_cleanup. */
      cleanup_olc(d, CLEANUP_ALL);
      mudlog(BRF, LVL_BUILDER, TRUE, "SYSERR: OLC: ibtedit_parse(): Reached NOTES case!");
      write_to_output(d, "Oops...\r\n");
      break;

    default:
      /* We should never get here. */
      cleanup_olc(d, CLEANUP_ALL);
      mudlog(BRF, LVL_BUILDER, TRUE, "SYSERR: OLC: ibtedit_parse(): Reached default case!");
      write_to_output(d, "Oops...\r\n");
      break;
  }

  /* If we get here, something was changed */
  OLC_ZNUM(d) = 1;
  ibtedit_disp_main_menu(d);
}

/*-------------------------------------------------------------------*/
void ibtedit_string_cleanup(struct descriptor_data *d, int terminator)
{
  switch (OLC_MODE(d)) {

  case IBTEDIT_BODY:
  case IBTEDIT_NOTES:
  default:
     ibtedit_disp_main_menu(d);
     break;
  }
}

/*-------------------------------------------------------------------*/
void free_ibt_lists()
{
  IBT_DATA *first_ibt, *last_ibt;
  int mode;

  for( mode=0; mode <=2; mode++){
    first_ibt = get_first_ibt(mode);
    last_ibt = get_last_ibt(mode);
    free_ibt_list(first_ibt, last_ibt);
  }
}

