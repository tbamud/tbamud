/**
* @file dg_scripts.c
* Contains the main script driver interface.
* 
* Part of the core tbaMUD source code distribution, which is a derivative
* of, and continuation of, CircleMUD.
* 
* This source code, which was not part of the CircleMUD legacy code,
* was created by the following people:                                      
* $Author: Mark A. Heilpern/egreen/Welcor $                              
* $Date: 2004/10/11 12:07:00$                                            
* $Revision: 1.0.14 $                                                    
*/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "dg_scripts.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "dg_event.h"
#include "db.h"
#include "screen.h"
#include "constants.h"
#include "spells.h"
#include "oasis.h"
#include "genzon.h" /* for real_zone_by_thing */
#include "act.h"
#include "modify.h"

#define PULSES_PER_MUD_HOUR     (SECS_PER_MUD_HOUR*PASSES_PER_SEC)

/* Local functions not used elsewhere */
static obj_data *find_obj(long n);
static room_data *find_room(long n);
static void do_stat_trigger(struct char_data *ch, trig_data *trig);
static void script_stat(char_data *ch, struct script_data *sc);
static int remove_trigger(struct script_data *sc, char *name);
static int is_num(char *arg);
static void eval_op(char *op, char *lhs, char *rhs, char *result, void *go,
          struct script_data *sc, trig_data *trig);
static char *matching_paren(char *p);
static void eval_expr(char *line, char *result, void *go, struct script_data *sc,
          trig_data *trig, int type);
static int eval_lhs_op_rhs(char *expr, char *result, void *go, struct script_data *sc,
          trig_data *trig, int type);
static int process_if(char *cond, void *go, struct script_data *sc,
          trig_data *trig, int type);
static struct cmdlist_element *find_end(trig_data *trig, struct cmdlist_element *cl);
static struct cmdlist_element *find_else_end(trig_data *trig,
          struct cmdlist_element *cl, void *go, struct script_data *sc, int type);
static void process_wait(void *go, trig_data *trig, int type, char *cmd,
          struct cmdlist_element *cl);
static void process_set(struct script_data *sc, trig_data *trig, char *cmd);
static void process_attach(void *go, struct script_data *sc, trig_data *trig,
          int type, char *cmd);
static void process_detach(void *go, struct script_data *sc, trig_data *trig,
          int type, char *cmd);
static void makeuid_var(void *go, struct script_data *sc, trig_data *trig,
          int type, char *cmd);
static int process_return(trig_data *trig, char *cmd);
static void process_unset(struct script_data *sc, trig_data *trig, char *cmd);
static void process_remote(struct script_data *sc, trig_data *trig, char *cmd);
static void process_rdelete(struct script_data *sc, trig_data *trig, char *cmd);
static void process_global(struct script_data *sc, trig_data *trig, char *cmd, long id);
static void process_context(struct script_data *sc, trig_data *trig, char *cmd);
static void extract_value(struct script_data *sc, trig_data *trig, char *cmd);
static void dg_letter_value(struct script_data *sc, trig_data *trig, char *cmd);
static struct cmdlist_element * find_case(struct trig_data *trig, struct cmdlist_element *cl,
          void *go, struct script_data *sc, int type, char *cond);
static struct cmdlist_element *find_done(struct cmdlist_element *cl);
static struct char_data *find_char_by_uid_in_lookup_table(long uid);
static struct obj_data *find_obj_by_uid_in_lookup_table(long uid);
static EVENTFUNC(trig_wait_event);


/* Return pointer to first occurrence of string ct in cs, or NULL if not 
 * present.  Case insensitive. All of ct must be found in cs for it to be 
 * a match.
 * @todo Move this function to string util library.
 * @param cs The string to search.
 * @param ct What to search for in cs.
 * @retval char * NULL if ct is not a substring of cs, or pointer to the
 * location in cs where substring ct begins. */
char *str_str(char *cs, char *ct)
{
  char *s, *t;

  if (!cs || !ct || !*ct)
    return NULL;

  while (*cs) {
    t = ct;

    while (*cs && (LOWER(*cs) != LOWER(*t)))
      cs++;

    s = cs;

    while (*t && *cs && (LOWER(*cs) == LOWER(*t))) {
      t++;
      cs++;
    }

    /* If there we haven reached the end of ct via t,
     * then the whole string was found. */
    if (!*t)
      return s;
  }

  return NULL;
}

/** Returns the number of people in a room.
 * @param vnum The virtual number of a room.
 * @retval int Returns -1 if the room does not exist, or the total number of
 * PCs and NPCs in the room. */
int trgvar_in_room(room_vnum vnum) 
{
    room_rnum rnum = real_room(vnum);
    int i = 0;
    char_data *ch;

    if (rnum == NOWHERE) {
        script_log("people.vnum: world[rnum] does not exist");
        return (-1);
    }

    for (ch = world[rnum].people; ch !=NULL; ch = ch->next_in_room)
        i++;

    return i;
}


/** Find out if an object is within a list of objects.
 * @param name Either the unique id of an object or a string identifying the
 * object. Note the unique id must be prefixed with UID_CHAR.
 * @param list The list of objects to look through.
 * @retval obj_data * Pointer to the object if it is found in the list of 
 * objects, NULL if the object is not found in the list. 
 */
obj_data *get_obj_in_list(char *name, obj_data *list)
{
    obj_data *i;
    long id;

    if (*name == UID_CHAR){
      id = atoi(name + 1);

      for (i = list; i; i = i->next_content)
        if (id == GET_ID(i))
          return i;
      
    } else {
      for (i = list; i; i = i->next_content)
        if (isname(name, i->name))
          return i;
    }

    return NULL;
}

/** Find out if an NPC or PC is carrying an object.
 * @param ch Pointer to the NPC/PC to search through.
 * @param name String describing either the name of the object or the unique
 * id of the object. Note the unique id must be prefixed with UID_CHAR.
 * @retval obj_data * Either a pointer to the first object found that matches
 * the name argument, or the NULL if the object isn't found.
 */
obj_data *get_object_in_equip(char_data * ch, char *name)
{
  int j, n = 0, number;
  obj_data *obj;
  char tmpname[MAX_INPUT_LENGTH];
  char *tmp = tmpname;
  long id;

  if (*name == UID_CHAR) {
    id = atoi(name + 1);

    for (j = 0; j < NUM_WEARS; j++)
      if ((obj = GET_EQ(ch, j)))
        if (id == GET_ID(obj))
          return (obj);
  } else if (is_number(name)) {
    obj_vnum ovnum = atoi(name);
    for (j = 0; j < NUM_WEARS; j++)
      if ((obj = GET_EQ(ch, j)))
        if (GET_OBJ_VNUM(obj) == ovnum)
          return (obj);
  } else {
    snprintf(tmpname, sizeof(tmpname), "%s", name);
    if (!(number = get_number(&tmp)))
      return NULL;

    for (j = 0; (j < NUM_WEARS) && (n <= number); j++)
      if ((obj = GET_EQ(ch, j)))
        if (isname(tmp, obj->name))
          if (++n == number)
            return (obj);
  }

  return NULL;
}

/** Figures out if the argument is a valid location to 'wear' equipment.
 * Handles 'held', 'light' and 'wield' positions - Welcor. After idea from 
 * Byron Ellacott. 
 * @param arg Either the name of the position, or the number of a wear
 * location definition to check for.
 * @retval int If arg is not a valid wear location name or number, return
 * -1, else return the defined number of the wear location.
 */
int find_eq_pos_script(char *arg)
{
  int i;
  struct eq_pos_list {
    const char *pos;
    int where;
  } eq_pos[] = {
    {"hold",     WEAR_HOLD},
    {"held",     WEAR_HOLD},
    {"light",    WEAR_LIGHT},
    {"wield",    WEAR_WIELD},
    {"rfinger",  WEAR_FINGER_R},
    {"lfinger",  WEAR_FINGER_L},
    {"neck1",    WEAR_NECK_1},
    {"neck2",    WEAR_NECK_2},
    {"body",     WEAR_BODY},
    {"head",     WEAR_HEAD},
    {"legs",     WEAR_LEGS},
    {"feet",     WEAR_FEET},
    {"hands",    WEAR_HANDS},
    {"arms",     WEAR_ARMS},
    {"shield",   WEAR_SHIELD},
    {"about",    WEAR_ABOUT},
    {"waist",    WEAR_WAIST},
    {"rwrist",   WEAR_WRIST_R},
    {"lwrist",   WEAR_WRIST_L},
    {"none", -1}
  };

  if (is_number(arg) && (i = atoi(arg)) >= 0 && i < NUM_WEARS)
    return i;

  for (i = 0;eq_pos[i].where != -1;i++) {
    if (!str_cmp(eq_pos[i].pos, arg))
      return eq_pos[i].where;
  }
  return (-1);
}

/** Figures out if an object can be worn on a defined wear location.
 * @param obj The object to check.
 * @param pos The defined wear location to check.
 * @retval int TRUE if obj can be worn on pos, FALSE if not.
 */
int can_wear_on_pos(struct obj_data *obj, int pos)
{
  switch (pos) {
    case WEAR_HOLD:
    case WEAR_LIGHT:    return CAN_WEAR(obj, ITEM_WEAR_HOLD);
    case WEAR_WIELD:    return CAN_WEAR(obj, ITEM_WEAR_WIELD);
    case WEAR_FINGER_R:
    case WEAR_FINGER_L: return CAN_WEAR(obj, ITEM_WEAR_FINGER);
    case WEAR_NECK_1:
    case WEAR_NECK_2:   return CAN_WEAR(obj, ITEM_WEAR_NECK);
    case WEAR_BODY:     return CAN_WEAR(obj, ITEM_WEAR_BODY);
    case WEAR_HEAD:     return CAN_WEAR(obj, ITEM_WEAR_HEAD);
    case WEAR_LEGS:     return CAN_WEAR(obj, ITEM_WEAR_LEGS);
    case WEAR_FEET:     return CAN_WEAR(obj, ITEM_WEAR_FEET);
    case WEAR_HANDS:    return CAN_WEAR(obj, ITEM_WEAR_HANDS);
    case WEAR_ARMS:     return CAN_WEAR(obj, ITEM_WEAR_ARMS);
    case WEAR_SHIELD:   return CAN_WEAR(obj, ITEM_WEAR_SHIELD);
    case WEAR_ABOUT:    return CAN_WEAR(obj, ITEM_WEAR_ABOUT);
    case WEAR_WAIST:    return CAN_WEAR(obj, ITEM_WEAR_WAIST);
    case WEAR_WRIST_R:
    case WEAR_WRIST_L:  return CAN_WEAR(obj, ITEM_WEAR_WRIST);
    default: return FALSE;
  }
}

/** Search for an NPC or PC by number routines.
 * @param n The unique ID (PC or NPC) to look for.
 * @retval char_data * Pointer to the character structure if it exists, or NULL
 * if it cannot be found.
 */
struct char_data *find_char(long n)
{
  if (n>=ROOM_ID_BASE) /* See note in dg_scripts.h */
    return NULL;

  return find_char_by_uid_in_lookup_table(n);
}

/** Search for an object by number routines.
 * @param n The unique ID to look for.
 * @retval obj_data * Pointer to the object if it exists, or NULL if it cannot
 * be found.
 */
static obj_data *find_obj(long n)
{
  if (n < OBJ_ID_BASE) /* see note in dg_scripts.h */
    return NULL;

  return find_obj_by_uid_in_lookup_table(n);
}

/* Search for a room with UID n.
 * @param n the Unique ID to look for.
 * @retval room_data * Pointer to the room if it exists, or NULL if it cannot
 * be found.
 */
static room_data *find_room(long n)
{
  room_rnum rnum;

  n -= ROOM_ID_BASE;
  if (n<0)
    return NULL;
  rnum = real_room((room_vnum)n);

  if (rnum != NOWHERE)
    return &world[rnum];

  return NULL;
}

/* Generic searches based only on name. */
/** Search the entire world for an NPC or PC by name. 
 * @param name String describing the name or the unique id of the char. 
 * Note the unique id must be prefixed with UID_CHAR.
 * @retval char_data * Pointer to the char or NULL if char is not found. */
char_data *get_char(char *name)
{
  char_data *i;

  if (*name == UID_CHAR) {
    i = find_char(atoi(name + 1));

    if (i && valid_dg_target(i, DG_ALLOW_GODS))
      return i;
  } else {
    for (i = character_list; i; i = i->next)
      if (isname(name, i->player.name) &&
          valid_dg_target(i, DG_ALLOW_GODS))
        return i;
  }

  return NULL;
}

/** Find a character by name in the same room as a known object.
 * @todo Should this function not be constrained to the same room as an object
 * if 'name' is a unique id? 
 * @param obj An object that will constrain the search to the location that
 * the object is in *if* the name argument is not a unique id.
 * @param name Character name keyword to search for, or unique ID. Unique
 * id must be prefixed with UID_CHAR. 
 * @retval char_data * Pointer to the the char if found, NULL if not. Will
 * only find god characters if DG_ALLOW_GODS is on. */
char_data *get_char_near_obj(obj_data *obj, char *name)
{
  char_data *ch;

  if (*name == UID_CHAR) {
    ch = find_char(atoi(name + 1));

    if (ch && valid_dg_target(ch, DG_ALLOW_GODS))
      return ch;
  } else {
    room_rnum num;
    if ((num = obj_room(obj)) != NOWHERE)
      for (ch = world[num].people; ch; ch = ch->next_in_room)
        if (isname(name, ch->player.name) &&
            valid_dg_target(ch, DG_ALLOW_GODS))
          return ch;
  }

  return NULL;
}

/** Find a character by name in a specific room.
 * @todo Should this function not be constrained to the room 
 * if 'name' is a unique id? 
 * @param room A room that will constrain the search to that location 
 * *if* the name argument is not a unique id.
 * @param name Character name keyword to search for, or unique ID. Unique
 * id must be prefixed with UID_CHAR. 
 * @retval char_data * Pointer to the the char if found, NULL if not. Will
 * only find god characters if DG_ALLOW_GODS is on. */
char_data *get_char_in_room(room_data *room, char *name)
{
  char_data *ch;

  if (*name == UID_CHAR) {
    ch = find_char(atoi(name + 1));

    if (ch && valid_dg_target(ch, DG_ALLOW_GODS))
      return ch;
  } else {
    for (ch = room->people; ch; ch = ch->next_in_room)
      if (isname(name, ch->player.name) &&
          valid_dg_target(ch, DG_ALLOW_GODS))
        return ch;
    }

  return NULL;
}

/** Find a named object near another object (either in the same room, as
 * a container or contained by).
 * @param obj The obj with which to constrain the search.
 * @param name The keyword of the object to search for. If 'self' or 'me'
 * are passed in as arguments, obj is returned. Can also be a unique object
 * id, and if so it must be prefixed with UID_CHAR.
 * @retval obj_data * Pointer to the object if found, NULL if not. */
obj_data *get_obj_near_obj(obj_data *obj, char *name)
{
  obj_data *i = NULL;
  char_data *ch;
  int rm;
  long id;

  if (!str_cmp(name, "self") || !str_cmp(name, "me"))
    return obj;

  /* is it inside ? */
  if (obj->contains && (i = get_obj_in_list(name, obj->contains)))
    return i;

  /* or outside ? */
  if (obj->in_obj) {
    if (*name == UID_CHAR) {
       id = atoi(name + 1);

      if (id == GET_ID(obj->in_obj))
        return obj->in_obj;
    } else if (isname(name, obj->in_obj->name))
      return obj->in_obj;
  }
  /* or worn ?*/
  else if (obj->worn_by && (i = get_object_in_equip(obj->worn_by, name)))
    return i;
  /* or carried ? */
  else if (obj->carried_by &&
          (i = get_obj_in_list(name, obj->carried_by->carrying)))
    return i;
  else if ((rm = obj_room(obj)) != NOWHERE) {
    /* check the floor */
    if ((i = get_obj_in_list(name, world[rm].contents)))
      return i;

    /* check peoples' inventory */
    for (ch = world[rm].people;ch ; ch = ch->next_in_room)
      if ((i = get_object_in_equip(ch, name)))
        return i;
  }
  return NULL;
}

/* returns the object in the world with name name, or NULL if not found */
obj_data *get_obj(char *name)
{
  obj_data *obj;

  if (*name == UID_CHAR)
    return find_obj(atoi(name + 1));
  else {
    for (obj = object_list; obj; obj = obj->next)
      if (isname(name, obj->name))
        return obj;
  }

  return NULL;
}

/* finds room by id or vnum.  returns NULL if not found */
room_data *get_room(char *name)
{
  room_rnum nr;

  if (*name == UID_CHAR)
    return find_room(atoi(name + 1));
  else if ((nr = real_room(atoi(name))) == NOWHERE)
    return NULL;
  else
    return &world[nr];
}

/* Returns a pointer to the first character in world by name name, or NULL if 
 * none found.  Starts searching with the person owing the object. */
char_data *get_char_by_obj(obj_data *obj, char *name)
{
  char_data *ch;

  if (*name == UID_CHAR) {
    ch = find_char(atoi(name + 1));

    if (ch && valid_dg_target(ch, DG_ALLOW_GODS))
      return ch;
  } else {
    if (obj->carried_by &&
        isname(name, obj->carried_by->player.name) &&
        valid_dg_target(obj->carried_by, DG_ALLOW_GODS))
      return obj->carried_by;

    if (obj->worn_by &&
        isname(name, obj->worn_by->player.name) &&
        valid_dg_target(obj->worn_by, DG_ALLOW_GODS))
      return obj->worn_by;

    for (ch = character_list; ch; ch = ch->next)
      if (isname(name, ch->player.name) &&
          valid_dg_target(ch, DG_ALLOW_GODS))
        return ch;
  }

  return NULL;
}

/* Returns a pointer to the first character in world by name name, or NULL if 
 * none found.  Starts searching in room room first. */
char_data *get_char_by_room(room_data *room, char *name)
{
  char_data *ch;

  if (*name == UID_CHAR) {
    ch = find_char(atoi(name + 1));

    if (ch && valid_dg_target(ch, DG_ALLOW_GODS))
      return ch;
  } else {
    for (ch = room->people; ch; ch = ch->next_in_room)
      if (isname(name, ch->player.name) &&
          valid_dg_target(ch, DG_ALLOW_GODS))
        return ch;

    for (ch = character_list; ch; ch = ch->next)
      if (isname(name, ch->player.name) &&
          valid_dg_target(ch, DG_ALLOW_GODS))
        return ch;
  }

  return NULL;
}

/* Returns the object in the world with name name, or NULL if not found search 
 * based on obj. */
obj_data *get_obj_by_obj(obj_data *obj, char *name)
{
  obj_data *i = NULL;
  int rm;

  if (*name == UID_CHAR)
    return find_obj(atoi(name + 1));

  if (!str_cmp(name, "self") || !str_cmp(name, "me"))
    return obj;

  if (obj->contains && (i = get_obj_in_list(name, obj->contains)))
    return i;

  if (obj->in_obj && isname(name, obj->in_obj->name))
      return obj->in_obj;

  if (obj->worn_by && (i = get_object_in_equip(obj->worn_by, name)))
    return i;

  if (obj->carried_by &&
     (i = get_obj_in_list(name, obj->carried_by->carrying)))
    return i;

  if (((rm = obj_room(obj)) != NOWHERE) &&
      (i = get_obj_in_list(name, world[rm].contents)))
    return i;

  return get_obj(name);
}

/* only searches the room */
obj_data *get_obj_in_room(room_data *room, char *name)
{
  obj_data *obj;
  long id;

  if (*name == UID_CHAR) {
      id = atoi(name + 1);
      for (obj = room->contents; obj; obj = obj->next_content)
          if (id == GET_ID(obj))
              return obj;
  } else {
      for (obj = room->contents; obj; obj = obj->next_content)
          if (isname(name, obj->name))
              return obj;
  }

  return NULL;
}

/* returns obj with name - searches room, then world */
obj_data *get_obj_by_room(room_data *room, char *name)
{
  obj_data *obj;

  if (*name == UID_CHAR)
    return find_obj(atoi(name+1));

  for (obj = room->contents; obj; obj = obj->next_content)
    if (isname(name, obj->name))
      return obj;

  for (obj = object_list; obj; obj = obj->next)
    if (isname(name, obj->name))
      return obj;

  return NULL;
}

/* checks every PULSE_SCRIPT for random triggers */
void script_trigger_check(void)
{
  char_data *ch;
  obj_data *obj;
  struct room_data *room=NULL;
  int nr;
  struct script_data *sc;

  for (ch = character_list; ch; ch = ch->next) {
    if (SCRIPT(ch)) {
      sc = SCRIPT(ch);

      if (IS_SET(SCRIPT_TYPES(sc), WTRIG_RANDOM) &&
          (!is_empty(world[IN_ROOM(ch)].zone) ||
           IS_SET(SCRIPT_TYPES(sc), WTRIG_GLOBAL)))
        random_mtrigger(ch);
    }
  }

  for (obj = object_list; obj; obj = obj->next) {
    if (SCRIPT(obj)) {
      sc = SCRIPT(obj);

      if (IS_SET(SCRIPT_TYPES(sc), OTRIG_RANDOM))
        random_otrigger(obj);
    }
  }

  for (nr = 0; nr <= top_of_world; nr++) {
    if (SCRIPT(&world[nr])) {
      room = &world[nr];
      sc = SCRIPT(room);

      if (IS_SET(SCRIPT_TYPES(sc), WTRIG_RANDOM) &&
          (!is_empty(room->zone) ||
           IS_SET(SCRIPT_TYPES(sc), WTRIG_GLOBAL)))
        random_wtrigger(room);
    }
  }
}

void check_time_triggers(void)
{
  char_data *ch;
  obj_data *obj;
  struct room_data *room=NULL;
  int nr;
  struct script_data *sc;

  for (ch = character_list; ch; ch = ch->next) {
    if (SCRIPT(ch)) {
      sc = SCRIPT(ch);

      if (IS_SET(SCRIPT_TYPES(sc), WTRIG_TIME) &&
          (!is_empty(world[IN_ROOM(ch)].zone) ||
           IS_SET(SCRIPT_TYPES(sc), WTRIG_GLOBAL)))
        time_mtrigger(ch);
    }
  }

  for (obj = object_list; obj; obj = obj->next) {
    if (SCRIPT(obj)) {
      sc = SCRIPT(obj);

      if (IS_SET(SCRIPT_TYPES(sc), OTRIG_TIME))
        time_otrigger(obj);
    }
  }

  for (nr = 0; nr <= top_of_world; nr++) {
    if (SCRIPT(&world[nr])) {
      room = &world[nr];
      sc = SCRIPT(room);

      if (IS_SET(SCRIPT_TYPES(sc), WTRIG_TIME) &&
          (!is_empty(room->zone) ||
           IS_SET(SCRIPT_TYPES(sc), WTRIG_GLOBAL)))
        time_wtrigger(room);
    }
  }
}

static EVENTFUNC(trig_wait_event)
{
  struct wait_event_data *wait_event_obj = (struct wait_event_data *)event_obj;
  trig_data *trig;
  void *go;
  int type;

  trig = wait_event_obj->trigger;
  go = wait_event_obj->go;
  type = wait_event_obj->type;

  free(wait_event_obj);
  GET_TRIG_WAIT(trig) = NULL;

#if 1  /* debugging */
  {
    int found = FALSE;
    if (type == MOB_TRIGGER) {
      struct char_data *tch;
      for (tch = character_list;tch && !found;tch = tch->next)
        if (tch == (struct char_data *)go)
          found = TRUE;
    } else if (type == OBJ_TRIGGER) {
      struct obj_data *obj;
      for (obj = object_list;obj && !found;obj = obj->next)
        if (obj == (struct obj_data *)go)
          found = TRUE;
    } else {
      room_rnum i;
      for (i = 0;i<top_of_world && !found;i++)
        if (&world[i] == (struct room_data *)go)
          found = TRUE;
    }
    if (!found) {
      log("Trigger restarted on unknown entity. Vnum: %d", GET_TRIG_VNUM(trig));
      log("Type: %s trigger", type==MOB_TRIGGER ? "Mob" : type == OBJ_TRIGGER ? "Obj" : "Room");
      log("attached %d places", trig_index[trig->nr]->number);
      script_log("Trigger restart attempt on unknown entity.");
      return 0;
    }
  }
#endif

  script_driver(&go, trig, type, TRIG_RESTART);

  /* Do not reenqueue*/
  return 0;
}

static void do_stat_trigger(struct char_data *ch, trig_data *trig)
{
    struct cmdlist_element *cmd_list;
    char sb[MAX_STRING_LENGTH], buf[MAX_STRING_LENGTH];
    int len = 0;

    if (!trig)
    {
        log("SYSERR: NULL trigger passed to do_stat_trigger.");
        return;
    }

    len += snprintf(sb, sizeof(sb), "Name: '%s%s%s',  VNum: [%s%5d%s], RNum: [%5d]\r\n",
              CCYEL(ch, C_NRM), GET_TRIG_NAME(trig), CCNRM(ch, C_NRM),
              CCGRN(ch, C_NRM), GET_TRIG_VNUM(trig), CCNRM(ch, C_NRM),
              GET_TRIG_RNUM(trig));

    if (trig->attach_type==OBJ_TRIGGER) {
      len += snprintf(sb + len, sizeof(sb)-len, "Trigger Intended Assignment: Objects\r\n");
      sprintbit(GET_TRIG_TYPE(trig), otrig_types, buf, sizeof(buf));
    } else if (trig->attach_type==WLD_TRIGGER) {
      len += snprintf(sb + len, sizeof(sb)-len, "Trigger Intended Assignment: Rooms\r\n");
      sprintbit(GET_TRIG_TYPE(trig), wtrig_types, buf, sizeof(buf));
    } else {
      len += snprintf(sb + len, sizeof(sb)-len, "Trigger Intended Assignment: Mobiles\r\n");
      sprintbit(GET_TRIG_TYPE(trig), trig_types, buf, sizeof(buf));
    }

    len += snprintf(sb + len, sizeof(sb)-len, "Trigger Type: %s, Numeric Arg: %d, Arg list: %s\r\n",
                     buf, GET_TRIG_NARG(trig),
                    ((GET_TRIG_ARG(trig) && *GET_TRIG_ARG(trig))
                     ? GET_TRIG_ARG(trig) : "None"));

    len += snprintf(sb + len, sizeof(sb)-len, "Commands:\r\n");

    cmd_list = trig->cmdlist;
    while (cmd_list) {
      if (cmd_list->cmd)
        len += snprintf(sb + len, sizeof(sb)-len, "%s\r\n", cmd_list->cmd);

        if (len>MAX_STRING_LENGTH-80) {
          len += snprintf(sb + len, sizeof(sb)-len, "*** Overflow - script too long! ***\r\n");
          break;
        }
      cmd_list = cmd_list->next;
    }

    page_string(ch->desc, sb, 1);
}

/* find the name of what the uid points to */
void find_uid_name(char *uid, char *name, size_t nlen)
{
  char_data *ch;
  obj_data *obj;

  if ((ch = get_char(uid)))
    snprintf(name, nlen, "%s", ch->player.name);
  else if ((obj = get_obj(uid)))
    snprintf(name, nlen, "%s", obj->name);
  else
    snprintf(name, nlen, "uid = %s, (not found)", uid + 1);
}

/* general function to display stats on script sc */
static void script_stat (char_data *ch, struct script_data *sc)
{
  struct trig_var_data *tv;
  trig_data *t;
  char name[MAX_INPUT_LENGTH];
  char namebuf[512];
  char buf1[MAX_STRING_LENGTH];

  send_to_char(ch, "Global Variables: %s\r\n", sc->global_vars ? "" : "None");
  send_to_char(ch, "Global context: %ld\r\n", sc->context);

  for (tv = sc->global_vars; tv; tv = tv->next) {
    snprintf(namebuf, sizeof(namebuf), "%s:%ld", tv->name, tv->context);
    if (*(tv->value) == UID_CHAR) {
      find_uid_name(tv->value, name, sizeof(name));
      send_to_char(ch, "    %15s:  %s\r\n", tv->context?namebuf:tv->name, name);
    } else
      send_to_char(ch, "    %15s:  %s\r\n", tv->context?namebuf:tv->name, tv->value);
  }

  for (t = TRIGGERS(sc); t; t = t->next) {
    send_to_char(ch, "\r\n  Trigger: %s%s%s, VNum: [%s%5d%s], RNum: [%5d]\r\n",
            CCYEL(ch, C_NRM), GET_TRIG_NAME(t), CCNRM(ch, C_NRM),
            CCGRN(ch, C_NRM), GET_TRIG_VNUM(t), CCNRM(ch, C_NRM),
            GET_TRIG_RNUM(t));

    if (t->attach_type==OBJ_TRIGGER) {
      send_to_char(ch, "  Trigger Intended Assignment: Objects\r\n");
      sprintbit(GET_TRIG_TYPE(t), otrig_types, buf1, sizeof(buf1));
    } else if (t->attach_type==WLD_TRIGGER) {
      send_to_char(ch, "  Trigger Intended Assignment: Rooms\r\n");
      sprintbit(GET_TRIG_TYPE(t), wtrig_types, buf1, sizeof(buf1));
    } else {
      send_to_char(ch, "  Trigger Intended Assignment: Mobiles\r\n");
      sprintbit(GET_TRIG_TYPE(t), trig_types, buf1, sizeof(buf1));
    }

    send_to_char(ch, "  Trigger Type: %s, Numeric Arg: %d, Arg list: %s\r\n",
            buf1, GET_TRIG_NARG(t),
            ((GET_TRIG_ARG(t) && *GET_TRIG_ARG(t)) ? GET_TRIG_ARG(t) :
             "None"));

    if (GET_TRIG_WAIT(t)) {
      send_to_char(ch, "    Wait: %ld, Current line: %s\r\n",
              event_time(GET_TRIG_WAIT(t)),
              t->curr_state ? t->curr_state->cmd : "End of Script");
      send_to_char(ch, "  Variables: %s\r\n", GET_TRIG_VARS(t) ? "" : "None");

      for (tv = GET_TRIG_VARS(t); tv; tv = tv->next) {
        if (*(tv->value) == UID_CHAR) {
          find_uid_name(tv->value, name, sizeof(name));
          send_to_char(ch, "    %15s:  %s\r\n", tv->name, name);
        } else
          send_to_char(ch, "    %15s:  %s\r\n", tv->name, tv->value);
      }
    }
  }
}

void do_sstat_room(struct char_data * ch, struct room_data *rm)
{
  send_to_char(ch, "Triggers:\r\n");
  if (!SCRIPT(rm)) {
    send_to_char(ch, "  None.\r\n");
    return;
  }

  script_stat(ch, SCRIPT(rm));
}

void do_sstat_object(char_data *ch, obj_data *j)
{
  send_to_char(ch, "Triggers:\r\n");
  if (!SCRIPT(j)) {
    send_to_char(ch, "  None.\r\n");
    return;
  }

  script_stat(ch, SCRIPT(j));
}

void do_sstat_character(char_data *ch, char_data *k)
{
  send_to_char(ch, "Triggers:\r\n");
  if (!SCRIPT(k)) {
    send_to_char(ch, "  None.\r\n");
    return;
  }

  script_stat(ch, SCRIPT(k));
}

/* Adds the trigger t to script sc in in location loc.  loc = -1 means add to 
 * the end, loc = 0 means add before all other triggers. */
void add_trigger(struct script_data *sc, trig_data *t, int loc)
{
  trig_data *i;
  int n;

  for (n = loc, i = TRIGGERS(sc); i && i->next && (n != 0); n--, i = i->next);

  if (!loc) {
          t->next = TRIGGERS(sc);
    TRIGGERS(sc) = t;
  } else if (!i)
    TRIGGERS(sc) = t;
  else {
    t->next = i->next;
    i->next = t;
  }

  SCRIPT_TYPES(sc) |= GET_TRIG_TYPE(t);

  t->next_in_world = trigger_list;
  trigger_list = t;
}

ACMD(do_attach)
{
  char_data *victim;
  obj_data *object;
  room_data *room;
  trig_data *trig;
  char targ_name[MAX_INPUT_LENGTH], trig_name[MAX_INPUT_LENGTH];
  char loc_name[MAX_INPUT_LENGTH], arg[MAX_INPUT_LENGTH];
  int loc, tn, rn, num_arg;
  room_rnum rnum;

  argument = two_arguments(argument, arg, trig_name);
  two_arguments(argument, targ_name, loc_name);

  if (!*arg || !*targ_name || !*trig_name) {
    send_to_char(ch, "Usage: attach { mob | obj | room } { trigger } { name } [ location ]\r\n");
    return;
  }

  num_arg = atoi(targ_name);
  tn = atoi(trig_name);
  loc = (*loc_name) ? atoi(loc_name) : -1;

  if (is_abbrev(arg, "mobile") || is_abbrev(arg, "mtr")) {
    victim = get_char_vis(ch, targ_name, NULL, FIND_CHAR_WORLD);
    if (!victim) { /* search room for one with this vnum */
      for (victim = world[IN_ROOM(ch)].people;victim;victim=victim->next_in_room)
        if (GET_MOB_VNUM(victim) == num_arg)
          break;

      if (!victim) {
        send_to_char(ch, "That mob does not exist.\r\n");
        return;
      }
    }
    if (!IS_NPC(victim) && !CONFIG_SCRIPT_PLAYERS)  {
      send_to_char(ch, "Players can't have scripts.\r\n");
      return;
    }
    if (!can_edit_zone(ch, world[IN_ROOM(ch)].zone)) {
      send_to_char(ch, "You can only attach triggers in your own zone.\r\n");
      return;
    }
    /* have a valid mob, now get trigger */
    rn = real_trigger(tn);
    if ((rn == NOTHING) || !(trig = read_trigger(rn))) {
      send_to_char(ch, "That trigger does not exist.\r\n");
      return;
    }

    if (!SCRIPT(victim))
      CREATE(SCRIPT(victim), struct script_data, 1);
    add_trigger(SCRIPT(victim), trig, loc);

    if (IS_NPC(victim))
    send_to_char(ch, "Trigger %d (%s) attached to %s [%d].\r\n",
                 tn, GET_TRIG_NAME(trig), GET_SHORT(victim), GET_MOB_VNUM(victim));
    else 
    send_to_char(ch, "Trigger %d (%s) attached to player named %s.\r\n", 
                 tn, GET_TRIG_NAME(trig), GET_NAME(victim));
  }

  else if (is_abbrev(arg, "object") || is_abbrev(arg, "otr")) {
    object = get_obj_vis(ch, targ_name, NULL);
    if (!object) { /* search room for one with this vnum */
      for (object = world[IN_ROOM(ch)].contents;object;object=object->next_content)
        if (GET_OBJ_VNUM(object) == num_arg)
          break;

      if (!object) { /* search inventory for one with this vnum */
        for (object = ch->carrying;object;object=object->next_content)
          if (GET_OBJ_VNUM(object) == num_arg)
            break;

        if (!object) {
          send_to_char(ch, "That object does not exist.\r\n");
          return;
        }
      }
    }

    if (!can_edit_zone(ch, world[IN_ROOM(ch)].zone)) {
      send_to_char(ch, "You can only attach triggers in your own zone.\r\n");
      return;
    }
    /* have a valid obj, now get trigger */
    rn = real_trigger(tn);
    if ((rn == NOTHING) || !(trig = read_trigger(rn))) {
      send_to_char(ch, "That trigger does not exist.\r\n");
      return;
    }

    if (!SCRIPT(object))
      CREATE(SCRIPT(object), struct script_data, 1);
    add_trigger(SCRIPT(object), trig, loc);

    send_to_char(ch, "Trigger %d (%s) attached to %s [%d].\r\n",
                 tn, GET_TRIG_NAME(trig),
                 (object->short_description ?
                  object->short_description : object->name),
                  GET_OBJ_VNUM(object));
  }

  else if (is_abbrev(arg, "room") || is_abbrev(arg, "wtr")) {
    if (strchr(targ_name, '.'))
      rnum = IN_ROOM(ch);
    else if (isdigit(*targ_name))
      rnum = find_target_room(ch, targ_name);
    else
      rnum = NOWHERE;

    if (rnum == NOWHERE) {
      send_to_char(ch, "You need to supply a room number or . for current room.\r\n");
      return;
    }

    if (!can_edit_zone(ch, world[rnum].zone)) {
      send_to_char(ch, "You can only attach triggers in your own zone.\r\n");
      return;
    }
    /* have a valid room, now get trigger */
    rn = real_trigger(tn);
    if ((rn == NOTHING) || !(trig = read_trigger(rn))) {
      send_to_char(ch, "That trigger does not exist.\r\n");
      return;
    }

    room = &world[rnum];

    if (!SCRIPT(room))
      CREATE(SCRIPT(room), struct script_data, 1);
    add_trigger(SCRIPT(room), trig, loc);

    send_to_char(ch, "Trigger %d (%s) attached to room %d.\r\n",
                 tn, GET_TRIG_NAME(trig), world[rnum].number);
  }

  else
    send_to_char(ch, "Please specify 'mob', 'obj', or 'room'.\r\n");
}

/* Removes the trigger specified by name, and the script of o if it removes the
 * last trigger.  name can either be a number, or a 'silly' name for the 
 * trigger, including things like 2.beggar-death. Returns 0 if did not find the
 * trigger, otherwise 1.  If it matters, you might need to check to see if all 
 * the triggers were removed after this function returns, in order to remove 
 * the script. */
static int remove_trigger(struct script_data *sc, char *name)
{
  trig_data *i, *j;
  int num = 0, string = FALSE, n;
  char *cname;


  if (!sc)
    return 0;

  if ((cname = strstr(name,".")) || (!isdigit(*name)) ) {
    string = TRUE;
    if (cname) {
      *cname = '\0';
      num = atoi(name);
      name = ++cname;
    }
  } else
    num = atoi(name);

  for (n = 0, j = NULL, i = TRIGGERS(sc); i; j = i, i = i->next) {
    if (string) {
      if (isname(name, GET_TRIG_NAME(i)))
        if (++n >= num)
          break;
    }

    /* This isn't clean. A numeric value will match if it's position OR vnum 
     * is found. originally the number was position-only. */
    else if (++n >= num)
      break;
    else if (trig_index[i->nr]->vnum == num)
      break;
  }

  if (i) {
    if (j) {
      j->next = i->next;
      extract_trigger(i);
    }

    /* this was the first trigger */
    else {
      TRIGGERS(sc) = i->next;
      extract_trigger(i);
    }

    /* update the script type bitvector */
    SCRIPT_TYPES(sc) = 0;
    for (i = TRIGGERS(sc); i; i = i->next)
      SCRIPT_TYPES(sc) |= GET_TRIG_TYPE(i);

    return 1;
  } else
    return 0;
}

ACMD(do_detach)
{
  char_data *victim = NULL;
  obj_data *object = NULL;
  struct room_data *room;
  char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH], arg3[MAX_INPUT_LENGTH];
  char *trigger = 0;
  int num_arg;

  argument = two_arguments(argument, arg1, arg2);
  one_argument(argument, arg3);

  if (!*arg1 || !*arg2) {
    send_to_char(ch, "Usage: detach [ mob | object | room ] { target } { trigger |"
                 " 'all' }\r\n");
    return;
  }

  /* vnum of mob/obj, if given */
  num_arg = atoi(arg2);

  if (!str_cmp(arg1, "room") || !str_cmp(arg1, "wtr")) {
    room = &world[IN_ROOM(ch)];
    if (!can_edit_zone(ch, room->zone)) {
      send_to_char(ch, "You can only detach triggers in your own zone\r\n");
      return;
    }
    if (!SCRIPT(room))
      send_to_char(ch, "This room does not have any triggers.\r\n");
    else if (!str_cmp(arg2, "all")) {
      extract_script(room, WLD_TRIGGER);
      send_to_char(ch, "All triggers removed from room.\r\n");
    } else if (remove_trigger(SCRIPT(room), arg2)) {
      send_to_char(ch, "Trigger removed.\r\n");
      if (!TRIGGERS(SCRIPT(room))) {
        extract_script(room, WLD_TRIGGER);
      }
    } else
      send_to_char(ch, "That trigger was not found.\r\n");
  }

  else {
    if (is_abbrev(arg1, "mobile") || !str_cmp(arg1, "mtr")) {
      victim = get_char_vis(ch, arg2, NULL, FIND_CHAR_WORLD);
      if (!victim) { /* search room for one with this vnum */
        for (victim = world[IN_ROOM(ch)].people;victim;victim=victim->next_in_room)
          if (GET_MOB_VNUM(victim) == num_arg)
            break;

        if (!victim) {
          send_to_char(ch, "No such mobile around.\r\n");
          return;
        }
      }

      if (arg3 == NULL || !*arg3)
        send_to_char(ch, "You must specify a trigger to remove.\r\n");
      else
        trigger = arg3;
    }

    else if (is_abbrev(arg1, "object") || !str_cmp(arg1, "otr")) {
      object = get_obj_vis(ch, arg2, NULL);
      if (!object) { /* search room for one with this vnum */
        for (object = world[IN_ROOM(ch)].contents;object;object=object->next_content)
          if (GET_OBJ_VNUM(object) == num_arg)
            break;

        if (!object) { /* search inventory for one with this vnum */
          for (object = ch->carrying;object;object=object->next_content)
            if (GET_OBJ_VNUM(object) == num_arg)
              break;

          if (!object) { /* give up */
            send_to_char(ch, "No such object around.\r\n");
            return;
          }
        }
      }

      if (arg3 == NULL || !*arg3)
        send_to_char(ch, "You must specify a trigger to remove.\r\n");
      else
        trigger = arg3;
    }
    else  {
      /* Thanks to Carlos Myers for fixing the line below */
      if ((object = get_obj_in_equip_vis(ch, arg1, NULL, ch->equipment)));
      else if ((object = get_obj_in_list_vis(ch, arg1, NULL, ch->carrying)));
      else if ((victim = get_char_room_vis(ch, arg1, NULL)));
      else if ((object = get_obj_in_list_vis(ch, arg1, NULL, world[IN_ROOM(ch)].contents)));
      else if ((victim = get_char_vis(ch, arg1, NULL, FIND_CHAR_WORLD)));
      else if ((object = get_obj_vis(ch, arg1, NULL)));
      else
        send_to_char(ch, "Nothing around by that name.\r\n");

      trigger = arg2;
    }

    if (victim) {
      if (!IS_NPC(victim) && !CONFIG_SCRIPT_PLAYERS)
      {
        send_to_char(ch, "Players don't have triggers.\r\n");
        return;
      }

      if (!SCRIPT(victim)) 
        send_to_char(ch, "That %s doesn't have any triggers.\r\n", IS_NPC(victim) ? "mob" : "player");  
      else if (!can_edit_zone(ch, real_zone_by_thing(GET_MOB_VNUM(victim))) && IS_NPC(victim)) {
        send_to_char(ch, "You can only detach triggers in your own zone\r\n");
        return;
      }
      else if (trigger && !str_cmp(trigger, "all")) {
        extract_script(victim, MOB_TRIGGER);
        send_to_char(ch, "All triggers removed from %s.\r\n", IS_NPC(victim) ? GET_SHORT(victim) : GET_NAME(victim));
      }

      else if (trigger && remove_trigger(SCRIPT(victim), trigger)) {
        send_to_char(ch, "Trigger removed.\r\n");
        if (!TRIGGERS(SCRIPT(victim))) {
          extract_script(victim, MOB_TRIGGER);
        }
      } else
        send_to_char(ch, "That trigger was not found.\r\n");
    }

    else if (object) {
      if (!SCRIPT(object))
        send_to_char(ch, "That object doesn't have any triggers.\r\n");

      else if (!can_edit_zone(ch, real_zone_by_thing(GET_OBJ_VNUM(object)))) {
        send_to_char(ch, "You can only detach triggers in your own zone\r\n");
        return;
      }
      else if (trigger && !str_cmp(trigger, "all")) {
        extract_script(object, OBJ_TRIGGER);
        send_to_char(ch, "All triggers removed from %s.\r\n",
                object->short_description ? object->short_description :
                object->name);
      }

      else if (remove_trigger(SCRIPT(object), trigger)) {
        send_to_char(ch, "Trigger removed.\r\n");
        if (!TRIGGERS(SCRIPT(object))) {
          extract_script(object, OBJ_TRIGGER);
        }
      } else
        send_to_char(ch, "That trigger was not found.\r\n");
    }
  }
}

/* Logs any errors caused by scripts to the system log. Will eventually allow 
 * on-line view of script errors. */
void script_vlog(const char *format, va_list args)
{
  char output[MAX_STRING_LENGTH];
  struct descriptor_data *i;

  /* parse the args, making the error message */ 
  vsnprintf(output, sizeof(output) - 2, format, args); 

  /* Save to the syslog file */ 
  basic_mud_log("SCRIPT ERROR: %s", output); 

  /* And send to imms */ 
  for (i = descriptor_list; i; i = i->next) { 
    if (STATE(i) != CON_PLAYING || IS_NPC(i->character)) /* switch */ 
      continue; 
    if (GET_LEVEL(i->character) < LVL_BUILDER) 
      continue; 
    if (PLR_FLAGGED(i->character, PLR_WRITING)) 
      continue; 
    if (NRM > (PRF_FLAGGED(i->character, PRF_LOG1) ? 1 : 0) + (PRF_FLAGGED(i->character, PRF_LOG2) ? 2 : 0)) 
      continue; 

    send_to_char(i->character, "%s[ %s ]%s\r\n", CCGRN(i->character, C_NRM), output, CCNRM(i->character, C_NRM)); 
  }
}

void script_log(const char *format, ...)
{
  va_list args;

  va_start(args, format);
  script_vlog(format, args);
  va_end(args);
}

/* Returns 1 if string is all digits, else 0. Bugfixed - would have returned 
 * true on num="------". */
static int is_num(char *arg)
{
   if (*arg == '\0')
      return FALSE;

   if (*arg == '+' || *arg == '-')
      arg++;

   for (; *arg != '\0'; arg++)
   {
      if (!isdigit(*arg))
         return FALSE;
   }

   return TRUE;
}

/* evaluates 'lhs op rhs', and copies to result */
static void eval_op(char *op, char *lhs, char *rhs, char *result, void *go,
             struct script_data *sc, trig_data *trig)
{
  unsigned char *p;
  int n;

  /* strip off extra spaces at begin and end */
  while (*lhs && isspace(*lhs))
    lhs++;
  while (*rhs && isspace(*rhs))
    rhs++;

  for (p = (unsigned char *) lhs; *p; p++);
  for (--p; isspace(*p) && ((char *)p > lhs); *p-- = '\0');
  for (p = (unsigned char *) rhs; *p; p++);
  for (--p; isspace(*p) && ((char *)p > rhs); *p-- = '\0');


  /* find the op, and figure out the value */
  if (!strcmp("||", op)) {
    if ((!*lhs || (*lhs == '0')) && (!*rhs || (*rhs == '0')))
      strcpy(result, "0");
    else
      strcpy(result, "1");
  }

  else if (!strcmp("&&", op)) {
    if (!*lhs || (*lhs == '0') || !*rhs || (*rhs == '0'))
      strcpy (result, "0");
    else
      strcpy (result, "1");
  }

  else if (!strcmp("==", op)) {
    if (is_num(lhs) && is_num(rhs))
      sprintf(result, "%d", atoi(lhs) == atoi(rhs));
    else
      sprintf(result, "%d", !str_cmp(lhs, rhs));
  }

  else if (!strcmp("!=", op)) {
    if (is_num(lhs) && is_num(rhs))
      sprintf(result, "%d", atoi(lhs) != atoi(rhs));
    else
      sprintf(result, "%d", str_cmp(lhs, rhs));
  }

  else if (!strcmp("<=", op)) {
    if (is_num(lhs) && is_num(rhs))
      sprintf(result, "%d", atoi(lhs) <= atoi(rhs));
    else
      sprintf(result, "%d", str_cmp(lhs, rhs) <= 0);
  }

  else if (!strcmp(">=", op)) {
    if (is_num(lhs) && is_num(rhs))
      sprintf(result, "%d", atoi(lhs) >= atoi(rhs));
    else
      sprintf(result, "%d", str_cmp(lhs, rhs) <= 0);
  }

  else if (!strcmp("<", op)) {
    if (is_num(lhs) && is_num(rhs))
      sprintf(result, "%d", atoi(lhs) < atoi(rhs));
    else
      sprintf(result, "%d", str_cmp(lhs, rhs) < 0);
  }

  else if (!strcmp(">", op)) {
    if (is_num(lhs) && is_num(rhs))
      sprintf(result, "%d", atoi(lhs) > atoi(rhs));
    else
      sprintf(result, "%d", str_cmp(lhs, rhs) > 0);
  }

  else if (!strcmp("/=", op))
    sprintf(result, "%c", str_str(lhs, rhs) ? '1' : '0');

  else if (!strcmp("*", op))
    sprintf(result, "%d", atoi(lhs) * atoi(rhs));

  else if (!strcmp("/", op))
    sprintf(result, "%d", (n = atoi(rhs)) ? (atoi(lhs) / n) : 0);

  else if (!strcmp("+", op))
    sprintf(result, "%d", atoi(lhs) + atoi(rhs));

  else if (!strcmp("-", op))
    sprintf(result, "%d", atoi(lhs) - atoi(rhs));

  else if (!strcmp("!", op)) {
    if (is_num(rhs))
      sprintf(result, "%d", !atoi(rhs));
    else
      sprintf(result, "%d", !*rhs);
  }
}

/* p points to the first quote, returns the matching end quote, or the last 
 * non-null char in p.*/
char *matching_quote(char *p)
{
  for (p++; *p && (*p != '"'); p++) {
    if (*p == '\\')
      p++;
  }

  if (!*p)
    p--;

  return p;
}

/* p points to the first paren.  returns a pointer to the matching closing 
 * paren, or the last non-null char in p. */
static char *matching_paren(char *p)
{
  int i;

  for (p++, i = 1; *p && i; p++) {
    if (*p == '(')
      i++;
    else if (*p == ')')
      i--;
    else if (*p == '"')
      p = matching_quote(p);
  }

  return --p;
}

/* evaluates line, and returns answer in result */
static void eval_expr(char *line, char *result, void *go, struct script_data *sc,
               trig_data *trig, int type)
{
  char expr[MAX_INPUT_LENGTH], *p;

  while (*line && isspace(*line))
    line++;

  if (eval_lhs_op_rhs(line, result, go, sc, trig, type));

  else if (*line == '(') {
    p = strcpy(expr, line);
    p = matching_paren(expr);
    *p = '\0';
    eval_expr(expr + 1, result, go, sc, trig, type);
  }

  else
    var_subst(go, sc, trig, type, line, result);
}

/* Evaluates expr if it is in the form lhs op rhs, and copies answer in result.
 * Returns 1 if expr is evaluated, else 0. */
static int eval_lhs_op_rhs(char *expr, char *result, void *go, struct script_data *sc,
                    trig_data *trig, int type)
{
  char *p, *tokens[MAX_INPUT_LENGTH];
  char line[MAX_INPUT_LENGTH], lhr[MAX_INPUT_LENGTH], rhr[MAX_INPUT_LENGTH];
  int i, j;

  /*
   * valid operands, in order of priority
   * each must also be defined in eval_op()
   */
  static char *ops[] = {
    "||",
    "&&",
    "==",
    "!=",
    "<=",
    ">=",
    "<",
    ">",
    "/=",
    "-",
    "+",
    "/",
    "*",
    "!",
    "\n"
  };

  p = strcpy(line, expr);

  /* Initialize tokens, an array of pointers to locations in line where the 
   * ops could possibly occur. */
  for (j = 0; *p; j++) {
    tokens[j] = p;
    if (*p == '(')
      p = matching_paren(p) + 1;
    else if (*p == '"')
      p = matching_quote(p) + 1;
    else if (isalnum(*p))
      for (p++; *p && (isalnum(*p) || isspace(*p)); p++);
    else
      p++;
  }
  tokens[j] = NULL;

  for (i = 0; *ops[i] != '\n'; i++)
    for (j = 0; tokens[j]; j++)
      if (!strn_cmp(ops[i], tokens[j], strlen(ops[i]))) {
        *tokens[j] = '\0';
        p = tokens[j] + strlen(ops[i]);

        eval_expr(line, lhr, go, sc, trig, type);
        eval_expr(p, rhr, go, sc, trig, type);
        eval_op(ops[i], lhr, rhr, result, go, sc, trig);

        return 1;
      }

  return 0;
}

/* returns 1 if cond is true, else 0 */
static int process_if(char *cond, void *go, struct script_data *sc,
               trig_data *trig, int type)
{
  char result[MAX_INPUT_LENGTH], *p;

  eval_expr(cond, result, go, sc, trig, type);

  p = result;
  skip_spaces(&p);

  if (!*p || *p == '0')
    return 0;
  else
    return 1;
}

/* Scans for end of if-block.  returns the line containg 'end', or the last
 * line of the trigger if not found. */
static struct cmdlist_element *find_end(trig_data *trig, struct cmdlist_element *cl)
{
  struct cmdlist_element *c;
  char *p;

  if (!(cl->next)) { /* rryan: if this is the last line, theres no end */
    script_log("Trigger VNum %d has 'if' without 'end'. (error 1)", GET_TRIG_VNUM(trig));
    return cl;
  }

  for (c = cl->next; c; c = c->next) {
    for (p = c->cmd; *p && isspace(*p); p++);

    if (!strn_cmp("if ", p, 3))
      c = find_end(trig, c);
    else if (!strn_cmp("end", p, 3))
      return c;

    /* thanks to Russell Ryan for this fix */
    if(!c->next) { /* rryan: this is the last line, we didn't find an end. */
      script_log("Trigger VNum %d has 'if' without 'end'. (error 2)", GET_TRIG_VNUM(trig));
      return c;
    }
  }

  /* rryan: we didn't find an end */
  script_log("Trigger VNum %d has 'if' without 'end'. (error 3)", GET_TRIG_VNUM(trig));
  return c;
}

/* Searches for valid elseif, else, or end to continue execution at. Returns 
 * line of elseif, else, or end if found, or last line of trigger. */
static struct cmdlist_element *find_else_end(trig_data *trig,
    struct cmdlist_element *cl, void *go, struct script_data *sc, int type)
{
  struct cmdlist_element *c;
  char *p;

  if (!(cl->next))
    return cl;

  for (c = cl->next;c->next; c = c->next) {
    for (p = c->cmd; *p && isspace(*p); p++); /* skip spaces */

    if (!strn_cmp("if ", p, 3))
      c = find_end(trig, c);

    else if (!strn_cmp("elseif ", p, 7)) {
      if (process_if(p + 7, go, sc, trig, type)) {
        GET_TRIG_DEPTH(trig)++;
        return c;
      }
    }

    else if (!strn_cmp("else", p, 4)) {
      GET_TRIG_DEPTH(trig)++;
      return c;
    }

    else if (!strn_cmp("end", p, 3))
      return c;

    /* thanks to Russell Ryan for this fix */
    if(!c->next) { /* rryan: this is the last line, return. */
      script_log("Trigger VNum %d has 'if' without 'end'. (error 4)", GET_TRIG_VNUM(trig));
      return c;
    }
  }

  /* rryan: if we got here, it's the last line, if its not an end, log it. */
  for (p = c->cmd; *p && isspace(*p); p++); /* skip spaces */
  if(strn_cmp("end", p, 3))
    script_log("Trigger VNum %d has 'if' without 'end'. (error 5)", GET_TRIG_VNUM(trig));
  return c;
}

/* processes any 'wait' commands in a trigger */
static void process_wait(void *go, trig_data *trig, int type, char *cmd,
                  struct cmdlist_element *cl)
{
  char buf[MAX_INPUT_LENGTH], *arg;
  struct wait_event_data *wait_event_obj;
  long when, hr, min, ntime;
  char c;

  arg = any_one_arg(cmd, buf);
  skip_spaces(&arg);

  if (!*arg) {
    script_log("Trigger: %s, VNum %d. wait w/o an arg: '%s'",
            GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig), cl->cmd);
    return;
  }

  if (!strn_cmp(arg, "until ", 6)) {

    /* valid forms of time are 14:30 and 1430 */
    if (sscanf(arg, "until %ld:%ld", &hr, &min) == 2)
      min += (hr * 60);
    else
      min = (hr % 100) + ((hr / 100) * 60);

    /* calculate the pulse of the day of "until" time */
    ntime = (min * SECS_PER_MUD_HOUR * PASSES_PER_SEC) / 60;

    /* calculate pulse of day of current time */
    when = (pulse % (SECS_PER_MUD_HOUR * PASSES_PER_SEC)) +
      (time_info.hours * SECS_PER_MUD_HOUR * PASSES_PER_SEC);

    if (when >= ntime) /* adjust for next day */
      when = (SECS_PER_MUD_DAY * PASSES_PER_SEC) - when + ntime;
    else
      when = ntime - when;
  }

  else {
    if (sscanf(arg, "%ld %c", &when, &c) == 2) {
      if (c == 't')
        when *= PULSES_PER_MUD_HOUR;
      else if (c == 's')
        when *= PASSES_PER_SEC;
    }
  }

  CREATE(wait_event_obj, struct wait_event_data, 1);
  wait_event_obj->trigger = trig;
  wait_event_obj->go = go;
  wait_event_obj->type = type;

  GET_TRIG_WAIT(trig) = event_create(trig_wait_event, wait_event_obj, when);
  trig->curr_state = cl->next;
}

/* processes a script set command */
static void process_set(struct script_data *sc, trig_data *trig, char *cmd)
{
  char arg[MAX_INPUT_LENGTH], name[MAX_INPUT_LENGTH], *value;

  value = two_arguments(cmd, arg, name);

  skip_spaces(&value);

  if (!*name) {
    script_log("Trigger: %s, VNum %d. set w/o an arg: '%s'",
            GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig), cmd);
    return;
  }

  add_var(&GET_TRIG_VARS(trig), name, value, sc ? sc->context : 0);

}

/* processes a script eval command */
void process_eval(void *go, struct script_data *sc, trig_data *trig,
                 int type, char *cmd)
{
  char arg[MAX_INPUT_LENGTH], name[MAX_INPUT_LENGTH];
  char result[MAX_INPUT_LENGTH], *expr;

  expr = one_argument(cmd, arg); /* cut off 'eval' */
  expr = one_argument(expr, name); /* cut off name */

  skip_spaces(&expr);

  if (!*name) {
    script_log("Trigger: %s, VNum %d. eval w/o an arg: '%s'",
            GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig), cmd);
    return;
  }

  eval_expr(expr, result, go, sc, trig, type);
  add_var(&GET_TRIG_VARS(trig), name, result, sc ? sc->context : 0);
}

/* script attaching a trigger to something */
static void process_attach(void *go, struct script_data *sc, trig_data *trig,
                    int type, char *cmd)
{
  char arg[MAX_INPUT_LENGTH], trignum_s[MAX_INPUT_LENGTH];
  char result[MAX_INPUT_LENGTH], *id_p;
  trig_data *newtrig;
  char_data *c=NULL;
  obj_data *o=NULL;
  room_data *r=NULL;
  long trignum, id;

  id_p = two_arguments(cmd, arg, trignum_s);
  skip_spaces(&id_p);

  if (!*trignum_s) {
    script_log("Trigger: %s, VNum %d. attach w/o an arg: '%s'",
            GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig), cmd);
    return;
  }

  if (!id_p || !*id_p || atoi(id_p)==0) {
    script_log("Trigger: %s, VNum %d. attach invalid id arg: '%s'",
            GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig), cmd);
    return;
  }

  /* parse and locate the id specified */
  eval_expr(id_p, result, go, sc, trig, type);
  if (!(id = atoi(result))) {
    script_log("Trigger: %s, VNum %d. attach invalid id arg: '%s'",
            GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig), cmd);
    return;
  }
  c = find_char(id);
  if (!c) {
    o = find_obj(id);
    if (!o) {
      r = find_room(id);
      if (!r) {
        script_log("Trigger: %s, VNum %d. attach invalid id arg: '%s'",
                GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig), cmd);
        return;
      }
    }
  }

  /* locate and load the trigger specified */
  trignum = real_trigger(atoi(trignum_s));
  if (trignum == NOTHING || !(newtrig=read_trigger(trignum))) {
    script_log("Trigger: %s, VNum %d. attach invalid trigger: '%s'",
            GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig), trignum_s);
    return;
  }

  if (c) {
    if (!IS_NPC(c) && !CONFIG_SCRIPT_PLAYERS) {
      script_log("Trigger: %s, VNum %d. attach invalid target: '%s'",
              GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig), GET_NAME(c));
      return;
    }
    if (!SCRIPT(c))
      CREATE(SCRIPT(c), struct script_data, 1);
    add_trigger(SCRIPT(c), newtrig, -1);
    return;
  }

  if (o) {
    if (!SCRIPT(o))
      CREATE(SCRIPT(o), struct script_data, 1);
    add_trigger(SCRIPT(o), newtrig, -1);
    return;
  }

  if (r) {
    if (!SCRIPT(r))
      CREATE(SCRIPT(r), struct script_data, 1);
    add_trigger(SCRIPT(r), newtrig, -1);
    return;
  }
}

/* script detaching a trigger from something */
static void process_detach(void *go, struct script_data *sc, trig_data *trig,
    int type, char *cmd)
{
  char arg[MAX_INPUT_LENGTH], trignum_s[MAX_INPUT_LENGTH];
  char result[MAX_INPUT_LENGTH], *id_p;
  char_data *c=NULL;
  obj_data *o=NULL;
  room_data *r=NULL;
  long id;

  id_p = two_arguments(cmd, arg, trignum_s);
  skip_spaces(&id_p);

  if (!*trignum_s) {
    script_log("Trigger: %s, VNum %d. detach w/o an arg: '%s'",
            GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig), cmd);
    return;
  }

  if (!id_p || !*id_p || atoi(id_p)==0) {
    script_log("Trigger: %s, VNum %d. detach invalid id arg: '%s'",
            GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig), cmd);
    return;
  }

  /* parse and locate the id specified */
  eval_expr(id_p, result, go, sc, trig, type);
  if (!(id = atoi(result))) {
    script_log("Trigger: %s, VNum %d. detach invalid id arg: '%s'",
            GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig), cmd);
    return;
  }
  c = find_char(id);
  if (!c) {
    o = find_obj(id);
    if (!o) {
      r = find_room(id);
      if (!r) {
        script_log("Trigger: %s, VNum %d. detach invalid id arg: '%s'",
                GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig), cmd);
        return;
      }
    }
  }


  if (c && SCRIPT(c)) {
    if (!strcmp(trignum_s, "all")) {
      extract_script(c, MOB_TRIGGER);
      return;
    }
    if (remove_trigger(SCRIPT(c), trignum_s)) {
      if (!TRIGGERS(SCRIPT(c))) {
        extract_script(c, MOB_TRIGGER);
      }
    }
    return;
  }

  if (o && SCRIPT(o)) {
    if (!strcmp(trignum_s, "all")) {
      extract_script(o, OBJ_TRIGGER);
      return;
    }
    if (remove_trigger(SCRIPT(o), trignum_s)) {
      if (!TRIGGERS(SCRIPT(o))) {
        extract_script(o, OBJ_TRIGGER);
      }
    }
    return;
  }

  if (r && SCRIPT(r)) {
    if (!strcmp(trignum_s, "all")) {
      extract_script(r, WLD_TRIGGER);
      return;
    }
    if (remove_trigger(SCRIPT(r), trignum_s)) {
      if (!TRIGGERS(SCRIPT(r))) {
        extract_script(r, WLD_TRIGGER);
      }
    }
    return;
  }

}

struct room_data *dg_room_of_obj(struct obj_data *obj)
{
  if (IN_ROOM(obj) != NOWHERE) return &world[IN_ROOM(obj)];
  if (obj->carried_by)        return &world[IN_ROOM(obj->carried_by)];
  if (obj->worn_by)           return &world[IN_ROOM(obj->worn_by)];
  if (obj->in_obj)            return (dg_room_of_obj(obj->in_obj));
  return NULL;
}

/* create a UID variable from the id number */
static void makeuid_var(void *go, struct script_data *sc, trig_data *trig,
                 int type, char *cmd)
{
  char junk[MAX_INPUT_LENGTH], varname[MAX_INPUT_LENGTH];
  char arg[MAX_INPUT_LENGTH], name[MAX_INPUT_LENGTH];
  char uid[MAX_INPUT_LENGTH];

  *uid = '\0';
  half_chop(cmd, junk, cmd);    /* makeuid */
  half_chop(cmd, varname, cmd); /* variable name */
  half_chop(cmd, arg, cmd);     /* numerical id or 'obj' 'mob' or 'room' */
  half_chop(cmd, name, cmd);    /* if the above was obj, mob or room, this is the name */

  if (!*varname) {
    script_log("Trigger: %s, VNum %d. makeuid w/o an arg: '%s'",
            GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig), cmd);

    return;
  }

  if (arg == NULL || !*arg) {
    script_log("Trigger: %s, VNum %d. makeuid invalid id arg: '%s'",
            GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig), cmd);
    return;
  }

  if (atoi(arg)!=0) { /* easy, if you pass an id number */
    char result[MAX_INPUT_LENGTH];

    eval_expr(arg, result, go, sc, trig, type);
    snprintf(uid, sizeof(uid), "%c%s", UID_CHAR, result);
  } else { /* a lot more work without it */
    if (name == NULL || !*name) {
      script_log("Trigger: %s, VNum %d. makeuid needs name: '%s'",
            GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig), cmd);
      return;
    }

    if (is_abbrev(arg, "mob")) {
      struct char_data *c = NULL;
      switch (type) {
        case WLD_TRIGGER:
          c = get_char_in_room((struct room_data *)go, name);
          break;
        case OBJ_TRIGGER:
          c = get_char_near_obj((struct obj_data *)go, name);
          break;
        case MOB_TRIGGER:
          c = get_char_room_vis((struct char_data *)go, name, NULL);
          break;
      }
      if (c)
        snprintf(uid, sizeof(uid), "%c%ld", UID_CHAR, GET_ID(c));
    } else if (is_abbrev(arg, "obj")) {
      struct obj_data *o = NULL;
      switch (type) {
        case WLD_TRIGGER:
          o = get_obj_in_room((struct room_data *)go, name);
          break;
        case OBJ_TRIGGER:
          o = get_obj_near_obj((struct obj_data *)go, name);
          break;
        case MOB_TRIGGER:
          if ((o = get_obj_in_list_vis((struct char_data *)go, name, NULL,
                    ((struct char_data *)go)->carrying)) == NULL)
            o = get_obj_in_list_vis((struct char_data *)go, name, NULL,
                    world[IN_ROOM((struct char_data *)go)].contents);
          break;
      }
      if (o)
        snprintf(uid, sizeof(uid), "%c%ld", UID_CHAR, GET_ID(o));
    } else if (is_abbrev(arg, "room")) {
      room_rnum r = NOWHERE;
      switch (type) {
        case WLD_TRIGGER:
          r = real_room(((struct room_data *) go)->number);
          break;
        case OBJ_TRIGGER:
          r = obj_room((struct obj_data *)go);
          break;
        case MOB_TRIGGER:
          r = IN_ROOM((struct char_data *)go);
          break;
      }
      if (r != NOWHERE)
        snprintf(uid, sizeof(uid), "%c%ld", UID_CHAR, (long)world[r].number+ROOM_ID_BASE);
    } else {
      script_log("Trigger: %s, VNum %d. makeuid syntax error: '%s'",
            GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig), cmd);

      return;
    }
  }
  if (*uid)
    add_var(&GET_TRIG_VARS(trig), varname, uid, sc ? sc->context : 0);
}

/* Processes a script return command. Returns the new value for the script to 
 * return. */
static int process_return(trig_data *trig, char *cmd)
{
  char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

  two_arguments(cmd, arg1, arg2);

  if (!*arg2) {
    script_log("Trigger: %s, VNum %d. return w/o an arg: '%s'",
            GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig), cmd);

    return 1;
  }

  return atoi(arg2);
}

/* Removes a variable from the global vars of sc, or the local vars of trig if 
 * not found in global list. */
static void process_unset(struct script_data *sc, trig_data *trig, char *cmd)
{
  char arg[MAX_INPUT_LENGTH], *var;

  var = any_one_arg(cmd, arg);

  skip_spaces(&var);

  if (!*var) {
    script_log("Trigger: %s, VNum %d. unset w/o an arg: '%s'",
            GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig), cmd);
    return;
  }

  if (!remove_var(&(sc->global_vars), var))
    remove_var(&GET_TRIG_VARS(trig), var);
}

/* Copy a locally owned variable to the globals of another script.
 * 'remote <variable_name> <uid>' */
static void process_remote(struct script_data *sc, trig_data *trig, char *cmd)
{
  struct trig_var_data *vd;
  struct script_data *sc_remote=NULL;
  char *line, *var, *uid_p;
  char arg[MAX_INPUT_LENGTH], buf[MAX_INPUT_LENGTH], buf2[MAX_INPUT_LENGTH];
  long uid, context;
  room_data *room;
  char_data *mob;
  obj_data *obj;

  line = any_one_arg(cmd, arg);
  two_arguments(line, buf, buf2);
  var = buf;
  uid_p = buf2;
  skip_spaces(&var);
  skip_spaces(&uid_p);


  if (!*buf || !*buf2) {
    script_log("Trigger: %s, VNum %d. remote: invalid arguments '%s'",
            GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig), cmd);
    return;
  }

  /* find the locally owned variable */
  for (vd = GET_TRIG_VARS(trig); vd; vd = vd->next)
    if (!str_cmp(vd->name, buf))
      break;

  if (!vd)
    for (vd = sc->global_vars; vd; vd = vd->next)
      if (!str_cmp(vd->name, var) &&
          (vd->context==0 || vd->context==sc->context))
        break;

  if (!vd) {
    script_log("Trigger: %s, VNum %d. local var '%s' not found in remote call",
            GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig), buf);
    return;
  }
  /* find the target script from the uid number */
  uid = atoi(buf2);
  if (uid<=0) {
    script_log("Trigger: %s, VNum %d. remote: illegal uid '%s'",
            GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig), buf2);
    return;
  }

  /* For all but PC's, context comes from the existing context. For PC's, 
   * context is 0 (global) */
  context = vd->context;

  if ((room = find_room(uid))) {
    sc_remote = SCRIPT(room);
  } else if ((mob = find_char(uid))) {
    sc_remote = SCRIPT(mob);
    if (!IS_NPC(mob)) context = 0;
  } else if ((obj = find_obj(uid))) {
    sc_remote = SCRIPT(obj);
  } else {
    script_log("Trigger: %s, VNum %d. remote: uid '%ld' invalid",
            GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig), uid);
    return;
  }

  if (sc_remote==NULL) return; /* no script to assign */

  add_var(&(sc_remote->global_vars), vd->name, vd->value, context);
}

/* Command-line interface to rdelete. Named vdelete so people didn't think it 
 * was to delete rooms. */
ACMD(do_vdelete)
{
  struct trig_var_data *vd, *vd_prev=NULL;
  struct script_data *sc_remote=NULL;
  char *var, *uid_p;
  char buf[MAX_INPUT_LENGTH], buf2[MAX_INPUT_LENGTH];
  long uid, context;
  room_data *room;
  char_data *mob;
  obj_data *obj;

  argument = two_arguments(argument, buf, buf2);
  var = buf;
  uid_p = buf2;
  skip_spaces(&var);
  skip_spaces(&uid_p);


  if (!*buf || !*buf2) {
    send_to_char(ch, "Usage: vdelete { <variablename> | * | all } <id>\r\n");
    return;
  }

  /* find the target script from the uid number */
  uid = atoi(buf2);
  if (uid<=0) {
    send_to_char(ch, "vdelete: illegal id specified.\r\n");
    return;
  }

  if ((room = find_room(uid))) {
    sc_remote = SCRIPT(room);
  } else if ((mob = find_char(uid))) {
    sc_remote = SCRIPT(mob);
    if (!IS_NPC(mob)) context = 0;
  } else if ((obj = find_obj(uid))) {
    sc_remote = SCRIPT(obj);
  } else {
    send_to_char(ch, "vdelete: cannot resolve specified id.\r\n");
    return;
  }

  if (sc_remote==NULL) {
    send_to_char(ch, "That id represents no global variables.(1)\r\n");
    return;
  }

  if (sc_remote->global_vars==NULL) {
    send_to_char(ch, "That id represents no global variables.(2)\r\n");
    return;
  }

  if (*var == '*' || is_abbrev(var, "all")) {
    struct trig_var_data *vd_next;
    for (vd = sc_remote->global_vars; vd; vd = vd_next) {
      vd_next = vd->next;
      free(vd->value);
      free(vd->name);
      free(vd);
    }
    sc_remote->global_vars = NULL;
    send_to_char(ch, "All variables deleted from that id.\r\n");
    return;
  }

  /* find the global */
  for (vd = sc_remote->global_vars; vd; vd_prev = vd, vd = vd->next)
    if (!str_cmp(vd->name, var))
      break;

  if (!vd) {
    send_to_char(ch, "That variable cannot be located.\r\n");
    return;
  }

  /* ok, delete the variable */
  if (vd_prev) vd_prev->next = vd->next;
  else sc_remote->global_vars = vd->next;

  /* and free up the space */
  free(vd->value);
  free(vd->name);
  free(vd);

  send_to_char(ch, "Deleted.\r\n");
}

/* Called from do_set - return 0 for failure, 1 for success.  ch and vict are 
 * verified. */
int perform_set_dg_var(struct char_data *ch, struct char_data *vict, char *val_arg)
{
  char var_name[MAX_INPUT_LENGTH], *var_value;

  var_value = any_one_arg(val_arg, var_name);

  if (var_name == NULL || !*var_name || var_value == NULL || !*var_value) {
    send_to_char(ch, "Usage: set <char> <varname> <value>\r\n");
    return 0;
  }
  if (!SCRIPT(vict))
    CREATE(SCRIPT(vict), struct script_data, 1);

  add_var(&(SCRIPT(vict)->global_vars), var_name, var_value, 0);
  return 1;
}

/* Delete a variable from the globals of another script.
 * 'rdelete <variable_name> <uid>' */
static void process_rdelete(struct script_data *sc, trig_data *trig, char *cmd)
{
  struct trig_var_data *vd, *vd_prev=NULL;
  struct script_data *sc_remote=NULL;
  char *line, *var, *uid_p;
  char arg[MAX_INPUT_LENGTH], buf[MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH];
  long uid, context;
  room_data *room;
  char_data *mob;
  obj_data *obj;

  line = any_one_arg(cmd, arg);
  two_arguments(line, buf, buf2);
  var = buf;
  uid_p = buf2;
  skip_spaces(&var);
  skip_spaces(&uid_p);

  if (!*buf || !*buf2) {
    script_log("Trigger: %s, VNum %d. rdelete: invalid arguments '%s'",
            GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig), cmd);
    return;
  }

  /* find the target script from the uid number */
  uid = atoi(buf2);
  if (uid<=0) {
    script_log("Trigger: %s, VNum %d. rdelete: illegal uid '%s'",
            GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig), buf2);
    return;
  }

  if ((room = find_room(uid))) {
    sc_remote = SCRIPT(room);
  } else if ((mob = find_char(uid))) {
    sc_remote = SCRIPT(mob);
    if (!IS_NPC(mob)) context = 0;
  } else if ((obj = find_obj(uid))) {
    sc_remote = SCRIPT(obj);
  } else {
    script_log("Trigger: %s, VNum %d. remote: uid '%ld' invalid",
            GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig), uid);
    return;
  }

  if (sc_remote==NULL) return; /* no script to delete a trigger from */
  if (sc_remote->global_vars==NULL) return; /* no script globals */

  /* find the global */
  for (vd = sc_remote->global_vars; vd; vd_prev = vd, vd = vd->next)
    if (!str_cmp(vd->name, var) &&
        (vd->context==0 || vd->context==sc->context))
      break;

  if (!vd) return; /* the variable doesn't exist, or is the wrong context */

  /* ok, delete the variable */
  if (vd_prev) vd_prev->next = vd->next;
  else sc_remote->global_vars = vd->next;

  /* and free up the space */
  free(vd->value);
  free(vd->name);
  free(vd);
}

/* Makes a local variable into a global variable. */
static void process_global(struct script_data *sc, trig_data *trig, char *cmd, long id)
{
  struct trig_var_data *vd;
  char arg[MAX_INPUT_LENGTH], *var;

  var = any_one_arg(cmd, arg);

  skip_spaces(&var);

  if (!*var) {
    script_log("Trigger: %s, VNum %d. global w/o an arg: '%s'",
            GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig), cmd);
    return;
  }

  for (vd = GET_TRIG_VARS(trig); vd; vd = vd->next)
    if (!str_cmp(vd->name, var))
      break;

  if (!vd) {
    script_log("Trigger: %s, VNum %d. local var '%s' not found in global call",
            GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig), var);
    return;
  }

  add_var(&(sc->global_vars), vd->name, vd->value, id);
  remove_var(&GET_TRIG_VARS(trig), vd->name);
}

/* set the current context for a script */
static void process_context(struct script_data *sc, trig_data *trig, char *cmd)
{
  char arg[MAX_INPUT_LENGTH], *var;

  var = any_one_arg(cmd, arg);

  skip_spaces(&var);

  if (!*var) {
    script_log("Trigger: %s, VNum %d. context w/o an arg: '%s'",
            GET_TRIG_NAME(trig), GET_TRIG_VNUM(trig), cmd);
    return;
  }

  sc->context = atol(var);
}

static void extract_value(struct script_data *sc, trig_data *trig, char *cmd)
{
  char buf[MAX_INPUT_LENGTH], buf2[MAX_INPUT_LENGTH];
  char *buf3;
  char to[128];
  int num;

  buf3 = any_one_arg(cmd, buf);
  half_chop(buf3, buf2, buf);
  strcpy(to, buf2);

  num = atoi(buf);
  if (num < 1) {
    script_log("extract number < 1!");
    return;
  }

  half_chop(buf, buf3, buf2);

  while (num>0) {
    half_chop(buf2, buf, buf2);
    num--;
  }

  add_var(&GET_TRIG_VARS(trig), to, buf, sc ? sc->context : 0);
}

/* Thanks to Jamie Nelson for 4 dimensions for this addition.
 * Syntax :
 * dg_letter <new varname> <letter position> <string to get from>. 
 *   set string L337-String
 *   dg_letter var1 4 %string%
 *   dg_letter var2 11 %string%
 *   now %var1% == 7 and %var2% == g
 *   Note that the index starts at 1. */
static void dg_letter_value(struct script_data *sc, trig_data *trig, char *cmd)
{
  /* Set the letter/number at position 'num' as the variable. */
  char junk[MAX_INPUT_LENGTH];
  char varname[MAX_INPUT_LENGTH];
  char num_s[MAX_INPUT_LENGTH];
  char string[MAX_INPUT_LENGTH];
  int num;

  half_chop(cmd, junk, cmd);   /* "dg_letter" */
  half_chop(cmd, varname, cmd);
  half_chop(cmd, num_s, string);

  num = atoi(num_s);

  script_log("The use of dg_letter is deprecated");
  script_log("- Use 'set <new variable> %%<text/var>.charat(index)%%' instead.");


  if (num < 1) {
    script_log("Trigger #%d : dg_letter number < 1!", GET_TRIG_VNUM(trig));
    return;
  }

  if (num > strlen(string)) {
    script_log("Trigger #%d : dg_letter number > strlen!", GET_TRIG_VNUM(trig));
    return;
  }

  *junk = string[num-1];
  *(junk+1) = '\0';
  add_var(&GET_TRIG_VARS(trig), varname, junk, sc->context);
}

/* This is the core driver for scripts.
 * Arguments:
 * void *go_adress
 *   A pointer to a pointer to the entity running the script. The reason for 
 *   this approcah is that we want to be able to see from the calling function,
 *   if the entity has been free'd.
 *
 * trig_data *trig
 *   A pointer to the current running trigger.
 *
 * int type
 *   MOB_TRIGGER, OBJ_TRIGGER or WLD_TRIGGER, respectively.
 *
 * int mode
     TRIG_NEW     just started from dg_triggers.c
     TRIG_RESTART restarted after a 'wait' */
int script_driver(void *go_adress, trig_data *trig, int type, int mode)
{
  static int depth = 0;
  int ret_val = 1;
  struct cmdlist_element *cl;
  char cmd[MAX_INPUT_LENGTH], *p;
  struct script_data *sc = 0;
  struct cmdlist_element *temp;
  unsigned long loops = 0;
  void *go = NULL;

  void obj_command_interpreter(obj_data *obj, char *argument);
  void wld_command_interpreter(struct room_data *room, char *argument);

  switch (type) {
    case MOB_TRIGGER:
      go = *(char_data **)go_adress;
      sc = SCRIPT((char_data *) go);
      break;
    case OBJ_TRIGGER:
      go = *(obj_data **)go_adress;
      sc = SCRIPT((obj_data *) go);
      break;
    case WLD_TRIGGER:
      go = *(room_data **)go_adress;
      sc = SCRIPT((room_data *) go);
      break;
  }

  if (depth > MAX_SCRIPT_DEPTH) {
    script_log("Trigger %d recursed beyond maximum allowed depth.", GET_TRIG_VNUM(trig));
    switch (type) {
      case MOB_TRIGGER:
        script_log("It was attached to %s [%d]",
           GET_NAME((char_data *) go), GET_MOB_VNUM((char_data *) go));
        break;
      case OBJ_TRIGGER:
        script_log("It was attached to %s [%d]",
           ((obj_data *) go)->short_description, GET_OBJ_VNUM((obj_data *) go));
        break;
      case WLD_TRIGGER:
        script_log("It was attached to %s [%d]",
           ((room_data *) go)->name, ((room_data *) go)->number);
        break;
    }

    extract_script(go, type);

    /* extract_script() works on rooms, but on mobiles and objects, it will be 
     * called again if the caller is load_mtrigger or load_otrigger if it is 
     * one of these, we must make sure the script is not just reloaded on the 
     * next mob. We make the calling code decide how to handle it, so it does
     * not get totally removed unless it's a load_xtrigger(). */
    return SCRIPT_ERROR_CODE;
  }

  depth++;

  if (mode == TRIG_NEW) {
    GET_TRIG_DEPTH(trig) = 1;
    GET_TRIG_LOOPS(trig) = 0;
    sc->context = 0;
  }

  dg_owner_purged = 0;

  for (cl = (mode == TRIG_NEW) ? trig->cmdlist : trig->curr_state;
      cl && GET_TRIG_DEPTH(trig); cl = cl->next) {
    for (p = cl->cmd; *p && isspace(*p); p++);

    if (*p == '*') /* comment */
      continue;

    else if (!strn_cmp(p, "if ", 3)) {
      if (process_if(p + 3, go, sc, trig, type))
        GET_TRIG_DEPTH(trig)++;
      else
        cl = find_else_end(trig, cl, go, sc, type);
    }

    else if (!strn_cmp("elseif ", p, 7) ||
       !strn_cmp("else", p, 4)) {
      /* If not in an if-block, ignore the extra 'else[if]' and warn about it. */
      if (GET_TRIG_DEPTH(trig) == 1) {
        script_log("Trigger VNum %d has 'else' without 'if'.",
                   GET_TRIG_VNUM(trig));
        continue;
      }
      cl = find_end(trig, cl);
      GET_TRIG_DEPTH(trig)--;
    } else if (!strn_cmp("while ", p, 6)) {
      temp = find_done(cl);
      if (!temp) {
        script_log("Trigger VNum %d has 'while' without 'done'.",
                   GET_TRIG_VNUM(trig));
        return ret_val;
      }
      if (process_if(p + 6, go, sc, trig, type)) {
         temp->original = cl;
      } else {
         cl = temp;
         loops = 0;
      }
    } else if (!strn_cmp("switch ", p, 7)) {
      cl = find_case(trig, cl, go, sc, type, p + 7);
    } else if (!strn_cmp("end", p, 3)) {
      /* If not in an if-block, ignore the extra 'end' and warn about it. */
      if (GET_TRIG_DEPTH(trig) == 1) {
        script_log("Trigger VNum %d has 'end' without 'if'.",
                   GET_TRIG_VNUM(trig));
        continue;
      }
      GET_TRIG_DEPTH(trig)--;
    } else if (!strn_cmp("done", p, 4)) {
      /* if in a while loop, cl->original is non-NULL */
      if (cl->original) {
      char *orig_cmd = cl->original->cmd;
      while (*orig_cmd && isspace(*orig_cmd)) orig_cmd++;
      if (cl->original && process_if(orig_cmd + 6, go, sc, trig,
          type)) {
        cl = cl->original;
        loops++;
        GET_TRIG_LOOPS(trig)++;
        if (loops == 30) {
          process_wait(go, trig, type, "wait 1", cl);
           depth--;
          return ret_val;
        }
          if (GET_TRIG_LOOPS(trig) >= 100) {
          script_log("Trigger VNum %d has looped 100 times!!!",
            GET_TRIG_VNUM(trig));
            break;
          }
        } else {
         /* if we're falling through a switch statement, this ends it. */
        }
      }
    } else if (!strn_cmp("break", p, 5)) {
      cl = find_done(cl);
    } else if (!strn_cmp("case", p, 4)) {
       /* Do nothing, this allows multiple cases to a single instance */
    }

    else {
      var_subst(go, sc, trig, type, p, cmd);

      if (!strn_cmp(cmd, "eval ", 5))
        process_eval(go, sc, trig, type, cmd);

      else if (!strn_cmp(cmd, "nop ", 4)); /* nop: do nothing */

      else if (!strn_cmp(cmd, "extract ", 8))
        extract_value(sc, trig, cmd);

      else if (!strn_cmp(cmd, "dg_letter ", 10))
        dg_letter_value(sc, trig, cmd);

      else if (!strn_cmp(cmd, "makeuid ", 8))
        makeuid_var(go, sc, trig, type, cmd);

      else if (!strn_cmp(cmd, "halt", 4))
        break;

      else if (!strn_cmp(cmd, "dg_cast ", 8))
        do_dg_cast(go, sc, trig, type, cmd);

      else if (!strn_cmp(cmd, "dg_affect ", 10))
        do_dg_affect(go, sc, trig, type, cmd);

      else if (!strn_cmp(cmd, "global ", 7))
        process_global(sc, trig, cmd, sc->context);

      else if (!strn_cmp(cmd, "context ", 8))
        process_context(sc, trig, cmd);

      else if (!strn_cmp(cmd, "remote ", 7))
        process_remote(sc, trig, cmd);

      else if (!strn_cmp(cmd, "rdelete ", 8))
        process_rdelete(sc, trig, cmd);

      else if (!strn_cmp(cmd, "return ", 7))
        ret_val = process_return(trig, cmd);

      else if (!strn_cmp(cmd, "set ", 4))
        process_set(sc, trig, cmd);

      else if (!strn_cmp(cmd, "unset ", 6))
        process_unset(sc, trig, cmd);

      else if (!strn_cmp(cmd, "wait ", 5)) {
        process_wait(go, trig, type, cmd, cl);
        depth--;
        return ret_val;
      }

      else if (!strn_cmp(cmd, "attach ", 7))
        process_attach(go, sc, trig, type, cmd);

      else if (!strn_cmp(cmd, "detach ", 7))
        process_detach(go, sc, trig, type, cmd);

      else {
        switch (type) {
          case MOB_TRIGGER:
            if (!script_command_interpreter((char_data *) go, cmd))
            command_interpreter((char_data *) go, cmd);
            break;
          case OBJ_TRIGGER:
            obj_command_interpreter((obj_data *) go, cmd);
            break;
          case WLD_TRIGGER:
            wld_command_interpreter((struct room_data *) go, cmd);
            break;
        }
        if (dg_owner_purged) {
          depth--;
          if (type == OBJ_TRIGGER)
            *(obj_data **)go_adress = NULL;
          return ret_val;
        }
      }
    }
  }

  switch (type) { /* the script may have been detached */
    case MOB_TRIGGER:    sc = SCRIPT((char_data *) go);           break;
    case OBJ_TRIGGER:    sc = SCRIPT((obj_data *) go);            break;
    case WLD_TRIGGER:    sc = SCRIPT((room_data *) go);    break;
  }
  if (sc)
  free_varlist(GET_TRIG_VARS(trig));
  GET_TRIG_VARS(trig) = NULL;
  GET_TRIG_DEPTH(trig) = 0;

  depth--;
  return ret_val;
}

/* returns the real number of the trigger with given virtual number */
trig_rnum real_trigger(trig_vnum vnum)
{
  trig_rnum bot, top, mid;

  bot = 0;
  top = top_of_trigt - 1;

  if (!top_of_trigt || trig_index[bot]->vnum > vnum || trig_index[top]->vnum < vnum)
    return (NOTHING);

  /* perform binary search on trigger-table */
  while (bot <= top) {
    mid = (bot + top) / 2;

    if (trig_index[mid]->vnum == vnum)
      return (mid);
    if (trig_index[mid]->vnum > vnum)
      top = mid - 1;
    else
      bot = mid + 1;
  }
  return (NOTHING);
}

ACMD(do_tstat)
{
  int rnum;
  char str[MAX_INPUT_LENGTH];

  half_chop(argument, str, argument);
  if (*str) {
    rnum = real_trigger(atoi(str));
    if (rnum == NOTHING) {
      send_to_char(ch, "That vnum does not exist.\r\n");
      return;
    }

    do_stat_trigger(ch, trig_index[rnum]->proto);
  } else
    send_to_char(ch, "Usage: tstat <vnum>\r\n");
}

/* Scans for a case/default instance. Returns the line containg the correct 
 * case instance, or the last line of the trigger if not found. */
static struct cmdlist_element *
find_case(struct trig_data *trig, struct cmdlist_element *cl,
          void *go, struct script_data *sc, int type, char *cond)
{
  char result[MAX_INPUT_LENGTH];
  struct cmdlist_element *c;
  char *p, *buf;

  eval_expr(cond, result, go, sc, trig, type);

  if (!(cl->next))
    return cl;

  for (c = cl->next; c->next; c = c->next) {
    for (p = c->cmd; *p && isspace(*p); p++);

    if (!strn_cmp("while ", p, 6) || !strn_cmp("switch", p, 6))
      c = find_done(c);
    else if (!strn_cmp("case ", p, 5)) {
      buf = (char*)malloc(MAX_STRING_LENGTH);
      eval_op("==", result, p + 5, buf, go, sc, trig);
      if (*buf && *buf!='0') {
        free(buf);
        return c;
      }
      free(buf);
    } else if (!strn_cmp("default", p, 7))
      return c;
    else if (!strn_cmp("done", p, 3))
     return c;
  }
  return c;
}

/* Scans for end of while/switch-blocks. Returns the line containg 'end', or 
 * the last line of the trigger if not found. Malformed scripts may cause NULL 
 * to be returned. */
static struct cmdlist_element *find_done(struct cmdlist_element *cl)
{
  struct cmdlist_element *c;
  char *p;

  if (!cl || !(cl->next))
    return cl;

  for (c = cl->next; c && c->next; c = c->next) {
    for (p = c->cmd; *p && isspace(*p); p++);

    if (!strn_cmp("while ", p, 6) || !strn_cmp("switch ", p, 7))
      c = find_done(c);
    else if (!strn_cmp("done", p, 3))
      return c;
  }

  return c;
}


/* load in a character's saved variables */
void read_saved_vars(struct char_data *ch)
{
  FILE *file;
  long context;
  char fn[127];
  char input_line[1024], *temp, *p;
  char varname[32];
  char context_str[16];

  /* If getting to the menu from inside the game, the vars aren't removed. So 
   * let's not allocate them again. */
  if (SCRIPT(ch))
    return;

  /* Create the space for the script structure which holds the vars. We need to
   * do this first, because later calls to 'remote' will need. A script already 
   * assigned. */
  CREATE(SCRIPT(ch), struct script_data, 1);

  /* find the file that holds the saved variables and open it*/
  get_filename(fn, sizeof(fn), SCRIPT_VARS_FILE, GET_NAME(ch));
  file = fopen(fn,"r");

  /* if we failed to open the file, return */
  if( !file ) {
    log("%s had no variable file", GET_NAME(ch));
    return;
  }
  /* walk through each line in the file parsing variables */
  do {
    if (get_line(file, input_line)>0) {
      p = temp = strdup(input_line);
      temp = any_one_arg(temp, varname);
      temp = any_one_arg(temp, context_str);
      skip_spaces(&temp); /* temp now points to the rest of the line */

      context = atol(context_str);
      add_var(&(SCRIPT(ch)->global_vars), varname, temp, context);
      free(p); /* plug memory hole */
    }
  } while( !feof(file) );

  /* close the file and return */
  fclose(file);
}

/* save a characters variables out to disk */
void save_char_vars(struct char_data *ch)
{
  FILE *file;
  char fn[127];
  struct trig_var_data *vars;

  /* Immediate return if no script (and therefore no variables) structure has 
   * been created. this will happen when the player is logging in */
  if (SCRIPT(ch) == NULL) return;

  /* we should never be called for an NPC, but just in case... */
  if (IS_NPC(ch)) return;

  get_filename(fn, sizeof(fn), SCRIPT_VARS_FILE, GET_NAME(ch));
  unlink(fn);

  /* make sure this char has global variables to save */
  if (ch->script->global_vars == NULL) return;
  vars = ch->script->global_vars;

  file = fopen(fn,"wt");
  if (!file) {
    mudlog( NRM, LVL_GOD, TRUE,
            "SYSERR: Could not open player variable file %s for writing.:%s",
            fn, strerror(errno));
    return;
  }
  /* Note that currently, context will always be zero. This may change in the 
   * future. */
  while (vars) {
    if (*vars->name != '-') /* don't save if it begins with - */
      fprintf(file, "%s %ld %s\n", vars->name, vars->context, vars->value);
    vars = vars->next;
  }

  fclose(file);
}

/* load in a character's saved variables from an ASCII pfile*/
void read_saved_vars_ascii(FILE *file, struct char_data *ch, int count)
{
  long context;
  char input_line[1024], *temp, *p;
  char varname[READ_SIZE];
  char context_str[READ_SIZE];
  int i;

  /* If getting to the menu from inside the game, the vars aren't removed. So 
   * let's not allocate them again. */
  if (SCRIPT(ch))
    return;

  /* Create the space for the script structure which holds the vars. We need to
   * do this first, because later calls to 'remote' will need. A script already
   * assigned. */
  CREATE(SCRIPT(ch), struct script_data, 1);

  /* walk through each line in the file parsing variables */
  for (i = 0; i < count; i++)
  {
    if (get_line(file, input_line)>0) {
      p = temp = strdup(input_line);
      temp = any_one_arg(temp, varname);
      temp = any_one_arg(temp, context_str);
      skip_spaces(&temp); /* temp now points to the rest of the line */

      context = atol(context_str);
      add_var(&(SCRIPT(ch)->global_vars), varname, temp, context);
      free(p); /* plug memory hole */
    }
  }
}

/* save a characters variables out to an ASCII pfile */
void save_char_vars_ascii(FILE *file, struct char_data *ch)
{
  struct trig_var_data *vars;
  int count = 0;
  /* Immediate return if no script (and therefore no variables) structure has 
   * been created. this will happen when the player is logging in */
  if (SCRIPT(ch) == NULL) return;

  /* we should never be called for an NPC, but just in case... */
  if (IS_NPC(ch)) return;

  /* make sure this char has global variables to save */
  if (ch->script->global_vars == NULL) return;

  /* Note that currently, context will always be zero. This may change in the 
   * future */
  for (vars = ch->script->global_vars;vars;vars = vars->next)
    if (*vars->name != '-')
      count++;

  if (count != 0) {
	  fprintf(file, "Vars: %d\n", count);

  for (vars = ch->script->global_vars;vars;vars = vars->next)
    if (*vars->name != '-') /* don't save if it begins with - */
      fprintf(file, "%s %ld %s\n", vars->name, vars->context, vars->value);
  }
}

/* find_char() helpers */
/* Must be power of 2. */
#define BUCKET_COUNT 64
/* To recognize an empty bucket. */
#define UID_OUT_OF_RANGE 1000000000

struct lookup_table_t {
  long uid;
  void * c;
  struct lookup_table_t *next;
};
struct lookup_table_t lookup_table[BUCKET_COUNT];

void init_lookup_table(void)
{
  int i;
  for (i = 0; i < BUCKET_COUNT; i++) {
    lookup_table[i].uid  = UID_OUT_OF_RANGE;
    lookup_table[i].c    = NULL;
    lookup_table[i].next = NULL;
  }
}

static struct char_data *find_char_by_uid_in_lookup_table(long uid)
{
  int bucket = (int) (uid & (BUCKET_COUNT - 1));
  struct lookup_table_t *lt = &lookup_table[bucket];

  for (;lt && lt->uid != uid ; lt = lt->next) ;

  if (lt)
    return (struct char_data *)(lt->c);

  log("find_char_by_uid_in_lookup_table : No entity with number %ld in lookup table", uid);
  return NULL;
}

static struct obj_data *find_obj_by_uid_in_lookup_table(long uid)
{
  int bucket = (int) (uid & (BUCKET_COUNT - 1));
  struct lookup_table_t *lt = &lookup_table[bucket];

  for (;lt && lt->uid != uid ; lt = lt->next) ;

  if (lt)
    return (struct obj_data *)(lt->c);

  log("find_obj_by_uid_in_lookup_table : No entity with number %ld in lookup table", uid);
  return NULL;
}

void add_to_lookup_table(long uid, void *c)
{
  int bucket = (int) (uid & (BUCKET_COUNT - 1));
  struct lookup_table_t *lt = &lookup_table[bucket];

  for (;lt->next; lt = lt->next)
  if (lt->c == c && lt->uid == uid) {
      log ("Add_to_lookup failed. Already there. (uid = %ld)", uid);
      return;
    }

  CREATE(lt->next, struct lookup_table_t, 1);
  lt->next->uid = uid;
  lt->next->c = c;
}

void remove_from_lookup_table(long uid)
{
  int bucket = (int) (uid & (BUCKET_COUNT - 1));
  struct lookup_table_t *lt = &lookup_table[bucket], *flt = NULL;

  /* This is not supposed to happen. UID 0 is not used. However, while I'm 
   * debugging the issue, let's just return right away. - Welcor */
  if (uid == 0)
    return;

  for (;lt;lt = lt->next)
    if (lt->uid == uid)
      flt = lt;

  if (flt) {
    for (lt = &lookup_table[bucket];lt->next != flt;lt = lt->next)
      ;
    lt->next = flt->next;
    free(flt);
    return;
  }

  log("remove_from_lookup. UID %ld not found.", uid);
}

bool check_flags_by_name_ar(int *array, int numflags, char *search, const char *namelist[]) 
{ 
  int i, item=-1; 

  for (i=0; i<numflags && item < 0; i++) 
    if (!strcmp(search, namelist[i])) item = i; 

  if (item < 0) return FALSE; 

  if (IS_SET_AR(array, item)) return TRUE; 

  return FALSE; 
}

int trig_is_attached(struct script_data *sc, int trig_num) 
{ 
  trig_data *t; 

  if (!sc || !TRIGGERS(sc)) return 0; 

  for (t = TRIGGERS(sc); t; t = t->next) 
    if (GET_TRIG_VNUM(t) == trig_num) 
      return 1; 

  return 0; 
}
