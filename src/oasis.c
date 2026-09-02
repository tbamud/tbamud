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
#include "constants.h" /* for connected_types */

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
    default: /* The caller has screwed up. */
      log("SYSERR: cleanup_olc: Unknown type!");
      break;
    }
  }

  /* cedit's copy, which used to be freed from inside the room gate above.
   * cedit never sets OLC_ROOM, so that arm was unreachable on every cedit
   * exit there has ever been -- clean quit, save-and-quit, or link loss --
   * and the config struct plus its eight strings went every time. On its
   * own here, and unconditional of cleanup type, because there is no
   * cleanup type under which this copy should outlive the editor:
   * cedit_save_internally deep-copies every string into the live config,
   * so nothing here is ever handed anywhere.
   *
   * 822 bytes over 9 allocations per cedit session, measured.
   */
  if (OLC_CONFIG(d)) {
    free_config(OLC_CONFIG(d));
    OLC_CONFIG(d) = NULL;
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
   /* hedit alone. Unconditional like the block above, and for the same
    * reason: it is a copy this editor owns. CLEANUP_STRUCTS skips
    * OLC_HELP's strings because those are handed to the help table on the
    * way out; this one is never handed to anything. */
   if (OLC_HELP_KEY(d)) {
     free(OLC_HELP_KEY(d));
     OLC_HELP_KEY(d) = NULL;
   }
   if (OLC_HELP_TEXT(d)) {
     free(OLC_HELP_TEXT(d));
     OLC_HELP_TEXT(d) = NULL;
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

    /* Only a descriptor that actually entered an editor should announce
     * leaving one.  dig, buildwalk and the copy commands borrow an
     * oasis_olc_data so that an editor's save function can do the
     * insertion for them and never touch STATE, so without this they told
     * the room "$n stops using OLC." and logged "stops editing zone N" for
     * a session that never started.  All three log what they really did
     * instead. */
    if (STATE(d) != CON_PLAYING) {
      act("$n stops using OLC.", TRUE, d->character, NULL, NULL, TO_ROOM);

      if (cleanup_type == CLEANUP_CONFIG)
        mudlog(BRF, MAX(LVL_IMMORT, GET_INVIS_LEV(d->character)), 
          TRUE, "OLC: %s stops editing the game configuration", GET_NAME(d->character));
      else if (STATE(d) == CON_TEDIT)
        mudlog(BRF, MAX(LVL_IMMORT, GET_INVIS_LEV(d->character)),
         TRUE, "OLC: %s stops editing text files.", GET_NAME(d->character));
      else if (STATE(d) == CON_HEDIT)
        mudlog(CMP, MAX(LVL_IMMORT, GET_INVIS_LEV(d->character)),
         TRUE, "OLC: %s stops editing help files.", GET_NAME(d->character));
      /* aedit reuses OLC_ZNUM as the SOCIAL index, so the zone_table lookup
       * below reads a random zone -- or past the table, since there are far
       * more socials than zones.  It logs what it was really editing. */
      else if (STATE(d) == CON_AEDIT)
        mudlog(CMP, MAX(LVL_IMMORT, GET_INVIS_LEV(d->character)),
         TRUE, "OLC: %s stops editing actions.", GET_NAME(d->character));
      /* aedit is not the only editor whose OLC_ZNUM is not a zone. Four
       * reach this line holding something else, and three of them are
       * still to go. msgedit and prefedit never write the field, so they
       * report zone_table[0] whatever the builder was doing. ibtedit does
       * write it, but as a dirty flag -- 0 or 1, set at ibt.c:828 and
       * 1064/1077/1143 and read back at 1028 -- so once anything has
       * changed it reports zone_table[1] instead. In range, and never once
       * true.
       *
       * They say what they were editing. The wording is this function's
       * own rather than an echo of each entry line: prefedit logs nothing
       * at all on the way in, and msgedit and ibtedit name the individual
       * record, which is not what is being left. */
      else if (STATE(d) == CON_MSGEDIT)
        mudlog(CMP, MAX(LVL_IMMORT, GET_INVIS_LEV(d->character)),
         TRUE, "OLC: %s stops editing messages.", GET_NAME(d->character));
      else if (STATE(d) == CON_PREFEDIT)
        mudlog(CMP, MAX(LVL_IMMORT, GET_INVIS_LEV(d->character)),
         TRUE, "OLC: %s stops editing preferences.", GET_NAME(d->character));
      else if (STATE(d) == CON_IBTEDIT)
        mudlog(CMP, MAX(LVL_IMMORT, GET_INVIS_LEV(d->character)),
         TRUE, "OLC: %s stops editing ideas, bugs and typos.", GET_NAME(d->character));
      /* Nothing reaches this today, and two things have to hold for that.
       * Every state left sets OLC_ZNUM from real_zone() or
       * real_zone_by_thing() and refuses to open when that answers NOWHERE.
       * And the callers that arrive still in CON_PLAYING are covered by the
       * STATE(d) != CON_PLAYING guard above -- which is what closes
       * aedit.c:95, where cleanup_olc is called with OLC_ZNUM at
       * top_of_socialt + 1 nine lines before STATE is set to CON_AEDIT. The
       * CON_AEDIT branch above would not have caught that one; this bound
       * did, before the guard was in front of it.
       *
       * It stays so that the next editor to keep something else in that
       * field gets a line naming itself rather than a read off the end of
       * the table. `>` and not `>=`: top_of_zone_table is a last index,
       * not a count. NOWHERE needs no test of its own -- it is 65535, and
       * a table with a higher last index than that would have an rnum
       * colliding with NOWHERE, which breaks far more than this line. */
      else if (OLC_ZNUM(d) > top_of_zone_table)
        mudlog(BRF, MAX(LVL_IMMORT, GET_INVIS_LEV(d->character)),
         TRUE, "SYSERR: cleanup_olc: %s left %d in OLC_ZNUM while in %s, "
               "which is not a zone.", GET_NAME(d->character),
               OLC_ZNUM(d), connected_types[STATE(d)]);
      else
        mudlog(CMP, MAX(LVL_IMMORT, GET_INVIS_LEV(d->character)),
          TRUE, "OLC: %s stops editing zone %d allowed zone %d", 
          GET_NAME(d->character), zone_table[OLC_ZNUM(d)].number, GET_OLC_ZONE(d->character));

      STATE(d) = CON_PLAYING;
    }
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

void sprint_olc_access(struct char_data *ch, char *buf, size_t buflen)
{
  if (!buflen)
    return;

  if (GET_OLC_ZONE(ch) == AEDIT_PERMISSION)
    strlcpy(buf, "Aedit", buflen);
  else if (GET_OLC_ZONE(ch) == HEDIT_PERMISSION)
    strlcpy(buf, "Hedit", buflen);
  else if (GET_OLC_ZONE(ch) == ALL_PERMISSION)
    strlcpy(buf, "All", buflen);
  else if (GET_OLC_ZONE(ch) == NOWHERE)
    strlcpy(buf, "OFF", buflen);
  else
    snprintf(buf, buflen, "%d", GET_OLC_ZONE(ch));
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

