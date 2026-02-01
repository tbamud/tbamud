/**************************************************************************
*  File: genzon.c                                          Part of tbaMUD *
*  Usage: Generic OLC Library - Zones.                                    *
*                                                                         *
*  Copyright 1996 by Harvey Gilpin, 1997-2001 by George Greer.            *
**************************************************************************/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "db.h"
#include "genolc.h"
#include "toml.h"
#include "toml_utils.h"
#include "genzon.h"
#include "py_triggers.h"

/* local functions */
static void remove_cmd_from_list(struct reset_com **list, int pos);

/* real zone of room/mobile/object/shop given */
zone_rnum real_zone_by_thing(room_vnum vznum)
{
  zone_rnum bot, top, mid;
  int low, high;

  bot = 0;
  top = top_of_zone_table;

  if (genolc_zone_bottom(bot) > vznum || zone_table[top].top < vznum)
    return (NOWHERE);

  /* perform binary search on zone-table */
  while (bot <= top) {
    mid = (bot + top) / 2;

    /* Upper/lower bounds of the zone. */
    low = genolc_zone_bottom(mid);
    high = zone_table[mid].top;

    if (low <= vznum && vznum <= high)
      return mid;
    if (low > vznum)
      top = mid - 1;
    else
      bot = mid + 1;
  }
  return (NOWHERE);
}

zone_rnum create_new_zone(zone_vnum vzone_num, room_vnum bottom, room_vnum top, const char **error)
{
  FILE *fp;
  struct zone_data *zone;
  int i, max_zone;
  zone_rnum rznum;
  char buf[MAX_STRING_LENGTH];

#if CIRCLE_UNSIGNED_INDEX
  max_zone = 655;
  if (vzone_num == NOWHERE) {
#else
  max_zone = 327;
  if (vzone_num < 0) {
#endif
    *error = "You can't make negative zones.\r\n";
    return NOWHERE;
   } else if (vzone_num > max_zone) {
#if CIRCLE_UNSIGNED_INDEX
    *error = "New zone cannot be higher than 655.\r\n";
#else
    *error = "New zone cannot be higher than 327.\r\n";
#endif
    return NOWHERE;
  } else if (bottom > top) {
    *error = "Bottom room cannot be greater than top room.\r\n";
    return NOWHERE;
  } else if (bottom <= 0) {
    *error = "Bottom room cannot be less then 0.\r\n";
    return NOWHERE;
  } else if (top >= IDXTYPE_MAX) {
    *error = "Top greater than IDXTYPE_MAX. (Commonly 65535)\r\n";
    return NOWHERE;
  }

  for (i = 0; i < top_of_zone_table; i++)
    if (zone_table[i].number == vzone_num) {
      *error = "That virtual zone already exists.\r\n";
      return NOWHERE;
     }

  /* Create the zone file. */
  snprintf(buf, sizeof(buf), "%s/%d.toml", ZON_PREFIX, vzone_num);
  if (!(fp = fopen(buf, "w"))) {
    mudlog(BRF, LVL_IMPL, TRUE, "SYSERR: OLC: Can't write new zone file.");
    *error = "Could not write zone file.\r\n";
    return NOWHERE;
  }
  fprintf(fp, "#%d\nNone~\nNew Zone~\n%d %d 30 2\nS\n$\n", vzone_num, bottom, top);
  fclose(fp);

  /* Create the room file. */
  snprintf(buf, sizeof(buf), "%s/%d.toml", WLD_PREFIX, vzone_num);
  if (!(fp = fopen(buf, "w"))) {
    mudlog(BRF, LVL_IMPL, TRUE, "SYSERR: OLC: Can't write new world file.");
    *error = "Could not write world file.\r\n";
    return NOWHERE;
  }
  fprintf(fp, "#%d\nThe Beginning~\nNot much here.\n~\n%d 0 0\nS\n$\n", bottom, vzone_num);
  fclose(fp);

  /* Create the mobile file. */
  snprintf(buf, sizeof(buf), "%s/%d.toml", MOB_PREFIX, vzone_num);
  if (!(fp = fopen(buf, "w"))) {
    mudlog(BRF, LVL_IMPL, TRUE, "SYSERR: OLC: Can't write new mob file.");
    *error = "Could not write mobile file.\r\n";
    return NOWHERE;
  }
  fprintf(fp, "$\n");
  fclose(fp);

  /* Create the object file. */
  snprintf(buf, sizeof(buf), "%s/%d.toml", OBJ_PREFIX, vzone_num);
  if (!(fp = fopen(buf, "w"))) {
    mudlog(BRF, LVL_IMPL, TRUE, "SYSERR: OLC: Can't write new obj file.");
    *error = "Could not write object file.\r\n";
    return NOWHERE;
  }
  fprintf(fp, "$\n");
  fclose(fp);

  /* Create the shop file. */
  snprintf(buf, sizeof(buf), "%s/%d.toml", SHP_PREFIX, vzone_num);
  if (!(fp = fopen(buf, "w"))) {
    mudlog(BRF, LVL_IMPL, TRUE, "SYSERR: OLC: Can't write new shop file.");
    *error = "Could not write shop file.\r\n";
    return NOWHERE;
  }
  fprintf(fp, "$~\n");
  fclose(fp);

  /* Create the quests file */
  snprintf(buf, sizeof(buf), "%s/%d.toml", QST_PREFIX, vzone_num);
  if (!(fp = fopen(buf, "w"))) {
    mudlog(BRF, LVL_IMPL, TRUE, "SYSERR: OLC: Can't write new quest file");
    *error = "Could not write quest file.\r\n";
    return NOWHERE;
  }
  fprintf(fp, "$~\n");
  fclose(fp);

  /* Create the trigger file. */
  snprintf(buf, sizeof(buf), "%s/%d.toml", TRG_PREFIX, vzone_num);
  if (!(fp = fopen(buf, "w"))) {
    mudlog(BRF, LVL_IMPL, TRUE, "SYSERR: OLC: Can't write new trigger file");
    *error = "Could not write trigger file.\r\n";
    return NOWHERE;
  }
  fprintf(fp, "$~\n");
  fclose(fp);

  /* Update index files. */
  create_world_index(vzone_num, "qst");
  create_world_index(vzone_num, "zon");
  create_world_index(vzone_num, "wld");
  create_world_index(vzone_num, "mob");
  create_world_index(vzone_num, "obj");
  create_world_index(vzone_num, "shp");
  create_world_index(vzone_num, "trg");

  /* Make a new zone in memory. This was the source of all the zedit new
   * crashes. It was happily overwriting the stack.  This new loop by Andrew
   * Helm fixes that problem and is more understandable at the same time.
   *
   * The variable is 'top_of_zone_table_table + 2' because we need record 0
   * through top_of_zone (top_of_zone_table + 1 items) and a new one which
   * makes it top_of_zone_table + 2 elements large. */
  RECREATE(zone_table, struct zone_data, top_of_zone_table + 2);
  zone_table[top_of_zone_table + 1].number = 32000;

  if (vzone_num > zone_table[top_of_zone_table].number)
    rznum = top_of_zone_table + 1;
  else {
    int j, room;
    for (i = top_of_zone_table + 1; i > 0 && vzone_num < zone_table[i - 1].number; i--) {
      zone_table[i] = zone_table[i - 1];
      for (j = zone_table[i].bot; j <= zone_table[i].top; j++)
        if ((room = real_room(j)) != NOWHERE)
          world[room].zone++;
    }
rznum = i;
  }
  zone = &zone_table[rznum];

  /* Ok, insert the new zone here. */
  zone->name = strdup("New Zone");
  zone->number = vzone_num;
  zone->builders = strdup("None");
  zone->bot = bottom;
  zone->top = top;
  zone->lifespan = 30;
  zone->age = 0;
  zone->reset_mode = 2;
  zone->min_level = -1;
  zone->max_level = -1;

  for (i=0; i<ZN_ARRAY_MAX; i++)  zone->zone_flags[i] = 0;

  /* No zone commands, just terminate it with an 'S' */
  CREATE(zone->cmd, struct reset_com, 1);
  zone->cmd[0].command = 'S';

  top_of_zone_table++;

  add_to_save_list(zone->number, SL_ZON);
  return rznum;
}

void create_world_index(int znum, const char *type)
{
  FILE *newfile, *oldfile;
  char new_name[32], old_name[32], *prefix;
  char **files = NULL;
  char errbuf[256];
  char file_name[32];
  toml_table_t *tab = NULL;
  toml_array_t *arr = NULL;
  int i, num, count, insert_at;

  switch (*type) {
  case 'z':
    prefix = ZON_PREFIX;
    break;
  case 'w':
    prefix = WLD_PREFIX;
    break;
  case 'o':
    prefix = OBJ_PREFIX;
    break;
  case 'm':
    prefix = MOB_PREFIX;
    break;
  case 's':
    prefix = SHP_PREFIX;
    break;
  case 't':
    prefix = TRG_PREFIX;
    break;
  case 'q':
    prefix = QST_PREFIX;
    break;
  default:
    /* Caller messed up. */
    return;
  }

  snprintf(old_name, sizeof(old_name), "%s/index.toml", prefix);
  snprintf(new_name, sizeof(new_name), "%s/newindex.toml", prefix);
  snprintf(file_name, sizeof(file_name), "%d.toml", znum);

  if (!(oldfile = fopen(old_name, "r"))) {
    mudlog(BRF, LVL_IMPL, TRUE, "SYSERR: OLC: Failed to open %s.", old_name);
    return;
  }

  tab = toml_parse_file(oldfile, errbuf, sizeof(errbuf));
  fclose(oldfile);
  if (!tab) {
    mudlog(BRF, LVL_IMPL, TRUE, "SYSERR: OLC: Failed to parse %s: %s.", old_name, errbuf);
    return;
  }

  arr = toml_array_in(tab, "files");
  if (!arr) {
    mudlog(BRF, LVL_IMPL, TRUE, "SYSERR: OLC: %s missing 'files' array.", old_name);
    toml_free(tab);
    return;
  }

  count = toml_array_nelem(arr);
  CREATE(files, char *, count + 1);
  for (i = 0; i < count; i++) {
    toml_datum_t d = toml_string_at(arr, i);
    if (!d.ok) {
      mudlog(BRF, LVL_IMPL, TRUE, "SYSERR: OLC: %s has invalid 'files' entry at %d.", old_name, i);
      toml_free(tab);
      while (i-- > 0)
        free(files[i]);
      free(files);
      return;
    }
    files[i] = d.u.s;
  }

  for (i = 0; i < count; i++) {
    if (!strcmp(files[i], file_name)) {
      toml_free(tab);
      for (i = 0; i < count; i++)
        free(files[i]);
      free(files);
      return;
    }
    if (sscanf(files[i], "%d", &num) == 1 && num == znum) {
      toml_free(tab);
      for (i = 0; i < count; i++)
        free(files[i]);
      free(files);
      return;
    }
  }

  insert_at = count;
  for (i = 0; i < count; i++) {
    if (sscanf(files[i], "%d", &num) == 1 && num > znum) {
      insert_at = i;
      break;
    }
  }

  RECREATE(files, char *, count + 2);
  if (insert_at < count)
    memmove(&files[insert_at + 1], &files[insert_at], sizeof(char *) * (count - insert_at));
  files[insert_at] = strdup(file_name);
  count++;

  if (!(newfile = fopen(new_name, "w"))) {
    mudlog(BRF, LVL_IMPL, TRUE, "SYSERR: OLC: Failed to open %s.", new_name);
    toml_free(tab);
    for (i = 0; i < count; i++)
      free(files[i]);
    free(files);
    return;
  }

  fprintf(newfile, "files = [\n");
  for (i = 0; i < count; i++) {
    fprintf(newfile, "  ");
    toml_write_string(newfile, files[i]);
    fprintf(newfile, "%s\n", (i + 1 < count) ? "," : "");
  }
  fprintf(newfile, "]\n");

  fclose(newfile);
  toml_free(tab);
  for (i = 0; i < count; i++)
    free(files[i]);
  free(files);

  /* Out with the old, in with the new. */
  remove(old_name);
  rename(new_name, old_name);
}

void remove_room_zone_commands(zone_rnum zone, room_rnum room_num)
{
  int subcmd = 0, cmd_room = -2;

  /* Delete all entries in zone_table that relate to this room so we can add
   * all the ones we have in their place. */
  while (zone_table[zone].cmd[subcmd].command != 'S') {
    switch (zone_table[zone].cmd[subcmd].command) {
    case 'M':
    case 'O':
    case 'T':
    case 'V':
      cmd_room = zone_table[zone].cmd[subcmd].arg3;
      break;
    case 'D':
    case 'R':
      cmd_room = zone_table[zone].cmd[subcmd].arg1;
      break;
    default:
      break;
    }
    if (cmd_room == room_num)
      remove_cmd_from_list(&zone_table[zone].cmd, subcmd);
    else
      subcmd++;
  }
}

/* Save all the zone_table for this zone to disk.  This function now writes
 * simple comments in the form of (<name>) to each record.  A header for each
 * field is also there. */
int save_zone(zone_rnum zone_num)
{
  int subcmd, arg1 = -1, arg2 = -1, arg3 = -1;
  char fname[128], oldname[128];
  FILE *zfile;

#if CIRCLE_UNSIGNED_INDEX
  if (zone_num == NOWHERE || zone_num > top_of_zone_table) {
#else
  if (zone_num < 0 || zone_num > top_of_zone_table) {
#endif
    log("SYSERR: GenOLC: save_zone: Invalid real zone number %d. (0-%d)", zone_num, top_of_zone_table);
    return FALSE;
  }

  snprintf(fname, sizeof(fname), "%s/%d.new", ZON_PREFIX, zone_table[zone_num].number);
  if (!(zfile = fopen(fname, "w"))) {
    mudlog(BRF, LVL_BUILDER, TRUE, "SYSERR: OLC: save_zones:  Can't write zone %d.", zone_table[zone_num].number);
    return FALSE;
  }

  fprintf(zfile, "[[zone]]\n");
  fprintf(zfile, "vnum = %d\n", zone_table[zone_num].number);
  toml_write_kv_string(zfile, "builders",
                       (zone_table[zone_num].builders && *zone_table[zone_num].builders)
                         ? zone_table[zone_num].builders : "None.");
  toml_write_kv_string(zfile, "name",
                       (zone_table[zone_num].name && *zone_table[zone_num].name)
                         ? convert_from_tabs(zone_table[zone_num].name) : "undefined");
  fprintf(zfile, "bot = %d\n", genolc_zone_bottom(zone_num));
  fprintf(zfile, "top = %d\n", zone_table[zone_num].top);
  fprintf(zfile, "lifespan = %d\n", zone_table[zone_num].lifespan);
  fprintf(zfile, "reset_mode = %d\n", zone_table[zone_num].reset_mode);
  fprintf(zfile, "flags = [%d, %d, %d, %d]\n",
          zone_table[zone_num].zone_flags[0],
          zone_table[zone_num].zone_flags[1],
          zone_table[zone_num].zone_flags[2],
          zone_table[zone_num].zone_flags[3]);
  fprintf(zfile, "min_level = %d\n", zone_table[zone_num].min_level);
  fprintf(zfile, "max_level = %d\n", zone_table[zone_num].max_level);

  for (subcmd = 0; ZCMD(zone_num, subcmd).command != 'S'; subcmd++) {
    switch (ZCMD(zone_num, subcmd).command) {
    case 'M':
      arg1 = mob_index[ZCMD(zone_num, subcmd).arg1].vnum;
      arg2 = ZCMD(zone_num, subcmd).arg2;
      arg3 = world[ZCMD(zone_num, subcmd).arg3].number;
      break;
    case 'O':
      arg1 = obj_index[ZCMD(zone_num, subcmd).arg1].vnum;
      arg2 = ZCMD(zone_num, subcmd).arg2;
      arg3 = world[ZCMD(zone_num, subcmd).arg3].number;
      break;
    case 'G':
      arg1 = obj_index[ZCMD(zone_num, subcmd).arg1].vnum;
      arg2 = ZCMD(zone_num, subcmd).arg2;
      arg3 = -1;
      break;
    case 'E':
      arg1 = obj_index[ZCMD(zone_num, subcmd).arg1].vnum;
      arg2 = ZCMD(zone_num, subcmd).arg2;
      arg3 = ZCMD(zone_num, subcmd).arg3;
      break;
    case 'P':
      arg1 = obj_index[ZCMD(zone_num, subcmd).arg1].vnum;
      arg2 = ZCMD(zone_num, subcmd).arg2;
      arg3 = obj_index[ZCMD(zone_num, subcmd).arg3].vnum;
      break;
    case 'D':
      arg1 = world[ZCMD(zone_num, subcmd).arg1].number;
      arg2 = ZCMD(zone_num, subcmd).arg2;
      arg3 = ZCMD(zone_num, subcmd).arg3;
      break;
    case 'R':
      arg1 = world[ZCMD(zone_num, subcmd).arg1].number;
      arg2 = obj_index[ZCMD(zone_num, subcmd).arg2].vnum;
      arg3 = -1;
      break;
    case 'T':
      arg1 = ZCMD(zone_num, subcmd).arg1; /* trigger type */
      arg2 = trig_index[ZCMD(zone_num, subcmd).arg2]->vnum; /* trigger vnum */
      arg3 = world[ZCMD(zone_num, subcmd).arg3].number; /* room num */
      break;
    case 'V':
      arg1 = ZCMD(zone_num, subcmd).arg1; /* trigger type */
      arg2 = ZCMD(zone_num, subcmd).arg2; /* context */
      arg3 = world[ZCMD(zone_num, subcmd).arg3].number;
      break;
    case '*':
      /* Invalid commands are replaced with '*' - Ignore them. */
      continue;
    default:
      mudlog(BRF, LVL_BUILDER, TRUE, "SYSERR: OLC: z_save_to_disk(): Unknown cmd '%c' - NOT saving", ZCMD(zone_num, subcmd).command);
      continue;
    }
    fprintf(zfile, "\n[[zone.command]]\n");
    fprintf(zfile, "command = \"%c\"\n", ZCMD(zone_num, subcmd).command);
    fprintf(zfile, "if_flag = %d\n", ZCMD(zone_num, subcmd).if_flag);
    fprintf(zfile, "arg1 = %d\n", arg1);
    fprintf(zfile, "arg2 = %d\n", arg2);
    fprintf(zfile, "arg3 = %d\n", arg3);
    if (ZCMD(zone_num, subcmd).command == 'V') {
      toml_write_kv_string(zfile, "sarg1", ZCMD(zone_num, subcmd).sarg1 ? ZCMD(zone_num, subcmd).sarg1 : "");
      toml_write_kv_string(zfile, "sarg2", ZCMD(zone_num, subcmd).sarg2 ? ZCMD(zone_num, subcmd).sarg2 : "");
    }
    fprintf(zfile, "line = %d\n", ZCMD(zone_num, subcmd).line);
  }
  fclose(zfile);
  snprintf(oldname, sizeof(oldname), "%s/%d.toml", ZON_PREFIX, zone_table[zone_num].number);
  remove(oldname);
  rename(fname, oldname);

  if (in_save_list(zone_table[zone_num].number, SL_ZON))
    remove_from_save_list(zone_table[zone_num].number, SL_ZON);
  return TRUE;
}

/* Some common code to count the number of comands in the list. */
int count_commands(struct reset_com *list)
{
  int count = 0;

  while (list[count].command != 'S')
    count++;

  return count;
}

/* Adds a new reset command into a list.  Takes a pointer to the list so that
 * it may play with the memory locations. */
void add_cmd_to_list(struct reset_com **list, struct reset_com *newcmd, int pos)
{
  int count, i, l;
  struct reset_com *newlist;

  /* Count number of commands (not including terminator). */
  count = count_commands(*list);

  /* Value is +2 for the terminator and new field to add. */
  CREATE(newlist, struct reset_com, count + 2);

  /* Even tighter loop to copy the old list and insert a new command. */
  for (i = 0, l = 0; i <= count; i++) {
    newlist[i] = ((i == pos) ? *newcmd : (*list)[l++]);
  }

  /* Add terminator, then insert new list. */
  newlist[count + 1].command = 'S';
  free(*list);
  *list = newlist;
}

/* Remove a reset command from a list. Takes a pointer to the list so that it
 * may play with the memory locations. */
static void remove_cmd_from_list(struct reset_com **list, int pos)
{
  int count, i, l;
  struct reset_com *newlist;

  /* Count number of commands (not including terminator). */
  count = count_commands(*list);

  /* Value is 'count' because we didn't include the terminator above but since
   * we're deleting one thing anyway we want one less. */
  CREATE(newlist, struct reset_com, count);

  /* Even tighter loop to copy old list and skip unwanted command. */
  for (i = 0, l = 0; i < count; i++) {
    if (i != pos) {
      newlist[l++] = (*list)[i];
    }
  }
  /* Add the terminator, then insert the new list. */
  newlist[count - 1].command = 'S';
  free(*list);
  *list = newlist;
}

/* Error check user input and then add new (blank) command. */
int new_command(struct zone_data *zone, int pos)
{
  int subcmd = 0;
  struct reset_com new_com;

  /* Error check to ensure users hasn't given too large an index. */
  while (zone->cmd[subcmd].command != 'S')
    subcmd++;

  if (pos < 0 || pos > subcmd)
    return 0;

  /* Ok, let's add a new (blank) command. */
  new_com.command = 'N';
  add_cmd_to_list(&zone->cmd, &new_com, pos);
  return 1;
}

/* Error check user input and then remove command. */
void delete_zone_command(struct zone_data *zone, int pos)
{
  int subcmd = 0;

  /* Error check to ensure users hasn't given too large an index. */
  while (zone->cmd[subcmd].command != 'S')
    subcmd++;

  if (pos < 0 || pos >= subcmd)
    return;

  /* Ok, let's zap it. */
  remove_cmd_from_list(&zone->cmd, pos);
}
