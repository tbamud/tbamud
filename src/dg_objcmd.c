/**************************************************************************
*  File: dg_objcmd.c                                       Part of tbaMUD *
*  Usage: Contains the command_interpreter for objects, object commands.  *
*                                                                         *
*  $Author: galion/Mark A. Heilpern/egreen/Welcor $                       *
*  $Date: 2004/10/11 12:07:00$                                            *
*  $Revision: 1.0.14 $                                                    *
**************************************************************************/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "screen.h"
#include "dg_scripts.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "constants.h"
#include "genzon.h" /* for access to real_zone_by_thing */
#include "fight.h" /* for die() */



/* Local functions */
#define OCMD(name)  \
   void (name)(obj_data *obj, char *argument, int cmd, int subcmd)

static void obj_log(obj_data *obj, const char *format, ...);
static room_rnum find_obj_target_room(obj_data *obj, char *rawroomstr);
static OCMD(do_oecho);
static OCMD(do_oforce);
static OCMD(do_ozoneecho);
static OCMD(do_osend);
static OCMD(do_orecho);
static OCMD(do_otimer);
static OCMD(do_otransform);
static OCMD(do_opurge);
static OCMD(do_oteleport);
static OCMD(do_dgoload);
static OCMD(do_odamage);
static OCMD(do_oasound);
static OCMD(do_odoor);
static OCMD(do_osetval);
static OCMD(do_oat);
static OCMD(do_omove);

struct obj_command_info {
   char *command;
   void        (*command_pointer)(obj_data *obj, char *argument, int cmd, int subcmd);
   int        subcmd;
};

/* do_osend */
#define SCMD_OSEND         0
#define SCMD_OECHOAROUND   1

/* attaches object name and vnum to msg and sends it to script_log */
static void obj_log(obj_data *obj, const char *format, ...)
{
  va_list args;
  char output[MAX_STRING_LENGTH];

  snprintf(output, sizeof(output), "Obj (%s, VNum %d):: %s", obj->short_description, GET_OBJ_VNUM(obj), format);

  va_start(args, format);
  script_vlog(output, args);
  va_end(args);
}

/* returns the real room number that the object or object's carrier is in */
room_rnum obj_room(obj_data *obj)
{
    if (IN_ROOM(obj) != NOWHERE)
        return IN_ROOM(obj);
    else if (obj->carried_by)
        return IN_ROOM(obj->carried_by);
    else if (obj->worn_by)
        return IN_ROOM(obj->worn_by);
    else if (obj->in_obj)
        return obj_room(obj->in_obj);
    else
        return NOWHERE;
}

/* returns the real room number, or NOWHERE if not found or invalid */
static room_rnum find_obj_target_room(obj_data *obj, char *rawroomstr)
{
    int tmp;
    room_rnum location;
    char_data *target_mob;
    obj_data *target_obj;
    char roomstr[MAX_INPUT_LENGTH];

    one_argument(rawroomstr, roomstr);

    if (!*roomstr)
        return NOWHERE;

    if (isdigit(*roomstr) && !strchr(roomstr, '.'))
    {
        tmp = atoi(roomstr);
        if ((location = real_room(tmp)) == NOWHERE)
            return NOWHERE;
    }

    else if ((target_mob = get_char_by_obj(obj, roomstr)))
        location = IN_ROOM(target_mob);
    else if ((target_obj = get_obj_by_obj(obj, roomstr)))
    {
        if (IN_ROOM(target_obj) != NOWHERE)
            location = IN_ROOM(target_obj);
        else
            return NOWHERE;
    }
    else
        return NOWHERE;

    /* a room has been found.  Check for permission */
    if (ROOM_FLAGGED(location, ROOM_GODROOM) ||
#ifdef ROOM_IMPROOM
        ROOM_FLAGGED(location, ROOM_IMPROOM) ||
#endif
        ROOM_FLAGGED(location, ROOM_HOUSE))
        return NOWHERE;

    if (ROOM_FLAGGED(location, ROOM_PRIVATE) &&
        world[location].people && world[location].people->next_in_room)
        return NOWHERE;

    return location;
}

/* Object commands */
static OCMD(do_oecho)
{
    int room;

    skip_spaces(&argument);

    if (!*argument)
        obj_log(obj, "oecho called with no args");

    else if ((room = obj_room(obj)) != NOWHERE)
    {
      if (world[room].people) { 
        sub_write(argument, world[room].people, TRUE, TO_ROOM); 
        sub_write(argument, world[room].people, TRUE, TO_CHAR); 
      } 
    }

    else
        obj_log(obj, "oecho called by object in NOWHERE");
}

static OCMD(do_oforce)
{
    char_data *ch, *next_ch;
    int room;
    char arg1[MAX_INPUT_LENGTH], *line;

    line = one_argument(argument, arg1);

    if (!*arg1 || !*line)
    {
        obj_log(obj, "oforce called with too few args");
        return;
    }

    if (!str_cmp(arg1, "all"))
    {
        if ((room = obj_room(obj)) == NOWHERE)
            obj_log(obj, "oforce called by object in NOWHERE");
        else
        {
            for (ch = world[room].people; ch; ch = next_ch)
            {
                next_ch = ch->next_in_room;
                if (valid_dg_target(ch, 0))
                {
                    command_interpreter(ch, line);
                }
            }
        }
    }

    else
    {
        if ((ch = get_char_by_obj(obj, arg1)))
        {
            if (valid_dg_target(ch, 0))
            {
                command_interpreter(ch, line);
            }
        }

        else
            obj_log(obj, "oforce: no target found");
    }
}

static OCMD(do_ozoneecho)
{
    int zone;
    char room_number[MAX_INPUT_LENGTH], buf[MAX_INPUT_LENGTH], *msg;

    msg = any_one_arg(argument, room_number);
    skip_spaces(&msg);

    if (!*room_number || !*msg)
	obj_log(obj, "ozoneecho called with too few args");

    else if ((zone = real_zone_by_thing(atoi(room_number))) == NOWHERE)
	obj_log(obj, "ozoneecho called for nonexistant zone");

    else {
	sprintf(buf, "%s\r\n", msg);
	send_to_zone(buf, zone);
    }
}

static OCMD(do_osend)
{
    char buf[MAX_INPUT_LENGTH], *msg;
    char_data *ch;

    msg = any_one_arg(argument, buf);

    if (!*buf)
    {
        obj_log(obj, "osend called with no args");
        return;
    }

    skip_spaces(&msg);

    if (!*msg)
    {
        obj_log(obj, "osend called without a message");
        return;
    }

    if ((ch = get_char_by_obj(obj, buf)))
    {
        if (subcmd == SCMD_OSEND)
            sub_write(msg, ch, TRUE, TO_CHAR);
        else if (subcmd == SCMD_OECHOAROUND)
            sub_write(msg, ch, TRUE, TO_ROOM);
    }

    else
        obj_log(obj, "no target found for osend");
}

/* Prints the message to everyone in the range of numbers. Thanks to Jamie 
 * Nelson of 4D for this contribution. */
static OCMD(do_orecho)
{
    char start[MAX_INPUT_LENGTH], finish[MAX_INPUT_LENGTH], *msg;

    msg = two_arguments(argument, start, finish);

    skip_spaces(&msg);

    if (!*msg || !*start || !*finish || !is_number(start) || !is_number(finish))
      obj_log(obj, "orecho: too few args");
    else
      send_to_range(atoi(start), atoi(finish), "%s\r\n", msg);

}

/* set the object's timer value */
static OCMD(do_otimer)
{
  char arg[MAX_INPUT_LENGTH];

  one_argument(argument, arg);

  if (!*arg)
    obj_log(obj, "otimer: missing argument");
  else if (!isdigit(*arg))
    obj_log(obj, "otimer: bad argument");
  else
    GET_OBJ_TIMER(obj) = atoi(arg);
}

/* Transform into a different object. Note: this shouldn't be used with 
 * containers unless both objects are containers! */
static OCMD(do_otransform)
{
  char arg[MAX_INPUT_LENGTH];
  obj_data *o, tmpobj;
  struct char_data *wearer=NULL;
  int pos = 0;

  one_argument(argument, arg);

  if (!*arg)
    obj_log(obj, "otransform: missing argument");
  else if (!isdigit(*arg))
    obj_log(obj, "otransform: bad argument");
  else {
    o = read_object(atoi(arg), VIRTUAL);
    if (o==NULL) {
      obj_log(obj, "otransform: bad object vnum");
      return;
    }

    if (obj->worn_by) {
      pos = obj->worn_on;
      wearer = obj->worn_by;
      unequip_char(obj->worn_by, pos);
    }

    /* move new obj info over to old object and delete new obj */
    memcpy(&tmpobj, o, sizeof(*o));
    tmpobj.in_room = IN_ROOM(obj);
    tmpobj.carried_by = obj->carried_by;
    tmpobj.worn_by = obj->worn_by;
    tmpobj.worn_on = obj->worn_on;
    tmpobj.in_obj = obj->in_obj;
    tmpobj.contains = obj->contains;
    tmpobj.id = obj->id;
    tmpobj.proto_script = obj->proto_script;
    tmpobj.script = obj->script;
    tmpobj.next_content = obj->next_content;
    tmpobj.next = obj->next;
    memcpy(obj, &tmpobj, sizeof(*obj));

    if (wearer) {
      equip_char(wearer, obj, pos);
    }

    extract_obj(o);
  }
}

/* purge all objects an npcs in room, or specified object or mob */
static OCMD(do_opurge)
{
    char arg[MAX_INPUT_LENGTH];
    char_data *ch, *next_ch;
    obj_data *o, *next_obj;
    int rm;

    one_argument(argument, arg);

    if (!*arg) {
      /* purge all */
      if ((rm = obj_room(obj)) != NOWHERE) {
        for (ch = world[rm].people; ch; ch = next_ch ) {
           next_ch = ch->next_in_room;
           if (IS_NPC(ch))
             extract_char(ch);
        }

        for (o = world[rm].contents; o; o = next_obj ) {
           next_obj = o->next_content;
           if (o != obj)
             extract_obj(o);
        }
      }

      return;
    } /* no arg */

    ch = get_char_by_obj(obj, arg);
    if (!ch) {
      o = get_obj_by_obj(obj, arg);
      if (o) {
        if (o==obj)
          dg_owner_purged = 1;
        extract_obj(o);
      } else
        obj_log(obj, "opurge: bad argument");

      return;
    }

    if (!IS_NPC(ch)) {
      obj_log(obj, "opurge: purging a PC");
      return;
    }

    extract_char(ch);
}

static OCMD(do_oteleport)
{
    char_data *ch, *next_ch;
    room_rnum target, rm;
    char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

    two_arguments(argument, arg1, arg2);

    if (!*arg1 || !*arg2)
    {
        obj_log(obj, "oteleport called with too few args");
        return;
    }

    target = find_obj_target_room(obj, arg2);

    if (target == NOWHERE)
        obj_log(obj, "oteleport target is an invalid room");

    else if (!str_cmp(arg1, "all"))
    {
        rm = obj_room(obj);
        if (target == rm)
            obj_log(obj, "oteleport target is itself");

        for (ch = world[rm].people; ch; ch = next_ch)
        {
            next_ch = ch->next_in_room;
            if (!valid_dg_target(ch, DG_ALLOW_GODS))
              continue;
            char_from_room(ch);
            char_to_room(ch, target);
            enter_wtrigger(&world[IN_ROOM(ch)], ch, -1);
        }
    }

    else
    {
        if ((ch = get_char_by_obj(obj, arg1))) {
          if (valid_dg_target(ch, DG_ALLOW_GODS)) {
            char_from_room(ch);
            char_to_room(ch, target);
            enter_wtrigger(&world[IN_ROOM(ch)], ch, -1);
          }
        }

        else
            obj_log(obj, "oteleport: no target found");
    }
}

static OCMD(do_dgoload)
{
    char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
    int number = 0, room;
    char_data *mob;
    obj_data *object;
    char *target;
    char_data *tch;
    obj_data *cnt;
    int pos;

    target = two_arguments(argument, arg1, arg2);

    if (!*arg1 || !*arg2 || !is_number(arg2) || ((number = atoi(arg2)) < 0))
    {
        obj_log(obj, "oload: bad syntax");
        return;
    }

    if ((room = obj_room(obj)) == NOWHERE)
    {
        obj_log(obj, "oload: object in NOWHERE trying to load");
        return;
    }

    /* load mob to target room - Jamie Nelson, April 13 2004 */
    if (is_abbrev(arg1, "mob")) {
      room_rnum rnum;
      if (!target || !*target) {
        rnum = room;
      } else {
        if (!isdigit(*target) || (rnum = real_room(atoi(target))) == NOWHERE) {
          obj_log(obj, "oload: room target vnum doesn't exist "
                       "(loading mob vnum %d to room %s)", number, target);
          return;
        }
      }
      if ((mob = read_mobile(number, VIRTUAL)) == NULL) {
        obj_log(obj, "oload: bad mob vnum");
        return;
      }
      char_to_room(mob, rnum);

      if (SCRIPT(obj)) { /* It _should_ have, but it might be detached. */
        char buf[MAX_INPUT_LENGTH];
        sprintf(buf, "%c%ld", UID_CHAR, GET_ID(mob));
        add_var(&(SCRIPT(obj)->global_vars), "lastloaded", buf, 0);
      }

      load_mtrigger(mob);
    }

    else if (is_abbrev(arg1, "obj")) {
      if ((object = read_object(number, VIRTUAL)) == NULL) {
        obj_log(obj, "oload: bad object vnum");
        return;
      }

      if (SCRIPT(obj)) { /* It _should_ have, but it might be detached. */
        char buf[MAX_INPUT_LENGTH];
        sprintf(buf, "%c%ld", UID_CHAR, GET_ID(object));
        add_var(&(SCRIPT(obj)->global_vars), "lastloaded", buf, 0);
      }

      /* special handling to make objects able to load on a person/in a container/worn etc. */
      if (!target || !*target) {
        obj_to_room(object, room);
        load_otrigger(object);
        return;
      }
      two_arguments(target, arg1, arg2); /* recycling ... */
      tch = get_char_near_obj(obj, arg1);
      if (tch) {
        if (arg2 != NULL && *arg2 && (pos = find_eq_pos_script(arg2)) >= 0 &&
            !GET_EQ(tch, pos) && can_wear_on_pos(object, pos)) {
          equip_char(tch, object, pos);
          load_otrigger(object);
          return;
        }
        obj_to_char(object, tch);
        load_otrigger(object);
        return;
      }
      cnt = get_obj_near_obj(obj, arg1);
      if (cnt && GET_OBJ_TYPE(cnt) == ITEM_CONTAINER) {
      	obj_to_obj(object, cnt);
        load_otrigger(object);
      	return;
      }
      /* neither char nor container found - just dump it in room */
      obj_to_room(object, room);
      load_otrigger(object);
      return;
    }

    else
        obj_log(obj, "oload: bad type");

}

static OCMD(do_odamage) {
  char name[MAX_INPUT_LENGTH], amount[MAX_INPUT_LENGTH];
  int dam = 0;
  char_data *ch;

  two_arguments(argument, name, amount);

  /* who cares if it's a number ? if not it'll just be 0 */
  if (!*name || !*amount) {
      obj_log(obj, "odamage: bad syntax");
      return;
  }

  dam = atoi(amount);
  ch = get_char_by_obj(obj, name);

  if (!ch) {
    obj_log(obj, "odamage: target not found");
    return;
  }
  script_damage(ch, dam);
}

static OCMD(do_oasound)
{
  room_rnum room;
  int door;

  skip_spaces(&argument);

  if (!*argument) {
    obj_log(obj, "oasound called with no args");
    return;
  }

  if ((room = obj_room(obj)) == NOWHERE) {
    obj_log(obj, "oecho called by object in NOWHERE");
    return;
  }

  for (door = 0; door < NUM_OF_DIRS; door++) {
    if (world[room].dir_option[door] != NULL &&
       (world[room].dir_option[door])->to_room != NOWHERE &&
       (world[room].dir_option[door])->to_room != room &&
        world[(world[room].dir_option[door])->to_room].people) { 
          sub_write(argument, world[(world[room].dir_option[door])->to_room].people, TRUE, TO_ROOM); 
          sub_write(argument, world[(world[room].dir_option[door])->to_room].people, TRUE, TO_CHAR); 
    }
  }
}

static OCMD(do_odoor)
{
    char target[MAX_INPUT_LENGTH], direction[MAX_INPUT_LENGTH];
    char field[MAX_INPUT_LENGTH], *value;
    room_data *rm;
    struct room_direction_data *newexit;
    int dir, fd, to_room;

    const char *door_field[] = {
        "purge",
        "description",
        "flags",
        "key",
        "name",
        "room",
        "\n"
    };

    argument = two_arguments(argument, target, direction);
    value = one_argument(argument, field);
    skip_spaces(&value);

    if (!*target || !*direction || !*field) {
        obj_log(obj, "odoor called with too few args");
        return;
    }

    if ((rm = get_room(target)) == NULL) {
        obj_log(obj, "odoor: invalid target");
        return;
    }

    if ((dir = search_block(direction, dirs, FALSE)) == -1) {
        obj_log(obj, "odoor: invalid direction");
        return;
    }

    if ((fd = search_block(field, door_field, FALSE)) == -1) {
        obj_log(obj, "odoor: invalid field");
        return;
    }

    newexit = rm->dir_option[dir];

    /* purge exit */
    if (fd == 0) {
        if (newexit) {
            if (newexit->general_description)
                free(newexit->general_description);
            if (newexit->keyword)
                free(newexit->keyword);
            free(newexit);
            rm->dir_option[dir] = NULL;
        }
    }

    else {
        if (!newexit) {
            CREATE(newexit, struct room_direction_data, 1);
            rm->dir_option[dir] = newexit;
        }

        switch (fd) {
        case 1:  /* description */
            if (newexit->general_description)
                free(newexit->general_description);
            CREATE(newexit->general_description, char, strlen(value) + 3);
            strcpy(newexit->general_description, value);
            strcat(newexit->general_description, "\r\n"); /* strcat : OK */
            break;
        case 2:  /* flags       */
            newexit->exit_info = (sh_int)asciiflag_conv(value);
            break;
        case 3:  /* key         */
            newexit->key = atoi(value);
            break;
        case 4:  /* name        */
            if (newexit->keyword)
                free(newexit->keyword);
            CREATE(newexit->keyword, char, strlen(value) + 1);
            strcpy(newexit->keyword, value);
            break;
        case 5:  /* room        */
            if ((to_room = real_room(atoi(value))) != NOWHERE)
                newexit->to_room = to_room;
            else
                obj_log(obj, "odoor: invalid door target");
            break;
        }
    }
}

static OCMD(do_osetval)
{
  char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
  int position, new_value, worn_on;
  struct char_data *worn_by = NULL;

  two_arguments(argument, arg1, arg2);
  if (arg1 == NULL || !*arg1 || arg2 == NULL || !*arg2 || !is_number(arg1) || !is_number(arg2)) {
    obj_log(obj, "osetval: bad syntax");
    return;
  }

  position = atoi(arg1);
  new_value = atoi(arg2);

  if (position>=0 && position<NUM_OBJ_VAL_POSITIONS) {
    worn_by = obj->worn_by;
    worn_on = obj->worn_on;

    if (worn_by != NULL) {
      unequip_char(worn_by, worn_on);
    }

    GET_OBJ_VAL(obj, position) = new_value;

    if (worn_by != NULL) {
      equip_char(worn_by, obj, worn_on);
    }
  } else
    obj_log(obj, "osetval: position out of bounds!");
}

/* Submitted by PurpleOnyx */
static OCMD(do_oat)
{
  room_rnum loc = NOWHERE;
  struct char_data *ch;
  struct obj_data *object;
  char arg[MAX_INPUT_LENGTH], *command;

  command = any_one_arg(argument, arg);

  if (!*arg) {
    obj_log(obj, "oat called with no args");
    return;
  }

  skip_spaces(&command);

  if (!*command) {
    obj_log(obj, "oat called without a command");
    return;
  }

  if (isdigit(*arg)) loc = real_room(atoi(arg));
  else if ((ch = get_char_by_obj(obj, arg))) loc = IN_ROOM(ch); 

  if (loc == NOWHERE) {
    obj_log(obj, "oat: location not found (%s)", arg);
    return;
  }

  if (!(object = read_object(GET_OBJ_VNUM(obj), VIRTUAL)))
    return;

  obj_to_room(object, loc);
  obj_command_interpreter(object, command);

  if (object->in_room == loc) 
    extract_obj(object);
}

static OCMD(do_omove)
{
    room_rnum target;
    char arg1[MAX_INPUT_LENGTH];

    one_argument(argument, arg1);

    if (!*arg1)
    {
        obj_log(obj, "omove called with too few args");
        return;
    }

    target = find_obj_target_room(obj, arg1);

    if (target == NOWHERE)
        obj_log(obj, "omove target is an invalid room");

    // Remove the object from it's current location
    if (obj->carried_by != NULL) {
      obj_from_char(obj);
    } else if (IN_ROOM(obj) != NOWHERE) {
      obj_from_room(obj);
    } else if (obj->in_obj != NULL) {
      obj_from_obj(obj);
    } else {
      obj_log(obj, "omove: target object is not in a room, held or in a container!");
      return;
    }

    obj_to_room(obj, target);
}

const struct obj_command_info obj_cmd_info[] = {
    { "RESERVED", 0, 0 },/* this must be first -- for specprocs */

    { "oasound "    , do_oasound  , 0 },
    { "oat "        , do_oat      , 0 },
    { "odoor "      , do_odoor    , 0 },
    { "odamage "    , do_odamage,   0 },
    { "oecho "      , do_oecho    , 0 },
    { "oechoaround ", do_osend    , SCMD_OECHOAROUND },
    { "oforce "     , do_oforce   , 0 },
    { "oload "      , do_dgoload  , 0 },
    { "opurge "     , do_opurge   , 0 },
    { "orecho "     , do_orecho   , 0 },
    { "osend "      , do_osend    , SCMD_OSEND },
    { "osetval "    , do_osetval  , 0 },
    { "oteleport "  , do_oteleport, 0 },
    { "otimer "     , do_otimer   , 0 },
    { "otransform " , do_otransform, 0 },
    { "ozoneecho "  , do_ozoneecho , 0 }, /* fix by Rumble */
    { "omove "      , do_omove     , 0 },
    { "\n", 0, 0 }        /* this must be last */
};

/* This is the command interpreter used by objects, called by script_driver. */
void obj_command_interpreter(obj_data *obj, char *argument)
{
    int cmd, length;
    char *line, arg[MAX_INPUT_LENGTH];

    skip_spaces(&argument);

    /* just drop to next line for hitting CR */
    if (!*argument)
        return;

    line = any_one_arg(argument, arg);


    /* find the command */
    for (length = strlen(arg),cmd = 0;
         *obj_cmd_info[cmd].command != '\n'; cmd++)
        if (!strncmp(obj_cmd_info[cmd].command, arg, length))
            break;

    if (*obj_cmd_info[cmd].command == '\n')
      obj_log(obj, "Unknown object cmd: '%s'", argument);
    else
        ((*obj_cmd_info[cmd].command_pointer)
         (obj, line, cmd, obj_cmd_info[cmd].subcmd));
}
