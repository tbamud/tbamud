/**************************************************************************
*  File: dg_handler.c                                      Part of tbaMUD *
*  Usage: Contains functions to handle memory for scripts.                *
*                                                                         *
*  All rights reserved.  See license for complete information.            *
*                                                                         *
*  Death's Gate MUD is based on CircleMUD, Copyright (C) 1993, 94.        *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
*                                                                         *
*  $Author: Mark A. Heilpern/egreen/Welcor $                              *
*  $Date: 2004/10/11 12:07:00$                                            *
*  $Revision: 1.0.14 $                                                    *
***************************************************************************/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "dg_scripts.h"
#include "comm.h"
#include "db.h"
#include "handler.h"
#include "spells.h"
#include "dg_event.h"
#include "constants.h"

/* frees memory associated with var */
void free_var_el(struct trig_var_data *var)
{
  if (var->name)
    free(var->name);
  if (var->value)
    free(var->value);
  free(var);
}

/* release memory allocated for a variable list */
void free_varlist(struct trig_var_data *vd)
{
    struct trig_var_data *i, *j;

    for (i = vd; i;) {
	j = i;
	i = i->next;
	free_var_el(j);
    }
}

/* Remove var name from var_list. Returns 1 if found, else 0. */
int remove_var(struct trig_var_data **var_list, char *name)
{
  struct trig_var_data *i, *j;

  for (j = NULL, i = *var_list; i && str_cmp(name, i->name);
       j = i, i = i->next);

  if (i) {
    if (j) {
      j->next = i->next;
      free_var_el(i);
    } else {
      *var_list = i->next;
      free_var_el(i);
    }

    return 1;
  }

  return 0;
}

/* Return memory used by a trigger. The command list is free'd when changed and
 * when shutting down. */
void free_trigger(struct trig_data *trig)
{
    free(trig->name);
    trig->name = NULL;

    if (trig->arglist) {
      free(trig->arglist);
      trig->arglist = NULL;
    }
    if (trig->var_list) {
      free_varlist(trig->var_list);
      trig->var_list = NULL;
    }
    if (GET_TRIG_WAIT(trig))
      event_cancel(GET_TRIG_WAIT(trig));

    free(trig);
}

/* remove a single trigger from a mob/obj/room */
void extract_trigger(struct trig_data *trig)
{
  struct trig_data *temp;

  if (GET_TRIG_WAIT(trig)) {
    event_cancel(GET_TRIG_WAIT(trig));
    GET_TRIG_WAIT(trig) = NULL;
  }

  trig_index[trig->nr]->number--;

  /* walk the trigger list and remove this one */
  REMOVE_FROM_LIST(trig, trigger_list, next_in_world);

  free_trigger(trig);
}

/* remove all triggers from a mob/obj/room */
void extract_script(void *thing, int type)
{
  struct script_data *sc = NULL;
  struct trig_data *trig, *next_trig;
  char_data *mob;
  obj_data *obj;
  room_data *room;

  switch (type) {
    case MOB_TRIGGER:
      mob = (struct char_data *)thing;
      sc = SCRIPT(mob);
      SCRIPT(mob) = NULL;
      break;
    case OBJ_TRIGGER:
      obj = (struct obj_data *)thing;
      sc = SCRIPT(obj);
      SCRIPT(obj) = NULL;
      break;
    case WLD_TRIGGER:
      room = (struct room_data *)thing;
      sc = SCRIPT(room);
      SCRIPT(room) = NULL;
      break;
  }

#if 1 /* debugging */
  {
    struct char_data *i = character_list;
    struct obj_data *j = object_list;
    room_rnum k;
    if (sc) {
      for ( ; i ; i = i->next)
        assert(sc != SCRIPT(i));

      for ( ; j ; j = j->next)
        assert(sc != SCRIPT(j));

      for (k = 0; k < top_of_world; k++)
        assert(sc != SCRIPT(&world[k]));
    }
  }
#endif
  for (trig = TRIGGERS(sc); trig; trig = next_trig) {
    next_trig = trig->next;
    extract_trigger(trig);
  }
  TRIGGERS(sc) = NULL;

  /* Thanks to James Long for tracking down this memory leak */
  free_varlist(sc->global_vars);

  free(sc);
}

/* erase the script memory of a mob */
void extract_script_mem(struct script_memory *sc)
{
  struct script_memory *next;
  while (sc) {
    next = sc->next;
    if (sc->cmd) free(sc->cmd);
    free(sc);
    sc = next;
  }
}

void free_proto_script(void *thing, int type)
{
  struct trig_proto_list *proto = NULL, *fproto;
  char_data *mob;
  obj_data *obj;
  room_data *room;

  switch (type) {
    case MOB_TRIGGER:
      mob = (struct char_data *)thing;
      proto = mob->proto_script;
      mob->proto_script = NULL;
      break;
    case OBJ_TRIGGER:
      obj = (struct obj_data *)thing;
      proto = obj->proto_script;
      obj->proto_script = NULL;
      break;
    case WLD_TRIGGER:
      room = (struct room_data *)thing;
      proto = room->proto_script;
      room->proto_script = NULL;
      break;
  }
#if 1 /* debugging */
  {
    struct char_data *i = character_list;
    struct obj_data *j = object_list;
    room_rnum k;
    if (proto) {
      for ( ; i ; i = i->next)
        assert(proto != i->proto_script);

      for ( ; j ; j = j->next)
        assert(proto != j->proto_script);

      for (k = 0; k < top_of_world; k++)
        assert(proto != world[k].proto_script);
    }
  }
#endif
  while (proto) {
    fproto = proto;
    proto = proto->next;
    free(fproto);
  }
}

void copy_proto_script(void *source, void *dest, int type)
{
  struct trig_proto_list *tp_src = NULL, *tp_dst = NULL;

  switch (type) {
    case MOB_TRIGGER:
      tp_src = ((char_data *)source)->proto_script;
      break;
    case OBJ_TRIGGER:
      tp_src = ((obj_data *)source)->proto_script;
      break;
    case WLD_TRIGGER:
      tp_src = ((room_data *)source)->proto_script;
      break;
  }

  if (tp_src) {
    CREATE(tp_dst, struct trig_proto_list, 1);
    switch (type) {
      case MOB_TRIGGER:
        ((char_data *)dest)->proto_script = tp_dst;
        break;
      case OBJ_TRIGGER:
        ((obj_data *)dest)->proto_script = tp_dst;
        break;
      case WLD_TRIGGER:
        ((room_data *)dest)->proto_script = tp_dst;
        break;
    }

    while (tp_src) {
      tp_dst->vnum = tp_src->vnum;
      tp_src = tp_src->next;
      if (tp_src)
        CREATE(tp_dst->next, struct trig_proto_list, 1);
      tp_dst = tp_dst->next;
    }
  }
}

void delete_variables(const char *charname)
{
  char filename[PATH_MAX];

  if (!get_filename(filename, sizeof(filename), SCRIPT_VARS_FILE, charname))
    return;

  if (remove(filename) < 0 && errno != ENOENT)
    log("SYSERR: deleting variable file %s: %s", filename, strerror(errno));
}

void update_wait_events(struct room_data *to, struct room_data *from)
{
  struct trig_data *trig;

  if (!SCRIPT(from))
    return;

  for (trig = TRIGGERS(SCRIPT(from)); trig; trig = trig->next) {
    if (!GET_TRIG_WAIT(trig))
      continue;

    ((struct wait_event_data *)GET_TRIG_WAIT(trig)->event_obj)->go = to;
  }
}
