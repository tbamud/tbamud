/**************************************************************************
*  File: house.c                                           Part of tbaMUD *
*  Usage: Handling of player houses.                                      *
*                                                                         *
*  All rights reserved.  See license for complete information.            *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
**************************************************************************/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "handler.h"
#include "db.h"
#include "interpreter.h"
#include "house.h"
#include "constants.h"
#include "modify.h"

/* local (file scope only) globals */
static struct house_control_rec house_control[MAX_HOUSES];
static int num_of_houses = 0;

/* local functions */
static int House_get_filename(room_vnum vnum, char *filename, size_t maxlen);
static int House_load(room_vnum vnum);
static void House_restore_weight(struct obj_data *obj);
static void House_delete_file(room_vnum vnum);
static int find_house(room_vnum vnum);
static void House_save_control(void);
static void hcontrol_build_house(struct char_data *ch, char *arg);
static void hcontrol_destroy_house(struct char_data *ch, char *arg);
static void hcontrol_pay_house(struct char_data *ch, char *arg);
static void House_listrent(struct char_data *ch, room_vnum vnum);
/* CONVERSION code starts here -- see comment below. */
static int ascii_convert_house(struct char_data *ch, obj_vnum vnum);
static void hcontrol_convert_houses(struct char_data *ch);
static struct obj_data *Obj_from_store(struct obj_file_elem object, int *location);
static bool house_converted_this_run(room_vnum vnum);
static void house_forget_conversion(room_vnum vnum);
/* CONVERSION code ends here -- see comment below. */

/* First, the basics: finding the filename; loading/saving objects */
/* Return a filename given a house vnum */
static int House_get_filename(room_vnum vnum, char *filename, size_t maxlen)
{
  if (vnum == NOWHERE)
    return (0);

  snprintf(filename, maxlen, LIB_HOUSE"%d.house", vnum);
  return (1);
}

/* Where objsave_reassemble() puts a house's top-level objects. */
static void House_place(struct obj_data *obj, void *dest)
{
  obj_to_room(obj, *(room_rnum *) dest);
}

/* Load all objects for a house */
static int House_load(room_vnum vnum)
{
  FILE *fl;
  char filename[MAX_STRING_LENGTH];
	obj_save_data *loaded, *current;
  room_rnum rnum;

  if ((rnum = real_room(vnum)) == NOWHERE)
    return (0);
  if (!House_get_filename(vnum, filename, sizeof(filename)))
    return (0);
  if (!(fl = fopen(filename, "r")))	/* no file found */
    return (0);

	loaded = objsave_parse_objects(fl);

  /* Put each object back where the file says it was: inside its container,
   * or on the floor.  Every record used to go straight to the room, so a
   * bag stored in a house came back empty after a reboot with its contents
   * lying beside it -- and the file could not have said otherwise, because
   * House_save() never wrote a location either.  objsave_reassemble() reads
   * the locations Crash_load_objs() has always read for a rent file. */
  objsave_reassemble(loaded, House_place, &rnum);

	/* now it's safe to free the obj_save_data list - all members of it
	 * have been put in the correct lists by objsave_reassemble()
	 */
	while (loaded != NULL) {
		current = loaded;
		loaded = loaded->next;
		free(current);
	}

  fclose(fl);

  return (1);
}

/* Save all objects for a house (recursive; initial call must be followed by a
 * call to House_restore_weight)  Assumes file is open already.
 *
 * location is 0 for the floor and one lower for each level of container,
 * the same scheme Crash_save() writes and objsave_reassemble() reads.  It
 * used to be written as 0 for everything, so a house file recorded no
 * containment at all and every reboot spilled every container.
 *
 * The order matters as much as the number: siblings first, then contents,
 * then the object itself, so that an object's contents sit directly before
 * it in the file with nothing else at their depth in between.  Contents
 * first and then siblings -- the old order -- would have left a bag's
 * contents separated from the bag by the next bag's, and the reader would
 * have filed them into the wrong one. */
int House_save(struct obj_data *obj, FILE *fp, int location)
{
  struct obj_data *tmp;
  int result = TRUE;

  if (obj) {
    if (!House_save(obj->next_content, fp, location))
      result = FALSE;
    if (!House_save(obj->contains, fp, MIN(0, location) - 1))
      result = FALSE;
    if (!objsave_save_obj_record(obj, fp, location))
      result = FALSE;

    /*
     * Keep the temporary weight bookkeeping balanced even after a write
     * failure so House_restore_weight() can always restore the live objects.
     *
     * Only containers that track their contents have this weight to give
     * back, and only up to the first one that does not: a weight that never
     * climbed past a closed container is not in the containers above it
     * either.  Taking it out of them anyway is what wrote a house's unlimited
     * containers to disk lighter than they are.  House_restore_weight() below
     * covers exactly the same run, so the two stay exact inverses.
     */
    for (tmp = obj->in_obj; tmp && TRACKS_CONTENT_WEIGHT(tmp); tmp = tmp->in_obj)
      GET_OBJ_WEIGHT(tmp) -= GET_OBJ_WEIGHT(obj);
  }

  return result;
}

/* restore weight of containers after House_save has changed them for saving */
static void House_restore_weight(struct obj_data *obj)
{
  if (obj) {
    House_restore_weight(obj->contains);
    House_restore_weight(obj->next_content);
    if (obj->in_obj && TRACKS_CONTENT_WEIGHT(obj->in_obj))
      GET_OBJ_WEIGHT(obj->in_obj) += GET_OBJ_WEIGHT(obj);
  }
}

/* Save all objects in a house */
void House_crashsave(room_vnum vnum)
{
  int rnum, result;
  char buf[MAX_STRING_LENGTH];
  FILE *fp;

  if ((rnum = real_room(vnum)) == NOWHERE)
    return;

  /* CONVERSION code starts here -- see comment below. */
  /* A house converted this run is on disk and not in the room; writing the
   * room over it would undo the conversion.  The test belongs here rather
   * than in House_save_all(), because do_save() calls this too -- and a
   * house owner typing "save" is not a rare event. */
  if (house_converted_this_run(vnum))
    return;
  /* CONVERSION code ends here -- see comment below. */

  if (!House_get_filename(vnum, buf, sizeof(buf)))
    return;
  if (!(fp = fopen(buf, "wb"))) {
    perror("SYSERR: Error saving house file");
    return;
  }

  result = House_save(world[rnum].contents, fp, 0);
  House_restore_weight(world[rnum].contents);

  if (!result) {
    fclose(fp);
    return;
  }

  if (fflush(fp) == EOF || ferror(fp)) {
    log("SYSERR: Error finalizing house file %s.", buf);
    fclose(fp);
    return;
  }

  if (fclose(fp) == EOF) {
    log("SYSERR: Error closing house file %s.", buf);
    return;
  }

  REMOVE_BIT_AR(ROOM_FLAGS(rnum), ROOM_HOUSE_CRASH);
}

/* Delete a house save file */
static void House_delete_file(room_vnum vnum)
{
  char filename[MAX_INPUT_LENGTH], binname[MAX_INPUT_LENGTH + 8];
  FILE *fl;

  if (!House_get_filename(vnum, filename, sizeof(filename)))
    return;

  /* CONVERSION code starts here -- see comment below. */
  /* A converted house left its binary original beside the new file.  With
   * the house gone that copy is orphaned, and it would make the converter
   * refuse a later house built on the same vnum as already converted --
   * which would also stay out of saving for the rest of the run. */
  snprintf(binname, sizeof(binname), "%s.bin", filename);
  remove(binname);
  house_forget_conversion(vnum);
  /* CONVERSION code ends here -- see comment below. */

  if (!(fl = fopen(filename, "rb"))) {
    if (errno != ENOENT)
      log("SYSERR: Error deleting house file #%d. (1): %s", vnum, strerror(errno));
    return;
  }
  fclose(fl);
  if (remove(filename) < 0)
    log("SYSERR: Error deleting house file #%d. (2): %s", vnum, strerror(errno));
}

/* List all objects in a house file */
static void House_listrent(struct char_data *ch, room_vnum vnum)
{
  FILE *fl;
  char filename[MAX_STRING_LENGTH];
  char buf[MAX_STRING_LENGTH];
	obj_save_data *loaded, *current;
	int len = 0;

  if (!House_get_filename(vnum, filename, sizeof(filename)))
    return;
  if (!(fl = fopen(filename, "rb"))) {
    send_to_char(ch, "No objects on file for house #%d.\r\n", vnum);
    return;
  }
  *buf = '\0';
	len = snprintf(buf, sizeof(buf), "filename: %s\r\n", filename);

	loaded = objsave_parse_objects(fl);

	for (current = loaded; current != NULL; current = current->next)
	  len += snprintf(buf+len, sizeof(buf)-len, " [%5d] (%5dau) %s\r\n",
	    GET_OBJ_VNUM(current->obj), GET_OBJ_RENT(current->obj), current->obj->short_description);

	/* now it's safe to free the obj_save_data list - all members of it
	 * have been put in the correct lists by obj_to_room()
	 */
	while (loaded != NULL) {
		current = loaded;
		loaded = loaded->next;
		extract_obj(current->obj);
		free(current);
	}

	page_string(ch->desc,buf,0);
  fclose(fl);
}

/* Functions for house administration (creation, deletion, etc. */
static int find_house(room_vnum vnum)
{
  int i;

  for (i = 0; i < num_of_houses; i++)
    if (house_control[i].vnum == vnum)
      return (i);

  return (NOWHERE);
}

/* Save the house control information */
static void House_save_control(void)
{
  FILE *fl;
  /* Fixed at compile time: HCONTROL_FILE is a string literal and so is the
   * suffix.  objsave_open_tmp(), which the rest of this follows, measures
   * the name it builds because its path is made at runtime from a player
   * name; there is nothing here that could come out any other length. */
  static const char tempfile[] = HCONTROL_FILE ".tmp";

  /* Build the new file beside the old one and put it in place only once it
   * is whole.  Writing straight to HCONTROL_FILE truncated it
   * before a single record had reached the disk, so a save that failed
   * part-way left a truncated control file where a good one had been.
   * Where nothing at all reached the disk -- the common case, since the
   * records usually fit the stream buffer -- every house in it was gone
   * at the next boot; where the tear fell later, only the records past it
   * were lost.
   *
   * A full disk is the ordinary way in, and it does not fail where it
   * looks like it should: fopen("wb") only truncates, which costs no
   * blocks, and a set of records small enough to fit the stream buffer --
   * twenty-one houses at 192 bytes against the usual 4096 -- is copied
   * into it and reported written.  The failure appears at the fclose(),
   * whose result nothing looked at. */
  if (!(fl = fopen(tempfile, "wb"))) {
    perror("SYSERR: Unable to open the temporary house control file");
    return;
  }
  /* write all the house control recs in one fell swoop.  Pretty nifty, eh? */
  if (fwrite(house_control, sizeof(struct house_control_rec), num_of_houses, fl) != (size_t)num_of_houses) {
    perror("SYSERR: Unable to save house control file on write");
    fclose(fl);
    remove(tempfile);
    return;
  }
  if (fclose(fl)) {
    perror("SYSERR: Unable to save house control file on close");
    remove(tempfile);
    return;
  }

  /* Replace without deleting the old file first: a failed rename may be
   * caused by a locked temporary file, not by the destination existing.
   * The Windows CRT cannot replace an existing file with rename(). */
#ifdef CIRCLE_WINDOWS
  if (!MoveFileExA(tempfile, HCONTROL_FILE, MOVEFILE_REPLACE_EXISTING)) {
    log("SYSERR: Unable to put the house control file in place: Windows error %lu",
        (unsigned long)GetLastError());
#else
  if (rename(tempfile, HCONTROL_FILE)) {
    perror("SYSERR: Unable to put the house control file in place");
#endif
    remove(tempfile);
  }
}

/* Call from boot_db - will load control recs, load objs, set atrium bits. 
 * Should do sanity checks on vnums & remove invalid records. */
void House_boot(void)
{
  struct house_control_rec temp_house;
  room_rnum real_house, real_atrium;
  FILE *fl;

  memset((char *)house_control,0,sizeof(struct house_control_rec)*MAX_HOUSES);

  if (!(fl = fopen(HCONTROL_FILE, "rb"))) {
    if (errno == ENOENT)
      log("   No houses to load. File '%s' does not exist.", HCONTROL_FILE);
    else
      perror("SYSERR: " HCONTROL_FILE);
    return;
  }
  while (!feof(fl) && num_of_houses < MAX_HOUSES) {
    if (fread(&temp_house, sizeof(struct house_control_rec), 1, fl) != 1)
      break;

    if (feof(fl))
      break;

    if (get_name_by_id(temp_house.owner) == NULL)
      continue;			/* owner no longer exists -- skip */

    if ((real_house = real_room(temp_house.vnum)) == NOWHERE)
      continue;			/* this vnum doesn't exist -- skip */

    if (find_house(temp_house.vnum) != NOWHERE)
      continue;			/* this vnum is already a house -- skip */

    if ((real_atrium = real_room(temp_house.atrium)) == NOWHERE)
      continue;			/* house doesn't have an atrium -- skip */

    if (temp_house.exit_num < 0 || temp_house.exit_num >= DIR_COUNT)
      continue;			/* invalid exit num -- skip */

    if (TOROOM(real_house, temp_house.exit_num) != real_atrium)
      continue;			/* exit num mismatch -- skip */

    house_control[num_of_houses++] = temp_house;

    SET_BIT_AR(ROOM_FLAGS(real_house), ROOM_HOUSE);
    SET_BIT_AR(ROOM_FLAGS(real_house), ROOM_PRIVATE);
    SET_BIT_AR(ROOM_FLAGS(real_atrium), ROOM_ATRIUM);
    House_load(temp_house.vnum);
  }

  fclose(fl);
  House_save_control();
}

/* How many houses stand in this zone, or are entered from a room in it. Both
 * go the same way when the zone does: House_boot skips a house whose room or
 * atrium no longer resolves, so it drops out of house control at the next
 * save and the file holding its contents is left behind, orphaned. */
int House_count_in_zone(zone_rnum zone)
{
  int i, count = 0;
  room_rnum r;

  for (i = 0; i < num_of_houses; i++) {
    r = real_room(house_control[i].vnum);
    if (r != NOWHERE && world[r].zone == zone) {
      count++;
      continue;
    }
    r = real_room(house_control[i].atrium);
    if (r != NOWHERE && world[r].zone == zone)
      count++;
  }
  return (count);
}

/* "House Control" functions */
static const char *HCONTROL_FORMAT =
"Usage: hcontrol build <house vnum> <exit direction> <player name>\r\n"
"       hcontrol destroy <house vnum>\r\n"
"       hcontrol pay <house vnum>\r\n"
"       hcontrol show [house vnum | .]\r\n";

void hcontrol_list_houses(struct char_data *ch, char *arg)
{
  int i;
  char *temp;
  char built_on[128], last_pay[128], own_name[MAX_NAME_LENGTH + 1];

	if (arg && *arg) {
		room_vnum toshow;

		if (*arg == '.')
			toshow = GET_ROOM_VNUM(IN_ROOM(ch));
		else
			toshow = atoi(arg);

	  if ((i = find_house(toshow)) == NOWHERE) {
  	  send_to_char(ch, "Unknown house, \"%s\".\r\n", arg);
	    return;
	  }
		House_listrent(ch, toshow);
		return;
	}

  if (!num_of_houses) {
    send_to_char(ch, "No houses have been defined.\r\n");
    return;
  }
  send_to_char(ch,
	"Address  Atrium  Build Date       Guests  Owner        Last Paymt\r\n"
	"-------  ------  ---------------  ------  ------------ ---------------\r\n");

  for (i = 0; i < num_of_houses; i++) {
    /* Avoid seeing <UNDEF> entries from self-deleted people. -gg 6/21/98 */
    if ((temp = get_name_by_id(house_control[i].owner)) == NULL)
      continue;

    if (house_control[i].built_on) {
      strftime(built_on, sizeof(built_on), "%a %b %d %Y", localtime(&(house_control[i].built_on)));
    } else
      strcpy(built_on, "Unknown"); /* strcpy: OK */

    if (house_control[i].last_payment) {
      strftime(last_pay, sizeof(last_pay), "%a %b %d %Y", localtime(&(house_control[i].last_payment)));
    } else
      strcpy(last_pay, "None");	/* strcpy: OK */

    /* Now we need a copy of the owner's name to capitalize. -gg 6/21/98 */
    strcpy(own_name, temp);	/* strcpy: OK (names guaranteed <= MAX_NAME_LENGTH+1) */
    send_to_char(ch, "%7d %7d  %-15s    %2d    %-12s %s\r\n",
	    house_control[i].vnum, house_control[i].atrium, built_on,
	    house_control[i].num_of_guests, CAP(own_name), last_pay);

    House_list_guests(ch, i, TRUE);
  }
}

static void hcontrol_build_house(struct char_data *ch, char *arg)
{
  char arg1[MAX_INPUT_LENGTH];
  struct house_control_rec temp_house;
  room_vnum virt_house, virt_atrium;
  room_rnum real_house, real_atrium;
  sh_int exit_num;
  long owner;

  if (num_of_houses >= MAX_HOUSES) {
    send_to_char(ch, "Max houses already defined.\r\n");
    return;
  }

  /* first arg: house's vnum */
  arg = one_argument(arg, arg1);
  if (!*arg1) {
    send_to_char(ch, "%s", HCONTROL_FORMAT);
    return;
  }
  virt_house = atoi(arg1);
  if ((real_house = real_room(virt_house)) == NOWHERE) {
    send_to_char(ch, "No such room exists.\r\n");
    return;
  }
  if ((find_house(virt_house)) != NOWHERE) {
    send_to_char(ch, "House already exists.\r\n");
    return;
  }

  /* second arg: direction of house's exit */
  arg = one_argument(arg, arg1);
  if (!*arg1) {
    send_to_char(ch, "%s", HCONTROL_FORMAT);
    return;
  }
  if ((exit_num = search_block(arg1, dirs, FALSE)) < 0) {
    send_to_char(ch, "'%s' is not a valid direction.\r\n", arg1);
    return;
  }
  if (TOROOM(real_house, exit_num) == NOWHERE) {
    send_to_char(ch, "There is no exit %s from room %d.\r\n", dirs[exit_num], virt_house);
    return;
  }

  real_atrium = TOROOM(real_house, exit_num);
  virt_atrium = GET_ROOM_VNUM(real_atrium);

  if (TOROOM(real_atrium, rev_dir[exit_num]) != real_house) {
    send_to_char(ch, "A house's exit must be a two-way door.\r\n");
    return;
  }

  /* third arg: player's name */
  one_argument(arg, arg1);
  if (!*arg1) {
    send_to_char(ch, "%s", HCONTROL_FORMAT);
    return;
  }
  if ((owner = get_id_by_name(arg1)) < 0) {
    send_to_char(ch, "Unknown player '%s'.\r\n", arg1);
    return;
  }

  temp_house.mode = HOUSE_PRIVATE;
  temp_house.vnum = virt_house;
  temp_house.atrium = virt_atrium;
  temp_house.exit_num = exit_num;
  temp_house.built_on = time(0);
  temp_house.last_payment = 0;
  temp_house.owner = owner;
  temp_house.num_of_guests = 0;

  house_control[num_of_houses++] = temp_house;

  SET_BIT_AR(ROOM_FLAGS(real_house), ROOM_HOUSE);
  SET_BIT_AR(ROOM_FLAGS(real_house), ROOM_PRIVATE);
  SET_BIT_AR(ROOM_FLAGS(real_atrium), ROOM_ATRIUM);
  House_crashsave(virt_house);

  send_to_char(ch, "House built.  Mazel tov!\r\n");
  House_save_control();
}

static void hcontrol_destroy_house(struct char_data *ch, char *arg)
{
  int i, j;
  room_rnum real_atrium, real_house;

  if (!*arg) {
    send_to_char(ch, "%s", HCONTROL_FORMAT);
    return;
  }
  if ((i = find_house(atoi(arg))) == NOWHERE) {
    send_to_char(ch, "Unknown house.\r\n");
    return;
  }
  if ((real_atrium = real_room(house_control[i].atrium)) == NOWHERE)
    log("SYSERR: House %d had invalid atrium %d!", atoi(arg), house_control[i].atrium);
  else
    REMOVE_BIT_AR(ROOM_FLAGS(real_atrium), ROOM_ATRIUM);

  if ((real_house = real_room(house_control[i].vnum)) == NOWHERE)
    log("SYSERR: House %d had invalid vnum %d!", atoi(arg), house_control[i].vnum);
  else {
    REMOVE_BIT_AR(ROOM_FLAGS(real_house), ROOM_HOUSE);
    REMOVE_BIT_AR(ROOM_FLAGS(real_house), ROOM_PRIVATE);
    REMOVE_BIT_AR(ROOM_FLAGS(real_house), ROOM_HOUSE_CRASH);
  }
  House_delete_file(house_control[i].vnum);

  for (j = i; j < num_of_houses - 1; j++)
    house_control[j] = house_control[j + 1];

  num_of_houses--;

  send_to_char(ch, "House deleted.\r\n");
  House_save_control();

  /* Now, reset the ROOM_ATRIUM flag on all existing houses' atriums, just in 
   * case the house we just deleted shared an atrium with another house. -JE */
  for (i = 0; i < num_of_houses; i++)
    if ((real_atrium = real_room(house_control[i].atrium)) != NOWHERE)
      SET_BIT_AR(ROOM_FLAGS(real_atrium), ROOM_ATRIUM);
}

static void hcontrol_pay_house(struct char_data *ch, char *arg)
{
  int i;

  if (!*arg)
    send_to_char(ch, "%s", HCONTROL_FORMAT);
  else if ((i = find_house(atoi(arg))) == NOWHERE)
    send_to_char(ch, "Unknown house.\r\n");
  else {
    mudlog(NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE, "Payment for house %s collected by %s.", arg, GET_NAME(ch));

    house_control[i].last_payment = time(0);
    House_save_control();
    send_to_char(ch, "Payment recorded.\r\n");
  }
}

/* The hcontrol command itself, used by imms to create/destroy houses */
ACMD(do_hcontrol)
{
  char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

  half_chop(argument, arg1, arg2);

  if (is_abbrev(arg1, "build"))
    hcontrol_build_house(ch, arg2);
  else if (is_abbrev(arg1, "destroy"))
    hcontrol_destroy_house(ch, arg2);
  else if (is_abbrev(arg1, "pay"))
    hcontrol_pay_house(ch, arg2);
  else if (is_abbrev(arg1, "show"))
    hcontrol_list_houses(ch, arg2);
/* CONVERSION code starts here -- see comment below not in hcontrol_format. */
	else if (!str_cmp(arg1, "asciiconvert"))
    hcontrol_convert_houses(ch);
/* CONVERSION ends here -- read more below. */
  else
    send_to_char(ch, "%s", HCONTROL_FORMAT);
}

/* The house command, used by mortal house owners to assign guests */
ACMD(do_house)
{
  char arg[MAX_INPUT_LENGTH];
  int i, j, id;

  one_argument(argument, arg);

  if (!ROOM_FLAGGED(IN_ROOM(ch), ROOM_HOUSE))
    send_to_char(ch, "You must be in your house to set guests.\r\n");
  else if ((i = find_house(GET_ROOM_VNUM(IN_ROOM(ch)))) == NOWHERE)
    send_to_char(ch, "Um.. this house seems to be screwed up.\r\n");
  else if (GET_IDNUM(ch) != house_control[i].owner)
    send_to_char(ch, "Only the primary owner can set guests.\r\n");
  else if (!*arg)
    House_list_guests(ch, i, FALSE);
  else if ((id = get_id_by_name(arg)) < 0)
    send_to_char(ch, "No such player.\r\n");
  else if (id == GET_IDNUM(ch))
    send_to_char(ch, "It's your house!\r\n");
  else {
    for (j = 0; j < house_control[i].num_of_guests; j++)
      if (house_control[i].guests[j] == id) {
	for (; j < house_control[i].num_of_guests; j++)
	  house_control[i].guests[j] = house_control[i].guests[j + 1];
	house_control[i].num_of_guests--;
	House_save_control();
	send_to_char(ch, "Guest deleted.\r\n");
	return;
      }
    if (house_control[i].num_of_guests == MAX_GUESTS) {
      send_to_char(ch, "You have too many guests.\r\n");
      return;
    }
    j = house_control[i].num_of_guests++;
    house_control[i].guests[j] = id;
    House_save_control();
    send_to_char(ch, "Guest added.\r\n");
  }
}

/* Misc. administrative functions */
/* crash-save all the houses */
void House_save_all(void)
{
  int i;
  room_rnum real_house;

  for (i = 0; i < num_of_houses; i++)
    if ((real_house = real_room(house_control[i].vnum)) != NOWHERE)
      if (ROOM_FLAGGED(real_house, ROOM_HOUSE_CRASH))
	House_crashsave(house_control[i].vnum);
}

/* note: arg passed must be house vnum, so there. */
int House_can_enter(struct char_data *ch, room_vnum house)
{
  int i, j;

  if (GET_LEVEL(ch) >= LVL_GRGOD || (i = find_house(house)) == NOWHERE)
    return (1);

  switch (house_control[i].mode) {
  case HOUSE_PRIVATE:
    if (GET_IDNUM(ch) == house_control[i].owner)
      return (1);
    for (j = 0; j < house_control[i].num_of_guests; j++)
      if (GET_IDNUM(ch) == house_control[i].guests[j])
	return (1);
  }

  return (0);
}

void House_list_guests(struct char_data *ch, int i, int quiet)
{
  int j, num_printed;
  char *temp;

  if (house_control[i].num_of_guests == 0) {
    if (!quiet)
      send_to_char(ch, "  Guests: None\r\n");
    return;
  }

  send_to_char(ch, "  Guests: ");

  for (num_printed = j = 0; j < house_control[i].num_of_guests; j++) {
    /* Avoid <UNDEF>. -gg 6/21/98 */
    if ((temp = get_name_by_id(house_control[i].guests[j])) == NULL)
      continue;

    num_printed++;
    send_to_char(ch, "%c%s ", UPPER(*temp), temp + 1);
  }

  if (num_printed == 0)
    send_to_char(ch, "all dead");

  send_to_char(ch, "\r\n");
}

/*************************************************************************
 * All code below this point and the code above, marked "CONVERSION"     *
 * can be removed after you have converted your house rent files using   *
 * the command                                                           *
 *   hcontrol asciiconvert                                               *
 *                                                                       *
 * You can only use this command as implementor.                         *
 * After you have converted your house files, I suggest a reboot, which  *
 * will let your house files load on the next bootup. -Welcor            *
 ************************************************************************/
/* Code for conversion to ascii house rent files. */

/* Houses converted since this boot.  For these the file is the house and
 * the room is not, so House_crashsave() leaves them alone; see the comment
 * where they are recorded. */
static room_vnum *converted_houses = NULL;
static int num_converted_houses = 0;

static bool house_converted_this_run(room_vnum vnum)
{
  int i;

  for (i = 0; i < num_converted_houses; i++)
    if (converted_houses[i] == vnum)
      return (TRUE);

  return (FALSE);
}

static void house_forget_conversion(room_vnum vnum)
{
  int i;

  for (i = 0; i < num_converted_houses; i++)
    if (converted_houses[i] == vnum) {
      converted_houses[i] = converted_houses[--num_converted_houses];
      return;
    }
}

static void hcontrol_convert_houses(struct char_data *ch)
{
  int i, failed = 0;

	if (GET_LEVEL(ch) < LVL_IMPL)
		{
			send_to_char(ch, "Sorry, but you are not powerful enough to do that.\r\n");
			return;
		}


  if (!num_of_houses) {
    send_to_char(ch, "No houses have been defined.\r\n");
    return;
  }

	send_to_char(ch, "Converting houses:\r\n");

  for (i = 0; i < num_of_houses; i++) {
	  send_to_char(ch, "  %d", house_control[i].vnum);

	  /* One house's failure is its own.  ascii_convert_house() has said what
	   * went wrong and left that house's files as it found them; stopping
	   * here would leave every house after it unconverted for a reason that
	   * has nothing to do with them. */
	  if (!ascii_convert_house(ch, house_control[i].vnum))
	  {
	  	failed++;
	  }
	  else
	  {
	  	send_to_char(ch, "...done\r\n");
	  }
  }

  if (failed)
	send_to_char(ch, "All done, except for %d house%s left unconverted; "
	                 "see the errors above.\r\n",
	             failed, failed == 1 ? "" : "s");
  else
	send_to_char(ch, "All done.\r\n");

  if (num_converted_houses)
	send_to_char(ch, "Converted houses are not saved again until a reboot, "
	                 "which loads them.\r\n");
}

static int ascii_convert_house(struct char_data *ch, obj_vnum vnum)
{
	FILE *in, *out;
	char infile[MAX_INPUT_LENGTH], backup[MAX_INPUT_LENGTH + 8], *outfile;
	char probe[32], *q;
	struct obj_file_elem object;
	struct obj_data *tmp;
	struct stat backup_st;
	long len;
	int i, j=0, skipped=0;

  House_get_filename(vnum, infile, sizeof(infile));
	snprintf(backup, sizeof(backup), "%s.bin", infile);

	/* A house whose binary original is already set aside has been through
	 * this once.  Running the command again would read the converted ascii
	 * file as though it were binary and rename the result over that
	 * original -- the one copy of the house that cannot be rebuilt.
	 *
	 * Ask stat() rather than opening it.  Opening tells us less and costs
	 * more: a named pipe left at this name answers open(O_RDONLY) by
	 * waiting for a writer that never comes, which stops the whole MUD,
	 * and a directory there answers it successfully, which would report a
	 * house converted that is still binary.  Only a regular file is a
	 * backup. */
	if (stat(backup, &backup_st) == 0 && S_ISREG(backup_st.st_mode))
	{
		/* No newline: hcontrol_convert_houses() writes "...done" after any
		 * non-zero return, and the whole line is one house. */
		send_to_char(ch, "...already converted");
		return (1);
	}

	CREATE(outfile, char, strlen(infile)+7);
	sprintf(outfile, "%s.ascii", infile);

  if (!(in = fopen(infile, "r+b")))	/* no file found */
  {
  	/* No rent file at all is unusual -- one deleted by hand, or a
  	 * control file brought in without them.  hcontrol build writes a
  	 * zero-byte one the moment a house is created, so a house nobody
  	 * has stored anything in still has a file.  Either way there is
  	 * nothing to convert and nothing wrong.  A file that is there and
  	 * will not open is a different thing and has to be said. */
  	if (errno != ENOENT)
  	{
  	  send_to_char(ch, "...cannot open the rent file: %s\r\n",
  	               strerror(errno));
  	  free(outfile);
  	  return (0);
  	}
  	send_to_char(ch, "...no rent file");
  	free(outfile);
    return (1);
  }

	/* House_crashsave() writes the ascii format to this very name, so for any
	 * house saved since ascii object files arrived, the file this command
	 * is pointed at is ascii already -- not converted, but
	 * overwritten with whatever the room held at the time, the binary
	 * contents gone with it.  Read as 72-byte binary records it yields
	 * vnums that mostly resolve to nothing, so what is written in its place
	 * bears no relation to the house: at best it is emptied, and the
	 * command reports success.
	 *
	 * An ascii object file opens with '#' and a vnum alone on the line.
	 * The line after it is blank for any object with nothing altered from
	 * its prototype: House_crashsave() calls House_save() with a locate
	 * of 0, so objsave_save_obj_record() omits its "Loc :" line, and such
	 * an object has no other tag to write either.  It emits the vnum and
	 * then the blank line that closes every record.  Failing that it is one
	 * of that function's tags, which are four characters and a colon.  The
	 * '#' and '$~' the test also accepts cannot stand there in a file this
	 * codebase wrote, since a record always closes with its blank line
	 * first; they are allowed for a file that came from somewhere else.
	 *
	 * A binary record can reach the first line by chance -- item_number
	 * 0x3023, 0x3123 and so on to 0x3923 put '#' and a digit in the
	 * first two bytes, and a location of 10 puts a newline in the third
	 * -- so the second line is checked too.  10 is a wear position, and
	 * the high half of a wear position is a zero byte, which is none of
	 * the alternatives, so such a record still converts.  Nothing else
	 * in the record ends the first line where the test expects: fgets()
	 * stops at '\n' and not at a carriage return, so a location of 13
	 * sends the read on into the record's other bytes, and what the
	 * second line then holds is arbitrary -- converted, almost always,
	 * which is the safe direction.  Note which way the two mistakes fall:
	 * reading an ascii file as binary empties the house, while reading a
	 * binary file as ascii only refuses to convert it.  The test belongs
	 * on the loose side of that. */
	/* Measure before reading anything.  fseek() on a stream that is not
	 * seekable fails without touching it, where a read would block: a
	 * named pipe left at this path answers open(O_RDWR) at once and then
	 * never reaches end of file, because this process holds the write
	 * end itself.  Refusing here costs a converted house nothing and
	 * keeps the probe below from hanging the MUD on one. */
	if (fseek(in, 0L, SEEK_END) != 0)
	{
	  send_to_char(ch, "...nothing to convert");
	  fclose(in);
	  free(outfile);
	  return (1);
	}
	len = ftell(in);
	rewind(in);

	if (fgets(probe, sizeof(probe), in) != NULL && *probe == '#')
	{
		/* Cast: this is deliberately reading a file that may be binary, and
		 * isdigit() of a negative char is undefined where char is signed. */
		for (q = probe + 1; isdigit((unsigned char)*q); q++);
		if (q > probe + 1 && (*q == '\n' || *q == '\r') &&
		    fgets(probe, sizeof(probe), in) != NULL &&
		    (*probe == '#' || *probe == '$' ||
		     *probe == '\n' || *probe == '\r' ||
		     (strlen(probe) > 4 && probe[4] == ':')))
		{
			send_to_char(ch, "...already in the ascii format; nothing to convert");
			fclose(in);
			free(outfile);
			return (1);
		}
	}
	rewind(in);

	/* Below the probe, so a real ascii house shorter than one record is
	 * still told apart from a binary one.  What is left here is a file
	 * too short to hold a 72-byte record and not recognisable as ascii:
	 * House_crashsave() writes no terminator, so a house whose room is
	 * empty is saved as a file of no length; this function itself writes
	 * "$~" alone, three bytes, for a house whose every record was skipped
	 * for want of a prototype; and a binary file can be left part-way
	 * through its first record.  Read as binary none of them is a record
	 * at all, and a "$~" would be installed over the file and the house
	 * taken out of saving for the rest of the run. */
	if (len < (long) sizeof(struct obj_file_elem))
	{
	  send_to_char(ch, "...nothing to convert");
	  fclose(in);
	  free(outfile);
	  return (1);
	}

  /* The scratch name is ordinarily absent, so stat() failing is the
   * common case and not an error.  What it rules out is something at that
   * name that is not a file: opening a named pipe for writing waits for a
   * reader that never comes, and this sweep now visits every house, so
   * one left anywhere under lib/house would stop the MUD rather than one
   * house.  A regular file is safe to truncate. */
  if (stat(outfile, &backup_st) == 0 && !S_ISREG(backup_st.st_mode))
  {
    send_to_char(ch, "...output name is not a file\r\n");
    free(outfile);
    fclose(in);
    return (0);
  }

  if (!(out = fopen(outfile, "w")))
  {
  	send_to_char(ch, "...cannot open output file\r\n");
		free(outfile);
		fclose(in);
    return (0);
  }

  /* Running out of file is how this loop is meant to end -- the record
   * count is not stored anywhere.  Returning on the short read at the end
   * left every conversion short of the "$~" terminator its output needs,
   * with both files still open and the caller told the house had failed,
   * which stopped it before the second house.  Tell the two apart after
   * the loop instead: an error is an error, the end of the file is not. */
  while (fread(&object, sizeof(struct obj_file_elem), 1, in) == 1)
  {
    tmp = Obj_from_store(object, &i);
    /* The item's prototype may have been deleted since the house was
     * last saved, and then Obj_from_store() has nothing to build and
     * returns NULL.  objsave_save_obj_record() reads the object it is
     * given straight away, without looking. */
    if (tmp == NULL) {
      skipped++;
      continue;
    }
    if (!objsave_save_obj_record(tmp, out, i))
    {
      send_to_char(ch, "...write error in house rent file.\r\n");
      /* Built by Obj_from_store() above and not written out, so it is
       * still in object_list and still counted against its prototype. */
      extract_obj(tmp);
      fclose(in);
      fclose(out);
      /* After the close: Windows will not remove a file that is open,
       * and the paths below already do it in this order. */
      remove(outfile);
      free(outfile);
      return (0);
    }
    /* Obj_from_store() builds the object with read_object(), which puts
     * it in object_list and counts it against the prototype.  It exists
     * only to be written out, so it goes again once it has been. */
    extract_obj(tmp);
    j++;
  }

  if (ferror(in)) {
    perror("SYSERR: Reading house file in House_load");
    send_to_char(ch, "...read error in house rent file.\r\n");
    fclose(in);
    fclose(out);
    remove(outfile);
    free(outfile);
    return (0);
  }

  /* A file that stops part-way through a record is not an error to stdio,
   * and the bytes are unusable either way -- but this is a one-way
   * migration, so say that something was dropped rather than report a
   * clean conversion. */
  if (ftell(in) % (long) sizeof(struct obj_file_elem) != 0)
    send_to_char(ch, "\r\n...rent file ends part-way through a record; the tail was skipped\r\n");

	fprintf(out, "$~\n");

	fclose(in);

	/* Everything written above may still be in the stream's buffer: a
	 * house's worth of objects rarely fills one, so objsave_save_obj_record()
	 * reports success for every record and the write only reaches the disk
	 * here.  An unchecked close would hand back a converted file that is
	 * empty, and say "...%d items" over it. */
	if (fclose(out))
	{
		send_to_char(ch, "...write error saving the converted file.\r\n");
		remove(outfile);
		free(outfile);
		return (0);
	}

	/* The converted file has to take the place of the one it was made
	 * from, or nothing will ever read it: House_load() opens <vnum>.house
	 * and only that.  Keep the original beside it as <vnum>.house.bin, so
	 * a conversion that turned out badly can be undone by moving one file
	 * back. */
	if (rename(infile, backup))
	{
		send_to_char(ch, "...cannot set the original aside; left it alone\r\n");
		remove(outfile);
		free(outfile);
		return (0);
	}
	if (rename(outfile, infile))
	{
		send_to_char(ch, "...cannot put the converted file in place\r\n");
		rename(backup, infile);
		remove(outfile);
		free(outfile);
		return (0);
	}

	free(outfile);

	/* The file is the house now; the room is not.  The room was loaded at
	 * boot from the binary file, which parsed to nothing, so it holds what
	 * the zone stocked and anything dropped in it since -- and
	 * House_crashsave() writes that back over the file, from
	 * House_save_all() on every autosave, at shutdown and on saveall for
	 * any house room an object has come or gone in, which the zone reset
	 * alone is enough to cause, and from do_save() whenever the room is
	 * flagged.  So the
	 * conversion would be undone by the next save, minutes later, having
	 * reported success.
	 *
	 * Loading the file into the room instead would double every object the
	 * two have in common -- a house saved from a room the zone stocks holds
	 * that stock, and the zone has stocked it again since boot.  So the
	 * house is left out of the saving instead, until the reboot this
	 * command's own comment recommends, which loads the file properly. */
	RECREATE(converted_houses, room_vnum, num_converted_houses + 1);
	converted_houses[num_converted_houses++] = vnum;

	/* An operator running a one-way migration should hear what it left
	 * behind, not just what it carried over. */
	if (skipped)
		send_to_char(ch, "...%d items, %d skipped (no prototype)", j, skipped);
	else
		send_to_char(ch, "...%d items", j);
	return 1;
}

/* The circle 3.1 function for reading rent files. No longer used by the rent system. */
static struct obj_data *Obj_from_store(struct obj_file_elem object, int *location)
{
  struct obj_data *obj;
  obj_rnum itemnum;
  int j, taeller;

  *location = 0;
  if ((itemnum = real_object(object.item_number)) == NOTHING)
    return (NULL);

  obj = read_object(itemnum, REAL);
#if USE_AUTOEQ
  *location = object.location;
#endif
  GET_OBJ_VAL(obj, 0) = object.value[0];
  GET_OBJ_VAL(obj, 1) = object.value[1];
  GET_OBJ_VAL(obj, 2) = object.value[2];
  GET_OBJ_VAL(obj, 3) = object.value[3];
  for(taeller = 0; taeller < EF_ARRAY_MAX; taeller++)
    GET_OBJ_EXTRA(obj)[taeller] = object.extra_flags[taeller];
  GET_OBJ_WEIGHT(obj) = object.weight;
  GET_OBJ_TIMER(obj) = object.timer;
  for(taeller = 0; taeller < AF_ARRAY_MAX; taeller++)
    GET_OBJ_AFFECT(obj)[taeller] = object.bitvector[taeller];

  for (j = 0; j < MAX_OBJ_AFFECT; j++)
    obj->affected[j] = object.affected[j];

  return (obj);
}
