/**************************************************************************
*  File: oasis.c                                           Part of tbaMUD *
*  Usage: Oasis - General.                                                *
*                                                                         *
* By Levork. Copyright 1996 Harvey Gilpin. 1997-2001 George Greer.        *
**************************************************************************/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "interpreter.h"
#include "comm.h"
#include "db.h"
#include "shop.h"
#include "genolc.h"
#include "genmob.h"
#include "genshp.h"
#include "genzon.h"
#include "genwld.h"
#include "genobj.h"
#include "oasis.h"
#include "screen.h"
#include "dg_olc.h"
#include "act.h"
#include "handler.h" /* for is_name */
#include "quest.h"
#include "ibt.h"
#include "msgedit.h"

/* Internal Data Structures */
/** @deprecated olc_scmd_info appears to be deprecated. Commented out for now.
static struct olc_scmd_info_t {
  const char *text;
  int con_type;
} olc_scmd_info[] = {
  { "room",	CON_REDIT },
  { "object",	CON_OEDIT },
  { "zone",	CON_ZEDIT },
  { "mobile",	CON_MEDIT },
  { "shop",	CON_SEDIT },
  { "config",   CON_CEDIT },
  { "trigger",  CON_TRIGEDIT },
  { "action",   CON_AEDIT },
  { "help",     CON_HEDIT },
  { "quest",     CON_QEDIT },
  { "\n",	-1	  }
};
*/

/* Global variables defined here, used elsewhere */
const char *nrm, *grn, *cyn, *yel;

/* Internal Function prototypes  */
static void free_config(struct config_data *data);

/* Only player characters should be using OLC anyway. */
void clear_screen(struct descriptor_data *d)
{
  if (PRF_FLAGGED(d->character, PRF_CLS))
    write_to_output(d, "[H[J");
}

/* Exported utilities */
/* Set the color string pointers for that which this char will see at color
 * level NRM.  Changing the entries here will change the colour scheme
 * throughout the OLC. */
void get_char_colors(struct char_data *ch)
{
  nrm = CCNRM(ch, C_NRM);
  grn = CCGRN(ch, C_NRM);
  cyn = CCCYN(ch, C_NRM);
  yel = CCYEL(ch, C_NRM);
}

/* This procedure frees up the strings and/or the structures attatched to a
 * descriptor, sets all flags back to how they should be. */
void cleanup_olc(struct descriptor_data *d, byte cleanup_type)
{
  /* Clean up WHAT? */
  if (d->olc == NULL)
    return;

  /* Check for a room. free_room doesn't perform sanity checks, we must be
   * careful here. */
  if (OLC_ROOM(d)) {
    switch (cleanup_type) {
    case CLEANUP_ALL:
      /* free(OLC_SCRIPT(d)) equivalent */
      free_proto_script(OLC_ROOM(d), WLD_TRIGGER);
      free_room(OLC_ROOM(d));
      break;
    case CLEANUP_STRUCTS:
      free(OLC_ROOM(d));
      break;
    case CLEANUP_CONFIG:
      free_config(OLC_CONFIG(d));
      break;
    default: /* The caller has screwed up. */
      log("SYSERR: cleanup_olc: Unknown type!");
      break;
    }
  }

  /* Check for an existing object in the OLC.  The strings aren't part of the
   * prototype any longer.  They get added with strdup(). */
  if (OLC_OBJ(d)) {
    free_object_strings(OLC_OBJ(d));
    free(OLC_OBJ(d));
  }

  /* Check for a mob.  free_mobile() makes sure strings are not in the
   * prototype. */
  if (OLC_MOB(d))
    free_mobile(OLC_MOB(d));

  /* Check for a zone.  cleanup_type is irrelevant here, free() everything. */
  if (OLC_ZONE(d)) {
    if (OLC_ZONE(d)->builders)
      free(OLC_ZONE(d)->builders);
    if (OLC_ZONE(d)->name)
      free(OLC_ZONE(d)->name);
    if (OLC_ZONE(d)->cmd)
      free(OLC_ZONE(d)->cmd);
    free(OLC_ZONE(d));
  }

  /* Check for a shop.  free_shop doesn't perform sanity checks, we must be
   * careful here. OLC_SHOP(d) is a _copy_ - no pointers to the original. Just
   * go ahead and free it all. */
  if (OLC_SHOP(d))
      free_shop(OLC_SHOP(d));

  /* Check for a quest. */
  if (OLC_QUEST(d)) {
    switch (cleanup_type) {
      case CLEANUP_ALL:
        free_quest(OLC_QUEST(d));
        break;
      case CLEANUP_STRUCTS:
        free(OLC_QUEST(d));
        break;
      default:
        break;
    }
  }

  /*. Check for aedit stuff -- M. Scott */
  if (OLC_ACTION(d))  {
    switch(cleanup_type)  {
      case CLEANUP_ALL:
 	free_action(OLC_ACTION(d));
 	break;
      case CLEANUP_STRUCTS:
        free(OLC_ACTION(d));
        break;
      default:
        /* Caller has screwed up */
 	break;
    }
  }

  /* Used for cleanup of Hedit */
  if (OLC_HELP(d))  {
    switch(cleanup_type)  {
      case CLEANUP_ALL:
 	free_help(OLC_HELP(d));
 	break;
      case CLEANUP_STRUCTS:
        free(OLC_HELP(d));
        break;
      default:
 	break;
    }
  }

   if (OLC_IBT(d)) {
	   free_olc_ibt(OLC_IBT(d));
	   OLC_IBT(d) = NULL;
   }
   
   if (OLC_MSG_LIST(d)) {
     free_message_list(OLC_MSG_LIST(d));
     OLC_MSG_LIST(d) = NULL;  
     OLC_MSG(d) = NULL;
   }

  /* Free storage if allocated (tedit, aedit, and trigedit). This is the command
   * list - it's been copied to disk already, so just free it -Welcor. */
   if (OLC_STORAGE(d)) {
     free(OLC_STORAGE(d));
     OLC_STORAGE(d) = NULL;
   }
   /* Free this one regardless. If we've left olc, we've either made a fresh
    * copy of it in the trig index, or we lost connection. Either way, we need
    * to get rid of this. */
   if (OLC_TRIG(d)) {
     free_trigger(OLC_TRIG(d));
     OLC_TRIG(d) = NULL;
   }

   /* Free this one regardless. If we've left olc, we've either copied the    *
    * preferences to the player, or we lost connection. Either way, we need   *
    * to get rid of this. */
   if(OLC_PREFS(d)) {
     /*. There is nothing else really to free, except this... .*/
     free(OLC_PREFS(d));
     OLC_PREFS(d) = NULL;
   }

   /* OLC_SCRIPT is always set as trig_proto of OLC_OBJ/MOB/ROOM. Therefore it
    * should not be free'd here. */

  /* Restore descriptor playing status. */
  if (d->character) {
    REMOVE_BIT_AR(PLR_FLAGS(d->character), PLR_WRITING);
    act("$n stops using OLC.", TRUE, d->character, NULL, NULL, TO_ROOM);

    if (cleanup_type == CLEANUP_CONFIG)
      mudlog(BRF, LVL_IMMORT, TRUE, "OLC: %s stops editing the game configuration", GET_NAME(d->character));
    else if (STATE(d) == CON_TEDIT)
      mudlog(BRF, LVL_IMMORT, TRUE, "OLC: %s stops editing text files.", GET_NAME(d->character));
    else if (STATE(d) == CON_HEDIT)
      mudlog(CMP, LVL_IMMORT, TRUE, "OLC: %s stops editing help files.", GET_NAME(d->character));
    else
      mudlog(CMP, LVL_IMMORT, TRUE, "OLC: %s stops editing zone %d allowed zone %d", GET_NAME(d->character), zone_table[OLC_ZNUM(d)].number, GET_OLC_ZONE(d->character));

    STATE(d) = CON_PLAYING;
  }

  free(d->olc);
  d->olc = NULL;
}

void split_argument(char *argument, char *tag)
{
  char *tmp = argument, *ttag = tag, *wrt = argument;
  int i;

  for (i = 0; *tmp; tmp++, i++) {
    if (*tmp != ' ' && *tmp != '=')
      *(ttag++) = *tmp;
    else if (*tmp == '=')
      break;
  }

  *ttag = '\0';

  while (*tmp == '=' || *tmp == ' ')
    tmp++;

  while (*tmp)
    *(wrt++) = *(tmp++);

  *wrt = '\0';
}

static void free_config(struct config_data *data)
{
  /* Free strings. */
  free_strings(data, OASIS_CFG);

  /* Free the data structure. */
  free(data);
}

/* Checks to see if a builder can modify the specified zone. Ch is the imm
 * requesting access to modify this zone. Rnum is the real number of the zone
 * attempted to be modified. Returns TRUE if the builder has access, otherwisei
 * FALSE. */
int can_edit_zone(struct char_data *ch, zone_rnum rnum)
{
  /* no access if called with bad arguments */
  if (!ch->desc || IS_NPC(ch) || rnum == NOWHERE)
    return FALSE;

  /* If zone is flagged NOBUILD, then No-one can edit it (use zunlock to open it) */
  if (rnum != HEDIT_PERMISSION && rnum != AEDIT_PERMISSION && ZONE_FLAGGED(rnum, ZONE_NOBUILD) )
    return FALSE;

  if (GET_OLC_ZONE(ch) == ALL_PERMISSION)
    return TRUE;

  if (GET_OLC_ZONE(ch) == HEDIT_PERMISSION && rnum == HEDIT_PERMISSION)
    return TRUE;

  if (GET_OLC_ZONE(ch) == AEDIT_PERMISSION && rnum == AEDIT_PERMISSION)
    return TRUE;

  /* always access if ch is high enough level */
  if (GET_LEVEL(ch) >= LVL_GRGOD)
    return (TRUE);

  /* always access if a player helped build the zone in the first place */
  if (rnum != HEDIT_PERMISSION && rnum != AEDIT_PERMISSION)
    if (is_name(GET_NAME(ch), zone_table[rnum].builders))
      return (TRUE);

  /* no access if you haven't been assigned a zone */
  if (GET_OLC_ZONE(ch) == NOWHERE) {
    return FALSE;
  }

  /* no access if you're not at least LVL_BUILDER */
  if (GET_LEVEL(ch) < LVL_BUILDER)
    return FALSE;

  /* always access if you're assigned to this zone */
  if (real_zone(GET_OLC_ZONE(ch)) == rnum)
    return TRUE;

  return (FALSE);
}

void send_cannot_edit(struct char_data *ch, zone_vnum zone)
{
  char buf[MAX_STRING_LENGTH];

  if (GET_OLC_ZONE(ch) != NOWHERE) {
    send_to_char(ch, "You do not have permission to edit zone %d.  Try zone %d.\r\n", zone, GET_OLC_ZONE(ch));
    sprintf(buf, "OLC: %s tried to edit zone %d (allowed zone %d).", GET_NAME(ch), zone, GET_OLC_ZONE(ch));
  } else {
    send_to_char(ch, "You do not have permission to edit zone %d.\r\n", zone);
    sprintf(buf, "OLC: %s tried to edit zone %d.", GET_NAME(ch), zone);
  }
  mudlog(BRF, LVL_IMPL, TRUE, "%s", buf);
}

