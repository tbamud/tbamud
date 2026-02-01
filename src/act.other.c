/**************************************************************************
*  File: act.other.c                                       Part of tbaMUD *
*  Usage: Miscellaneous player-level commands.                             *
*                                                                         *
*  All rights reserved.  See license for complete information.            *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
**************************************************************************/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "screen.h"
#include "house.h"
#include "constants.h"
#include "py_triggers.h"
#include "act.h"
#include "spec_procs.h"
#include "class.h"
#include "fight.h"
#include "mail.h"  /* for has_mail() */
#include "shop.h"
#include "quest.h"
#include "modify.h"

/* Local defined utility functions */
/* do_group utility functions */
static void print_group(struct char_data *ch);
static void display_group_list(struct char_data * ch);

static bool change_has_emote_tokens(const char *text)
{
  for (; text && *text; text++) {
    switch (*text) {
      case '~': case '!': case '%': case '^':
      case '#': case '&': case '=': case '+':
      case '@':
        return TRUE;
    }
  }
  return FALSE;
}

static bool change_ends_with_punct(const char *text)
{
  size_t len = text ? strlen(text) : 0;
  if (len == 0)
    return FALSE;
  return (text[len - 1] == '.' || text[len - 1] == '!' || text[len - 1] == '?');
}

static void change_trim_trailing_spaces(char *text)
{
  size_t len = text ? strlen(text) : 0;
  while (len > 0 && isspace((unsigned char)text[len - 1])) {
    text[len - 1] = '\0';
    len--;
  }
}

#define REROLL_REVIEW_SECONDS (2 * SECS_PER_REAL_HOUR)

static void reroll_clear_saved(struct char_data *ch)
{
  GET_REROLL_EXPIRES(ch) = 0;
  memset(&GET_REROLL_OLD_ABILS(ch), 0, sizeof(struct char_ability_data));
}

ACMD(do_quit)
{
  char first[MAX_INPUT_LENGTH];
  char *rest;

  if (IS_NPC(ch) || !ch->desc)
    return;

  /* Parse optional "ooc" sub-arg: quit ooc <message> */
  rest = (char *)argument;
  skip_spaces(&rest);
  rest = one_argument(rest, first);
  bool quit_ooc = (*first && is_abbrev(first, "ooc")) ? TRUE : FALSE;

  /* Keep original safety controls */
  if (!quit_ooc && subcmd != SCMD_QUIT && GET_LEVEL(ch) < LVL_IMMORT)
    send_to_char(ch, "You have to type quit--no less, to quit!\r\n");
  else if (GET_POS(ch) == POS_FIGHTING)
    send_to_char(ch, "No way!  You're fighting for your life!\r\n");
  else if (GET_POS(ch) < POS_STUNNED) {
    send_to_char(ch, "You die before your time...\r\n");
    die(ch, NULL);
  }
  /* New: normal quit must be in a QUIT room (mortals only). */
  else if (!quit_ooc && GET_LEVEL(ch) < LVL_IMMORT &&
           !ROOM_FLAGGED(IN_ROOM(ch), ROOM_QUIT)) {
    send_to_char(ch, "You cannot quit here. Find a room marked [Quit].\r\n");
  }
  /* For quit ooc, require a message for staff context. */
  else if (quit_ooc) {
    skip_spaces(&rest);
    if (!*rest) {
      send_to_char(ch, "Usage must include a reason to quit ooc: quit ooc <message>\r\n");
      return;
    }

    act("$n has left the game.", TRUE, ch, 0, 0, TO_ROOM);
    mudlog(CMP, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE,
           "%s used QUIT OOC in room %d: %s",
           GET_NAME(ch), GET_ROOM_VNUM(IN_ROOM(ch)), rest);

    if (GET_QUEST_TIME(ch) != -1)
      quest_timeout(ch);

    send_to_char(ch, "You step out-of-character and leave the world...\r\n");

    /* Save character and objects */
    save_char(ch);
    Crash_rentsave(ch, 0);

    /* Requirement: respawn in the same (possibly non-QUIT) room. */
    GET_LOADROOM(ch) = GET_ROOM_VNUM(IN_ROOM(ch));

    /* Stop snooping so you can't see passwords during deletion or change. */
    if (ch->desc->snoop_by) {
      write_to_output(ch->desc->snoop_by, "Your victim is no longer among us.\r\n");
      ch->desc->snoop_by->snooping = NULL;
      ch->desc->snoop_by = NULL;
    }

    extract_char(ch);   /* Char is saved before extracting. */
  }
  else {
    /* Normal quit (in a QUIT room, or immortal bypass) */
    act("$n has left the game.", TRUE, ch, 0, 0, TO_ROOM);
    mudlog(NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE,
           "%s has quit the game.", GET_NAME(ch));

    if (GET_QUEST_TIME(ch) != -1)
      quest_timeout(ch);

    send_to_char(ch, "Goodbye, friend.. Come back soon!\r\n");

    /* Save character and objects */
    save_char(ch);
    Crash_rentsave(ch, 0);

    /* Requirement: respawn in the same QUIT room they logged out in. */
    GET_LOADROOM(ch) = GET_ROOM_VNUM(IN_ROOM(ch));

    /* Stop snooping so you can't see passwords during deletion or change. */
    if (ch->desc->snoop_by) {
      write_to_output(ch->desc->snoop_by, "Your victim is no longer among us.\r\n");
      ch->desc->snoop_by->snooping = NULL;
      ch->desc->snoop_by = NULL;
    }

    SET_BIT_AR(PLR_FLAGS(ch), PLR_QUITING);
    extract_char(ch);   /* Char is saved before extracting. */
  }
}

ACMD(do_save)
{
  char a1[MAX_INPUT_LENGTH], a2[MAX_INPUT_LENGTH];

  /* Accept both orders: "save room" or "room save" */
  two_arguments(argument, a1, a2);

  /* order-agnostic check */
  int wants_room = ((*a1 && !str_cmp(a1, "room")) ||
                    (*a2 && !str_cmp(a2, "room")));

  if (wants_room) {
    room_rnum rnum = IN_ROOM(ch);

    if (rnum == NOWHERE) {
      send_to_char(ch, "You're not in a valid room.\r\n");
      return;
    }

    /* Not a SAVE room? Fall back to normal character save semantics. */
    if (!ROOM_FLAGGED(rnum, ROOM_SAVE)) {
      send_to_char(ch, "Saving %s.\r\n", GET_NAME(ch));
      save_char(ch);
      Crash_crashsave(ch);   /* keep whatever your tree normally calls */
      return;
    }

    /* Room is flagged SAVE → persist its contents */
    if (RoomSave_now(rnum)) {
      send_to_char(ch, "Saving room.\r\n");
      mudlog(NRM, LVL_IMMORT, FALSE,
             "RoomSave: manual save of room %d by %s.",
             world[rnum].number, GET_NAME(ch));
      /* If you added a dirty-save API and want to clear the bit on manual save,
         you can optionally call it here (guard with a macro if desired):
         #ifdef ROOMSAVE_HAVE_DIRTY_API
         RoomSave_clear_dirty(rnum);
         #endif
      */
    } else {
      send_to_char(ch, "Room save failed; see logs.\r\n");
      mudlog(NRM, LVL_IMMORT, TRUE,
             "SYSERR: RoomSave: manual save FAILED for room %d by %s.",
             world[rnum].number, GET_NAME(ch));
    }
    return;
  }

  /* No "room" token present → normal character save */
  send_to_char(ch, "Saving %s.\r\n", GET_NAME(ch));
  save_char(ch);
  Crash_crashsave(ch);
}

ACMD(do_change)
{
  char option[MAX_INPUT_LENGTH];
  char suffix[MAX_INPUT_LENGTH];
  char base_buf[MAX_INPUT_LENGTH];
  char ldesc[MAX_STRING_LENGTH];
  char *rest = argument;
  const char *base;

  rest = one_argument(rest, option);
  if (!*option) {
    send_to_char(ch, "Usage: change ldesc <string>\r\n");
    return;
  }

  if (!is_abbrev(option, "ldesc")) {
    send_to_char(ch, "Unknown change option. Available: ldesc\r\n");
    return;
  }

  skip_spaces(&rest);
  if (!*rest) {
    send_to_char(ch, "Usage: change ldesc <string>\r\n");
    return;
  }

  if (change_has_emote_tokens(rest)) {
    send_to_char(ch, "You can't use emote tokens in your ldesc.\r\n");
    return;
  }

  strlcpy(suffix, rest, sizeof(suffix));
  change_trim_trailing_spaces(suffix);
  if (!*suffix) {
    send_to_char(ch, "Usage: change ldesc <string>\r\n");
    return;
  }

  if (!change_ends_with_punct(suffix))
    strlcat(suffix, ".", sizeof(suffix));

  base = (GET_SHORT_DESC(ch) && *GET_SHORT_DESC(ch)) ? GET_SHORT_DESC(ch) : GET_NAME(ch);
  if (!base || !*base)
    base = "someone";

  strlcpy(base_buf, base, sizeof(base_buf));
  if (*base_buf)
    base_buf[0] = UPPER(*base_buf);

  snprintf(ldesc, sizeof(ldesc), "%s %s\r\n", base_buf, suffix);

  if (ch->player.long_descr) {
    if (!IS_NPC(ch) || GET_MOB_RNUM(ch) == NOBODY ||
        ch->player.long_descr != mob_proto[GET_MOB_RNUM(ch)].player.long_descr) {
      free(ch->player.long_descr);
    }
  }
  ch->player.long_descr = strdup(ldesc);
  ch->char_specials.custom_ldesc = TRUE;

  send_to_char(ch, "Long description updated.\r\n");
}

ACMD(do_reroll)
{
  char arg[MAX_INPUT_LENGTH];
  struct char_data *vict;
  time_t now;
  time_t remaining;
  bool expired = FALSE;

  one_argument(argument, arg);

  if (IS_NPC(ch)) {
    send_to_char(ch, "You can't reroll stats.\r\n");
    return;
  }

  now = time(0);
  if (GET_REROLL_EXPIRES(ch) > 0 && now >= GET_REROLL_EXPIRES(ch)) {
    reroll_clear_saved(ch);
    save_char(ch);
    expired = TRUE;
  }

  if (*arg && is_abbrev(arg, "undo")) {
    if (!GET_REROLL_USED(ch)) {
      send_to_char(ch, "You haven't rerolled yet.\r\n");
      return;
    }
    if (GET_REROLL_EXPIRES(ch) <= 0 || expired) {
      send_to_char(ch, "You no longer have a reroll pending; your stats are permanent.\r\n");
      return;
    }

    ch->real_abils = GET_REROLL_OLD_ABILS(ch);
    affect_total(ch);
    reroll_clear_saved(ch);
    save_char(ch);

    send_to_char(ch, "Your original stats have been restored. You cannot reroll again.\r\n");
    send_to_char(ch, "Stats: Str %d, Int %d, Wis %d, Dex %d, Con %d, Cha %d\r\n",
                 GET_STR(ch), GET_INT(ch), GET_WIS(ch),
                 GET_DEX(ch), GET_CON(ch), GET_CHA(ch));
    return;
  }

  if (*arg && GET_LEVEL(ch) >= LVL_GRGOD) {
    if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_WORLD)))
      send_to_char(ch, "There is no such player.\r\n");
    else if (IS_NPC(vict))
      send_to_char(ch, "You can't do that to a mob!\r\n");
    else {
      roll_real_abils(vict);
      affect_total(vict);
      send_to_char(ch, "Rerolled...\r\n");
      log("(GC) %s has rerolled %s.", GET_NAME(ch), GET_NAME(vict));
      send_to_char(ch, "New stats: Str %d, Int %d, Wis %d, Dex %d, Con %d, Cha %d\r\n",
                   GET_STR(vict), GET_INT(vict), GET_WIS(vict),
                   GET_DEX(vict), GET_CON(vict), GET_CHA(vict));
      save_char(vict);
    }
    return;
  }

  if (*arg) {
    send_to_char(ch, "Usage: reroll | reroll undo\r\n");
    return;
  }

  if (GET_REROLL_USED(ch)) {
    if (GET_REROLL_EXPIRES(ch) > 0 && now < GET_REROLL_EXPIRES(ch)) {
      remaining = GET_REROLL_EXPIRES(ch) - now;
      int hours = remaining / SECS_PER_REAL_HOUR;
      int mins = (remaining % SECS_PER_REAL_HOUR) / SECS_PER_REAL_MIN;

      if (hours > 0)
        send_to_char(ch, "You have already rerolled. You can 'reroll undo' within %d hour%s %d minute%s.\r\n",
                     hours, hours == 1 ? "" : "s", mins, mins == 1 ? "" : "s");
      else
        send_to_char(ch, "You have already rerolled. You can 'reroll undo' within %d minute%s.\r\n",
                     mins, mins == 1 ? "" : "s");
    } else {
      send_to_char(ch, "You have already rerolled and cannot reroll again.\r\n");
    }
    return;
  }

  GET_REROLL_OLD_ABILS(ch) = ch->real_abils;
  roll_real_abils(ch);
  affect_total(ch);
  GET_REROLL_USED(ch) = TRUE;
  GET_REROLL_EXPIRES(ch) = now + REROLL_REVIEW_SECONDS;
  save_char(ch);

  send_to_char(ch, "Your stats have been rerolled. You have 2 hours to review them.\r\n");
  send_to_char(ch, "New stats: Str %d, Int %d, Wis %d, Dex %d, Con %d, Cha %d\r\n",
               GET_STR(ch), GET_INT(ch), GET_WIS(ch),
               GET_DEX(ch), GET_CON(ch), GET_CHA(ch));
  send_to_char(ch, "Use 'reroll undo' to restore your original stats before the timer expires.\r\n");
}

/* Generic function for commands which are normally overridden by special
 * procedures - i.e., shop commands, mail commands, etc. */
ACMD(do_not_here)
{
  send_to_char(ch, "Sorry, but you cannot do that here!\r\n");
}

#define STEALTH_BASE_DC 10
#define SLEIGHT_BASE_DC 10

int get_stealth_skill_value(struct char_data *ch)
{
  int skill = GET_SKILL(ch, SKILL_STEALTH);
  int legacy = MAX(GET_SKILL(ch, SKILL_HIDE), GET_SKILL(ch, SKILL_SNEAK));

  if (skill <= 0 && legacy > 0) {
    skill = MIN(legacy, 100);
    SET_SKILL(ch, SKILL_STEALTH, skill);
  }

  return skill;
}

int roll_stealth_check(struct char_data *ch)
{
  int mode = has_stealth_disadv(ch) ? -1 : 0;

  get_stealth_skill_value(ch);
  return roll_skill_check(ch, SKILL_STEALTH, mode, NULL);
}

int roll_sleight_check(struct char_data *ch)
{
  int total = roll_skill_check(ch, SKILL_SLEIGHT_OF_HAND, 0, NULL);

  if (FIGHTING(ch))
    total -= 4;

  return total;
}

static int compute_steal_dc(struct char_data *thief, struct char_data *vict, int weight)
{
  int dc = SLEIGHT_BASE_DC + MAX(0, weight);

  if (!vict)
    return dc;

  if (GET_LEVEL(vict) >= LVL_IMMORT)
    return 1000;

  if (GET_POS(vict) < POS_SLEEPING)
    return MAX(0, weight);

  if (!AWAKE(vict)) {
    dc = MAX(0, dc - 5);
    if (thief && AFF_FLAGGED(thief, AFF_HIDE))
      dc -= 5;
    return MAX(0, dc);
  }

  dc += GET_ABILITY_MOD(GET_WIS(vict));
  if (GET_SKILL(vict, SKILL_PERCEPTION) > 0)
    dc += get_total_proficiency_bonus(vict);
  if (FIGHTING(vict))
    dc -= 4;
  if (AFF_FLAGGED(vict, AFF_SCAN))
    dc += 5;
  if (thief && AFF_FLAGGED(thief, AFF_HIDE))
    dc -= 5;
  if (GET_MOB_SPEC(vict) == shop_keeper)
    dc += 20;

  return dc;
}

static struct obj_data *find_container_on_character(struct char_data *viewer,
                                                    struct char_data *vict,
                                                    const char *name)
{
  struct obj_data *obj;
  int eq;

  if (!viewer || !vict || !name || !*name)
    return NULL;

  for (obj = vict->carrying; obj; obj = obj->next_content) {
    if (!CAN_SEE_OBJ(viewer, obj))
      continue;
    if (isname(name, obj->name))
      return obj;
  }

  for (eq = 0; eq < NUM_WEARS; eq++) {
    if (!(obj = GET_EQ(vict, eq)))
      continue;
    if (!CAN_SEE_OBJ(viewer, obj))
      continue;
    if (isname(name, obj->name))
      return obj;
  }

  return NULL;
}

static bool sleight_can_take_obj(struct char_data *ch, struct obj_data *obj)
{
  if (!obj)
    return FALSE;

  if (GET_OBJ_TYPE(obj) == ITEM_MONEY)
    update_money_obj(obj);

  if (!CAN_WEAR(obj, ITEM_WEAR_TAKE)) {
    act("$p: you can't take that!", FALSE, ch, obj, 0, TO_CHAR);
    return FALSE;
  }

  if (!IS_NPC(ch) && !PRF_FLAGGED(ch, PRF_NOHASSLE)) {
    if (IS_CARRYING_N(ch) >= CAN_CARRY_N(ch)) {
      act("$p: you can't carry that many items.", FALSE, ch, obj, 0, TO_CHAR);
      return FALSE;
    }
    if ((IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(obj)) > CAN_CARRY_W(ch)) {
      act("$p: you can't carry that much weight.", FALSE, ch, obj, 0, TO_CHAR);
      return FALSE;
    }
  }

  return TRUE;
}

static void sleight_check_money(struct char_data *ch, struct obj_data *obj)
{
  if (!obj || GET_OBJ_TYPE(obj) != ITEM_MONEY)
    return;
  update_money_obj(obj);
}

static int count_coins_in_list(const struct obj_data *list)
{
  int total = 0;
  const struct obj_data *obj;

  for (obj = list; obj; obj = obj->next_content) {
    if (GET_OBJ_TYPE(obj) == ITEM_MONEY)
      total += MAX(0, GET_OBJ_VAL(obj, 0));
    if (obj->contains)
      total += count_coins_in_list(obj->contains);
  }

  return total;
}

static int remove_coins_from_list(struct obj_data *list, int amount)
{
  struct obj_data *obj, *next_obj;
  int removed = 0;

  for (obj = list; obj && removed < amount; obj = next_obj) {
    next_obj = obj->next_content;

    if (GET_OBJ_TYPE(obj) == ITEM_MONEY) {
      int pile = MAX(0, GET_OBJ_VAL(obj, 0));
      int take = MIN(pile, amount - removed);

      if (take > 0) {
        if (take == pile) {
          removed += take;
          extract_obj(obj);
          continue;
        }
        GET_OBJ_VAL(obj, 0) = pile - take;
        update_money_obj(obj);
        removed += take;
      }
    }

    if (obj->contains && removed < amount)
      removed += remove_coins_from_list(obj->contains, amount - removed);
  }

  return removed;
}

static bool sleight_merge_money_pile(struct char_data *ch, struct obj_data *obj)
{
  struct obj_data *target;
  int coins;

  if (!ch || !obj || GET_OBJ_TYPE(obj) != ITEM_MONEY)
    return FALSE;

  for (target = ch->carrying; target; target = target->next_content) {
    if (target != obj && GET_OBJ_TYPE(target) == ITEM_MONEY)
      break;
  }

  if (!target)
    return FALSE;

  coins = MAX(0, GET_OBJ_VAL(obj, 0));
  if (coins <= 0)
    return FALSE;

  GET_OBJ_VAL(target, 0) += coins;
  update_money_obj(target);
  GET_COINS(ch) = MIN(MAX_COINS, GET_COINS(ch) + coins);
  extract_obj(obj);

  return TRUE;
}

static bool sleight_observer_notices(struct char_data *actor,
                                     struct char_data *viewer,
                                     int sleight_total)
{
  int d20, total;

  if (!viewer || viewer == actor)
    return FALSE;
  if (!AWAKE(viewer))
    return FALSE;

  if (GET_LEVEL(viewer) >= LVL_IMMORT) {
    gain_skill(viewer, "perception", TRUE);
    return TRUE;
  }

  if (can_scan_for_sneak(viewer)) {
    total = roll_skill_check(viewer, SKILL_PERCEPTION, 0, &d20);
    if (FIGHTING(viewer))
      total -= 4;
  } else {
    d20 = roll_d20();
    total = d20;
  }

  if (d20 == 1) {
    gain_skill(viewer, "perception", FALSE);
    return FALSE;
  }

  if (d20 == 20 || total >= sleight_total) {
    gain_skill(viewer, "perception", TRUE);
    return TRUE;
  }

  gain_skill(viewer, "perception", FALSE);
  return FALSE;
}

static void sleight_send_notice(struct char_data *viewer,
                                struct char_data *actor,
                                const char *verb,
                                const char *prep,
                                const char *item_desc,
                                const char *container_desc)
{
  char line[MAX_STRING_LENGTH];
  char actor_desc[MAX_INPUT_LENGTH];
  char item_clean[MAX_STRING_LENGTH];
  char cont_clean[MAX_STRING_LENGTH];

  strlcpy(actor_desc, PERS(actor, viewer), sizeof(actor_desc));
  strlcpy(item_clean, item_desc ? item_desc : "something", sizeof(item_clean));
  strlcpy(cont_clean, container_desc ? container_desc : "something", sizeof(cont_clean));

  if (!strn_cmp(item_clean, "a ", 2))
    memmove(item_clean, item_clean + 2, strlen(item_clean) - 1);
  else if (!strn_cmp(item_clean, "an ", 3))
    memmove(item_clean, item_clean + 3, strlen(item_clean) - 2);

  if (!strn_cmp(cont_clean, "a ", 2))
    memmove(cont_clean, cont_clean + 2, strlen(cont_clean) - 1);
  else if (!strn_cmp(cont_clean, "an ", 3))
    memmove(cont_clean, cont_clean + 3, strlen(cont_clean) - 2);

  snprintf(line, sizeof(line), "%s tries to %s %s %s %s %s.",
           actor_desc, verb, item_clean, prep, HSHR(actor), cont_clean);

  send_to_char(viewer, "You notice:\r\n  %s\r\n", line);
}

static void sleight_check_observers(struct char_data *actor,
                                    int sleight_total,
                                    const char *verb,
                                    const char *prep,
                                    const char *item_desc,
                                    const char *container_desc)
{
  struct char_data *viewer;

  if (!actor || IN_ROOM(actor) == NOWHERE)
    return;

  for (viewer = world[IN_ROOM(actor)].people; viewer; viewer = viewer->next_in_room) {
    if (viewer == actor)
      continue;
    if (sleight_observer_notices(actor, viewer, sleight_total))
      sleight_send_notice(viewer, actor, verb, prep, item_desc, container_desc);
  }
}

static int sneak_effect_duration(struct char_data *ch)
{
  int skill = get_stealth_skill_value(ch);
  if (skill <= 0)
    return 1;

  return MAX(1, skill / 10);
}

bool can_scan_for_sneak(struct char_data *ch)
{
  if (!AFF_FLAGGED(ch, AFF_SCAN))
    return FALSE;
  if (!GET_SKILL(ch, SKILL_PERCEPTION))
    return FALSE;
  if (AFF_FLAGGED(ch, AFF_BLIND))
    return FALSE;
  if (IS_DARK(IN_ROOM(ch)) && !CAN_SEE_IN_DARK(ch))
    return FALSE;
  return TRUE;
}

int roll_scan_perception(struct char_data *ch)
{
  int total = roll_skill_check(ch, SKILL_PERCEPTION, 0, NULL);

  if (FIGHTING(ch))
    total -= 4;

  return total;
}

static int listen_effect_duration(struct char_data *ch)
{
  int skill = GET_SKILL(ch, SKILL_PERCEPTION);

  if (skill <= 0)
    return 1;

  return MAX(1, skill / 10);
}

ACMD(do_sneak)
{
  struct affected_type af;
  int total, dc;
  int stealth_skill = get_stealth_skill_value(ch);

  if (!stealth_skill) {
    send_to_char(ch, "You have no idea how to do that.\r\n");
    return;
  }

  if (FIGHTING(ch)){
    send_to_char(ch, "While fighting!?\r\n");
    return; 
  }    /* you can't sneak while in active melee */

  send_to_char(ch, "Okay, you'll try to move silently for a while.\r\n");

  /* Remove prior sneak affect if present (refresh logic) */
  if (AFF_FLAGGED(ch, AFF_SNEAK))
    affect_from_char(ch, SKILL_SNEAK);

  /* --- 5e-style Stealth check (DEX + proficiency) --- */
  total = roll_stealth_check(ch);
  dc = STEALTH_BASE_DC;

  if (total < dc) {
    gain_skill(ch, "stealth", FALSE);
    WAIT_STATE(ch, PULSE_VIOLENCE / 2);
    GET_STAMINA(ch) -= 10;
    return;
  }

  /* Success: apply Sneak affect */
  new_affect(&af);
  af.spell    = SKILL_SNEAK;
  af.location = APPLY_NONE;
  af.modifier = 0;
  af.duration = sneak_effect_duration(ch);
  memset(af.bitvector, 0, sizeof(af.bitvector));
  SET_BIT_AR(af.bitvector, AFF_SNEAK);
  affect_to_char(ch, &af);

  /* Store a stealth check value for movement contests (reuse Hide’s field) */
  /* If you’ve already hidden with a higher roll, keep the stronger value. */
  SET_STEALTH_CHECK(ch, MAX(GET_STEALTH_CHECK(ch), total));

  gain_skill(ch, "stealth", TRUE);
  GET_STAMINA(ch) -= 10;
}

ACMD(do_hide)
{
  int total, dc;
  int stealth_skill = get_stealth_skill_value(ch);

  if (!stealth_skill) {
    send_to_char(ch, "You have no idea how to do that.\r\n");
    return;
  }

  send_to_char(ch, "You attempt to hide yourself.\r\n");

  /* If already hidden, drop it before re-attempting */
  if (AFF_FLAGGED(ch, AFF_HIDE)) {
    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_HIDE);
    GET_STEALTH_CHECK(ch) = 0;
  }

  if (FIGHTING(ch)){
    send_to_char(ch, "While fighting!?\r\n");
    return; 
  }    /* you can't hide while in active melee */

  /* --- 5e Stealth (DEX) ability check --- */
  /* Baseline difficulty: hiding in general */
  /* TODO: Maybe change dc based on terrain/populated rooms in the future */
  dc = STEALTH_BASE_DC;
  total = roll_stealth_check(ch);

  if (total < dc) {
    /* Failure */
    gain_skill(ch, "stealth", FALSE);
    WAIT_STATE(ch, PULSE_VIOLENCE / 2);
    GET_STAMINA(ch) -= 10;
    return;
  }

  /* Success: set flag and store this specific Stealth result */
  SET_BIT_AR(AFF_FLAGS(ch), AFF_HIDE);
  GET_STEALTH_CHECK(ch) = total;

  send_to_char(ch, "You hide yourself as best you can.\r\n");
  gain_skill(ch, "stealth", TRUE);
  WAIT_STATE(ch, PULSE_VIOLENCE / 2);
  GET_STAMINA(ch) -= 10;
}

static void remember_scan_target(struct char_data *ch, struct char_data *tch)
{
  struct scan_result_data *node;
  long uid;

  if (!ch || !tch)
    return;
  if (IS_NPC(ch))
    return;
  if (!ch->player_specials || ch->player_specials == &dummy_mob)
    return;

  uid = char_script_id(tch);
  for (node = GET_SCAN_RESULTS(ch); node; node = node->next) {
    if (node->target_uid == uid) {
      node->room = IN_ROOM(ch);
      return;
    }
  }

  CREATE(node, struct scan_result_data, 1);
  node->target_uid = uid;
  node->room = IN_ROOM(ch);
  node->next = GET_SCAN_RESULTS(ch);
  GET_SCAN_RESULTS(ch) = node;
}

static void forget_scan_target(struct char_data *ch, struct char_data *tch)
{
  struct scan_result_data **node;
  long uid;

  if (!ch || !tch)
    return;
  if (IS_NPC(ch))
    return;
  if (!ch->player_specials || ch->player_specials == &dummy_mob)
    return;

  uid = char_script_id(tch);
  for (node = &GET_SCAN_RESULTS(ch); *node; node = &((*node)->next)) {
    if ((*node)->target_uid == uid) {
      struct scan_result_data *old = *node;
      *node = old->next;
      free(old);
      return;
    }
  }
}

void stealth_process_room_movement(struct char_data *ch, room_rnum room, int dir, bool leaving)
{
  struct char_data *viewer;
  int stealth_total;
  bool base_failure;
  const char *dir_word;
  char msg[MAX_INPUT_LENGTH];
  char sdesc_buf[MAX_INPUT_LENGTH];
  const char *name_desc;
  const char *format;

  if (!ch || room == NOWHERE)
    return;

  stealth_total = roll_stealth_check(ch);
  base_failure = (stealth_total < STEALTH_BASE_DC);

  if (dir >= 0 && dir < NUM_OF_DIRS) {
    dir_word = leaving ? dirs[dir] : dirs[rev_dir[dir]];
  } else {
    dir_word = "somewhere";
  }

  if (get_char_sdesc(ch) && *get_char_sdesc(ch)) {
    strlcpy(sdesc_buf, get_char_sdesc(ch), sizeof(sdesc_buf));
    if (*sdesc_buf)
      sdesc_buf[0] = UPPER(sdesc_buf[0]);
    name_desc = sdesc_buf;
  } else {
    name_desc = "Someone";
  }

  format = leaving ?
    "%s tries to stealthily move to the %s." :
    "%s stealthily moves in from the %s.";
  snprintf(msg, sizeof(msg), format, name_desc, dir_word);

  for (viewer = world[room].people; viewer; viewer = viewer->next_in_room) {
    bool viewer_can_scan, saw_with_scan = FALSE, send_echo = FALSE;

    if (viewer == ch)
      continue;

    viewer_can_scan = can_scan_for_sneak(viewer);

    if (viewer_can_scan) {
      int perception_total = roll_scan_perception(viewer);

      if (perception_total >= stealth_total) {
        saw_with_scan = TRUE;
        send_echo = TRUE;
        remember_scan_target(viewer, ch);
      } else if (!base_failure) {
        forget_scan_target(viewer, ch);
      }

      gain_skill(viewer, "perception", saw_with_scan ? TRUE : FALSE);
    }

    if (!send_echo && base_failure) {
      if (!viewer_can_scan && !CAN_SEE(viewer, ch))
        continue;
      send_echo = TRUE;
      if (viewer_can_scan)
        remember_scan_target(viewer, ch);
    }

    if (send_echo)
      send_to_char(viewer, "%s\r\n", msg);
  }
}

void clear_scan_results(struct char_data *ch)
{
  struct scan_result_data *node, *next;

  if (!ch || IS_NPC(ch))
    return;
  if (!ch->player_specials || ch->player_specials == &dummy_mob)
    return;

  for (node = GET_SCAN_RESULTS(ch); node; node = next) {
    next = node->next;
    free(node);
  }

  GET_SCAN_RESULTS(ch) = NULL;
}

bool scan_can_target(struct char_data *ch, struct char_data *tch)
{
  struct scan_result_data *node;
  long uid;

  if (!ch || !tch)
    return FALSE;
  if (IS_NPC(ch))
    return FALSE;
  if (!ch->player_specials || ch->player_specials == &dummy_mob)
    return FALSE;
  if (!AFF_FLAGGED(ch, AFF_SCAN))
    return FALSE;

  uid = char_script_id(tch);

  for (node = GET_SCAN_RESULTS(ch); node; node = node->next)
    if (node->target_uid == uid && node->room == IN_ROOM(ch))
      return TRUE;

  return FALSE;
}

static int scan_target_dc(struct char_data *tch)
{
  if (GET_STEALTH_CHECK(tch) <= 0)
    SET_STEALTH_CHECK(tch, 5);

  /* Give hiders a modest buffer so high skill matters but success remains possible. */
  return GET_STEALTH_CHECK(tch) + 2;
}

bool scan_confirm_target(struct char_data *ch, struct char_data *tch)
{
  int total;

  if (!ch || !tch)
    return FALSE;
  if (!AFF_FLAGGED(ch, AFF_SCAN))
    return FALSE;
  if (!GET_SKILL(ch, SKILL_PERCEPTION))
    return FALSE;

  total = roll_skill_check(ch, SKILL_PERCEPTION, 0, NULL);

  if (FIGHTING(ch))
    total -= 4;

  if (total >= scan_target_dc(tch)) {
    remember_scan_target(ch, tch);
    return TRUE;
  }

  return FALSE;
}

static int scan_effect_duration(struct char_data *ch)
{
  int skill = GET_SKILL(ch, SKILL_PERCEPTION);
  int minutes;

  if (skill < 20)
    minutes = 15;
  else if (skill < 40)
    minutes = 20;
  else if (skill < 60)
    minutes = 25;
  else if (skill < 80)
    minutes = 30;
  else
    minutes = 45;

  /* Affect durations tick once per mud hour (75 seconds). */
  return MAX(1, (minutes * SECS_PER_REAL_MIN) / SECS_PER_MUD_HOUR);
}

bool perform_scan_sweep(struct char_data *ch)
{
  struct char_data *tch;
  int total;
  bool had_targets = FALSE;
  bool found_any   = FALSE;

  if (ch == NULL || IN_ROOM(ch) == NOWHERE)
    return FALSE;
  if (!AFF_FLAGGED(ch, AFF_SCAN))
    return FALSE;
  if (!GET_SKILL(ch, SKILL_PERCEPTION))
    return FALSE;
  if (AFF_FLAGGED(ch, AFF_BLIND))
    return FALSE;
  if (IS_DARK(IN_ROOM(ch)) && !CAN_SEE_IN_DARK(ch))
    return FALSE;

  for (tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room) {
    if (tch == ch)
      continue;
    if (!AFF_FLAGGED(tch, AFF_HIDE))
      continue;
    if (IS_NPC(tch))
      continue;
    had_targets = TRUE;
  }

  if (!had_targets)
    return FALSE;

  total = roll_skill_check(ch, SKILL_PERCEPTION, 0, NULL);

  if (FIGHTING(ch))
    total -= 4;

  for (tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room) {
    if (tch == ch)
      continue;
    if (!AFF_FLAGGED(tch, AFF_HIDE))
      continue;
    if (IS_NPC(tch))
      continue;

    if (total >= scan_target_dc(tch)) {
      char hidden_ldesc[MAX_STRING_LENGTH];
      if (build_hidden_ldesc(tch, hidden_ldesc, sizeof(hidden_ldesc)))
        send_to_char(ch, "%s", hidden_ldesc);
      else
        send_to_char(ch, "A shadowy figure.\r\n");
      remember_scan_target(ch, tch);
      found_any = TRUE;
    } else {
      forget_scan_target(ch, tch);
    }
  }

  gain_skill(ch, "perception", found_any ? TRUE : FALSE);
  return found_any;
}

/* Scan: apply a perception-based buff that auto-checks rooms while it lasts */
ACMD(do_scan)
{
  struct affected_type af;

  if (!GET_SKILL(ch, SKILL_PERCEPTION)) {
    send_to_char(ch, "You have no idea how to do that.\r\n");
    return;
  }

  if (AFF_FLAGGED(ch, AFF_SCAN)) {
    affect_from_char(ch, SKILL_PERCEPTION);
    affect_from_char(ch, SPELL_SCAN_AFFECT);
    send_to_char(ch, "You lower your guard and stop scanning the area.\r\n");
    act("$n relaxes, no longer scanning so intently.", TRUE, ch, 0, 0, TO_ROOM);
    return;
  }

  new_affect(&af);
  af.spell    = SPELL_SCAN_AFFECT;
  af.location = APPLY_NONE;
  af.modifier = 0;
  af.duration = scan_effect_duration(ch);
  memset(af.bitvector, 0, sizeof(af.bitvector));
  SET_BIT_AR(af.bitvector, AFF_SCAN);
  affect_to_char(ch, &af);

  send_to_char(ch, "You sharpen your senses and begin scanning for hidden threats.\r\n");
  act("$n studies $s surroundings with a wary gaze.", TRUE, ch, 0, 0, TO_ROOM);

  WAIT_STATE(ch, PULSE_VIOLENCE / 2);
  GET_STAMINA(ch) -= 10;
}

ACMD(do_listen)
{
  struct affected_type af;

  if (!GET_SKILL(ch, SKILL_PERCEPTION)) {
    send_to_char(ch, "You have no idea how to do that.\r\n");
    return;
  }

  if (AFF_FLAGGED(ch, AFF_LISTEN)) {
    affect_from_char(ch, SPELL_LISTEN_AFFECT);
    send_to_char(ch, "You stop actively listening for hushed voices.\r\n");
    return;
  }

  new_affect(&af);
  af.spell    = SPELL_LISTEN_AFFECT;
  af.location = APPLY_NONE;
  af.modifier = 0;
  af.duration = listen_effect_duration(ch);
  memset(af.bitvector, 0, sizeof(af.bitvector));
  SET_BIT_AR(af.bitvector, AFF_LISTEN);
  affect_to_char(ch, &af);

  send_to_char(ch, "You focus entirely on every whisper and distant sound.\r\n");

  WAIT_STATE(ch, PULSE_VIOLENCE / 2);
  GET_STAMINA(ch) -= 10;
}

ACMD(do_palm)
{
  struct obj_data *container, *item;
  char item_name[MAX_INPUT_LENGTH], cont_name[MAX_INPUT_LENGTH];
  char item_desc[MAX_STRING_LENGTH], cont_desc[MAX_STRING_LENGTH];
  int sleight_total;
  bool base_fail;

  if (!GET_SKILL(ch, SKILL_SLEIGHT_OF_HAND)) {
    send_to_char(ch, "You have no idea how to do that.\r\n");
    return;
  }

  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL)) {
    send_to_char(ch, "This room just has such a peaceful, easy feeling...\r\n");
    return;
  }

  two_arguments(argument, item_name, cont_name);

  if (!*item_name || !*cont_name) {
    send_to_char(ch, "Usage: palm <item> <container>\r\n");
    return;
  }

  if (!(container = find_container_on_character(ch, ch, cont_name))) {
    send_to_char(ch, "You aren't carrying or wearing anything like that.\r\n");
    return;
  }

  if (!obj_is_storage(container) && GET_OBJ_TYPE(container) != ITEM_FURNITURE) {
    send_to_char(ch, "That's not even a container.\r\n");
    return;
  }

  if (obj_storage_is_closed(container)) {
    send_to_char(ch, "You'd better open it first.\r\n");
    return;
  }

  if (!(item = get_obj_in_list_vis(ch, item_name, NULL, container->contains))) {
    send_to_char(ch, "You don't see that inside %s.\r\n", OBJS(container, ch));
    return;
  }

  if (!sleight_can_take_obj(ch, item))
    return;

  strlcpy(item_desc, OBJS(item, ch), sizeof(item_desc));
  strlcpy(cont_desc, OBJS(container, ch), sizeof(cont_desc));

  sleight_total = roll_sleight_check(ch);
  base_fail = (sleight_total < SLEIGHT_BASE_DC);

  sleight_check_observers(ch, sleight_total,
                          "palm", "from", item_desc, cont_desc);

  if (!get_otrigger(item, ch))
    return;

  int coin_amt = (GET_OBJ_TYPE(item) == ITEM_MONEY) ? GET_OBJ_VAL(item, 0) : 0;

  obj_from_obj(item);
  obj_to_char(item, ch);
  if (!sleight_merge_money_pile(ch, item))
    sleight_check_money(ch, item);
  if (coin_amt == 1)
    send_to_char(ch, "There was one coin.\r\n");
  else if (coin_amt > 1)
    send_to_char(ch, "There were %d coins.\r\n", coin_amt);

  if (base_fail)
    send_to_char(ch, "You get %s from your %s.\r\n", item_desc, cont_desc);
  else
    send_to_char(ch, "You quietly palm %s from your %s.\r\n", item_desc, cont_desc);

  gain_skill(ch, "sleight of hand", base_fail ? FALSE : TRUE);
}

ACMD(do_slip)
{
  struct obj_data *container, *obj;
  char obj_name[MAX_INPUT_LENGTH], cont_name[MAX_INPUT_LENGTH], arg3[MAX_INPUT_LENGTH];
  char item_desc[MAX_STRING_LENGTH], cont_desc[MAX_STRING_LENGTH];
  int sleight_total;
  bool base_fail;
  int howmany = 1;
  int amount_specified = 0;

  if (!GET_SKILL(ch, SKILL_SLEIGHT_OF_HAND)) {
    send_to_char(ch, "You have no idea how to do that.\r\n");
    return;
  }

  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL)) {
    send_to_char(ch, "This room just has such a peaceful, easy feeling...\r\n");
    return;
  }

  one_argument(two_arguments(argument, obj_name, cont_name), arg3);

  if (*arg3 && is_number(obj_name)) {
    howmany = atoi(obj_name);
    strlcpy(obj_name, cont_name, sizeof(obj_name));
    strlcpy(cont_name, arg3, sizeof(cont_name));
    amount_specified = 1;
  }

  if (!*obj_name || !*cont_name) {
    send_to_char(ch, "Usage: slip <item> <container>\r\n");
    return;
  }

  if (!(container = find_container_on_character(ch, ch, cont_name))) {
    send_to_char(ch, "You aren't carrying or wearing anything like that.\r\n");
    return;
  }

  if (!obj_is_storage(container) && GET_OBJ_TYPE(container) != ITEM_FURNITURE) {
    send_to_char(ch, "That's not even a container.\r\n");
    return;
  }

  if (obj_storage_is_closed(container)) {
    send_to_char(ch, "You'd better open it first.\r\n");
    return;
  }

  if (!(obj = get_obj_in_list_vis(ch, obj_name, NULL, ch->carrying))) {
    send_to_char(ch, "You aren't even carrying that.\r\n");
    return;
  }

  if (amount_specified && GET_OBJ_TYPE(obj) == ITEM_MONEY && howmany > 0) {
    int pile = GET_OBJ_VAL(obj, 0);

    if (howmany < pile) {
      struct obj_data *split = create_money(howmany);
      if (!split) {
        send_to_char(ch, "You fumble the coins.\r\n");
        return;
      }
      GET_OBJ_VAL(obj, 0) = pile - howmany;
      update_money_obj(obj);
      GET_COINS(ch) = MAX(0, GET_COINS(ch) - howmany);
      obj = split;
    }
  }

  if (OBJ_FLAGGED(obj, ITEM_NODROP)) {
    send_to_char(ch, "It refuses to leave your hands.\r\n");
    return;
  }

  if ((GET_OBJ_VAL(container, 0) > 0) &&
      (GET_OBJ_WEIGHT(container) + GET_OBJ_WEIGHT(obj) > GET_OBJ_VAL(container, 0))) {
    act("$p won't fit inside $P.", FALSE, ch, obj, container, TO_CHAR);
    return;
  }

  strlcpy(item_desc, OBJS(obj, ch), sizeof(item_desc));
  strlcpy(cont_desc, OBJS(container, ch), sizeof(cont_desc));

  sleight_total = roll_sleight_check(ch);
  base_fail = (sleight_total < SLEIGHT_BASE_DC);

  sleight_check_observers(ch, sleight_total,
                          "slip", "into", item_desc, cont_desc);

  if (!drop_otrigger(obj, ch))
    return;

  if (obj->carried_by)
    obj_from_char(obj);
  else if (obj->in_obj)
    obj_from_obj(obj);
  else if (IN_ROOM(obj) != NOWHERE)
    obj_from_room(obj);
  obj_to_obj(obj, container);

  if (base_fail)
    send_to_char(ch, "You put %s in your %s.\r\n", item_desc, cont_desc);
  else
    send_to_char(ch, "You quietly slip %s into your %s.\r\n", item_desc, cont_desc);

  gain_skill(ch, "sleight of hand", base_fail ? FALSE : TRUE);
}

ACMD(do_steal)
{
  struct char_data *vict;
  struct obj_data *obj;
  char vict_name[MAX_INPUT_LENGTH], obj_name[MAX_INPUT_LENGTH];
  int coins, eq_pos, ohoh = 0;
  int sleight_total, dc;

  if (!GET_SKILL(ch, SKILL_SLEIGHT_OF_HAND)) {
    send_to_char(ch, "You have no idea how to do that.\r\n");
    return;
  }
  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL)) {
    send_to_char(ch, "This room just has such a peaceful, easy feeling...\r\n");
    return;
  }

  two_arguments(argument, obj_name, vict_name);

  if (!(vict = get_char_vis(ch, vict_name, NULL, FIND_CHAR_ROOM))) {
    send_to_char(ch, "Steal what from who?\r\n");
    return;
  } else if (vict == ch) {
    send_to_char(ch, "Come on now, that's rather stupid!\r\n");
    return;
  }

  if (GET_LEVEL(vict) >= LVL_IMMORT) {
    send_to_char(ch, "You cannot steal from an immortal.\r\n");
    return;
  }

  sleight_total = roll_sleight_check(ch);

  if (str_cmp(obj_name, "coins") && str_cmp(obj_name, "coin")) {

    if (!(obj = get_obj_in_list_vis(ch, obj_name, NULL, vict->carrying))) {

      for (eq_pos = 0; eq_pos < NUM_WEARS; eq_pos++)
	if (GET_EQ(vict, eq_pos) &&
	    (isname(obj_name, GET_EQ(vict, eq_pos)->name)) &&
	    CAN_SEE_OBJ(ch, GET_EQ(vict, eq_pos))) {
	  obj = GET_EQ(vict, eq_pos);
	  break;
	}
      if (!obj) {
	act("$E hasn't got that item.", FALSE, ch, 0, vict, TO_CHAR);
	return;
      } else {			/* It is equipment */
	if ((GET_POS(vict) > POS_STUNNED)) {
	  send_to_char(ch, "Steal the equipment now?  Impossible!\r\n");
	  return;
	} else {
          if (!give_otrigger(obj, vict, ch) ||
              !receive_mtrigger(ch, vict, obj) ) {
            send_to_char(ch, "Impossible!\r\n");
            return;
          }
	  act("You unequip $p and steal it.", FALSE, ch, obj, 0, TO_CHAR);
	  act("$n steals $p from $N.", FALSE, ch, obj, vict, TO_NOTVICT);
	  obj_to_char(unequip_char(vict, eq_pos), ch);
	}
      }
    } else {			/* obj found in inventory */

      dc = compute_steal_dc(ch, vict, GET_OBJ_WEIGHT(obj));

      if (GET_LEVEL(ch) < LVL_IMMORT && sleight_total < dc) {
        ohoh = TRUE;
        send_to_char(ch, "Oops..\r\n");
        act("$n tried to steal something from you!", FALSE, ch, 0, vict, TO_VICT);
        act("$n tries to steal something from $N.", TRUE, ch, 0, vict, TO_NOTVICT);
        gain_skill(ch, "sleight of hand", FALSE);
      } else {			/* Steal the item */
          if (IS_CARRYING_N(ch) + 1 < CAN_CARRY_N(ch)) {
            if (!give_otrigger(obj, vict, ch) || !receive_mtrigger(ch, vict, obj) ) {
              send_to_char(ch, "Impossible!\r\n");
              return;
          }
	  if (IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(obj) < CAN_CARRY_W(ch)) {
	    obj_from_char(obj);
	    obj_to_char(obj, ch);
	    send_to_char(ch, "Got it!\r\n");
	  }
	} else
	  send_to_char(ch, "You cannot carry that much.\r\n");
      }
    }
  } else {			/* Steal some coins */
    dc = compute_steal_dc(ch, vict, 0);
    if (GET_LEVEL(ch) < LVL_IMMORT && AWAKE(vict) && (sleight_total < dc)) {
      ohoh = TRUE;
      send_to_char(ch, "Oops..\r\n");
      act("You discover that $n has $s hands in your wallet.", FALSE, ch, 0, vict, TO_VICT);
      act("$n tries to steal coins from $N.", TRUE, ch, 0, vict, TO_NOTVICT);
      gain_skill(ch, "sleight of hand", FALSE);
    } else {
      int total = count_coins_in_list(vict->carrying);
      int max_steal = total / 10;
      if (max_steal > 0) {
        coins = rand_number(1, max_steal);
        coins = remove_coins_from_list(vict->carrying, coins);
      } else {
        coins = 0;
      }

      if (coins > 0) {
        GET_COINS(vict) = MAX(0, GET_COINS(vict) - coins);
        add_coins_to_char(ch, coins);
        gain_skill(ch, "sleight of hand", TRUE);
        if (coins > 1)
          send_to_char(ch, "Bingo!  You got %d coins.\r\n", coins);
        else
          send_to_char(ch, "You manage to swipe a solitary coin.\r\n");
      } else {
        send_to_char(ch, "You couldn't get any coins...\r\n");
      }
    }
  }

  if (ohoh && IS_NPC(vict) && AWAKE(vict))
    hit(vict, ch, TYPE_UNDEFINED);
}

ACMD(do_skills)
{

  list_skills(ch);

}

ACMD(do_visible)
{
  if (GET_LEVEL(ch) >= LVL_IMMORT) {
    perform_immort_vis(ch);
    return;
  }

  if AFF_FLAGGED(ch, AFF_INVISIBLE) {
    appear(ch);
    send_to_char(ch, "You break the spell of invisibility.\r\n");
  } else
    send_to_char(ch, "You are already visible.\r\n");
}

static void print_group(struct char_data *ch)
{
  struct char_data * k;

  send_to_char(ch, "Your group consists of:\r\n");

  while ((k = (struct char_data *) simple_list(ch->group->members)) != NULL)
    send_to_char(ch, "%-*s: %s[%4d/%-4d]H [%4d/%-4d]M [%4d/%-4d]V%s\r\n",
	    count_color_chars(GET_NAME(k))+22, GET_NAME(k), 
	    GROUP_LEADER(GROUP(ch)) == k ? CBGRN(ch, C_NRM) : CCGRN(ch, C_NRM),
	    GET_HIT(k), GET_MAX_HIT(k),
	    GET_MANA(k), GET_MAX_MANA(k),
	    GET_STAMINA(k), GET_MAX_STAMINA(k),
	    CCNRM(ch, C_NRM));
}

static void display_group_list(struct char_data * ch)
{
  struct group_data * group;
  int count = 0;
	
  if (group_list->iSize) {
    send_to_char(ch, "#   Group Leader     # of Members    In Zone\r\n"
                     "---------------------------------------------------\r\n");
		
    while ((group = (struct group_data *) simple_list(group_list)) != NULL) {
			if (IS_SET(GROUP_FLAGS(group), GROUP_NPC))
			  continue;
      if (GROUP_LEADER(group) && !IS_SET(GROUP_FLAGS(group), GROUP_ANON))
        send_to_char(ch, "%-2d) %s%-12s     %-2d              %s%s\r\n", 
          ++count,
          IS_SET(GROUP_FLAGS(group), GROUP_OPEN) ? CCGRN(ch, C_NRM) : CCRED(ch, C_NRM), 
          GET_NAME(GROUP_LEADER(group)), group->members->iSize, zone_table[world[IN_ROOM(GROUP_LEADER(group))].zone].name,
          CCNRM(ch, C_NRM));
      else
        send_to_char(ch, "%-2d) Hidden\r\n", ++count);
				
		}
  }
  if (count)
    send_to_char(ch, "\r\n"
                     "%sSeeking Members%s\r\n"
                     "%sClosed%s\r\n", 
                     CCGRN(ch, C_NRM), CCNRM(ch, C_NRM),
                     CCRED(ch, C_NRM), CCNRM(ch, C_NRM));
  else
    send_to_char(ch, "\r\n"
                     "Currently no groups formed.\r\n");
}

/* Vatiken's Group System: Version 1.1 */
ACMD(do_group)
{
  char buf[MAX_STRING_LENGTH];
  struct char_data *vict;

  argument = one_argument(argument, buf);

  if (!*buf) {
    if (GROUP(ch))
      print_group(ch);
    else
      send_to_char(ch, "You must specify a group option, or type HELP GROUP for more info.\r\n");
    return;
  }
  
  if (is_abbrev(buf, "new")) {
    if (GROUP(ch))
      send_to_char(ch, "You are already in a group.\r\n");
    else
      create_group(ch);
  } else if (is_abbrev(buf, "list"))
    display_group_list(ch);
  else if (is_abbrev(buf, "join")) {
    skip_spaces(&argument);
    if (!(vict = get_char_vis(ch, argument, NULL, FIND_CHAR_ROOM))) {
      send_to_char(ch, "Join who?\r\n");
      return;
    } else if (vict == ch) {
      send_to_char(ch, "That would be one lonely grouping.\r\n");
      return;
    } else if (GROUP(ch)) {
      send_to_char(ch, "But you are already part of a group.\r\n");
      return;
    } else if (!GROUP(vict)) {
      act("$E$u is not part of a group!", FALSE, ch, 0, vict, TO_CHAR);
      return;
    } else if (!IS_SET(GROUP_FLAGS(GROUP(vict)), GROUP_OPEN)) {
      send_to_char(ch, "That group isn't accepting members.\r\n");
      return;
    }   
    join_group(ch, GROUP(vict)); 
  } else if (is_abbrev(buf, "kick")) {
    skip_spaces(&argument);
    if (!(vict = get_char_vis(ch, argument, NULL, FIND_CHAR_ROOM))) {
      send_to_char(ch, "Kick out who?\r\n");
      return;
    } else if (vict == ch) {
      send_to_char(ch, "There are easier ways to leave the group.\r\n");
      return;
    } else if (!GROUP(ch) ) {
      send_to_char(ch, "But you are not part of a group.\r\n");
      return;
    } else if (GROUP_LEADER(GROUP(ch)) != ch ) {
      send_to_char(ch, "Only the group's leader can kick members out.\r\n");
      return;
    } else if (GROUP(vict) != GROUP(ch)) {
      act("$E$u is not a member of your group!", FALSE, ch, 0, vict, TO_CHAR);
      return;
    } 
    send_to_char(ch, "You have kicked %s out of the group.\r\n", GET_NAME(vict));
    send_to_char(vict, "You have been kicked out of the group.\r\n"); 
    leave_group(vict);
  } else if (is_abbrev(buf, "regroup")) {
    if (!GROUP(ch)) {
      send_to_char(ch, "But you aren't part of a group!\r\n");
      return;
    }
    vict = GROUP_LEADER(GROUP(ch));
    if (ch == vict) {
      send_to_char(ch, "You are the group leader and cannot re-group.\r\n");
    } else {
      leave_group(ch);
      join_group(ch, GROUP(vict));
    }
  } else if (is_abbrev(buf, "leave")) {
    
    if (!GROUP(ch)) {
      send_to_char(ch, "But you aren't part of a group!\r\n");
      return;
    }
		
    leave_group(ch);
  } else if (is_abbrev(buf, "option")) {
    skip_spaces(&argument);
    if (!GROUP(ch)) {
      send_to_char(ch, "But you aren't part of a group!\r\n");
      return;
    } else if (GROUP_LEADER(GROUP(ch)) != ch) {
      send_to_char(ch, "Only the group leader can adjust the group flags.\r\n");
      return;
    }
    if (is_abbrev(argument, "open")) {
      TOGGLE_BIT(GROUP_FLAGS(GROUP(ch)), GROUP_OPEN);
      send_to_char(ch, "The group is now %s to new members.\r\n", IS_SET(GROUP_FLAGS(GROUP(ch)), GROUP_OPEN) ? "open" : "closed");
    } else if (is_abbrev(argument, "anonymous")) {
      TOGGLE_BIT(GROUP_FLAGS(GROUP(ch)), GROUP_ANON);
      send_to_char(ch, "The group location is now %s to other players.\r\n", IS_SET(GROUP_FLAGS(GROUP(ch)), GROUP_ANON) ? "invisible" : "visible");
    } else 
      send_to_char(ch, "The flag options are: Open, Anonymous\r\n");
  } else {
    send_to_char(ch, "You must specify a group option, or type HELP GROUP for more info.\r\n");		
  }

}

ACMD(do_report)
{
  struct group_data *group;

  if ((group = GROUP(ch)) == NULL) {
    send_to_char(ch, "But you are not a member of any group!\r\n");
    return;
  }

  send_to_group(NULL, group, "%s reports: %d/%dH, %d/%dM, %d/%dV\r\n",
	  GET_NAME(ch),
	  GET_HIT(ch), GET_MAX_HIT(ch),
	  GET_MANA(ch), GET_MAX_MANA(ch),
	  GET_STAMINA(ch), GET_MAX_STAMINA(ch));
}

ACMD(do_split)
{
  char buf[MAX_INPUT_LENGTH];
  int amount, num = 0, share, rest;
  size_t len;
  struct char_data *k;
  
  if (IS_NPC(ch))
    return;

  one_argument(argument, buf);

  if (is_number(buf)) {
    amount = atoi(buf);
    if (amount <= 0) {
      send_to_char(ch, "Sorry, you can't do that.\r\n");
      return;
    }
    if (amount > GET_COINS(ch)) {
      send_to_char(ch, "You don't seem to have that many coins to split.\r\n");
      return;
    }
    
    if (GROUP(ch))
      while ((k = (struct char_data *) simple_list(GROUP(ch)->members)) != NULL)
        if (IN_ROOM(ch) == IN_ROOM(k) && !IS_NPC(k))
          num++;

    if (num && GROUP(ch)) {
      share = amount / num;
      rest = amount % num;
    } else {
      send_to_char(ch, "With whom do you wish to share your coins?\r\n");
      return;
    }

    decrease_coins(ch, share * (num - 1));

    /* Abusing signed/unsigned to make sizeof work. */
    len = snprintf(buf, sizeof(buf), "%s splits %d coins; you receive %d.\r\n",
		GET_NAME(ch), amount, share);
    if (rest && len < sizeof(buf)) {
      snprintf(buf + len, sizeof(buf) - len,
		"%d coin%s %s not splitable, so %s keeps the coins.\r\n", rest,
		(rest == 1) ? "" : "s", (rest == 1) ? "was" : "were", GET_NAME(ch));
    }

    while ((k = (struct char_data *) simple_list(GROUP(ch)->members)) != NULL)
      if (k != ch && IN_ROOM(ch) == IN_ROOM(k) && !IS_NPC(k)) {
	      increase_coins(k, share);
	      send_to_char(k, "%s", buf);
			}

    send_to_char(ch, "You split %d coins among %d members -- %d coins each.\r\n",
	    amount, num, share);

    if (rest) {
      send_to_char(ch, "%d coin%s %s not splitable, so you keep the coins.\r\n",
		rest, (rest == 1) ? "" : "s", (rest == 1) ? "was" : "were");
    }
  } else {
    send_to_char(ch, "How many coins do you wish to split with your group?\r\n");
    return;
  }
}

ACMD(do_use)
{
  char buf[MAX_INPUT_LENGTH], arg[MAX_INPUT_LENGTH];
  struct obj_data *mag_item;

  half_chop(argument, arg, buf);
  if (!*arg) {
    send_to_char(ch, "What do you want to %s?\r\n", CMD_NAME);
    return;
  }
  mag_item = GET_EQ(ch, WEAR_HOLD);

  if (!mag_item || !isname(arg, mag_item->name)) {
    switch (subcmd) {
    case SCMD_RECITE:
    case SCMD_QUAFF:
      if (!(mag_item = get_obj_in_list_vis(ch, arg, NULL, ch->carrying))) {
	send_to_char(ch, "You don't seem to have %s %s.\r\n", AN(arg), arg);
	return;
      }
      break;
    case SCMD_USE:
      send_to_char(ch, "You don't seem to be holding %s %s.\r\n", AN(arg), arg);
      return;
    default:
      log("SYSERR: Unknown subcmd %d passed to do_use.", subcmd);
      /* SYSERR_DESC: This is the same as the unhandled case in do_gen_ps(),
       * but in the function which handles 'quaff', 'recite', and 'use'. */
      return;
    }
  }
  switch (subcmd) {
  case SCMD_QUAFF:
    if (GET_OBJ_TYPE(mag_item) != ITEM_POTION) {
      send_to_char(ch, "You can only quaff potions.\r\n");
      return;
    }
    break;
  case SCMD_RECITE:
    if (GET_OBJ_TYPE(mag_item) != ITEM_SCROLL) {
      send_to_char(ch, "You can only recite scrolls.\r\n");
      return;
    }
    break;
  case SCMD_USE:
    if ((GET_OBJ_TYPE(mag_item) != ITEM_WAND) &&
	(GET_OBJ_TYPE(mag_item) != ITEM_STAFF)) {
      send_to_char(ch, "You can't seem to figure out how to use it.\r\n");
      return;
    }
    break;
  }

  mag_objectmagic(ch, mag_item, buf);
}

ACMD(do_display)
{
  size_t i;

  if (IS_NPC(ch)) {
    send_to_char(ch, "Monsters don't need displays.  Go away.\r\n");
    return;
  }
  skip_spaces(&argument);

  if (!*argument) {
    send_to_char(ch, "Usage: prompt { { H | M | S } | all | auto | none }\r\n");
    return;
  }

  if (!str_cmp(argument, "auto")) {
    TOGGLE_BIT_AR(PRF_FLAGS(ch), PRF_DISPAUTO);
    send_to_char(ch, "Auto prompt %sabled.\r\n", PRF_FLAGGED(ch, PRF_DISPAUTO) ? "en" : "dis");
    return;
  }

  if (!str_cmp(argument, "on") || !str_cmp(argument, "all")) {
    SET_BIT_AR(PRF_FLAGS(ch), PRF_DISPHP);
    SET_BIT_AR(PRF_FLAGS(ch), PRF_DISPMANA);
    SET_BIT_AR(PRF_FLAGS(ch), PRF_DISPSTAMINA);
  } else if (!str_cmp(argument, "off") || !str_cmp(argument, "none")) {
    REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_DISPHP);
    REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_DISPMANA);
    REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_DISPSTAMINA);
  } else {
    REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_DISPHP);
    REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_DISPMANA);
    REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_DISPSTAMINA);

    for (i = 0; i < strlen(argument); i++) {
      switch (LOWER(argument[i])) {
      case 'h':
        SET_BIT_AR(PRF_FLAGS(ch), PRF_DISPHP);
	break;
      case 'm':
        SET_BIT_AR(PRF_FLAGS(ch), PRF_DISPMANA);
	break;
      case 's':
      case 'v':
        SET_BIT_AR(PRF_FLAGS(ch), PRF_DISPSTAMINA);
	break;
      default:
	send_to_char(ch, "Usage: prompt { { H | M | S } | all | auto | none }\r\n");
	return;
      }
    }
  }

  send_to_char(ch, "%s", CONFIG_OK);
}

#define TOG_OFF 0
#define TOG_ON  1
ACMD(do_gen_tog)
{
  long result;
  int i;
  char arg[MAX_INPUT_LENGTH];

  const char *tog_messages[][2] = {
    {"You are now safe from summoning by other players.\r\n",
    "You may now be summoned by other players.\r\n"},
    {"Nohassle disabled.\r\n",
    "Nohassle enabled.\r\n"},
    {"Brief mode off.\r\n",
    "Brief mode on.\r\n"},
    {"Compact mode off.\r\n",
    "Compact mode on.\r\n"},
    {"You can now hear shouts.\r\n",
    "You are now deaf to shouts.\r\n"},
    {"You can now hear the Wiz-channel.\r\n",
    "You are now deaf to the Wiz-channel.\r\n"},
    {"You are no longer part of the Quest.\r\n",
    "Okay, you are part of the Quest!\r\n"},
    {"You will no longer see the room flags.\r\n",
    "You will now see the room flags.\r\n"},
    {"You will now have your communication repeated.\r\n",
    "You will no longer have your communication repeated.\r\n"},
    {"HolyLight mode off.\r\n",
    "HolyLight mode on.\r\n"},
    {"Nameserver_is_slow changed to NO; IP addresses will now be resolved.\r\n",
    "Nameserver_is_slow changed to YES; sitenames will no longer be resolved.\r\n"},
    {"Autoexits disabled.\r\n",
    "Autoexits enabled.\r\n"},
    {"Will no longer track through doors.\r\n",
    "Will now track through doors.\r\n"},
    {"Will no longer clear screen in OLC.\r\n",
    "Will now clear screen in OLC.\r\n"},
    {"Buildwalk Off.\r\n",
    "Buildwalk On.\r\n"},
    {"AFK flag is now off.\r\n",
    "AFK flag is now on.\r\n"},
    {"Autoloot disabled.\r\n",
    "Autoloot enabled.\r\n"},
    {"Autosplit disabled.\r\n",
    "Autosplit enabled.\r\n"},
    {"Autoassist disabled.\r\n",
    "Autoassist enabled.\r\n"},
    {"Automap disabled.\r\n",
    "Automap enabled.\r\n"},
    {"Autokey disabled.\r\n",
    "Autokey enabled.\r\n"},
    {"Autodoor disabled.\r\n",
    "Autodoor enabled.\r\n"},
    {"ZoneResets disabled.\r\n",
    "ZoneResets enabled.\r\n"}
  };

  if (IS_NPC(ch))
    return;

  switch (subcmd) {
  case SCMD_NOSUMMON:
    result = PRF_TOG_CHK(ch, PRF_SUMMONABLE);
    break;
  case SCMD_NOHASSLE:
    result = PRF_TOG_CHK(ch, PRF_NOHASSLE);
    break;
  case SCMD_BRIEF:
    result = PRF_TOG_CHK(ch, PRF_BRIEF);
    break;
  case SCMD_COMPACT:
    result = PRF_TOG_CHK(ch, PRF_COMPACT);
    break;
  case SCMD_NOSHOUT:
    result = PRF_TOG_CHK(ch, PRF_NOSHOUT);
    break;
  case SCMD_NOWIZ:
    result = PRF_TOG_CHK(ch, PRF_NOWIZ);
    break;
  case SCMD_QUEST:
    result = PRF_TOG_CHK(ch, PRF_QUEST);
    break;
  case SCMD_SHOWVNUMS:
    result = PRF_TOG_CHK(ch, PRF_SHOWVNUMS);
    break;
  case SCMD_NOREPEAT:
    result = PRF_TOG_CHK(ch, PRF_NOREPEAT);
    break;
  case SCMD_HOLYLIGHT:
    result = PRF_TOG_CHK(ch, PRF_HOLYLIGHT);
    break;
  case SCMD_AUTOEXIT:
    result = PRF_TOG_CHK(ch, PRF_AUTOEXIT);
    break;
  case SCMD_CLS:
    result = PRF_TOG_CHK(ch, PRF_CLS);
    break;    
  case SCMD_BUILDWALK:
    if (GET_LEVEL(ch) < LVL_BUILDER) {
      send_to_char(ch, "Builders only, sorry.\r\n");
      return;
    }
    result = PRF_TOG_CHK(ch, PRF_BUILDWALK);
    if (PRF_FLAGGED(ch, PRF_BUILDWALK)) {
      one_argument(argument, arg);
      for (i=0; *arg && *(sector_types[i]) != '\n'; i++)
        if (is_abbrev(arg, sector_types[i]))
          break;
      if (*(sector_types[i]) == '\n') 
        i=0;
      GET_BUILDWALK_SECTOR(ch) = i;
      send_to_char(ch, "Default sector type is %s\r\n", sector_types[i]);
  
      mudlog(CMP, GET_LEVEL(ch), TRUE,
             "OLC: %s turned buildwalk on. Allowed zone %d", GET_NAME(ch), GET_OLC_ZONE(ch));
    } else
      mudlog(CMP, GET_LEVEL(ch), TRUE,
             "OLC: %s turned buildwalk off. Allowed zone %d", GET_NAME(ch), GET_OLC_ZONE(ch));
    break;
  case SCMD_AFK:
    result = PRF_TOG_CHK(ch, PRF_AFK);
    if (PRF_FLAGGED(ch, PRF_AFK))
      act("$n has gone AFK.", TRUE, ch, 0, 0, TO_ROOM);
    else {
      act("$n has come back from AFK.", TRUE, ch, 0, 0, TO_ROOM);
      if (has_mail(GET_IDNUM(ch)))
        send_to_char(ch, "You have mail waiting.\r\n");
    }
    break;
  case SCMD_AUTOLOOT:
    result = PRF_TOG_CHK(ch, PRF_AUTOLOOT);
    break;
  case SCMD_AUTOSPLIT:
    result = PRF_TOG_CHK(ch, PRF_AUTOSPLIT);
    break;
  case SCMD_AUTOASSIST:
    result = PRF_TOG_CHK(ch, PRF_AUTOASSIST);
    break;
  case SCMD_AUTOMAP:
    result = PRF_TOG_CHK(ch, PRF_AUTOMAP);
    break;
  case SCMD_AUTOKEY:
    result = PRF_TOG_CHK(ch, PRF_AUTOKEY);
    break;
  case SCMD_AUTODOOR:
    result = PRF_TOG_CHK(ch, PRF_AUTODOOR);
    break;
  case SCMD_ZONERESETS:
    result = PRF_TOG_CHK(ch, PRF_ZONERESETS);
    break;
  default:
    log("SYSERR: Unknown subcmd %d in do_gen_toggle.", subcmd);
    return;
  }

  if (result)
    send_to_char(ch, "%s", tog_messages[subcmd][TOG_ON]);
  else
    send_to_char(ch, "%s", tog_messages[subcmd][TOG_OFF]);

  return;
}
