/**************************************************************************
*  File: genwld.c                                          Part of tbaMUD *
*  Usage: Generic OLC Library - Rooms.                                    *
*                                                                         *
*  By Levork. Copyright 1996 by Harvey Gilpin, 1997-2001 by George Greer. *
**************************************************************************/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "db.h"
#include "handler.h"
#include "comm.h"
#include "genolc.h"
#include "genwld.h"
#include "genzon.h"
#include "shop.h"
#include "dg_olc.h"
#include "mud_event.h"


/* This function will copy the strings so be sure you free your own copies of 
 * the description, title, and such. */
room_rnum add_room(struct room_data *room)
{
  struct char_data *tch;
  struct obj_data *tobj;
  int j, found = FALSE;
  byte tlight;
  room_rnum i;

  if (room == NULL)
    return NOWHERE;

  if ((i = real_room(room->number)) != NOWHERE) {
    if (SCRIPT(&world[i]))
      extract_script(&world[i], WLD_TRIGGER);
    tch = world[i].people;
    tobj = world[i].contents;
    tlight = world[i].light;
    copy_room(&world[i], room);
    world[i].people = tch;
    world[i].contents = tobj;
    /* light counts the light sources currently in the room, so it belongs
     * to the live room and not to the copy being saved.  redit captured it
     * when editing began; restoring that would undo every torch that has
     * come or gone since, and the count never recovers. */
    world[i].light = tlight;
    add_to_save_list(zone_table[room->zone].number, SL_WLD);
    log("GenOLC: add_room: Updated existing room #%d.", room->number);
    return i;
  }

  RECREATE(world, struct room_data, top_of_world + 2);
  top_of_world++;

  for (i = top_of_world; i > 0; i--) {
    if (room->number > world[i - 1].number) {
      world[i] = *room;
      copy_room_strings(&world[i], room);
      found = i;
      break;
    } else {
      /* Copy the room over now. */
      world[i] = world[i - 1];
      update_wait_events(&world[i], &world[i-1]);

      /* People in this room must have their in_rooms moved up one. */
      for (tch = world[i].people; tch; tch = tch->next_in_room)
	IN_ROOM(tch) += (IN_ROOM(tch) != NOWHERE);

      /* Move objects too. */
      for (tobj = world[i].contents; tobj; tobj = tobj->next_content)
	IN_ROOM(tobj) += (IN_ROOM(tobj) != NOWHERE);
    }
  }
  if (!found) {
    world[0] = *room;	/* Last place, in front. */
    copy_room_strings(&world[0], room);
  }

  log("GenOLC: add_room: Added room %d at index #%d.", room->number, found);
  /* found is equal to the array index where we added the room. */

  /* Update every zone's loading table, not just this room's zone and the
   * ones above it.  Nothing confines a reset command to the zone that owns
   * the room it targets -- the stock world ships 26 aimed at a zone higher
   * than the one they sit in -- and such a command, pointing at a room at
   * or after the insertion point, needs the same fixup.  delete_room() has
   * always scanned from 0 for exactly this reason. */
  for (i = 0; i <= top_of_zone_table; i++)
    for (j = 0; ZCMD(i, j).command != 'S'; j++)
      switch (ZCMD(i, j).command) {
      case 'M':
      case 'O':
      case 'T':
      case 'V':
	ZCMD(i, j).arg3 += (ZCMD(i, j).arg3 != NOWHERE && ZCMD(i, j).arg3 >= found);
	break;
      case 'D':
      case 'R':
	ZCMD(i, j).arg1 += (ZCMD(i, j).arg1 != NOWHERE && ZCMD(i, j).arg1 >= found);
      case 'G':
      case 'P':
      case 'E':
      case '*':
	/* Known zone entries we don't care about. */
        break;
      default:
        mudlog(BRF, LVL_GOD, TRUE, "SYSERR: GenOLC: add_room: Unknown zone entry found!");
      }

  /* Update the loadroom table. Adds 1 or 0. */
  r_mortal_start_room += (r_mortal_start_room >= found);
  r_immort_start_room += (r_immort_start_room >= found);
  r_frozen_start_room += (r_frozen_start_room >= found);

  /* was_in_room is a bare room rnum kept on the character; it is not a link
   * into any room's people list, so the pass above cannot reach it.  That
   * pass walks world[i].people for each room it shifts and fixes IN_ROOM
   * alone.  A character idled into the void is in world[1].people --
   * limits.c does char_from_room() then char_to_room(ch, 1) -- and index 1
   * is shifted only when the new room lands at or below it, which is
   * almost never.  The room they were pulled out of, recorded here, is a
   * different index entirely and may well be at or above the insertion
   * point.  Without this they come back one room off. */
  for (tch = character_list; tch; tch = tch->next)
    GET_WAS_IN(tch) += (GET_WAS_IN(tch) != NOWHERE && GET_WAS_IN(tch) >= found);

  /* Update world exits. */
  i = top_of_world + 1;
  do {
    i--;
    for (j = 0; j < DIR_COUNT; j++)
      if (W_EXIT(i, j) && W_EXIT(i, j)->to_room != NOWHERE)
	W_EXIT(i, j)->to_room += (W_EXIT(i, j)->to_room >= found);
  } while (i > 0);

  add_to_save_list(zone_table[room->zone].number, SL_WLD);

  /* Return what array entry we placed the new room in. */
  return found;
}

int delete_room(room_rnum rnum)
{
  room_rnum i;
  int j;
  struct char_data *ppl, *next_ppl;
  struct obj_data *obj, *next_obj;
  struct room_data *room;

  if (rnum <= 0 || rnum > top_of_world)	/* Can't delete void yet. */
    return FALSE;

  room = &world[rnum];

  add_to_save_list(zone_table[room->zone].number, SL_WLD);

  /* This is something you might want to read about in the logs. */
  log("GenOLC: delete_room: Deleting room #%d (%s).", room->number, room->name);

  if (r_mortal_start_room == rnum) {
    log("WARNING: GenOLC: delete_room: Deleting mortal start room!");
    r_mortal_start_room = 0;	/* The Void */
  }
  if (r_immort_start_room == rnum) {
    log("WARNING: GenOLC: delete_room: Deleting immortal start room!");
    r_immort_start_room = 0;	/* The Void */
  }
  if (r_frozen_start_room == rnum) {
    log("WARNING: GenOLC: delete_room: Deleting frozen start room!");
    r_frozen_start_room = 0;	/* The Void */
  }

  /* Deleting one of these is handled above; deleting a room *below* one is
   * not, and add_room() has always incremented all three for exactly that
   * reason.  Without the decrement, a start room quietly points one room
   * high for the rest of the boot -- and it is read on every login, not on
   * some rare path.  check_start_rooms() resolves all three at boot and
   * falls back rather than leaving NOWHERE, so no guard is needed here,
   * just as add_room() needs none. */
  r_mortal_start_room -= (r_mortal_start_room > rnum);
  r_immort_start_room -= (r_immort_start_room > rnum);
  r_frozen_start_room -= (r_frozen_start_room > rnum);

  /* Dump the contents of this room into the Void.  We could also just extract 
   * the people, mobs, and objects here. */
  for (obj = world[rnum].contents; obj; obj = next_obj) {
    next_obj = obj->next_content;
    obj_from_room(obj);
    obj_to_room(obj, 0);
  }
  for (ppl = world[rnum].people; ppl; ppl = next_ppl) {
    next_ppl = ppl->next_in_room;
    char_from_room(ppl);
    char_to_room(ppl, 0);
  }

  free_room_strings(room);
  if (SCRIPT(room))
    extract_script(room, WLD_TRIGGER);
  free_proto_script(room, WLD_TRIGGER);

  if (room->events != NULL) {
	  if (room->events->iSize > 0) {
		struct event * pEvent;

		while ((pEvent = simple_list(room->events)) != NULL)
		  event_cancel(pEvent);
	  }
	  free_list(room->events);
    room->events = NULL;
  }

  /* Change any exit going to this room to go the void. Also fix all the exits 
   * pointing to rooms above this. */
  i = top_of_world + 1;
  do {
    i--;
    for (j = 0; j < DIR_COUNT; j++) {
      if (W_EXIT(i, j) == NULL)
        continue;
      else if (W_EXIT(i, j)->to_room > rnum)
        W_EXIT(i, j)->to_room -= (W_EXIT(i, j)->to_room != NOWHERE); /* with unsigned NOWHERE > any rnum */
      else if (W_EXIT(i, j)->to_room == rnum) {
      	if ((!W_EXIT(i, j)->keyword || !*W_EXIT(i, j)->keyword) &&
      	    (!W_EXIT(i, j)->general_description || !*W_EXIT(i, j)->general_description)) {
          /* no description, remove exit completely */
          if (W_EXIT(i, j)->keyword)
            free(W_EXIT(i, j)->keyword);
          if (W_EXIT(i, j)->general_description)
            free(W_EXIT(i, j)->general_description);
          free(W_EXIT(i, j));
          W_EXIT(i, j) = NULL;
        } else {
          /* description is set, just point to nowhere */
          W_EXIT(i, j)->to_room = NOWHERE;
        }
      }
    }
  } while (i > 0);

  /* Find what zone that room was in so we can update the loading table.
   * Every zone whose commands are cancelled or renumbered here has to be
   * written back out as well, or its .zon keeps pointing at a room that
   * no longer exists.  save_zone() drops cancelled ('*') commands, so
   * saving is what actually removes them. */
  for (i = 0; i <= top_of_zone_table; i++) {
    int zone_touched = FALSE;

    for (j = 0; ZCMD(i , j).command != 'S'; j++)
      switch (ZCMD(i, j).command) {
      case 'M':
      case 'O':
      case 'T':
      case 'V':
	if (ZCMD(i, j).arg3 == rnum) {
	  ZCMD(i, j).command = '*';	/* Cancel command. */
	  zone_touched = TRUE;
	} else if (ZCMD(i, j).arg3 > rnum) {
	  ZCMD(i, j).arg3 -= (ZCMD(i, j).arg3 != NOWHERE); /* with unsigned NOWHERE > any rnum */
	  zone_touched = TRUE;
	}
	break;
      case 'D':
      case 'R':
	if (ZCMD(i, j).arg1 == rnum) {
	  ZCMD(i, j).command = '*';	/* Cancel command. */
	  zone_touched = TRUE;
	} else if (ZCMD(i, j).arg1 > rnum) {
	  ZCMD(i, j).arg1 -= (ZCMD(i, j).arg1 != NOWHERE); /* with unsigned NOWHERE > any rnum */
	  zone_touched = TRUE;
	}
      case 'G':
      case 'P':
      case 'E':
      case '*':
        /* Known zone entries we don't care about. */
        break;
      default:
        mudlog(BRF, LVL_GOD, TRUE, "SYSERR: GenOLC: delete_room: Unknown zone entry found!");
      }

    if (zone_touched)
      add_to_save_list(zone_table[i].number, SL_ZON);
  }

  /* Remove this room from all shop lists. */
  for (i = 0; i <= top_shop; i++) {
    for (j = 0;SHOP_ROOM(i, j) != NOWHERE;j++) {
      if (SHOP_ROOM(i, j) == world[rnum].number)
        SHOP_ROOM(i, j) = 0; /* set to the void */
    }
  }
  /* was_in_room is a bare room rnum kept on the character, not a link into
   * any room's people list, so the shift below cannot reach it -- that loop
   * walks world[i].people and repairs IN_ROOM alone, and a linkdead
   * character is not in the room it records.  This is add_room()'s pass in
   * reverse: the room they were pulled out of is gone, so there is nothing
   * to send them back to and the reference is dropped; a room deleted below
   * it moves their index down like every other rnum here.  NOWHERE is what
   * game_loop() already tests for before returning anyone, so clearing it
   * leaves them where they are rather than somewhere arbitrary. */
  for (ppl = character_list; ppl; ppl = ppl->next) {
    if (GET_WAS_IN(ppl) == rnum)
      GET_WAS_IN(ppl) = NOWHERE;
    else if (GET_WAS_IN(ppl) > rnum)
      GET_WAS_IN(ppl) -= (GET_WAS_IN(ppl) != NOWHERE); /* with unsigned NOWHERE > any rnum */
  }

  /* Now we actually move the rooms down. */
  for (i = rnum; i < top_of_world; i++) {
    world[i] = world[i + 1];
    update_wait_events(&world[i], &world[i+1]);

    for (ppl = world[i].people; ppl; ppl = ppl->next_in_room)
      IN_ROOM(ppl) -= (IN_ROOM(ppl) != NOWHERE);	/* Redundant check? */

    for (obj = world[i].contents; obj; obj = obj->next_content)
      IN_ROOM(obj) -= (IN_ROOM(obj) != NOWHERE);	/* Redundant check? */
  }

  top_of_world--;
  RECREATE(world, struct room_data, top_of_world + 1);

  return TRUE;
}

int save_rooms(zone_rnum rzone)
{
  int i;
  struct room_data *room;
  FILE *sf;
  char filename[128];
  char buf[MAX_STRING_LENGTH];
  char buf1[MAX_STRING_LENGTH];
  char buf2[MAX_STRING_LENGTH];

#if CIRCLE_UNSIGNED_INDEX
  if (rzone == NOWHERE || rzone > top_of_zone_table) {
#else
  if (rzone < 0 || rzone > top_of_zone_table) {
#endif
    log("SYSERR: GenOLC: save_rooms: Invalid zone number %d passed! (0-%d)", rzone, top_of_zone_table);
    return FALSE;
  }

  log("GenOLC: save_rooms: Saving rooms in zone #%d (%d-%d).",
	zone_table[rzone].number, genolc_zone_bottom(rzone), zone_table[rzone].top);

  snprintf(filename, sizeof(filename), "%s/%d.new", WLD_PREFIX, zone_table[rzone].number);
  if (!(sf = fopen(filename, "w"))) {
    perror("SYSERR: save_rooms");
    return FALSE;
  }

  for (i = genolc_zone_bottom(rzone); i <= zone_table[rzone].top; i++) {
    room_rnum rnum;

    if ((rnum = real_room(i)) != NOWHERE) {
      int j;

      room = (world + rnum);

      /* Copy the description and strip off trailing newlines. */
      strncpy(buf, room->description ? room->description : "Empty room.", sizeof(buf)-1 );
      strip_cr(buf);

      /* Save the numeric and string section of the file. */
      int n = snprintf(buf2, MAX_STRING_LENGTH, "#%d\n"
			"%s%c\n"
			"%s%c\n"
			"%d %d %d %d %d %d\n",
	room->number,
	room->name ? room->name : "Untitled", STRING_TERMINATOR,
	buf, STRING_TERMINATOR,
	zone_table[room->zone].number, room->room_flags[0], room->room_flags[1], room->room_flags[2], 
	  room->room_flags[3], room->sector_type 
      );
      
      if(n >= MAX_STRING_LENGTH) {
        mudlog(BRF,LVL_BUILDER,TRUE,
               "SYSERR: Could not save room #%d due to size (%d > maximum of %d).",
               room->number, n, MAX_STRING_LENGTH);
        continue;
      }

  fprintf(sf, "%s", convert_from_tabs(buf2));
 
      /* Now you write out the exits for the room. */
      for (j = 0; j < DIR_COUNT; j++) {
	if (R_EXIT(room, j)) {
	  int dflag;
	  if (R_EXIT(room, j)->general_description) {
	    strncpy(buf, R_EXIT(room, j)->general_description, sizeof(buf)-1);
	    strip_cr(buf);
	  } else
	    *buf = '\0';

	  /* Figure out door flag. */
	  if (IS_SET(R_EXIT(room, j)->exit_info, EX_ISDOOR)) {
	    if (IS_SET(R_EXIT(room, j)->exit_info, EX_PICKPROOF))
	      dflag = 2;
	    else
	      dflag = 1;
	      
	   if (IS_SET(R_EXIT(room, j)->exit_info, EX_HIDDEN))
          dflag += 2;   
          
	  } else
	    dflag = 0;

	  if (R_EXIT(room, j)->keyword)
	    strncpy(buf1, R_EXIT(room, j)->keyword, sizeof(buf1)-1 );
	  else
	    *buf1 = '\0';

	  /* Now write the exit to the file. */
	  fprintf(sf,	"D%d\n"
			"%s~\n"
			"%s~\n"
			"%d %d %d\n", j, buf, buf1, dflag,
		R_EXIT(room, j)->key != NOTHING ? R_EXIT(room, j)->key : -1,
		R_EXIT(room, j)->to_room != NOWHERE ? world[R_EXIT(room, j)->to_room].number : -1);

	}
      }

      if (room->ex_description) {
        struct extra_descr_data *xdesc;

	for (xdesc = room->ex_description; xdesc; xdesc = xdesc->next) {
	  strncpy(buf, xdesc->description, sizeof(buf) - 1);
	  buf[sizeof(buf) - 1] = '\0';
	  strip_cr(buf);
	  fprintf(sf,	"E\n"
			"%s~\n"
			"%s~\n", xdesc->keyword, buf);
	}
      }
      fprintf(sf, "S\n");
      script_save_to_disk(sf, room, WLD_TRIGGER);
    }
  }

  /* Write the final line and close it. */
  fprintf(sf, "$~\n");
  fclose(sf);

  /* Old file we're replacing. */
  snprintf(buf, sizeof(buf), "%s/%d.wld", WLD_PREFIX, zone_table[rzone].number);

  remove(buf);
  rename(filename, buf);

  if (in_save_list(zone_table[rzone].number, SL_WLD))
    remove_from_save_list(zone_table[rzone].number, SL_WLD);
  return TRUE;
}

int copy_room(struct room_data *to, struct room_data *from)
{
  free_room_strings(to);
  *to = *from;
  copy_room_strings(to, from);
  to->events = from->events;

  /* Don't put people and objects in two locations. Should this be done here? */
  from->people = NULL;
  from->contents = NULL;
  from->events = NULL;

  return TRUE;
}

/* Copy strings over so bad things don't happen.  We do not free the existing 
 * strings here because copy_room() did a shallow copy previously and we'd be 
 * freeing the very strings we're copying.  If this function is used elsewhere,
 * be sure to free_room_strings() the 'dest' room first. */
int copy_room_strings(struct room_data *dest, struct room_data *source)
{
  int i;

  if (dest == NULL || source == NULL) {
    log("SYSERR: GenOLC: copy_room_strings: NULL values passed.");
    return FALSE;
  }

  dest->description = str_udup(source->description);
  dest->name = str_udup(source->name);

  for (i = 0; i < DIR_COUNT; i++) {
    if (!R_EXIT(source, i))
      continue;

    CREATE(R_EXIT(dest, i), struct room_direction_data, 1);
    *R_EXIT(dest, i) = *R_EXIT(source, i);
    if (R_EXIT(source, i)->general_description)
      R_EXIT(dest, i)->general_description = strdup(R_EXIT(source, i)->general_description);
    if (R_EXIT(source, i)->keyword)
      R_EXIT(dest, i)->keyword = strdup(R_EXIT(source, i)->keyword);
  }

  if (source->ex_description)
    copy_ex_descriptions(&dest->ex_description, source->ex_description);

  return TRUE;
}

int free_room_strings(struct room_data *room)
{
  int i;

  /* Free descriptions. */
  if (room->name)
    free(room->name);
  if (room->description)
    free(room->description);
  if (room->ex_description)
    free_ex_descriptions(room->ex_description);

  /* Free exits. */
  for (i = 0; i < DIR_COUNT; i++) {
    if (room->dir_option[i]) {
      if (room->dir_option[i]->general_description)
        free(room->dir_option[i]->general_description);

      if (room->dir_option[i]->keyword)
        free(room->dir_option[i]->keyword);

      free(room->dir_option[i]);
      room->dir_option[i] = NULL;
    }
  }

  return TRUE;
}
