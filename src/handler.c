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
#include "dg_scripts.h"
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

  if (!strcmp(str, namelist)) /* the easy way */
    return 1;

  newlist = strdup(namelist); /* make a copy since strtok 'modifies' strings */
  for(curtok = strtok(newlist, WHITESPACE); curtok; curtok = strtok(NULL, WHITESPACE))
    if(curtok && is_abbrev(str, curtok)) {
      /* Don't allow abbreviated numbers. - Sryth */
      if (isdigit(*str) && (atoi(str) != atoi(curtok)))
        return 0;
      free(newlist);
      return 1;
    }
  free(newlist);
  return 0;
}

static void aff_apply_modify(struct char_data *ch, byte loc, sbyte mod, char *msg)
{
  switch (loc) {
  case APPLY_NONE:
    break;

  case APPLY_STR:
    GET_STR(ch) += mod;
    break;
  case APPLY_DEX:
    GET_DEX(ch) += mod;
    break;
  case APPLY_INT:
    GET_INT(ch) += mod;
    break;
  case APPLY_WIS:
    GET_WIS(ch) += mod;
    break;
  case APPLY_CON:
    GET_CON(ch) += mod;
    break;
  case APPLY_CHA:
    GET_CHA(ch) += mod;
    break;

  /* Do Not Use. */
  case APPLY_CLASS:
    break;
  case APPLY_LEVEL:
    break;

  case APPLY_AGE:
    ch->player.time.birth -= (mod * SECS_PER_MUD_YEAR);
    break;

  case APPLY_CHAR_WEIGHT:
    GET_WEIGHT(ch) += mod;
    break;

  case APPLY_CHAR_HEIGHT:
    GET_HEIGHT(ch) += mod;
    break;

  case APPLY_MANA:
    GET_MAX_MANA(ch) += mod;
    break;

  case APPLY_HIT:
    GET_MAX_HIT(ch) += mod;
    break;

  case APPLY_MOVE:
    GET_MAX_MOVE(ch) += mod;
    break;

  case APPLY_GOLD:
    break;

  case APPLY_EXP:
    break;

  case APPLY_AC:
    GET_AC(ch) += mod;
    break;

  case APPLY_HITROLL:
    GET_HITROLL(ch) += mod;
    break;

  case APPLY_DAMROLL:
    GET_DAMROLL(ch) += mod;
    break;

  case APPLY_SAVING_PARA:
    GET_SAVE(ch, SAVING_PARA) += mod;
    break;

  case APPLY_SAVING_ROD:
    GET_SAVE(ch, SAVING_ROD) += mod;
    break;

  case APPLY_SAVING_PETRI:
    GET_SAVE(ch, SAVING_PETRI) += mod;
    break;

  case APPLY_SAVING_BREATH:
    GET_SAVE(ch, SAVING_BREATH) += mod;
    break;

  case APPLY_SAVING_SPELL:
    GET_SAVE(ch, SAVING_SPELL) += mod;
    break;

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

  for (i = 0; i < NUM_WEARS; i++) {
    if (GET_EQ(ch, i))
      for (j = 0; j < MAX_OBJ_AFFECT; j++)
	affect_modify_ar(ch, GET_EQ(ch, i)->affected[j].location,
		      GET_EQ(ch, i)->affected[j].modifier,
		      GET_OBJ_AFFECT(GET_EQ(ch, i)), FALSE);
  }

  for (af = ch->affected; af; af = af->next)
    affect_modify_ar(ch, af->location, af->modifier, af->bitvector, FALSE);

  ch->aff_abils = ch->real_abils;

  for (i = 0; i < NUM_WEARS; i++) {
    if (GET_EQ(ch, i))
      for (j = 0; j < MAX_OBJ_AFFECT; j++)
	affect_modify_ar(ch, GET_EQ(ch, i)->affected[j].location,
		      GET_EQ(ch, i)->affected[j].modifier,
		      GET_OBJ_AFFECT(GET_EQ(ch, i)), TRUE);
  }

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
  } else {
    if (GET_STR(ch) > 18) {
      i = GET_ADD(ch) + ((GET_STR(ch) - 18) * 10);
      GET_ADD(ch) = MIN(i, 100);
      GET_STR(ch) = 18;
    }
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

/* Give an object to a char. */
void obj_to_char(struct obj_data *object, struct char_data *ch)
{
  if (object && ch) {
    object->next_content = ch->carrying;
    ch->carrying = object;
    object->carried_by = ch;
    IN_ROOM(object) = NOWHERE;
    IS_CARRYING_W(ch) += GET_OBJ_WEIGHT(object);
    IS_CARRYING_N(ch)++;

    autoquest_trigger_check(ch, NULL, object, AQ_OBJ_FIND);

    /* set flag for crash-save system, but not on mobs! */
    if (!IS_NPC(ch))
      SET_BIT_AR(PLR_FLAGS(ch), PLR_CRASH);
  } else
    log("SYSERR: NULL obj (%p) or char (%p) passed to obj_to_char.", (void *)object, (void *)ch);
}

/* take an object from a char */
void obj_from_char(struct obj_data *object)
{
  struct obj_data *temp;

  if (object == NULL) {
    log("SYSERR: NULL object passed to obj_from_char.");
    return;
  }
  REMOVE_FROM_LIST(object, object->carried_by->carrying, next_content);

  /* set flag for crash-save system, but not on mobs! */
  if (!IS_NPC(object->carried_by))
    SET_BIT_AR(PLR_FLAGS(object->carried_by), PLR_CRASH);

  IS_CARRYING_W(object->carried_by) -= GET_OBJ_WEIGHT(object);
  IS_CARRYING_N(object->carried_by)--;
  object->carried_by = NULL;
  object->next_content = NULL;
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

int invalid_align(struct char_data *ch, struct obj_data *obj)
{
  if (OBJ_FLAGGED(obj, ITEM_ANTI_EVIL) && IS_EVIL(ch))
    return TRUE;
  if (OBJ_FLAGGED(obj, ITEM_ANTI_GOOD) && IS_GOOD(ch))
    return TRUE;
  if (OBJ_FLAGGED(obj, ITEM_ANTI_NEUTRAL) && IS_NEUTRAL(ch))
    return TRUE;
  return FALSE;
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
  if (invalid_align(ch, obj) || invalid_class(ch, obj)) {
    act("You are zapped by $p and instantly let go of it.", FALSE, ch, obj, 0, TO_CHAR);
    act("$n is zapped by $p and instantly lets go of it.", FALSE, ch, obj, 0, TO_ROOM);
    /* Changed to drop in inventory instead of the ground. */
    obj_to_char(obj, ch);
    return;
  }

  GET_EQ(ch, pos) = obj;
  obj->worn_by = ch;
  obj->worn_on = pos;

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
  if (!object || room == NOWHERE || room > top_of_world)
    log("SYSERR: Illegal value(s) passed to obj_to_room. (Room #%d/%d, obj %p)",
	room, top_of_world, (void *)object);
  else {
    object->next_content = world[room].contents;
    world[room].contents = object;
    IN_ROOM(object) = room;
    object->carried_by = NULL;
    if (ROOM_FLAGGED(room, ROOM_HOUSE))
      SET_BIT_AR(ROOM_FLAGS(room), ROOM_HOUSE_CRASH);
  }
}

/* Take an object from a room */
void obj_from_room(struct obj_data *object)
{
  struct obj_data *temp;
  struct char_data *t, *tempch;

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

  obj->next_content = obj_to->contains;
  obj_to->contains = obj;
  obj->in_obj = obj_to;
  tmp_obj = obj->in_obj;

  /* Add weight to container, unless unlimited. */
  if (GET_OBJ_VAL(obj->in_obj, 0) > 0) {
    for (tmp_obj = obj->in_obj; tmp_obj->in_obj; tmp_obj = tmp_obj->in_obj)
      GET_OBJ_WEIGHT(tmp_obj) += GET_OBJ_WEIGHT(obj);

    /* top level object.  Subtract weight from inventory if necessary. */
    GET_OBJ_WEIGHT(tmp_obj) += GET_OBJ_WEIGHT(obj);
    if (tmp_obj->carried_by)
      IS_CARRYING_W(tmp_obj->carried_by) += GET_OBJ_WEIGHT(obj);
  }
}

/* remove an object from an object */
void obj_from_obj(struct obj_data *obj)
{
  struct obj_data *temp, *obj_from;

  if (obj->in_obj == NULL) {
    log("SYSERR: (%s): trying to illegally extract obj from obj.", __FILE__);
    return;
  }
  obj_from = obj->in_obj;
  temp = obj->in_obj;
  REMOVE_FROM_LIST(obj, obj_from->contains, next_content);

  /* Subtract weight from containers container unless unlimited. */
  if (GET_OBJ_VAL(obj->in_obj, 0) > 0) {
    for (temp = obj->in_obj; temp->in_obj; temp = temp->in_obj)
      GET_OBJ_WEIGHT(temp) -= GET_OBJ_WEIGHT(obj);

    /* Subtract weight from char that carries the object */
    GET_OBJ_WEIGHT(temp) -= GET_OBJ_WEIGHT(obj);
    if (temp->carried_by)
      IS_CARRYING_W(temp->carried_by) -= GET_OBJ_WEIGHT(obj);
  }
  obj->in_obj = NULL;
  obj->next_content = NULL;
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
      STATE(ch->desc) = CON_MENU;
      write_to_output(ch->desc, "%s", CONFIG_MENU);
    }
  }

  /* On with the character's assets... */
  if (ch->followers || ch->master)
    die_follower(ch);

  /* Check to see if we are grouped! */
  if (GROUP(ch))
    leave_group(ch);

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
    save_char(ch);
    Crash_delete_crashfile(ch);
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
    return (ch);

  /* 0.<name> means PC with name */
  if (*number == 0)
    return (get_player_vis(ch, name, NULL, FIND_CHAR_ROOM));

  for (i = world[IN_ROOM(ch)].people; i && *number; i = i->next_in_room)
    if (isname(name, i->player.name))
      if (CAN_SEE(ch, i))
	if (--(*number) == 0)
	  return (i);

  return (NULL);
}

struct char_data *get_char_world_vis(struct char_data *ch, char *name, int *number)
{
  struct char_data *i;
  int num;

  if (!number) {
    number = &num;
    num = get_number(&name);
  }

  if ((i = get_char_room_vis(ch, name, number)) != NULL)
    return (i);

  if (*number == 0)
    return get_player_vis(ch, name, NULL, 0);

  for (i = character_list; i && *number; i = i->next) {
    if (IN_ROOM(ch) == IN_ROOM(i))
      continue;
    if (!isname(name, i->player.name))
      continue;
    if (!CAN_SEE(ch, i))
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

const char *money_desc(int amount)
{
  int cnt;
  struct {
    int limit;
    const char *description;
  } money_table[] = {
    {          1, "a gold coin"				},
    {         10, "a tiny pile of gold coins"		},
    {         20, "a handful of gold coins"		},
    {         75, "a little pile of gold coins"		},
    {        200, "a small pile of gold coins"		},
    {       1000, "a pile of gold coins"		},
    {       5000, "a big pile of gold coins"		},
    {      10000, "a large heap of gold coins"		},
    {      20000, "a huge mound of gold coins"		},
    {      75000, "an enormous mound of gold coins"	},
    {     150000, "a small mountain of gold coins"	},
    {     250000, "a mountain of gold coins"		},
    {     500000, "a huge mountain of gold coins"	},
    {    1000000, "an enormous mountain of gold coins"	},
    {          0, NULL					},
  };

  if (amount <= 0) {
    log("SYSERR: Try to create negative or 0 money (%d).", amount);
    return (NULL);
  }

  for (cnt = 0; money_table[cnt].limit; cnt++)
    if (amount <= money_table[cnt].limit)
      return (money_table[cnt].description);

  return ("an absolutely colossal mountain of gold coins");
}

struct obj_data *create_money(int amount)
{
  struct obj_data *obj;
  struct extra_descr_data *new_descr;
  char buf[200];
  int y;

  if (amount <= 0) {
    log("SYSERR: Try to create negative or 0 money. (%d)", amount);
    return (NULL);
  }
  obj = create_obj();
  CREATE(new_descr, struct extra_descr_data, 1);

  if (amount == 1) {
    obj->name = strdup("coin gold");
    obj->short_description = strdup("a gold coin");
    obj->description = strdup("One miserable gold coin is lying here.");
    new_descr->keyword = strdup("coin gold");
    new_descr->description = strdup("It's just one miserable little gold coin.");
  } else {
    obj->name = strdup("coins gold");
    obj->short_description = strdup(money_desc(amount));
    snprintf(buf, sizeof(buf), "%s is lying here.", money_desc(amount));
    obj->description = strdup(CAP(buf));

    new_descr->keyword = strdup("coins gold");
    if (amount < 10)
      snprintf(buf, sizeof(buf), "There are %d coins.", amount);
    else if (amount < 100)
      snprintf(buf, sizeof(buf), "There are about %d coins.", 10 * (amount / 10));
    else if (amount < 1000)
      snprintf(buf, sizeof(buf), "It looks to be about %d coins.", 100 * (amount / 100));
    else if (amount < 100000)
      snprintf(buf, sizeof(buf), "You guess there are, maybe, %d coins.",
	      1000 * ((amount / 1000) + rand_number(0, (amount / 1000))));
    else
      strcpy(buf, "There are a LOT of coins.");	/* strcpy: OK (is < 200) */
    new_descr->description = strdup(buf);
  }

  new_descr->next = NULL;
  obj->ex_description = new_descr;

  GET_OBJ_TYPE(obj) = ITEM_MONEY;
  for(y = 0; y < TW_ARRAY_MAX; y++)
    obj->obj_flags.wear_flags[y] = 0;
  SET_BIT_AR(GET_OBJ_WEAR(obj), ITEM_WEAR_TAKE);
  GET_OBJ_VAL(obj, 0) = amount;
  GET_OBJ_COST(obj) = amount;
  obj->item_number = NOTHING;

  return (obj);
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
