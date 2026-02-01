/**************************************************************************
*  File: handler.c                                         Part of tbaMUD *
*  Usage: Internal funcs: moving and finding chars/objs.                  *
*                                                                         *
*  All rights reserved.  See license for complete information.            *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "handler.h"
#include "screen.h"
#include "interpreter.h"
#include "spells.h"
#include "py_triggers.h"
#include "act.h"
#include "class.h"
#include "fight.h"
#include "quest.h"
#include "mud_event.h"

/* local file scope variables */
static int extractions_pending = 0;

/* local file scope functions */
static int apply_ac(struct char_data *ch, int eq_pos);
static void update_object(struct obj_data *obj, int use);
static void affect_modify_ar(struct char_data * ch, byte loc, sbyte mod, int bitv[], bool add);
static bool is_shadow_keyword(const char *name);
static int obj_coin_count(struct obj_data *obj);
static struct char_data *obj_owner(struct obj_data *obj);
static void adjust_char_coins(struct char_data *ch, int amount);

char *fname(const char *namelist)
{
  static char holder[READ_SIZE];
  char *point;

  for (point = holder; isalpha(*namelist); namelist++, point++)
    *point = *namelist;

  *point = '\0';

  return (holder);
}

/* Leave this here even if you put in a newer isname().  Used for OasisOLC. */
int is_name(const char *str, const char *namelist)
{
  const char *curname, *curstr;

  if (!str || !namelist || !*str || !*namelist)
    return (0);

  curname = namelist;
  for (;;) {
    for (curstr = str;; curstr++, curname++) {
      if (!*curstr && !isalpha(*curname))
        return (1);

      if (!*curname)
        return (0);

      if (!*curstr || *curname == ' ')
        break;

      if (LOWER(*curstr) != LOWER(*curname))
        break;
    }

    /* skip to next name */
   for (; isalpha(*curname); curname++)
     ; /* semi-colon moved to suppress warning */
     if (!*curname)
       return (0);
    curname++;                  /* first char of new name */
  }
}

/* allow abbreviations */
#define WHITESPACE " \t"
int isname(const char *str, const char *namelist)
{
  char *newlist;
  char *curtok;

  if (!str || !*str || !namelist || !*namelist)
    return 0;

  /* exact full-string match check */
  if (!strcasecmp(str, namelist))
    return 1;

  newlist = strdup(namelist); /* make a copy since strtok modifies the string */
  if (!newlist)
    return 0;

  for (curtok = strtok(newlist, WHITESPACE); curtok; curtok = strtok(NULL, WHITESPACE)) {
    /* Compare each token as a full word, case-insensitive */
    if (!strcasecmp(str, curtok)) {
      free(newlist);
      return 1;
    }
  }

  free(newlist);
  return 0;
}

static bool is_shadow_keyword(const char *name)
{
  if (!name || !*name)
    return FALSE;
  return isname(name, "shadow shadowy figure");
}

static void aff_apply_modify(struct char_data *ch, byte loc, sbyte mod, char *msg)
{
  switch (loc) {
  case APPLY_NONE:
    break;

  /* --- ability scores --- */
  case APPLY_STR:  GET_STR(ch) += mod; break;
  case APPLY_DEX:  GET_DEX(ch) += mod; break;
  case APPLY_INT:  GET_INT(ch) += mod; break;
  case APPLY_WIS:  GET_WIS(ch) += mod; break;
  case APPLY_CON:  GET_CON(ch) += mod; break;
  case APPLY_CHA:  GET_CHA(ch) += mod; break;

  /* Do Not Use. */
  case APPLY_CLASS:
  case APPLY_LEVEL:
    break;

  case APPLY_AGE:
    ch->player.time.birth -= (mod * SECS_PER_MUD_YEAR);
    break;

  case APPLY_CHAR_WEIGHT:  GET_WEIGHT(ch) += mod; break;
  case APPLY_CHAR_HEIGHT:  GET_HEIGHT(ch) += mod; break;
  case APPLY_MANA:         GET_MAX_MANA(ch) += mod; break;
  case APPLY_HIT:          GET_MAX_HIT(ch) += mod; break;
  case APPLY_STAMINA:         GET_MAX_STAMINA(ch) += mod; break;
  case APPLY_COINS:         break;
  case APPLY_EXP:          break;

  case APPLY_AC:           GET_AC(ch) += mod; break;
  case APPLY_PROFICIENCY:  GET_PROF_MOD(ch) += mod; break;

  /* --- new 5e-style saving throws --- */
  case APPLY_SAVE_STR: SAVE_STR(ch) += mod; break;
  case APPLY_SAVE_DEX: SAVE_DEX(ch) += mod; break;
  case APPLY_SAVE_CON: SAVE_CON(ch) += mod; break;
  case APPLY_SAVE_INT: SAVE_INT(ch) += mod; break;
  case APPLY_SAVE_WIS: SAVE_WIS(ch) += mod; break;
  case APPLY_SAVE_CHA: SAVE_CHA(ch) += mod; break;

  default:
    log("SYSERR: Unknown apply adjust %d attempt (%s, affect_modify).", loc, __FILE__);
    break;
  } /* switch */
}

static void affect_modify_ar(struct char_data * ch, byte loc, sbyte mod, int bitv[], bool add)
{
  int i , j;

  if (add) {
    for(i = 0; i < AF_ARRAY_MAX; i++)
      for(j = 0; j < 32; j++)
        if(IS_SET_AR(bitv, (i*32)+j))
          SET_BIT_AR(AFF_FLAGS(ch), (i*32)+j);
  } else {
    for(i = 0; i < AF_ARRAY_MAX; i++)
      for(j = 0; j < 32; j++)
        if(IS_SET_AR(bitv, (i*32)+j))
          REMOVE_BIT_AR(AFF_FLAGS(ch), (i*32)+j);
    mod = -mod;
  }

  aff_apply_modify(ch, loc, mod, "affect_modify_ar");
}

/* This updates a character by subtracting everything he is affected by
 * restoring original abilities, and then affecting all again. */
void affect_total(struct char_data *ch)
{
  struct affected_type *af;
  int i, j;

  /* First, remove all object-based modifiers. */
  for (i = 0; i < NUM_WEARS; i++) {
    if (GET_EQ(ch, i))
      for (j = 0; j < MAX_OBJ_AFFECT; j++)
        affect_modify_ar(ch,
                         GET_EQ(ch, i)->affected[j].location,
                         GET_EQ(ch, i)->affected[j].modifier,
                         GET_OBJ_AFFECT(GET_EQ(ch, i)), FALSE);
  }

  /* Then remove all spell/affect modifiers. */
  for (af = ch->affected; af; af = af->next)
    affect_modify_ar(ch, af->location, af->modifier, af->bitvector, FALSE);

  /* Reset derived abilities to real abilities. */
  ch->aff_abils = ch->real_abils;

  /* Reset transient modifiers that are re-applied below. */
  GET_PROF_MOD(ch) = 0;  /* Proficiency delta (from APPLY_PROFICIENCY) */

  /* Re-apply object-based modifiers. */
  for (i = 0; i < NUM_WEARS; i++) {
    if (GET_EQ(ch, i))
      for (j = 0; j < MAX_OBJ_AFFECT; j++)
        affect_modify_ar(ch,
                         GET_EQ(ch, i)->affected[j].location,
                         GET_EQ(ch, i)->affected[j].modifier,
                         GET_OBJ_AFFECT(GET_EQ(ch, i)), TRUE);
  }

  /* Re-apply spell/affect modifiers. */
  for (af = ch->affected; af; af = af->next)
    affect_modify_ar(ch, af->location, af->modifier, af->bitvector, TRUE);

  /* Make certain values are between 0..25, not < 0 and not > 25! */
  i = (IS_NPC(ch) || GET_LEVEL(ch) >= LVL_GRGOD) ? 25 : 18;

  GET_DEX(ch) = MAX(0, MIN(GET_DEX(ch), i));
  GET_INT(ch) = MAX(0, MIN(GET_INT(ch), i));
  GET_WIS(ch) = MAX(0, MIN(GET_WIS(ch), i));
  GET_CON(ch) = MAX(0, MIN(GET_CON(ch), i));
  GET_CHA(ch) = MAX(0, MIN(GET_CHA(ch), i));
  GET_STR(ch) = MAX(0, GET_STR(ch));

  if (IS_NPC(ch) || GET_LEVEL(ch) >= LVL_GRGOD) {
    GET_STR(ch) = MIN(GET_STR(ch), i);
  } 
}

/* Insert an affect_type in a char_data structure. Automatically sets
 * apropriate bits and apply's */
void affect_to_char(struct char_data *ch, struct affected_type *af)
{
  struct affected_type *affected_alloc;

  CREATE(affected_alloc, struct affected_type, 1);

  *affected_alloc = *af;
  affected_alloc->next = ch->affected;
  ch->affected = affected_alloc;

  affect_modify_ar(ch, af->location, af->modifier, af->bitvector, TRUE);
  affect_total(ch);
}

/* Remove an affected_type structure from a char (called when duration reaches
 * zero). Pointer *af must never be NIL!  Frees mem and calls
 * affect_location_apply */
void affect_remove(struct char_data *ch, struct affected_type *af)
{
  struct affected_type *temp;

  if (ch->affected == NULL) {
    core_dump();
    return;
  }

  affect_modify_ar(ch, af->location, af->modifier, af->bitvector, FALSE);
  REMOVE_FROM_LIST(af, ch->affected, next);
  if (af->spell == SKILL_PERCEPTION)
    clear_scan_results(ch);
  free(af);
  affect_total(ch);
}

/* Call affect_remove with every affect from the spell "type" */
void affect_from_char(struct char_data *ch, int type)
{
  struct affected_type *hjp, *next;

  for (hjp = ch->affected; hjp; hjp = next) {
    next = hjp->next;
    if (hjp->spell == type)
      affect_remove(ch, hjp);
  }
}

/* Return TRUE if a char is affected by a spell (SPELL_XXX), FALSE indicates
 * not affected. */
bool affected_by_spell(struct char_data *ch, int type)
{
  struct affected_type *hjp;

  for (hjp = ch->affected; hjp; hjp = hjp->next)
    if (hjp->spell == type)
      return (TRUE);

  return (FALSE);
}

void affect_join(struct char_data *ch, struct affected_type *af,
		      bool add_dur, bool avg_dur, bool add_mod, bool avg_mod)
{
  struct affected_type *hjp, *next;
  bool found = FALSE;

  for (hjp = ch->affected; !found && hjp; hjp = next) {
    next = hjp->next;

    if ((hjp->spell == af->spell) && (hjp->location == af->location)) {
      if (add_dur)
	af->duration += hjp->duration;
      else if (avg_dur)
        af->duration = (af->duration+hjp->duration)/2;
      if (add_mod)
	af->modifier += hjp->modifier;
      else if (avg_mod)
        af->modifier = (af->modifier+hjp->modifier)/2;

      affect_remove(ch, hjp);
      affect_to_char(ch, af);
      found = TRUE;
    }
  }
  if (!found)
    affect_to_char(ch, af);
}

/* move a player out of a room */
void char_from_room(struct char_data *ch)
{
  struct char_data *temp;

  if (ch == NULL || IN_ROOM(ch) == NOWHERE) {
    log("SYSERR: NULL character or NOWHERE in %s, char_from_room", __FILE__);
    exit(1);
  }

  if (FIGHTING(ch) != NULL)
    stop_fighting(ch);

  char_from_furniture(ch);

  if (GET_EQ(ch, WEAR_LIGHT) != NULL)
    if (GET_OBJ_TYPE(GET_EQ(ch, WEAR_LIGHT)) == ITEM_LIGHT)
      if (GET_OBJ_VAL(GET_EQ(ch, WEAR_LIGHT), 2))	/* Light is ON */
	world[IN_ROOM(ch)].light--;

  REMOVE_FROM_LIST(ch, world[IN_ROOM(ch)].people, next_in_room);
  IN_ROOM(ch) = NOWHERE;
  ch->next_in_room = NULL;
}

/* place a character in a room */
void char_to_room(struct char_data *ch, room_rnum room)
{
  if (ch == NULL || room == NOWHERE || room > top_of_world)
    log("SYSERR: Illegal value(s) passed to char_to_room. (Room: %d/%d Ch: %p",
		room, top_of_world, (void *)ch);
  else {
    ch->next_in_room = world[room].people;
    world[room].people = ch;
    IN_ROOM(ch) = room;

    autoquest_trigger_check(ch, 0, 0, AQ_ROOM_FIND);
    autoquest_trigger_check(ch, 0, 0, AQ_MOB_FIND);

    if (GET_EQ(ch, WEAR_LIGHT))
      if (GET_OBJ_TYPE(GET_EQ(ch, WEAR_LIGHT)) == ITEM_LIGHT)
	if (GET_OBJ_VAL(GET_EQ(ch, WEAR_LIGHT), 2))	/* Light ON */
	  world[room].light++;

    /* Stop fighting now, if we left. */
    if (FIGHTING(ch) && IN_ROOM(ch) != IN_ROOM(FIGHTING(ch))) {
      stop_fighting(FIGHTING(ch));
      stop_fighting(ch);
    }
  }
}

static int obj_coin_count(struct obj_data *obj)
{
  int total = 0;
  struct obj_data *child;

  if (!obj)
    return 0;

  if (GET_OBJ_TYPE(obj) == ITEM_MONEY)
    total += MAX(0, GET_OBJ_VAL(obj, 0));

  for (child = obj->contains; child; child = child->next_content)
    total += obj_coin_count(child);

  return total;
}

static struct char_data *obj_owner(struct obj_data *obj)
{
  struct obj_data *top = obj;

  while (top) {
    if (top->carried_by)
      return top->carried_by;
    if (top->worn_by)
      return top->worn_by;
    top = top->in_obj;
  }

  return NULL;
}

static void adjust_char_coins(struct char_data *ch, int amount)
{
  if (!ch || amount == 0)
    return;

  if (amount > 0)
    GET_COINS(ch) = MIN(MAX_COINS, GET_COINS(ch) + amount);
  else
    GET_COINS(ch) = MAX(0, GET_COINS(ch) + amount);
}

/* Give an object to a char. */
void obj_to_char(struct obj_data *object, struct char_data *ch)
{
  room_rnum __rs_room = RoomSave_room_of_obj(object);  /* where the item currently lives */

  if (object && ch) {
    struct char_data *old_owner = obj_owner(object);
    int coin_count = obj_coin_count(object);

    object->next_content = ch->carrying;
    ch->carrying = object;
    object->carried_by = ch;
    IN_ROOM(object) = NOWHERE;
    if (__rs_room != NOWHERE)
      RoomSave_mark_dirty_room(__rs_room);
    IS_CARRYING_W(ch) += GET_OBJ_WEIGHT(object);
    IS_CARRYING_N(ch)++;
    if (AFF_FLAGGED(ch, AFF_MOUNTED) && MOUNT(ch) && RIDDEN_BY(MOUNT(ch)) == ch)
      IS_CARRYING_W(MOUNT(ch)) += GET_OBJ_WEIGHT(object);

    autoquest_trigger_check(ch, NULL, object, AQ_OBJ_FIND);

    /* set flag for crash-save system, but not on mobs! */
    if (!IS_NPC(ch))
      SET_BIT_AR(PLR_FLAGS(ch), PLR_CRASH);

    if (coin_count > 0 && old_owner != ch) {
      adjust_char_coins(old_owner, -coin_count);
      adjust_char_coins(ch, coin_count);
    }
  } else
    log("SYSERR: NULL obj (%p) or char (%p) passed to obj_to_char.", (void *)object, (void *)ch);
}

/* take an object from a char */
void obj_from_char(struct obj_data *object)
{
  struct obj_data *temp;
  room_rnum __rs_room = IN_ROOM(object->carried_by);

  if (object == NULL) {
    log("SYSERR: NULL object passed to obj_from_char.");
    return;
  }
  {
    struct char_data *old_owner = obj_owner(object);
    int coin_count = obj_coin_count(object);

    if (coin_count > 0 && old_owner) {
      adjust_char_coins(old_owner, -coin_count);
    }
  }
  REMOVE_FROM_LIST(object, object->carried_by->carrying, next_content);

  /* set flag for crash-save system, but not on mobs! */
  if (!IS_NPC(object->carried_by))
    SET_BIT_AR(PLR_FLAGS(object->carried_by), PLR_CRASH);

  IS_CARRYING_W(object->carried_by) -= GET_OBJ_WEIGHT(object);
  IS_CARRYING_N(object->carried_by)--;
  if (AFF_FLAGGED(object->carried_by, AFF_MOUNTED) &&
      MOUNT(object->carried_by) &&
      RIDDEN_BY(MOUNT(object->carried_by)) == object->carried_by)
    IS_CARRYING_W(MOUNT(object->carried_by)) =
      MAX(0, IS_CARRYING_W(MOUNT(object->carried_by)) - GET_OBJ_WEIGHT(object));
  object->carried_by = NULL;
  object->next_content = NULL;
  if (__rs_room != NOWHERE)
    RoomSave_mark_dirty_room(__rs_room);
}

/* Return the effect of a piece of armor in position eq_pos */
static int apply_ac(struct char_data *ch, int eq_pos)
{
  int factor;

  if (GET_EQ(ch, eq_pos) == NULL) {
    core_dump();
    return (0);
  }

  if (!(GET_OBJ_TYPE(GET_EQ(ch, eq_pos)) == ITEM_ARMOR))
    return (0);

  switch (eq_pos) {

  case WEAR_BODY:
    factor = 3;
    break;			/* 30% */
  case WEAR_HEAD:
    factor = 2;
    break;			/* 20% */
  case WEAR_LEGS:
    factor = 2;
    break;			/* 20% */
  default:
    factor = 1;
    break;			/* all others 10% */
  }

  return (factor * GET_OBJ_VAL(GET_EQ(ch, eq_pos), 0));
}

void equip_char(struct char_data *ch, struct obj_data *obj, int pos)
{
  int j;

  if (pos < 0 || pos >= NUM_WEARS) {
    core_dump();
    return;
  }

  if (GET_EQ(ch, pos)) {
    log("SYSERR: Char is already equipped: %s, %s", GET_NAME(ch),
	    obj->short_description);
    return;
  }
  if (obj->carried_by) {
    log("SYSERR: EQUIP: Obj is carried_by when equip.");
    return;
  }
  if (IN_ROOM(obj) != NOWHERE) {
    log("SYSERR: EQUIP: Obj is in_room when equip.");
    return;
  }
  if (invalid_class(ch, obj)) {
    act("You are zapped by $p and instantly let go of it.", FALSE, ch, obj, 0, TO_CHAR);
    act("$n is zapped by $p and instantly lets go of it.", FALSE, ch, obj, 0, TO_ROOM);
    /* Changed to drop in inventory instead of the ground. */
    obj_to_char(obj, ch);
    return;
  }

  GET_EQ(ch, pos) = obj;
  obj->worn_by = ch;
  obj->worn_on = pos;
  IS_CARRYING_W(ch) += GET_OBJ_WEIGHT(obj);
  if (AFF_FLAGGED(ch, AFF_MOUNTED) && MOUNT(ch) && RIDDEN_BY(MOUNT(ch)) == ch)
    IS_CARRYING_W(MOUNT(ch)) += GET_OBJ_WEIGHT(obj);

  if (GET_OBJ_TYPE(obj) == ITEM_ARMOR)
    GET_AC(ch) -= apply_ac(ch, pos);

  if (IN_ROOM(ch) != NOWHERE) {
    if (pos == WEAR_LIGHT && GET_OBJ_TYPE(obj) == ITEM_LIGHT)
      if (GET_OBJ_VAL(obj, 2))	/* if light is ON */
	world[IN_ROOM(ch)].light++;
  } else
    log("SYSERR: IN_ROOM(ch) = NOWHERE when equipping char %s.", GET_NAME(ch));

  for (j = 0; j < MAX_OBJ_AFFECT; j++)
    affect_modify_ar(ch, obj->affected[j].location,
		  obj->affected[j].modifier,
		  GET_OBJ_AFFECT(obj), TRUE);

  affect_total(ch);
}

struct obj_data *unequip_char(struct char_data *ch, int pos)
{
  int j;
  struct obj_data *obj;

  if ((pos < 0 || pos >= NUM_WEARS) || GET_EQ(ch, pos) == NULL) {
    core_dump();
    return (NULL);
  }

  obj = GET_EQ(ch, pos);
  obj->worn_by = NULL;
  obj->worn_on = -1;
  IS_CARRYING_W(ch) -= GET_OBJ_WEIGHT(obj);
  if (AFF_FLAGGED(ch, AFF_MOUNTED) && MOUNT(ch) && RIDDEN_BY(MOUNT(ch)) == ch)
    IS_CARRYING_W(MOUNT(ch)) =
      MAX(0, IS_CARRYING_W(MOUNT(ch)) - GET_OBJ_WEIGHT(obj));

  if (GET_OBJ_TYPE(obj) == ITEM_ARMOR)
    GET_AC(ch) += apply_ac(ch, pos);

  if (IN_ROOM(ch) != NOWHERE) {
    if (pos == WEAR_LIGHT && GET_OBJ_TYPE(obj) == ITEM_LIGHT)
      if (GET_OBJ_VAL(obj, 2))	/* if light is ON */
	world[IN_ROOM(ch)].light--;
  } else
    log("SYSERR: IN_ROOM(ch) = NOWHERE when unequipping char %s.", GET_NAME(ch));

  GET_EQ(ch, pos) = NULL;

  for (j = 0; j < MAX_OBJ_AFFECT; j++)
    affect_modify_ar(ch, obj->affected[j].location,
		  obj->affected[j].modifier,
		  GET_OBJ_AFFECT(obj), FALSE);

  affect_total(ch);

  return (obj);
}

int get_number(char **name)
{
  int i;
  char *ppos;
  char number[MAX_INPUT_LENGTH];

  *number = '\0';

  if ((ppos = strchr(*name, '.')) != NULL) {
    *ppos++ = '\0';
    strlcpy(number, *name, sizeof(number));
    strcpy(*name, ppos);	/* strcpy: OK (always smaller) */

    for (i = 0; *(number + i); i++)
      if (!isdigit(*(number + i)))
	return (0);

    return (atoi(number));
  }
  return (1);
}

/* Search a given list for an object number, and return a ptr to that obj */
struct obj_data *get_obj_in_list_num(int num, struct obj_data *list)
{
  struct obj_data *i;

  for (i = list; i; i = i->next_content)
    if (GET_OBJ_RNUM(i) == num)
      return (i);

  return (NULL);
}

/* search the entire world for an object number, and return a pointer  */
struct obj_data *get_obj_num(obj_rnum nr)
{
  struct obj_data *i;

  for (i = object_list; i; i = i->next)
    if (GET_OBJ_RNUM(i) == nr)
      return (i);

  return (NULL);
}

/* search a room for a char, and return a pointer if found..  */
struct char_data *get_char_room(char *name, int *number, room_rnum room)
{
  struct char_data *i;
  int num;

  if (!number) {
    number = &num;
    num = get_number(&name);
  }

  if (*number == 0)
    return (NULL);

  for (i = world[room].people; i && *number; i = i->next_in_room)
    if (isname(name, i->player.name))
      if (--(*number) == 0)
	return (i);

  return (NULL);
}

/* search all over the world for a char num, and return a pointer if found */
struct char_data *get_char_num(mob_rnum nr)
{
  struct char_data *i;

  for (i = character_list; i; i = i->next)
    if (GET_MOB_RNUM(i) == nr)
      return (i);

  return (NULL);
}

/* put an object in a room */
void obj_to_room(struct obj_data *object, room_rnum room)
{
  if (!object || room == NOWHERE || room > top_of_world){
    log("SYSERR: Illegal value(s) passed to obj_to_room. (Room #%d/%d, obj %p)",
	room, top_of_world, (void *)object);
  }
  else {
    if (world[room].contents == NULL){  // if list is empty
      world[room].contents = object; // add object to list
    }
    else {
      struct obj_data *i = world[room].contents; // define a temporary pointer
      while (i->next_content != NULL) i = i->next_content; // find the first without a next_content
        i->next_content = object; // add object at the end
    }
    object->next_content = NULL; // mostly for sanity. should do nothing.
    IN_ROOM(object) = room;
    object->carried_by = NULL;
    if (ROOM_FLAGGED(room, ROOM_HOUSE))
      SET_BIT_AR(ROOM_FLAGS(room), ROOM_HOUSE_CRASH);
    /* RoomSave: this room’s contents changed */
    RoomSave_mark_dirty_room(room);
  }
}

/* Take an object from a room */
void obj_from_room(struct obj_data *object)
{
  struct obj_data *temp;
  struct char_data *t, *tempch;
  room_rnum __rs_was_room = IN_ROOM(object);

  if (!object || IN_ROOM(object) == NOWHERE) {
    log("SYSERR: NULL object (%p) or obj not in a room (%d) passed to obj_from_room",
	(void *)object, IN_ROOM(object));
    return;
  }

   /* if people are sitting in it, toss their butt to the ground */
  if (OBJ_SAT_IN_BY(object) != NULL) {
    for (tempch = OBJ_SAT_IN_BY(object); tempch; tempch = t) {
       t = NEXT_SITTING(tempch);
       SITTING(tempch) = NULL;
       NEXT_SITTING(tempch) = NULL;
    }
  }

  REMOVE_FROM_LIST(object, world[IN_ROOM(object)].contents, next_content);

  if (ROOM_FLAGGED(IN_ROOM(object), ROOM_HOUSE))
    SET_BIT_AR(ROOM_FLAGS(IN_ROOM(object)), ROOM_HOUSE_CRASH);
  IN_ROOM(object) = NOWHERE;
  object->next_content = NULL;
  /* RoomSave: room lost an object */
  RoomSave_mark_dirty_room(__rs_was_room);
}

/* put an object in an object (quaint)  */
void obj_to_obj(struct obj_data *obj, struct obj_data *obj_to)
{
  struct obj_data *tmp_obj;

  if (!obj || !obj_to || obj == obj_to) {
    log("SYSERR: NULL object (%p) or same source (%p) and target (%p) obj passed to obj_to_obj.",
	(void *)obj, (void *)obj, (void *)obj_to);
    return;
  }
  {
    struct char_data *old_owner = obj_owner(obj);
    struct char_data *new_owner = obj_owner(obj_to);
    int coin_count = obj_coin_count(obj);

    if (coin_count > 0 && old_owner != new_owner) {
      adjust_char_coins(old_owner, -coin_count);
      adjust_char_coins(new_owner, coin_count);
    }
  }

  obj->next_content = obj_to->contains;
  obj_to->contains = obj;
  obj->in_obj = obj_to;
  /* RoomSave: container changed; mark the room the container ultimately lives in */
  RoomSave_mark_dirty_room(RoomSave_room_of_obj(obj_to));

  /* Add weight to container, unless unlimited. */
  if (GET_OBJ_VAL(obj->in_obj, 0) > 0) {
    for (tmp_obj = obj->in_obj; tmp_obj->in_obj; tmp_obj = tmp_obj->in_obj)
      GET_OBJ_WEIGHT(tmp_obj) += GET_OBJ_WEIGHT(obj);

    /* top level object.  Subtract weight from inventory if necessary. */
    GET_OBJ_WEIGHT(tmp_obj) += GET_OBJ_WEIGHT(obj);
    if (tmp_obj->carried_by)
      IS_CARRYING_W(tmp_obj->carried_by) += GET_OBJ_WEIGHT(obj);
    else if (tmp_obj->worn_by)
      IS_CARRYING_W(tmp_obj->worn_by) += GET_OBJ_WEIGHT(obj);
  }
}

/* remove an object from an object */
void obj_from_obj(struct obj_data *obj)
{
  struct obj_data *temp, *obj_from;
  struct obj_data *__rs_container = obj->in_obj;  /* capture before unlink */

  if (obj->in_obj == NULL) {
    log("SYSERR: (%s): trying to illegally extract obj from obj.", __FILE__);
    return;
  }
  {
    struct char_data *old_owner = obj_owner(obj);
    int coin_count = obj_coin_count(obj);

    if (coin_count > 0 && old_owner) {
      adjust_char_coins(old_owner, -coin_count);
    }
  }
  obj_from = obj->in_obj;
  REMOVE_FROM_LIST(obj, obj_from->contains, next_content);

  /* Subtract weight from containers container unless unlimited. */
  if (GET_OBJ_VAL(obj->in_obj, 0) > 0) {
    for (temp = obj->in_obj; temp->in_obj; temp = temp->in_obj)
      GET_OBJ_WEIGHT(temp) -= GET_OBJ_WEIGHT(obj);

    /* Subtract weight from char that carries the object */
    GET_OBJ_WEIGHT(temp) -= GET_OBJ_WEIGHT(obj);
    if (temp->carried_by)
      IS_CARRYING_W(temp->carried_by) -= GET_OBJ_WEIGHT(obj);
    else if (temp->worn_by)
      IS_CARRYING_W(temp->worn_by) -= GET_OBJ_WEIGHT(obj);
  }
  obj->in_obj = NULL;
  obj->next_content = NULL;
  /* RoomSave: container changed; mark the room the container ultimately lives in */
  RoomSave_mark_dirty_room(RoomSave_room_of_obj(__rs_container));
}

/* Set all carried_by to point to new owner */
void object_list_new_owner(struct obj_data *list, struct char_data *ch)
{
  if (list) {
    object_list_new_owner(list->contains, ch);
    object_list_new_owner(list->next_content, ch);
    list->carried_by = ch;
  }
}

/* Extract an object from the world */
void extract_obj(struct obj_data *obj)
{
  struct char_data *ch, *next = NULL;
  struct obj_data *temp;
  room_rnum __rs_room = RoomSave_room_of_obj(obj);

  if (obj->worn_by != NULL)
    if (unequip_char(obj->worn_by, obj->worn_on) != obj)
      log("SYSERR: Inconsistent worn_by and worn_on pointers!!");
  if (IN_ROOM(obj) != NOWHERE)
    obj_from_room(obj);
  else if (obj->carried_by)
    obj_from_char(obj);
  else if (obj->in_obj)
    obj_from_obj(obj);

  if (OBJ_SAT_IN_BY(obj)){
    for (ch = OBJ_SAT_IN_BY(obj); OBJ_SAT_IN_BY(obj); ch = next){
      if (!NEXT_SITTING(ch))
        OBJ_SAT_IN_BY(obj) = NULL;
      else
        OBJ_SAT_IN_BY(obj) = (next = NEXT_SITTING(ch));
      SITTING(ch) = NULL;
      NEXT_SITTING(ch) = NULL;
    }
  }

  /* Get rid of the contents of the object, as well. */
  while (obj->contains)
    extract_obj(obj->contains);

  REMOVE_FROM_LIST(obj, object_list, next);

  if (GET_OBJ_RNUM(obj) != NOTHING)
    (obj_index[GET_OBJ_RNUM(obj)].number)--;

  if (SCRIPT(obj))
    extract_script(obj, OBJ_TRIGGER);

  if (obj->events != NULL) {
	  if (obj->events->iSize > 0) {
		struct event * pEvent;

		while ((pEvent = simple_list(obj->events)) != NULL)
		  event_cancel(pEvent);
	  }
	  free_list(obj->events);
    obj->events = NULL;
  }

  if (GET_OBJ_RNUM(obj) == NOTHING || obj->proto_script != obj_proto[GET_OBJ_RNUM(obj)].proto_script)
    free_proto_script(obj, OBJ_TRIGGER);

  if (__rs_room != NOWHERE)
    RoomSave_mark_dirty_room(__rs_room);
    
  free_obj(obj);
}

static void update_object(struct obj_data *obj, int use)
{
  /* dont update objects with a timer trigger */
  if (!SCRIPT_CHECK(obj, OTRIG_TIMER) && (GET_OBJ_TIMER(obj) > 0))
    GET_OBJ_TIMER(obj) -= use;
  if (obj->contains)
    update_object(obj->contains, use);
  if (obj->next_content)
    update_object(obj->next_content, use);
}

void update_char_objects(struct char_data *ch)
{
  int i;

  if (GET_EQ(ch, WEAR_LIGHT) != NULL)
    if (GET_OBJ_TYPE(GET_EQ(ch, WEAR_LIGHT)) == ITEM_LIGHT)
      if (GET_OBJ_VAL(GET_EQ(ch, WEAR_LIGHT), 2) > 0) {
	i = --GET_OBJ_VAL(GET_EQ(ch, WEAR_LIGHT), 2);
	if (i == 1) {
	  send_to_char(ch, "Your light begins to flicker and fade.\r\n");
	  act("$n's light begins to flicker and fade.", FALSE, ch, 0, 0, TO_ROOM);
	} else if (i == 0) {
	  send_to_char(ch, "Your light sputters out and dies.\r\n");
	  act("$n's light sputters out and dies.", FALSE, ch, 0, 0, TO_ROOM);
	  world[IN_ROOM(ch)].light--;
	}
      }

  for (i = 0; i < NUM_WEARS; i++)
    if (GET_EQ(ch, i))
      update_object(GET_EQ(ch, i), 2);

  if (ch->carrying)
    update_object(ch->carrying, 1);
}

/* Extract a ch completely from the world, and leave his stuff behind */
void extract_char_final(struct char_data *ch)
{
  struct char_data *k, *temp;
  struct descriptor_data *d;
  struct obj_data *obj;
  int i;

  if (IN_ROOM(ch) == NOWHERE) {
    log("SYSERR: NOWHERE extracting char %s. (%s, extract_char_final)",
        GET_NAME(ch), __FILE__);
    exit(1);
  }

  /* We're booting the character of someone who has switched so first we need
   * to stuff them back into their own body.  This will set ch->desc we're
   * checking below this loop to the proper value. */
  if (!IS_NPC(ch) && !ch->desc) {
    for (d = descriptor_list; d; d = d->next)
      if (d->original == ch) {
	do_return(d->character, NULL, 0, 0);
        break;
      }
  }

  if (ch->desc) {
    /* This time we're extracting the body someone has switched into (not the
     * body of someone switching as above) so we need to put the switcher back
     * to their own body. If this body is not possessed, the owner won't have a
     * body after the removal so dump them to the main menu. */
    if (ch->desc->original)
      do_return(ch, NULL, 0, 0);
    else {
      /* Now we boot anybody trying to log in with the same character, to help
       * guard against duping.  CON_DISCONNECT is used to close a descriptor
       * without extracting the d->character associated with it, for being
       * link-dead, so we want CON_CLOSE to clean everything up. If we're
       * here, we know it's a player so no IS_NPC check required. */
      for (d = descriptor_list; d; d = d->next) {
        if (d == ch->desc)
          continue;
        if (d->character && GET_IDNUM(ch) == GET_IDNUM(d->character))
          STATE(d) = CON_CLOSE;
      }
      if (GET_POS(ch) == POS_DEAD) {
        int pfilepos = GET_PFILEPOS(ch);

        if (pfilepos < 0)
          pfilepos = get_ptable_by_name(GET_NAME(ch));
        if (pfilepos >= 0)
          SET_BIT(player_table[pfilepos].flags, PINDEX_DELETED);

        STATE(ch->desc) = CON_ACCOUNT_MENU;
        send_account_menu(ch->desc);
        ch->desc->character = NULL;
        ch->desc = NULL;
      } else {
        STATE(ch->desc) = CON_ACCOUNT_MENU;
        send_account_menu(ch->desc);
      }
    }
  }

  if (AFF_FLAGGED(ch, AFF_MOUNTED) || MOUNT(ch)) {
    struct char_data *mount = MOUNT(ch);
    if (mount && RIDDEN_BY(mount) == ch) {
      int rider_weight = GET_WEIGHT(ch) + IS_CARRYING_W(ch);
      IS_CARRYING_W(mount) = MAX(0, IS_CARRYING_W(mount) - rider_weight);
      RIDDEN_BY(mount) = NULL;
    }
    MOUNT(ch) = NULL;
    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_MOUNTED);
  }
  if (RIDDEN_BY(ch)) {
    struct char_data *rider = RIDDEN_BY(ch);
    if (rider && MOUNT(rider) == ch) {
      MOUNT(rider) = NULL;
      REMOVE_BIT_AR(AFF_FLAGS(rider), AFF_MOUNTED);
    }
    RIDDEN_BY(ch) = NULL;
  }

  /* On with the character's assets... */
  if (ch->followers || ch->master)
    die_follower(ch);

  /* Check to see if we are grouped! */
  if (GROUP(ch))
    leave_group(ch);

  /* Only drop items for NPCs or players not quitting cleanly. */
  if (IS_NPC(ch) || !PLR_FLAGGED(ch, PLR_QUITING)) {
    /* transfer objects to room, if any */
    while (ch->carrying) {
      obj = ch->carrying;
      obj_from_char(obj);
      obj_to_room(obj, IN_ROOM(ch));
    }

    /* transfer equipment to room, if any */
    for (i = 0; i < NUM_WEARS; i++)
      if (GET_EQ(ch, i))
        obj_to_room(unequip_char(ch, i), IN_ROOM(ch));
  }

  if (FIGHTING(ch))
    stop_fighting(ch);

  for (k = combat_list; k; k = temp) {
    temp = k->next_fighting;
    if (FIGHTING(k) == ch)
      stop_fighting(k);
  }
  
  /* Whipe character from the memory of hunters and other intelligent NPCs... */
  for (temp = character_list; temp; temp = temp->next) {
    /* PCs can't use MEMORY, and don't use HUNTING() */
    if (!IS_NPC(temp))
      continue;
    /* If "temp" is hunting our extracted char, stop the hunt. */
    if (HUNTING(temp) == ch)
      HUNTING(temp) = NULL;
    /* If "temp" has allocated memory data and our ch is a PC, forget the 
     * extracted character (if he/she is remembered) */  
    if (!IS_NPC(ch) && GET_POS(ch) == POS_DEAD && MEMORY(temp))
      forget(temp, ch); /* forget() is safe to use without a check. */
  }

  char_from_room(ch);

  if (IS_NPC(ch)) {
    if (GET_MOB_RNUM(ch) != NOTHING)	/* prototyped */
      mob_index[GET_MOB_RNUM(ch)].number--;
    clearMemory(ch);

    if (SCRIPT(ch))
      extract_script(ch, MOB_TRIGGER);

    if (SCRIPT_MEM(ch))
      extract_script_mem(SCRIPT_MEM(ch));
  } else {
    if (GET_POS(ch) == POS_DEAD) {
      int pfilepos = GET_PFILEPOS(ch);

      if (pfilepos < 0)
        pfilepos = get_ptable_by_name(GET_NAME(ch));

      if (pfilepos >= 0) {
        remove_player(pfilepos);
      } else {
        char filename[PATH_MAX];
        int i;

        log("SYSERR: Could not locate player index entry for %s on death cleanup.",
            GET_NAME(ch));
        for (i = 0; i < MAX_FILES; i++) {
          if (get_filename(filename, sizeof(filename), i, GET_NAME(ch)))
            unlink(filename);
        }
      }
    } else {
      save_char(ch);
      Crash_delete_crashfile(ch);
      REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_QUITING);
    }
  }

  /* If there's a descriptor, they're in the menu now. */
  if (IS_NPC(ch) || !ch->desc)
    free_char(ch);
}

/* Why do we do this? Because trying to iterate over the character list with
 * 'ch = ch->next' does bad things if the current character happens to die. The
 * trivial workaround of 'vict = next_vict' doesn't work if the _next_ person
 * in the list gets killed, for example, by an area spell. Why do we leave them
 * on the character_list? Because code doing 'vict = vict->next' would get
 * really confused otherwise.
 *
 * Fixed a bug where it would over-count extractions if you try to extract the
 * same character twice (e.g. double-purging in a script) -khufu / EmpireMUD
 */
void extract_char(struct char_data *ch)
{
  char_from_furniture(ch);
  clear_char_event_list(ch);

  if (IS_NPC(ch) && !MOB_FLAGGED(ch, MOB_NOTDEADYET)) {
    SET_BIT_AR(MOB_FLAGS(ch), MOB_NOTDEADYET);
    ++extractions_pending;
  }
  else if (!IS_NPC(ch) && !PLR_FLAGGED(ch, PLR_NOTDEADYET)) {
    SET_BIT_AR(PLR_FLAGS(ch), PLR_NOTDEADYET);
    ++extractions_pending;
  }
}

/* I'm not particularly pleased with the MOB/PLR hoops that have to be jumped
 * through but it hardly calls for a completely new variable. Ideally it would
 * be its own list, but that would change the '->next' pointer, potentially
 * confusing some code. -gg This doesn't handle recursive extractions. */
void extract_pending_chars(void)
{
  struct char_data *vict, *next_vict, *prev_vict;

  if (extractions_pending < 0)
    log("SYSERR: Negative (%d) extractions pending.", extractions_pending);

  for (vict = character_list, prev_vict = NULL; vict && extractions_pending; vict = next_vict) {
    next_vict = vict->next;

    if (MOB_FLAGGED(vict, MOB_NOTDEADYET))
      REMOVE_BIT_AR(MOB_FLAGS(vict), MOB_NOTDEADYET);
    else if (PLR_FLAGGED(vict, PLR_NOTDEADYET))
      REMOVE_BIT_AR(PLR_FLAGS(vict), PLR_NOTDEADYET);
    else {
      /* Last non-free'd character to continue chain from. */
      prev_vict = vict;
      continue;
    }

    extract_char_final(vict);
    extractions_pending--;

    if (prev_vict)
      prev_vict->next = next_vict;
    else
      character_list = next_vict;
  }

  if (extractions_pending > 0)
    log("SYSERR: Couldn't find %d extractions as counted.", extractions_pending);

  extractions_pending = 0;
}

/* Here follows high-level versions of some earlier routines, ie functions
 * which incorporate the actual player-data */
struct char_data *get_player_vis(struct char_data *ch, char *name, int *number, int inroom)
{
  struct char_data *i;
  int num;

  if (!number) {
    number = &num;
    num = get_number(&name);
  }

  for (i = character_list; i; i = i->next) {
    if (IS_NPC(i))
      continue;
    if (inroom == FIND_CHAR_ROOM && IN_ROOM(i) != IN_ROOM(ch))
      continue;
    if (str_cmp(i->player.name, name)) /* If not same, continue */
      continue;
    if (!CAN_SEE(ch, i))
      continue;
    if (--(*number) != 0)
      continue;
    return (i);
  }

  return (NULL);
}

struct char_data *get_char_room_vis(struct char_data *ch, char *name, int *number)
{
  struct char_data *i;
  int num;

  if (!number) {
    number = &num;
    num = get_number(&name);
  }

  /* JE */
  if (!str_cmp(name, "self") || !str_cmp(name, "me"))
    return ch;

  /* 0.<name> means PC with name */
  if (*number == 0)
    return get_player_vis(ch, name, NULL, FIND_CHAR_ROOM);

  for (i = world[IN_ROOM(ch)].people; i && *number; i = i->next_in_room) {
    bool match = FALSE;

    if (IS_NPC(i)) {
      /* NPCs: match either keywords or their name (unchanged) */
      const char *keywords = GET_KEYWORDS(i);
      const char *proper   = GET_NAME(i);

      if ((keywords && isname(name, keywords)) ||
          (proper   && isname(name, proper)))
        match = TRUE;

    } else {
      /* PCs: match against name + sanitized short description */
      const char *proper = GET_NAME(i);
      const char *sdesc  = GET_SHORT_DESC(i);

      if (sdesc && *sdesc) {
        char clean_sdesc[MAX_INPUT_LENGTH];
        char tmp[MAX_INPUT_LENGTH * 2];
        int w = 0;

        /* Turn punctuation etc. into spaces so "tall," -> "tall" */
        for (int r = 0; sdesc[r] && w < (int)sizeof(clean_sdesc) - 1; r++) {
          unsigned char c = (unsigned char)sdesc[r];

          if (isalnum(c) || c == '\'' || c == '-') {
            clean_sdesc[w++] = c;
          } else {
            /* normalize anything else (spaces, commas, etc.) to a single space */
            clean_sdesc[w++] = ' ';
          }
        }
        clean_sdesc[w] = '\0';

        if (proper && *proper)
          snprintf(tmp, sizeof(tmp), "%s %s", proper, clean_sdesc);
        else
          snprintf(tmp, sizeof(tmp), "%s", clean_sdesc);

        if (isname(name, tmp))
          match = TRUE;
      } else if (proper && isname(name, proper)) {
        /* Fallback: no sdesc yet, use name only */
        match = TRUE;
      }
    }

    if (match) {
      bool can_target = CAN_SEE(ch, i);

      if (!can_target && GET_LEVEL(ch) >= LVL_IMMORT)
        can_target = TRUE;

      if (!can_target &&
          AFF_FLAGGED(ch, AFF_SCAN) &&
          AFF_FLAGGED(i, AFF_HIDE) &&
          scan_can_target(ch, i)) {
        can_target = scan_confirm_target(ch, i);
      }

      if (can_target && --(*number) == 0)
        return i;
    }
  }

  if ((AFF_FLAGGED(ch, AFF_SCAN) || GET_LEVEL(ch) >= LVL_IMMORT) && is_shadow_keyword(name)) {
    for (i = world[IN_ROOM(ch)].people; i && *number; i = i->next_in_room) {
      if (i == ch)
        continue;
      if (!AFF_FLAGGED(i, AFF_HIDE))
        continue;
      if (GET_LEVEL(ch) < LVL_IMMORT && !scan_can_target(ch, i))
        continue;

      if (--(*number) == 0) {
        if (GET_LEVEL(ch) >= LVL_IMMORT || scan_confirm_target(ch, i))
          return i;
        return NULL;
      }
    }
  }

  return NULL;
}

struct char_data *get_char_world_vis(struct char_data *ch, char *name, int *number)
{
  struct char_data *i;
  int num;

  if (!number) {
    number = &num;
    num = get_number(&name);
  }

  /* First, try to find character in the same room */
  if ((i = get_char_room_vis(ch, name, number)) != NULL)
    return (i);

  /* 0.<name> means PC with name */
  if (*number == 0)
    return get_player_vis(ch, name, NULL, 0);

  for (i = character_list; i && *number; i = i->next) {
    if (IN_ROOM(ch) == IN_ROOM(i))
      continue;

    bool match = FALSE;

    if (IS_NPC(i)) {
      const char *keywords = GET_KEYWORDS(i);
      const char *proper   = GET_NAME(i);
      if ((keywords && isname(name, keywords)) || (proper && isname(name, proper)))
        match = TRUE;
    } else {
      const char *namelist = GET_NAME(i);
      if (namelist && isname(name, namelist))
        match = TRUE;
    }

    if (!match)
      continue;

    bool can_target = CAN_SEE(ch, i);

    if (!can_target && GET_LEVEL(ch) >= LVL_IMMORT)
      can_target = TRUE;

    if (!can_target &&
        AFF_FLAGGED(ch, AFF_SCAN) &&
        AFF_FLAGGED(i, AFF_HIDE) &&
        scan_can_target(ch, i)) {
      can_target = scan_confirm_target(ch, i);
    }

    if (!can_target)
      continue;

    if (--(*number) != 0)
      continue;

    return (i);
  }

  return (NULL);
}

struct char_data *get_char_vis(struct char_data *ch, char *name, int *number, int where)
{
  if (where == FIND_CHAR_ROOM)
    return get_char_room_vis(ch, name, number);
  else if (where == FIND_CHAR_WORLD)
    return get_char_world_vis(ch, name, number);
  else
    return (NULL);
}

struct obj_data *get_obj_in_list_vis(struct char_data *ch, char *name, int *number, struct obj_data *list)
{
  struct obj_data *i;
  int num;

  if (!number) {
    number = &num;
    num = get_number(&name);
  }

  if (*number == 0)
    return (NULL);

  for (i = list; i && *number; i = i->next_content)
    if (isname(name, i->name))
      if (CAN_SEE_OBJ(ch, i))
	if (--(*number) == 0)
	  return (i);

  return (NULL);
}

/* search the entire world for an object, and return a pointer  */
struct obj_data *get_obj_vis(struct char_data *ch, char *name, int *number)
{
  struct obj_data *i;
  int num;

  if (!number) {
    number = &num;
    num = get_number(&name);
  }

  if (*number == 0)
    return (NULL);

  /* scan items carried */
  if ((i = get_obj_in_list_vis(ch, name, number, ch->carrying)) != NULL)
    return (i);

  /* scan room */
  if ((i = get_obj_in_list_vis(ch, name, number, world[IN_ROOM(ch)].contents)) != NULL)
    return (i);

  /* ok.. no luck yet. scan the entire obj list   */
  for (i = object_list; i && *number; i = i->next)
    if (isname(name, i->name))
      if (CAN_SEE_OBJ(ch, i))
	if (--(*number) == 0)
	  return (i);

  return (NULL);
}

struct obj_data *get_obj_in_equip_vis(struct char_data *ch, char *arg, int *number, struct obj_data *equipment[])
{
  int j, num;

  if (!number) {
    number = &num;
    num = get_number(&arg);
  }

  if (*number == 0)
    return (NULL);

  for (j = 0; j < NUM_WEARS; j++)
    if (equipment[j] && CAN_SEE_OBJ(ch, equipment[j]) && isname(arg, equipment[j]->name))
      if (--(*number) == 0)
        return (equipment[j]);

  return (NULL);
}

int get_obj_pos_in_equip_vis(struct char_data *ch, char *arg, int *number, struct obj_data *equipment[])
{
  int j, num;

  if (!number) {
    number = &num;
    num = get_number(&arg);
  }

  if (*number == 0)
    return (-1);

  for (j = 0; j < NUM_WEARS; j++)
    if (equipment[j] && CAN_SEE_OBJ(ch, equipment[j]) && isname(arg, equipment[j]->name))
      if (--(*number) == 0)
        return (j);

  return (-1);
}

static int money_weight(int amount)
{
  const int coins_per_weight = 30;

  if (amount <= 0)
    return 0;

  return MAX(1, (amount + coins_per_weight - 1) / coins_per_weight);
}

const char *money_desc(int amount)
{
  if (amount <= 0) {
    log("SYSERR: Try to describe negative or 0 coins (%d).", amount);
    return (NULL);
  }

  if (amount == 1)
    return "a single ceramic coin";
  if (amount == 2)
    return "a couple ceramic coins";
  if (amount < 10)
    return "a few ceramic coins";
  return "a pile of ceramic coins";
}

const char *money_pile_desc(int piles)
{
  if (piles <= 0)
    return NULL;
  if (piles == 1)
    return "a pile of ceramic coins";
  if (piles == 2)
    return "a couple piles of ceramic coins";
  if (piles <= 5)
    return "a few piles of ceramic coins";
  if (piles <= 9)
    return "several piles of ceramic coins";
  return "many piles of ceramic coins";
}

void update_money_obj(struct obj_data *obj)
{
  struct extra_descr_data *new_descr;
  const char *desc;
  char buf[200];
  int amount;
  int new_weight;
  int delta_weight;

  if (!obj || GET_OBJ_TYPE(obj) != ITEM_MONEY)
    return;

  amount = MAX(0, GET_OBJ_VAL(obj, 0));
  desc = money_desc(amount);

  if (obj->name && (obj->item_number == NOTHING ||
      obj->name != obj_proto[obj->item_number].name)) {
    free(obj->name);
  }
  obj->name = strdup("coin coins ceramic");

  if (obj->short_description && (obj->item_number == NOTHING ||
      obj->short_description != obj_proto[obj->item_number].short_description)) {
    free(obj->short_description);
  }
  obj->short_description = desc ? strdup(desc) : strdup("ceramic coins");

  if (obj->description && (obj->item_number == NOTHING ||
      obj->description != obj_proto[obj->item_number].description)) {
    free(obj->description);
  }

  if (amount == 1)
    snprintf(buf, sizeof(buf), "A single ceramic coin is lying here.");
  else if (amount == 2)
    snprintf(buf, sizeof(buf), "A couple ceramic coins are lying here.");
  else if (amount < 10)
    snprintf(buf, sizeof(buf), "A few ceramic coins are lying here.");
  else
    snprintf(buf, sizeof(buf), "A pile of ceramic coins is lying here.");
  obj->description = strdup(buf);

  if (!obj->ex_description) {
    CREATE(new_descr, struct extra_descr_data, 1);
    new_descr->next = NULL;
    obj->ex_description = new_descr;
  }
  new_descr = obj->ex_description;

  if (new_descr->keyword)
    free(new_descr->keyword);
  new_descr->keyword = strdup("coin coins ceramic");

  if (new_descr->description)
    free(new_descr->description);

  if (amount < 10)
    snprintf(buf, sizeof(buf), "There are %d ceramic coins.", amount);
  else if (amount < 100)
    snprintf(buf, sizeof(buf), "There are about %d ceramic coins.", 10 * (amount / 10));
  else if (amount < 1000)
    snprintf(buf, sizeof(buf), "It looks to be about %d ceramic coins.", 100 * (amount / 100));
  else if (amount < 100000)
    snprintf(buf, sizeof(buf), "You guess there are, maybe, %d ceramic coins.",
	     1000 * ((amount / 1000) + rand_number(0, (amount / 1000))));
  else
    strcpy(buf, "There are a LOT of ceramic coins.");	/* strcpy: OK (is < 200) */
  new_descr->description = strdup(buf);

  SET_BIT_AR(GET_OBJ_WEAR(obj), ITEM_WEAR_TAKE);
  GET_OBJ_COST(obj) = amount;

  new_weight = money_weight(amount);
  delta_weight = new_weight - GET_OBJ_WEIGHT(obj);
  if (delta_weight != 0) {
    if (IN_ROOM(obj) != NOWHERE || obj->carried_by || obj->in_obj)
      weight_change_object(obj, delta_weight);
    else
      GET_OBJ_WEIGHT(obj) = new_weight;
  }
}

struct obj_data *create_money(int amount)
{
  struct obj_data *obj;
  int y;

  if (amount <= 0) {
    log("SYSERR: Try to create negative or 0 coins. (%d)", amount);
    return (NULL);
  }
  obj = create_obj();

  GET_OBJ_TYPE(obj) = ITEM_MONEY;
  for(y = 0; y < TW_ARRAY_MAX; y++)
    obj->obj_flags.wear_flags[y] = 0;
  SET_BIT_AR(GET_OBJ_WEAR(obj), ITEM_WEAR_TAKE);
  GET_OBJ_VAL(obj, 0) = amount;
  obj->item_number = NOTHING;
  update_money_obj(obj);

  return (obj);
}

static void count_coins_in_list(struct obj_data *list, int *total)
{
  struct obj_data *obj;

  for (obj = list; obj; obj = obj->next_content) {
    if (GET_OBJ_TYPE(obj) == ITEM_MONEY)
      *total += MAX(0, GET_OBJ_VAL(obj, 0));
    if (obj->contains)
      count_coins_in_list(obj->contains, total);
  }
}

int count_char_coins(struct char_data *ch)
{
  int total = 0;
  int i;
  struct obj_data *obj;

  if (!ch)
    return 0;

  count_coins_in_list(ch->carrying, &total);
  for (i = 0; i < NUM_WEARS; i++) {
    obj = GET_EQ(ch, i);
    if (obj)
      count_coins_in_list(obj, &total);
  }

  return total;
}

static void take_coins_from_list(struct obj_data *list, int *remaining, struct char_data *owner)
{
  struct obj_data *obj;
  struct obj_data *next_obj;

  for (obj = list; obj && *remaining > 0; obj = next_obj) {
    next_obj = obj->next_content;

    if (GET_OBJ_TYPE(obj) == ITEM_MONEY) {
      int pile = GET_OBJ_VAL(obj, 0);
      int take = MIN(pile, *remaining);

      if (take > 0) {
        if (take == pile) {
          *remaining -= take;
          extract_obj(obj);
          continue;
        }

        GET_OBJ_VAL(obj, 0) = pile - take;
        *remaining -= take;
        update_money_obj(obj);
        adjust_char_coins(owner, -take);
      }
    }

    if (obj->contains && *remaining > 0)
      take_coins_from_list(obj->contains, remaining, owner);
  }
}

int remove_coins_from_char(struct char_data *ch, int amount)
{
  int remaining;
  int i;
  struct obj_data *obj;

  if (!ch || amount <= 0)
    return 0;

  remaining = amount;
  take_coins_from_list(ch->carrying, &remaining, ch);
  for (i = 0; i < NUM_WEARS && remaining > 0; i++) {
    obj = GET_EQ(ch, i);
    if (obj)
      take_coins_from_list(obj, &remaining, ch);
  }

  return amount - remaining;
}

void add_coins_to_char(struct char_data *ch, int amount)
{
  struct obj_data *money;

  if (!ch || amount <= 0)
    return;

  money = create_money(amount);
  if (!money)
    return;

  obj_to_char(money, ch);
}

/* Generic Find, designed to find any object orcharacter.
 *  *arg     is the pointer containing the string to be searched for.
 *           This string doesn't have to be a single word, the routine
 *           extracts the next word itself.
 *  bitv..   All those bits that you want to "search through".
 *           Bit found will be result of the function
 *  *ch      This is the person that is trying to "find"
 *  **tar_ch Will be NULL if no character was found, otherwise points
 * **tar_obj Will be NULL if no object was found, otherwise points
 *
 * The routine used to return a pointer to the next word in *arg (just
 * like the one_argument routine), but now it returns an integer that
 * describes what it filled in. */
int generic_find(char *arg, bitvector_t bitvector, struct char_data *ch,
		     struct char_data **tar_ch, struct obj_data **tar_obj)
{
  int i, found, number;
  char name_val[MAX_INPUT_LENGTH];
  char *name = name_val;

  *tar_ch = NULL;
  *tar_obj = NULL;

  one_argument(arg, name);

  if (!*name)
    return (0);
  if (!(number = get_number(&name)))
    return (0);

  if (IS_SET(bitvector, FIND_CHAR_ROOM)) {	/* Find person in room */
    if ((*tar_ch = get_char_room_vis(ch, name, &number)) != NULL)
      return (FIND_CHAR_ROOM);
  }

  if (IS_SET(bitvector, FIND_CHAR_WORLD)) {
    if ((*tar_ch = get_char_world_vis(ch, name, &number)) != NULL)
      return (FIND_CHAR_WORLD);
  }

  if (IS_SET(bitvector, FIND_OBJ_EQUIP)) {
    for (found = FALSE, i = 0; i < NUM_WEARS && !found; i++)
      if (GET_EQ(ch, i) && isname(name, GET_EQ(ch, i)->name) && --number == 0) {
	*tar_obj = GET_EQ(ch, i);
	found = TRUE;
      }
    if (found)
      return (FIND_OBJ_EQUIP);
  }

  if (IS_SET(bitvector, FIND_OBJ_INV)) {
    if ((*tar_obj = get_obj_in_list_vis(ch, name, &number, ch->carrying)) != NULL)
      return (FIND_OBJ_INV);
  }

  if (IS_SET(bitvector, FIND_OBJ_ROOM)) {
    if ((*tar_obj = get_obj_in_list_vis(ch, name, &number, world[IN_ROOM(ch)].contents)) != NULL)
      return (FIND_OBJ_ROOM);
  }

  if (IS_SET(bitvector, FIND_OBJ_WORLD)) {
    if ((*tar_obj = get_obj_vis(ch, name, &number)))
      return (FIND_OBJ_WORLD);
  }

  return (0);
}

/* a function to scan for "all" or "all.x" */
int find_all_dots(char *arg)
{
  if (!strcmp(arg, "all"))
    return (FIND_ALL);
  else if (!strncmp(arg, "all.", 4)) {
    strcpy(arg, arg + 4);	/* strcpy: OK (always less) */
    return (FIND_ALLDOT);
  } else
    return (FIND_INDIV);
}

/* Group Handlers */
struct group_data * create_group(struct char_data * leader) 
{
  struct group_data * new_group;
  
  /* Allocate Group Memory & Attach to Group List*/
  CREATE(new_group, struct group_data, 1);
  add_to_list(new_group, group_list);
  
  /* Allocate Members List */
  new_group->members = create_list();
  
  /* Clear Data */
  new_group->group_flags = 0;
  
  /* Assign Data */
  SET_BIT(GROUP_FLAGS(new_group), GROUP_OPEN);
  
  if (IS_NPC(leader))
    SET_BIT(GROUP_FLAGS(new_group), GROUP_NPC);
  
  join_group(leader, new_group);
  
  return (new_group);
}

void free_group(struct group_data * group)
{
  struct char_data *tch;
	struct iterator_data Iterator;
	
  if (group->members->iSize) {
		for (tch = (struct char_data *) merge_iterator(&Iterator, group->members);
		  tch; 
		    tch = next_in_list(&Iterator))
          leave_group(tch);
          
    remove_iterator(&Iterator);
  }
  
  free_list(group->members);
  remove_from_list(group, group_list);
  free(group);
}

void leave_group(struct char_data *ch)
{
  struct group_data *group;
  struct char_data *tch;
  struct iterator_data Iterator;
  bool found_pc = FALSE;
	
  if ((group = ch->group) == NULL)
    return;

  send_to_group(NULL, group, "%s has left the group.\r\n", GET_NAME(ch));

  remove_from_list(ch, group->members);
  ch->group = NULL;
  
  if (group->members->iSize) {
    for (tch = (struct char_data *) merge_iterator(&Iterator, group->members);
      tch; tch = next_in_list(&Iterator))
        if (!IS_NPC(tch)) 
          found_pc = TRUE;
          
    remove_iterator(&Iterator);  
  }

  if (!found_pc)
    SET_BIT(GROUP_FLAGS(group), GROUP_NPC);
  
  if (GROUP_LEADER(group) == ch && group->members->iSize) {
    group->leader = (struct char_data *) random_from_list(group->members);
    send_to_group(NULL, group, "%s has assumed leadership of the group.\r\n", GET_NAME(GROUP_LEADER(group)));
  } else if (group->members->iSize == 0)
    free_group(group); 
}

void join_group(struct char_data *ch, struct group_data *group)
{
  add_to_list(ch, group->members);
	
  if (group->leader == NULL)
    group->leader = ch;
	  
  ch->group = group;  
  
  if (IS_SET(group->group_flags, GROUP_NPC) && !IS_NPC(ch))
    REMOVE_BIT(GROUP_FLAGS(group), GROUP_NPC);
	
  if (group->leader == ch)
    send_to_group(NULL, group, "%s becomes leader of the group.\r\n", GET_NAME(ch));
  else
    send_to_group(NULL, group, "%s joins the group.\r\n", GET_NAME(ch));		
}
