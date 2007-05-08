/**************************************************************************
*  File: oasis_copy.c                                      Part of tbaMUD *
*  Usage: Oasis OLC copying.                                              *
*                                                                         *
* By Levork. Copyright 1996 Harvey Gilpin. 1997-2001 George Greer.        *
* 2002 Kip Potter [Mythran].                                              *
**************************************************************************/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "genolc.h"
#include "genzon.h"
#include "genwld.h"
#include "oasis.h"
#include "improved-edit.h"
#include "constants.h"

/* Internal Functions */
ACMD(do_dig);
ACMD(do_room_copy);
room_vnum redit_find_new_vnum(zone_rnum zone);
int buildwalk(struct char_data *ch, int dir);

/* Commands */
ACMD(do_dig)
{
  char sdir[MAX_INPUT_LENGTH], sroom[MAX_INPUT_LENGTH], *new_room_name;
  room_vnum rvnum = NOWHERE;
  room_rnum rrnum = NOWHERE;
  zone_rnum zone;
  int dir = 0, rawvnum;
  struct descriptor_data *d = ch->desc; /* will save us some typing */

  /* Grab the room's name (if available). */
  new_room_name = two_arguments(argument, sdir, sroom);
  skip_spaces(&new_room_name);

  /* Can't dig if we don't know where to go. */
  if (!*sdir || !*sroom) {
    send_to_char(ch, "Format: dig <direction> <room> - to create an exit\r\n"
                     "        dig <direction> -1     - to delete an exit\r\n");
    return;
  }

  rawvnum = atoi(sroom);
  if (rawvnum == -1)
    rvnum = NOWHERE;
  else
    rvnum = (room_vnum)rawvnum;
  rrnum = real_room(rvnum);
  dir = search_block(sdir, dirs, FALSE);
  zone = world[IN_ROOM(ch)].zone;

  if (dir < 0) {
    send_to_char(ch, "You cannot create an exit to the '%s'.\r\n", sdir);
    return;
  }
  /* Make sure that the builder has access to the zone he's in. */
  if ((zone == NOWHERE) || !can_edit_zone(ch, zone)) {
    send_to_char(ch, "You do not have permission to edit this zone.\r\n");
    return;
  }
  /* Lets not allow digging to limbo. After all, it'd just get us more errors 
   * on 'show errors.' */
  if (rvnum == 0) {
   send_to_char(ch, "The target exists, but you can't dig to limbo!\r\n");
   return;
  }
  /* Target room == -1 removes the exit. */
  if (rvnum == NOTHING) {
    if (W_EXIT(IN_ROOM(ch), dir)) {
      /* free the old pointers, if any */
      if (W_EXIT(IN_ROOM(ch), dir)->general_description)
        free(W_EXIT(IN_ROOM(ch), dir)->general_description);
      if (W_EXIT(IN_ROOM(ch), dir)->keyword)
        free(W_EXIT(IN_ROOM(ch), dir)->keyword);
      free(W_EXIT(IN_ROOM(ch), dir));
      W_EXIT(IN_ROOM(ch), dir) = NULL;
      add_to_save_list(zone_table[world[IN_ROOM(ch)].zone].number, SL_WLD);
      send_to_char(ch, "You remove the exit to the %s.\r\n", dirs[dir]);
      return;
    }
    send_to_char(ch, "There is no exit to the %s.\r\n"
                     "No exit removed.\r\n", dirs[dir]);
    return;
  }
  /* Can't dig in a direction, if it's already a door. */
  if (W_EXIT(IN_ROOM(ch), dir)) {
      send_to_char(ch, "There already is an exit to the %s.\r\n", dirs[dir]);
      return;
  }

  /* Make sure that the builder has access to the zone he's linking to. */
  zone = real_zone_by_thing(rvnum);
  if (zone == NOWHERE) {
    send_to_char(ch, "You cannot link to a non-existing zone!\r\n");
    return;
  }
  if (!can_edit_zone(ch, zone)) {
    send_to_char(ch, "You do not have permission to edit room #%d.\r\n", rvnum);
    return;
  }
  /* Now we know the builder is allowed to make the link. */
  /* If the room doesn't exist, create it.*/
  if (rrnum == NOWHERE) {
    /* Give the descriptor an olc struct. This way we can let 
     * redit_save_internally handle the room adding. */
    if (d->olc) {
      mudlog(BRF, LVL_IMMORT, TRUE, "SYSERR: do_dig: Player already had olc structure.");
      free(d->olc);
    }
    CREATE(d->olc, struct oasis_olc_data, 1);
    OLC_ZNUM(d) = zone;
    OLC_NUM(d) = rvnum;
    CREATE(OLC_ROOM(d), struct room_data, 1);


    /* Copy the room's name. */
    if (*new_room_name)
     OLC_ROOM(d)->name = strdup(new_room_name);
    else
     OLC_ROOM(d)->name = strdup("An unfinished room");

    /* Copy the room's description.*/
    OLC_ROOM(d)->description = strdup("You are in an unfinished room.\r\n");
    OLC_ROOM(d)->zone = OLC_ZNUM(d);
    OLC_ROOM(d)->number = NOWHERE;

    /* Save the new room to memory. redit_save_internally handles adding the 
     * room in the right place, etc. */
    redit_save_internally(d);
    OLC_VAL(d) = 0;

    send_to_char(ch, "New room (%d) created.\r\n", rvnum);
    cleanup_olc(d, CLEANUP_STRUCTS);
    /* Update rrnum to the correct room rnum after adding the room. */
    rrnum = real_room(rvnum);
  }

  /* Now dig. */
  CREATE(W_EXIT(IN_ROOM(ch), dir), struct room_direction_data, 1);
  W_EXIT(IN_ROOM(ch), dir)->general_description = NULL;
  W_EXIT(IN_ROOM(ch), dir)->keyword = NULL;
  W_EXIT(IN_ROOM(ch), dir)->to_room = rrnum;
  add_to_save_list(zone_table[world[IN_ROOM(ch)].zone].number, SL_WLD);

  send_to_char(ch, "You make an exit %s to room %d (%s).\r\n",
                   dirs[dir], rvnum, world[rrnum].name);

  /* Check if we can dig from there to here. */
  if (W_EXIT(rrnum, rev_dir[dir]))
    send_to_char(ch, "You cannot dig from %d to here. The target room already has an exit to the %s.\r\n",
                     rvnum, dirs[rev_dir[dir]]);
  else {
    CREATE(W_EXIT(rrnum, rev_dir[dir]), struct room_direction_data, 1);
    W_EXIT(rrnum, rev_dir[dir])->general_description = NULL;
    W_EXIT(rrnum, rev_dir[dir])->keyword = NULL;
    W_EXIT(rrnum, rev_dir[dir])->to_room = IN_ROOM(ch);
    add_to_save_list(zone_table[world[rrnum].zone].number, SL_WLD);
  }
}

ACMD(do_room_copy)
{
   struct room_data *room_src, *room_dst;
   int room_num, j, buf_num, taeller;
   zone_rnum dst_zone;
   struct descriptor_data *dsc;
   char buf[MAX_INPUT_LENGTH];

   one_argument(argument, buf);

   if (!*buf) {
     send_to_char(ch, "Usage: rclone <target room>\r\n");
     return;
   }

   if (real_room((buf_num = atoi(buf))) != NOWHERE) {
     send_to_char(ch, "That room already exist!\r\n");
     return;
   }

   if ((dst_zone = real_zone_by_thing(buf_num)) == NOWHERE) {
     send_to_char(ch, "Sorry, there is no zone for that number!\r\n");
     return;
   }

   if (!can_edit_zone(ch, dst_zone) ||
       !can_edit_zone(ch, world[IN_ROOM(ch)].zone) ) {
     send_to_char(ch, "You may only copy rooms within your designated zone(s)!\r\n");
     return;
   }


   room_src = &world[IN_ROOM(ch)];
   CREATE(room_dst, struct room_data, 1);

   room_dst->zone = dst_zone;

   /* Allocate space for all strings. */
   send_to_char(ch, "Cloning room....\r\n");

   room_dst->name = str_udup(world[IN_ROOM(ch)].name);
   room_dst->description = str_udup(world[IN_ROOM(ch)].description);
   room_dst->description = str_udup(world[IN_ROOM(ch)].description);
   room_dst->number = buf_num;
   room_dst->sector_type = world[IN_ROOM(ch)].sector_type;
   for(taeller=0; taeller < RF_ARRAY_MAX; taeller++)
     room_dst->room_flags[taeller] = ROOM_FLAGS(IN_ROOM(ch))[taeller];

  /* Extra descriptions, if necessary. */
  send_to_char(ch, "Cloning extra descriptions....\r\n");
  if (world[IN_ROOM(ch)].ex_description) {
    struct extra_descr_data *tdesc, *temp, *temp2;
    CREATE(temp, struct extra_descr_data, 1);

    room_dst->ex_description = temp;
    for (tdesc = world[IN_ROOM(ch)].ex_description; tdesc; tdesc = tdesc->next) {
      temp->keyword = strdup(tdesc->keyword);
      temp->description = strdup(tdesc->description);
      if (tdesc->next) {
	CREATE(temp2, struct extra_descr_data, 1);
	temp->next = temp2;
	temp = temp2;
      } else
	temp->next = NULL;
    }
  }
   /* Now save the room in the right place. */
  send_to_char(ch, "Saving new room...\r\n");

  if ((room_num = add_room(room_dst)) == NOWHERE) {
    send_to_char(ch, "Something went wrong...\r\n");
    log("SYSERR: do_room_copy: Something failed! (%d)", room_num);
    return;
  }
  /* Idea contributed by C.Raehl. */
  for (dsc = descriptor_list; dsc; dsc = dsc->next) {
    if (dsc == ch->desc)
      continue;

    if (STATE(dsc) == CON_ZEDIT) {
      for (j = 0; OLC_ZONE(dsc)->cmd[j].command != 'S'; j++)
        switch (OLC_ZONE(dsc)->cmd[j].command) {
          case 'O':
          case 'M':
            OLC_ZONE(dsc)->cmd[j].arg3 += (OLC_ZONE(dsc)->cmd[j].arg3 >= room_num);
            break;
          case 'D':
            OLC_ZONE(dsc)->cmd[j].arg2 += (OLC_ZONE(dsc)->cmd[j].arg2 >= room_num);
            /* Fall through */
          case 'R':
            OLC_ZONE(dsc)->cmd[j].arg1 += (OLC_ZONE(dsc)->cmd[j].arg1 >= room_num);
            break;
          }
    } else if (STATE(dsc) == CON_REDIT) {
      for (j = 0; j < NUM_OF_DIRS; j++)
        if (OLC_ROOM(dsc)->dir_option[j])
          if (OLC_ROOM(dsc)->dir_option[j]->to_room >= room_num)
            OLC_ROOM(dsc)->dir_option[j]->to_room++;
    }
  }
  add_to_save_list(real_zone_by_thing(atoi(buf)), SL_WLD);
  redit_save_to_disk(real_zone_by_thing(atoi(buf)));
  send_to_char(ch, "Room cloned to %d.\r\nAll Done.\r\n", buf_num);
}

/* BuildWalk - OasisOLC Extension by D. Tyler Barnes. */
/* For buildwalk. Finds the next free vnum in the zone */
room_vnum redit_find_new_vnum(zone_rnum zone)
{
  room_vnum vnum = genolc_zone_bottom(zone);
  room_rnum rnum = real_room(vnum);

  if (rnum == NOWHERE)
    return NOWHERE;

  for(;;) {
    if (vnum > zone_table[zone].top)
      return(NOWHERE);
    if (rnum > top_of_world || world[rnum].number > vnum)
      break;
    rnum++;
    vnum++;
  }
  return(vnum);
}

int buildwalk(struct char_data *ch, int dir)
{
  char buf[MAX_INPUT_LENGTH];
  room_vnum vnum;
  room_rnum rnum;

  if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_BUILDWALK) &&
      GET_LEVEL(ch) >= LVL_BUILDER) {

    get_char_colors(ch);

    if (!can_edit_zone(ch, world[IN_ROOM(ch)].zone)) {
      send_to_char(ch, "You do not have build permissions in this zone.\r\n");
    } else if ((vnum = redit_find_new_vnum(world[IN_ROOM(ch)].zone)) == NOWHERE) {
      send_to_char(ch, "No free vnums are available in this zone!\r\n");
    } else {
      struct descriptor_data *d = ch->desc;
      /* Give the descriptor an olc struct. This way we can let 
       * redit_save_internally handle the room adding. */
      if (d->olc) {
        mudlog(BRF, LVL_IMMORT, TRUE, "SYSERR: buildwalk(): Player already had olc structure.");
        free(d->olc);
      }
      CREATE(d->olc, struct oasis_olc_data, 1);
      OLC_ZNUM(d) = world[IN_ROOM(ch)].zone;
      OLC_NUM(d) = vnum;
      CREATE(OLC_ROOM(d), struct room_data, 1);

      OLC_ROOM(d)->name = strdup("New BuildWalk Room");

      sprintf(buf, "This unfinished room was created by %s.\r\n", GET_NAME(ch));
      OLC_ROOM(d)->description = strdup(buf);
      OLC_ROOM(d)->zone = OLC_ZNUM(d);
      OLC_ROOM(d)->number = NOWHERE;

      /* Save the new room to memory. redit_save_internally handles adding the 
       * room in the right place, etc. */
      redit_save_internally(d);
      OLC_VAL(d) = 0;

      /* Link rooms */
      rnum = real_room(vnum);
      CREATE(EXIT(ch, dir), struct room_direction_data, 1);
      EXIT(ch, dir)->to_room = rnum;
      CREATE(world[rnum].dir_option[rev_dir[dir]], struct room_direction_data, 1);
      world[rnum].dir_option[rev_dir[dir]]->to_room = IN_ROOM(ch);

      /* Report room creation to user */
      send_to_char(ch, "%sRoom #%d created by BuildWalk.%s\r\n", yel, vnum, nrm);
      cleanup_olc(d, CLEANUP_STRUCTS);

      return (1);

    }
  }

  return(0);
}
