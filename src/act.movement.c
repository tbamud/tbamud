/**************************************************************************
*  File: act.movement.c                                    Part of tbaMUD *
*  Usage: Movement commands, door handling, & sleep/rest/etc state.       *
*                                                                         *
*  All rights reserved.  See license complete information.                *
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
#include "house.h"
#include "constants.h"
#include "py_triggers.h"
#include "act.h"
#include "fight.h"
#include "oasis.h" /* for buildwalk */


/* local only functions */
/* do_simple_move utility functions */
static int has_boat(struct char_data *ch);
/* do_gen_door utility functions */
static int find_door(struct char_data *ch, const char *type, char *dir, const char *cmdname);
static int has_key(struct char_data *ch, obj_vnum key);
static void do_doorcmd(struct char_data *ch, struct obj_data *obj, int door, int scmd);
static int ok_pick(struct char_data *ch, obj_vnum keynum, int pickproof, int scmd);
/* furniture helpers */
static bool extract_furniture_token(struct char_data *ch, char *argument, char *out_token,
                                    size_t out_size, const char *verb_cap);
static struct obj_data *get_numbered_furniture(struct char_data *ch, int number);
static struct obj_data *find_furniture_target(struct char_data *ch, char *token,
                                              bool *used_number, int *ordinal);
static bool validate_furniture_use(struct char_data *ch, struct obj_data *furniture,
                                   int position_bit, const char *verb_inf,
                                   bool already_there);
static void attach_char_to_furniture(struct char_data *ch, struct obj_data *furniture);
static const char *position_gerund(int pos);
static void clear_mount_state(struct char_data *ch);
static bool mount_skill_check(struct char_data *ch, int dc);
static bool resolve_mounted_move(struct char_data *ch, struct char_data **mount);
static int count_hitched_mounts(struct char_data *ch);
static int max_hitched_mounts(struct char_data *ch);
static struct char_data *first_hitched_mount_in_room(struct char_data *ch);
static struct char_data *find_hitched_mount_for_pack(struct char_data *ch, struct obj_data *obj);


/* simple function to determine if char can walk on water */
static int has_boat(struct char_data *ch)
{
  struct obj_data *obj;
  int i;

  if (GET_LEVEL(ch) > LVL_IMMORT)
    return (1);

  if (AFF_FLAGGED(ch, AFF_WATERWALK) || AFF_FLAGGED(ch, AFF_FLYING))
    return (1);

  /* non-wearable boats in inventory will do it */
  for (obj = ch->carrying; obj; obj = obj->next_content)
    if (GET_OBJ_TYPE(obj) == ITEM_BOAT && (find_eq_pos(ch, obj, NULL) < 0))
      return (1);

  /* and any boat you're wearing will do it too */
  for (i = 0; i < NUM_WEARS; i++)
    if (GET_EQ(ch, i) && GET_OBJ_TYPE(GET_EQ(ch, i)) == ITEM_BOAT)
      return (1);

  return (0);
}

/* Simple function to determine if char can fly. */
static int has_flight(struct char_data *ch)
{
  struct obj_data *obj;
  int i;

  if (GET_LEVEL(ch) > LVL_IMMORT)
    return (1);

  if (AFF_FLAGGED(ch, AFF_FLYING))
    return (1);

  /* Non-wearable flying items in inventory will do it. */
  for (obj = ch->carrying; obj; obj = obj->next_content)
    if (OBJAFF_FLAGGED(obj, AFF_FLYING) && OBJAFF_FLAGGED(obj, AFF_FLYING))
      return (1);

  /* Any equipped objects with AFF_FLYING will do it too. */
  for (i = 0; i < NUM_WEARS; i++)
    if (GET_EQ(ch, i) && OBJAFF_FLAGGED(GET_EQ(ch, i), AFF_FLYING))
      return (1);

  return (0);
}

/* Simple function to determine if char can scuba. */
static int has_scuba(struct char_data *ch)
{
  struct obj_data *obj;
  int i;

  if (GET_LEVEL(ch) > LVL_IMMORT)
    return (1);

  if (AFF_FLAGGED(ch, AFF_SCUBA))
    return (1);

  /* Non-wearable scuba items in inventory will do it. */
  for (obj = ch->carrying; obj; obj = obj->next_content)
    if (OBJAFF_FLAGGED(obj, AFF_SCUBA) && (find_eq_pos(ch, obj, NULL) < 0))
      return (1);

  /* Any equipped objects with AFF_SCUBA will do it too. */
  for (i = 0; i < NUM_WEARS; i++)
    if (GET_EQ(ch, i) && OBJAFF_FLAGGED(GET_EQ(ch, i), AFF_SCUBA))
      return (1);

  return (0);
}

static bool extract_furniture_token(struct char_data *ch, char *argument, char *out_token,
                                    size_t out_size, const char *verb_cap)
{
  char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

  if (!argument || !*argument)
    return FALSE;

  two_arguments(argument, arg1, arg2);

  if (!*arg1) {
    send_to_char(ch, "%s where?\r\n", verb_cap);
    return FALSE;
  }

  if (is_abbrev(arg1, "at")) {
    if (!*arg2) {
      send_to_char(ch, "%s at what?\r\n", verb_cap);
      return FALSE;
    }
    strlcpy(out_token, arg2, out_size);
  } else {
    strlcpy(out_token, arg1, out_size);
  }

  return TRUE;
}

static struct obj_data *get_numbered_furniture(struct char_data *ch, int number)
{
  struct obj_data *obj;
  int count = 0;
  int allowed_positions = 0;

  if (number <= 0)
    return NULL;

  for (obj = world[IN_ROOM(ch)].contents; obj; obj = obj->next_content) {
    if (GET_OBJ_TYPE(obj) != ITEM_FURNITURE)
      continue;
    if (GET_OBJ_VAL(obj, VAL_FURN_CAPACITY) <= 0)
      continue;
    allowed_positions = GET_OBJ_VAL(obj, VAL_FURN_POSITIONS);
    if (allowed_positions > 0 && !(allowed_positions & (1 << 1)))
      continue;
    if (!CAN_SEE_OBJ(ch, obj))
      continue;
    count++;
    if (count == number)
      return obj;
  }
  return NULL;
}

static struct obj_data *find_furniture_target(struct char_data *ch, char *token,
                                              bool *used_number, int *ordinal)
{
  struct obj_data *furniture = NULL;
  bool number_used = FALSE;
  int ord = 0;

  if (token && *token && is_number(token)) {
    int which = atoi(token);
    furniture = get_numbered_furniture(ch, which);
    number_used = TRUE;
    ord = which;
  } else {
    furniture = get_obj_in_list_vis(ch, token, NULL, world[ch->in_room].contents);
  }

  if (used_number)
    *used_number = number_used;
  if (ordinal)
    *ordinal = ord;

  return furniture;
}

static bool validate_furniture_use(struct char_data *ch, struct obj_data *furniture,
                                   int position_bit, const char *verb_inf,
                                   bool already_there)
{
  int allowed_positions;

  if (!furniture)
    return FALSE;

  if (GET_OBJ_TYPE(furniture) != ITEM_FURNITURE) {
    send_to_char(ch, "You can't %s on that!\r\n", verb_inf);
    return FALSE;
  }

  allowed_positions = GET_OBJ_VAL(furniture, VAL_FURN_POSITIONS);
  if (allowed_positions > 0 && !(allowed_positions & position_bit)) {
    act("$p doesn't look comfortable for that.", TRUE, ch, furniture, 0, TO_CHAR);
    return FALSE;
  }

  if (!already_there &&
      GET_OBJ_VAL(furniture, VAL_FURN_MAX_OCC) >= GET_OBJ_VAL(furniture, VAL_FURN_CAPACITY)) {
    act("$p looks full.", TRUE, ch, furniture, 0, TO_CHAR);
    return FALSE;
  }

  return TRUE;
}

static void attach_char_to_furniture(struct char_data *ch, struct obj_data *furniture)
{
  struct char_data *tempch;

  if (!furniture)
    return;

  if (!OBJ_SAT_IN_BY(furniture))
    OBJ_SAT_IN_BY(furniture) = ch;
  for (tempch = OBJ_SAT_IN_BY(furniture); tempch != ch; tempch = NEXT_SITTING(tempch)) {
    if (NEXT_SITTING(tempch))
      continue;
    NEXT_SITTING(tempch) = ch;
  }

  SITTING(ch) = furniture;
  NEXT_SITTING(ch) = NULL;
  GET_OBJ_VAL(furniture, VAL_FURN_MAX_OCC) += 1;
}

static const char *position_gerund(int pos)
{
  switch (pos) {
  case POS_SLEEPING:
    return "sleeping";
  case POS_RESTING:
    return "resting";
  case POS_SITTING:
    return "sitting";
  case POS_STANDING:
    return "standing";
  default:
    return "using";
  }
}

static void clear_mount_state(struct char_data *ch)
{
  struct char_data *mount;

  if (!ch)
    return;

  mount = MOUNT(ch);
  if (mount && RIDDEN_BY(mount) == ch) {
    int rider_weight = GET_WEIGHT(ch) + IS_CARRYING_W(ch);
    IS_CARRYING_W(mount) = MAX(0, IS_CARRYING_W(mount) - rider_weight);
    RIDDEN_BY(mount) = NULL;
  }
  MOUNT(ch) = NULL;
  REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_MOUNTED);
}

static bool mount_skill_check(struct char_data *ch, int dc)
{
  int total = roll_skill_check(ch, SKILL_ANIMAL_HANDLING, 0, NULL);
  bool success = (total >= dc);

  gain_skill(ch, "animal handling", success);
  return success;
}

static bool resolve_mounted_move(struct char_data *ch, struct char_data **mount)
{
  struct char_data *mount_ch;

  if (!AFF_FLAGGED(ch, AFF_MOUNTED))
    return FALSE;

  mount_ch = MOUNT(ch);
  if (!mount_ch || IN_ROOM(mount_ch) != IN_ROOM(ch)) {
    clear_mount_state(ch);
    send_to_char(ch, "You aren't mounted on anything.\r\n");
    return FALSE;
  }

  if (!mount_skill_check(ch, 5)) {
    send_to_char(ch, "%s refuses to move.\r\n", get_char_sdesc(mount_ch));
    act("$n tries to urge $N forward, but $N refuses to move.",
        TRUE, ch, 0, mount_ch, TO_ROOM);
    return FALSE;
  }

  if (!mount_skill_check(ch, 3)) {
    int dam = dice(1, 8);

    send_to_char(ch, "You are thrown from %s!\r\n", get_char_sdesc(mount_ch));
    act("$n is thrown from $N!", TRUE, ch, 0, mount_ch, TO_ROOM);
    clear_mount_state(ch);
    GET_POS(ch) = POS_RESTING;
    WAIT_STATE(ch, PULSE_VIOLENCE);
    damage(ch, ch, dam, TYPE_SUFFERING);
    return FALSE;
  }

  if (mount)
    *mount = mount_ch;
  return TRUE;
}

static int count_hitched_mounts(struct char_data *ch)
{
  struct follow_type *follow;
  int count = 0;

  if (!ch)
    return 0;

  for (follow = ch->followers; follow; follow = follow->next) {
    if (HITCHED_TO(follow->follower) == ch &&
        MOB_FLAGGED(follow->follower, MOB_MOUNT))
      count++;
  }

  return count;
}

static int max_hitched_mounts(struct char_data *ch)
{
  int skill = GET_SKILL(ch, SKILL_ANIMAL_HANDLING);

  if (skill > 59)
    return 2;
  return 1;
}

static struct char_data *first_hitched_mount_in_room(struct char_data *ch)
{
  struct follow_type *follow;

  if (!ch)
    return NULL;

  for (follow = ch->followers; follow; follow = follow->next) {
    if (HITCHED_TO(follow->follower) == ch &&
        MOB_FLAGGED(follow->follower, MOB_MOUNT) &&
        IN_ROOM(follow->follower) == IN_ROOM(ch))
      return follow->follower;
  }

  return NULL;
}

static struct char_data *find_hitched_mount_for_pack(struct char_data *ch, struct obj_data *obj)
{
  struct follow_type *follow;

  if (!ch || !obj)
    return NULL;

  for (follow = ch->followers; follow; follow = follow->next) {
    struct char_data *mount = follow->follower;

    if (HITCHED_TO(mount) != ch || !MOB_FLAGGED(mount, MOB_MOUNT))
      continue;
    if (IN_ROOM(mount) != IN_ROOM(ch))
      continue;
    if ((IS_CARRYING_N(mount) + 1) > CAN_CARRY_N(mount))
      continue;
    if ((IS_CARRYING_W(mount) + GET_OBJ_WEIGHT(obj)) > CAN_CARRY_W(mount))
      continue;

    return mount;
  }

  return NULL;
}
/** Move a PC/NPC character from their current location to a new location. This
 * is the standard movement locomotion function that all normal walking
 * movement by characters should be sent through. This function also defines
 * the move cost of normal locomotion as:
 * ( (move cost for source room) + (move cost for destination) ) / 2
 *
 * @pre Function assumes that ch has no master controlling character, that
 * ch has no followers (in other words followers won't be moved by this
 * function) and that the direction traveled in is one of the valid, enumerated
 * direction.
 * @param ch The character structure to attempt to move.
 * @param dir The defined direction (NORTH, SOUTH, etc...) to attempt to
 * move into.
 * @param need_specials_check If TRUE will cause 
 * @retval int 1 for a successful move (ch is now in a new location)		
 * or 0 for a failed move (ch is still in the original location). */
int do_simple_move(struct char_data *ch, int dir, int need_specials_check)
{
  /* Begin Local variable definitions */
  /*---------------------------------------------------------------------*/
  /* Used in our special proc check. By default, we pass a NULL argument
   * when checking for specials */
  char spec_proc_args[MAX_INPUT_LENGTH] = "";
  /* The room the character is currently in and will move from... */
  room_rnum was_in = IN_ROOM(ch);
  /* ... and the room the character will move into. */
  room_rnum going_to = EXIT(ch, dir)->to_room;
  /* How many stamina points are required to travel from was_in to going_to.
   * We redefine this later when we need it. */
  int need_movement = 0;
  /* Character whose stamina is used for movement (mounts override). */
  struct char_data *stamina_ch = ch;
  bool mounted_move = FALSE;
  /* Contains the "leave" message to display to the was_in room. */
  char leave_message[SMALL_BUFSIZE];
  /*---------------------------------------------------------------------*/
  /* End Local variable definitions */


  /* Begin checks that can prevent a character from leaving the was_in room. */
  /* Future checks should be implemented within this section and return 0.   */
  /*---------------------------------------------------------------------*/
  /* Check for special routines that might activate because of the move and
   * also might prevent the movement. Special requires commands, so we pass
   * in the "command" equivalent of the direction (ie. North is '1' in the
   * command list, but NORTH is defined as '0').
   * Note -- only check if following; this avoids 'double spec-proc' bug */
  if (need_specials_check && special(ch, dir + 1, spec_proc_args))
    return 0;

  /* Leave Trigger Checks: Does a leave trigger block exit from the room? */
  if (!leave_mtrigger(ch, dir) || IN_ROOM(ch) != was_in) /* prevent teleport crashes */
    return 0;
  if (!leave_wtrigger(&world[IN_ROOM(ch)], ch, dir) || IN_ROOM(ch) != was_in) /* prevent teleport crashes */
    return 0;
  if (!leave_otrigger(&world[IN_ROOM(ch)], ch, dir) || IN_ROOM(ch) != was_in) /* prevent teleport crashes */
    return 0;

  /* Charm effect: Does it override the movement? */
  if (AFF_FLAGGED(ch, AFF_CHARM) && ch->master && was_in == IN_ROOM(ch->master))
  {
    send_to_char(ch, "The thought of leaving your master makes you weep.\r\n");
    act("$n bursts into tears.", FALSE, ch, 0, 0, TO_ROOM);
    return (0);
  }

  /* Water, No Swimming Rooms: Does the deep water prevent movement? */
  if ((SECT(was_in) == SECT_WATER_NOSWIM) ||
      (SECT(going_to) == SECT_WATER_NOSWIM))
  {
    if (!has_boat(ch))
    {
      send_to_char(ch, "You need a boat to go there.\r\n");
      return (0);
    }
  }

  /* Flying Required: Does lack of flying prevent movement? */
  if ((SECT(was_in) == SECT_FLYING) || (SECT(going_to) == SECT_FLYING))
  {
    if (!has_flight(ch))
    {
      send_to_char(ch, "You need to be flying to go there!\r\n");
      return (0);
    }
  }

  /* Underwater Room: Does lack of underwater breathing prevent movement? */
  if ((SECT(was_in) == SECT_UNDERWATER) || (SECT(going_to) == SECT_UNDERWATER))
  {
    if (!has_scuba(ch) && !IS_NPC(ch) && !PRF_FLAGGED(ch, PRF_NOHASSLE)) {
      send_to_char(ch, "You need to be able to breathe water to go there!\r\n");
      return (0);
    }
  }

  /* Houses: Can the player walk into the house? */
  if (ROOM_FLAGGED(was_in, ROOM_ATRIUM))
  {
    if (!House_can_enter(ch, GET_ROOM_VNUM(going_to)))
    {
      send_to_char(ch, "That's private property -- no trespassing!\r\n");
      return (0);
    }
  }

  /* Check zone level recommendations */
  if ((ZONE_MINLVL(GET_ROOM_ZONE(going_to)) != -1) && ZONE_MINLVL(GET_ROOM_ZONE(going_to)) > GET_LEVEL(ch)) {
    send_to_char(ch, "This zone is above your recommended level.\r\n");
  }

  /* Check zone flag restrictions */
  if (ZONE_FLAGGED(GET_ROOM_ZONE(going_to), ZONE_CLOSED)) {
    send_to_char(ch, "A mysterious barrier forces you back! That area is off-limits.\r\n");
    return (0);
  }
  if (ZONE_FLAGGED(GET_ROOM_ZONE(going_to), ZONE_NOIMMORT) && (GET_LEVEL(ch) >= LVL_IMMORT) && (GET_LEVEL(ch) < LVL_GRGOD)) {
    send_to_char(ch, "A mysterious barrier forces you back! That area is off-limits.\r\n");
    return (0);
  }

  /* Room Size Capacity: Is the room full of people already? */
  if (ROOM_FLAGGED(going_to, ROOM_TUNNEL) &&
      num_pc_in_room(&(world[going_to])) >= CONFIG_TUNNEL_SIZE)
  {
    if (CONFIG_TUNNEL_SIZE > 1)
      send_to_char(ch, "There isn't enough room for you to go there!\r\n");
    else
      send_to_char(ch, "There isn't enough room there for more than one person!\r\n");
    return (0);
  }

  /* Room Level Requirements: Is ch privileged enough to enter the room? */
  if (ROOM_FLAGGED(going_to, ROOM_GODROOM) && GET_LEVEL(ch) < LVL_GOD)
  {
    send_to_char(ch, "You aren't godly enough to use that room!\r\n");
    return (0);
  }

  /* All checks passed, nothing will prevent movement now other than lack of
   * stamina points. */
  /* stamina points needed is avg. move loss for src and destination sect type */
  need_movement = (movement_loss[SECT(was_in)] +
		   movement_loss[SECT(going_to)]) / 2;

  /* Move Point Requirement Check */
  if (AFF_FLAGGED(ch, AFF_MOUNTED) && MOUNT(ch) && IN_ROOM(MOUNT(ch)) == was_in)
  {
    stamina_ch = MOUNT(ch);
    mounted_move = TRUE;
  }

  if (GET_STAMINA(stamina_ch) < need_movement && (mounted_move || !IS_NPC(ch)))
  {
    if (need_specials_check && ch->master)
      send_to_char(ch, "You are too exhausted to follow.\r\n");
    else if (mounted_move)
      send_to_char(ch, "Your mount is too exhausted.\r\n");
    else
      send_to_char(ch, "You are too exhausted.\r\n");

    return (0);
  }

  /*---------------------------------------------------------------------*/
  /* End checks that can prevent a character from leaving the was_in room. */


  /* Begin: the leave operation. */
  /*---------------------------------------------------------------------*/
  /* If applicable, subtract movement cost. */
  if (GET_LEVEL(ch) < LVL_IMMORT && (mounted_move || !IS_NPC(ch)))
    GET_STAMINA(stamina_ch) -= need_movement;

  /* Generate the leave message and display to others in the was_in room. */
  if (AFF_FLAGGED(ch, AFF_SNEAK)) {
    stealth_process_room_movement(ch, was_in, dir, TRUE);
  } else {
    snprintf(leave_message, sizeof(leave_message), "$n leaves %s.", dirs[dir]);
    act(leave_message, TRUE, ch, 0, 0, TO_ROOM);
  }

  char_from_room(ch);
  char_to_room(ch, going_to);
  /*---------------------------------------------------------------------*/
  /* End: the leave operation. The character is now in the new room. */


  /* Begin: Post-move operations. */
  /*---------------------------------------------------------------------*/
  /* Post Move Trigger Checks: Check the new room for triggers.
   * Assumptions: The character has already truly left the was_in room. If
   * the entry trigger "prevents" movement into the room, it is the triggers
   * job to provide a message to the original was_in room. */
  if (!entry_mtrigger(ch) || !enter_wtrigger(&world[going_to], ch, dir)) {
    char_from_room(ch);
    char_to_room(ch, was_in);
    return 0;
  }

  /* Display arrival information to anyone in the destination room... */
  if (AFF_FLAGGED(ch, AFF_SNEAK))
    stealth_process_room_movement(ch, going_to, dir, FALSE);
  else
    act("$n has arrived.", TRUE, ch, 0, 0, TO_ROOM);

  /* ... and the room description to the character. */
  if (ch->desc != NULL)
    look_at_room(ch, 0);

  /* ... and Kill the player if the room is a death trap. */
  if (ROOM_FLAGGED(going_to, ROOM_DEATH) && GET_LEVEL(ch) < LVL_IMMORT)
  {
    mudlog(BRF, LVL_IMMORT, TRUE, "%s hit death trap #%d (%s)", GET_NAME(ch), GET_ROOM_VNUM(going_to), world[going_to].name);
    death_cry(ch);
    if (!IS_NPC(ch))
      GET_POS(ch) = POS_DEAD;
    extract_char(ch);
    return (0);
  }

  /* At this point, the character is safe and in the room. */
  /* Fire memory and greet triggers, check and see if the greet trigger
   * prevents movement, and if so, move the player back to the previous room. */
  entry_memory_mtrigger(ch);
  if (!greet_mtrigger(ch, dir))
  {
    char_from_room(ch);
    char_to_room(ch, was_in);
    look_at_room(ch, 0);
    /* Failed move, return a failure */
    return (0);
  }
  else
    greet_memory_mtrigger(ch);
  /*---------------------------------------------------------------------*/
  /* End: Post-move operations. */

  /* Only here is the move successful *and* complete. Return success for
   * calling functions to handle post move operations. */
  clear_custom_ldesc(ch);
  return (1);
}

int perform_move(struct char_data *ch, int dir, int need_specials_check)
{
  room_rnum was_in;
  struct follow_type *k, *next;
  struct char_data *mount = NULL;

  if (ch == NULL || dir < 0 || dir >= NUM_OF_DIRS || FIGHTING(ch))
    return (0);
  else if (!CONFIG_DIAGONAL_DIRS && IS_DIAGONAL(dir))
    send_to_char(ch, "Alas, you cannot go that way...\r\n");
  else if ((!EXIT(ch, dir) && !buildwalk(ch, dir)) || EXIT(ch, dir)->to_room == NOWHERE)
    send_to_char(ch, "Alas, you cannot go that way...\r\n");
  else if (EXIT_FLAGGED(EXIT(ch, dir), EX_CLOSED) && (GET_LEVEL(ch) < LVL_IMMORT || (!IS_NPC(ch) && !PRF_FLAGGED(ch, PRF_NOHASSLE)))) {
    if (EXIT(ch, dir)->keyword)
      send_to_char(ch, "The %s seems to be closed.\r\n", fname(EXIT(ch, dir)->keyword));
    else
      send_to_char(ch, "It seems to be closed.\r\n");
  } else {
    was_in = IN_ROOM(ch);
    if (AFF_FLAGGED(ch, AFF_MOUNTED) &&
        !resolve_mounted_move(ch, &mount))
      return (0);

    if (!do_simple_move(ch, dir, need_specials_check))
      return (0);

    if (mount && IN_ROOM(mount) == was_in) {
      char_from_room(mount);
      char_to_room(mount, IN_ROOM(ch));
    }

    if (!ch->followers)
      return (1);

    for (k = ch->followers; k; k = next) {
      next = k->next;
      if ((IN_ROOM(k->follower) == was_in) &&
	  (GET_POS(k->follower) >= POS_STANDING)) {
	act("You follow $N.\r\n", FALSE, k->follower, 0, ch, TO_CHAR);
	perform_move(k->follower, dir, 1);
      }
    }
    return (1);
  }
  return (0);
}

ACMD(do_move)
{
  /* These subcmd defines are mapped precisely to the direction defines. */
  perform_move(ch, subcmd, 0);
}

static int find_door(struct char_data *ch, const char *type, char *dir, const char *cmdname)
{
  int door;

  if (*dir) {			/* a direction was specified */
    if ((door = search_block(dir, dirs, FALSE)) == -1) { /* Partial Match */
      if ((door = search_block(dir, autoexits, FALSE)) == -1) { /* Check 'short' dirs too */
        send_to_char(ch, "That's not a direction.\r\n");
        return (-1);
      }
    }
    if (EXIT(ch, door)) {	/* Braces added according to indent. -gg */
      if (EXIT(ch, door)->keyword) {
        if (is_name(type, EXIT(ch, door)->keyword))
          return (door);
        else {
          send_to_char(ch, "I see no %s there.\r\n", type);
          return (-1);
        }
      } else
	return (door);
    } else {
      send_to_char(ch, "I really don't see how you can %s anything there.\r\n", cmdname);
      return (-1);
    }
  } else {			/* try to locate the keyword */
    if (!*type) {
      send_to_char(ch, "What is it you want to %s?\r\n", cmdname);
      return (-1);
    }
    for (door = 0; door < DIR_COUNT; door++)
    {
      if (EXIT(ch, door))
      {
        if (EXIT(ch, door)->keyword)
        {
          if (isname(type, EXIT(ch, door)->keyword))
          {
            if ((!IS_NPC(ch)) && (!PRF_FLAGGED(ch, PRF_AUTODOOR)))
              return door;
            else if (is_abbrev(cmdname, "open"))
            {
              if (IS_SET(EXIT(ch, door)->exit_info, EX_CLOSED))
                return door;
              else if (IS_SET(EXIT(ch, door)->exit_info, EX_LOCKED))
                return door;
            }
            else if ((is_abbrev(cmdname, "close")) && (!(IS_SET(EXIT(ch, door)->exit_info, EX_CLOSED))) )
              return door;
            else if ((is_abbrev(cmdname, "lock")) && (!(IS_SET(EXIT(ch, door)->exit_info, EX_LOCKED))) )
              return door;
            else if ((is_abbrev(cmdname, "unlock")) && (IS_SET(EXIT(ch, door)->exit_info, EX_LOCKED)) )
              return door;
            else if ((is_abbrev(cmdname, "pick")) && (IS_SET(EXIT(ch, door)->exit_info, EX_LOCKED)) )
              return door;
          }
        }
      }
    }

    if ((!IS_NPC(ch)) && (!PRF_FLAGGED(ch, PRF_AUTODOOR)))
      send_to_char(ch, "There doesn't seem to be %s %s here.\r\n", AN(type), type);
    else if (is_abbrev(cmdname, "open"))
      send_to_char(ch, "There doesn't seem to be %s %s that can be opened.\r\n", AN(type), type);
    else if (is_abbrev(cmdname, "close"))
      send_to_char(ch, "There doesn't seem to be %s %s that can be closed.\r\n", AN(type), type);
    else if (is_abbrev(cmdname, "lock"))
      send_to_char(ch, "There doesn't seem to be %s %s that can be locked.\r\n", AN(type), type);
    else if (is_abbrev(cmdname, "unlock"))
      send_to_char(ch, "There doesn't seem to be %s %s that can be unlocked.\r\n", AN(type), type);
    else
      send_to_char(ch, "There doesn't seem to be %s %s that can be picked.\r\n", AN(type), type);

    return (-1);
  }
}

int has_key(struct char_data *ch, obj_vnum key)
{
  struct obj_data *o;

  if (key == NOTHING)
    return (0);

  for (o = ch->carrying; o; o = o->next_content)
    if (GET_OBJ_VNUM(o) == key)
      return (1);

  if (GET_EQ(ch, WEAR_HOLD))
    if (GET_OBJ_VNUM(GET_EQ(ch, WEAR_HOLD)) == key)
      return (1);

  return (0);
}

#define NEED_OPEN	(1 << 0)
#define NEED_CLOSED	(1 << 1)
#define NEED_UNLOCKED	(1 << 2)
#define NEED_LOCKED	(1 << 3)

/* cmd_door is required external from act.movement.c */
const char *cmd_door[] =
{
  "open",
  "close",
  "unlock",
  "lock",
  "pick"
};

static const int flags_door[] =
{
  NEED_CLOSED | NEED_UNLOCKED,
  NEED_OPEN,
  NEED_CLOSED | NEED_LOCKED,
  NEED_CLOSED | NEED_UNLOCKED,
  NEED_CLOSED | NEED_LOCKED
};

#define EXITN(room, door)		(world[room].dir_option[door])
#define OPEN_DOOR(room, obj, door)	((obj) ?\
		(REMOVE_BIT(GET_OBJ_VAL(obj, 1), CONT_CLOSED)) :\
		(REMOVE_BIT(EXITN(room, door)->exit_info, EX_CLOSED)))
#define CLOSE_DOOR(room, obj, door)	((obj) ?\
		(SET_BIT(GET_OBJ_VAL(obj, 1), CONT_CLOSED)) :\
		(SET_BIT(EXITN(room, door)->exit_info, EX_CLOSED)))
#define LOCK_DOOR(room, obj, door)	((obj) ?\
		(SET_BIT(GET_OBJ_VAL(obj, 1), CONT_LOCKED)) :\
		(SET_BIT(EXITN(room, door)->exit_info, EX_LOCKED)))
#define UNLOCK_DOOR(room, obj, door)	((obj) ?\
		(REMOVE_BIT(GET_OBJ_VAL(obj, 1), CONT_LOCKED)) :\
		(REMOVE_BIT(EXITN(room, door)->exit_info, EX_LOCKED)))
#define TOGGLE_LOCK(room, obj, door)	((obj) ?\
		(TOGGLE_BIT(GET_OBJ_VAL(obj, 1), CONT_LOCKED)) :\
		(TOGGLE_BIT(EXITN(room, door)->exit_info, EX_LOCKED)))

static void do_doorcmd(struct char_data *ch, struct obj_data *obj, int door, int scmd)
{
  char buf[MAX_STRING_LENGTH];
  size_t len;
  room_rnum other_room = NOWHERE;
  struct room_direction_data *back = NULL;

  if (!door_mtrigger(ch, scmd, door))
    return;

  if (!door_wtrigger(ch, scmd, door))
    return;

  len = snprintf(buf, sizeof(buf), "$n %ss ", cmd_door[scmd]);
  if (!obj && ((other_room = EXIT(ch, door)->to_room) != NOWHERE))
    if ((back = world[other_room].dir_option[rev_dir[door]]) != NULL)
      if (back->to_room != IN_ROOM(ch))
        back = NULL;

  switch (scmd) {
  case SCMD_OPEN:
    OPEN_DOOR(IN_ROOM(ch), obj, door);
    if (back)
      OPEN_DOOR(other_room, obj, rev_dir[door]);
    send_to_char(ch, "%s", CONFIG_OK);
    break;

  case SCMD_CLOSE:
    CLOSE_DOOR(IN_ROOM(ch), obj, door);
    if (back)
      CLOSE_DOOR(other_room, obj, rev_dir[door]);
    send_to_char(ch, "%s", CONFIG_OK);
    break;

  case SCMD_LOCK:
    LOCK_DOOR(IN_ROOM(ch), obj, door);
    if (back)
      LOCK_DOOR(other_room, obj, rev_dir[door]);
    send_to_char(ch, "*Click*\r\n");
    break;

  case SCMD_UNLOCK:
    UNLOCK_DOOR(IN_ROOM(ch), obj, door);
    if (back)
      UNLOCK_DOOR(other_room, obj, rev_dir[door]);
    send_to_char(ch, "*Click*\r\n");
    break;

  case SCMD_PICK:
    TOGGLE_LOCK(IN_ROOM(ch), obj, door);
    if (back)
      TOGGLE_LOCK(other_room, obj, rev_dir[door]);
    send_to_char(ch, "The lock quickly yields to your skills.\r\n");
    len = strlcpy(buf, "$n skillfully picks the lock on ", sizeof(buf));
    gain_skill(ch, "pick lock", TRUE);
    break;
  }

  /* Notify the room. */
  if (len < sizeof(buf))
    snprintf(buf + len, sizeof(buf) - len, "%s%s.",
      obj ? "" : "the ", obj ? "$p" : EXIT(ch, door)->keyword ? "$F" : "door");
  if (!obj || IN_ROOM(obj) != NOWHERE)
    act(buf, FALSE, ch, obj, obj ? 0 : EXIT(ch, door)->keyword, TO_ROOM);

  /* Notify the other room */
  if (back && (scmd == SCMD_OPEN || scmd == SCMD_CLOSE))
      send_to_room(EXIT(ch, door)->to_room, "The %s is %s%s from the other side.\r\n",
        back->keyword ? fname(back->keyword) : "door", cmd_door[scmd],
        scmd == SCMD_CLOSE ? "d" : "ed");
}

static int ok_pick(struct char_data *ch, obj_vnum keynum, int pickproof, int scmd)
{
  int percent, skill_lvl;

  if (scmd != SCMD_PICK)
    return (1);

  percent = rand_number(1, 101);
  skill_lvl = GET_SKILL(ch, SKILL_PICK_LOCK) + GET_ABILITY_MOD(GET_DEX(ch));

  if (keynum == NOTHING) {
    send_to_char(ch, "Odd - you can't seem to find a keyhole.\r\n");
  }
  else if (pickproof) {
    send_to_char(ch, "It resists your attempts to pick it.\r\n");
  }
  else if (percent > skill_lvl) {
    send_to_char(ch, "You failed to pick the lock.\r\n");
    gain_skill(ch, "pick lock", FALSE);
  }
  else {
    return (1);
  }

  return (0);
}

#define DOOR_IS_OPENABLE(ch, obj, door)	((obj) ? ((GET_OBJ_TYPE(obj) == \
    ITEM_CONTAINER) && OBJVAL_FLAGGED(obj, CONT_CLOSEABLE)) :\
    (EXIT_FLAGGED(EXIT(ch, door), EX_ISDOOR)))
#define DOOR_IS_OPEN(ch, obj, door) ((obj) ? (!OBJVAL_FLAGGED(obj, \
    CONT_CLOSED)) : (!EXIT_FLAGGED(EXIT(ch, door), EX_CLOSED)))
#define DOOR_IS_UNLOCKED(ch, obj, door)	((obj) ? (!OBJVAL_FLAGGED(obj, \
    CONT_LOCKED)) : (!EXIT_FLAGGED(EXIT(ch, door), EX_LOCKED)))
#define DOOR_IS_PICKPROOF(ch, obj, door) ((obj) ? (OBJVAL_FLAGGED(obj, \
    CONT_PICKPROOF)) : (EXIT_FLAGGED(EXIT(ch, door), EX_PICKPROOF)))
#define DOOR_IS_CLOSED(ch, obj, door) (!(DOOR_IS_OPEN(ch, obj, door)))
#define DOOR_IS_LOCKED(ch, obj, door) (!(DOOR_IS_UNLOCKED(ch, obj, door)))
#define DOOR_KEY(ch, obj, door)	((obj) ? (GET_OBJ_VAL(obj, 2)) : \
    (EXIT(ch, door)->key))

/* For worn items like backpacks, cloaks, etc */
static void do_worn_openclose(struct char_data *ch, struct obj_data *obj, int subcmd)
{
  if (subcmd == SCMD_OPEN) {
    if (GET_OBJ_VAL(obj, WORN_IS_CLOSED) == 0) {
      send_to_char(ch, "But it's currently open!\r\n");
      return;
    }
    GET_OBJ_VAL(obj, WORN_IS_CLOSED) = 0;
    send_to_char(ch, "%s", CONFIG_OK);
    act("$n opens $p.", FALSE, ch, obj, 0, TO_ROOM);
    return;
  }

  if (subcmd == SCMD_CLOSE) {
    if (GET_OBJ_VAL(obj, WORN_IS_CLOSED) == 1) {
      send_to_char(ch, "But it's already closed!\r\n");
      return;
    }
    GET_OBJ_VAL(obj, WORN_IS_CLOSED) = 1;
    send_to_char(ch, "%s", CONFIG_OK);
    act("$n closes $p.", FALSE, ch, obj, 0, TO_ROOM);
    return;
  }
}

ACMD(do_gen_door)
{
  int door = -1;
  obj_vnum keynum;
  char type[MAX_INPUT_LENGTH], dir[MAX_INPUT_LENGTH];
  struct obj_data *obj = NULL;
  struct char_data *victim = NULL;

  skip_spaces(&argument);
  if (!*argument) {
    send_to_char(ch, "%c%s what?\r\n", UPPER(*cmd_door[subcmd]), cmd_door[subcmd] + 1);
    return;
  }
  two_arguments(argument, type, dir);
  if (!generic_find(type, FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_OBJ_EQUIP, ch, &victim, &obj))
    door = find_door(ch, type, dir, cmd_door[subcmd]);

  if (obj && GET_OBJ_TYPE(obj) == ITEM_WORN &&
      GET_OBJ_VAL(obj, WORN_CAN_OPEN_CLOSE) == 1) {

    if (subcmd == SCMD_OPEN || subcmd == SCMD_CLOSE) {
      do_worn_openclose(ch, obj, subcmd);
      return;
    }

    /* lock/unlock/pick on closable worn items: treat as not applicable */
    /* fall through to door search by discarding obj */
    obj = NULL;
    door = find_door(ch, type, dir, cmd_door[subcmd]);
  }

  if ((obj) && (GET_OBJ_TYPE(obj) != ITEM_CONTAINER)) {
    obj = NULL;
    door = find_door(ch, type, dir, cmd_door[subcmd]);
  }

  if ((obj) || (door >= 0)) {
    keynum = DOOR_KEY(ch, obj, door);
    if (!(DOOR_IS_OPENABLE(ch, obj, door)))
      send_to_char(ch, "You can't %s that!\r\n", cmd_door[subcmd]);
    else if (!DOOR_IS_OPEN(ch, obj, door) && IS_SET(flags_door[subcmd], NEED_OPEN))
      send_to_char(ch, "But it's already closed!\r\n");
    else if (!DOOR_IS_CLOSED(ch, obj, door) && IS_SET(flags_door[subcmd], NEED_CLOSED))
      send_to_char(ch, "But it's currently open!\r\n");
    else if (!(DOOR_IS_LOCKED(ch, obj, door)) && IS_SET(flags_door[subcmd], NEED_LOCKED))
      send_to_char(ch, "Oh.. it wasn't locked, after all..\r\n");
    else if (!(DOOR_IS_UNLOCKED(ch, obj, door)) && IS_SET(flags_door[subcmd], NEED_UNLOCKED) && ((!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_AUTOKEY))) && (has_key(ch, keynum)) )
    {
      send_to_char(ch, "It is locked, but you have the key.\r\n");
      do_doorcmd(ch, obj, door, SCMD_UNLOCK);
      do_doorcmd(ch, obj, door, subcmd);
    }
    else if (!(DOOR_IS_UNLOCKED(ch, obj, door)) && IS_SET(flags_door[subcmd], NEED_UNLOCKED) && ((!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_AUTOKEY))) && (!has_key(ch, keynum)) )
    {
      send_to_char(ch, "It is locked, and you do not have the key!\r\n");
    }
    else if (!(DOOR_IS_UNLOCKED(ch, obj, door)) && IS_SET(flags_door[subcmd], NEED_UNLOCKED) &&
             (GET_LEVEL(ch) < LVL_IMMORT || (!IS_NPC(ch) && !PRF_FLAGGED(ch, PRF_NOHASSLE))))
      send_to_char(ch, "It seems to be locked.\r\n");
    else if (!has_key(ch, keynum) && (GET_LEVEL(ch) < LVL_GOD) && ((subcmd == SCMD_LOCK) || (subcmd == SCMD_UNLOCK)))
      send_to_char(ch, "You don't seem to have the proper key.\r\n");
    else if (ok_pick(ch, keynum, DOOR_IS_PICKPROOF(ch, obj, door), subcmd))
      do_doorcmd(ch, obj, door, subcmd);
  }
  return;
}

ACMD(do_enter)
{
  char buf[MAX_INPUT_LENGTH];
  int door;

  one_argument(argument, buf);

  if (*buf) {			/* an argument was supplied, search for door
				 * keyword */
    for (door = 0; door < DIR_COUNT; door++)
      if (EXIT(ch, door))
        if (EXIT(ch, door)->keyword)
          if (!str_cmp(EXIT(ch, door)->keyword, buf)) {
            perform_move(ch, door, 1);
            return;
          }
    send_to_char(ch, "There is no %s here.\r\n", buf);
  } else if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_INDOORS))
    send_to_char(ch, "You are already indoors.\r\n");
  else {
    /* try to locate an entrance */
    for (door = 0; door < DIR_COUNT; door++)
      if (EXIT(ch, door))
	if (EXIT(ch, door)->to_room != NOWHERE)
	  if (!EXIT_FLAGGED(EXIT(ch, door), EX_CLOSED) &&
	      ROOM_FLAGGED(EXIT(ch, door)->to_room, ROOM_INDOORS)) {
	    perform_move(ch, door, 1);
	    return;
	  }
    send_to_char(ch, "You can't seem to find anything to enter.\r\n");
  }
}

ACMD(do_leave)
{
  int door;

  if (OUTSIDE(ch))
    send_to_char(ch, "You are outside.. where do you want to go?\r\n");
  else {
    for (door = 0; door < DIR_COUNT; door++)
      if (EXIT(ch, door))
	if (EXIT(ch, door)->to_room != NOWHERE)
	  if (!EXIT_FLAGGED(EXIT(ch, door), EX_CLOSED) &&
	    !ROOM_FLAGGED(EXIT(ch, door)->to_room, ROOM_INDOORS)) {
	    perform_move(ch, door, 1);
	    return;
	  }
    send_to_char(ch, "I see no obvious exits to the outside.\r\n");
  }
}

ACMD(do_stand)
{
  char token[MAX_INPUT_LENGTH];
  struct obj_data *furniture = NULL, *current_furniture = SITTING(ch);
  struct char_data *mount = NULL;
  char arg[MAX_INPUT_LENGTH];
  bool has_target = FALSE, used_number = FALSE;
  int ordinal = 0;
  int orig_pos = GET_POS(ch);

  if (*argument) {
    one_argument(argument, arg);
    if (*arg && !is_abbrev(arg, "at"))
      mount = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM);
    if (mount && MOB_FLAGGED(mount, MOB_MOUNT) && HITCHED_TO(mount) != ch) {
      act("You don't have $N hitched.", FALSE, ch, 0, mount, TO_CHAR);
      return;
    }
    if (mount && HITCHED_TO(mount) == ch && MOB_FLAGGED(mount, MOB_MOUNT)) {
      if (FIGHTING(mount)) {
        act("$N is fighting right now.", FALSE, ch, 0, mount, TO_CHAR);
        return;
      }
      if (GET_POS(mount) == POS_RESTING) {
        act("You get $N to stand up.", FALSE, ch, 0, mount, TO_CHAR);
        act("$n gets $N to stand up.", TRUE, ch, 0, mount, TO_ROOM);
        GET_POS(mount) = POS_STANDING;
        clear_custom_ldesc(mount);
        return;
      }
      if (GET_POS(mount) == POS_SLEEPING) {
        act("$N has to wake up first.", FALSE, ch, 0, mount, TO_CHAR);
        return;
      }
      act("$N is already standing.", FALSE, ch, 0, mount, TO_CHAR);
      return;
    }
  }

  if (!*argument && AFF_FLAGGED(ch, AFF_MOUNTED) && MOUNT(ch)) {
    mount = MOUNT(ch);
    if (IN_ROOM(mount) != IN_ROOM(ch) || RIDDEN_BY(mount) != ch) {
      clear_mount_state(ch);
      send_to_char(ch, "You aren't mounted on anything.\r\n");
      return;
    }
    if (FIGHTING(mount)) {
      send_to_char(ch, "Your mount is fighting right now.\r\n");
      return;
    }
    if (GET_POS(mount) != POS_SLEEPING) {
      act("You get $N to rest and dismount.", FALSE, ch, 0, mount, TO_CHAR);
      act("$n gets $N to rest and dismounts.", TRUE, ch, 0, mount, TO_ROOM);
      GET_POS(mount) = POS_RESTING;
      clear_custom_ldesc(mount);
    } else {
      act("$N is already asleep.", FALSE, ch, 0, mount, TO_CHAR);
    }
    clear_mount_state(ch);
    return;
  }

  if (AFF_FLAGGED(ch, AFF_MOUNTED)) {
    send_to_char(ch, "You must dismount first.\r\n");
    return;
  }

  if (*argument) {
    if (!extract_furniture_token(ch, argument, token, sizeof(token), "Stand"))
      return;
    has_target = TRUE;
    furniture = find_furniture_target(ch, token, &used_number, &ordinal);
    if (!furniture) {
      if (used_number)
        send_to_char(ch, "You don't see furniture #%d here.\r\n", ordinal);
      else
        send_to_char(ch, "You don't see that here.\r\n");
      return;
    }
  }

  if (has_target) {
    bool already_there = (current_furniture && current_furniture == furniture);

    if (GET_POS(ch) == POS_SLEEPING) {
      send_to_char(ch, "You have to wake up first!\r\n");
      return;
    } else if (GET_POS(ch) == POS_FIGHTING) {
      send_to_char(ch, "Do you not consider fighting as standing?\r\n");
      return;
    }

    if (!validate_furniture_use(ch, furniture, (1 << 0), "stand", already_there))
      return;

    if (already_there && GET_POS(ch) == POS_STANDING) {
      act("You are already standing at $p.", TRUE, ch, furniture, 0, TO_CHAR);
      return;
    }

    if (!already_there && current_furniture && GET_OBJ_TYPE(current_furniture) == ITEM_FURNITURE) {
      const char *leaving = position_gerund(GET_POS(ch));
      act("You stop $T on $p.", TRUE, ch, current_furniture, (char *)leaving, TO_CHAR);
      act("$n stops $T on $p.", TRUE, ch, current_furniture, (char *)leaving, TO_ROOM);
      char_from_furniture(ch);
    }

    if (!already_there)
      attach_char_to_furniture(ch, furniture);

    const char *self_msg = "You stand at $p.";
    const char *room_msg = "$n stands at $p.";

    if (orig_pos == POS_SITTING) {
      self_msg = "You stand up at $p.";
      room_msg = "$n stands up at $p.";
    } else if (orig_pos == POS_RESTING) {
      self_msg = "You stop resting and stand at $p.";
      room_msg = "$n stops resting and stands at $p.";
    } else if (orig_pos < POS_SITTING) {
      self_msg = "You struggle to your feet at $p.";
      room_msg = "$n struggles to $s feet at $p.";
    }

    act(self_msg, TRUE, ch, furniture, 0, TO_CHAR);
    act(room_msg, TRUE, ch, furniture, 0, TO_ROOM);
    GET_POS(ch) = FIGHTING(ch) ? POS_FIGHTING : POS_STANDING;
    clear_custom_ldesc(ch);
    return;
  }

  switch (GET_POS(ch)) {
  case POS_STANDING:
    send_to_char(ch, "You are already standing.\r\n");
    break;

  case POS_SITTING:
    if (current_furniture && GET_OBJ_TYPE(current_furniture) == ITEM_FURNITURE) {
      act("You stand up from $p.", TRUE, ch, current_furniture, 0, TO_CHAR);
      act("$n stands up from $p.", TRUE, ch, current_furniture, 0, TO_ROOM);
    } else {
      send_to_char(ch, "You stand up.\r\n");
      act("$n clambers to $s feet.", TRUE, ch, 0, 0, TO_ROOM);
    }
    /* Were they sitting in something? */
    char_from_furniture(ch);
    /* Will be standing after a successful bash and may still be fighting. */
    GET_POS(ch) = FIGHTING(ch) ? POS_FIGHTING : POS_STANDING;
    clear_custom_ldesc(ch);
    break;

  case POS_RESTING:
    if (current_furniture && GET_OBJ_TYPE(current_furniture) == ITEM_FURNITURE) {
      act("You stop resting, and stand up from $p.", TRUE, ch, current_furniture, 0, TO_CHAR);
      act("$n stops resting, and stands up from $p.", TRUE, ch, current_furniture, 0, TO_ROOM);
    } else {
      send_to_char(ch, "You stop resting, and stand up.\r\n");
      act("$n stops resting, and clambers on $s feet.", TRUE, ch, 0, 0, TO_ROOM);
    }
    GET_POS(ch) = POS_STANDING;
    /* Were they sitting in something. */
    char_from_furniture(ch);
    clear_custom_ldesc(ch);
    break;

  case POS_SLEEPING:
    send_to_char(ch, "You have to wake up first!\r\n");
    break;

  case POS_FIGHTING:
    send_to_char(ch, "Do you not consider fighting as standing?\r\n");
    break;

  default:
    send_to_char(ch, "You stop floating around, and put your feet on the ground.\r\n");
    if (current_furniture && GET_OBJ_TYPE(current_furniture) == ITEM_FURNITURE)
      act("$n stops floating around, and stands up from $p.", TRUE, ch, current_furniture, 0, TO_ROOM);
    else
      act("$n stops floating around, and puts $s feet on the ground.", TRUE, ch, 0, 0, TO_ROOM);
    GET_POS(ch) = POS_STANDING;
    clear_custom_ldesc(ch);
    break;
  }
}

ACMD(do_sit)
{
  char token[MAX_INPUT_LENGTH];
  struct obj_data *furniture = NULL, *current_furniture = NULL;
  bool has_target = FALSE, used_number = FALSE;
  int ordinal = 0;
  int orig_pos = GET_POS(ch);

  if (AFF_FLAGGED(ch, AFF_MOUNTED)) {
    send_to_char(ch, "You must dismount first.\r\n");
    return;
  }

  if (*argument) {
    if (!extract_furniture_token(ch, argument, token, sizeof(token), "Sit"))
      return;
    has_target = TRUE;
    furniture = find_furniture_target(ch, token, &used_number, &ordinal);
    if (!furniture) {
      if (used_number)
        send_to_char(ch, "You don't see furniture #%d here.\r\n", ordinal);
      else
        send_to_char(ch, "You don't see that here.\r\n");
      return;
    }
  }

  current_furniture = SITTING(ch);

  if (has_target) {
    bool already_there = (current_furniture && current_furniture == furniture);

    if (GET_POS(ch) == POS_SLEEPING) {
      send_to_char(ch, "You have to wake up first.\r\n");
      return;
    }
    if (GET_POS(ch) == POS_FIGHTING) {
      send_to_char(ch, "Sit down while fighting? Are you MAD?\r\n");
      return;
    }

    if (!validate_furniture_use(ch, furniture, (1 << 1), "sit", already_there))
      return;

    if (already_there) {
      if (GET_POS(ch) == POS_SITTING) {
        act("You are already sitting on $p.", TRUE, ch, furniture, 0, TO_CHAR);
        return;
      }
    } else if (current_furniture && GET_OBJ_TYPE(current_furniture) == ITEM_FURNITURE) {
      const char *leaving = position_gerund(GET_POS(ch));
      act("You stop $T on $p.", TRUE, ch, current_furniture, (char *)leaving, TO_CHAR);
      act("$n stops $T on $p.", TRUE, ch, current_furniture, (char *)leaving, TO_ROOM);
      char_from_furniture(ch);
    }

    if (!already_there)
      attach_char_to_furniture(ch, furniture);

    const char *self_msg = "You sit on $p.";
    const char *room_msg = "$n sits on $p.";

    if (orig_pos == POS_STANDING) {
      self_msg = "You sit down on $p.";
      room_msg = "$n sits down on $p.";
    } else if (orig_pos == POS_RESTING) {
      self_msg = "You stop resting and sit up on $p.";
      room_msg = "$n stops resting and sits up on $p.";
    }

    act(self_msg, TRUE, ch, furniture, 0, TO_CHAR);
    act(room_msg, TRUE, ch, furniture, 0, TO_ROOM);
    GET_POS(ch) = POS_SITTING;
    clear_custom_ldesc(ch);
    return;
  }

  switch (GET_POS(ch)) {
  case POS_STANDING:
    send_to_char(ch, "You sit down.\r\n");
    act("$n sits down.", FALSE, ch, 0, 0, TO_ROOM);
    GET_POS(ch) = POS_SITTING;
    clear_custom_ldesc(ch);
    break;
  case POS_SITTING:
    send_to_char(ch, "You're sitting already.\r\n");
    break;
  case POS_RESTING:
    send_to_char(ch, "You stop resting, and sit up.\r\n");
    if (current_furniture && GET_OBJ_TYPE(current_furniture) == ITEM_FURNITURE)
      act("$n stops resting and sits up on $p.", TRUE, ch, current_furniture, 0, TO_ROOM);
    else
      act("$n stops resting and sits up.", TRUE, ch, 0, 0, TO_ROOM);
    GET_POS(ch) = POS_SITTING;
    clear_custom_ldesc(ch);
    break;
  case POS_SLEEPING:
    send_to_char(ch, "You have to wake up first.\r\n");
    break;
  case POS_FIGHTING:
    send_to_char(ch, "Sit down while fighting? Are you MAD?\r\n");
    break;
  default:
    send_to_char(ch, "You stop floating around, and sit down.\r\n");
    if (current_furniture && GET_OBJ_TYPE(current_furniture) == ITEM_FURNITURE)
      act("$n stops floating around, and sits down on $p.", TRUE, ch, current_furniture, 0, TO_ROOM);
    else
      act("$n stops floating around, and sits down.", TRUE, ch, 0, 0, TO_ROOM);
    GET_POS(ch) = POS_SITTING;
    clear_custom_ldesc(ch);
    break;
  }
}

ACMD(do_rest)
{
  char token[MAX_INPUT_LENGTH];
  struct obj_data *furniture = NULL, *current_furniture = SITTING(ch);
  struct char_data *mount = NULL;
  char arg[MAX_INPUT_LENGTH];
  bool has_target = FALSE, used_number = FALSE;
  int ordinal = 0;

  if (*argument) {
    one_argument(argument, arg);
    if (*arg && !is_abbrev(arg, "at"))
      mount = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM);
    if (mount && MOB_FLAGGED(mount, MOB_MOUNT) && HITCHED_TO(mount) != ch) {
      act("You don't have $N hitched.", FALSE, ch, 0, mount, TO_CHAR);
      return;
    }
    if (mount && HITCHED_TO(mount) == ch && MOB_FLAGGED(mount, MOB_MOUNT)) {
      if (FIGHTING(mount)) {
        act("$N is fighting right now.", FALSE, ch, 0, mount, TO_CHAR);
        return;
      }
      if (GET_POS(mount) == POS_RESTING) {
        act("$N is already resting.", FALSE, ch, 0, mount, TO_CHAR);
        return;
      }
      if (GET_POS(mount) == POS_SLEEPING) {
        act("$N is already asleep.", FALSE, ch, 0, mount, TO_CHAR);
        return;
      }
      act("You pull on $N's reins, forcing it to rest.", FALSE, ch, 0, mount, TO_CHAR);
      act("$n pulls on $N's reins, forcing it to rest.", TRUE, ch, 0, mount, TO_ROOM);
      GET_POS(mount) = POS_RESTING;
      clear_custom_ldesc(mount);
      return;
    }
  }

  if (!*argument && AFF_FLAGGED(ch, AFF_MOUNTED) && MOUNT(ch)) {
    mount = MOUNT(ch);
    if (IN_ROOM(mount) != IN_ROOM(ch) || RIDDEN_BY(mount) != ch) {
      clear_mount_state(ch);
      send_to_char(ch, "You aren't mounted on anything.\r\n");
      return;
    }
    if (FIGHTING(mount)) {
      send_to_char(ch, "Your mount is fighting right now.\r\n");
      return;
    }
    if (GET_POS(mount) == POS_SLEEPING) {
      act("$N is already asleep.", FALSE, ch, 0, mount, TO_CHAR);
      clear_mount_state(ch);
      return;
    }
    act("You pull on $N's reins, forcing it to rest.", FALSE, ch, 0, mount, TO_CHAR);
    act("$n pulls on $N's reins, forcing it to rest and dismounts.", TRUE, ch, 0, mount, TO_ROOM);
    GET_POS(mount) = POS_RESTING;
    clear_custom_ldesc(mount);
    clear_mount_state(ch);
    return;
  }

  if (AFF_FLAGGED(ch, AFF_MOUNTED)) {
    send_to_char(ch, "You must dismount first.\r\n");
    return;
  }

  if (*argument) {
    if (!extract_furniture_token(ch, argument, token, sizeof(token), "Rest"))
      return;
    has_target = TRUE;
    furniture = find_furniture_target(ch, token, &used_number, &ordinal);
    if (!furniture) {
      if (used_number)
        send_to_char(ch, "You don't see furniture #%d here.\r\n", ordinal);
      else
        send_to_char(ch, "You don't see that here.\r\n");
      return;
    }
  }

  if (has_target) {
    bool already_there = (current_furniture && current_furniture == furniture);

    if (GET_POS(ch) == POS_SLEEPING) {
      send_to_char(ch, "You have to wake up first.\r\n");
      return;
    }
    if (GET_POS(ch) == POS_FIGHTING) {
      send_to_char(ch, "Rest while fighting?  Are you MAD?\r\n");
      return;
    }

    if (!validate_furniture_use(ch, furniture, (1 << 2), "rest", already_there))
      return;

    if (already_there) {
      if (GET_POS(ch) == POS_RESTING) {
        act("You are already resting on $p.", TRUE, ch, furniture, 0, TO_CHAR);
        return;
      }
    } else if (current_furniture && GET_OBJ_TYPE(current_furniture) == ITEM_FURNITURE) {
      const char *leaving = position_gerund(GET_POS(ch));
      act("You stop $T on $p.", TRUE, ch, current_furniture, (char *)leaving, TO_CHAR);
      act("$n stops $T on $p.", TRUE, ch, current_furniture, (char *)leaving, TO_ROOM);
      char_from_furniture(ch);
    }

    if (!already_there)
      attach_char_to_furniture(ch, furniture);

    const char *self_msg = "You rest on $p.";
    const char *room_msg = "$n rests on $p.";

    if (GET_POS(ch) == POS_STANDING) {
      self_msg = "You sit down and rest on $p.";
      room_msg = "$n sits down and rests on $p.";
    } else if (GET_POS(ch) == POS_SITTING) {
      self_msg = "You rest on $p.";
      room_msg = "$n rests on $p.";
    } else if (GET_POS(ch) == POS_RESTING && !already_there) {
      self_msg = "You continue resting on $p.";
      room_msg = "$n continues resting on $p.";
    }

    act(self_msg, TRUE, ch, furniture, 0, TO_CHAR);
    act(room_msg, TRUE, ch, furniture, 0, TO_ROOM);
    GET_POS(ch) = POS_RESTING;
    clear_custom_ldesc(ch);
    return;
  }

  switch (GET_POS(ch)) {
  case POS_STANDING:
    if (current_furniture && GET_OBJ_TYPE(current_furniture) == ITEM_FURNITURE) {
      act("You sit down and rest on $p.", TRUE, ch, current_furniture, 0, TO_CHAR);
      act("$n sits down and rests on $p.", TRUE, ch, current_furniture, 0, TO_ROOM);
    } else {
      send_to_char(ch, "You sit down and rest your tired bones.\r\n");
      act("$n sits down and rests.", TRUE, ch, 0, 0, TO_ROOM);
    }
    GET_POS(ch) = POS_RESTING;
    clear_custom_ldesc(ch);
    break;

  case POS_SITTING:
    /* Check if sitting on furniture that allows resting */
    if (current_furniture && GET_OBJ_TYPE(current_furniture) == ITEM_FURNITURE) {
      int allowed_positions = GET_OBJ_VAL(current_furniture, VAL_FURN_POSITIONS); /* VAL_FURN_POSITIONS */
      if (allowed_positions > 0 && !(allowed_positions & (1 << 2))) { /* Check REST bit (bit 2) */
        act("$p doesn't look comfortable for resting.", TRUE, ch, current_furniture, 0, TO_CHAR);
        return;
      }
      /* Valid furniture + resting */
      act("You rest on $p.", TRUE, ch, current_furniture, 0, TO_CHAR);
      act("$n rests on $p.", TRUE, ch, current_furniture, 0, TO_ROOM);
    } else {
      send_to_char(ch, "You rest your tired bones.\r\n");
      act("$n rests.", TRUE, ch, 0, 0, TO_ROOM);
    }
    GET_POS(ch) = POS_RESTING;
    clear_custom_ldesc(ch);
    break;

  case POS_RESTING:
    send_to_char(ch, "You are already resting.\r\n");
    break;

  case POS_SLEEPING:
    send_to_char(ch, "You have to wake up first.\r\n");
    break;

  case POS_FIGHTING:
    send_to_char(ch, "Rest while fighting?  Are you MAD?\r\n");
    break;

  default:
    send_to_char(ch, "You stop floating around, and stop to rest your tired bones.\r\n");
    if (current_furniture && GET_OBJ_TYPE(current_furniture) == ITEM_FURNITURE)
      act("$n stops floating around, and rests on $p.", FALSE, ch, current_furniture, 0, TO_ROOM);
    else
      act("$n stops floating around, and rests.", FALSE, ch, 0, 0, TO_ROOM);
    GET_POS(ch) = POS_RESTING;
    clear_custom_ldesc(ch);
    break;
  }
}

ACMD(do_sleep)
{
  char token[MAX_INPUT_LENGTH];
  struct obj_data *furniture = NULL, *current_furniture = SITTING(ch);
  bool has_target = FALSE, used_number = FALSE;
  int ordinal = 0;

  if (AFF_FLAGGED(ch, AFF_MOUNTED)) {
    send_to_char(ch, "You must dismount first.\r\n");
    return;
  }

  if (*argument) {
    if (!extract_furniture_token(ch, argument, token, sizeof(token), "Sleep"))
      return;
    has_target = TRUE;
    furniture = find_furniture_target(ch, token, &used_number, &ordinal);
    if (!furniture) {
      if (used_number)
        send_to_char(ch, "You don't see furniture #%d here.\r\n", ordinal);
      else
        send_to_char(ch, "You don't see that here.\r\n");
      return;
    }
  }

  if (has_target) {
    bool already_there = (current_furniture && current_furniture == furniture);

    if (GET_POS(ch) == POS_SLEEPING) {
      send_to_char(ch, "You are already sound asleep.\r\n");
      return;
    }
    if (GET_POS(ch) == POS_FIGHTING) {
      send_to_char(ch, "Sleep while fighting?  Are you MAD?\r\n");
      return;
    }

    if (!validate_furniture_use(ch, furniture, (1 << 3), "sleep", already_there))
      return;

    if (!already_there && current_furniture && GET_OBJ_TYPE(current_furniture) == ITEM_FURNITURE) {
      const char *leaving = position_gerund(GET_POS(ch));
      act("You stop $T on $p.", TRUE, ch, current_furniture, (char *)leaving, TO_CHAR);
      act("$n stops $T on $p.", TRUE, ch, current_furniture, (char *)leaving, TO_ROOM);
      char_from_furniture(ch);
    }

    if (!already_there)
      attach_char_to_furniture(ch, furniture);

    act("You go to sleep on $p.", TRUE, ch, furniture, 0, TO_CHAR);
    act("$n lies down and falls asleep on $p.", TRUE, ch, furniture, 0, TO_ROOM);
    GET_POS(ch) = POS_SLEEPING;
    clear_custom_ldesc(ch);
    return;
  }

  switch (GET_POS(ch)) {
  case POS_STANDING:
  case POS_SITTING:
  case POS_RESTING:
    /* Check if sitting/resting on furniture that allows sleeping */
    if (current_furniture && GET_OBJ_TYPE(current_furniture) == ITEM_FURNITURE) {
      int allowed_positions = GET_OBJ_VAL(current_furniture, VAL_FURN_POSITIONS); /* VAL_FURN_POSITIONS */
      if (allowed_positions > 0 && !(allowed_positions & (1 << 3))) { /* Check SLEEP bit (bit 3) */
        act("$p doesn't look comfortable for sleeping.", TRUE, ch, current_furniture, 0, TO_CHAR);
        return;
      }
      /* Valid furniture + sleeping */
      act("You go to sleep on $p.", TRUE, ch, current_furniture, 0, TO_CHAR);
      act("$n lies down and falls asleep on $p.", TRUE, ch, current_furniture, 0, TO_ROOM);
    } else {
      send_to_char(ch, "You go to sleep.\r\n");
      act("$n lies down and falls asleep.", TRUE, ch, 0, 0, TO_ROOM);
    }
    GET_POS(ch) = POS_SLEEPING;
    clear_custom_ldesc(ch);
    break;

  case POS_SLEEPING:
    send_to_char(ch, "You are already sound asleep.\r\n");
    break;

  case POS_FIGHTING:
    send_to_char(ch, "Sleep while fighting?  Are you MAD?\r\n");
    break;

  default:
    send_to_char(ch, "You stop floating around, and lie down to sleep.\r\n");
    if (current_furniture && GET_OBJ_TYPE(current_furniture) == ITEM_FURNITURE)
      act("$n stops floating around, and lies down to sleep on $p.", TRUE, ch, current_furniture, 0, TO_ROOM);
    else
      act("$n stops floating around, and lies down to sleep.", TRUE, ch, 0, 0, TO_ROOM);
    GET_POS(ch) = POS_SLEEPING;
    clear_custom_ldesc(ch);
    break;
  }
}

ACMD(do_wake)
{
  char arg[MAX_INPUT_LENGTH];
  struct char_data *vict;
  int self = 0;

  one_argument(argument, arg);
  if (*arg) {
    if (GET_POS(ch) == POS_SLEEPING)
      send_to_char(ch, "Maybe you should wake yourself up first.\r\n");
    else if ((vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM)) == NULL)
      send_to_char(ch, "%s", CONFIG_NOPERSON);
    else if (vict == ch)
      self = 1;
    else if (AWAKE(vict))
      act("$E is already awake.", FALSE, ch, 0, vict, TO_CHAR);
    else if (AFF_FLAGGED(vict, AFF_SLEEP))
      act("You can't wake $M up!", FALSE, ch, 0, vict, TO_CHAR);
    else if (GET_POS(vict) < POS_SLEEPING)
      act("$E's in pretty bad shape!", FALSE, ch, 0, vict, TO_CHAR);
    else {
      act("You wake $M up.", FALSE, ch, 0, vict, TO_CHAR);
      act("You are awakened by $n.", FALSE, ch, 0, vict, TO_VICT | TO_SLEEP);
      GET_POS(vict) = POS_SITTING;
    }
    if (!self)
      return;
  }

  if (AFF_FLAGGED(ch, AFF_SLEEP))
    send_to_char(ch, "You can't wake up!\r\n");
  else if (GET_POS(ch) > POS_SLEEPING)
    send_to_char(ch, "You are already awake...\r\n");
  else {
    if (SITTING(ch) && GET_OBJ_TYPE(SITTING(ch)) == ITEM_FURNITURE) {
      act("You awaken, and sit up on $p.", TRUE, ch, SITTING(ch), 0, TO_CHAR | TO_SLEEP);
      act("$n awakens, and sits up on $p.", TRUE, ch, SITTING(ch), 0, TO_ROOM);
    } else {
      send_to_char(ch, "You awaken, and sit up.\r\n");
      act("$n awakens.", TRUE, ch, 0, 0, TO_ROOM);
    }
    GET_POS(ch) = POS_SITTING;
  }
}

ACMD(do_follow)
{
  char buf[MAX_INPUT_LENGTH];
  struct char_data *leader;

  one_argument(argument, buf);

  if (*buf) {
    if (!(leader = get_char_vis(ch, buf, NULL, FIND_CHAR_ROOM))) {
      send_to_char(ch, "%s", CONFIG_NOPERSON);
      return;
    }
  } else {
    if (ch->master != (char_data*)  NULL) {
      send_to_char(ch, "You are following %s.\r\n", 
         GET_NAME(ch->master));
    } else {
      send_to_char(ch, "Whom do you wish to follow?\r\n");
    }
    return;
  }

  if (ch->master == leader) {
    act("You are already following $M.", FALSE, ch, 0, leader, TO_CHAR);
    return;
  }
  if (AFF_FLAGGED(ch, AFF_CHARM) && (ch->master)) {
    act("But you only feel like following $N!", FALSE, ch, 0, ch->master, TO_CHAR);
  } else {			/* Not Charmed follow person */
    if (leader == ch) {
      if (!ch->master) {
        send_to_char(ch, "You are already following yourself.\r\n");
        return;
      }
      stop_follower(ch);
    } else {
      if (circle_follow(ch, leader)) {
        send_to_char(ch, "Sorry, but following in loops is not allowed.\r\n");
        return;
      }
      if (ch->master)
        stop_follower(ch);

      add_follower(ch, leader);
    }
  }
}

ACMD(do_mount)
{
  char arg[MAX_INPUT_LENGTH];
  struct char_data *mount;

  one_argument(argument, arg);

  if (!*arg) {
    send_to_char(ch, "Mount what?\r\n");
    return;
  }

  if (AFF_FLAGGED(ch, AFF_MOUNTED)) {
    send_to_char(ch, "You are already mounted.\r\n");
    return;
  }

  if (!(mount = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM))) {
    send_to_char(ch, "%s", CONFIG_NOPERSON);
    return;
  }

  if (mount == ch) {
    send_to_char(ch, "You can't mount yourself.\r\n");
    return;
  }

  if (!IS_NPC(mount) || !MOB_FLAGGED(mount, MOB_MOUNT)) {
    send_to_char(ch, "You can't mount %s!\r\n", get_char_sdesc(mount));
    return;
  }

  if (HITCHED_TO(mount) && HITCHED_TO(mount) != ch) {
    act("$N is already hitched to someone else.", FALSE, ch, 0, mount, TO_CHAR);
    return;
  }

  if (RIDDEN_BY(mount)) {
    act("$N is already being ridden.", FALSE, ch, 0, mount, TO_CHAR);
    return;
  }

  if ((GET_WEIGHT(ch) + IS_CARRYING_W(ch) + IS_CARRYING_W(mount)) > CAN_CARRY_W(mount)) {
    act("$N can't carry that much weight.", FALSE, ch, 0, mount, TO_CHAR);
    return;
  }

  SET_BIT_AR(AFF_FLAGS(ch), AFF_MOUNTED);
  MOUNT(ch) = mount;
  RIDDEN_BY(mount) = ch;
  IS_CARRYING_W(mount) += GET_WEIGHT(ch) + IS_CARRYING_W(ch);

  act("You mount $N.", FALSE, ch, 0, mount, TO_CHAR);
  act("$n mounts $N.", TRUE, ch, 0, mount, TO_ROOM);
}

ACMD(do_dismount)
{
  struct char_data *mount = MOUNT(ch);

  if (!AFF_FLAGGED(ch, AFF_MOUNTED) || !mount) {
    clear_mount_state(ch);
    send_to_char(ch, "You aren't mounted on anything.\r\n");
    return;
  }

  act("You dismount $N.", FALSE, ch, 0, mount, TO_CHAR);
  act("$n dismounts $N.", TRUE, ch, 0, mount, TO_ROOM);
  clear_mount_state(ch);
}

ACMD(do_hitch)
{
  char arg[MAX_INPUT_LENGTH];
  struct char_data *mount;
  int max_hitched;

  one_argument(argument, arg);

  if (!*arg) {
    send_to_char(ch, "Hitch what?\r\n");
    return;
  }

  if (!(mount = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM))) {
    send_to_char(ch, "%s", CONFIG_NOPERSON);
    return;
  }

  if (mount == ch) {
    send_to_char(ch, "You can't hitch yourself.\r\n");
    return;
  }

  if (!IS_NPC(mount) || !MOB_FLAGGED(mount, MOB_MOUNT)) {
    send_to_char(ch, "You can't hitch %s!\r\n", get_char_sdesc(mount));
    return;
  }

  if (RIDDEN_BY(mount) && RIDDEN_BY(mount) != ch) {
    act("$N is already being ridden.", FALSE, ch, 0, mount, TO_CHAR);
    return;
  }

  if (HITCHED_TO(mount) == ch) {
    act("$N is already hitched to you.", FALSE, ch, 0, mount, TO_CHAR);
    return;
  }

  if (HITCHED_TO(mount) && HITCHED_TO(mount) != ch) {
    act("$N is already hitched to someone else.", FALSE, ch, 0, mount, TO_CHAR);
    return;
  }

  if (mount->master && mount->master != ch) {
    act("$N is already following someone else.", FALSE, ch, 0, mount, TO_CHAR);
    return;
  }

  max_hitched = max_hitched_mounts(ch);
  if (count_hitched_mounts(ch) >= max_hitched) {
    send_to_char(ch, "You can't hitch any more mounts.\r\n");
    return;
  }

  if (!mount->master)
    add_follower(mount, ch);

  HITCHED_TO(mount) = ch;
  act("You hitch $N to you.", FALSE, ch, 0, mount, TO_CHAR);
  act("$n hitches $N to $m.", TRUE, ch, 0, mount, TO_ROOM);
}

ACMD(do_unhitch)
{
  char arg[MAX_INPUT_LENGTH];
  struct char_data *mount;

  one_argument(argument, arg);

  if (!*arg) {
    send_to_char(ch, "Unhitch what?\r\n");
    return;
  }

  if (!(mount = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM))) {
    send_to_char(ch, "%s", CONFIG_NOPERSON);
    return;
  }

  if (HITCHED_TO(mount) != ch || !MOB_FLAGGED(mount, MOB_MOUNT)) {
    act("You don't have $N hitched.", FALSE, ch, 0, mount, TO_CHAR);
    return;
  }

  if (mount->master == ch)
    stop_follower(mount);
  else
    HITCHED_TO(mount) = NULL;

  act("You unhitch $N.", FALSE, ch, 0, mount, TO_CHAR);
  act("$n unhitches $N.", TRUE, ch, 0, mount, TO_ROOM);
}

ACMD(do_pack)
{
  char arg[MAX_INPUT_LENGTH];
  struct char_data *mount;
  struct obj_data *obj;
  struct follow_type *follow;
  bool found_mount = FALSE;

  one_argument(argument, arg);

  if (!*arg) {
    for (follow = ch->followers; follow; follow = follow->next) {
      bool found_item = FALSE;

      mount = follow->follower;
      if (HITCHED_TO(mount) != ch || !MOB_FLAGGED(mount, MOB_MOUNT))
        continue;
      if (IN_ROOM(mount) != IN_ROOM(ch))
        continue;

      found_mount = TRUE;
      send_to_char(ch, "Packed on %s:\r\n", get_char_sdesc(mount));

      for (obj = mount->carrying; obj; obj = obj->next_content) {
        if (GET_OBJ_TYPE(obj) != ITEM_CONTAINER)
          continue;
        if (!CAN_SEE_OBJ(ch, obj))
          continue;
        send_to_char(ch, "  %s\r\n", obj->short_description ? obj->short_description : "something");
        found_item = TRUE;
      }

      if (!found_item)
        send_to_char(ch, "  Nothing.\r\n");
    }

    if (!found_mount)
      send_to_char(ch, "You don't have any mounts hitched.\r\n");
    return;
  }

  if (!(obj = get_obj_in_list_vis(ch, arg, NULL, ch->carrying))) {
    send_to_char(ch, "You don't seem to have that.\r\n");
    return;
  }

  if (GET_OBJ_TYPE(obj) != ITEM_CONTAINER) {
    send_to_char(ch, "You can only pack containers onto a mount.\r\n");
    return;
  }

  mount = find_hitched_mount_for_pack(ch, obj);
  if (!mount) {
    if (!first_hitched_mount_in_room(ch))
      send_to_char(ch, "You don't have any mounts hitched here.\r\n");
    else
      send_to_char(ch, "Your mount can't carry that much.\r\n");
    return;
  }

  obj_from_char(obj);
  obj_to_char(obj, mount);

  act("You pack $p onto $N.", FALSE, ch, obj, mount, TO_CHAR);
  act("$n packs $p onto $N.", TRUE, ch, obj, mount, TO_ROOM);
}

ACMD(do_unpack)
{
  char arg[MAX_INPUT_LENGTH];
  struct char_data *mount;
  struct obj_data *obj;
  struct follow_type *follow;

  one_argument(argument, arg);

  if (!*arg) {
    send_to_char(ch, "Unpack what?\r\n");
    return;
  }

  for (follow = ch->followers; follow; follow = follow->next) {
    mount = follow->follower;

    if (HITCHED_TO(mount) != ch || !MOB_FLAGGED(mount, MOB_MOUNT))
      continue;
    if (IN_ROOM(mount) != IN_ROOM(ch))
      continue;

    obj = get_obj_in_list_vis(ch, arg, NULL, mount->carrying);
    if (!obj)
      continue;
    if (GET_OBJ_TYPE(obj) != ITEM_CONTAINER)
      continue;

    if (IS_CARRYING_N(ch) >= CAN_CARRY_N(ch)) {
      send_to_char(ch, "You can't carry any more items.\r\n");
      return;
    }
    if ((IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(obj)) > CAN_CARRY_W(ch)) {
      send_to_char(ch, "You can't carry that much weight.\r\n");
      return;
    }

    obj_from_char(obj);
    obj_to_char(obj, ch);

    act("You unpack $p from $N.", FALSE, ch, obj, mount, TO_CHAR);
    act("$n unpacks $p from $N.", TRUE, ch, obj, mount, TO_ROOM);
    return;
  }

  send_to_char(ch, "You don't have that packed on a hitched mount.\r\n");
}

ACMD(do_unfollow)
{
  if (ch->master) {
    if (AFF_FLAGGED(ch, AFF_CHARM)) { 
       send_to_char(ch, "You feel compelled to follow %s.\r\n",
         GET_NAME(ch->master));
    } else {
      stop_follower(ch);
    }
  } else {
    send_to_char(ch, "You are not following anyone.\r\n");
  }
  return;
}
