/**************************************************************************
*  File: limits.c                                          Part of tbaMUD *
*  Usage: Limits & gain funcs for HMV, exp, hunger/thirst, idle time.     *
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
#include "constants.h"
#include "spells.h"
#include "comm.h"
#include "db.h"
#include "handler.h"
#include "interpreter.h"
#include "py_triggers.h"
#include "class.h"
#include "fight.h"
#include "screen.h"
#include "mud_event.h"
#include "set.h"
#include <time.h>

/* local file scope function prototypes */
static int graf(int grafage, int p0, int p1, int p2, int p3, int p4, int p5, int p6);
static void check_idling(struct char_data *ch);


/* When age < 15 return the value p0
   When age is 15..29 calculate the line between p1 & p2
   When age is 30..44 calculate the line between p2 & p3
   When age is 45..59 calculate the line between p3 & p4
   When age is 60..79 calculate the line between p4 & p5
   When age >= 80 return the value p6 */
static int graf(int grafage, int p0, int p1, int p2, int p3, int p4, int p5, int p6)
{

  if (grafage < 15)
    return (p0);					/* < 15   */
  else if (grafage <= 29)
    return (p1 + (((grafage - 15) * (p2 - p1)) / 15));	/* 15..29 */
  else if (grafage <= 44)
    return (p2 + (((grafage - 30) * (p3 - p2)) / 15));	/* 30..44 */
  else if (grafage <= 59)
    return (p3 + (((grafage - 45) * (p4 - p3)) / 15));	/* 45..59 */
  else if (grafage <= 79)
    return (p4 + (((grafage - 60) * (p5 - p4)) / 20));	/* 60..79 */
  else
    return (p6);					/* >= 80 */
}

/* The hit_limit, mana_limit, and move_limit functions are gone.  They added an
 * unnecessary level of complexity to the internal structure, weren't
 * particularly useful, and led to some annoying bugs.  From the players' point
 * of view, the only difference the removal of these functions will make is
 * that a character's age will now only affect the HMV gain per tick, and _not_
 * the HMV maximums. */
/* manapoint gain pr. game hour */
int mana_gain(struct char_data *ch)
{
  int gain;

  if (IS_NPC(ch)) {
    /* Neat and fast */
    gain = GET_LEVEL(ch);
  } else {
    gain = graf(age(ch)->year, 4, 8, 12, 16, 12, 10, 8);

    /* Class calculations */

    /* Skill/Spell calculations */

    /* Position calculations    */
    switch (GET_POS(ch)) {
    case POS_SLEEPING:
      gain *= 2;
      break;
    case POS_RESTING:
      gain += (gain / 2);	/* Divide by 2 */
      break;
    case POS_SITTING:
      gain += (gain / 4);	/* Divide by 4 */
      break;
    }

    if (IS_SORCEROR(ch) || IS_CLERIC(ch))
      gain *= 2;

    if ((GET_COND(ch, HUNGER) == 0) || (GET_COND(ch, THIRST) == 0))
      gain /= 4;
  }

  if (AFF_FLAGGED(ch, AFF_POISON))
    gain /= 4;

  return (gain);
}

/* Hitpoint gain pr. game hour */
int hit_gain(struct char_data *ch)
{
  int gain;

  if (IS_NPC(ch)) {
    /* Neat and fast */
    gain = GET_LEVEL(ch);
  } else {

    gain = graf(age(ch)->year, 8, 12, 20, 32, 16, 10, 4);

    /* Class/Level calculations */
    /* Skill/Spell calculations */
    /* Position calculations    */

    switch (GET_POS(ch)) {
    case POS_SLEEPING:
      gain += (gain / 2);	/* Divide by 2 */
      break;
    case POS_RESTING:
      gain += (gain / 4);	/* Divide by 4 */
      break;
    case POS_SITTING:
      gain += (gain / 8);	/* Divide by 8 */
      break;
    }

    if (IS_SORCEROR(ch) || IS_CLERIC(ch))
      gain /= 2;	/* Ouch. */

    if ((GET_COND(ch, HUNGER) == 0) || (GET_COND(ch, THIRST) == 0))
      gain /= 4;
  }

  if (AFF_FLAGGED(ch, AFF_POISON))
    gain /= 4;

  return (gain);
}

/* stamina gain pr. game hour */
int move_gain(struct char_data *ch)
{
  int gain;

  if (IS_NPC(ch)) {
    /* Neat and fast */
    gain = GET_LEVEL(ch);
  } else {
    gain = graf(age(ch)->year, 16, 20, 24, 20, 16, 12, 10);

    /* Class/Level calculations */
    /* Skill/Spell calculations */
    /* Position calculations    */
    switch (GET_POS(ch)) {
    case POS_SLEEPING:
      gain += (gain / 2);	/* Divide by 2 */
      break;
    case POS_RESTING:
      gain += (gain / 4);	/* Divide by 4 */
      break;
    case POS_SITTING:
      gain += (gain / 8);	/* Divide by 8 */
      break;
    }

    if ((GET_COND(ch, HUNGER) == 0) || (GET_COND(ch, THIRST) == 0))
      gain /= 4;
  }

  if (AFF_FLAGGED(ch, AFF_POISON))
    gain /= 4;

  return (gain);
}

void run_autowiz(void)
{
#if defined(CIRCLE_UNIX) || defined(CIRCLE_WINDOWS)
  if (CONFIG_USE_AUTOWIZ) {
    size_t res;
    char buf[256];

#if defined(CIRCLE_UNIX)
    res = snprintf(buf, sizeof(buf), "nice ../bin/autowiz %d %s %d %s %d &",
	CONFIG_MIN_WIZLIST_LEV, WIZLIST_FILE, LVL_IMMORT, IMMLIST_FILE, (int) getpid());
#elif defined(CIRCLE_WINDOWS)
    res = snprintf(buf, sizeof(buf), "autowiz %d %s %d %s",
	CONFIG_MIN_WIZLIST_LEV, WIZLIST_FILE, LVL_IMMORT, IMMLIST_FILE);
#endif /* CIRCLE_WINDOWS */

    /* Abusing signed -> unsigned conversion to avoid '-1' check. */
    if (res < sizeof(buf)) {
      mudlog(CMP, LVL_IMMORT, FALSE, "Initiating autowiz.");
      reboot_wizlists();
      int rval = system(buf);
      if(rval != 0)
        mudlog(BRF, LVL_IMMORT, TRUE, "Warning: autowiz failed with return value %d", rval);
    } else
      log("Cannot run autowiz: command-line doesn't fit in buffer.");
  }
#endif /* CIRCLE_UNIX || CIRCLE_WINDOWS */
}

/* Requires: find_skill_num(), GET_WIS(), wis_app[], GET_SKILL(), SET_SKILL(), rand_number()
 * Cooldown: 1 hour - 5 * WIS bonus minutes (floored at 0)
 * Rolls: failure -> 1..20, success -> 1..100
 * Cap: per-class max (see class.c)
 */
void gain_skill(struct char_data *ch, char *skill, bool success)
{
  int skill_num, base, roll, increase;
  int wisb, cd_seconds;
  int cap;
  time_t now;

  if (IS_NPC(ch))
    return;

  /* Resolve index and validate against table size */
  skill_num = find_skill_num(skill);
  if (skill_num <= 0 || skill_num > MAX_SKILLS)
    return;

  /* Respect per-skill cooldown: do nothing if still cooling down */
  now = time(0);
  if (GET_SKILL_NEXT_GAIN(ch, skill_num) != 0 &&
      now < GET_SKILL_NEXT_GAIN(ch, skill_num)) {
    return;
  }

  base = GET_SKILL(ch, skill_num);
  cap = class_skill_max(GET_CLASS(ch), skill_num);
  if (cap <= 0)
    return;
  /* If already capped, bail early (and don’t start cooldown) */
  if (base >= cap)
    return;

  /* Wisdom bonus from wis_app[] (constants.c). Higher = better learning & shorter cooldown. */
  wisb = GET_ABILITY_MOD(GET_WIS(ch));

  if (success) {
    /* Learning from success is harder: 1..100, threshold scales with WIS */
    roll = rand_number(1, 100);
    /* Old 1..400 with (400 - wisb*4) ⇒ scaled: (100 - wisb) */
    if (roll >= (100 - wisb)) {
      increase = base + 1;
      SET_SKILL(ch, skill_num, MIN(cap, increase));

      /* Cooldown only when an increase actually happens */
      cd_seconds = 3600 - (wisb * 5 * 60);  /* 1 hour - 5 * WIS minutes */
      if (cd_seconds < 0) cd_seconds = 0;
      GET_SKILL_NEXT_GAIN(ch, skill_num) = now + cd_seconds;
    }
  } else {
    /* Learning from failure is easier: 1..20, threshold scales with WIS */
    roll = rand_number(1, 20);
    /* Old 1..100 with (100 - wisb) ⇒ scaled: (20 - wisb) */
    if (roll >= (20 - wisb)) {
      increase = base + 1;
      SET_SKILL(ch, skill_num, MIN(cap, increase));

      /* Cooldown only when an increase actually happens */
      cd_seconds = 3600 - (wisb * 5 * 60);  /* 1 hour - 5 * WIS minutes */
      if (cd_seconds < 0) cd_seconds = 0;
      GET_SKILL_NEXT_GAIN(ch, skill_num) = now + cd_seconds;
    }
  }
}

void gain_exp(struct char_data *ch, int gain)
{
  if (!IS_NPC(ch) && ((GET_LEVEL(ch) < 1 || GET_LEVEL(ch) >= LVL_IMMORT)))
    return;

  if (IS_NPC(ch)) {
    GET_EXP(ch) += gain;
    return;
  }

  if (gain > 0) {
    gain = MIN(CONFIG_MAX_EXP_GAIN, gain);	/* cap max gain per kill */
    GET_EXP(ch) += gain;
  } else if (gain < 0) {
    gain = MAX(-CONFIG_MAX_EXP_LOSS, gain);	/* cap max loss per death */
    GET_EXP(ch) += gain;
    if (GET_EXP(ch) < 0)
      GET_EXP(ch) = 0;
  }

  if (GET_LEVEL(ch) >= LVL_IMMORT && !PLR_FLAGGED(ch, PLR_NOWIZLIST))
    run_autowiz();
}

void gain_exp_regardless(struct char_data *ch, int gain)
{
  int is_altered = FALSE;
  int num_levels = 0;

  GET_EXP(ch) += gain;
  if (GET_EXP(ch) < 0)
    GET_EXP(ch) = 0;

  if (!IS_NPC(ch)) {
    while (GET_LEVEL(ch) < LVL_IMPL &&
	GET_EXP(ch) >= level_exp(GET_CLASS(ch), GET_LEVEL(ch) + 1)) {
      GET_LEVEL(ch) += 1;
      num_levels++;
      advance_level(ch);
      is_altered = TRUE;
    }

    if (is_altered) {
      mudlog(BRF, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE, "%s advanced %d level%s to level %d.",
		GET_NAME(ch), num_levels, num_levels == 1 ? "" : "s", GET_LEVEL(ch));
      if (num_levels == 1)
        send_to_char(ch, "You rise a level!\r\n");
      else
	send_to_char(ch, "You rise %d levels!\r\n", num_levels);
    }
  }
  if (GET_LEVEL(ch) >= LVL_IMMORT && !PLR_FLAGGED(ch, PLR_NOWIZLIST))
    run_autowiz();
}

void gain_condition(struct char_data *ch, int condition, int value)
{
  bool intoxicated;

  if (IS_NPC(ch) || GET_COND(ch, condition) == -1)	/* No change */
    return;

  intoxicated = (GET_COND(ch, DRUNK) > 0);

  GET_COND(ch, condition) += value;

  GET_COND(ch, condition) = MAX(0, GET_COND(ch, condition));
  GET_COND(ch, condition) = MIN(24, GET_COND(ch, condition));

  if (GET_COND(ch, condition) || PLR_FLAGGED(ch, PLR_WRITING))
    return;

  switch (condition) {
  case HUNGER:
    send_to_char(ch, "You are hungry.\r\n");
    break;
  case THIRST:
    send_to_char(ch, "You are thirsty.\r\n");
    break;
  case DRUNK:
    if (intoxicated)
      send_to_char(ch, "You are now sober.\r\n");
    break;
  default:
    break;
  }

}

static void check_idling(struct char_data *ch)
{
  if (ch->char_specials.timer > CONFIG_IDLE_VOID) {
    if (GET_WAS_IN(ch) == NOWHERE && IN_ROOM(ch) != NOWHERE) {
      GET_WAS_IN(ch) = IN_ROOM(ch);
      if (FIGHTING(ch)) {
	stop_fighting(FIGHTING(ch));
	stop_fighting(ch);
      }
      act("$n disappears into the void.", TRUE, ch, 0, 0, TO_ROOM);
      send_to_char(ch, "You have been idle, and are pulled into a void.\r\n");
      save_char(ch);
      Crash_crashsave(ch);
      char_from_room(ch);
      char_to_room(ch, 1);
    } else if (ch->char_specials.timer > (CONFIG_IDLE_VOID * 2)) {
      if (IN_ROOM(ch) != NOWHERE)
	char_from_room(ch);
      char_to_room(ch, 3);
      if (ch->desc) {
	STATE(ch->desc) = CON_DISCONNECT;
	/*
	 * For the 'if (d->character)' test in close_socket().
	 * -gg 3/1/98 (Happy anniversary.)
	 */
	ch->desc->character = NULL;
	ch->desc = NULL;
      }
      Crash_idlesave(ch);
      mudlog(CMP, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE, "%s idle-saved and extracted (idle).", GET_NAME(ch));
      add_llog_entry(ch, LAST_IDLEOUT);
      extract_char(ch);
    }
  }
}

/* Update PCs, NPCs, and objects */
void point_update(void)
{
  struct char_data *i, *next_char;
  struct obj_data *j, *next_thing, *jj, *next_thing2;
  /* Room-save autosave pulse counter (static so it persists across calls) */
  static int roomsave_pulse = 0;

  /* characters */
  for (i = character_list; i; i = next_char) {
    next_char = i->next;

    gain_condition(i, HUNGER, -1);
    gain_condition(i, DRUNK, -1);
    gain_condition(i, THIRST, -1);

    if (GET_POS(i) >= POS_STUNNED) {
      GET_HIT(i) = MIN(GET_HIT(i) + hit_gain(i), GET_MAX_HIT(i));
      GET_MANA(i) = MIN(GET_MANA(i) + mana_gain(i), GET_MAX_MANA(i));
      GET_STAMINA(i) = MIN(GET_STAMINA(i) + move_gain(i), GET_MAX_STAMINA(i));
      if (AFF_FLAGGED(i, AFF_POISON))
        if (damage(i, i, 2, SPELL_POISON) == -1)
          continue; /* Oops, they died. -gg 6/24/98 */
      if (GET_POS(i) <= POS_STUNNED)
        update_pos(i);
    } else if (GET_POS(i) == POS_INCAP) {
      if (damage(i, i, 1, TYPE_SUFFERING) == -1)
        continue;
    } else if (GET_POS(i) == POS_MORTALLYW) {
      if (damage(i, i, 2, TYPE_SUFFERING) == -1)
        continue;
    }
    if (!IS_NPC(i)) {
      update_char_objects(i);
      (i->char_specials.timer)++;
      if (GET_LEVEL(i) < CONFIG_IDLE_MAX_LEVEL)
        check_idling(i);
    }
  }

  /* objects */
  for (j = object_list; j; j = next_thing) {
    next_thing = j->next; /* Next in object list */

    /* If this is a corpse */
    if (IS_CORPSE(j)) {
      /* timer count down */
      if (GET_OBJ_TIMER(j) > 0)
        GET_OBJ_TIMER(j)--;

      if (!GET_OBJ_TIMER(j)) {

        if (j->carried_by)
          act("$p decays in your hands.", FALSE, j->carried_by, j, 0, TO_CHAR);
        else if ((IN_ROOM(j) != NOWHERE) && (world[IN_ROOM(j)].people)) {
          act("A quivering horde of maggots consumes $p.",
              TRUE, world[IN_ROOM(j)].people, j, 0, TO_ROOM);
          act("A quivering horde of maggots consumes $p.",
              TRUE, world[IN_ROOM(j)].people, j, 0, TO_CHAR);
        }
        for (jj = j->contains; jj; jj = next_thing2) {
          next_thing2 = jj->next_content; /* Next in inventory */
          obj_from_obj(jj);

          if (j->in_obj)
            obj_to_obj(jj, j->in_obj);
          else if (j->carried_by)
            obj_to_room(jj, IN_ROOM(j->carried_by));
          else if (IN_ROOM(j) != NOWHERE)
            obj_to_room(jj, IN_ROOM(j));
          else
            core_dump();
        }
        extract_obj(j);
      }
    }
    /* If the timer is set, count it down and at 0, try the trigger
     * note to .rej hand-patchers: make this last in your point-update() */
    else if (GET_OBJ_TIMER(j)>0) {
      GET_OBJ_TIMER(j)--;
      if (!GET_OBJ_TIMER(j))
        timer_otrigger(j);
    }
  }

  /* ---- Room SAVE autosave (every 10 minutes; adjust the 600 as desired) ----
   * Requires: #include "set.h" at the top of this file.
   * Saves all rooms flagged ROOM_SAVE via set.c.
   */
  if (++roomsave_pulse >= (PASSES_PER_SEC * 600)) {
    roomsave_pulse = 0;
    RoomSave_autosave_tick();
  }
}

/* Note: amt may be negative */
int increase_coins(struct char_data *ch, int amt)
{
  int curr_coins;
  int add;

  if (!ch)
    return 0;

  curr_coins = GET_COINS(ch);

  if (amt < 0)
    return decrease_coins(ch, -amt);
  if (amt == 0)
    return curr_coins;

  add = MIN(amt, MAX_COINS - curr_coins);
  if (add <= 0)
    return curr_coins;

  add_coins_to_char(ch, add);

  if (GET_COINS(ch) == MAX_COINS)
    send_to_char(ch, "%sYou have reached the maximum coins!\r\n%sYou must spend them or bank them before you can gain any more.\r\n", QBRED, QNRM);

  return GET_COINS(ch);
}

int decrease_coins(struct char_data *ch, int deduction)
{
  if (!ch || deduction <= 0)
    return GET_COINS(ch);

  remove_coins_from_char(ch, deduction);
  return GET_COINS(ch);
}

int increase_bank_coins(struct char_data *ch, int amt)
{
  int curr_bank;

  if (IS_NPC(ch)) return 0;

  curr_bank = GET_BANK_COINS(ch);

  if (amt < 0) {
    GET_BANK_COINS(ch) = MAX(0, curr_bank+amt);
    /* Validate to prevent overflow */
    if (GET_BANK_COINS(ch) > curr_bank) GET_BANK_COINS(ch) = 0;
  } else {
    GET_BANK_COINS(ch) = MIN(MAX_BANK_COINS, curr_bank+amt);
    /* Validate to prevent overflow */
    if (GET_BANK_COINS(ch) < curr_bank) GET_BANK_COINS(ch) = MAX_BANK_COINS;
  }
  if (GET_BANK_COINS(ch) == MAX_BANK_COINS)
    send_to_char(ch, "%sYou have reached the maximum bank balance!\r\n%sYou cannot put more into your account unless you withdraw some first.\r\n", QBRED, QNRM);
  return (GET_BANK_COINS(ch));
}

int decrease_bank_coins(struct char_data *ch, int deduction)
{
  int amt;
  amt = (deduction * -1);
  increase_bank_coins(ch, amt);
  return (GET_BANK_COINS(ch));
}
