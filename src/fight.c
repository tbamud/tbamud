/**************************************************************************
*  File: fight.c                                           Part of tbaMUD *
*  Usage: Combat system.                                                  *
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
#include "handler.h"
#include "interpreter.h"
#include "db.h"
#include "spells.h"
#include "screen.h"
#include "constants.h"
#include "py_triggers.h"
#include "act.h"
#include "class.h"
#include "fight.h"
#include "shop.h"
#include "quest.h"


/* locally defined global variables, used externally */
/* head of l-list of fighting chars */
struct char_data *combat_list = NULL;
/* Weapon attack texts */
struct attack_hit_type attack_hit_text[] =
{
  {"hit", "hits"},    /* 0 */
  {"sting", "stings"},
  {"whip", "whips"},
  {"slash", "slashes"},
  {"bite", "bites"},
  {"bludgeon", "bludgeons"},  /* 5 */
  {"crush", "crushes"},
  {"pound", "pounds"},
  {"claw", "claws"},
  {"maul", "mauls"},
  {"thrash", "thrashes"}, /* 10 */
  {"pierce", "pierces"},
  {"blast", "blasts"},
  {"punch", "punches"},
  {"stab", "stabs"}
};

/* local (file scope only) variables */
static struct char_data *next_combat_list = NULL;

/* local file scope utility functions */
static void perform_group_gain(struct char_data *ch, int base, struct char_data *victim);
static void dam_message(int dam, struct char_data *ch, struct char_data *victim, int w_type);
static void make_corpse(struct char_data *ch);
static void group_gain(struct char_data *ch, struct char_data *victim);
static void solo_gain(struct char_data *ch, struct char_data *victim);
/** @todo refactor this function name */
static char *replace_string(const char *str, const char *weapon_singular, const char *weapon_plural);
static int roll_damage(struct char_data *ch, struct char_data *victim,
                       struct obj_data *wielded, int w_type);

/* Base damage roller; STR-based while there are no ranged types. */
static int roll_damage(struct char_data *ch, struct char_data *victim,
                       struct obj_data *wielded, int w_type)
{
  int dam = 0;

  if (wielded && GET_OBJ_TYPE(wielded) == ITEM_WEAPON) {
    int ndice = GET_OBJ_VAL(wielded, 0); /* #dice */
    int sdice = GET_OBJ_VAL(wielded, 1); /* sides */
    dam = dice(ndice, sdice);
    dam += GET_ABILITY_MOD(GET_STR(ch));     /* STR adds to weapon damage */
  } else {
    /* unarmed */
    dam = dice(1, 2);
    dam += GET_ABILITY_MOD(GET_STR(ch));
  }

  if (dam < 0) dam = 0;
  return dam;
}

/* Map the current attack (unarmed/weapon damage type) to SKILL_* constant. */
static int weapon_family_skill_num(struct char_data *ch, struct obj_data *wielded, int w_type) {
  /* Unarmed? */
  if (!wielded || GET_OBJ_TYPE(wielded) != ITEM_WEAPON)
    return SKILL_UNARMED;

  /* NOTE: w_type here is TYPE_HIT + GET_OBJ_VAL(wielded, 2) or mob attack type + TYPE_HIT.
     Adjust the cases below to match your game's TYPE_* values. */
  switch (w_type) {
    /* --- Piercing family --- */
    case TYPE_STAB:
    case TYPE_PIERCE:
    case TYPE_WHIP:
    case TYPE_STING:
      return SKILL_PIERCING_WEAPONS;

    /* --- Slashing family --- */
    case TYPE_SLASH:
    case TYPE_MAUL:
    case TYPE_CLAW:
      return SKILL_SLASHING_WEAPONS;

    /* --- Bludgeoning family --- */
    case TYPE_BLUDGEON:
    case TYPE_CRUSH:
    case TYPE_THRASH:
    case TYPE_POUND:
      return SKILL_BLUDGEONING_WEAPONS;

    /* Fallback */
    default:
      return SKILL_UNARMED;
  }
}

/* Map SKILL_* constants to the strings your gain_skill(name, failure) expects. */
static const char *skill_name_for_gain(int skillnum) {
  switch (skillnum) {
    case SKILL_UNARMED:              return "unarmed";
    case SKILL_PIERCING_WEAPONS:     return "piercing weapons";
    case SKILL_SLASHING_WEAPONS:     return "slashing weapons";
    case SKILL_BLUDGEONING_WEAPONS:  return "bludgeoning weapons";
    case SKILL_SHIELD_USE:           return "shield use";
    default:                         return "unarmed";
  }
}

#define IS_WEAPON(type) (((type) >= TYPE_HIT) && ((type) < TYPE_SUFFERING))
/* The Fight related routines */
void appear(struct char_data *ch)
{
  if (affected_by_spell(ch, SPELL_INVISIBLE))
    affect_from_char(ch, SPELL_INVISIBLE);

  REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_INVISIBLE);
  REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_HIDE);

  if (GET_LEVEL(ch) < LVL_IMMORT)
    act("$n slowly fades into existence.", FALSE, ch, 0, 0, TO_ROOM);
  else
    act("You feel a strange presence as $n appears, seemingly from nowhere.",
	FALSE, ch, 0, 0, TO_ROOM);
}

int compute_armor_class(struct char_data *ch)
{
  int armorclass = GET_AC(ch);

  if (AWAKE(ch))
    armorclass += GET_ABILITY_MOD(GET_DEX(ch)) * 10;

  return (MAX(-100, armorclass));      /* -100 is lowest */
}

void update_pos(struct char_data *victim)
{
  if ((GET_HIT(victim) > 0) && (GET_POS(victim) > POS_STUNNED))
    return;
  else if (GET_HIT(victim) > 0)
    GET_POS(victim) = POS_STANDING;
  else if (GET_HIT(victim) <= -11)
    GET_POS(victim) = POS_DEAD;
  else if (GET_HIT(victim) <= -6)
    GET_POS(victim) = POS_MORTALLYW;
  else if (GET_HIT(victim) <= -3)
    GET_POS(victim) = POS_INCAP;
  else
    GET_POS(victim) = POS_STUNNED;
}

/* start one char fighting another (yes, it is horrible, I know... )  */
void set_fighting(struct char_data *ch, struct char_data *vict)
{
  if (ch == vict)
    return;

  if (FIGHTING(ch)) {
    core_dump();
    return;
  }

  ch->next_fighting = combat_list;
  combat_list = ch;

  if (AFF_FLAGGED(ch, AFF_SLEEP))
    affect_from_char(ch, SPELL_SLEEP);

  FIGHTING(ch) = vict;
  GET_POS(ch) = POS_FIGHTING;

}

/* remove a char from the list of fighting chars */
void stop_fighting(struct char_data *ch)
{
  struct char_data *temp;

  if (ch == next_combat_list)
    next_combat_list = ch->next_fighting;

  REMOVE_FROM_LIST(ch, combat_list, next_fighting);
  ch->next_fighting = NULL;
  FIGHTING(ch) = NULL;
  GET_POS(ch) = POS_STANDING;
  update_pos(ch);
}

static void make_corpse(struct char_data *ch)
{
  char buf2[MAX_NAME_LENGTH + 64];
  struct obj_data *corpse, *o;
  int i, x, y;

  corpse = create_obj();

  corpse->corpse_mob_vnum = IS_NPC(ch) ? GET_MOB_VNUM(ch) : 0;

  corpse->item_number = NOTHING;
  IN_ROOM(corpse) = NOWHERE;
  corpse->name = strdup("corpse");

  /* Use short description if available, otherwise fall back to name */
  const char *who = NULL;

  if (GET_SHORT_DESC(ch) && *GET_SHORT_DESC(ch))
    who = GET_SHORT_DESC(ch);
  else
    who = GET_NAME(ch);

  snprintf(buf2, sizeof(buf2), "The corpse of %s is lying here.", who);
  corpse->description = strdup(buf2);

  snprintf(buf2, sizeof(buf2), "the corpse of %s", who);
  corpse->short_description = strdup(buf2);

  GET_OBJ_TYPE(corpse) = ITEM_CONTAINER;
  for (x = y = 0; x < EF_ARRAY_MAX || y < TW_ARRAY_MAX; x++, y++) {
    if (x < EF_ARRAY_MAX)
      GET_OBJ_EXTRA_AR(corpse, x) = 0;
    if (y < TW_ARRAY_MAX)
      corpse->obj_flags.wear_flags[y] = 0;
  }
  SET_BIT_AR(GET_OBJ_WEAR(corpse), ITEM_WEAR_TAKE);
  SET_BIT_AR(GET_OBJ_EXTRA(corpse), ITEM_NODONATE);
  GET_OBJ_VAL(corpse, 0) = 0;    /* You can't store stuff in a corpse */
  GET_OBJ_VAL(corpse, 3) = 1;    /* corpse identifier */
  GET_OBJ_WEIGHT(corpse) = GET_WEIGHT(ch) + IS_CARRYING_W(ch);
  if (IS_NPC(ch))
    GET_OBJ_TIMER(corpse) = CONFIG_MAX_NPC_CORPSE_TIME;
  else
    GET_OBJ_TIMER(corpse) = CONFIG_MAX_PC_CORPSE_TIME;

  /* transfer character's inventory to the corpse */
  corpse->contains = ch->carrying;
  for (o = corpse->contains; o != NULL; o = o->next_content)
    o->in_obj = corpse;
  object_list_new_owner(corpse, NULL);

  /* transfer character's equipment to the corpse */
  for (i = 0; i < NUM_WEARS; i++)
    if (GET_EQ(ch, i)) {
      remove_otrigger(GET_EQ(ch, i), ch);
      obj_to_obj(unequip_char(ch, i), corpse);
    }

  /* transfer coins */
  if (GET_COINS(ch) > 0)
    GET_COINS(ch) = 0;
  ch->carrying = NULL;
  IS_CARRYING_N(ch) = 0;
  IS_CARRYING_W(ch) = 0;

  obj_to_room(corpse, IN_ROOM(ch));
}

void death_cry(struct char_data *ch)
{
  int door;

  act("Your blood freezes as you hear $n's death cry.", FALSE, ch, 0, 0, TO_ROOM);

  for (door = 0; door < DIR_COUNT; door++)
    if (CAN_GO(ch, door))
      send_to_room(world[IN_ROOM(ch)].dir_option[door]->to_room, "Your blood freezes as you hear someone's death cry.\r\n");
}

void raw_kill(struct char_data * ch, struct char_data * killer)
{
struct char_data *i;

  if (FIGHTING(ch))
    stop_fighting(ch);

  while (ch->affected)
    affect_remove(ch, ch->affected);

  /* To make ordinary commands work in scripts.  welcor*/
  GET_POS(ch) = POS_STANDING;

  if (killer) {
    if (death_mtrigger(ch, killer))
      death_cry(ch);
  } else
    death_cry(ch);

  if (killer) {
    if (killer->group) {
      while ((i = (struct char_data *) simple_list(killer->group->members)) != NULL)
        if(IN_ROOM(i) == IN_ROOM(ch)  || (world[IN_ROOM(i)].zone == world[IN_ROOM(ch)].zone))
          autoquest_trigger_check(i, ch, NULL, AQ_MOB_KILL);      
    } else
        autoquest_trigger_check(killer, ch, NULL, AQ_MOB_KILL);
  }

  /* Alert Group if Applicable */
  if (GROUP(ch))
    send_to_group(ch, GROUP(ch), "%s has died.\r\n", GET_NAME(ch));

  update_pos(ch);
  GET_POS(ch) = POS_DEAD;

  make_corpse(ch);
  extract_char(ch);

  if (killer) {
    autoquest_trigger_check(killer, NULL, NULL, AQ_MOB_SAVE);
    autoquest_trigger_check(killer, NULL, NULL, AQ_ROOM_CLEAR);
  }
}

void die(struct char_data * ch, struct char_data * killer)
{
  if (!IS_NPC(ch)) {
  }
  raw_kill(ch, killer);
}

static void perform_group_gain(struct char_data *ch, int base,
			     struct char_data *victim)
{
  (void)ch;
  (void)base;
  (void)victim;
}

static void group_gain(struct char_data *ch, struct char_data *victim)
{
  int tot_members = 0, base, tot_gain;
  struct char_data *k;
  
  while ((k = (struct char_data *) simple_list(GROUP(ch)->members)) != NULL)
    if (IN_ROOM(ch) == IN_ROOM(k))
      tot_members++;

  /* round up to the nearest tot_members */
  tot_gain = (GET_EXP(victim) / 3) + tot_members - 1;

  /* prevent illegal xp creation when killing players */
  if (!IS_NPC(victim))
    tot_gain = MIN(CONFIG_MAX_EXP_LOSS * 2 / 3, tot_gain);

  if (tot_members >= 1)
    base = MAX(1, tot_gain / tot_members);
  else
    base = 0;

  while ((k = (struct char_data *) simple_list(GROUP(ch)->members)) != NULL)
    if (IN_ROOM(k) == IN_ROOM(ch))
      perform_group_gain(k, base, victim);
}

static void solo_gain(struct char_data *ch, struct char_data *victim)
{
  int exp;

  exp = MIN(CONFIG_MAX_EXP_GAIN, GET_EXP(victim) / 3);

  /* Calculate level-difference bonus */
  if (IS_NPC(ch))
    exp += MAX(0, (exp * MIN(4, (GET_LEVEL(victim) - GET_LEVEL(ch)))) / 8);
  else
    exp += MAX(0, (exp * MIN(8, (GET_LEVEL(victim) - GET_LEVEL(ch)))) / 8);

  exp = MAX(exp, 1);
}

static char *replace_string(const char *str, const char *weapon_singular, const char *weapon_plural)
{
  static char buf[256];
  char *cp = buf;

  for (; *str; str++) {
    if (*str == '#') {
      switch (*(++str)) {
      case 'W':
	for (; *weapon_plural; *(cp++) = *(weapon_plural++));
	break;
      case 'w':
	for (; *weapon_singular; *(cp++) = *(weapon_singular++));
	break;
      default:
	*(cp++) = '#';
	break;
      }
    } else
      *(cp++) = *str;

    *cp = 0;
  }				/* For */

  return (buf);
}

/* message for doing damage with a weapon */
static void dam_message(int dam, struct char_data *ch, struct char_data *victim,
		      int w_type)
{
  char *buf;
  int msgnum;

  static struct dam_weapon_type {
    const char *to_room;
    const char *to_char;
    const char *to_victim;
  } dam_weapons[] = {

    /* use #w for singular (i.e. "slash") and #W for plural (i.e. "slashes") */

    {
      "$n tries to #w $N, but misses.",	/* 0: 0     */
      "You try to #w $N, but miss.",
      "$n tries to #w you, but misses."
    },

    {
      "$n tickles $N as $e #W $M.",	/* 1: 1..2  */
      "You tickle $N as you #w $M.",
      "$n tickles you as $e #W you."
    },

    {
      "$n barely #W $N.",		/* 2: 3..4  */
      "You barely #w $N.",
      "$n barely #W you."
    },

    {
      "$n #W $N.",			/* 3: 5..6  */
      "You #w $N.",
      "$n #W you."
    },

    {
      "$n #W $N hard.",			/* 4: 7..10  */
      "You #w $N hard.",
      "$n #W you hard."
    },

    {
      "$n #W $N very hard.",		/* 5: 11..14  */
      "You #w $N very hard.",
      "$n #W you very hard."
    },

    {
      "$n #W $N extremely hard.",	/* 6: 15..19  */
      "You #w $N extremely hard.",
      "$n #W you extremely hard."
    },

    {
      "$n massacres $N to small fragments with $s #w.",	/* 7: 19..23 */
      "You massacre $N to small fragments with your #w.",
      "$n massacres you to small fragments with $s #w."
    },

    {
      "$n OBLITERATES $N with $s deadly #w!!",	/* 8: > 23   */
      "You OBLITERATE $N with your deadly #w!!",
      "$n OBLITERATES you with $s deadly #w!!"
    }
  };

  w_type -= TYPE_HIT;		/* Change to base of table with text */

  if (dam == 0)		msgnum = 0;
  else if (dam <= 2)    msgnum = 1;
  else if (dam <= 4)    msgnum = 2;
  else if (dam <= 6)    msgnum = 3;
  else if (dam <= 10)   msgnum = 4;
  else if (dam <= 14)   msgnum = 5;
  else if (dam <= 19)   msgnum = 6;
  else if (dam <= 23)   msgnum = 7;
  else			msgnum = 8;

  /* damage message to onlookers */
  buf = replace_string(dam_weapons[msgnum].to_room,
	  attack_hit_text[w_type].singular, attack_hit_text[w_type].plural);
  act(buf, FALSE, ch, NULL, victim, TO_NOTVICT);

  /* damage message to damager */
  if (GET_LEVEL(ch) >= LVL_IMMORT)
	send_to_char(ch, "(%d) ", dam);
  buf = replace_string(dam_weapons[msgnum].to_char,
	  attack_hit_text[w_type].singular, attack_hit_text[w_type].plural);
  act(buf, FALSE, ch, NULL, victim, TO_CHAR);
  send_to_char(ch, CCNRM(ch, C_CMP));

  /* damage message to damagee */
  if (GET_LEVEL(victim) >= LVL_IMMORT)
    send_to_char(victim, "\tR(%d)", dam);
  buf = replace_string(dam_weapons[msgnum].to_victim,
	  attack_hit_text[w_type].singular, attack_hit_text[w_type].plural);
  act(buf, FALSE, ch, NULL, victim, TO_VICT | TO_SLEEP);
  send_to_char(victim, CCNRM(victim, C_CMP));
}

/*  message for doing damage with a spell or skill. Also used for weapon
 *  damage on miss and death blows. */
int skill_message(int dam, struct char_data *ch, struct char_data *vict,
		      int attacktype)
{
  int i, j, nr;
  struct message_type *msg;

  struct obj_data *weap = GET_EQ(ch, WEAR_WIELD);

  /* @todo restructure the messages library to a pointer based system as
   * opposed to the current cyclic location system. */
  for (i = 0; i < MAX_MESSAGES; i++) {
    if (fight_messages[i].a_type == attacktype) {
      nr = dice(1, fight_messages[i].number_of_attacks);
      for (j = 1, msg = fight_messages[i].msg; (j < nr) && msg; j++)
        msg = msg->next;

      if (!IS_NPC(vict) && (GET_LEVEL(vict) >= LVL_IMPL)) {
        act(msg->god_msg.attacker_msg, FALSE, ch, weap, vict, TO_CHAR);
        act(msg->god_msg.victim_msg, FALSE, ch, weap, vict, TO_VICT);
        act(msg->god_msg.room_msg, FALSE, ch, weap, vict, TO_NOTVICT);
      } else if (dam != 0) {
        /*
         * Don't send redundant color codes for TYPE_SUFFERING & other types
         * of damage without attacker_msg.
         */
        if (GET_POS(vict) == POS_DEAD) {
          if (msg->die_msg.attacker_msg) {
            send_to_char(ch, CCYEL(ch, C_CMP));
            act(msg->die_msg.attacker_msg, FALSE, ch, weap, vict, TO_CHAR);
            send_to_char(ch, CCNRM(ch, C_CMP));
          }

          send_to_char(vict, CCRED(vict, C_CMP));
          act(msg->die_msg.victim_msg, FALSE, ch, weap, vict, TO_VICT | TO_SLEEP);
          send_to_char(vict, CCNRM(vict, C_CMP));

          act(msg->die_msg.room_msg, FALSE, ch, weap, vict, TO_NOTVICT);
        } else {
          if (msg->hit_msg.attacker_msg) {
            send_to_char(ch, CCYEL(ch, C_CMP));
            act(msg->hit_msg.attacker_msg, FALSE, ch, weap, vict, TO_CHAR);
            send_to_char(ch, CCNRM(ch, C_CMP));
          }

          send_to_char(vict, CCRED(vict, C_CMP));
          act(msg->hit_msg.victim_msg, FALSE, ch, weap, vict, TO_VICT | TO_SLEEP);
          send_to_char(vict, CCNRM(vict, C_CMP));

          act(msg->hit_msg.room_msg, FALSE, ch, weap, vict, TO_NOTVICT);
        }
      } else if (ch != vict) {	/* Dam == 0 */
        if (msg->miss_msg.attacker_msg) {
          send_to_char(ch, CCYEL(ch, C_CMP));
          act(msg->miss_msg.attacker_msg, FALSE, ch, weap, vict, TO_CHAR);
          send_to_char(ch, CCNRM(ch, C_CMP));
        }

        send_to_char(vict, CCRED(vict, C_CMP));
        act(msg->miss_msg.victim_msg, FALSE, ch, weap, vict, TO_VICT | TO_SLEEP);
        send_to_char(vict, CCNRM(vict, C_CMP));

        act(msg->miss_msg.room_msg, FALSE, ch, weap, vict, TO_NOTVICT);
      }
      return (1);
    }
  }
  return (0);
}

/* This function returns the following codes:
 *	< 0	Victim died.
 *	= 0	No damage.
 *	> 0	How much damage done. */
int damage(struct char_data *ch, struct char_data *victim, int dam, int attacktype)
{
  long local_coins = 0;
  char local_buf[256];
  struct char_data *tmp_char;
  struct obj_data *corpse_obj;
  int prev_hit = 0;

  if (GET_POS(victim) <= POS_DEAD) {
    /* This is "normal"-ish now with delayed extraction. -gg 3/15/2001 */
    if (PLR_FLAGGED(victim, PLR_NOTDEADYET) || MOB_FLAGGED(victim, MOB_NOTDEADYET))
      return (-1);

    log("SYSERR: Attempt to damage corpse '%s' in room #%d by '%s'.",
        GET_NAME(victim), GET_ROOM_VNUM(IN_ROOM(victim)), GET_NAME(ch));
    die(victim, ch);
    return (-1);			/* -je, 7/7/92 */
  }

  /* peaceful rooms */
  if (ch->nr != real_mobile(DG_CASTER_PROXY) &&
      ch != victim && ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL)) {
    send_to_char(ch, "This room just has such a peaceful, easy feeling...\r\n");
    return (0);
  }

  /* shopkeeper and MOB_NOKILL protection */
  if (!ok_damage_shopkeeper(ch, victim) || MOB_FLAGGED(victim, MOB_NOKILL)) {
    send_to_char(ch, "This mob is protected.\r\n");
    return (0);
  }

  /* Immortals cannot be damaged. */
  if (!IS_NPC(victim) && GET_REAL_LEVEL(victim) >= LVL_IMMORT)
    dam = 0;

  dam = damage_mtrigger(ch, victim, dam, attacktype);
  if (dam == -1) {
  	return (0);
  }

  if (victim != ch) {
    /* Start the attacker fighting the victim */
    if (GET_POS(ch) > POS_STUNNED && (FIGHTING(ch) == NULL))
      set_fighting(ch, victim);

    /* Start the victim fighting the attacker */
    if (GET_POS(victim) > POS_STUNNED && (FIGHTING(victim) == NULL)) {
      set_fighting(victim, ch);
      if (MOB_FLAGGED(victim, MOB_MEMORY) && !IS_NPC(ch))
	remember(victim, ch);
    }
  }

  /* If you attack a pet, it hates your guts */
  if (victim->master == ch)
    stop_follower(victim);

  /* If the attacker is invisible, he becomes visible */
  if (AFF_FLAGGED(ch, AFF_INVISIBLE) || AFF_FLAGGED(ch, AFF_HIDE))
    appear(ch);

  /* Cut damage in half if victim has sanct, to a minimum 1 */
  if (AFF_FLAGGED(victim, AFF_SANCTUARY) && dam >= 2)
    dam /= 2;

  /* Check for PK if this is not a PK MUD */
  /* Set the maximum damage per round and subtract the hit points */
  dam = MAX(MIN(dam, 100), 0);
  prev_hit = GET_HIT(victim);
  GET_HIT(victim) -= dam;

  update_pos(victim);

  /* skill_message sends a message from the messages file in lib/misc.
   * dam_message just sends a generic "You hit $n extremely hard.".
   * skill_message is preferable to dam_message because it is more
   * descriptive.
   *
   * If we are _not_ attacking with a weapon (i.e. a spell), always use
   * skill_message. If we are attacking with a weapon: If this is a miss or a
   * death blow, send a skill_message if one exists; if not, default to a
   * dam_message. Otherwise, always send a dam_message. */
  if (!IS_WEAPON(attacktype))
    skill_message(dam, ch, victim, attacktype);
  else {
    if (GET_POS(victim) == POS_DEAD || dam == 0) {
      if (!skill_message(dam, ch, victim, attacktype))
	dam_message(dam, ch, victim, attacktype);
    } else {
      dam_message(dam, ch, victim, attacktype);
    }
  }

  /* Use send_to_char -- act() doesn't send message if you are DEAD. */
  switch (GET_POS(victim)) {
  case POS_MORTALLYW:
    act("$n is mortally wounded, and will die soon, if not aided.", TRUE, victim, 0, 0, TO_ROOM);
    send_to_char(victim, "You are mortally wounded, and will die soon, if not aided.\r\n");
    break;
  case POS_INCAP:
    act("$n is incapacitated and will slowly die, if not aided.", TRUE, victim, 0, 0, TO_ROOM);
    send_to_char(victim, "You are incapacitated and will slowly die, if not aided.\r\n");
    break;
  case POS_STUNNED:
    act("$n is stunned, but will probably regain consciousness again.", TRUE, victim, 0, 0, TO_ROOM);
    send_to_char(victim, "You're stunned, but will probably regain consciousness again.\r\n");
    break;
  case POS_DEAD:
    act("$n is dead!  R.I.P.", FALSE, victim, 0, 0, TO_ROOM);
    send_to_char(victim, "You are dead!  Sorry...\r\n");
    break;

  default:			/* >= POSITION SLEEPING */
    if (dam > (GET_MAX_HIT(victim) / 4))
      send_to_char(victim, "That really did HURT!\r\n");

    if (GET_HIT(victim) < (GET_MAX_HIT(victim) / 4)) {
      send_to_char(victim, "%sYou wish that your wounds would stop BLEEDING so much!%s\r\n",
		CCRED(victim, C_SPR), CCNRM(victim, C_SPR));
      if (ch != victim && MOB_FLAGGED(victim, MOB_WIMPY))
	do_flee(victim, NULL, 0, 0);
    }
    if (!IS_NPC(victim) && GET_WIMP_LEV(victim) && (victim != ch) &&
	GET_HIT(victim) < GET_WIMP_LEV(victim) && GET_HIT(victim) > 0) {
      send_to_char(victim, "You wimp out, and attempt to flee!\r\n");
      do_flee(victim, NULL, 0, 0);
    }
    break;
  }

  /* Help out poor linkless people who are attacked */
  if (!IS_NPC(victim) && !(victim->desc) && GET_POS(victim) > POS_STUNNED) {
    do_flee(victim, NULL, 0, 0);
    if (!FIGHTING(victim)) {
      act("$n is rescued by divine forces.", FALSE, victim, 0, 0, TO_ROOM);
      GET_WAS_IN(victim) = IN_ROOM(victim);
      char_from_room(victim);
      char_to_room(victim, 0);
    }
  }

  /* stop someone from fighting if they're stunned or worse */
  if (GET_POS(victim) <= POS_STUNNED && FIGHTING(victim) != NULL)
    stop_fighting(victim);

  /* Uh oh.  Victim died. */
  if (GET_POS(victim) == POS_DEAD) {
    if (ch != victim && (IS_NPC(victim) || victim->desc)) {
      if (GROUP(ch))
	group_gain(ch, victim);
      else
        solo_gain(ch, victim);
    }

    if (!IS_NPC(victim)) {
      /* NPC-safe invis check for killer (ch) */
      int ch_invis = IS_NPC(ch) ? 0 : GET_INVIS_LEV(ch);  /* <-- added */
      mudlog(BRF, MAX(LVL_IMMORT, MAX(ch_invis, GET_INVIS_LEV(victim))),
        TRUE, "%s killed by %s at %s",
        GET_NAME(victim), GET_NAME(ch), world[IN_ROOM(victim)].name);
      if (MOB_FLAGGED(ch, MOB_MEMORY))
	forget(ch, victim);
    }
    /* Can't determine GET_COINS on corpse, so do now and store */
    if (IS_NPC(victim)) {
      local_coins = GET_COINS(victim);
      sprintf(local_buf,"%ld", (long)local_coins);
    }

    die(victim, ch);
    if (GROUP(ch) && (local_coins > 0) && PRF_FLAGGED(ch, PRF_AUTOSPLIT) ) {
      generic_find("corpse", FIND_OBJ_ROOM, ch, &tmp_char, &corpse_obj);
      if (corpse_obj) {
        do_get(ch, "all.coin corpse", 0, 0);
        do_split(ch, local_buf, 0, 0);
      }
    }
    if (!IS_NPC(ch) && (ch != victim) && PRF_FLAGGED(ch, PRF_AUTOLOOT)) {
      do_get(ch, "all corpse", 0, 0);
    }
    return (-1);
  }
  return (dam);
}

/*
 * hit() -- one character attempts to hit another with a weapon or attack.
 * Ascending AC (5e-like), nat 1/20, bounded bonuses, and skill gains.
 * Since there are no ranged types yet, we always use STR for attack & damage mods.
 */
void hit(struct char_data *ch, struct char_data *victim, int type)
{
  struct obj_data *wielded = GET_EQ(ch, WEAR_WIELD);
  bool wielded_weapon = (wielded && GET_OBJ_TYPE(wielded) == ITEM_WEAPON);
  struct obj_data *shield  = GET_EQ(victim, WEAR_SHIELD);
  int w_type, d20, attack_mod = 0, target_ac, dam = 0;
  bool hit_success = FALSE;
  bool attacker_immortal, victim_immortal;
  bool attacker_is_player, victim_is_player;
  int unarmed_die_size = 0;
  int unarmed_prof_bonus = 0;
  int unarmed_str_mod = 0;

  /* Basic sanity */
  if (!ch || !victim) return;

  attacker_is_player = (ch->desc != NULL);
  victim_is_player = (victim->desc != NULL);

  attacker_immortal = (attacker_is_player &&
      (GET_REAL_LEVEL(ch) >= LVL_IMMORT || PRF_FLAGGED(ch, PRF_NOHASSLE)));
  victim_immortal = (victim_is_player &&
      (GET_REAL_LEVEL(victim) >= LVL_IMMORT || PRF_FLAGGED(victim, PRF_NOHASSLE)));

  /* Determine attack message type exactly like stock code */
  if (wielded_weapon)
    w_type = GET_OBJ_VAL(wielded, 2) + TYPE_HIT;
  else {
    if (IS_NPC(ch) && ch->mob_specials.attack_type != 0)
      w_type = ch->mob_specials.attack_type + TYPE_HIT;
    else
      w_type = TYPE_HIT;
  } /* matches stock message mapping */

  if (victim_immortal) {
    damage(ch, victim, 0, w_type);
    return;
  }

  if (attacker_immortal) {
    if (wielded_weapon) {
      int ndice = GET_OBJ_VAL(wielded, 0);
      int sdice = GET_OBJ_VAL(wielded, 1);
      dam = (ndice * sdice) + GET_ABILITY_MOD(GET_STR(ch));
    } else {
      int prof_bonus = (!IS_NPC(ch)) ? GET_PROFICIENCY(GET_SKILL(ch, SKILL_UNARMED)) : 0;
      int str_mod = GET_ABILITY_MOD(GET_STR(ch));
      int die_size;

      switch (prof_bonus) {
        case 0:  die_size = 4; break;  /* untrained */
        case 1:  die_size = 6; break;  /* trained */
        case 2:  die_size = 8; break;  /* expert */
        default: die_size = 10; break; /* master or above */
      }

      if (IS_NPC(ch) && prof_bonus <= 0) {
        prof_bonus = MIN(6, (GET_LEVEL(ch) / 4));
      }

      dam = die_size + str_mod + prof_bonus;
    }

    damage(ch, victim, dam, w_type);
    return;
  }

  /* Roll d20 */
  d20 = rand_number(1, 20);

  /* Ability modifier: STR only (no ranged types yet) */
  attack_mod += GET_ABILITY_MOD(GET_STR(ch));

  /* Skill family & proficiency */
  {
    int  skillnum  = weapon_family_skill_num(ch, wielded, w_type);
    const char *skillname = skill_name_for_gain(skillnum);

    /* proficiency from current % */
    if (!IS_NPC(ch))
      attack_mod += GET_PROFICIENCY(GET_SKILL(ch, skillnum));

    /* --- UNARMED ATTACK HANDLING --- */
    if (!wielded_weapon) {
      int prof_bonus = (!IS_NPC(ch)) ? GET_PROFICIENCY(GET_SKILL(ch, SKILL_UNARMED)) : 0;
      int str_mod = GET_ABILITY_MOD(GET_STR(ch));
      int die_size;

      /* Simple scaling by proficiency tier */
      switch (prof_bonus) {
        case 0:  die_size = 4; break;  /* untrained */
        case 1:  die_size = 6; break;  /* trained */
        case 2:  die_size = 8; break;  /* expert */
        default: die_size = 10; break; /* master or above */
      }
      unarmed_die_size = die_size;

      /* NPC fallback scaling */
      if (IS_NPC(ch) && prof_bonus <= 0) {
        prof_bonus = MIN(6, (GET_LEVEL(ch) / 4)); /* level scaling */
      }

      /* base damage roll for unarmed attacks */
      dam = dice(1, die_size) + str_mod + prof_bonus;
      unarmed_prof_bonus = prof_bonus;
      unarmed_str_mod = str_mod;

      /* mark attack type for damage() messaging */
      w_type = TYPE_HIT;
    }

    /* Weapon magic (cap +3) */
    if (wielded && GET_OBJ_TYPE(wielded) == ITEM_WEAPON) {
      int wmag = GET_OBJ_VAL(wielded, VAL_ARMOR_MAGIC_BONUS);
      if (wmag > MAX_WEAPON_MAGIC) wmag = MAX_WEAPON_MAGIC;
      attack_mod += wmag;
    }

    /* Situational attack modifiers hook (spells, conditions) */
    attack_mod += 0;

    /* Cap total attack bonus to +10 */
    if (attack_mod > MAX_TOTAL_ATTACK_BONUS)
      attack_mod = MAX_TOTAL_ATTACK_BONUS;

    /* Ascending AC target */
    target_ac = compute_armor_class_asc(victim);

    /* Nat 1/20, then normal resolution (immortals always hit). */
    if (attacker_immortal) hit_success = TRUE;
    else if (d20 == 1)       hit_success = FALSE;
    else if (d20 == 20) hit_success = TRUE;
    else                hit_success = ((d20 + attack_mod) >= target_ac);

    /* Apply result */
    if (hit_success) {
      /* Roll damage up front (needed for shield durability)
       * If we are unarmed, dam was already rolled above.
       * If wielding a weapon, roll normally.
       */
      if (wielded_weapon)
        dam = roll_damage(ch, victim, wielded, w_type);
      if (attacker_immortal) {
        if (wielded_weapon) {
          int ndice = GET_OBJ_VAL(wielded, 0);
          int sdice = GET_OBJ_VAL(wielded, 1);
          dam = (ndice * sdice) + GET_ABILITY_MOD(GET_STR(ch));
        } else if (unarmed_die_size > 0) {
          dam = unarmed_die_size + unarmed_str_mod + unarmed_prof_bonus;
        }
      }

      /* --- SHIELD BLOCK CHECK ---
       * Only happens if an attack actually lands.
       */
      if (shield && !attacker_immortal) {
        int def_prof = (!IS_NPC(victim)) ? GET_PROFICIENCY(GET_SKILL(victim, SKILL_SHIELD_USE)) : 0;
        int block_chance = def_prof * 10;   /* 0–60% total chance to block an attack */

        if (block_chance > 0 && rand_number(1, 100) <= block_chance) {
          /* Block succeeded! */
          act("You block $N's attack with $p!", FALSE, victim, shield, ch, TO_CHAR);
          act("$n blocks your attack with $s $p!", FALSE, victim, shield, ch, TO_VICT);
          act("$n blocks $N's attack with $s $p!", TRUE, victim, shield, ch, TO_NOTVICT);

          /* Durability reduction based on damage prevented */
          int *dur = &GET_OBJ_VAL(shield, 3);
          int loss = MAX(1, dam / 10);  /* at least 1% per block */
          *dur -= loss;

          if (*dur <= 0) {
            act("Your $p shatters into pieces!", FALSE, victim, shield, 0, TO_CHAR);
            act("$n's $p shatters into pieces!", TRUE, victim, shield, 0, TO_ROOM);
            extract_obj(shield);
          }

          /* Train shield use skill on success */
          if (!IS_NPC(victim)) {
            gain_skill(victim, "shield use", TRUE);
          }

          return; /* Attack nullified entirely */
        }
      }

      /* No block: apply normal damage */
      damage(ch, victim, dam, w_type);
    } else {
      damage(ch, victim, 0, w_type); /* miss messaging */
    }

    /* --- Skill gains --- */
    if (!IS_NPC(ch) && skillname) {
      gain_skill(ch, (char *)skillname, hit_success);
    }

    /* Defender shield use: every swing trains it if they’re wearing a shield.
       Treat a MISS as a "success" for the shield user (they successfully defended). */
    if (!IS_NPC(victim) && GET_EQ(victim, WEAR_SHIELD)) {
      gain_skill(victim, "shield use", !hit_success);
    }
  }

  /* Optional combat numbers for debugging / builders */
  if (CONFIG_DEBUG_MODE >= NRM) {
    const char *crit = (d20 == 20) ? " (CRIT)" : ((d20 == 1) ? " (NAT 1)" : "");
    send_to_char(ch,
      "\t1Attack:\tn d20=%d%s, mod=%+d \t1⇒\tn total=%d vs AC %d — %s\r\n",
      d20, crit, attack_mod, d20 + attack_mod, target_ac,
      hit_success ? "\t2HIT\tn" : "\t1MISS\tn");
    if (!IS_NPC(victim)) {
      send_to_char(victim,
        "\t1Defense:\tn %s rolled total=%d vs your AC %d — %s%s\r\n",
        GET_NAME(ch), d20 + attack_mod, target_ac,
        hit_success ? "\t1HIT\tn" : "\t2MISS\tn",
        (d20 == 20) ? " (CRIT)" : ((d20 == 1) ? " (NAT 1)" : ""));
    }
  }
}

/* control the fights going on.  Called every 2 seconds from comm.c. */
void perform_violence(void)
{
  struct char_data *ch, *tch;

  for (ch = combat_list; ch; ch = next_combat_list) {
    next_combat_list = ch->next_fighting;

    if (FIGHTING(ch) == NULL || IN_ROOM(ch) != IN_ROOM(FIGHTING(ch))) {
      stop_fighting(ch);
      continue;
    }

    if (IS_NPC(ch)) {
      if (GET_MOB_WAIT(ch) > 0) {
        GET_MOB_WAIT(ch) -= PULSE_VIOLENCE;
        continue;
      }
      GET_MOB_WAIT(ch) = 0;
      if (GET_POS(ch) < POS_FIGHTING) {
        GET_POS(ch) = POS_FIGHTING;
        act("$n scrambles to $s feet!", TRUE, ch, 0, 0, TO_ROOM);
      }
    }

    if (GET_POS(ch) < POS_FIGHTING) {
      send_to_char(ch, "You can't fight while sitting!!\r\n");
      continue;
    }

 if (GROUP(ch) && GROUP(ch)->members && GROUP(ch)->members->iSize) {
      struct iterator_data Iterator;

      tch = (struct char_data *) merge_iterator(&Iterator, GROUP(ch)->members);
    for (; tch ; tch = next_in_list(&Iterator)) {
        if (tch == ch)
          continue;
        if (!IS_NPC(tch) && !PRF_FLAGGED(tch, PRF_AUTOASSIST))
          continue;
        if (IN_ROOM(ch) != IN_ROOM(tch))
          continue;
        if (FIGHTING(tch))
          continue;
        if (GET_POS(tch) != POS_STANDING)
          continue;
        if (!CAN_SEE(tch, ch))
          continue;
      
        do_assist(tch, GET_NAME(ch), 0, 0);				  
      }
    }

    hit(ch, FIGHTING(ch), TYPE_UNDEFINED);
    if (MOB_FLAGGED(ch, MOB_SPEC) && GET_MOB_SPEC(ch) && !MOB_FLAGGED(ch, MOB_NOTDEADYET)) {
      char actbuf[MAX_INPUT_LENGTH] = "";
      (GET_MOB_SPEC(ch)) (ch, ch, 0, actbuf);
    }
  }
}
