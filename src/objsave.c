/**************************************************************************
*  File: objsave.c                                         Part of tbaMUD *
*  Usage: loading/saving player objects for rent and crash-save           *
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
#include "spells.h"
#include "act.h"
#include "class.h"
#include "config.h"
#include "modify.h"
#include "genolc.h" /* for strip_cr and sprintascii */

/* these factors should be unique integers */
#define RENT_FACTOR    1
#define CRYO_FACTOR    4

#define LOC_INVENTORY  0
#define MAX_BAG_ROWS   5

/* local functions */
static int Crash_save(struct obj_data *obj, FILE *fp, int location);
static void Crash_extract_norent_eq(struct char_data *ch);
static void auto_equip(struct char_data *ch, struct obj_data *obj, int location);
static int Crash_offer_rent(struct char_data *ch, struct char_data *receptionist, int display, int factor);
static int Crash_report_unrentables(struct char_data *ch, struct char_data *recep, struct obj_data *obj);
static void Crash_report_rent(struct char_data *ch, struct char_data *recep, struct obj_data *obj, long *cost, long *nitems, int display, int factor);
static int gen_receptionist(struct char_data *ch, struct char_data *recep, int cmd, char *arg, int mode);
static void Crash_rent_deadline(struct char_data *ch, struct char_data *recep, long cost);
static void Crash_restore_weight(struct obj_data *obj);
static void Crash_extract_objs(struct obj_data *obj);
static int Crash_is_unrentable(struct obj_data *obj);
static void Crash_extract_norents(struct obj_data *obj);
static void Crash_extract_expensive(struct obj_data *obj);
static void Crash_calculate_rent(struct obj_data *obj, int *cost);
static void Crash_cryosave(struct char_data *ch, int cost);
static int Crash_load_objs(struct char_data *ch);
static int handle_obj(struct obj_data *obj, struct char_data *ch, int locate, struct obj_data **cont_rows);
static int objsave_write_rentcode(FILE *fl, int rentcode, int cost_per_day, struct char_data *ch);

/* Writes one object record to FILE.  Old name: Obj_to_store() */
int objsave_save_obj_record(struct obj_data *obj, FILE *fp, int locate)
{
  int counter2;
  struct extra_descr_data *ex_desc;
  char buf1[MAX_STRING_LENGTH +1];
  struct obj_data *temp = NULL;

  if (GET_OBJ_VNUM(obj) != NOTHING)
    temp=read_object(GET_OBJ_VNUM(obj), VIRTUAL);
  else {
    temp = create_obj();
    temp->item_number = NOWHERE;
  }

  if (obj->action_description) {

    strcpy(buf1, obj->action_description);
    strip_cr(buf1);
  } else
    *buf1 = 0;

  fprintf(fp, "#%d\n", GET_OBJ_VNUM(obj));
  if (locate)
    fprintf(fp, "Loc : %d\n", locate);
  if (GET_OBJ_VAL(obj, 0) != GET_OBJ_VAL(temp, 0) ||
      GET_OBJ_VAL(obj, 1) != GET_OBJ_VAL(temp, 1) ||
      GET_OBJ_VAL(obj, 2) != GET_OBJ_VAL(temp, 2) ||
      GET_OBJ_VAL(obj, 3) != GET_OBJ_VAL(temp, 3))
    fprintf(fp,
             "Vals: %d %d %d %d\n",
             GET_OBJ_VAL(obj, 0),
             GET_OBJ_VAL(obj, 1),
             GET_OBJ_VAL(obj, 2),
             GET_OBJ_VAL(obj, 3)
             );
  if (GET_OBJ_EXTRA(obj) != GET_OBJ_EXTRA(temp))
    fprintf(fp, "Flag: %d %d %d %d\n", GET_OBJ_EXTRA(obj)[0], GET_OBJ_EXTRA(obj)[1], GET_OBJ_EXTRA(obj)[2], GET_OBJ_EXTRA(obj)[3]);

#define TEST_OBJS(obj1, obj2, field) ((!obj1->field || !obj2->field || \
                                      strcmp(obj1->field, obj2->field)))
#define TEST_OBJN(field) (obj->obj_flags.field != temp->obj_flags.field)

  if (TEST_OBJS(obj, temp, name))
    fprintf(fp, "Name: %s\n", obj->name ? obj->name : "Undefined");
  if (TEST_OBJS(obj, temp, short_description))
    fprintf(fp, "Shrt: %s\n", obj->short_description ? obj->short_description : "Undefined");

  /* These two could be a pain on the read... we'll see... */
  if (TEST_OBJS(obj, temp, description))
    fprintf(fp, "Desc: %s\n", obj->description ? obj->description : "Undefined");

  /* Only even try to process this if an action desc exists */
  if (obj->action_description || temp->action_description)
    if (TEST_OBJS(obj, temp, action_description))
      fprintf(fp, "ADes:\n%s~\n", buf1);

  if (TEST_OBJN(type_flag))
    fprintf(fp, "Type: %d\n", GET_OBJ_TYPE(obj));
  if (TEST_OBJN(weight))
    fprintf(fp, "Wght: %d\n", GET_OBJ_WEIGHT(obj));
  if (TEST_OBJN(cost))
    fprintf(fp, "Cost: %d\n", GET_OBJ_COST(obj));
  if (TEST_OBJN(cost_per_day))
    fprintf(fp, "Rent: %d\n", GET_OBJ_RENT(obj));
  if (TEST_OBJN(bitvector))
    fprintf(fp, "Perm: %d %d %d %d\n", GET_OBJ_PERM(obj)[0], GET_OBJ_PERM(obj)[1], GET_OBJ_PERM(obj)[2], GET_OBJ_PERM(obj)[3]);
  if (TEST_OBJN(wear_flags))
    fprintf(fp, "Wear: %d %d %d %d\n", GET_OBJ_WEAR(obj)[0], GET_OBJ_WEAR(obj)[1], GET_OBJ_WEAR(obj)[2], GET_OBJ_WEAR(obj)[3]);

  /* Do we have affects? */
  for (counter2 = 0; counter2 < MAX_OBJ_AFFECT; counter2++)
    if (obj->affected[counter2].modifier != temp->affected[counter2].modifier)
      fprintf(fp, "Aff : %d %d %d\n",
               counter2,
               obj->affected[counter2].location,
               obj->affected[counter2].modifier
               );

  /* Do we have extra descriptions? */
  if (obj->ex_description || temp->ex_description) {
    /* To be reimplemented.  Need to handle this case in loading as
       well */
    if ((obj->ex_description && temp->ex_description &&
         obj->ex_description != temp->ex_description) ||
        !obj->ex_description || !temp->ex_description) {
      for (ex_desc = obj->ex_description; ex_desc; ex_desc = ex_desc->next) {
        /*. Sanity check to prevent nasty protection faults . */
        if (!*ex_desc->keyword || !*ex_desc->description) {
          continue;
        }
        strcpy(buf1, ex_desc->description);
        strip_cr(buf1);
        fprintf(fp, "EDes:\n"
                 "%s~\n"
                 "%s~\n",
                 ex_desc->keyword,
                 buf1
                 );
      }
    }
  }

  fprintf(fp, "\n");

  extract_obj(temp);

  return 1;
}

#undef TEST_OBJS
#undef TEST_OBJN

/* AutoEQ by Burkhard Knopf. */
static void auto_equip(struct char_data *ch, struct obj_data *obj, int location)
{
  int j;

  /* Lots of checks... */
  if (location > 0) {  /* Was wearing it. */
    switch (j = (location - 1)) {
    case WEAR_LIGHT:
      break;
    case WEAR_FINGER_R:
    case WEAR_FINGER_L:
      if (!CAN_WEAR(obj, ITEM_WEAR_FINGER)) /* not fitting :( */
        location = LOC_INVENTORY;
      break;
    case WEAR_NECK_1:
    case WEAR_NECK_2:
      if (!CAN_WEAR(obj, ITEM_WEAR_NECK))
        location = LOC_INVENTORY;
      break;
    case WEAR_BODY:
      if (!CAN_WEAR(obj, ITEM_WEAR_BODY))
        location = LOC_INVENTORY;
      break;
    case WEAR_HEAD:
      if (!CAN_WEAR(obj, ITEM_WEAR_HEAD))
        location = LOC_INVENTORY;
      break;
    case WEAR_LEGS:
      if (!CAN_WEAR(obj, ITEM_WEAR_LEGS))
        location = LOC_INVENTORY;
      break;
    case WEAR_FEET:
      if (!CAN_WEAR(obj, ITEM_WEAR_FEET))
        location = LOC_INVENTORY;
      break;
    case WEAR_HANDS:
      if (!CAN_WEAR(obj, ITEM_WEAR_HANDS))
        location = LOC_INVENTORY;
      break;
    case WEAR_ARMS:
      if (!CAN_WEAR(obj, ITEM_WEAR_ARMS))
        location = LOC_INVENTORY;
      break;
    case WEAR_SHIELD:
      if (!CAN_WEAR(obj, ITEM_WEAR_SHIELD))
        location = LOC_INVENTORY;
      break;
    case WEAR_ABOUT:
      if (!CAN_WEAR(obj, ITEM_WEAR_ABOUT))
        location = LOC_INVENTORY;
      break;
    case WEAR_WAIST:
      if (!CAN_WEAR(obj, ITEM_WEAR_WAIST))
        location = LOC_INVENTORY;
      break;
    case WEAR_WRIST_R:
    case WEAR_WRIST_L:
      if (!CAN_WEAR(obj, ITEM_WEAR_WRIST))
        location = LOC_INVENTORY;
      break;
    case WEAR_WIELD:
      if (!CAN_WEAR(obj, ITEM_WEAR_WIELD))
        location = LOC_INVENTORY;
      break;
    case WEAR_HOLD:
      if (CAN_WEAR(obj, ITEM_WEAR_HOLD))
        break;
      if (IS_WARRIOR(ch) && CAN_WEAR(obj, ITEM_WEAR_WIELD) && GET_OBJ_TYPE(obj) == ITEM_WEAPON)
        break;
      location = LOC_INVENTORY;
      break;
    default:
      location = LOC_INVENTORY;
    }

    if (location > 0) {      /* Wearable. */
      if (!GET_EQ(ch,j)) {
        /* Check the characters's alignment to prevent them from being zapped
	 * through the auto-equipping. */
        if (invalid_align(ch, obj) || invalid_class(ch, obj))
          location = LOC_INVENTORY;
        else
          equip_char(ch, obj, j);
      } else {  /* Oops, saved a player with double equipment? */
        mudlog(BRF, LVL_IMMORT, TRUE,
               "SYSERR: autoeq: '%s' already equipped in position %d.", GET_NAME(ch), location);
        location = LOC_INVENTORY;
      }
    }
  }
  if (location <= 0)  /* Inventory */
    obj_to_char(obj, ch);
}

int Crash_delete_file(char *name)
{
  char filename[MAX_INPUT_LENGTH];
  FILE *fl;

  if (!get_filename(filename, sizeof(filename), CRASH_FILE, name))
    return FALSE;

  if (!(fl = fopen(filename, "r"))) {
    if (errno != ENOENT)  /* if it fails but NOT because of no file */
      log("SYSERR: deleting crash file %s (1): %s", filename, strerror(errno));
    return FALSE;
  }
  fclose(fl);

  /* if it fails, NOT because of no file */
  if (remove(filename) < 0 && errno != ENOENT)
    log("SYSERR: deleting crash file %s (2): %s", filename, strerror(errno));

  return TRUE;
}

int Crash_delete_crashfile(struct char_data *ch)
{
  char filename[MAX_INPUT_LENGTH];
  int numread;
  FILE *fl;
  int rentcode;
  char line[READ_SIZE];

  if (!get_filename(filename, sizeof(filename), CRASH_FILE, GET_NAME(ch)))
    return FALSE;

  if (!(fl = fopen(filename, "r"))) {
    if (errno != ENOENT)  /* if it fails, NOT because of no file */
      log("SYSERR: checking for crash file %s (3): %s", filename, strerror(errno));
    return FALSE;
  }
  numread = get_line(fl,line);
  fclose(fl);

  if (numread == FALSE)
    return FALSE;
  sscanf(line,"%d ",&rentcode);

  if (rentcode == RENT_CRASH)
    Crash_delete_file(GET_NAME(ch));

  return TRUE;
}

int Crash_clean_file(char *name)
{
  char filename[MAX_INPUT_LENGTH], filetype[20];
  int numread;
  FILE *fl;
  int rentcode, timed, netcost, gold, account, nitems;
  char line[READ_SIZE];

  if (!get_filename(filename, sizeof(filename), CRASH_FILE, name))
    return FALSE;

  /* Open so that permission problems will be flagged now, at boot time. */
  if (!(fl = fopen(filename, "r"))) {
    if (errno != ENOENT)  /* if it fails, NOT because of no file */
      log("SYSERR: OPENING OBJECT FILE %s (4): %s", filename, strerror(errno));
    return FALSE;
  }

  numread = get_line(fl,line);
  fclose(fl);
  if (numread == FALSE)
    return FALSE;

  sscanf(line, "%d %d %d %d %d %d",&rentcode,&timed,&netcost,
         &gold,&account,&nitems);

  if ((rentcode == RENT_CRASH) ||
      (rentcode == RENT_FORCED) ||
      (rentcode == RENT_TIMEDOUT) ) {
    if (timed < time(0) - (CONFIG_CRASH_TIMEOUT * SECS_PER_REAL_DAY)) {
      Crash_delete_file(name);
      switch (rentcode) {
      case RENT_CRASH:
        strcpy(filetype, "crash");
        break;
      case RENT_FORCED:
        strcpy(filetype, "forced rent");
        break;
      case RENT_TIMEDOUT:
        strcpy(filetype, "idlesave");
        break;
      default:
        strcpy(filetype, "UNKNOWN!");
        break;
      }
      log("    Deleting %s's %s file.", name, filetype);
      return TRUE;
    }
    /* Must retrieve rented items w/in 30 days */
  } else if (rentcode == RENT_RENTED)
    if (timed < time(0) - (CONFIG_RENT_TIMEOUT * SECS_PER_REAL_DAY)) {
      Crash_delete_file(name);
      log("    Deleting %s's rent file.", name);
      return TRUE;
    }
  return FALSE;
}

void update_obj_file(void)
{
  int i;

  for (i = 0; i <= top_of_p_table; i++)
    if (*player_table[i].name)
      Crash_clean_file(player_table[i].name);
}

void Crash_listrent(struct char_data *ch, char *name)
{
  FILE *fl;
  char filename[MAX_INPUT_LENGTH], buf[MAX_STRING_LENGTH], line[READ_SIZE];
  obj_save_data *loaded, *current;
  int rentcode,timed,netcost,gold,account,nitems, numread, len;

  if (!get_filename(filename, sizeof(filename), CRASH_FILE, name))
    return;

  if (!(fl = fopen(filename, "r"))) {
    send_to_char(ch, "%s has no rent file.\r\n", name);
    return;
  }
  len = snprintf(buf, sizeof(buf),"%s\r\n", filename);

  numread = get_line(fl, line);

  /* Oops, can't get the data, punt. */
  if (numread == FALSE) {
    send_to_char(ch, "Error reading rent information.\r\n");
    fclose(fl);
    return;
  }

  sscanf(line,"%d %d %d %d %d %d",
        &rentcode,&timed,&netcost,&gold,&account,&nitems);

  switch (rentcode) {
  case RENT_RENTED:
    len += snprintf(buf+len, sizeof(buf)-len, "Rent\r\n");
    break;
  case RENT_CRASH:
    len += snprintf(buf+len, sizeof(buf)-len,"Crash\r\n");
    break;
  case RENT_CRYO:
    len += snprintf(buf+len, sizeof(buf)-len, "Cryo\r\n");
    break;
  case RENT_TIMEDOUT:
  case RENT_FORCED:
    len += snprintf(buf+len, sizeof(buf)-len, "TimedOut\r\n");
    break;
  default:
    len += snprintf(buf+len, sizeof(buf)-len, "Undef\r\n");
    break;
  }

	loaded = objsave_parse_objects(fl);

	for (current = loaded; current != NULL; current=current->next)
	  len += snprintf(buf+len, sizeof(buf)-len, "[%5d] (%5dau) %-20s\r\n",
                GET_OBJ_VNUM(current->obj),
                GET_OBJ_RENT(current->obj),
                current->obj->short_description);

	/* Now it's safe to free the obj_save_data list and the objects on it. */
	while (loaded != NULL) {
		current = loaded;
		loaded = loaded->next;
		extract_obj(current->obj);
		free(current);
	}

  page_string(ch->desc,buf,0);
  fclose(fl);
}

/* Return values:
 *  0 - successful load, keep char in rent room.
 *  1 - load failure or load of crash items -- put char in temple.
 *  2 - rented equipment lost (no $) */
int Crash_load(struct char_data *ch)
{
  return (Crash_load_objs(ch));
}

static int Crash_save(struct obj_data *obj, FILE *fp, int location)
{
  struct obj_data *tmp;
  int result;

  if (obj) {
    Crash_save(obj->next_content, fp, location);
    Crash_save(obj->contains, fp, MIN(0, location) - 1);

    result = objsave_save_obj_record(obj, fp, location);

    for (tmp = obj->in_obj; tmp; tmp = tmp->in_obj)
      GET_OBJ_WEIGHT(tmp) -= GET_OBJ_WEIGHT(obj);

    if (!result)
      return FALSE;
  }
  return (TRUE);
}

static void Crash_restore_weight(struct obj_data *obj)
{
  if (obj) {
    Crash_restore_weight(obj->contains);
    Crash_restore_weight(obj->next_content);
    if (obj->in_obj)
      GET_OBJ_WEIGHT(obj->in_obj) += GET_OBJ_WEIGHT(obj);
  }
}

/* Get !RENT items from equipment to inventory and extract !RENT out of worn
 * containers. */
static void Crash_extract_norent_eq(struct char_data *ch)
{
  int j;

  for (j = 0; j < NUM_WEARS; j++) {
    if (GET_EQ(ch, j) == NULL)
      continue;

    if (Crash_is_unrentable(GET_EQ(ch, j)))
      obj_to_char(unequip_char(ch, j), ch);
    else
      Crash_extract_norents(GET_EQ(ch, j));
  }
}

static void Crash_extract_objs(struct obj_data *obj)
{
  if (obj) {
    Crash_extract_objs(obj->contains);
    Crash_extract_objs(obj->next_content);
    extract_obj(obj);
  }
}

static int Crash_is_unrentable(struct obj_data *obj)
{
  if (!obj)
    return FALSE;

  if (OBJ_FLAGGED(obj, ITEM_NORENT) ||
      GET_OBJ_RENT(obj) < 0 ||
      GET_OBJ_RNUM(obj) == NOTHING ||
      GET_OBJ_TYPE(obj) == ITEM_KEY) {
 log("Crash_is_unrentable: removing object %s", obj->short_description);
    return TRUE;
  }

  return FALSE;
}

static void Crash_extract_norents(struct obj_data *obj)
{
  if (obj) {
    Crash_extract_norents(obj->contains);
    Crash_extract_norents(obj->next_content);
    if (Crash_is_unrentable(obj))
      extract_obj(obj);
  }
}

static void Crash_extract_expensive(struct obj_data *obj)
{
  struct obj_data *tobj, *max;

  max = obj;
  for (tobj = obj; tobj; tobj = tobj->next_content)
    if (GET_OBJ_RENT(tobj) > GET_OBJ_RENT(max))
      max = tobj;
  extract_obj(max);
}

static void Crash_calculate_rent(struct obj_data *obj, int *cost)
{
  if (obj) {
    *cost += MAX(0, GET_OBJ_RENT(obj));
    Crash_calculate_rent(obj->contains, cost);
    Crash_calculate_rent(obj->next_content, cost);
  }
}

void Crash_crashsave(struct char_data *ch)
{
  char buf[MAX_INPUT_LENGTH];
  int j;
  FILE *fp;

  if (IS_NPC(ch))
    return;

  if (!get_filename(buf, sizeof(buf), CRASH_FILE, GET_NAME(ch)))
    return;

  if (!(fp = fopen(buf, "w")))
    return;

  if (!objsave_write_rentcode(fp, RENT_CRASH, 0, ch))
  	return;

  for (j = 0; j < NUM_WEARS; j++)
    if (GET_EQ(ch, j)) {
      if (!Crash_save(GET_EQ(ch, j), fp, j + 1)) {
        fclose(fp);
        return;
      }
      Crash_restore_weight(GET_EQ(ch, j));
    }

  if (!Crash_save(ch->carrying, fp, 0)) {
    fclose(fp);
    return;
  }
  Crash_restore_weight(ch->carrying);

  fprintf(fp, "$~\n");
  fclose(fp);
  REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_CRASH);
}

void Crash_idlesave(struct char_data *ch)
{
  char buf[MAX_INPUT_LENGTH];
  int j;
  int cost, cost_eq;
  FILE *fp;

  if (IS_NPC(ch))
    return;

  if (!get_filename(buf, sizeof(buf), CRASH_FILE, GET_NAME(ch)))
    return;

  if (!(fp = fopen(buf, "w")))
    return;

  Crash_extract_norent_eq(ch);
  Crash_extract_norents(ch->carrying);

  cost = 0;
  Crash_calculate_rent(ch->carrying, &cost);

  cost_eq = 0;
  for (j = 0; j < NUM_WEARS; j++)
    Crash_calculate_rent(GET_EQ(ch, j), &cost_eq);

  cost += cost_eq;
  cost *= 2;    /* forcerent cost is 2x normal rent */

  if (cost > GET_GOLD(ch) + GET_BANK_GOLD(ch)) {
    for (j = 0; j < NUM_WEARS; j++)  /* Unequip players with low gold. */
      if (GET_EQ(ch, j))
        obj_to_char(unequip_char(ch, j), ch);

    while ((cost > GET_GOLD(ch) + GET_BANK_GOLD(ch)) && ch->carrying) {
      Crash_extract_expensive(ch->carrying);
      cost = 0;
      Crash_calculate_rent(ch->carrying, &cost);
      cost *= 2;
    }
  }

  if (ch->carrying == NULL) {
    for (j = 0; j < NUM_WEARS && GET_EQ(ch, j) == NULL; j++) /* Nothing */ ;
    if (j == NUM_WEARS) {  /* No equipment or inventory. */
      fclose(fp);
      Crash_delete_file(GET_NAME(ch));
      return;
    }
  }

  if (!objsave_write_rentcode(fp, RENT_TIMEDOUT, cost, ch))
  	return;

  for (j = 0; j < NUM_WEARS; j++) {
    if (GET_EQ(ch, j)) {
      if (!Crash_save(GET_EQ(ch, j), fp, j + 1)) {
        fclose(fp);
        return;
      }
      Crash_restore_weight(GET_EQ(ch, j));
      Crash_extract_objs(GET_EQ(ch, j));
    }
  }
  if (!Crash_save(ch->carrying, fp, 0)) {
    fclose(fp);
    return;
  }
  fprintf(fp, "$~\n");
  fclose(fp);

  Crash_extract_objs(ch->carrying);
}

void Crash_rentsave(struct char_data *ch, int cost)
{
  char buf[MAX_INPUT_LENGTH];
  int j;
  FILE *fp;

  if (IS_NPC(ch))
    return;

  if (!get_filename(buf, sizeof(buf), CRASH_FILE, GET_NAME(ch)))
    return;

  if (!(fp = fopen(buf, "w")))
    return;

  Crash_extract_norent_eq(ch);
  Crash_extract_norents(ch->carrying);

  if (!objsave_write_rentcode(fp, RENT_RENTED, cost, ch))
  	return;

  for (j = 0; j < NUM_WEARS; j++)
    if (GET_EQ(ch, j)) {
      if (!Crash_save(GET_EQ(ch,j), fp, j + 1)) {
        fclose(fp);
        return;
      }
      Crash_restore_weight(GET_EQ(ch, j));
      Crash_extract_objs(GET_EQ(ch, j));

    }
  if (!Crash_save(ch->carrying, fp, 0)) {
    fclose(fp);
    return;
  }
  fprintf(fp, "$~\n");
  fclose(fp);

  Crash_extract_objs(ch->carrying);
}

static int objsave_write_rentcode(FILE *fl, int rentcode, int cost_per_day, struct char_data *ch)
{
  if (fprintf(fl, "%d %ld %d %d %d %d\r\n",
          rentcode,
          (long) time(0),
          cost_per_day,
          GET_GOLD(ch),
          GET_BANK_GOLD(ch),
          0)
       < 1)
    {
       perror("Syserr: Writing rent code");
       return FALSE;
    }
  return TRUE;

}

static void Crash_cryosave(struct char_data *ch, int cost)
{
  char buf[MAX_INPUT_LENGTH];
  int j;
  FILE *fp;

  if (IS_NPC(ch))
    return;

  if (!get_filename(buf, sizeof(buf), CRASH_FILE, GET_NAME(ch)))
    return;

  if (!(fp = fopen(buf, "w")))
    return;

  Crash_extract_norent_eq(ch);
  Crash_extract_norents(ch->carrying);

  GET_GOLD(ch) = MAX(0, GET_GOLD(ch) - cost);

  if (!objsave_write_rentcode(fp, RENT_CRYO, 0, ch))
  	return;

  for (j = 0; j < NUM_WEARS; j++)
    if (GET_EQ(ch, j)) {
      if (!Crash_save(GET_EQ(ch, j), fp, j + 1)) {
        fclose(fp);
        return;
      }
      Crash_restore_weight(GET_EQ(ch, j));
      Crash_extract_objs(GET_EQ(ch, j));
    }
  if (!Crash_save(ch->carrying, fp, 0)) {
    fclose(fp);
    return;
  }
  fprintf(fp, "$~\n");
  fclose(fp);

  Crash_extract_objs(ch->carrying);
  SET_BIT_AR(PLR_FLAGS(ch), PLR_CRYO);
}

/* Routines used for the receptionist. */
static void Crash_rent_deadline(struct char_data *ch, struct char_data *recep,
                         long cost)
{
  long rent_deadline;
  char buf[MAX_STRING_LENGTH];

  if (!cost)
    return;

  rent_deadline = ((GET_GOLD(ch) + GET_BANK_GOLD(ch)) / cost);
  snprintf(buf, sizeof(buf), "$n tells you, 'You can rent for %ld day%s with the gold you have\r\n"
         "on hand and in the bank.'\r\n", rent_deadline, rent_deadline != 1 ? "s" : "");
act(buf, FALSE, recep, 0, ch, TO_VICT);
}

static int Crash_report_unrentables(struct char_data *ch, struct char_data *recep,
                             struct obj_data *obj)
{
  char buf[128];
  int has_norents = 0;

  if (obj) {
    if (Crash_is_unrentable(obj)) {
      has_norents = 1;
      sprintf(buf, "$n tells you, 'You cannot store %s.'", OBJS(obj, ch));
      act(buf, FALSE, recep, 0, ch, TO_VICT);
    }
    has_norents += Crash_report_unrentables(ch, recep, obj->contains);
    has_norents += Crash_report_unrentables(ch, recep, obj->next_content);
  }
  return (has_norents);
}

static void Crash_report_rent(struct char_data *ch, struct char_data *recep, struct
    obj_data *obj, long *cost, long *nitems, int display, int factor)
{
  static char buf[256];

  if (obj) {
    if (!Crash_is_unrentable(obj)) {
      (*nitems)++;
      *cost += MAX(0, (GET_OBJ_RENT(obj) * factor));
      if (display) {
        sprintf(buf, "$n tells you, '%5d coins for %s..'",
                (GET_OBJ_RENT(obj) * factor), OBJS(obj, ch));
        act(buf, FALSE, recep, 0, ch, TO_VICT);
      }
    }
    Crash_report_rent(ch, recep, obj->contains, cost, nitems, display, factor);
    Crash_report_rent(ch, recep, obj->next_content, cost, nitems, display, factor);
  }
}

static int Crash_offer_rent(struct char_data *ch, struct char_data *recep,
                     int display, int factor)
{
  char buf[MAX_INPUT_LENGTH];
  int i;
  long totalcost = 0, numitems = 0, norent;

  norent = Crash_report_unrentables(ch, recep, ch->carrying);
  for (i = 0; i < NUM_WEARS; i++)
    norent += Crash_report_unrentables(ch, recep, GET_EQ(ch, i));

  if (norent)
    return FALSE;

  totalcost = CONFIG_MIN_RENT_COST * factor;

  Crash_report_rent(ch, recep, ch->carrying, &totalcost, &numitems, display, factor);

  for (i = 0; i < NUM_WEARS; i++)
    Crash_report_rent(ch, recep, GET_EQ(ch, i), &totalcost, &numitems, display, factor);

  if (!numitems) {
    act("$n tells you, 'But you are not carrying anything!  Just quit!'",
        FALSE, recep, 0, ch, TO_VICT);
    return FALSE;
  }
  if (numitems > CONFIG_MAX_OBJ_SAVE) {
    sprintf(buf, "$n tells you, 'Sorry, but I cannot store more than %d items.'",
        CONFIG_MAX_OBJ_SAVE);
    act(buf, FALSE, recep, 0, ch, TO_VICT);
    return FALSE;
  }
  if (display) {
    sprintf(buf, "$n tells you, 'Plus, my %d coin fee..'",
        CONFIG_MIN_RENT_COST * factor);
    act(buf, FALSE, recep, 0, ch, TO_VICT);
    sprintf(buf, "$n tells you, 'For a total of %ld coins%s.'",
            totalcost, (factor == RENT_FACTOR ? " per day" : ""));
    act(buf, FALSE, recep, 0, ch, TO_VICT);
    if (totalcost > GET_GOLD(ch) + GET_BANK_GOLD(ch)) {
      act("$n tells you, '...which I see you can't afford.'",
          FALSE, recep, 0, ch, TO_VICT);
      return FALSE;
    } else if (factor == RENT_FACTOR)
      Crash_rent_deadline(ch, recep, totalcost);
  }
  return (totalcost);
}

static int gen_receptionist(struct char_data *ch, struct char_data *recep, int cmd,
    char *arg, int mode)
{
  int cost;
  char buf[128];
  const char *action_table[] = { "smile", "dance", "sigh", "blush", "burp",
	  "cough", "fart", "twiddle", "yawn" };

  if (!cmd && !rand_number(0, 5)) {
    do_action(recep, NULL, find_command(action_table[rand_number(0, 8)]), 0);
    return (FALSE);
  }

  if (!ch->desc || IS_NPC(ch))
    return (FALSE);

  if (!CMD_IS("offer") && !CMD_IS("rent"))
    return (FALSE);

  if (!AWAKE(recep)) {
    send_to_char(ch, "%s is unable to talk to you...\r\n", HSSH(recep));
    return (TRUE);
  }

  if (!CAN_SEE(recep, ch)) {
    act("$n says, 'I don't deal with people I can't see!'", FALSE, recep, 0, 0, TO_ROOM);
    return (TRUE);
  }

  if (CONFIG_FREE_RENT) {
    act("$n tells you, 'Rent is free here.  Just quit, and your objects will be saved!'",
	FALSE, recep, 0, ch, TO_VICT);
    return TRUE;
  }

  if (CMD_IS("rent")) {

    if (!(cost = Crash_offer_rent(ch, recep, FALSE, mode)))
      return (TRUE);
    if (mode == RENT_FACTOR)
      snprintf(buf, sizeof(buf), "$n tells you, 'Rent will cost you %d gold coins per day.'", cost);
    else if (mode == CRYO_FACTOR)
      snprintf(buf, sizeof(buf), "$n tells you, 'It will cost you %d gold coins to be frozen.'", cost);
    act(buf, FALSE, recep, 0, ch, TO_VICT);

    if (cost > GET_GOLD(ch) + GET_BANK_GOLD(ch)) {
      act("$n tells you, '...which I see you can't afford.'",
	  FALSE, recep, 0, ch, TO_VICT);
      return (TRUE);
    }
    if (cost && (mode == RENT_FACTOR))
      Crash_rent_deadline(ch, recep, cost);

    if (mode == RENT_FACTOR) {
      act("$n stores your belongings and helps you into your private chamber.", FALSE, recep, 0, ch, TO_VICT);
      Crash_rentsave(ch, cost);
      mudlog(NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE, "%s has rented (%d/day, %d tot.)",
		GET_NAME(ch), cost, GET_GOLD(ch) + GET_BANK_GOLD(ch));
    } else {			/* cryo */
      act("$n stores your belongings and helps you into your private chamber.\r\n"
	  "A white mist appears in the room, chilling you to the bone...\r\n"
	  "You begin to lose consciousness...",
	  FALSE, recep, 0, ch, TO_VICT);
      Crash_cryosave(ch, cost);
      mudlog(NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE, "%s has cryo-rented.", GET_NAME(ch));
      SET_BIT_AR(PLR_FLAGS(ch), PLR_CRYO);
    }

    act("$n helps $N into $S private chamber.", FALSE, recep, 0, ch, TO_NOTVICT);

    GET_LOADROOM(ch) = GET_ROOM_VNUM(IN_ROOM(ch));
    extract_char(ch);	/* It saves. */
  } else {
    Crash_offer_rent(ch, recep, TRUE, mode);
    act("$N gives $n an offer.", FALSE, ch, 0, recep, TO_ROOM);
  }
  return (TRUE);
}

SPECIAL(receptionist)
{
  return (gen_receptionist(ch, (struct char_data *)me, cmd, argument, RENT_FACTOR));
}

SPECIAL(cryogenicist)
{
  return (gen_receptionist(ch, (struct char_data *)me, cmd, argument, CRYO_FACTOR));
}

void Crash_save_all(void)
{
  struct descriptor_data *d;
  for (d = descriptor_list; d; d = d->next) {
    if ((STATE(d) == CON_PLAYING) && !IS_NPC(d->character)) {
      if (PLR_FLAGGED(d->character, PLR_CRASH)) {
        Crash_crashsave(d->character);
        save_char(d->character);
        REMOVE_BIT_AR(PLR_FLAGS(d->character), PLR_CRASH);
      }
    }
  }
}

/* Parses the object records stored in fl, and returns the first object in a
 * linked list, which also handles location if worn. This list can then be
 * handled by house code, listrent code, autoeq code, etc. */
obj_save_data *objsave_parse_objects(FILE *fl)
{
  obj_save_data *head, *current, *tempsave;
  char f1[128], f2[128], f3[128], f4[128], line[READ_SIZE];
  int t[4],i, nr;
  struct obj_data *temp;

  CREATE(current, obj_save_data, 1);
  head = current;
  current->locate = 0;

  temp = NULL;
  while (TRUE) {
    char tag[6];
    int num;

    /* if the file is done, wrap it all up */
    if(get_line(fl, line) == FALSE || (*line == '$' && line[1] == '~')) {
      if (temp == NULL && current->obj == NULL) {
        /* Remove current from list. */
        tempsave = head;
        if (tempsave == current) {
          free(current);
          head = NULL;
        } else {
          while (tempsave) {
            if (tempsave->next == current)
              tempsave->next = NULL;
            tempsave = tempsave->next;
          }
          free(current);
        }
      }
    else if (temp != NULL && current->obj == NULL)
      current->obj = temp;
    else if (temp == NULL && current->obj != NULL) {
      /* Do nothing. */
  } else if (temp != NULL && current->obj != NULL) {
      if (temp != current->obj)
        log("inconsistent object pointers in objsave_parse_objects: %p/%p", temp, current->obj);
    }

    break;
  }

    /* if it's a new record, wrap up the old one, and make space for a new one */
    if (*line == '#') {
      /* check for false alarm. */
      if (sscanf(line, "#%d", &nr) == 1) {
        /* If we attempt to load an object with a legal VNUM 0-65534, that
         * does not exist, skip it. If the object has a VNUM of NOTHING or
         * 65535, then we assume it doesn't exist on purpose. (Custom Item,
         * Coins, Corpse, etc...) */
        if (real_object(nr) == NOTHING && nr != NOTHING) {
            log("SYSERR: Prevented loading of non-existant item #%d.", nr);
            continue;
          }
          
        if (temp) {
          current->obj = temp;
          CREATE(current->next, obj_save_data, 1);
          current=current->next;

          current->locate = 0;
          temp = NULL;
        }
      } else
        continue;
        
      /* we have the number, check it, load obj. */
      if (nr == NOTHING) {   /* then it is unique */
        temp = create_obj();
        temp->item_number=NOTHING;
      } else if (nr < 0) {
        continue;
      } else {
        if(real_object(nr) != NOTHING) {
          temp=read_object(nr,VIRTUAL);
	/* Go read next line - nothing more to see here. */
        } else {
          log("Nonexistent object %d found in rent file.", nr);
        }
      }
      /* go read next line - nothing more to see here. */
      continue;
    }

    /* If "temp" is NULL, we are most likely progressing through
     * a non-existant object, so just keep continuing till we find 
     * the next object */
    if (temp == NULL)
      continue;

    tag_argument(line, tag);
    num = atoi(line);

    switch(*tag) {
    case 'A':
      if (!strcmp(tag, "ADes")) {
        char error[40];
        snprintf(error, sizeof(error)-1, "rent(Ades):%s", temp->name);
        temp->action_description = fread_string(fl, error);
      } else if (!strcmp(tag, "Aff ")) {
        sscanf(line, "%d %d %d", &t[0], &t[1], &t[2]);
        if (t[0] < MAX_OBJ_AFFECT) {
          temp->affected[t[0]].location = t[1];
          temp->affected[t[0]].modifier = t[2];
        }
      }
      break;
    case 'C':
      if (!strcmp(tag, "Cost"))
        GET_OBJ_COST(temp) = num;
      break;
    case 'D':
      if (!strcmp(tag, "Desc"))
        temp->description = strdup(line);
      break;
    case 'E':
      if(!strcmp(tag, "EDes")) {
        struct extra_descr_data *new_desc;
        char error[40];
        snprintf(error, sizeof(error)-1, "rent(Edes): %s", temp->name);
        if (temp->item_number != NOTHING && /* Regular object */
            temp->ex_description &&   /* with ex_desc == prototype */
            (temp->ex_description == obj_proto[real_object(temp->item_number)].ex_description))
          temp->ex_description = NULL;
        CREATE(new_desc, struct extra_descr_data, 1);
        new_desc->keyword = fread_string(fl, error);
        new_desc->description = fread_string(fl, error);
        new_desc->next = temp->ex_description;
        temp->ex_description = new_desc;
      }
      break;
    case 'F':
      if (!strcmp(tag, "Flag")) {
        sscanf(line, "%s %s %s %s", f1, f2, f3, f4);
        GET_OBJ_EXTRA(temp)[0] = asciiflag_conv(f1);
        GET_OBJ_EXTRA(temp)[1] = asciiflag_conv(f2);
        GET_OBJ_EXTRA(temp)[2] = asciiflag_conv(f3);
        GET_OBJ_EXTRA(temp)[3] = asciiflag_conv(f4);
      }
      break;
    case 'L':
      if(!strcmp(tag, "Loc "))
        current->locate = num;
      break;
    case 'N':
      if (!strcmp(tag, "Name"))
        temp->name = strdup(line);
      break;
    case 'P':
      if (!strcmp(tag, "Perm")) {
        sscanf(line, "%s %s %s %s", f1, f2, f3, f4);
        GET_OBJ_PERM(temp)[0] = asciiflag_conv(f1);
        GET_OBJ_PERM(temp)[1] = asciiflag_conv(f2);
        GET_OBJ_PERM(temp)[2] = asciiflag_conv(f3);
        GET_OBJ_PERM(temp)[3] = asciiflag_conv(f4);
      }
      break;
    case 'R':
      if (!strcmp(tag, "Rent"))
        GET_OBJ_RENT(temp) = num;
      break;
    case 'S':
      if (!strcmp(tag, "Shrt"))
        temp->short_description = strdup(line);
      break;
    case 'T':
      if (!strcmp(tag, "Type"))
        GET_OBJ_TYPE(temp) = num;
      break;
    case 'W':
      if (!strcmp(tag, "Wear")) {
        sscanf(line, "%s %s %s %s", f1, f2, f3, f4);
        GET_OBJ_WEAR(temp)[0] = asciiflag_conv(f1);
        GET_OBJ_WEAR(temp)[1] = asciiflag_conv(f2);
        GET_OBJ_WEAR(temp)[2] = asciiflag_conv(f3);
        GET_OBJ_WEAR(temp)[3] = asciiflag_conv(f4);
      }
      else if (!strcmp(tag, "Wght"))
        GET_OBJ_WEIGHT(temp) = num;
      break;
    case 'V':
      if (!strcmp(tag, "Vals")) {
        sscanf(line, "%d %d %d %d", &t[0], &t[1], &t[2], &t[3]);
        for (i = 0; i < NUM_OBJ_VAL_POSITIONS; i++)
          GET_OBJ_VAL(temp, i) = t[i];
      }
      break;
    default:
      log("Unknown tag in rentfile: %s", tag);
    }
  }

  return head;
}

static int Crash_load_objs(struct char_data *ch) {
  FILE *fl;
  char filename[MAX_STRING_LENGTH];
  char line[READ_SIZE];
  char buf[MAX_STRING_LENGTH];
  char str[64];
  int i, num_of_days, orig_rent_code, num_objs=0;
  unsigned long cost;
  struct obj_data *cont_row[MAX_BAG_ROWS];
  int rentcode,timed,netcost,gold,account,nitems;
	obj_save_data *loaded, *current;

  if (!get_filename(filename, sizeof(filename), CRASH_FILE, GET_NAME(ch)))
    return 1;

  for (i = 0; i < MAX_BAG_ROWS; i++)
    cont_row[i] = NULL;

  if (!(fl = fopen(filename, "r"))) {
    if (errno != ENOENT) { /* if it fails, NOT because of no file */
      sprintf(buf, "SYSERR: READING OBJECT FILE %s (5)", filename);
      perror(buf);
      send_to_char(ch, "\r\n********************* NOTICE *********************\r\n"
                       "There was a problem loading your objects from disk.\r\n"
                       "Contact a God for assistance.\r\n");
    }
    mudlog(NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE, "%s entering game with no equipment.", GET_NAME(ch));
    return 1;
  }
  if (get_line(fl, line))
    sscanf(line,"%d %d %d %d %d %d",&rentcode, &timed,
           &netcost,&gold,&account,&nitems);

  if (rentcode == RENT_RENTED || rentcode == RENT_TIMEDOUT) {
    sprintf(str, "%d", SECS_PER_REAL_DAY);
    num_of_days = (int)((float) (time(0) - timed) / (float)atoi(str));
    cost = (unsigned int) (netcost * num_of_days);
    if (cost > (unsigned int)GET_GOLD(ch) + (unsigned int)GET_BANK_GOLD(ch)) {
      fclose(fl);
      mudlog(BRF, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE,
             "%s entering game, rented equipment lost (no $).", GET_NAME(ch));
      Crash_crashsave(ch);
      return 2;
    } else {
      GET_BANK_GOLD(ch) -= MAX(cost - GET_GOLD(ch), 0);
      GET_GOLD(ch) = MAX(GET_GOLD(ch) - cost, 0);
      save_char(ch);
    }
  }
  switch (orig_rent_code = rentcode) {
  case RENT_RENTED:
    mudlog(NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE,
           "%s un-renting and entering game.", GET_NAME(ch));
    break;
  case RENT_CRASH:

    mudlog(NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE,
           "%s retrieving crash-saved items and entering game.", GET_NAME(ch));
    break;
  case RENT_CRYO:
    mudlog(NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE,
           "%s un-cryo'ing and entering game.", GET_NAME(ch));
    break;
  case RENT_FORCED:
  case RENT_TIMEDOUT:
    mudlog(NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE,
           "%s retrieving force-saved items and entering game.", GET_NAME(ch));
    break;
  default:
    mudlog(NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE,
           "WARNING: %s entering game with undefined rent code.", GET_NAME(ch));
    break;
  }

	loaded = objsave_parse_objects(fl);
	for (current = loaded; current != NULL; current=current->next)
	  num_objs += handle_obj(current->obj, ch, current->locate, cont_row);

	/* now it's safe to free the obj_save_data list - all members of it
	 * have been put in the correct lists by handle_obj() */
	while (loaded != NULL) {
		current = loaded;
		loaded = loaded->next;
		free(current);
	}

  /* Little hoarding check. -gg 3/1/98 */
 mudlog(NRM, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE, "%s (level %d) has %d %s (max %d).",
         GET_NAME(ch), GET_LEVEL(ch), num_objs, num_objs > 1 ? "objects" : "object", CONFIG_MAX_OBJ_SAVE);

  fclose(fl);

  if ((orig_rent_code == RENT_RENTED) || (orig_rent_code == RENT_CRYO))
    return 0;
  else
    return 1;
}

static int handle_obj(struct obj_data *temp, struct char_data *ch, int locate, struct obj_data **cont_row)
{
  int j;
  struct obj_data *obj1;

  if (!temp)  /* this should never happen, but.... */
    return FALSE;

  auto_equip(ch, temp, locate);

  /* What to do with a new loaded item:
   * If there's a list with <locate> less than 1 below this: (equipped items
   * are assumed to have <locate>==0 here) then its container has disappeared
   * from the file   *gasp* -> put all the list back to ch's inventory if
   * there's a list of contents with <locate> 1 below this: check if it's a
   * container - if so: get it from ch, fill it, and give it back to ch (this
   * way the container has its correct weight before modifying ch) - if not:
   * the container is missing -> put all the list to ch's inventory. For items
   * with negative <locate>: If there's already a list of contents with the
   * same <locate> put obj to it if not, start a new list. Since <locate> for
   * contents is < 0 the list indices are switched to non-negative. */
  if (locate > 0) { /* item equipped */

    for (j = MAX_BAG_ROWS-1;j > 0;j--)
      if (cont_row[j]) { /* no container -> back to ch's inventory */
        for (;cont_row[j];cont_row[j] = obj1) {
          obj1 = cont_row[j]->next_content;
          obj_to_char(cont_row[j], ch);
        }
        cont_row[j] = NULL;
      }
    if (cont_row[0]) { /* content list existing */
      if (GET_OBJ_TYPE(temp) == ITEM_CONTAINER) {
        /* rem item ; fill ; equip again */
        temp = unequip_char(ch, locate-1);
        temp->contains = NULL; /* should be empty - but who knows */
        for (;cont_row[0];cont_row[0] = obj1) {
          obj1 = cont_row[0]->next_content;
          obj_to_obj(cont_row[0], temp);
        }
        equip_char(ch, temp, locate-1);
      } else { /* object isn't container -> empty content list */
        for (;cont_row[0];cont_row[0] = obj1) {
          obj1 = cont_row[0]->next_content;
          obj_to_char(cont_row[0], ch);
        }
        cont_row[0] = NULL;
      }
    }
  } else { /* locate <= 0 */
    for (j = MAX_BAG_ROWS-1;j > -locate;j--)
      if (cont_row[j]) { /* no container -> back to ch's inventory */
        for (;cont_row[j];cont_row[j] = obj1) {
          obj1 = cont_row[j]->next_content;
          obj_to_char(cont_row[j], ch);
        }
        cont_row[j] = NULL;
      }

    if (j == -locate && cont_row[j]) { /* content list existing */
      if (GET_OBJ_TYPE(temp) == ITEM_CONTAINER) {
        /* take item ; fill ; give to char again */
        obj_from_char(temp);
        temp->contains = NULL;
        for (;cont_row[j];cont_row[j] = obj1) {
          obj1 = cont_row[j]->next_content;
          obj_to_obj(cont_row[j], temp);
        }
        obj_to_char(temp, ch); /* add to inv first ... */
      } else { /* object isn't container -> empty content list */
        for (;cont_row[j];cont_row[j] = obj1) {
          obj1 = cont_row[j]->next_content;
          obj_to_char(cont_row[j], ch);
        }
        cont_row[j] = NULL;
      }
    }

    if (locate < 0 && locate >= -MAX_BAG_ROWS) {
      /* let obj be part of content list
         but put it at the list's end thus having the items
         in the same order as before renting */
      obj_from_char(temp);
      if ((obj1 = cont_row[-locate-1])) {
        while (obj1->next_content)
          obj1 = obj1->next_content;
        obj1->next_content = temp;
      } else
        cont_row[-locate-1] = temp;
    }
  } /* locate less than zero */

  return TRUE;
}

