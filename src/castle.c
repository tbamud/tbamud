/* ************************************************************************
*   File: castle.c                                      Part of CircleMUD *
*  Usage: Special procedures for King's Castle area                       *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Special procedures for Kings Castle by Pjotr (d90-pem@nada.kth.se)     *
*  Coded by Sapowox (d90-jkr@nada.kth.se)                                 *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include "conf.h"
#include "sysdep.h"


#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"


/* IMPORTANT!
   The below defined number is the zone number of the Kings Castle.
   Change it to apply to your chosen zone number. The default zone
   number (On Alex and Alfa) is 80 (That is rooms and mobs have numbers
   in the 8000 series... */

#define Z_KINGS_C 150


/* external variables */
extern struct time_info_data time_info;
extern int mini_mud;

/* local functions */
mob_vnum castle_virtual(mob_vnum offset);
room_rnum castle_real_room(room_vnum roomoffset);
struct char_data *find_npc_by_name(struct char_data *chAtChar, const char *pszName, int iLen);
int block_way(struct char_data *ch, int cmd, char *arg, room_vnum iIn_room, int iProhibited_direction);
void assign_kings_castle(void);
int member_of_staff(struct char_data *chChar);
int member_of_royal_guard(struct char_data *chChar);
struct char_data *find_guard(struct char_data *chAtChar);
struct char_data *get_victim(struct char_data *chAtChar);
int banzaii(struct char_data *ch);
int do_npc_rescue(struct char_data *ch_hero, struct char_data *ch_victim);
int is_trash(struct obj_data *i);
void fry_victim(struct char_data *ch);
int castle_cleaner(struct char_data *ch, int cmd, int gripe);
int castle_twin_proc(struct char_data *ch, int cmd, char *arg, int ctlnum, const char *twinname);
void castle_mob_spec(mob_vnum mobnum, SPECIAL(*specproc));


/**********************************************************************\
|* Special procedures for Kings Castle by Pjotr (d90-pem@nada.kth.se) *|
|* Coded by Sapowox (d90-jkr@nada.kth.se)                             *|
\**********************************************************************/

SPECIAL(CastleGuard);
SPECIAL(James);
SPECIAL(cleaning);
SPECIAL(DicknDavid);
SPECIAL(tim);
SPECIAL(tom);
SPECIAL(king_welmar);
SPECIAL(training_master);
SPECIAL(peter);
SPECIAL(jerry);
SPECIAL(guild);
ACMD(do_gen_door);
ACMD(do_follow);

/*
 * Assign castle special procedures.
 *
 * NOTE: The mobile number isn't fully specified. It's only an offset
 *	from the zone's base.
 */
void castle_mob_spec(mob_vnum mobnum, SPECIAL(*specproc))
{
  mob_vnum vmv = castle_virtual(mobnum);
  mob_rnum rmr = NOBODY;

  if (vmv != NOBODY)
    rmr = real_mobile(vmv);

  if (rmr == NOBODY) {
    if (!mini_mud)
      log("SYSERR: assign_kings_castle(): can't find mob #%d.", vmv);
  } else
    mob_index[rmr].func = specproc;
}


mob_vnum castle_virtual(mob_vnum offset)
{
  zone_rnum zon;

  if ((zon = real_zone(Z_KINGS_C)) == NOWHERE)
    return NOBODY;

  return zone_table[zon].bot + offset;
}


room_rnum castle_real_room(room_vnum roomoffset)
{
  zone_rnum zon;

  if ((zon = real_zone(Z_KINGS_C)) == NOWHERE)
    return NOWHERE;

  return real_room(zone_table[zon].bot + roomoffset);
}


/*
 * Routine: assign_kings_castle
 *
 * Used to assign function pointers to all mobiles in the Kings Castle.
 * Called from spec_assign.c.
 */
void assign_kings_castle(void)
{
  castle_mob_spec(0, CastleGuard);	/* Gwydion */
  /* Added the previous line -- Furry */
  castle_mob_spec(1, king_welmar);	/* Our dear friend, the King */
  castle_mob_spec(3, CastleGuard);	/* Jim */
  castle_mob_spec(4, CastleGuard);	/* Brian */
  castle_mob_spec(5, CastleGuard);	/* Mick */
  castle_mob_spec(6, CastleGuard);	/* Matt */
  castle_mob_spec(7, CastleGuard);	/* Jochem */
  castle_mob_spec(8, CastleGuard);	/* Anne */
  castle_mob_spec(9, CastleGuard);	/* Andrew */
  castle_mob_spec(10, CastleGuard);	/* Bertram */
  castle_mob_spec(11, CastleGuard);	/* Jeanette */
  castle_mob_spec(12, peter);		/* Peter */
  castle_mob_spec(13, training_master);	/* The training master */
  castle_mob_spec(16, James);		/* James the Butler */
  castle_mob_spec(17, cleaning);	/* Ze Cleaning Fomen */
  castle_mob_spec(20, tim);		/* Tim, Tom's twin */
  castle_mob_spec(21, tom);		/* Tom, Tim's twin */
  castle_mob_spec(24, DicknDavid);	/* Dick, guard of the
					 * Treasury */
  castle_mob_spec(25, DicknDavid);	/* David, Dicks brother */
  castle_mob_spec(26, jerry);		/* Jerry, the Gambler */
  castle_mob_spec(27, CastleGuard);	/* Michael */
  castle_mob_spec(28, CastleGuard);	/* Hans */
  castle_mob_spec(29, CastleGuard);	/* Boris */
}


/*
 * Routine: member_of_staff
 *
 * Used to see if a character is a member of the castle staff.
 * Used mainly by BANZAI:ng NPC:s.
 */
int member_of_staff(struct char_data *chChar)
{
  int ch_num;

  if (!IS_NPC(chChar))
    return (FALSE);

  ch_num = GET_MOB_VNUM(chChar);

  if (ch_num == castle_virtual(1))
    return (TRUE);

  if (ch_num > castle_virtual(2) && ch_num < castle_virtual(15))
    return (TRUE);

  if (ch_num > castle_virtual(15) && ch_num < castle_virtual(18))
    return (TRUE);

  if (ch_num > castle_virtual(18) && ch_num < castle_virtual(30))
    return (TRUE);

  return (FALSE);
}


/*
 * Function: member_of_royal_guard
 *
 * Returns TRUE if the character is a guard on duty, otherwise FALSE.
 * Used by Peter the captain of the royal guard.
 */
int member_of_royal_guard(struct char_data *chChar)
{
  int ch_num;

  if (!chChar || !IS_NPC(chChar))
    return (FALSE);

  ch_num = GET_MOB_VNUM(chChar);

  if (ch_num == castle_virtual(3) || ch_num == castle_virtual(6))
    return (TRUE);

  if (ch_num > castle_virtual(7) && ch_num < castle_virtual(12))
    return (TRUE);

  if (ch_num > castle_virtual(23) && ch_num < castle_virtual(26))
    return (TRUE);

  return (FALSE);
}


/*
 * Function: find_npc_by_name
 *
 * Returns a pointer to an npc by the given name.
 * Used by Tim and Tom
 */
struct char_data *find_npc_by_name(struct char_data *chAtChar,
		const char *pszName, int iLen)
{
  struct char_data *ch;

  for (ch = world[IN_ROOM(chAtChar)].people; ch; ch = ch->next_in_room)
    if (IS_NPC(ch) && !strncmp(pszName, ch->player.short_descr, iLen))
      return (ch);

  return (NULL);
}


/*
 * Function: find_guard
 *
 * Returns the pointer to a guard on duty.
 * Used by Peter the Captain of the Royal Guard
 */
struct char_data *find_guard(struct char_data *chAtChar)
{
  struct char_data *ch;

  for (ch = world[IN_ROOM(chAtChar)].people; ch; ch = ch->next_in_room)
    if (!FIGHTING(ch) && member_of_royal_guard(ch))
      return (ch);

  return (NULL);
}


/*
 * Function: get_victim
 *
 * Returns a pointer to a randomly chosen character in the same room,
 * fighting someone in the castle staff...
 * Used by BANZAII-ing characters and King Welmar...
 */
struct char_data *get_victim(struct char_data *chAtChar)
{
  struct char_data *ch;
  int iNum_bad_guys = 0, iVictim;

  for (ch = world[IN_ROOM(chAtChar)].people; ch; ch = ch->next_in_room)
    if (FIGHTING(ch) && member_of_staff(FIGHTING(ch)))
      iNum_bad_guys++;

  if (!iNum_bad_guys)
    return (NULL);

  iVictim = rand_number(0, iNum_bad_guys);	/* How nice, we give them a chance */
  if (!iVictim)
    return (NULL);

  iNum_bad_guys = 0;

  for (ch = world[IN_ROOM(chAtChar)].people; ch; ch = ch->next_in_room) {
    if (FIGHTING(ch) == NULL)
      continue;

    if (!member_of_staff(FIGHTING(ch)))
      continue;

    if (++iNum_bad_guys != iVictim)
      continue;

    return (ch);
  }

  return (NULL);
}


/*
 * Function: banzaii
 *
 * Makes a character banzaii on attackers of the castle staff.
 * Used by Guards, Tim, Tom, Dick, David, Peter, Master, King and Guards.
 */
int banzaii(struct char_data *ch)
{
  struct char_data *chOpponent;

  if (!AWAKE(ch) || GET_POS(ch) == POS_FIGHTING || !(chOpponent = get_victim(ch)))
    return (FALSE);

  act("$n roars: 'Protect the Kingdom of Great King Welmar!  BANZAIIII!!!'",
	FALSE, ch, 0, 0, TO_ROOM);
  hit(ch, chOpponent, TYPE_UNDEFINED);
  return (TRUE);
}


/*
 * Function: do_npc_rescue
 *
 * Makes ch_hero rescue ch_victim.
 * Used by Tim and Tom
 */
int do_npc_rescue(struct char_data *ch_hero, struct char_data *ch_victim)
{
  struct char_data *ch_bad_guy;

  for (ch_bad_guy = world[IN_ROOM(ch_hero)].people;
       ch_bad_guy && (FIGHTING(ch_bad_guy) != ch_victim);
       ch_bad_guy = ch_bad_guy->next_in_room);

  /* NO WAY I'll rescue the one I'm fighting! */
  if (!ch_bad_guy || ch_bad_guy == ch_hero)
    return (FALSE);

  act("You bravely rescue $N.\r\n", FALSE, ch_hero, 0, ch_victim, TO_CHAR);
  act("You are rescued by $N, your loyal friend!\r\n",
	FALSE, ch_victim, 0, ch_hero, TO_CHAR);
  act("$n heroically rescues $N.", FALSE, ch_hero, 0, ch_victim, TO_NOTVICT);

  if (FIGHTING(ch_bad_guy))
    stop_fighting(ch_bad_guy);
  if (FIGHTING(ch_hero))
    stop_fighting(ch_hero);

  set_fighting(ch_hero, ch_bad_guy);
  set_fighting(ch_bad_guy, ch_hero);
  return (TRUE);
}


/*
 * Procedure to block a person trying to enter a room.
 * Used by Tim/Tom at Kings bedroom and Dick/David at treasury.
 */
int block_way(struct char_data *ch, int cmd, char *arg, room_vnum iIn_room,
	          int iProhibited_direction)
{
  if (cmd != ++iProhibited_direction)
    return (FALSE);

  if (ch->player.short_descr && !strncmp(ch->player.short_descr, "King Welmar", 11))
    return (FALSE);

  if (IN_ROOM(ch) != real_room(iIn_room))
    return (FALSE);

  if (!member_of_staff(ch))
    act("The guard roars at $n and pushes $m back.", FALSE, ch, 0, 0, TO_ROOM);

  send_to_char(ch, "The guard roars: 'Entrance is Prohibited!', and pushes you back.\r\n");
  return (TRUE);
}


/*
 * Routine to check if an object is trash...
 * Used by James the Butler and the Cleaning Lady.
 */
int is_trash(struct obj_data *i)
{
  if (!OBJWEAR_FLAGGED(i, ITEM_WEAR_TAKE))
    return (FALSE);

  if (GET_OBJ_TYPE(i) == ITEM_DRINKCON || GET_OBJ_COST(i) <= 10)
    return (TRUE);

  return (FALSE);
}


/*
 * Function: fry_victim
 *
 * Finds a suitabe victim, and cast some _NASTY_ spell on him.
 * Used by King Welmar
 */
void fry_victim(struct char_data *ch)
{
  struct char_data *tch;

  if (ch->points.mana < 10)
    return;

  /* Find someone suitable to fry ! */
  if (!(tch = get_victim(ch)))
    return;

  switch (rand_number(0, 8)) {
  case 1:
  case 2:
  case 3:
    send_to_char(ch, "You raise your hand in a dramatical gesture.\r\n");
    act("$n raises $s hand in a dramatical gesture.", 1, ch, 0, 0, TO_ROOM);
    cast_spell(ch, tch, 0, SPELL_COLOR_SPRAY);
    break;
  case 4:
  case 5:
    send_to_char(ch, "You concentrate and mumble to yourself.\r\n");
    act("$n concentrates, and mumbles to $mself.", 1, ch, 0, 0, TO_ROOM);
    cast_spell(ch, tch, 0, SPELL_HARM);
    break;
  case 6:
  case 7:
    act("You look deeply into the eyes of $N.", 1, ch, 0, tch, TO_CHAR);
    act("$n looks deeply into the eyes of $N.", 1, ch, 0, tch, TO_NOTVICT);
    act("You see an ill-boding flame in the eye of $n.", 1, ch, 0, tch, TO_VICT);
    cast_spell(ch, tch, 0, SPELL_FIREBALL);
    break;
  default:
    if (!rand_number(0, 1))
      cast_spell(ch, ch, 0, SPELL_HEAL);
    break;
  }

  ch->points.mana -= 10;

  return;
}


/*
 * Function: king_welmar
 *
 * Control the actions and movements of the King.
 * Used by King Welmar.
 */
SPECIAL(king_welmar)
{
  char actbuf[MAX_INPUT_LENGTH];

  const char *monolog[] = {
    "$n proclaims 'Primus in regnis Geticis coronam'.",
    "$n proclaims 'regiam gessi, subiique regis'.",
    "$n proclaims 'munus et mores colui sereno'.",
    "$n proclaims 'principe dignos'."
  };

  const char bedroom_path[] = "s33004o1c1S.";
  const char throne_path[] = "W3o3cG52211rg.";
  const char monolog_path[] = "ABCDPPPP.";

  static const char *path;
  static int path_index;
  static bool move = FALSE;

  if (!move) {
    if (time_info.hours == 8 && IN_ROOM(ch) == castle_real_room(51)) {
      move = TRUE;
      path = throne_path;
      path_index = 0;
    } else if (time_info.hours == 21 && IN_ROOM(ch) == castle_real_room(17)) {
      move = TRUE;
      path = bedroom_path;
      path_index = 0;
    } else if (time_info.hours == 12 && IN_ROOM(ch) == castle_real_room(17)) {
      move = TRUE;
      path = monolog_path;
      path_index = 0;
    }
  }
  if (cmd || (GET_POS(ch) < POS_SLEEPING) ||
      (GET_POS(ch) == POS_SLEEPING && !move))
    return (FALSE);

  if (GET_POS(ch) == POS_FIGHTING) {
    fry_victim(ch);
    return (FALSE);
  } else if (banzaii(ch))
    return (FALSE);

  if (!move)
    return (FALSE);

  switch (path[path_index]) {
  case '0':
  case '1':
  case '2':
  case '3':
  case '4':
  case '5':
    perform_move(ch, path[path_index] - '0', 1);
    break;

  case 'A':
  case 'B':
  case 'C':
  case 'D':
    act(monolog[path[path_index] - 'A'], FALSE, ch, 0, 0, TO_ROOM);
    break;

  case 'P':
    break;

  case 'W':
    GET_POS(ch) = POS_STANDING;
    act("$n awakens and stands up.", FALSE, ch, 0, 0, TO_ROOM);
    break;

  case 'S':
    GET_POS(ch) = POS_SLEEPING;
    act("$n lies down on $s beautiful bed and instantly falls asleep.", FALSE, ch, 0, 0, TO_ROOM);
    break;

  case 'r':
    GET_POS(ch) = POS_SITTING;
    act("$n sits down on $s great throne.", FALSE, ch, 0, 0, TO_ROOM);
    break;

  case 's':
    GET_POS(ch) = POS_STANDING;
    act("$n stands up.", FALSE, ch, 0, 0, TO_ROOM);
    break;

  case 'G':
    act("$n says 'Good morning, trusted friends.'", FALSE, ch, 0, 0, TO_ROOM);
    break;

  case 'g':
    act("$n says 'Good morning, dear subjects.'", FALSE, ch, 0, 0, TO_ROOM);
    break;

  case 'o':
    do_gen_door(ch, strcpy(actbuf, "door"), 0, SCMD_UNLOCK);	/* strcpy: OK */
    do_gen_door(ch, strcpy(actbuf, "door"), 0, SCMD_OPEN);	/* strcpy: OK */
    break;

  case 'c':
    do_gen_door(ch, strcpy(actbuf, "door"), 0, SCMD_CLOSE);	/* strcpy: OK */
    do_gen_door(ch, strcpy(actbuf, "door"), 0, SCMD_LOCK);	/* strcpy: OK */
    break;

  case '.':
    move = FALSE;
    break;
  }

  path_index++;
  return (FALSE);
}


/*
 * Function: training_master
 *
 * Acts actions to the training room, if his students are present.
 * Also allowes warrior-class to practice.
 * Used by the Training Master.
 */
SPECIAL(training_master)
{
  struct char_data *pupil1, *pupil2 = NULL, *tch;

  if (!AWAKE(ch) || (GET_POS(ch) == POS_FIGHTING))
    return (FALSE);

  if (cmd)
    return (FALSE);

  if (banzaii(ch) || rand_number(0, 2))
    return (FALSE);

  if (!(pupil1 = find_npc_by_name(ch, "Brian", 5)))
    return (FALSE);

  if (!(pupil2 = find_npc_by_name(ch, "Mick", 4)))
    return (FALSE);

  if (FIGHTING(pupil1) || FIGHTING(pupil2))
    return (FALSE);

  if (rand_number(0, 1)) {
    tch = pupil1;
    pupil1 = pupil2;
    pupil2 = tch;
  }

  switch (rand_number(0, 7)) {
  case 0:
    act("$n hits $N on $s head with a powerful blow.", FALSE, pupil1, 0, pupil2, TO_NOTVICT);
    act("You hit $N on $s head with a powerful blow.", FALSE, pupil1, 0, pupil2, TO_CHAR);
    act("$n hits you on your head with a powerful blow.", FALSE, pupil1, 0, pupil2, TO_VICT);
    break;

  case 1:
    act("$n hits $N in $s chest with a thrust.", FALSE, pupil1, 0, pupil2, TO_NOTVICT);
    act("You manage to thrust $N in the chest.", FALSE, pupil1, 0, pupil2, TO_CHAR);
    act("$n manages to thrust you in your chest.", FALSE, pupil1, 0, pupil2, TO_VICT);
    break;

  case 2:
    send_to_char(ch, "You command your pupils to bow.\r\n");
    act("$n commands $s pupils to bow.", FALSE, ch, 0, 0, TO_ROOM);
    act("$n bows before $N.", FALSE, pupil1, 0, pupil2, TO_NOTVICT);
    act("$N bows before $n.", FALSE, pupil1, 0, pupil2, TO_NOTVICT);
    act("You bow before $N, who returns your gesture.", FALSE, pupil1, 0, pupil2, TO_CHAR);
    act("You bow before $n, who returns your gesture.", FALSE, pupil1, 0, pupil2, TO_VICT);
    break;

  case 3:
    act("$N yells at $n, as he fumbles and drops $s sword.", FALSE, pupil1, 0, ch, TO_NOTVICT);
    act("$n quickly picks up $s weapon.", FALSE, pupil1, 0, 0, TO_ROOM);
    act("$N yells at you, as you fumble, losing your weapon.", FALSE, pupil1, 0, ch, TO_CHAR);
    send_to_char(pupil1, "You quickly pick up your weapon again.\r\n");
    act("You yell at $n, as he fumbles, losing $s weapon.", FALSE, pupil1, 0, ch, TO_VICT);
    break;

  case 4:
    act("$N tricks $n, and slashes him across the back.", FALSE, pupil1, 0, pupil2, TO_NOTVICT);
    act("$N tricks you, and slashes you across your back.", FALSE, pupil1, 0, pupil2, TO_CHAR);
    act("You trick $n, and quickly slash him across $s back.", FALSE, pupil1, 0, pupil2, TO_VICT);
    break;

  case 5:
    act("$n lunges a blow at $N but $N parries skillfully.", FALSE, pupil1, 0, pupil2, TO_NOTVICT);
    act("You lunge a blow at $N but $E parries skillfully.", FALSE, pupil1, 0, pupil2, TO_CHAR);
    act("$n lunges a blow at you, but you skillfully parry it.", FALSE, pupil1, 0, pupil2, TO_VICT);
    break;

  case 6:
    act("$n clumsily tries to kick $N, but misses.", FALSE, pupil1, 0, pupil2, TO_NOTVICT);
    act("You clumsily miss $N with your poor excuse for a kick.", FALSE, pupil1, 0, pupil2, TO_CHAR);
    act("$n fails an unusually clumsy attempt at kicking you.", FALSE, pupil1, 0, pupil2, TO_VICT);
    break;

  default:
    send_to_char(ch, "You show your pupils an advanced technique.\r\n");
    act("$n shows $s pupils an advanced technique.", FALSE, ch, 0, 0, TO_ROOM);
    break;
  }

  return (FALSE);
}


SPECIAL(tom)
{
  return castle_twin_proc(ch, cmd, argument, 48, "Tim");
}

SPECIAL(tim)
{
  return castle_twin_proc(ch, cmd, argument, 49, "Tom");
}

/*
 * Common routine for the Castle Twins.
 */
int castle_twin_proc(struct char_data *ch, int cmd, char *arg, int ctlnum, const char *twinname)
{
  struct char_data *king, *twin;

  if (!AWAKE(ch))
    return (FALSE);

  if (cmd)
    return block_way(ch, cmd, arg, castle_virtual(ctlnum), 1);

  if ((king = find_npc_by_name(ch, "King Welmar", 11)) != NULL) {
    char actbuf[MAX_INPUT_LENGTH];

    if (!ch->master)
      do_follow(ch, strcpy(actbuf, "King Welmar"), 0, 0);	/* strcpy: OK */
    if (FIGHTING(king))
      do_npc_rescue(ch, king);
  }

  if ((twin = find_npc_by_name(ch, twinname, strlen(twinname))) != NULL)
    if (FIGHTING(twin) && 2 * GET_HIT(twin) < GET_HIT(ch))
      do_npc_rescue(ch, twin);

  if (GET_POS(ch) != POS_FIGHTING)
    banzaii(ch);

  return (FALSE);
}


/*
 * Routine for James the Butler.
 * Complains if he finds any trash...
 *
 * This doesn't make sure he _can_ carry it...
 */
SPECIAL(James)
{
  return castle_cleaner(ch, cmd, TRUE);
}

/*
 * Common code for James and the Cleaning Woman.
 */
int castle_cleaner(struct char_data *ch, int cmd, int gripe)
{
  struct obj_data *i;

  if (cmd || !AWAKE(ch) || GET_POS(ch) == POS_FIGHTING)
    return (FALSE);

  for (i = world[IN_ROOM(ch)].contents; i; i = i->next_content) {
    if (!is_trash(i))
      continue;

    if (gripe) {
      act("$n says: 'My oh my!  I ought to fire that lazy cleaning woman!'",
          FALSE, ch, 0, 0, TO_ROOM);
      act("$n picks up a piece of trash.", FALSE, ch, 0, 0, TO_ROOM);
    }
    obj_from_room(i);
    obj_to_char(i, ch);
    return (TRUE);
  }

  return (FALSE);
}


/*
 * Routine for the Cleaning Woman.
 * Picks up any trash she finds...
 */
SPECIAL(cleaning)
{
  return castle_cleaner(ch, cmd, FALSE);
}


/*
 * Routine: CastleGuard
 *
 * Standard routine for ordinary castle guards.
 */
SPECIAL(CastleGuard)
{
  if (cmd || !AWAKE(ch) || (GET_POS(ch) == POS_FIGHTING))
    return (FALSE);

  return (banzaii(ch));
}


/*
 * Routine: DicknDave
 *
 * Routine for the guards Dick and David.
 */
SPECIAL(DicknDavid)
{
  if (!AWAKE(ch))
    return (FALSE);

  if (!cmd && GET_POS(ch) != POS_FIGHTING)
    banzaii(ch);

  return (block_way(ch, cmd, argument, castle_virtual(36), 1));
}


/*
 * Routine: peter
 * Routine for Captain of the Guards.
 */
SPECIAL(peter)
{
  struct char_data *ch_guard = NULL;

  if (cmd || !AWAKE(ch) || GET_POS(ch) == POS_FIGHTING)
    return (FALSE);

  if (banzaii(ch))
    return (FALSE);

  if (!(rand_number(0, 3)) && (ch_guard = find_guard(ch)))
    switch (rand_number(0, 5)) {
    case 0:
      act("$N comes sharply into attention as $n inspects $M.",
	  FALSE, ch, 0, ch_guard, TO_NOTVICT);
      act("$N comes sharply into attention as you inspect $M.",
	  FALSE, ch, 0, ch_guard, TO_CHAR);
      act("You go sharply into attention as $n inspects you.",
	  FALSE, ch, 0, ch_guard, TO_VICT);
      break;
    case 1:
      act("$N looks very small, as $n roars at $M.",
	  FALSE, ch, 0, ch_guard, TO_NOTVICT);
      act("$N looks very small as you roar at $M.",
	  FALSE, ch, 0, ch_guard, TO_CHAR);
      act("You feel very small as $N roars at you.",
	  FALSE, ch, 0, ch_guard, TO_VICT);
      break;
    case 2:
      act("$n gives $N some Royal directions.",
	  FALSE, ch, 0, ch_guard, TO_NOTVICT);
      act("You give $N some Royal directions.",
	  FALSE, ch, 0, ch_guard, TO_CHAR);
      act("$n gives you some Royal directions.",
	  FALSE, ch, 0, ch_guard, TO_VICT);
      break;
    case 3:
      act("$n looks at you.", FALSE, ch, 0, ch_guard, TO_VICT);
      act("$n looks at $N.", FALSE, ch, 0, ch_guard, TO_NOTVICT);
      act("$n growls: 'Those boots need polishing!'",
	  FALSE, ch, 0, ch_guard, TO_ROOM);
      act("You growl at $N.", FALSE, ch, 0, ch_guard, TO_CHAR);
      break;
    case 4:
      act("$n looks at you.", FALSE, ch, 0, ch_guard, TO_VICT);
      act("$n looks at $N.", FALSE, ch, 0, ch_guard, TO_NOTVICT);
      act("$n growls: 'Straighten that collar!'",
	  FALSE, ch, 0, ch_guard, TO_ROOM);
      act("You growl at $N.", FALSE, ch, 0, ch_guard, TO_CHAR);
      break;
    default:
      act("$n looks at you.", FALSE, ch, 0, ch_guard, TO_VICT);
      act("$n looks at $N.", FALSE, ch, 0, ch_guard, TO_NOTVICT);
      act("$n growls: 'That chain mail looks rusty!  CLEAN IT !!!'",
	  FALSE, ch, 0, ch_guard, TO_ROOM);
      act("You growl at $N.", FALSE, ch, 0, ch_guard, TO_CHAR);
      break;
    }

  return (FALSE);
}


/*
 * Procedure for Jerry and Michael in x08 of King's Castle.
 * Code by Sapowox modified by Pjotr.(Original code from Master)
 */
SPECIAL(jerry)
{
  struct char_data *gambler1, *gambler2 = NULL, *tch;

  if (!AWAKE(ch) || (GET_POS(ch) == POS_FIGHTING))
    return (FALSE);

  if (cmd)
    return (FALSE);

  if (banzaii(ch) || rand_number(0, 2))
    return (FALSE);

  if (!(gambler1 = ch))
    return (FALSE);

  if (!(gambler2 = find_npc_by_name(ch, "Michael", 7)))
    return (FALSE);

  if (FIGHTING(gambler1) || FIGHTING(gambler2))
    return (FALSE);

  if (rand_number(0, 1)) {
    tch = gambler1;
    gambler1 = gambler2;
    gambler2 = tch;
  }

  switch (rand_number(0, 5)) {
  case 0:
    act("$n rolls the dice and cheers loudly at the result.",
	    FALSE, gambler1, 0, gambler2, TO_NOTVICT);
    act("You roll the dice and cheer. GREAT!",
	    FALSE, gambler1, 0, gambler2, TO_CHAR);
    act("$n cheers loudly as $e rolls the dice.",
	    FALSE, gambler1, 0, gambler2, TO_VICT);
    break;
  case 1:
    act("$n curses the Goddess of Luck roundly as he sees $N's roll.",
	    FALSE, gambler1, 0, gambler2, TO_NOTVICT);
    act("You curse the Goddess of Luck as $N rolls.",
	    FALSE, gambler1, 0, gambler2, TO_CHAR);
    act("$n swears angrily. You are in luck!",
	    FALSE, gambler1, 0, gambler2, TO_VICT);
    break;
  case 2:
    act("$n sighs loudly and gives $N some gold.",
	    FALSE, gambler1, 0, gambler2, TO_NOTVICT);
    act("You sigh loudly at the pain of having to give $N some gold.",
	    FALSE, gambler1, 0, gambler2, TO_CHAR);
    act("$n sighs loudly as $e gives you your rightful win.",
	    FALSE, gambler1, 0, gambler2, TO_VICT);
    break;
  case 3:
    act("$n smiles remorsefully as $N's roll tops $s.",
	    FALSE, gambler1, 0, gambler2, TO_NOTVICT);
    act("You smile sadly as you see that $N beats you. Again.",
	    FALSE, gambler1, 0, gambler2, TO_CHAR);
    act("$n smiles remorsefully as your roll tops $s.",
	    FALSE, gambler1, 0, gambler2, TO_VICT);
    break;
  case 4:
    act("$n excitedly follows the dice with $s eyes.",
	    FALSE, gambler1, 0, gambler2, TO_NOTVICT);
    act("You excitedly follow the dice with your eyes.",
	    FALSE, gambler1, 0, gambler2, TO_CHAR);
    act("$n excitedly follows the dice with $s eyes.",
	    FALSE, gambler1, 0, gambler2, TO_VICT);
    break;
  default:
    act("$n says 'Well, my luck has to change soon', as he shakes the dice.",
	    FALSE, gambler1, 0, gambler2, TO_NOTVICT);
    act("You say 'Well, my luck has to change soon' and shake the dice.",
	    FALSE, gambler1, 0, gambler2, TO_CHAR);
    act("$n says 'Well, my luck has to change soon', as he shakes the dice.",
	    FALSE, gambler1, 0, gambler2, TO_VICT);
    break;
  }
  return (FALSE);
}
