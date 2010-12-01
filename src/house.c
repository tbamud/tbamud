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
#include "act.h"
#include "house.h"
#include "constants.h"
#include "modify.h"
#include "genolc.h"
#include "screen.h"

/* local globals */
struct house_control_data *house_control = NULL;

/* External Functions */
const char *get_spec_func_name(SPECIAL(*func));

/* local functions */
static int House_get_filename(room_vnum vnum, char *filename, size_t maxlen);
static int House_load(room_vnum vnum);
static void House_restore_weight(struct obj_data *obj);
static void hcontrol_build_house(struct char_data *ch, char *arg);
static void hcontrol_destroy_house(struct char_data *ch, char *arg);
static void hcontrol_pay_house(struct char_data *ch, char *arg);
static void House_listrent(struct char_data *ch, room_vnum vnum);

/* Control List Functions */

/* House loading */
static void parse_house_flags(struct house_control_data *h, char *line);
static void parse_house_guests(struct house_control_data *h, FILE *fl);

/* CONVERSION code starts here -- see comment below. */
static int ascii_convert_house(struct char_data *ch, obj_vnum vnum);
static void hcontrol_convert_houses(struct char_data *ch);
static struct obj_data *Obj_from_store(struct obj_file_elem object, int *location);
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

	for (current = loaded; current != NULL; current = current->next)
    obj_to_room(current->obj, rnum);

	/* now it's safe to free the obj_save_data list - all members of it
	 * have been put in the correct lists by obj_to_room()
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
 * call to House_restore_weight)  Assumes file is open already. */
int House_save(struct obj_data *obj, FILE *fp)
{
  struct house_control_data *hse;
  struct obj_data *tmp;
  int result;
  room_vnum rv;

  rv = world[(IN_ROOM(obj))].number;
  hse = find_house(rv);

  /* NORENT objects are skipped, unless house is flagged to save them */
  if ( obj && (!OBJ_FLAGGED(obj, ITEM_NORENT) || HOUSE_FLAGGED(hse, HOUSE_SAVENORENT)) ) {
    House_save(obj->contains, fp);
    House_save(obj->next_content, fp);
    result = objsave_save_obj_record(obj, fp, 0);
    if (!result)
      return (0);

    for (tmp = obj->in_obj; tmp; tmp = tmp->in_obj)
      GET_OBJ_WEIGHT(tmp) -= GET_OBJ_WEIGHT(obj);
  }
  return (1);
}

/* restore weight of containers after House_save has changed them for saving */
static void House_restore_weight(struct obj_data *obj)
{
  if (obj) {
    House_restore_weight(obj->contains);
    House_restore_weight(obj->next_content);
    if (obj->in_obj)
      GET_OBJ_WEIGHT(obj->in_obj) += GET_OBJ_WEIGHT(obj);
  }
}

/* Save all objects in a house */
void House_crashsave(room_vnum vnum)
{
  int rnum;
  char buf[MAX_STRING_LENGTH];
  FILE *fp;
  struct house_control_data *hse;

  hse = find_house(vnum);
  if (hse && HOUSE_FLAGGED(hse, HOUSE_NOSAVE))
    return;

  if ((rnum = real_room(vnum)) == NOWHERE)
    return;
  if (!House_get_filename(vnum, buf, sizeof(buf)))
    return;
  if (!(fp = fopen(buf, "wb"))) {
    perror("SYSERR: Error saving house file");
    return;
  }
  if (!House_save(world[rnum].contents, fp)) {
    fclose(fp);
    return;
  }
  fclose(fp);
  House_restore_weight(world[rnum].contents);
  REMOVE_BIT_AR(ROOM_FLAGS(rnum), ROOM_HOUSE_CRASH);
}

/* Delete a house save file */
void House_delete_file(room_vnum vnum)
{
  char filename[MAX_INPUT_LENGTH];
  FILE *fl;

  if (!House_get_filename(vnum, filename, sizeof(filename)))
    return;
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
  struct house_control_data *hse;
  char filename[MAX_STRING_LENGTH];
  char buf[MAX_STRING_LENGTH];
	obj_save_data *loaded, *current;
	int len = 0;

  hse = find_house(vnum);
  if (hse && HOUSE_FLAGGED(hse, HOUSE_RENTFREE)) {
    send_to_char(ch, "House #%d is rent free!\r\n", vnum);
  }

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
struct house_control_data *find_house(room_vnum vnum)
{
  struct house_control_data *h = NULL, *next_h = NULL;

  for (h = house_control; h; h = next_h) {
    next_h = h->next;
    if (h->vnum == vnum)
      return (h);
  }
  return (NULL);
}

void clear_house_control_data(struct house_control_data *h)
{
  int i;

  h->vnum         = NOWHERE;
  h->atrium       = NOWHERE;
  h->owner        = NOBODY;
  h->mode         = HOUSE_PRIVATE;
  h->exit_num     = 0;
  h->built_on     = 0;
  h->built_by     = NOBODY;
  h->last_payment = 0;
  h->receptionist = NOBODY;
  h->guests       = NULL;
  h->next         = NULL;

  for (i=0; i< HS_ARRAY_MAX; i++)
    h->house_flags[i] = 0;
}

/* Create a new house control struct with 'default' values, not in the list */
struct house_control_data *new_house(void)
{
  struct house_control_data *h;

  CREATE(h, struct house_control_data, 1);

  clear_house_control_data(h);

  return(h);
}

/* Add a guest ID to the specified house */
void add_house_guest(struct house_control_data *h_data, long guest_id)
{
  struct guest_data *g = NULL;

  if (!h_data || guest_id <= 0 || guest_id == NOBODY) return;

  CREATE(g, struct guest_data, 1);

  if (!g) {
    log("SYSERR: Unable to add house guest (vnum=%d, id=%ld)", h_data->vnum, guest_id);
  }
  /* Store the ID */
  g->id   = guest_id;

  /* add it onto the front of the list */
  g->next = h_data->guests;
  h_data->guests = g;
}

/* Return the number of house guests in the specified house */
int count_house_guests(struct house_control_data *h)
{
  int count = 0;
  struct guest_data *g, *next_g;

  for (g=h->guests, count=0; g; g = next_g) {
    next_g = g->next;
    count++;
  }
  return count;
}

/* Return TRUE if the specified player ID is in the house guest list */
bool is_house_guest(struct house_control_data *h, long id_num)
{
  int count = 0;
  struct guest_data *g, *next_g;

  for (g=h->guests, count=0; g; g = next_g) {
    next_g = g->next;
    if (g->id == id_num) return TRUE;
  }
  return FALSE;
}

/* remove a guest from the guest list for the house */
bool remove_house_guest(struct house_control_data *h, long id_num)
{
  int count = 0;
  struct guest_data *g, *next_g;

  /* First entry in the list? */
  g = (h->guests);
  if (g->id == id_num) {
    next_g = g->next;
	free (g);
	h->guests = next_g;
  }

  for (g=h->guests, count=0; g; g = next_g) {
    next_g = g->next;
    if (next_g->id == id_num) {
      g->next = next_g->next;
      free(next_g);
      return TRUE;
	}
  }
  return FALSE;
}

/* Return the number of houses in the list */
int count_houses(void)
{
  int count = 0;
  struct house_control_data *h, *next_h;

  for (h=house_control, count=0; h; h = next_h) {
    next_h = h->next;
    count++;
  }
  return count;
}

/* Add a house to the house_control list, allocating memory for it */
struct house_control_data *add_house(struct house_control_data *h_data)
{
  struct house_control_data *h;
  struct guest_data *g, *next_g = NULL;
  room_rnum real_house, real_atrium;

  /* Only valid houses can be added to the list, ignore all others */
  if (get_name_by_id(h_data->owner) == NULL)
    return NULL;			/* owner no longer exists -- skip */

  if ((real_house = real_room(h_data->vnum)) == NOWHERE)
    return NULL;			/* this vnum doesn't exist -- skip */

  if (find_house(h_data->vnum) != NULL)
    return NULL;			/* this vnum is already a house -- skip */

  if ((real_atrium = real_room(h_data->atrium)) == NOWHERE)
    return NULL;			/* house doesn't have an atrium -- skip */

  if (h_data->exit_num < 0 || h_data->exit_num >= DIR_COUNT)
    return NULL;			/* invalid exit num -- skip */

  if (TOROOM(real_house, h_data->exit_num) != real_atrium)
    return NULL;			/* exit num mismatch -- skip */

  /* All checks passed, add this house to the house_control list */
  h = new_house();

  /* Copy the data from the temp struct */
  *h = *h_data;

  /* Except the pointer to the guest list */
  h->guests = NULL;

  /* Then, add the guests */
  for (g=h_data->guests; g; g=next_g) {
    next_g = g->next;
    add_house_guest(h,g->id);
  }

  /* Finally, add it to the current list */
  h->next = house_control;
  house_control = h;

  return (h);
}

/* Set up a house with flags, contents and receptionist */
void set_house(struct house_control_data *h)
{
  struct char_data *mob = NULL;
  room_rnum real_house, real_atrium;
  mob_rnum real_mob;
  SPECIAL(*sp_func);

  if ((real_house = real_room(h->vnum)) == NOWHERE)
    return;			/* this vnum doesn't exist -- skip */

  if ((real_atrium = real_room(h->atrium)) == NOWHERE)
    return;			/* house doesn't have an atrium -- skip */

  /* House is OK, set flags and load contents */
  SET_BIT_AR(ROOM_FLAGS(real_house), ROOM_HOUSE);
  SET_BIT_AR(ROOM_FLAGS(real_house), ROOM_PRIVATE);
  SET_BIT_AR(ROOM_FLAGS(real_atrium), ROOM_ATRIUM);
  House_load(h->vnum);

  if ((real_mob = real_mobile(h->receptionist)) == NOBODY)
    return;  /* Invalid Receptionist Mob VNUM */

  /* Attach the required 'standard' Spec-Proc, unless flagged */
  if (!HOUSE_FLAGGED(h, HOUSE_NOSPEC)) {
    switch (h->mode) {
      case HOUSE_PRIVATE:
      case HOUSE_GOD:
        /* Load a standard receptionist */
        sp_func = house_receptionist;
        break;

      case HOUSE_SHOP:
        /* Load a Player-Owned Shopkeeper */
        sp_func = house_shopkeeper;
        break;

      default:
        sp_func = NULL;
        break;
    }

    /* NOTE: if the mob had a previous spec-proc assigned, it loses it */
    if (sp_func) {
      if ((mob_index[(real_mob)].func) && (sp_func != mob_index[(real_mob)].func)) {
        log("SYSERR: House %d, receptionist %d already had %s spec-proc in set_house",
             h->vnum, h->receptionist, get_spec_func_name(mob_index[(real_mob)].func));
	  }
      mob_index[(real_mob)].func = sp_func;
    }

    /* Finally, load the mob */
    mob = read_mobile(real_mob, REAL);
    char_to_room(mob, real_atrium);
  }
  House_crashsave(h->vnum);
}

/* Free memory used by house guest list for the specified house */
void free_house_guests(struct house_control_data *h)
{
  struct guest_data *g;

  while (h->guests) {
    g = (h->guests)->next;
    free(h->guests);
    h->guests = g;
  }
}

/* Free memory used by specifed house */
void free_house(struct house_control_data *h)
{
  /* Free guest list */
  if (h->guests)
    free_house_guests(h);

  /* Free house struct */
  free(h);
}

/* erase all houses, and free memory */
void free_house_list(void)
{
  struct house_control_data *h;

  while (house_control) {
    h = house_control->next;
    free_house(house_control);
    house_control = h;
  }
}

/* Remove a house from the control list, and free memory */
bool delete_house_control(struct house_control_data *to_del)
{
  struct house_control_data *h, *next_h = NULL;

  if (!house_control) return FALSE;

  /* Is it the first in the list */
  if (house_control == to_del) {
    h = house_control;
    house_control = h->next;
    free_house(h);
    return TRUE;
  }

  /* Find it in the list */
  for (h = house_control; h; h = next_h) {
    next_h = h->next;
    if (next_h == to_del) {
      h->next = next_h->next;
      free_house(next_h);
      return TRUE;
	}
  }

  return FALSE;
}

/* Save the house control information */
void House_save_control(void)
{
  FILE *fl;
  char bits[127], bits2[127], bits3[127], bits4[127];
  struct house_control_data *h, *next_h;
  struct guest_data *g, *next_g;

  if (!(fl = fopen(HCONTROL_FILE, "w"))) {
    perror("SYSERR: Unable to open house control file.");
    return;
  }
  for (h=house_control; h; h = next_h) {
    next_h = h->next;
    fprintf(fl, "#%d\n", h->vnum);                     /* New House, Room VNUM  */

    fprintf(fl, "Atrm: %d\n", h->atrium);              /* Atrium Room VNUM      */
    fprintf(fl, "ADir: %d\n", h->exit_num);            /* Atrium Direction      */

    fprintf(fl, "Bldr: %ld\n", h->built_by);           /* House Builder ID      */
    fprintf(fl, "BldT: %d\n", (int)(h->built_on));     /* House Build Date/Time */

    sprintascii(bits,  h->house_flags[0]);
    sprintascii(bits2, h->house_flags[1]);
    sprintascii(bits3, h->house_flags[2]);
    sprintascii(bits4, h->house_flags[3]);
    fprintf(fl, "Flgs: %s %s %s %s\n", bits, bits2, bits3, bits4); /* House Flags */

    if (h->guests) {
      fprintf(fl, "Gsts: \n");                         /* Guest List            */
      for (g=(h->guests); g; g = next_g) {
        next_g = g->next;
        fprintf(fl, "%ld\n", g->id);                   /* Individual Guest      */
      }
      fprintf(fl, "~\n");                              /* End Guest List        */
    }

    fprintf(fl, "Mode: %d\n", h->mode);                /* House Mode (Type)     */
    fprintf(fl, "Ownr: %ld\n", h->owner);              /* Owner ID Number       */
    fprintf(fl, "Paym: %d\n", (int)(h->last_payment)); /* Last Payment Date     */
    fprintf(fl, "Recp: %d\n", h->receptionist);        /* Receptionist VNUM     */
  }
  fprintf(fl, "$\n");  /* End of file */

  fclose(fl);
}

static void parse_house_flags(struct house_control_data *h, char *line)
{
  char f1[128], f2[128], f3[128], f4[128];

  if (sscanf(line, "%s %s %s %s", f1, f2, f3, f4) == 4) {
    h->house_flags[0] = asciiflag_conv(f1);
    h->house_flags[1] = asciiflag_conv(f2);
    h->house_flags[2] = asciiflag_conv(f3);
    h->house_flags[3] = asciiflag_conv(f4);
  } else
    h->house_flags[0] = asciiflag_conv(line);
}

static void parse_house_guests(struct house_control_data *h, FILE *fl)
{
  long num = 0;
  char line[MAX_INPUT_LENGTH + 1];

  do {
    get_line(fl, line);
    /* Add guest if line isn't end of list or a comment */
    if ((*line != '*') && (*line != '~')) {
      sscanf(line, "%ld", &num);
      if (num != 0)
	    add_house_guest(h, num);
    }
  } while (!feof(fl) && *line && *line != '~' && *line != '$' && num != 0);
}

/* Call from boot_db - will load control recs, load objs, set atrium bits.
 * Should do sanity checks on vnums & remove invalid records. */
void House_boot(void)
{
  struct house_control_data temp_house, *h;
  FILE *fl;
  char line[MAX_INPUT_LENGTH + 1], tag[6];;
  bool first = TRUE, bad_tag = FALSE;

  /* Clear any old lists in memory */
  if (house_control) free_house_list();

  if (!(fl = fopen(HCONTROL_FILE, "r"))) {
    if (errno == ENOENT)
      log("   No houses to load. File '%s' does not exist.", HCONTROL_FILE);
    else
      perror("SYSERR: " HCONTROL_FILE);
    return;
  }

  while (get_line(fl, line)) {
    while (*line == '*') {
      get_line(fl, line);  /* Skip comment lines */
    }
    /* Start a new house or end of file? */
    if (*line == '#' || *line == '$' || feof(fl)) {
      /* Save the current house */
      if (!first) {
        if ((h = add_house(&temp_house)) != NULL)
          set_house(h);  /* Set up the room flags, contents and spec mob */
      } else {
        first = FALSE;
      }
      if (*line == '$') {
        /* End of file reached, tidy up, and get out */
        fclose(fl);
        House_save_control();  /* Re-save, losing invalid houses */
        return;
      } else if (feof(fl)) {
        /* End of file incorrectly reached, warn, tidy up, and get out */
        log("SYSERR: End of house control file reached before '$'");
        fclose(fl);
        House_save_control();  /* Re-save, losing invalid houses */
        return;
      }
      /* We didn't reach the end of the file, so start a new house */
      clear_house_control_data(&temp_house);
      temp_house.vnum = atoi(line+1);
    }
    tag_argument(line, tag);

    switch (*tag) {
      case 'A':
             if (!strcmp(tag, "Atrm")) temp_house.atrium       = atoi(line);
        else if (!strcmp(tag, "ADir")) temp_house.exit_num     = atoi(line);
        else bad_tag = TRUE;
        break;

      case 'B':
             if (!strcmp(tag, "Bldr")) temp_house.built_by     = atol(line);
        else if (!strcmp(tag, "BldT")) temp_house.built_on     = atol(line);
        else bad_tag = TRUE;
        break;

      case 'F':
             if (!strcmp(tag, "Flgs")) parse_house_flags(&temp_house, line);
        else bad_tag = TRUE;
        break;

      case 'G':
             if (!strcmp(tag, "Gsts")) parse_house_guests(&temp_house, fl);
        else bad_tag = TRUE;
        break;

      case 'M':
             if (!strcmp(tag, "Mode")) temp_house.mode         = atoi(line);
        else bad_tag = TRUE;
        break;

      case 'O':
             if (!strcmp(tag, "Ownr")) temp_house.owner        = atol(line);
        else bad_tag = TRUE;
        break;

      case 'P':
             if (!strcmp(tag, "Paym")) temp_house.last_payment = atol(line);
        else bad_tag = TRUE;
        break;

      case 'R':
             if (!strcmp(tag, "Recp")) temp_house.receptionist = (mob_vnum)(atoi(line));
        else bad_tag = TRUE;
        break;

      default:
        bad_tag = TRUE;
        break;
    }

    if (bad_tag) {
      bad_tag = FALSE;
      log("SYSERR: Unknown tag %s in house control file %s", tag, HCONTROL_FILE);
	}
  }

  /* End of file incorrectly reached, warn, tidy up, and get out */
  log("SYSERR: End of house control file reached before '$'");
  fclose(fl);

  House_save_control();  /* Re-save, losing invalid houses */
}

void stat_house(struct char_data *ch, struct house_control_data *hse)
{
  room_rnum rm;
  char *timestr;
  char built_on[128], last_pay[128], buf[MAX_STRING_LENGTH];

  if (!ch || !hse) return;

  if (hse->built_on) {
    timestr = asctime(localtime(&(hse->built_on)));
    *(timestr + 10) = '\0';
    strlcpy(built_on, timestr, sizeof(built_on));
  } else
    strcpy(built_on, "Unknown");	/* strcpy: OK (for 'strlen("Unknown") < 128') */

  if (hse->last_payment) {
    timestr = asctime(localtime(&(hse->last_payment)));
    *(timestr + 10) = '\0';
    strlcpy(last_pay, timestr, sizeof(last_pay));
  } else
    strcpy(last_pay, "None");	/* strcpy: OK (for 'strlen("None") < 128') */

  send_to_char(ch, "House Type  : %s%d%s - %s%s%s\r\n",
                QYEL, hse->mode, QNRM,
                QCYN, house_types[(hse->mode)], QNRM);

  send_to_char(ch, "Owner       : %s[%s%5ld%s]%s - %s%s%s\r\n",
                QCYN, QYEL, hse->owner, QCYN, QNRM,
                QCYN, CAP(get_name_by_id(hse->owner)), QNRM);

  rm = real_room(hse->vnum);
  send_to_char(ch, "Private Room: %s[%s%5d%s]%s - %s%s%s\r\n",
                QCYN, QYEL, hse->vnum, QCYN, QNRM,
                QCYN, (rm == NOWHERE) ? "<Not Set>" : world[rm].name, QNRM);

  rm = real_room(hse->atrium);
  send_to_char(ch, "Atrium Room : %s[%s%5d%s]%s - %s%s%s\r\n",
                QCYN, QYEL, hse->atrium, QCYN, QNRM,
                QCYN, (rm == NOWHERE) ? "<Not Set>" : world[rm].name, QNRM);

  send_to_char(ch, "House Built : %s%s%s by %s%s%s\r\n",
                QYEL, built_on, QNRM,
                QCYN, (get_name_by_id(hse->built_by) == NULL) ? "<Unknown>" : CAP(get_name_by_id(hse->built_by)), QNRM);

  send_to_char(ch, "Last Payment: %s%s%s\r\n",
                QYEL, last_pay, QNRM);

  sprintbitarray((hse->house_flags), house_bits, HS_ARRAY_MAX, buf);
  send_to_char(ch, "House Flags : %s%s%s\r\n",
                QCYN, buf, QNRM);

  send_to_char(ch, "There are %d guests:\r\n", count_house_guests(hse));
  House_list_guests(ch, hse, TRUE);

  send_to_char(ch, "Rent Info:\r\n");
  House_listrent(ch, hse->vnum);
}

/* "House Control" functions */
const char *HCONTROL_FORMAT =
"Usage: hcontrol build <house vnum> <exit direction> <player name>\r\n"
"       hcontrol destroy <house vnum>\r\n"
"       hcontrol pay <house vnum>\r\n"
"       hcontrol show [house vnum | .]\r\n";

void hcontrol_list_houses(struct char_data *ch, char *arg)
{
  struct house_control_data *h = NULL, *next_h = NULL;
  char *timestr, *temp;
  char built_on[128], last_pay[128], own_name[MAX_NAME_LENGTH + 1];
  room_vnum toshow;

  if (arg && *arg) {
    if (*arg == '.')
      toshow = GET_ROOM_VNUM(IN_ROOM(ch));
    else
      toshow = atoi(arg);

    if ((h = find_house(toshow)) == NULL) {
      send_to_char(ch, "Unknown house, \"%s\".\r\n", arg);
      return;
    }
    stat_house(ch, h);
    return;
  }

  if (!house_control) {
    send_to_char(ch, "No houses have been defined.\r\n");
    return;
  }
  send_to_char(ch,
	"Address  Atrium  Build Date  Guests  Owner        Last Paymt\r\n"
	"-------  ------  ----------  ------  ------------ ----------\r\n");

  for (h = house_control; h; h = next_h) {
    next_h = h->next;

    /* Avoid seeing <UNDEF> entries from self-deleted people. -gg 6/21/98 */
    if ((temp = get_name_by_id(h->owner)) == NULL)
      continue;

    if (h->built_on) {
      timestr = asctime(localtime(&(h->built_on)));
      *(timestr + 10) = '\0';
      strlcpy(built_on, timestr, sizeof(built_on));
    } else
      strcpy(built_on, "Unknown");	/* strcpy: OK (for 'strlen("Unknown") < 128') */

    if (h->last_payment) {
      timestr = asctime(localtime(&(h->last_payment)));
      *(timestr + 10) = '\0';
      strlcpy(last_pay, timestr, sizeof(last_pay));
    } else
      strcpy(last_pay, "None");	/* strcpy: OK (for 'strlen("None") < 128') */

    /* Now we need a copy of the owner's name to capitalize. -gg 6/21/98 */
    strcpy(own_name, temp);	/* strcpy: OK (names guaranteed <= MAX_NAME_LENGTH+1) */
    send_to_char(ch, "%7d %7d  %-10s    %2d    %-12s %s\r\n",
	    h->vnum, h->atrium, built_on,
	    count_house_guests(h), CAP(own_name), last_pay);

    House_list_guests(ch, h, TRUE);
  }
}

static void hcontrol_build_house(struct char_data *ch, char *arg)
{
  char arg1[MAX_INPUT_LENGTH];
  struct house_control_data *temp_house, *h = NULL;
  room_vnum virt_house, virt_atrium;
  room_rnum real_house, real_atrium;
  sh_int exit_num;
  long owner;

  if (count_houses() >= MAX_HOUSES) {
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
  if ((find_house(virt_house)) != NULL) {
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
  /* allocate memory for temp_house */
  if ((temp_house = new_house()) == NULL) {
    send_to_char(ch, "Unable to build a house right now...\r\n");
    log("SYSERR: Unable to allocate memory for a house in hcontrol_build_house");
    return;
  }

  temp_house->mode         = HOUSE_PRIVATE;
  temp_house->vnum         = virt_house;
  temp_house->atrium       = virt_atrium;
  temp_house->exit_num     = exit_num;
  temp_house->built_on     = time(0);
  temp_house->last_payment = 0;
  temp_house->owner        = owner;
  temp_house->guests       = NULL;

  if ((h = add_house(temp_house)) != NULL) {
    set_house(h);
  } else {
    send_to_char(ch, "Unable to build a house right now...\r\n");
    log("SYSERR: Unable to add a house in hcontrol_build_house");
    free_house(temp_house);
    return;
  }

  /* A new house is added to the list, so free memory used by temp_house */
  free_house(temp_house);

  send_to_char(ch, "House built.  Mazel tov!\r\n");
  House_save_control();
}

static void hcontrol_destroy_house(struct char_data *ch, char *arg)
{
  struct house_control_data *h = NULL, *next_h = NULL;
  room_rnum real_atrium, real_house;

  if (!*arg) {
    send_to_char(ch, "%s", HCONTROL_FORMAT);
    return;
  }
  if ((h = find_house(atoi(arg))) == NULL) {
    send_to_char(ch, "Unknown house.\r\n");
    return;
  }
  if ((real_atrium = real_room(h->atrium)) == NOWHERE)
    log("SYSERR: House %d had invalid atrium %d!", atoi(arg), h->atrium);
  else
    REMOVE_BIT_AR(ROOM_FLAGS(real_atrium), ROOM_ATRIUM);

  if ((real_house = real_room(h->vnum)) == NOWHERE)
    log("SYSERR: House %d had invalid vnum %d!", atoi(arg), h->vnum);
  else {
    REMOVE_BIT_AR(ROOM_FLAGS(real_house), ROOM_HOUSE);
    REMOVE_BIT_AR(ROOM_FLAGS(real_house), ROOM_PRIVATE);
    REMOVE_BIT_AR(ROOM_FLAGS(real_house), ROOM_HOUSE_CRASH);
  }
  House_delete_file(h->vnum);

  delete_house_control(h);

  send_to_char(ch, "House deleted.\r\n");
  House_save_control();

  /* Now, reset the ROOM_ATRIUM flag on all existing houses' atriums, just in
   * case the house we just deleted shared an atrium with another house. -JE */
  for (h = house_control; h; h = next_h) {
    next_h = h->next;
    if ((real_atrium = real_room(h->atrium)) != NOWHERE)
      SET_BIT_AR(ROOM_FLAGS(real_atrium), ROOM_ATRIUM);
  }
}

static void hcontrol_pay_house(struct char_data *ch, char *arg)
{
  struct house_control_data *hse;

  if (!*arg)
    send_to_char(ch, "%s", HCONTROL_FORMAT);
  else if ((hse = find_house(atoi(arg))) == NULL)
    send_to_char(ch, "Unknown house.\r\n");
  else if (HOUSE_FLAGGED(hse, HOUSE_FREE))
    send_to_char(ch, "That is a FREE house. Payment cannot be taken.\r\n");
  else {
    mudlog(NRM, MAX(ADMLVL_IMMORT, GET_INVIS_LEV(ch)), TRUE, "Payment for house %s collected by %s.", arg, GET_NAME(ch));

    hse->last_payment = time(0);
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
  struct house_control_data *hse = NULL;
  char arg[MAX_INPUT_LENGTH];
  long id;

  one_argument(argument, arg);

  if (!ROOM_FLAGGED(IN_ROOM(ch), ROOM_HOUSE))
    send_to_char(ch, "You must be in your house to set guests.\r\n");
  else if ((hse = find_house(GET_ROOM_VNUM(IN_ROOM(ch)))) == NULL)
    send_to_char(ch, "Um.. this house seems to be screwed up.\r\n");
  else if (HOUSE_FLAGGED(hse, HOUSE_NOGUESTS))
    send_to_char(ch, "Sorry, this house can't have a guest list.\r\n");
  else if (GET_IDNUM(ch) != hse->owner)
    send_to_char(ch, "Only the primary owner can set guests.\r\n");
  else if (!*arg)
    House_list_guests(ch, hse, FALSE);
  else if ((id = get_id_by_name(arg)) < 0)
    send_to_char(ch, "No such player.\r\n");
  else if (id == GET_IDNUM(ch))
    send_to_char(ch, "It's your house!\r\n");
  else {
    if (is_house_guest(hse, id)) {
      remove_house_guest(hse, id);
      send_to_char(ch, "Guest deleted.\r\n");
    } else {
      if (count_house_guests(hse) == MAX_GUESTS) {
        send_to_char(ch, "You have too many guests.\r\n");
        return;
      }
      add_house_guest(hse, id);
      send_to_char(ch, "Guest added.\r\n");
    }
    House_save_control();
  }
}

/* Misc. administrative functions */
/* crash-save all the houses */
void House_save_all(void)
{
  struct house_control_data *hse = NULL, *next_hse = NULL;
  room_rnum real_house;

  for (hse = house_control; hse; hse = next_hse) {
    next_hse = hse->next;
    if ((real_house = real_room(hse->vnum)) != NOWHERE)
      if (ROOM_FLAGGED(real_house, ROOM_HOUSE_CRASH))
        House_crashsave(hse->vnum);
  }
}

/* note: arg passed must be house vnum, so there. */
int House_can_enter(struct char_data *ch, room_vnum house)
{
  struct house_control_data *hse = NULL;
  struct guest_data *g, *next_g = NULL;

  /* Not a house? */
  if ((hse = find_house(house)) == NULL)
    return (1);

  /* Even when admin-flagged able to enter all houses, there may be restrictions */
  if (ADM_FLAGGED(ch, ADM_ALLHOUSES)) {
    if (HOUSE_FLAGGED(hse, HOUSE_IMPONLY) && IS_ADMIN(ch, ADMLVL_IMPL))
      return (1);
    else if (IS_ADMIN(ch, ADMLVL_GOD))
      return (1);
    else if (!HOUSE_FLAGGED(hse, HOUSE_NOIMMS) && IS_ADMIN(ch, ADMLVL_IMMORT))
      return (1);
  }

  if (GET_IDNUM(ch) == hse->owner)
    return (1);

  switch (hse->mode) {
    case HOUSE_PRIVATE:
    case HOUSE_SHOP:
      for (g = hse->guests; g; g = next_g) {
        next_g = g->next;
        if (GET_IDNUM(ch) == g->id)
          return (1);
      }
      break;

    case HOUSE_GOD:
      for (g = hse->guests; g; g = next_g) {
        next_g = g->next;
        if (GET_IDNUM(ch) == g->id) {
          if (HOUSE_FLAGGED(hse, HOUSE_IMPONLY) && IS_ADMIN(ch, ADMLVL_IMPL))
            return (1);
          else if (IS_ADMIN(ch, ADMLVL_GOD))
            return (1);
          else if (!HOUSE_FLAGGED(hse, HOUSE_NOIMMS) && IS_ADMIN(ch, ADMLVL_IMMORT))
            return (1);
        }
      }
      break;

    default:
      log("SYSERR: Unhandled mode (%d) for house %d in House_can_enter", hse->mode, hse->vnum );
      break;
  }

  return (0);
}

void House_list_guests(struct char_data *ch, struct house_control_data *h, int quiet)
{
  struct guest_data *g, *next_g = NULL;
  int num_printed;
  char *temp;

  if (count_house_guests(h) == 0) {
    if (!quiet)
      send_to_char(ch, "  Guests: None\r\n");
    return;
  }

  send_to_char(ch, "  Guests: ");

  for (num_printed = 0, g = h->guests; g; g = next_g) {
    next_g = g->next;

    /* Avoid <UNDEF>. -gg 6/21/98 */
    if ((temp = get_name_by_id(g->id)) == NULL)
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
static void hcontrol_convert_houses(struct char_data *ch)
{
  struct house_control_data *h, *next_h = NULL;

	if (!IS_ADMIN(ch, ADMLVL_IMPL))
		{
			send_to_char(ch, "Sorry, but you are not powerful enough to do that.\r\n");
			return;
		}


  if (!house_control) {
    send_to_char(ch, "No houses have been defined.\r\n");
    return;
  }

	send_to_char(ch, "Converting houses:\r\n");

  for (h = house_control; h; h = next_h) {
    next_h = h->next;
    send_to_char(ch, "  %d", h->vnum);

    if (!ascii_convert_house(ch, h->vnum))
    {
      /* Let ascii_convert_house() tell about the error. */
      return;
    }
    else
    {
      send_to_char(ch, "...done\r\n");
    }
  }
  send_to_char(ch, "All done.\r\n");
}

static int ascii_convert_house(struct char_data *ch, obj_vnum vnum)
{
	FILE *in, *out;
	char infile[MAX_INPUT_LENGTH], *outfile;
	struct obj_data *tmp;
	int i, j=0, k;

  House_get_filename(vnum, infile, sizeof(infile));

	CREATE(outfile, char, strlen(infile)+7);
	sprintf(outfile, "%s.ascii", infile);

  if (!(in = fopen(infile, "r+b")))	/* no file found */
  {
  	send_to_char(ch, "...no object file found\r\n");
  	free(outfile);
    return (0);
  }

  if (!(out = fopen(outfile, "w")))
  {
  	send_to_char(ch, "...cannot open output file\r\n");
		free(outfile);
		fclose(in);
    return (0);
  }

  while (!feof(in)) {
    struct obj_file_elem object;
    k = fread(&object, sizeof(struct obj_file_elem), 1, in);
    if (ferror(in)) {
      perror("SYSERR: Reading house file in House_load");
      send_to_char(ch, "...read error in house rent file.\r\n");
      free(outfile);
      fclose(in);
      fclose(out);
      return (0);
    }
    if (!feof(in))
    {
    	tmp = Obj_from_store(object, &i);
      if (!objsave_save_obj_record(tmp, out, i))
      {
	      send_to_char(ch, "...write error in house rent file.\r\n");
	      free(outfile);
	      fclose(in);
	      fclose(out);
	      return (0);
      }
      j++;
    }
  }

	fprintf(out, "$~\n");

	fclose(in);
	fclose(out);

	free(outfile);

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

/* Standard House Receptionist (house guard mob)  */
/* Mob Special Function                           */
/* Prevents non-guests from entering a house      */
SPECIAL(house_receptionist)
{
  struct house_control_data *h, *next_h;
  struct char_data *guard = (struct char_data *) me;

  if (!IS_MOVE(cmd) || IS_AFFECTED(guard, AFF_BLIND))
    return FALSE;

  if (ADM_FLAGGED(ch, ADM_ALLHOUSES))
    return FALSE;

  /* Go through house list */
  for (h = house_control; h; h = next_h)
  {
    next_h = h->next;

    /* Is this the correct House Number, and moving in the right direction*/
    if ((world[(IN_ROOM(ch))].number == h->atrium) && IS_MOVE(cmd) && (cmd == rev_dir[(h->exit_num)]))
    {
      /* Always allow the house owner and guests */
      if ( (GET_IDNUM(ch) == h->owner) || (is_house_guest(h, GET_IDNUM(ch))) )
      {
         return FALSE;
      }
    }
  }


  /* Not house owner or guest - refuse admission */
  act("$N humiliates you, and blocks your way.",  FALSE, ch, 0, guard, TO_CHAR);
  act("$N humiliates $n, and blocks $s way.", FALSE, ch, 0, guard, TO_ROOM);

  return TRUE;
}

/* Player owned shops - Created by Jamdog - 22nd February 2007      */
/* Mob Special Function                                             */
/* 'Stock-room' must also be created and made into a player house   */
/* so that only the shop owner can get the piles of gold left there */
SPECIAL(house_shopkeeper)
{
  room_rnum private_room;
  struct obj_data *i, *j;
  int num=1, num_items=0;
  struct house_control_data *hse = NULL, *next_hse = NULL;
  char *temp, shop_owner[MAX_NAME_LENGTH + 1], buf[MAX_STRING_LENGTH];
  bool found=FALSE;

  if (!cmd)
    return FALSE;

  /* Gross. */
  private_room =  + 1;

  /* Grab the name of the shop owner */
  for (hse = house_control; hse && !found; hse = next_hse)
  {
    next_hse = hse->next;
    if (hse->atrium == world[(IN_ROOM(ch))].number)
    {
      /* Avoid seeing <UNDEF> entries from self-deleted people. */
      if ((temp = get_name_by_id(hse->owner)) == NULL)
      {
        sprintf(shop_owner, "Someone");
      }
      else
      {
        sprintf(shop_owner, "%s", CAP(get_name_by_id(hse->owner)) );
      }
      private_room = real_room(hse->vnum);
      found=TRUE;
    }
  }

  if (found==FALSE) {
    log("SYSERR: player_owned_shop spec_proc on a mob that's not in a house! (atrium room %d)", world[IN_ROOM(ch)].number);
    return FALSE;
  }

  if (private_room == NOWHERE) {
    log("SYSERR: player_owned_shop has invalid private room! (atrium room %d)", world[IN_ROOM(ch)].number);
    return FALSE;
  }

  if (CMD_IS("list"))
  {
    if (IS_NPC(ch))
    {
      send_to_char(ch, "Mobiles can't buy from a player-owned shop!\r\n");
      return TRUE;
    }
    sprintf(buf, "Owner: @W%s@n", shop_owner);
    send_to_char(ch, "Player-owned Shop %*s\r\n", count_color_chars(buf)+55, buf);
    send_to_char(ch, " ##   Available   Item                                               Cost\r\n");
    send_to_char(ch, "-------------------------------------------------------------------------\r\n");

    for (i = world[private_room].contents; i; i = i->next_content)
    {
      num_items = 0;
      for (j = world[private_room].contents; j != i; j = j->next_content)
        if (!strcmp(j->short_description, i->short_description))
        break;

      if (j != i)
        continue;

      for (j = i; j; j = j->next_content)
        if (!strcmp(j->short_description, i->short_description))
          num_items++;

      if (CAN_SEE_OBJ(ch, i) && (*i->description != '.' || ADM_FLAGGED(ch, ADM_SEESECRET)) && !(GET_OBJ_TYPE(i) == ITEM_MONEY))
      {
        send_to_char(ch, "%3d)  %5d      %-*s %11d\r\n", num++, num_items, count_color_chars(i->short_description)+44, i->short_description, GET_OBJ_COST(i));
      }
    }

    return (TRUE);
  } else if (CMD_IS("buy")) {

    skip_spaces(&argument);

    i = get_obj_in_list_vis(ch, argument, NULL, world[private_room].contents);

    if ((i == NULL) || (GET_OBJ_TYPE(i) == ITEM_MONEY))
    {
      send_to_char(ch, "There is no such item for sale!\r\n");
      return (TRUE);
    }
    if (GET_GOLD(ch) < GET_OBJ_COST(i))
    {
      send_to_char(ch, "You don't have enough gold!\r\n");
      return (TRUE);
    }

    /* Just to avoid crashes, if the object has no cost, then don't try to make a pile of no gold */
    if (GET_OBJ_COST(i) > 0)
    {
      /* Take gold from player */
      decrease_gold(ch, GET_OBJ_COST(i));

      /* Put gold in stock-room */
      j = create_money(GET_OBJ_COST(i));
      obj_to_room(j, private_room);
    }

    /* Move item from stock-room to player's inventory */
    obj_from_room(i);
    obj_to_char(i, ch);

    /* Let everyone know what's happening */
    send_to_char(ch, "%s hands you %s, and takes your payment.\r\n", CAP(GET_NAME((struct char_data *)me)), i->short_description);
    act("$n buys $p from $N.", FALSE, ch, i, (struct char_data *)me, TO_ROOM);
    send_to_char(ch, "%s thanks you for your custom, please come again!\r\n", shop_owner);

    return (TRUE);
  }
  return(FALSE);
}
