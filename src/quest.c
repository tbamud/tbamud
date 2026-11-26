/* ***********************************************************************
*    File:   quest.c                                  Part of CircleMUD  *
* Version:   2.1 (December 2005) Written for CircleMud CWG / Suntzu      *
* Purpose:   To provide special quest-related code.                      *
* Copyright: Kenneth Ray                                                 *
* Original Version Details:                                              *
* Morgaelin - quest.c                                                    *
* Copyright (C) 1997 MS                                                  *
*********************************************************************** */

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "comm.h"
#include "screen.h"
#include "quest.h"
#include "act.h" /* for do_tell */


/*--------------------------------------------------------------------------
 * Exported global variables
 *--------------------------------------------------------------------------*/
const char *quest_types[] = {
  "Object",
  "Room",
  "Find mob",
  "Kill mob",
  "Save mob",
  "Return object",
  "Clear room",
  "\n"
};
const char *aq_flags[] = {
  "REPEATABLE",
  "\n"
};


/*--------------------------------------------------------------------------
 * Local (file scope) global variables
 *--------------------------------------------------------------------------*/
static int cmd_tell;

static const char *quest_cmd[] = {
  "list", "history", "join", "leave", "progress", "status", "\n"};

static const char *quest_mort_usage =
  "Usage: quest list | history | progress | join <nn> | leave";

static const char *quest_imm_usage =
  "Usage: quest list | history | progress | join <nn> | leave | status <vnum>";

/*--------------------------------------------------------------------------*/
/* Utility Functions                                                        */
/*--------------------------------------------------------------------------*/

qst_rnum real_quest(qst_vnum vnum)
{
  int rnum;

  for (rnum = 0; rnum < total_quests; rnum++)
    if (QST_NUM(rnum) == vnum)
      return(rnum);
  return(NOTHING);
}

int is_complete(struct char_data *ch, qst_vnum vnum)
{
  int i;

  for (i = 0; i < GET_NUM_QUESTS(ch); i++)
    if (ch->player_specials->saved.completed_quests[i] == vnum)
      return TRUE;
  return FALSE;
}

qst_vnum find_quest_by_qmnum(struct char_data *ch, mob_vnum qm, int num)
{
  qst_rnum rnum;
  int found=0;
  for (rnum = 0; rnum < total_quests; rnum++) {
    if (qm == QST_MASTER(rnum))
      if (++found == num)
        return (QST_NUM(rnum));
  }
  return NOTHING;
}

/*--------------------------------------------------------------------------*/
/* Quest Loading and Unloading Functions                                    */
/*--------------------------------------------------------------------------*/

void destroy_quests(void)
{
  qst_rnum rnum = 0;

  if (!aquest_table)
    return;

  for (rnum = 0; rnum < total_quests; rnum++){
    free_quest_strings(&aquest_table[rnum]);
  }
  free(aquest_table);
  aquest_table = NULL;
  total_quests = 0;

  return;
}

int count_quests(qst_vnum low, qst_vnum high)
{
  int i, j;

  if (!aquest_table)
    return 0;

  for (i = j = 0; i < total_quests; i++)
    if (QST_NUM(i) >= low && QST_NUM(i) <= high)
      j++;

  return j;
}

void parse_quest(FILE *quest_f, int nr)
{
  static char line[256];
  static int i = 0, j;
  int retval = 0, t[7];
  char f1[WORLD_FLAG_FIELD + 1], buf2[MAX_STRING_LENGTH];
  aquest_table[i].vnum = nr;
  aquest_table[i].qm = NOBODY;
  aquest_table[i].name = NULL;
  aquest_table[i].desc = NULL;
  aquest_table[i].info = NULL;
  aquest_table[i].done = NULL;
  aquest_table[i].quit = NULL;
  aquest_table[i].flags = 0;
  aquest_table[i].type = -1;
  aquest_table[i].target = -1;
  aquest_table[i].prereq = NOTHING;
  for (j = 0; j < 7; j++)
    aquest_table[i].value[j] = 0;
  aquest_table[i].prev_quest = NOTHING;
  aquest_table[i].next_quest = NOTHING;
  aquest_table[i].func = NULL;

  aquest_table[i].gold_reward = 0;
  aquest_table[i].exp_reward  = 0;
  aquest_table[i].obj_reward  = NOTHING;

  /* begin to parse the data */
  aquest_table[i].name = fread_string(quest_f, buf2);
  aquest_table[i].desc = fread_string(quest_f, buf2);
  aquest_table[i].info = fread_string(quest_f, buf2);
  aquest_table[i].done = fread_string(quest_f, buf2);
  aquest_table[i].quit = fread_string(quest_f, buf2);
  if (!get_line(quest_f, line, sizeof(line)) ||
      (retval = sscanf(line, " %d %d " FLAG_FIELD_FMT " %d %d %d %d",
             t, t+1, f1, t+2, t+3, t + 4, t + 5)) != 7) {
    log("Format error in numeric line (expected 7, got %d), %s\n",
        retval, line);
    exit(1);
  }
  aquest_table[i].type       = t[0];
  aquest_table[i].qm         = (real_mobile(t[1]) == NOBODY) ? NOBODY : t[1];
  aquest_table[i].flags      = asciiflag_conv(f1);
  aquest_table[i].target     = (t[2] == -1) ? NOTHING : t[2];
  aquest_table[i].prev_quest = (t[3] == -1) ? NOTHING : t[3];
  aquest_table[i].next_quest = (t[4] == -1) ? NOTHING : t[4];
  aquest_table[i].prereq     = (t[5] == -1) ? NOTHING : t[5];
  if (!get_line(quest_f, line, sizeof(line)) ||
      (retval = sscanf(line, " %d %d %d %d %d %d %d",
          t, t+1, t+2, t+3, t+4, t + 5, t + 6)) != 7) {
    log("Format error in numeric line (expected 7, got %d), %s\n",
        retval, line);
    exit(1);
  }
  for (j = 0; j < 7; j++)
    aquest_table[i].value[j] = t[j];

  if (!get_line(quest_f, line, sizeof(line)) ||
      (retval = sscanf(line, " %d %d %d",
             t, t+1, t+2)) != 3) {
    log("Format error in numeric (rewards) line (expected 3, got %d), %s\n",
        retval, line);
    exit(1);
  }

  aquest_table[i].gold_reward = t[0];
  aquest_table[i].exp_reward  = t[1];
  aquest_table[i].obj_reward  = (t[2] == -1) ? NOTHING : t[2];

  for (;;) {
    if (!get_line(quest_f, line, sizeof(line))) {
      log("Format error in %s\n", line);
      exit(1);
    }
    switch(*line) {
    case 'S':
      total_quests = ++i;
      return;
    }
  }
} /* parse_quest */

/* The merged command table is rebuilt from scratch whenever a social is
 * added or removed, and every index into it moves with it. The values
 * below are indices into that table, handed straight to do_action(), so
 * they have to be taken again each time it is rebuilt rather than once at
 * boot. create_command_list() calls this; see the note there. */
void assign_quest_command_indices(void)
{
  cmd_tell = find_command("tell");
}

void assign_the_quests(void)
{
  qst_rnum i, rnum;
  mob_rnum mrnum;
  SPECIAL (*secondary);

  assign_quest_command_indices();

  for (rnum = 0; rnum < total_quests; rnum ++) {
    if (QST_MASTER(rnum) == NOBODY) {
      log("SYSERR: Quest #%d has no questmaster specified.", QST_NUM(rnum));
      continue;
    }
    if ((mrnum = real_mobile(QST_MASTER(rnum))) == NOBODY) {
      log("SYSERR: Quest #%d has an invalid questmaster.", QST_NUM(rnum));
      continue;
    }

    /*
     * The questmaster special replaces the mob's original special proc.
     * Give every quest for this master the same secondary proc so runtime
     * behavior does not depend on quest ordering or which quest is deleted.
     */
    if (mob_index[mrnum].func != questmaster) {
      secondary = mob_index[mrnum].func;

      for (i = 0; i < total_quests; i++)
        if (QST_MASTER(i) == QST_MASTER(rnum))
          QST_FUNC(i) = secondary;

      mob_index[mrnum].func = questmaster;
    }
  }
}

/*--------------------------------------------------------------------------*/
/* Quest Completion Functions                                               */
/*--------------------------------------------------------------------------*/
void set_quest(struct char_data *ch, qst_rnum rnum)
{
  GET_QUEST(ch) = QST_NUM(rnum);
  GET_QUEST_TIME(ch) = QST_TIME(rnum);
  GET_QUEST_COUNTER(ch) = QST_QUANTITY(rnum);
  SET_BIT_AR(PRF_FLAGS(ch), PRF_QUEST);
  return;
}

void clear_quest(struct char_data *ch)
{
  GET_QUEST(ch) = NOTHING;
  GET_QUEST_TIME(ch) = -1;
  GET_QUEST_COUNTER(ch) = 0;
  REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_QUEST);
  return;
}

void add_completed_quest(struct char_data *ch, qst_vnum vnum)
{
  qst_vnum *temp;
  int i;

  CREATE(temp, qst_vnum, GET_NUM_QUESTS(ch) +1);
  for (i=0; i < GET_NUM_QUESTS(ch); i++)
    temp[i] = ch->player_specials->saved.completed_quests[i];

  temp[GET_NUM_QUESTS(ch)] = vnum;
  GET_NUM_QUESTS(ch)++;

  if (ch->player_specials->saved.completed_quests)
    free(ch->player_specials->saved.completed_quests);
  ch->player_specials->saved.completed_quests = temp;
}

void remove_completed_quest(struct char_data *ch, qst_vnum vnum)
{
  qst_vnum *temp;
  int i, j = 0;

  CREATE(temp, qst_vnum, GET_NUM_QUESTS(ch));
  for (i = 0; i < GET_NUM_QUESTS(ch); i++)
    if (ch->player_specials->saved.completed_quests[i] != vnum)
      temp[j++] = ch->player_specials->saved.completed_quests[i];

  GET_NUM_QUESTS(ch)--;

  if (ch->player_specials->saved.completed_quests)
    free(ch->player_specials->saved.completed_quests);
  ch->player_specials->saved.completed_quests = temp;
}

void generic_complete_quest(struct char_data *ch)
{
  qst_rnum rnum;
  qst_vnum vnum = GET_QUEST(ch);
  struct obj_data *new_obj;
  int happy_qp, happy_gold, happy_exp;

  /* An earlier trigger may have completed this quest, or a builder may
   * have deleted it while the player was on it. */
  if ((rnum = real_quest(vnum)) == NOTHING)
    return;

  /* Legacy quests with nonpositive counts finish on their first credit. */
  GET_QUEST_COUNTER(ch) = MAX(1, GET_QUEST_COUNTER(ch)) - 1;
  if (GET_QUEST_COUNTER(ch) == 0) {
    if (IS_HAPPYHOUR && IS_HAPPYQP) {
      happy_qp = (int)(QST_POINTS(rnum) * (((float)(100+HAPPY_QP))/(float)100));
      happy_qp = MAX(happy_qp, 0);
      GET_QUESTPOINTS(ch) += happy_qp;
      send_to_char(ch,
          "%s\r\nYou have been awarded %d quest points for your service.\r\n",
          QST_DONE(rnum), happy_qp);
	} else {
      GET_QUESTPOINTS(ch) += QST_POINTS(rnum);
      send_to_char(ch,
          "%s\r\nYou have been awarded %d quest points for your service.\r\n",
          QST_DONE(rnum), QST_POINTS(rnum));
    }
    if (QST_GOLD(rnum)) {
      if ((IS_HAPPYHOUR) && (IS_HAPPYGOLD)) {
        happy_gold = (int)(QST_GOLD(rnum) * (((float)(100+HAPPY_GOLD))/(float)100));
        happy_gold = MAX(happy_gold, 0);
        increase_gold(ch, happy_gold);
        send_to_char(ch,
              "You have been awarded %d gold coins for your service.\r\n",
              happy_gold);
	  } else {
        increase_gold(ch, QST_GOLD(rnum));
        send_to_char(ch,
              "You have been awarded %d gold coins for your service.\r\n",
              QST_GOLD(rnum));
      }
    }
    if (QST_EXP(rnum)) {
      if ((IS_HAPPYHOUR) && (IS_HAPPYEXP)) {
        happy_exp = (int)(QST_EXP(rnum) * (((float)(100+HAPPY_EXP))/(float)100));
        happy_exp = MAX(happy_exp, 0);
        gain_exp(ch, happy_exp);
        send_to_char(ch,
              "You have been awarded %d experience for your service.\r\n",
              happy_exp);
      } else {
        gain_exp(ch, QST_EXP(rnum));
        send_to_char(ch,
              "You have been awarded %d experience points for your service.\r\n",
              QST_EXP(rnum));
      }
    }
    if (QST_OBJ(rnum) && QST_OBJ(rnum) != NOTHING) {
      if (real_object(QST_OBJ(rnum)) != NOTHING) {
        if ((new_obj = read_object((QST_OBJ(rnum)),VIRTUAL)) != NULL) {
            obj_to_char(new_obj, ch);
            send_to_char(ch, "You have been presented with %s%s for your service.\r\n",
                GET_OBJ_SHORT(new_obj), CCNRM(ch, C_NRM));
        }
      }
    }
    if (!IS_SET(QST_FLAGS(rnum), AQ_REPEATABLE))
      add_completed_quest(ch, vnum);
    clear_quest(ch);
    if ((real_quest(QST_NEXT(rnum)) != NOTHING) &&
        (QST_NEXT(rnum) != vnum) &&
        !is_complete(ch, QST_NEXT(rnum))) {
      rnum = real_quest(QST_NEXT(rnum));
      set_quest(ch, rnum);
      send_to_char(ch,
          "The next stage of your quest awaits:\r\n%s",
          QST_INFO(rnum));
    }
  }
  save_char(ch);
}

static bool quest_room_is_clear(room_rnum room, qst_rnum quest)
{
  struct char_data *mob;

  for (mob = world[room].people; mob; mob = mob->next_in_room) {
    /* A pet that loses its charm during this fight still counts as a pet. */
    if (!IS_NPC(mob) || MOB_FLAGGED(mob, MOB_NOTDEADYET) || is_player_pet_kill(mob))
      continue;
    if (QST_TYPE(quest) != AQ_MOB_SAVE || GET_MOB_VNUM(mob) != QST_TARGET(quest))
      return FALSE;
  }
  return TRUE;
}

/* Area attacks and spells from a staff resolve one target at a time.  Only
 * the last guard's death can qualify, but the captive must survive the whole
 * action.  Death scripts can start another action before the first ends. */
static unsigned int rescue_action_depth;
static const struct char_data *first_char_before_action;
static struct pending_rescue {
  struct char_data *holder;
  const struct char_data *captive;
  bool recovery;
  int goals_left;
  qst_vnum quest;
  room_rnum room;
  struct pending_rescue *next;
} *pending_rescues;

void begin_rescue_action(void)
{
  if (rescue_action_depth++ == 0)
    first_char_before_action = character_list;
}

static void remember_rescue(struct char_data *ch, const struct char_data *captive,
                            bool recovery)
{
  struct pending_rescue *entry;
  struct char_data *mob;

  /* read_mobile() prepends its result to character_list. Death scripts can
   * load a replacement before the last guard dies. */
  for (mob = character_list; mob && mob != first_char_before_action; mob = mob->next)
    if (mob == captive)
      return;
  for (entry = pending_rescues; entry; entry = entry->next)
    if (entry->holder == ch && entry->quest == GET_QUEST(ch) &&
        entry->goals_left == GET_QUEST_COUNTER(ch) &&
        entry->room == IN_ROOM(ch) && entry->captive == captive) {
      if (!recovery)
        entry->recovery = FALSE;
      return;
    }
  CREATE(entry, struct pending_rescue, 1);
  entry->holder = ch;
  entry->captive = captive;
  entry->recovery = recovery;
  entry->quest = GET_QUEST(ch);
  entry->goals_left = GET_QUEST_COUNTER(ch);
  entry->room = IN_ROOM(ch);
  entry->next = pending_rescues;
  pending_rescues = entry;
}

static void check_rescue_goal(struct char_data *ch, qst_rnum quest,
                              const struct char_data *captive, bool recovery)
{
  struct char_data *mob;

  if (recovery && (QST_QUANTITY(quest) > 1 || GET_QUEST_COUNTER(ch) > 1))
    return;
  if (!quest_room_is_clear(IN_ROOM(ch), quest))
    return;

  for (mob = world[IN_ROOM(ch)].people; mob; mob = mob->next_in_room) {
    if ((captive && mob != captive) || !IS_NPC(mob) || DEAD(mob) ||
        GET_POS(mob) < POS_STUNNED || GET_MOB_VNUM(mob) != QST_TARGET(quest))
      continue;
    if (rescue_action_depth) {
      /* One of the captives that qualified now must survive the attack. */
      remember_rescue(ch, mob, recovery);
    } else {
      generic_complete_quest(ch);
      return;
    }
  }
}

static bool alive_in_room(const struct char_data *ch, room_rnum room)
{
  return ch && IN_ROOM(ch) == room && !DEAD(ch) && GET_POS(ch) > POS_DEAD;
}

void end_rescue_action(void)
{
  struct pending_rescue *entry, *next;

  if (--rescue_action_depth)
    return;
  first_char_before_action = NULL;

  /* Extraction is deferred until the end of the pulse. Finish these checks
   * synchronously, while the combatants still exist. */
  entry = pending_rescues;
  pending_rescues = NULL;
  for (; entry; entry = next) {
    struct char_data *ch = entry->holder;
    qst_rnum quest = real_quest(entry->quest);

    next = entry->next;
    /* Multiple surviving copies represent the same outstanding goal. */
    if (VALID_ROOM_RNUM(entry->room) && alive_in_room(ch, entry->room) &&
        GET_QUEST(ch) == entry->quest &&
        GET_QUEST_COUNTER(ch) == entry->goals_left &&
        quest != NOTHING && QST_TYPE(quest) == AQ_MOB_SAVE)
      check_rescue_goal(ch, quest, entry->captive, entry->recovery);
    free(entry);
  }
}

void check_rescue_on_kill(struct char_data *ch, struct char_data *vict,
                          room_rnum death_room)
{
  qst_rnum quest;

  if (!VALID_ROOM_RNUM(death_room) || !alive_in_room(ch, death_room) ||
      IS_NPC(ch) || GET_QUEST(ch) == NOTHING ||
      !vict || !IS_NPC(vict) || !DEAD(vict) || is_player_pet_kill(vict))
    return;

  quest = real_quest(GET_QUEST(ch));
  if (quest != NOTHING && QST_TYPE(quest) == AQ_MOB_SAVE &&
      GET_MOB_VNUM(vict) != QST_TARGET(quest))
    check_rescue_goal(ch, quest, NULL, FALSE);
}

typedef void side_check_fn(struct char_data *ch, void *data);

/* Save mob and Room clear credit goes to the side of whoever acted: a
 * player's charmed pet acts for its master, and a player for their group.
 * check runs for each player on that side who is alive and in room.  The
 * group gets its own iterator, since a group spell may already be walking it
 * with simple_list(). */
static void for_side_in_room(struct char_data *ch, room_rnum room,
                             side_check_fn *check, void *data)
{
  struct iterator_data iterator;
  struct char_data *i;

  if (is_player_pet(ch))
    ch = ch->master;
  if (!GROUP(ch)) {
    if (!IS_NPC(ch) && alive_in_room(ch, room))
      check(ch, data);
    return;
  }
  for (i = (struct char_data *) merge_iterator(&iterator, GROUP(ch)->members);
       i; i = (struct char_data *) next_in_list(&iterator))
    if (!IS_NPC(i) && alive_in_room(i, room))
      check(i, data);
  remove_iterator(&iterator);
}

/* The killer and the players on their side who were there when the victim
 * died.  A death script can move people in or out before raw_kill() gives
 * credit. */
struct kill_witnesses {
  struct char_data *killer;
  struct char_data **who;
  int count;
  int size;
};

static void add_witness(struct char_data *ch, void *data)
{
  struct kill_witnesses *witnesses = data;

  if (witnesses->count == witnesses->size) {
    witnesses->size = witnesses->size ? 2 * witnesses->size : 4;
    RECREATE(witnesses->who, struct char_data *, witnesses->size);
  }
  witnesses->who[witnesses->count++] = ch;
}

struct kill_witnesses *note_kill_witnesses(struct char_data *killer,
                                           room_rnum death_room)
{
  struct kill_witnesses *witnesses;

  if (!VALID_ROOM_RNUM(death_room) || !alive_in_room(killer, death_room))
    return NULL;
  CREATE(witnesses, struct kill_witnesses, 1);
  witnesses->killer = killer;
  for_side_in_room(killer, death_room, add_witness, witnesses);
  return witnesses;
}

/* Each witness still in the death room gets credit for vict's death and
 * the list is freed.  A pet's or a player's death clears nothing.  The
 * check is for the quest the witness holds now: a kill stage that this death
 * finished may have handed them a Save mob or Room clear stage, and the same
 * death counts toward it.  A witness gets one check, so a finished rescue
 * does not go on to the stage after it. */
void credit_kill_witnesses(struct kill_witnesses *witnesses,
                           struct char_data *vict, room_rnum death_room)
{
  int n;

  if (!witnesses)
    return;
  if (IS_NPC(vict) && !is_player_pet_kill(vict) && alive_in_room(witnesses->killer, death_room))
    for (n = 0; n < witnesses->count; n++) {
      struct char_data *ch = witnesses->who[n];
      qst_rnum quest;

      if (!alive_in_room(ch, death_room) || GET_QUEST(ch) == NOTHING ||
          (quest = real_quest(GET_QUEST(ch))) == NOTHING)
        continue;
      if (QST_TYPE(quest) == AQ_MOB_SAVE)
        check_rescue_on_kill(ch, vict, death_room);
      else if (QST_TYPE(quest) == AQ_ROOM_CLEAR)
        autoquest_trigger_check(ch, NULL, NULL, AQ_ROOM_CLEAR);
    }
  free(witnesses->who);
  free(witnesses);
}

/* One player on the healer's side, in the patient's room. */
static void rescue_on_recovery(struct char_data *ch, void *data)
{
  struct char_data *patient = data;
  qst_rnum quest;

  if (GET_QUEST(ch) == NOTHING)
    return;

  quest = real_quest(GET_QUEST(ch));
  if (quest != NOTHING && QST_TYPE(quest) == AQ_MOB_SAVE &&
      GET_MOB_VNUM(patient) == QST_TARGET(quest))
    check_rescue_goal(ch, quest, patient, TRUE);
}

/* Bringing the captive back from dying finishes a rescue for the healer's
 * side, as a kill does for the killer's. */
void check_rescue_on_recovery(struct char_data *ch, struct char_data *patient,
                              int old_position)
{
  if ((old_position != POS_INCAP && old_position != POS_MORTALLYW) ||
      !patient || !IS_NPC(patient) || DEAD(patient) ||
      GET_POS(patient) < POS_STUNNED || !VALID_ROOM_RNUM(IN_ROOM(patient)) ||
      !alive_in_room(ch, IN_ROOM(patient)))
    return;

  for_side_in_room(ch, IN_ROOM(patient), rescue_on_recovery, patient);
}

void autoquest_trigger_check(struct char_data *ch, struct char_data *vict,
                struct obj_data *object, int type)
{
  struct char_data *i;
  qst_rnum rnum;

  if (IS_NPC(ch))
    return;
  if (GET_QUEST(ch) == NOTHING)  /* No current quest, skip this */
    return;
  if (GET_QUEST_TYPE(ch) != type)
    return;
  if ((rnum = real_quest(GET_QUEST(ch))) == NOTHING)
    return;
  switch (type) {
    case AQ_OBJ_FIND:
      if (QST_TARGET(rnum) == GET_OBJ_VNUM(object))
        generic_complete_quest(ch);
      break;
    case AQ_ROOM_FIND:
      if (QST_TARGET(rnum) == world[IN_ROOM(ch)].number)
        generic_complete_quest(ch);
      break;
    case AQ_MOB_FIND:
      for (i=world[IN_ROOM(ch)].people; i; i = i->next_in_room)
        if (IS_NPC(i))
          if (QST_TARGET(rnum) == GET_MOB_VNUM(i))
            generic_complete_quest(ch);
      break;
    case AQ_MOB_KILL:
      if (!IS_NPC(ch) && IS_NPC(vict) && (ch != vict))
          if (QST_TARGET(rnum) == GET_MOB_VNUM(vict))
            generic_complete_quest(ch);
      break;
    case AQ_MOB_SAVE:
      if (vict)
        check_rescue_on_kill(ch, vict, IN_ROOM(vict));
      break;
    case AQ_OBJ_RETURN:
      if (IS_NPC(vict) && (GET_MOB_VNUM(vict) == QST_RETURNMOB(rnum)))
        if (object && (GET_OBJ_VNUM(object) == QST_TARGET(rnum))) {
          generic_complete_quest(ch);
          extract_obj(object);
        }
      break;
    case AQ_ROOM_CLEAR:
      if (QST_TARGET(rnum) == world[IN_ROOM(ch)].number &&
          quest_room_is_clear(IN_ROOM(ch), rnum))
        generic_complete_quest(ch);
      break;
    default:
      log("SYSERR: Invalid quest type passed to autoquest_trigger_check");
      break;
  }
}

void quest_timeout(struct char_data *ch)
{
  if ((GET_QUEST(ch) != NOTHING) && (GET_QUEST_TIME(ch) != -1)) {
    clear_quest(ch);
    send_to_char(ch, "You have run out of time to complete the quest.\r\n");
  }
}

void check_timed_quests(void)
{
  struct char_data *ch;

  for (ch = character_list; ch; ch = ch->next)
    if (!IS_NPC(ch) && (GET_QUEST(ch) != NOTHING) && (GET_QUEST_TIME(ch) != -1))
      if (GET_QUEST_TIME(ch) <= 0 || --GET_QUEST_TIME(ch) == 0)
        quest_timeout(ch);
}

/*--------------------------------------------------------------------------*/
/* Quest Command Helper Functions                                           */
/*--------------------------------------------------------------------------*/

void list_quests(struct char_data *ch, zone_rnum zone, qst_vnum vmin, qst_vnum vmax)
{
  qst_rnum rnum;
  qst_vnum bottom, top;
  int counter = 0;

  if (zone != NOWHERE) {
    bottom = zone_table[zone].bot;
    top    = zone_table[zone].top;
  } else {
    bottom = vmin;
    top    = vmax;
  }
  /* Print the header for the quest listing. */
  send_to_char (ch,
  "Index VNum    Description                                  Questmaster\r\n"
  "----- ------- -------------------------------------------- -----------\r\n");
  for (rnum = 0; rnum < total_quests ; rnum++)
    if (QST_NUM(rnum) >= bottom && QST_NUM(rnum) <= top)
      send_to_char(ch, "\tg%4d\tn) [\tg%-5d\tn] \tc%-44.44s\tn \ty[%5d]\tn\r\n",
          ++counter, QST_NUM(rnum), QST_NAME(rnum),
          QST_MASTER(rnum) == NOBODY ? 0 : QST_MASTER(rnum));
  if (!counter)
    send_to_char(ch, "None found.\r\n");
}

static void quest_hist(struct char_data *ch)
{
  int i = 0, counter = 0;
  qst_rnum rnum = NOTHING;

  send_to_char(ch, "Quests that you have completed:\r\n"
    "Index Description                                          Questmaster\r\n"
    "----- ---------------------------------------------------- -----------\r\n");
  for (i = 0; i < GET_NUM_QUESTS(ch); i++) {
    if ((rnum = real_quest(ch->player_specials->saved.completed_quests[i])) != NOTHING)
      send_to_char(ch, "\tg%4d\tn) \tc%-52.52s\tn \ty%s\tn\r\n",
 ++counter, QST_DESC(rnum), (real_mobile(QST_MASTER(rnum)) == NOBODY) ? "Unknown" : GET_NAME(&mob_proto[(real_mobile(QST_MASTER(rnum)))]));
    else
      send_to_char(ch,
        "\tg%4d\tn) \tcUnknown Quest (it no longer exists)\tn\r\n", ++counter);
  }
  if (!counter)
    send_to_char(ch, "You haven't completed any quests yet.\r\n");
}

static void quest_join(struct char_data *ch, struct char_data *qm, char argument[MAX_INPUT_LENGTH])
{
  qst_vnum vnum;
  qst_rnum rnum;
  char buf[MAX_INPUT_LENGTH];

  if (!*argument)
    snprintf(buf, sizeof(buf),
             "%s What quest did you wish to join?", GET_NAME(ch));
  else if (GET_QUEST(ch) != NOTHING)
    snprintf(buf, sizeof(buf),
             "%s But you are already part of a quest!", GET_NAME(ch));
  else if((vnum = find_quest_by_qmnum(ch, GET_MOB_VNUM(qm), atoi(argument))) == NOTHING)
    snprintf(buf, sizeof(buf),
             "%s I don't know of such a quest!", GET_NAME(ch));
  else if ((rnum = real_quest(vnum)) == NOTHING)
    snprintf(buf, sizeof(buf),
             "%s I don't know of such a quest!", GET_NAME(ch));
  else if (GET_LEVEL(ch) < QST_MINLEVEL(rnum))
    snprintf(buf, sizeof(buf),
             "%s You are not experienced enough for that quest!", GET_NAME(ch));
  else if (GET_LEVEL(ch) > QST_MAXLEVEL(rnum))
    snprintf(buf, sizeof(buf),
             "%s You are too experienced for that quest!", GET_NAME(ch));
  else if (is_complete(ch, vnum))
    snprintf(buf, sizeof(buf),
             "%s You have already completed that quest!", GET_NAME(ch));
  else if ((QST_PREV(rnum) != NOTHING) && !is_complete(ch, QST_PREV(rnum)))
    snprintf(buf, sizeof(buf),
             "%s That quest is not available to you yet!", GET_NAME(ch));
  else if ((QST_PREREQ(rnum) != NOTHING) &&
           (real_object(QST_PREREQ(rnum)) != NOTHING) &&
           (get_obj_in_list_num(real_object(QST_PREREQ(rnum)),
       ch->carrying) == NULL))
    snprintf(buf, sizeof(buf),
             "%s You need to have %s first!", GET_NAME(ch),
      obj_proto[real_object(QST_PREREQ(rnum))].short_description);
  else {
    act("You join the quest.",    TRUE, ch, NULL, NULL, TO_CHAR);
    act("$n has joined a quest.", TRUE, ch, NULL, NULL, TO_ROOM);
    snprintf(buf, sizeof(buf),
             "%s Listen carefully to the instructions.", GET_NAME(ch));
    do_tell(qm, buf, cmd_tell, 0);
    set_quest(ch, rnum);
    send_to_char(ch, "%s", QST_INFO(rnum));
    if (QST_TIME(rnum) != -1)
      snprintf(buf, sizeof(buf),
        "%s You have a time limit of %d turn%s to complete the quest.",
        GET_NAME(ch), QST_TIME(rnum), QST_TIME(rnum) == 1 ? "" : "s");
    else
      snprintf(buf, sizeof(buf),
        "%s You can take however long you want to complete the quest.",
 GET_NAME(ch));
  }
  do_tell(qm, buf, cmd_tell, 0);
  save_char(ch);
}

void quest_list(struct char_data *ch, struct char_data *qm, char argument[MAX_INPUT_LENGTH])
{
  qst_vnum vnum;
  qst_rnum rnum, prev;

  if ((vnum = find_quest_by_qmnum(ch, GET_MOB_VNUM(qm), atoi(argument))) == NOTHING)
    send_to_char(ch, "That is not a valid quest!\r\n");
  else if ((rnum = real_quest(vnum)) == NOTHING)
    send_to_char(ch, "That is not a valid quest!\r\n");
  else if (QST_INFO(rnum)) {
    send_to_char(ch,"Complete Details on Quest %d \tc%s\tn:\r\n%s",
                      vnum,
         QST_DESC(rnum),
         QST_INFO(rnum));
    if (QST_PREV(rnum) != NOTHING) {
      /* Holding a vnum and that vnum still resolving are two different
       * things.  Deleting a quest leaves every quest that named it as a
       * prerequisite holding a link real_quest() now answers NOTHING for,
       * and QST_NAME() is a bare aquest_table[i], so the read is
       * aquest_table[65535].name.  quest_stat() guards the same dangling
       * link on the immortal side; this is the one a mortal reaches. */
      if ((prev = real_quest(QST_PREV(rnum))) == NOTHING)
        send_to_char(ch, "You have to have completed an unknown quest first.\r\n");
      else
        send_to_char(ch, "You have to have completed quest %s first.\r\n",
            QST_NAME(prev));
    }
    if (QST_TIME(rnum) != -1)
      send_to_char(ch,
         "There is a time limit of %d turn%s to complete the quest.\r\n",
          QST_TIME(rnum),
          QST_TIME(rnum) == 1 ? "" : "s");
  } else
    send_to_char(ch, "There is no further information on that quest.\r\n");
}

static void quest_quit(struct char_data *ch)
{
  qst_rnum rnum;

  if (GET_QUEST(ch) == NOTHING)
    send_to_char(ch, "But you currently aren't on a quest!\r\n");
  else if ((rnum = real_quest(GET_QUEST(ch))) == NOTHING) {
    clear_quest(ch);
    send_to_char(ch, "You are now no longer part of the quest.\r\n");
    save_char(ch);
  } else {
    clear_quest(ch);
    if (QST_QUIT(rnum) && (str_cmp(QST_QUIT(rnum), "undefined") != 0))
      send_to_char(ch, "%s", QST_QUIT(rnum));
    else
      send_to_char(ch, "You are now no longer part of the quest.\r\n");
    if (QST_PENALTY(rnum)) {
      GET_QUESTPOINTS(ch) -= QST_PENALTY(rnum);
      send_to_char(ch,
        "You have lost %d quest points for your cowardice.\r\n",
        QST_PENALTY(rnum));
    }
    save_char(ch);
  }
}

static void quest_progress(struct char_data *ch)
{
  qst_rnum rnum;

  if (GET_QUEST(ch) == NOTHING)
    send_to_char(ch, "But you currently aren't on a quest!\r\n");
  else if ((rnum = real_quest(GET_QUEST(ch))) == NOTHING) {
    clear_quest(ch);
    send_to_char(ch, "Your quest seems to no longer exist.\r\n");
  } else {
    send_to_char(ch, "You are on the following quest:\r\n%s\r\n%s",
        QST_DESC(rnum), QST_INFO(rnum));
    if (QST_QUANTITY(rnum) > 1)
      send_to_char(ch,
          "You still have to achieve %d out of %d goals for the quest.\r\n",
   GET_QUEST_COUNTER(ch), QST_QUANTITY(rnum));
    if (GET_QUEST_TIME(ch) > 0)
      send_to_char(ch,
          "You have %d turn%s remaining to complete the quest.\r\n",
   GET_QUEST_TIME(ch),
   GET_QUEST_TIME(ch) == 1 ? "" : "s");
  }
}

static void quest_show(struct char_data *ch, mob_vnum qm)
{
  qst_rnum rnum;
  int counter = 0;

  send_to_char(ch,
  "The following quests are available:\r\n"
  "Index Description                                          ( Vnum) Done?\r\n"
  "----- ---------------------------------------------------- ------- -----\r\n");
  for (rnum = 0; rnum < total_quests; rnum++)
    if (qm == QST_MASTER(rnum))
      send_to_char(ch, "\tg%4d\tn) \tc%-52.52s\tn \ty(%5d)\tn \ty(%s)\tn\r\n",
        ++counter, QST_DESC(rnum), QST_NUM(rnum),
        (is_complete(ch, QST_NUM(rnum)) ? "Yes" : "No "));
  if (!counter)
    send_to_char(ch, "There are no quests available here at the moment.\r\n");
}

static void quest_stat(struct char_data *ch, char *argument)
{
  qst_rnum rnum;
  mob_rnum qmrnum;
  char buf[MAX_STRING_LENGTH];
  char targetname[MAX_STRING_LENGTH];

  if (!*argument)
    send_to_char(ch, "%s\r\n", quest_imm_usage);
  else if ((rnum = real_quest(atoi(argument))) == NOTHING )
    send_to_char(ch, "That quest does not exist.\r\n");
  else {
    sprintbit(QST_FLAGS(rnum), aq_flags, buf, sizeof(buf));
    switch (QST_TYPE(rnum)) {
      case AQ_OBJ_FIND:
      case AQ_OBJ_RETURN:
        snprintf(targetname, sizeof(targetname), "%s",
                 real_object(QST_TARGET(rnum)) == NOTHING ?
                 "An unknown object" :
    obj_proto[real_object(QST_TARGET(rnum))].short_description);
 break;
      case AQ_ROOM_FIND:
      case AQ_ROOM_CLEAR:
        snprintf(targetname, sizeof(targetname), "%s",
          real_room(QST_TARGET(rnum)) == NOWHERE ?
                 "An unknown room" :
    world[real_room(QST_TARGET(rnum))].name);
        break;
      case AQ_MOB_FIND:
      case AQ_MOB_KILL:
      case AQ_MOB_SAVE:
 snprintf(targetname, sizeof(targetname), "%s",
                 real_mobile(QST_TARGET(rnum)) == NOBODY ?
    "An unknown mobile" :
    GET_NAME(&mob_proto[real_mobile(QST_TARGET(rnum))]));
 break;
      default:
 snprintf(targetname, sizeof(targetname), "Unknown");
 break;
    }
    qmrnum = real_mobile(QST_MASTER(rnum));
    send_to_char(ch,
        "VNum  : [\ty%5d\tn], RNum: [\ty%5d\tn] -- Questmaster: [\ty%5d\tn] \ty%s\tn\r\n"
        "Name  : \ty%s\tn\r\n"
 "Desc  : \ty%s\tn\r\n"
 "Accept Message:\r\n\tc%s\tn"
 "Completion Message:\r\n\tc%s\tn"
 "Quit Message:\r\n\tc%s\tn"
 "Type  : \ty%s\tn\r\n"
        "Target: \ty%d\tn \ty%s\tn, Quantity: \ty%d\tn\r\n"
 "Value : \ty%d\tn, Penalty: \ty%d\tn, Min Level: \ty%2d\tn, Max Level: \ty%2d\tn\r\n"
 "Flags : \tc%s\tn\r\n",
     QST_NUM(rnum), rnum,
 QST_MASTER(rnum) == NOBODY ? -1 : QST_MASTER(rnum),
 (qmrnum == NOBODY) ? "(Invalid vnum)" : GET_NAME(&mob_proto[(qmrnum)]),
        QST_NAME(rnum), QST_DESC(rnum),
        QST_INFO(rnum), QST_DONE(rnum),
 (QST_QUIT(rnum) &&
  (str_cmp(QST_QUIT(rnum), "undefined") != 0)
          ? QST_QUIT(rnum) : "Nothing\r\n"),
     QST_TYPE_NAME(QST_TYPE(rnum)),
 QST_TARGET(rnum) == NOBODY ? -1 : QST_TARGET(rnum),
 targetname,
 QST_QUANTITY(rnum),
     QST_POINTS(rnum), QST_PENALTY(rnum), QST_MINLEVEL(rnum),
 QST_MAXLEVEL(rnum), buf);
    if (QST_PREREQ(rnum) != NOTHING)
      send_to_char(ch, "Preq  : [\ty%5d\tn] \ty%s\tn\r\n",
        QST_PREREQ(rnum) == NOTHING ? -1 : QST_PREREQ(rnum),
        QST_PREREQ(rnum) == NOTHING ? "" :
   real_object(QST_PREREQ(rnum)) == NOTHING ? "an unknown object" :
       obj_proto[real_object(QST_PREREQ(rnum))].short_description);
    if (QST_TYPE(rnum) == AQ_OBJ_RETURN)
      send_to_char(ch, "Mob   : [\ty%5d\tn] \ty%s\tn\r\n",
        QST_RETURNMOB(rnum),
 real_mobile(QST_RETURNMOB(rnum)) == NOBODY ? "an unknown mob" :
           mob_proto[real_mobile(QST_RETURNMOB(rnum))].player.short_descr);
    if (QST_TIME(rnum) != -1)
      send_to_char(ch, "Limit : There is a time limit of %d turn%s to complete.\r\n",
   QST_TIME(rnum),
   QST_TIME(rnum) == 1 ? "" : "s");
    else
      send_to_char(ch, "Limit : There is no time limit on this quest.\r\n");
    /* A chain link can outlive what it points at: deleting a quest leaves
     * every quest that named it holding a vnum real_quest() answers NOTHING
     * for, and QST_DESC(NOTHING) reads aquest_table[65535]. */
    send_to_char(ch, "Prior :");
    if (QST_PREV(rnum) == NOTHING)
      send_to_char(ch, " \tyNone.\tn\r\n");
    else
      send_to_char(ch, " [\ty%5d\tn] \tc%s\tn\r\n",
        QST_PREV(rnum), real_quest(QST_PREV(rnum)) == NOTHING ?
          "an unknown quest" : QST_DESC(real_quest(QST_PREV(rnum))));
    send_to_char(ch, "Next  :");
    if (QST_NEXT(rnum) == NOTHING)
      send_to_char(ch, " \tyNone.\tn\r\n");
    else
      send_to_char(ch, " [\ty%5d\tn] \tc%s\tn\r\n",
        QST_NEXT(rnum), real_quest(QST_NEXT(rnum)) == NOTHING ?
          "an unknown quest" : QST_DESC(real_quest(QST_NEXT(rnum))));
  }
}

/*--------------------------------------------------------------------------*/
/* Quest Command Processing Function and Questmaster Special                */
/*--------------------------------------------------------------------------*/

ACMD(do_quest)
{
  char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
  int  tp;

  two_arguments(argument, arg1, arg2);
  if (!*arg1)
    send_to_char(ch, "%s\r\n", GET_LEVEL(ch) < LVL_IMMORT ?
                     quest_mort_usage : quest_imm_usage);
  else if (((tp = search_block(arg1, quest_cmd, FALSE)) == -1))
    send_to_char(ch, "%s\r\n", GET_LEVEL(ch) < LVL_IMMORT ?
                     quest_mort_usage : quest_imm_usage);
  else {
    switch (tp) {
      case SCMD_QUEST_LIST:
      case SCMD_QUEST_JOIN:
        /* list, join should hve been handled by questmaster spec proc */
        send_to_char(ch, "Sorry, but you cannot do that here!\r\n");
        break;
      case SCMD_QUEST_HISTORY:
        quest_hist(ch);
        break;
      case SCMD_QUEST_LEAVE:
        quest_quit(ch);
        break;
      case SCMD_QUEST_PROGRESS:
 quest_progress(ch);
 break;
      case SCMD_QUEST_STATUS:
        if (GET_LEVEL(ch) < LVL_IMMORT)
          send_to_char(ch, "%s\r\n", quest_mort_usage);
        else
          quest_stat(ch, arg2);
        break;
      default: /* Whe should never get here, but... */
        send_to_char(ch, "%s\r\n", GET_LEVEL(ch) < LVL_IMMORT ?
                     quest_mort_usage : quest_imm_usage);
 break;
    } /* switch on subcmd number */
  }
}

SPECIAL(questmaster)
{
  qst_rnum rnum;
  char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
  int  tp;
  struct char_data *qm = (struct char_data *)me;

  /* check that qm mob has quests assigned */
  for (rnum = 0; (rnum < total_quests &&
      QST_MASTER(rnum) != GET_MOB_VNUM(qm)) ; rnum ++);
  if (rnum >= total_quests)
    return FALSE; /* No quests for this mob */
  else if (QST_FUNC(rnum) && (QST_FUNC(rnum) (ch, me, cmd, argument)))
    return TRUE;  /* The secondary spec proc handled this command */
  else if (CMD_IS("quest")) {
    two_arguments(argument, arg1, arg2);
    if (!*arg1)
      return FALSE;
    else if (((tp = search_block(arg1, quest_cmd, FALSE)) == -1))
      return FALSE;
    else {
      switch (tp) {
      case SCMD_QUEST_LIST:
        if (!*arg2)
          quest_show(ch, GET_MOB_VNUM(qm));
        else
   quest_list(ch, qm, arg2);
        break;
      case SCMD_QUEST_JOIN:
        quest_join(ch, qm, arg2);
        break;
      default:
 return FALSE; /* fall through to the do_quest command processor */
      } /* switch on subcmd number */
      return TRUE;
    }
  } else {
    return FALSE; /* not a questmaster command */
  }
}
