/* ***********************************************************************
*    File:   genqst.c                                 Part of CircleMUD  *
* Version:   2.0 (November 2005) Written for CircleMud CWG / Suntzu      *
* Purpose:   To provide special quest-related code.                      *
* Copyright: Kenneth Ray                                                 *
* Original Version Details:                                              *
* Copyright 1996 by Harvey Gilpin           *
* Copyright 1997-2001 by George Greer (greerga@circlemud.org)     *
************************************************************************ */

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "db.h"
#include "quest.h"
#include "genolc.h"
#include "genzon.h" /* for create_world_index */


/*-------------------------------------------------------------------*/

int copy_quest(struct aq_data *to, struct aq_data *from, int free_old_strings)
{
  int i;
  if (free_old_strings)
    free_quest_strings(to);
  to->vnum       = from->vnum;
  to->flags      = from->flags;
  to->type       = from->type;
  to->qm         = from->qm;
  to->target     = from->target;
  to->prereq     = from->prereq;
  to->prev_quest = from->prev_quest;
  to->next_quest = from->next_quest;
  for (i = 0; i < 7; i++){
    to->value[i] = from->value[i];
  }
  to->gold_reward = from->gold_reward;
  to->exp_reward  = from->exp_reward;
  to->obj_reward  = from->obj_reward;
  to->func        = from->func;
  return copy_quest_strings(to, from);
}

int copy_quest_strings(struct aq_data *to, struct aq_data *from)
{
  if (from == NULL || to == NULL) {
    log("SYSERR: GenQST: copy_quest_strings: Null values passed.");
    return FALSE;
  }
  to->name = str_udup(from->name);
  to->desc = str_udup(from->desc);
  to->info = str_udup(from->info);
  to->done = str_udup(from->done);
  to->quit = str_udup(from->quit);
  return TRUE;
}

void free_quest_strings(struct aq_data *quest)
{
  if (quest->name)
    free(quest->name);
  if (quest->desc)
    free(quest->desc);
  if (quest->info)
    free(quest->info);
  if (quest->done)
    free(quest->done);
  if (quest->quit)
    free(quest->quit);
}

void free_quest(struct aq_data *quest)
{
  free_quest_strings(quest);
  free(quest);
}

/*-------------------------------------------------------------------*/

int add_quest(struct aq_data *nqst)
{
  qst_rnum rnum;
  mob_rnum qmrnum;
  zone_rnum rznum = real_zone_by_thing(nqst->vnum);

  /* The quest already exists, just update it.  */
  if ((rnum = real_quest(nqst->vnum)) != NOWHERE) {
    copy_quest(&aquest_table[rnum], nqst, TRUE);
  } else {
    /* increase the number of quest table entries */
    total_quests++;
    RECREATE(aquest_table, struct aq_data, total_quests );
    /* Initialise top quest strings to null */
    QST_NAME(total_quests - 1) = NULL;
    QST_DESC(total_quests - 1) = NULL;
    QST_INFO(total_quests - 1) = NULL;
    QST_DONE(total_quests - 1) = NULL;
    QST_QUIT(total_quests - 1) = NULL;
    /* Now process enties from the top down to see where the new one goes */
    for (rnum = total_quests - 1; rnum > 0; rnum--) {
      if (nqst->vnum > QST_NUM(rnum - 1))
        break; //found the place
      aquest_table[rnum] = aquest_table[rnum - 1]; //shift quest up one
    }
    copy_quest(&aquest_table[rnum], nqst, FALSE);
  }
  qmrnum = real_mobile(QST_MASTER(rnum));
  /* Make sure we assign spec procs to the questmaster */
  if (qmrnum != NOBODY && mob_index[qmrnum].func &&
     mob_index[qmrnum].func != questmaster)
     QST_FUNC(rnum) = mob_index[qmrnum].func;
  if(qmrnum != NOBODY) 
    mob_index[qmrnum].func = questmaster;

  /* And make sure we save the updated quest information to disk */
  if (rznum != NOWHERE)
    add_to_save_list(zone_table[rznum].number, SL_QST);
  else
    mudlog(BRF, LVL_BUILDER, TRUE,
           "SYSERR: GenOLC: Cannot determine quest zone.");

  return rnum;
}

/*-------------------------------------------------------------------*/

int delete_quest(qst_rnum rnum)
{
  qst_rnum i;
  zone_rnum rznum;
  mob_vnum qm = QST_MASTER(rnum);
  SPECIAL (*tempfunc);
  int  quests_remaining = 0;

  if (rnum >= total_quests)
    return FALSE;
  rznum = real_zone_by_thing(QST_NUM(rnum)); 
  log("GenOLC: delete_quest: Deleting quest #%d (%s).",
       QST_NUM(rnum), QST_NAME(rnum));
  /* make a note of the quest master's secondary spec proc */
  tempfunc = QST_FUNC(rnum);


  free_quest_strings(&aquest_table[rnum]);
  for (i = rnum; i < total_quests - 1; i++) {
    aquest_table[i] = aquest_table[i + 1];
  }
  total_quests--;
  if (total_quests > 0)
    RECREATE(aquest_table, struct aq_data, total_quests);
  else {
    free(aquest_table);
    aquest_table = NULL; 
   }
  if (rznum != NOWHERE)
     add_to_save_list(zone_table[rznum].number, SL_QST);
  else
    mudlog(BRF, LVL_BUILDER, TRUE,
           "SYSERR: GenOLC: Cannot determine quest zone.");
  /* does the questmaster mob have any quests left? */
  if (qm != NOBODY) {
    for (i = 0; i < total_quests; i++) {
      if (QST_MASTER(i) == qm)
        quests_remaining++;
    }
    if (quests_remaining == 0)
      mob_index[qm].func = tempfunc; // point back to original spec proc
  }
  return TRUE;
}

/*-------------------------------------------------------------------*/

int save_quests(zone_rnum zone_num)
{
  FILE *sf;
  char filename[128], oldname[128], quest_flags[MAX_STRING_LENGTH];
  char quest_desc[MAX_STRING_LENGTH], quest_info[MAX_STRING_LENGTH];
  char quest_done[MAX_STRING_LENGTH], quest_quit[MAX_STRING_LENGTH];
  int i, num_quests = 0;

#if CIRCLE_UNSIGNED_INDEX
  if (zone_num == NOWHERE || zone_num > top_of_zone_table) {
#else
  if (zone_num < 0 || zone_num > top_of_zone_table) {
#endif
    log("SYSERR: GenOLC: save_quests: Invalid zone number %d passed! (0-%d)",
         zone_num, top_of_zone_table);
    return FALSE;
  }

  log("GenOLC: save_quests: Saving quests in zone #%d (%d-%d).",
        zone_table[zone_num].number,
 genolc_zone_bottom(zone_num), zone_table[zone_num].top);

  snprintf(filename, sizeof(filename), "%s/%d.new",
 QST_PREFIX, zone_table[zone_num].number);
  if (!(sf = fopen(filename, "w"))) {
    perror("SYSERR: save_quests");
    return FALSE;
  }
  for (i = genolc_zone_bottom(zone_num); i <= zone_table[zone_num].top; i++) {
    qst_rnum rnum;
    if ((rnum = real_quest(i)) != NOTHING) {
      /* Copy the text strings and strip off trailing newlines. */
      strncpy(quest_desc, QST_DESC(rnum) ? QST_DESC(rnum) : "undefined",
              sizeof(quest_desc)-1 );
      strncpy(quest_info, QST_INFO(rnum) ? QST_INFO(rnum) : "undefined",
              sizeof(quest_info)-1 );
      strncpy(quest_done, QST_DONE(rnum) ? QST_DONE(rnum) : "undefined",
              sizeof(quest_done)-1 );
      strncpy(quest_quit, QST_QUIT(rnum) ? QST_QUIT(rnum) : "undefined",
              sizeof(quest_quit)-1 );
      strip_cr(quest_desc);
      strip_cr(quest_info);
      strip_cr(quest_done);
      strip_cr(quest_quit);
      /* Save the quest details to the file.  */
      sprintascii(quest_flags, QST_FLAGS(rnum));
      fprintf(sf,
        "#%d\n"
        "%s%c\n"
        "%s%c\n"
        "%s%c\n"
        "%s%c\n"
        "%s%c\n"
        "%d %d %s %d %d %d %d\n"
        "%d %d %d %d %d %d %d\n"
        "%d %d %d\n"
        "S\n",
        QST_NUM(rnum),
        QST_NAME(rnum) ? QST_NAME(rnum) : "Untitled", STRING_TERMINATOR,
        quest_desc, STRING_TERMINATOR,
        quest_info, STRING_TERMINATOR,
        quest_done, STRING_TERMINATOR,
        quest_quit, STRING_TERMINATOR,
        QST_TYPE(rnum),
        QST_MASTER(rnum) == NOBODY ? -1 : QST_MASTER(rnum),
        quest_flags,
        QST_TARGET(rnum) == NOTHING ? -1 : QST_TARGET(rnum),
        QST_PREV(rnum)   == NOTHING ? -1 : QST_PREV(rnum),
        QST_NEXT(rnum)   == NOTHING ? -1 : QST_NEXT(rnum),
        QST_PREREQ(rnum) == NOTHING ? -1 : QST_PREREQ(rnum),
        QST_POINTS(rnum), QST_PENALTY(rnum), QST_MINLEVEL(rnum),
        QST_MAXLEVEL(rnum), QST_TIME(rnum),
 QST_RETURNMOB(rnum) == NOBODY ? -1 : QST_RETURNMOB(rnum),
 QST_QUANTITY(rnum), QST_GOLD(rnum), QST_EXP(rnum), QST_OBJ(rnum)
      );
      num_quests++;
    }
  }
  /* Write the final line and close it.  */
  fprintf(sf, "$~\n");
  fclose(sf);

  /* Old file we're replacing. */
  snprintf(oldname, sizeof(oldname), "%s/%d.qst",
           QST_PREFIX, zone_table[zone_num].number);
  remove(oldname);
  rename(filename, oldname);

  /* Do we need to update the index file? */
  if (num_quests > 0)
    create_world_index(zone_table[zone_num].number, "qst");

  if (in_save_list(zone_table[zone_num].number, SL_QST))
    remove_from_save_list(zone_table[zone_num].number, SL_QST);
  return TRUE;
}

