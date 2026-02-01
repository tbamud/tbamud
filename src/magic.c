/**************************************************************************
*  File: magic.c                                           Part of tbaMUD *
*  Usage: Low-level functions for magic; spell template code.             *
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
#include "spells.h"
#include "handler.h"
#include "db.h"
#include "interpreter.h"
#include "constants.h"
#include "py_triggers.h"
#include "class.h"
#include "fight.h"
#include "mud_event.h"


/* local file scope function prototypes */
static int mag_materials(struct char_data *ch, IDXTYPE item0, IDXTYPE item1, IDXTYPE item2, int extract, int verbose);
static void perform_mag_groups(int level, struct char_data *ch, struct char_data *tch, int spellnum, int savetype);


/* Roll a 5e-style saving throw: d20 + ability mod + proficiency vs DC. */
int mag_savingthrow(struct char_data *ch, int ability, int dc)
{
  int roll, total, target;

  if (!ch)
    return FALSE;

  roll = roll_d20();
  total = roll + get_save_mod(ch, ability);
  target = MAX(1, dc);

  if (total >= target)
    return TRUE;

  return FALSE;
}

/* affect_update: called from comm.c (causes spells to wear off) */
void affect_update(void)
{
  struct affected_type *af, *next;
  struct char_data *i;

  for (i = character_list; i; i = i->next)
    for (af = i->affected; af; af = next) {
      next = af->next;
      if (af->duration >= 1)
	af->duration--;
      else if (af->duration == -1)	/* No action */
	;
      else {
	if ((af->spell > 0) && (af->spell <= MAX_SPELLS))
	  if (!af->next || (af->next->spell != af->spell) ||
	      (af->next->duration > 0))
	    if (spell_info[af->spell].wear_off_msg)
	      send_to_char(i, "%s\r\n", spell_info[af->spell].wear_off_msg);
	affect_remove(i, af);
      }
    }
}

/* Checks for up to 3 vnums (spell reagents) in the player's inventory. If
 * multiple vnums are passed in, the function ANDs the items together as
 * requirements (ie. if one or more are missing, the spell will not fail).
 * @param ch The caster of the spell.
 * @param item0 The first required item of the spell, NOTHING if not required.
 * @param item1 The second required item of the spell, NOTHING if not required.
 * @param item2 The third required item of the spell, NOTHING if not required.
 * @param extract TRUE if mag_materials should consume (destroy) the items in
 * the players inventory, FALSE if not. Items will only be removed on a
 * successful cast.
 * @param verbose TRUE to provide some generic failure or success messages,
 * FALSE to send no in game messages from this function.
 * @retval int TRUE if ch has all materials to cast the spell, FALSE if not.
 */
static int mag_materials(struct char_data *ch, IDXTYPE item0,
    IDXTYPE item1, IDXTYPE item2, int extract, int verbose)
{
  /* Begin Local variable definitions. */
  /*------------------------------------------------------------------------*/
  /* Used for object searches. */
  struct obj_data *tobj = NULL;
  /* Points to found reagents. */
  struct obj_data *obj0 = NULL, *obj1 = NULL, *obj2 = NULL;
  /*------------------------------------------------------------------------*/
  /* End Local variable definitions. */

  /* Begin success checks. Checks must pass to signal a success. */
  /*------------------------------------------------------------------------*/
  /* Check for the objects in the players inventory. */
  for (tobj = ch->carrying; tobj; tobj = tobj->next_content)
  {
    if ((item0 != NOTHING) && (GET_OBJ_VNUM(tobj) == item0))
    {
      obj0 = tobj;
      item0 = NOTHING;
    }
    else if ((item1 != NOTHING) && (GET_OBJ_VNUM(tobj) == item1))
    {
      obj1 = tobj;
      item1 = NOTHING;
    }
    else if ((item2 != NOTHING) && (GET_OBJ_VNUM(tobj) == item2))
    {
      obj2 = tobj;
      item2 = NOTHING;
    }
  }

  /* If we needed items, but didn't find all of them, then the spell is a
   * failure. */
  if ((item0 != NOTHING) || (item1 != NOTHING) || (item2 != NOTHING))
  {
    /* Generic spell failure messages. */
    if (verbose)
    {
      switch (rand_number(0, 2))
      {
      case 0:
        send_to_char(ch, "A wart sprouts on your nose.\r\n");
        break;
      case 1:
        send_to_char(ch, "Your hair falls out in clumps.\r\n");
        break;
      case 2:
        send_to_char(ch, "A huge corn develops on your big toe.\r\n");
        break;
      }
    }
    /* Return fales, the material check has failed. */
    return (FALSE);
  }
  /*------------------------------------------------------------------------*/
  /* End success checks. */

  /* From here on, ch has all required materials in their inventory and the
   * material check will return a success. */

  /* Begin Material Processing. */
  /*------------------------------------------------------------------------*/
  /* Extract (destroy) the materials, if so called for. */
  if (extract)
  {
    if (obj0 != NULL)
      extract_obj(obj0);
    if (obj1 != NULL)
      extract_obj(obj1);
    if (obj2 != NULL)
      extract_obj(obj2);
    /* Generic success messages that signals extracted objects. */
    if (verbose)
    {
      send_to_char(ch, "A puff of smoke rises from your pack.\r\n");
      act("A puff of smoke rises from $n's pack.", TRUE, ch, NULL, NULL, TO_ROOM);
    }
  }

  /* Don't extract the objects, but signal materials successfully found. */
  if(!extract && verbose)
  {
    send_to_char(ch, "Your pack rumbles.\r\n");
    act("Something rumbles in $n's pack.", TRUE, ch, NULL, NULL, TO_ROOM);
  }
  /*------------------------------------------------------------------------*/
  /* End Material Processing. */

  /* Signal to calling function that the materials were successfully found
   * and processed. */
  return (TRUE);
}


/* Every spell that does damage comes through here.  This calculates the amount
 * of damage, adds in any modifiers, determines what the saves are, tests for
 * save and calls damage(). -1 = dead, otherwise the amount of damage done. */
int mag_damage(int level, struct char_data *ch, struct char_data *victim,
		     int spellnum, int savetype)
{
  int dam = 0;
  int save_dc;

  if (victim == NULL || ch == NULL)
    return (0);

  switch (spellnum) {
    /* Mostly mages */
  case SPELL_MAGIC_MISSILE:
  case SPELL_CHILL_TOUCH:	/* chill touch also has an affect */
    if (IS_SORCEROR(ch))
      dam = dice(1, 8) + 1;
    else
      dam = dice(1, 6) + 1;
    break;
  case SPELL_BURNING_HANDS:
    if (IS_SORCEROR(ch))
      dam = dice(3, 8) + 3;
    else
      dam = dice(3, 6) + 3;
    break;
  case SPELL_SHOCKING_GRASP:
    if (IS_SORCEROR(ch))
      dam = dice(5, 8) + 5;
    else
      dam = dice(5, 6) + 5;
    break;
  case SPELL_LIGHTNING_BOLT:
    if (IS_SORCEROR(ch))
      dam = dice(7, 8) + 7;
    else
      dam = dice(7, 6) + 7;
    break;
  case SPELL_COLOR_SPRAY:
    if (IS_SORCEROR(ch))
      dam = dice(9, 8) + 9;
    else
      dam = dice(9, 6) + 9;
    break;
  case SPELL_FIREBALL:
    if (IS_SORCEROR(ch))
      dam = dice(11, 8) + 11;
    else
      dam = dice(11, 6) + 11;
    break;

    /* Mostly clerics */
  case SPELL_CALL_LIGHTNING:
    dam = dice(7, 8) + 7;
    break;

  case SPELL_HARM:
    dam = dice(8, 8) + 8;
    break;

  case SPELL_ENERGY_DRAIN:
    if (GET_LEVEL(victim) <= 2)
      dam = 100;
    else
      dam = dice(1, 10);
    break;

    /* Area spells */
  case SPELL_EARTHQUAKE:
    dam = dice(2, 8) + level;
    break;

  } /* switch(spellnum) */


  save_dc = compute_save_dc(ch, level, spellnum);
  if (!IS_NPC(ch) && GET_REAL_LEVEL(ch) >= LVL_IMMORT)
    save_dc = 1000;

  /* divide damage by two if victim makes his saving throw */
  if (mag_savingthrow(victim, savetype, save_dc))
    dam /= 2;

  /* and finally, inflict the damage */
  return (damage(ch, victim, dam, spellnum));
}


/* Every spell that does an affect comes through here.  This determines the
 * effect, whether it is added or replacement, whether it is legal or not, etc.
 * affect_join(vict, aff, add_dur, avg_dur, add_mod, avg_mod) */
#define MAX_SPELL_AFFECTS 5	/* change if more needed */

void mag_affects(int level, struct char_data *ch, struct char_data *victim,
		      int spellnum, int savetype)
{
  struct affected_type af[MAX_SPELL_AFFECTS];
  bool accum_affect = FALSE, accum_duration = FALSE;
  const char *to_vict = NULL, *to_room = NULL;
  int i, j;
  int save_dc;


  if (victim == NULL || ch == NULL)
    return;

  save_dc = compute_save_dc(ch, level, spellnum);
  if (!IS_NPC(ch) && GET_REAL_LEVEL(ch) >= LVL_IMMORT)
    save_dc = 1000;

  for (i = 0; i < MAX_SPELL_AFFECTS; i++) {
    new_affect(&(af[i]));
    af[i].spell = spellnum;
  }

  switch (spellnum) {

  case SPELL_CHILL_TOUCH:
    af[0].location = APPLY_STR;
    if (mag_savingthrow(victim, ABIL_CON, save_dc))
      af[0].duration = 1;    /* resisted: brief weakening */
    else
      af[0].duration = 4;    /* failed: longer effect */
    af[0].modifier = -1;
    accum_duration = TRUE;
    to_vict = "You feel your strength wither!";
    break;

  case SPELL_ARMOR:
    af[0].location = APPLY_AC;
    af[0].modifier = +2;   /* +2 AC in 5e terms */
    af[0].duration = 24;
    accum_duration = TRUE;
    to_vict = "You feel someone protecting you.";
    break;

  case SPELL_BLESS:
    af[0].location = APPLY_SAVE_WIS;
    af[0].modifier = +1;   /* bonus to Wisdom saves */
    af[0].duration = 6;

    /* Optional: extend to CHA saves if you want to emulate "morale" boost */
    af[1].location = APPLY_SAVE_CHA;
    af[1].modifier = +1;
    af[1].duration = 6;

    accum_duration = TRUE;
    to_vict = "You feel righteous.";
    break;

  case SPELL_BLINDNESS:
    if (MOB_FLAGGED(victim, MOB_NOBLIND) || GET_LEVEL(victim) >= LVL_IMMORT ||
        mag_savingthrow(victim, ABIL_CON, save_dc)) {
      send_to_char(ch, "You fail.\r\n");
      return;
    }

    af[0].location = APPLY_SAVE_DEX;   /* penalize Dex saves */
    af[0].modifier = -2;
    af[0].duration = 2;
    SET_BIT_AR(af[0].bitvector, AFF_BLIND);

    af[1].location = APPLY_AC;         /* easier to hit */
    af[1].modifier = +40;
    af[1].duration = 2;
    SET_BIT_AR(af[1].bitvector, AFF_BLIND);

    to_room = "$n seems to be blinded!";
    to_vict = "You have been blinded!";
    break;

  case SPELL_CURSE:
    if (mag_savingthrow(victim, ABIL_WIS, save_dc)) {
      send_to_char(ch, "%s", CONFIG_NOEFFECT);
      return;
    }

    af[0].location = APPLY_SAVE_WIS;    /* harder to resist magic */
    af[0].duration = 1 + (GET_LEVEL(ch) / 2);
    af[0].modifier = -1;
    SET_BIT_AR(af[0].bitvector, AFF_CURSE);

    af[1].location = APPLY_SAVE_CHA;    /* morale/charisma weakened */
    af[1].duration = 1 + (GET_LEVEL(ch) / 2);
    af[1].modifier = -1;
    SET_BIT_AR(af[1].bitvector, AFF_CURSE);

    accum_duration = TRUE;
    accum_affect = TRUE;
    to_room = "$n briefly glows red!";
    to_vict = "You feel very uncomfortable.";
    break;

  case SPELL_DETECT_INVIS:
    af[0].duration = 12 + level;
    SET_BIT_AR(af[0].bitvector, AFF_DETECT_INVIS);
    accum_duration = TRUE;
    to_vict = "Your eyes tingle.";
    break;

  case SPELL_DETECT_MAGIC:
    af[0].duration = 12 + level;
    SET_BIT_AR(af[0].bitvector, AFF_DETECT_MAGIC);
    accum_duration = TRUE;
    to_vict = "Your eyes tingle.";
    break;

  case SPELL_FLY:
    af[0].duration = 24;
    SET_BIT_AR(af[0].bitvector, AFF_FLYING);
    accum_duration = TRUE;
    to_vict = "You float above the ground.";
    break;

  case SPELL_INFRAVISION:
    af[0].duration = 12 + level;
    SET_BIT_AR(af[0].bitvector, AFF_INFRAVISION);
    accum_duration = TRUE;
    to_vict = "Your eyes glow red.";
    to_room = "$n's eyes glow red.";
    break;

  case SPELL_INVISIBLE:
    if (!victim)
      victim = ch;

    af[0].duration = 12 + (GET_LEVEL(ch) / 4);
    af[0].modifier = +2;   /* ascending AC: harder to hit */
    af[0].location = APPLY_AC;
    SET_BIT_AR(af[0].bitvector, AFF_INVISIBLE);
    accum_duration = TRUE;
    to_vict = "You vanish.";
    to_room = "$n slowly fades out of existence.";
    break;

  case SPELL_POISON:
    if (mag_savingthrow(victim, ABIL_CON, save_dc)) {
      send_to_char(ch, "%s", CONFIG_NOEFFECT);
      return;
    }

    af[0].location = APPLY_STR;
    af[0].duration = GET_LEVEL(ch);
    af[0].modifier = -2;
    SET_BIT_AR(af[0].bitvector, AFF_POISON);
    to_vict = "You feel very sick.";
    to_room = "$n gets violently ill!";
    break;

  case SPELL_SANCTUARY:
    af[0].duration = 4;
    SET_BIT_AR(af[0].bitvector, AFF_SANCTUARY);
    accum_duration = TRUE;
    to_vict = "A white aura momentarily surrounds you.";
    to_room = "$n is surrounded by a white aura.";
    break;

  case SPELL_SLEEP:
    if (MOB_FLAGGED(victim, MOB_NOSLEEP))
      return;
    if (mag_savingthrow(victim, ABIL_WIS, save_dc))
      return;

    af[0].duration = 4 + (GET_LEVEL(ch) / 4);
    SET_BIT_AR(af[0].bitvector, AFF_SLEEP);

    if (GET_POS(victim) > POS_SLEEPING) {
      send_to_char(victim, "You feel very sleepy...  Zzzz......\r\n");
      act("$n goes to sleep.", TRUE, victim, 0, 0, TO_ROOM);
      GET_POS(victim) = POS_SLEEPING;
    }
    break;

  case SPELL_STRENGTH:

    af[0].location = APPLY_STR;
    af[0].duration = (GET_LEVEL(ch) / 2) + 4;
    af[0].modifier = 2; /* +2 bonus static */
    accum_duration = TRUE;
    accum_affect = TRUE;
    to_vict = "You feel stronger!";
    break;

  case SPELL_SENSE_LIFE:
    to_vict = "You feel your awareness improve.";
    af[0].duration = GET_LEVEL(ch);
    SET_BIT_AR(af[0].bitvector, AFF_SENSE_LIFE);
    accum_duration = TRUE;
    break;

  case SPELL_WATERWALK:
    af[0].duration = 24;
    SET_BIT_AR(af[0].bitvector, AFF_WATERWALK);
    accum_duration = TRUE;
    to_vict = "You feel webbing between your toes.";
    break;
  }

  /* If this is a mob that has this affect set in its mob file, do not perform
   * the affect.  This prevents people from un-sancting mobs by sancting them
   * and waiting for it to fade, for example. */
  if (IS_NPC(victim) && !affected_by_spell(victim, spellnum)) {
    for (i = 0; i < MAX_SPELL_AFFECTS; i++) {
      for (j=1; j<NUM_AFF_FLAGS; j++) {
        if (IS_SET_AR(af[i].bitvector, j) && AFF_FLAGGED(victim, j)) {
          send_to_char(ch, "%s", CONFIG_NOEFFECT);
          return;
        }
      }
    }
  }

  /* If the victim is already affected by this spell, and the spell does not
   * have an accumulative effect, then fail the spell. */
  if (affected_by_spell(victim,spellnum) && !(accum_duration||accum_affect)) {
    send_to_char(ch, "%s", CONFIG_NOEFFECT);
    return;
  }

  for (i = 0; i < MAX_SPELL_AFFECTS; i++)
    if (af[i].bitvector[0] || af[i].bitvector[1] ||
        af[i].bitvector[2] || af[i].bitvector[3] ||
        (af[i].location != APPLY_NONE))
      affect_join(victim, af+i, accum_duration, FALSE, accum_affect, FALSE);

  if (to_vict != NULL)
    act(to_vict, FALSE, victim, 0, ch, TO_CHAR);
  if (to_room != NULL)
    act(to_room, TRUE, victim, 0, ch, TO_ROOM);
}

/* This function is used to provide services to mag_groups.  This function is
 * the one you should change to add new group spells. */
static void perform_mag_groups(int level, struct char_data *ch,
			struct char_data *tch, int spellnum, int savetype)
{
  switch (spellnum) {
    case SPELL_GROUP_HEAL:
    mag_points(level, ch, tch, SPELL_HEAL, savetype);
    break;
  case SPELL_GROUP_ARMOR:
    mag_affects(level, ch, tch, SPELL_ARMOR, savetype);
    break;
  case SPELL_GROUP_RECALL:
    spell_recall(level, ch, tch, NULL);
    break;
  }
}

/* Every spell that affects the group should run through here perform_mag_groups
 * contains the switch statement to send us to the right magic. Group spells
 * affect everyone grouped with the caster who is in the room, caster last. To
 * add new group spells, you shouldn't have to change anything in mag_groups.
 * Just add a new case to perform_mag_groups. */
void mag_groups(int level, struct char_data *ch, int spellnum, int savetype)
{
  struct char_data *tch;

  if (ch == NULL)
    return;

  if (!GROUP(ch))
    return;
    
  while ((tch = (struct char_data *) simple_list(GROUP(ch)->members)) != NULL) {
    if (IN_ROOM(tch) != IN_ROOM(ch))
      continue;
    if (tch == ch)
      continue;
    perform_mag_groups(level, ch, tch, spellnum, savetype);
  }
  perform_mag_groups(level, ch, ch, spellnum, savetype);
}


/* Mass spells affect every creature in the room except the caster. No spells
 * of this class currently implemented. */
void mag_masses(int level, struct char_data *ch, int spellnum, int savetype)
{
  struct char_data *tch, *tch_next;

  for (tch = world[IN_ROOM(ch)].people; tch; tch = tch_next) {
    tch_next = tch->next_in_room;
    if (tch == ch)
      continue;

    switch (spellnum) {
    }
  }
}

/* Every spell that affects an area (room) runs through here.  These are
 * generally offensive spells.  This calls mag_damage to do the actual damage.
 * All spells listed here must also have a case in mag_damage() in order for
 * them to work. Area spells have limited targets within the room. */
void mag_areas(int level, struct char_data *ch, int spellnum, int savetype)
{
  struct char_data *tch, *next_tch;
  const char *to_char = NULL, *to_room = NULL;

  if (ch == NULL)
    return;

  /* to add spells just add the message here plus an entry in mag_damage for
   * the damaging part of the spell.   */
  switch (spellnum) {
  case SPELL_EARTHQUAKE:
    to_char = "You gesture and the earth begins to shake all around you!";
    to_room ="$n gracefully gestures and the earth begins to shake violently!";
    break;
  }

  if (to_char != NULL)
    act(to_char, FALSE, ch, 0, 0, TO_CHAR);
  if (to_room != NULL)
    act(to_room, FALSE, ch, 0, 0, TO_ROOM);


  for (tch = world[IN_ROOM(ch)].people; tch; tch = next_tch) {
    next_tch = tch->next_in_room;

    /* The skips: 1: the caster
     *            2: immortals
     *            3: if no pk on this mud, skips over all players
     *            4: pets (charmed NPCs)
     *            5: other players in the same group (if the spell is 'violent') 
     *            6: Flying people if earthquake is the spell                         */
    if (tch == ch)
      continue;
    if (!IS_NPC(tch) && GET_LEVEL(tch) >= LVL_IMMORT)
      continue;
    if (!IS_NPC(ch) && IS_NPC(tch) && AFF_FLAGGED(tch, AFF_CHARM))
      continue;
    if (!IS_NPC(tch) && spell_info[spellnum].violent && GROUP(tch) && GROUP(ch) && GROUP(ch) == GROUP(tch))
      continue;
	if ((spellnum == SPELL_EARTHQUAKE) && AFF_FLAGGED(tch, AFF_FLYING))
	  continue;
    /* Doesn't matter if they die here so we don't check. -gg 6/24/98 */
    mag_damage(level, ch, tch, spellnum, 1);
  }
}

/*----------------------------------------------------------------------------*/
/* Begin Magic Summoning - Generic Routines and Local Globals */
/*----------------------------------------------------------------------------*/

/* Every spell which summons/gates/conjours a mob comes through here. */
/* These use act(), don't put the \r\n. */
static const char *mag_summon_msgs[] = {
  "\r\n",
  "$n makes a strange magical gesture; you feel a strong breeze!",
  "$n animates a corpse!",
  "$N appears from a cloud of thick blue smoke!",
  "$N appears from a cloud of thick green smoke!",
  "$N appears from a cloud of thick red smoke!",
  "$N disappears in a thick black cloud!"
  "As $n makes a strange magical gesture, you feel a strong breeze.",
  "As $n makes a strange magical gesture, you feel a searing heat.",
  "As $n makes a strange magical gesture, you feel a sudden chill.",
  "As $n makes a strange magical gesture, you feel the dust swirl.",
  "$n magically divides!",
  "$n animates a corpse!"
};

/* Keep the \r\n because these use send_to_char. */
static const char *mag_summon_fail_msgs[] = {
  "\r\n",
  "There are no such creatures.\r\n",
  "Uh oh...\r\n",
  "Oh dear.\r\n",
  "Gosh durnit!\r\n",
  "The elements resist!\r\n",
  "You failed.\r\n",
  "There is no corpse!\r\n"
};

/* Defines for Mag_Summons */
#define MOB_CLONE            10   /**< vnum for the clone mob. */
#define OBJ_CLONE            161  /**< vnum for clone material. */
#define MOB_ZOMBIE           11   /**< vnum for the zombie mob. */

void mag_summons(int level, struct char_data *ch, struct obj_data *obj,
		      int spellnum, int savetype)
{
  struct char_data *mob = NULL;
  struct obj_data *tobj, *next_obj;
  int pfail = 0, msg = 0, fmsg = 0, num = 1, handle_corpse = FALSE, i;
  mob_vnum mob_num;

  if (ch == NULL)
    return;

  switch (spellnum) {
  case SPELL_CLONE:
    msg = 10;
    fmsg = rand_number(2, 6);	/* Random fail message. */
    mob_num = MOB_CLONE;
    /*
     * We have designated the clone spell as the example for how to use the
     * mag_materials function.
     * In stock tbaMUD it checks to see if the character has item with
     * vnum 161 which is a set of sacrificial entrails. If we have the entrails
     * the spell will succeed,  and if not, the spell will fail 102% of the time
     * (prevents random success... see below).
     * The object is extracted and the generic cast messages are displayed.
     */
    if( !mag_materials(ch, OBJ_CLONE, NOTHING, NOTHING, TRUE, TRUE) )
      pfail = 102; /* No materials, spell fails. */
    else
      pfail = 0;	/* We have the entrails, spell is successfully cast. */
    break;

  case SPELL_ANIMATE_DEAD:
    if (obj == NULL || !IS_CORPSE(obj)) {
      act(mag_summon_fail_msgs[7], FALSE, ch, 0, 0, TO_CHAR);
      return;
    }
    handle_corpse = TRUE;
    msg = 11;
    fmsg = rand_number(2, 6);	/* Random fail message. */
    mob_num = MOB_ZOMBIE;
    pfail = 10;	/* 10% failure, should vary in the future. */
    break;

  default:
    return;
  }

  if (AFF_FLAGGED(ch, AFF_CHARM)) {
    send_to_char(ch, "You are too giddy to have any followers!\r\n");
    return;
  }
  if (rand_number(0, 101) < pfail) {
    send_to_char(ch, "%s", mag_summon_fail_msgs[fmsg]);
    return;
  }
  for (i = 0; i < num; i++) {
    if (!(mob = read_mobile(mob_num, VIRTUAL))) {
      send_to_char(ch, "You don't quite remember how to make that creature.\r\n");
      return;
    }
    char_to_room(mob, IN_ROOM(ch));
    IS_CARRYING_W(mob) = 0;
    IS_CARRYING_N(mob) = 0;
    SET_BIT_AR(AFF_FLAGS(mob), AFF_CHARM);
    if (spellnum == SPELL_CLONE) {
      /* Don't mess up the prototype; use new string copies. */
      mob->player.name = strdup(GET_NAME(ch));
      mob->player.short_descr = strdup(GET_NAME(ch));
    }
    act(mag_summon_msgs[msg], FALSE, ch, 0, mob, TO_ROOM);
    load_mtrigger(mob);
    add_follower(mob, ch);
    
    if (GROUP(ch) && GROUP_LEADER(GROUP(ch)) == ch)
      join_group(mob, GROUP(ch));    
  }
  if (handle_corpse) {
    for (tobj = obj->contains; tobj; tobj = next_obj) {
      next_obj = tobj->next_content;
      obj_from_obj(tobj);
      obj_to_char(tobj, mob);
    }
    extract_obj(obj);
  }
}

/* Clean up the defines used for mag_summons. */
#undef MOB_CLONE
#undef OBJ_CLONE
#undef MOB_ZOMBIE

/*----------------------------------------------------------------------------*/
/* End Magic Summoning - Generic Routines and Local Globals */
/*----------------------------------------------------------------------------*/


void mag_points(int level, struct char_data *ch, struct char_data *victim,
		     int spellnum, int savetype)
{
  int healing = 0, move = 0;

  if (victim == NULL)
    return;

  switch (spellnum) {
  case SPELL_CURE_LIGHT:
    healing = dice(1, 8) + 1 + (level / 4);
    send_to_char(victim, "You feel better.\r\n");
    break;
  case SPELL_CURE_CRITIC:
    healing = dice(3, 8) + 3 + (level / 4);
    send_to_char(victim, "You feel a lot better!\r\n");
    break;
  case SPELL_HEAL:
    healing = 100 + dice(3, 8);
    send_to_char(victim, "A warm feeling floods your body.\r\n");
    break;
  }
  GET_HIT(victim) = MIN(GET_MAX_HIT(victim), GET_HIT(victim) + healing);
  GET_STAMINA(victim) = MIN(GET_MAX_STAMINA(victim), GET_STAMINA(victim) + move);
  update_pos(victim);
}

void mag_unaffects(int level, struct char_data *ch, struct char_data *victim,
		        int spellnum, int type)
{
  int spell = 0, msg_not_affected = TRUE;
  const char *to_vict = NULL, *to_room = NULL;

  if (victim == NULL)
    return;

  switch (spellnum) {
  case SPELL_HEAL:
    /* Heal also restores health, so don't give the "no effect" message if the
     * target isn't afflicted by the 'blindness' spell. */
    msg_not_affected = FALSE;
    /* fall-through */
  case SPELL_CURE_BLIND:
    spell = SPELL_BLINDNESS;
    to_vict = "Your vision returns!";
    to_room = "There's a momentary gleam in $n's eyes.";
    break;
  case SPELL_REMOVE_POISON:
    spell = SPELL_POISON;
    to_vict = "A warm feeling runs through your body!";
    to_room = "$n looks better.";
    break;
  case SPELL_REMOVE_CURSE:
    spell = SPELL_CURSE;
    to_vict = "You don't feel so unlucky.";
    break;
  default:
    log("SYSERR: unknown spellnum %d passed to mag_unaffects.", spellnum);
    return;
  }

  if (!affected_by_spell(victim, spell)) {
    if (msg_not_affected)
      send_to_char(ch, "%s", CONFIG_NOEFFECT);
    return;
  }

  affect_from_char(victim, spell);
  if (to_vict != NULL)
    act(to_vict, FALSE, victim, 0, ch, TO_CHAR);
  if (to_room != NULL)
    act(to_room, TRUE, victim, 0, ch, TO_ROOM);
}

void mag_alter_objs(int level, struct char_data *ch, struct obj_data *obj,
		         int spellnum, int savetype)
{
  const char *to_char = NULL, *to_room = NULL;

  if (obj == NULL)
    return;

  switch (spellnum) {
    case SPELL_BLESS:
      if (!OBJ_FLAGGED(obj, ITEM_BLESS) &&
	  (GET_OBJ_WEIGHT(obj) <= 5 * GET_LEVEL(ch))) {
	SET_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_BLESS);
	to_char = "$p glows briefly.";
      }
      break;
    case SPELL_CURSE:
      if (!OBJ_FLAGGED(obj, ITEM_NODROP)) {
	SET_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_NODROP);
	if (GET_OBJ_TYPE(obj) == ITEM_WEAPON)
	  GET_OBJ_VAL(obj, 2)--;
	to_char = "$p briefly glows red.";
      }
      break;
    case SPELL_INVISIBLE:
      if (!OBJ_FLAGGED(obj, ITEM_NOINVIS) && !OBJ_FLAGGED(obj, ITEM_INVISIBLE)) {
        SET_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_INVISIBLE);
        to_char = "$p vanishes.";
      }
      break;
    case SPELL_POISON:
      if (((GET_OBJ_TYPE(obj) == ITEM_DRINKCON) ||
         (GET_OBJ_TYPE(obj) == ITEM_FOUNTAIN) ||
         (GET_OBJ_TYPE(obj) == ITEM_FOOD)) && !GET_OBJ_VAL(obj, 3)) {
      GET_OBJ_VAL(obj, 3) = 1;
      to_char = "$p steams briefly.";
      }
      break;
    case SPELL_REMOVE_CURSE:
      if (OBJ_FLAGGED(obj, ITEM_NODROP)) {
        REMOVE_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_NODROP);
        if (GET_OBJ_TYPE(obj) == ITEM_WEAPON)
          GET_OBJ_VAL(obj, 2)++;
        to_char = "$p briefly glows blue.";
      }
      break;
    case SPELL_REMOVE_POISON:
      if (((GET_OBJ_TYPE(obj) == ITEM_DRINKCON) ||
         (GET_OBJ_TYPE(obj) == ITEM_FOUNTAIN) ||
         (GET_OBJ_TYPE(obj) == ITEM_FOOD)) && GET_OBJ_VAL(obj, 3)) {
        GET_OBJ_VAL(obj, 3) = 0;
        to_char = "$p steams briefly.";
      }
      break;
  }

  if (to_char == NULL)
    send_to_char(ch, "%s", CONFIG_NOEFFECT);
  else
    act(to_char, TRUE, ch, obj, 0, TO_CHAR);

  if (to_room != NULL)
    act(to_room, TRUE, ch, obj, 0, TO_ROOM);
  else if (to_char != NULL)
    act(to_char, TRUE, ch, obj, 0, TO_ROOM);
}

void mag_creations(int level, struct char_data *ch, int spellnum)
{
  struct obj_data *tobj;
  obj_vnum z;

  if (ch == NULL)
    return;
  /* level = MAX(MIN(level, LVL_IMPL), 1); - Hm, not used. */

  switch (spellnum) {
  case SPELL_CREATE_FOOD:
    z = 10;
    break;
  default:
    send_to_char(ch, "Spell unimplemented, it would seem.\r\n");
    return;
  }

  if (!(tobj = read_object(z, VIRTUAL))) {
    send_to_char(ch, "I seem to have goofed.\r\n");
    log("SYSERR: spell_creations, spell %d, obj %d: obj not found",
	    spellnum, z);
    return;
  }
  obj_to_char(tobj, ch);
  act("$n creates $p.", FALSE, ch, tobj, 0, TO_ROOM);
  act("You create $p.", FALSE, ch, tobj, 0, TO_CHAR);
  load_otrigger(tobj);
}

void mag_rooms(int level, struct char_data *ch, int spellnum)
{
  room_rnum rnum;
  int duration = 0;
  bool failure = FALSE;
  event_id IdNum = eNULL;
  const char *msg = NULL;
  const char *room = NULL;
  
  rnum = IN_ROOM(ch);
  
  if (ROOM_FLAGGED(rnum, ROOM_NOMAGIC))
    failure = TRUE;
  
  switch (spellnum) {
    case SPELL_DARKNESS:
      IdNum = eSPL_DARKNESS;
      if (ROOM_FLAGGED(rnum, ROOM_DARK))
        failure = TRUE;
        
      duration = 5;
      SET_BIT_AR(ROOM_FLAGS(rnum), ROOM_DARK);
        
      msg = "You cast a shroud of darkness upon the area.";
      room = "$n casts a shroud of darkness upon this area.";
    break;
  
  }
  
  if (failure || IdNum == eNULL) {
    send_to_char(ch, "You failed!\r\n");
    return;
  }
  
  send_to_char(ch, "%s\r\n", msg);
  act(room, FALSE, ch, 0, 0, TO_ROOM);
  
  NEW_EVENT(eSPL_DARKNESS, &world[rnum], NULL, duration * PASSES_PER_SEC);
}
