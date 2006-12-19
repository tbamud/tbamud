/* ************************************************************************ 
*   File: objsave.c                                     Part of CircleMUD *
*  Usage: loading/saving player objects for rent and crash-save           *
*                                                                         * 
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University * 
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include "conf.h"
#include "sysdep.h"


#include "structs.h"
#include "comm.h"
#include "handler.h"
#include "db.h"
#include "interpreter.h"
#include "utils.h"
#include "spells.h"

/* these factors should be unique integers */
#define RENT_FACTOR    1
#define CRYO_FACTOR    4

#define LOC_INVENTORY  0
#define MAX_BAG_ROWS   5

/* external variables */
extern struct player_index_element *player_table;
extern int top_of_p_table;
extern int rent_file_timeout, crash_file_timeout;
extern int free_rent;
extern int min_rent_cost;
extern int max_obj_save;  /* change in config.c */

/* Extern functions */
ACMD(do_action);
ACMD(do_tell);
SPECIAL(receptionist);
SPECIAL(cryogenicist);
int invalid_class(struct char_data *ch, struct obj_data *obj);
bitvector_t asciiflag_conv(char *flag);
int sprintascii(char *out, bitvector_t bits);

/* local functions */
void Crash_extract_norent_eq(struct char_data *ch);
void auto_equip(struct char_data *ch, struct obj_data *obj, int location);
int Crash_offer_rent(struct char_data *ch, struct char_data *receptionist, int display, int factor);
int Crash_report_unrentables(struct char_data *ch, struct char_data *recep, struct obj_data *obj);
void Crash_report_rent(struct char_data *ch, struct char_data *recep, struct obj_data *obj, long *cost, long *nitems, int display, int factor);
struct obj_data *Obj_from_store(struct obj_file_elem object, int *location);
int Obj_to_store(struct obj_data *obj, FILE *fl, int location);
void update_obj_file(void);
int Crash_write_rentcode(struct char_data *ch, FILE *fl, struct rent_info *rent);
int gen_receptionist(struct char_data *ch, struct char_data *recep, int cmd, char *arg, int mode);
int Crash_save(struct obj_data *obj, FILE *fp, int location);
void Crash_rent_deadline(struct char_data *ch, struct char_data *recep, long cost);
void Crash_restore_weight(struct obj_data *obj);
void Crash_extract_objs(struct obj_data *obj);
int Crash_is_unrentable(struct obj_data *obj);
void Crash_extract_norents(struct obj_data *obj);
void Crash_extract_expensive(struct obj_data *obj);
void Crash_calculate_rent(struct obj_data *obj, int *cost);
void Crash_rentsave(struct char_data *ch, int cost);
void Crash_cryosave(struct char_data *ch, int cost);
int Crash_load_objs(struct char_data *ch);
void tag_argument(char *argument, char *tag);
void strip_string(char *buffer);
int handle_obj(struct obj_data *obj, struct char_data *ch, int locate, struct obj_data **cont_rows);

/*
 * The following function is pulled from my genolc.c file.
 * Feel free to use any similar function
 */

/* Change BITSIZE to 1LL for bitvector_t of long long*/
#ifdef BITSIZE
#undef BITSIZE
#endif

#define BITSIZE 1

#ifdef BITSET
#undef BITSET
#endif

#define BITSET(bit_pattern, bit) (bit_pattern & (BITSIZE << bit))

struct obj_data *Obj_from_store(struct obj_file_elem object, int *location)
{
  struct obj_data *obj;
  int j;

  *location = 0;
  if (real_object(object.item_number) != NOTHING) {
    obj = read_object(object.item_number, VIRTUAL);
#if USE_AUTOEQ
    *location = object.location;
#endif
    GET_OBJ_VAL(obj, 0) = object.value[0];
    GET_OBJ_VAL(obj, 1) = object.value[1];
    GET_OBJ_VAL(obj, 2) = object.value[2];
    GET_OBJ_VAL(obj, 3) = object.value[3];
    GET_OBJ_EXTRA(obj) = object.extra_flags;
    GET_OBJ_WEIGHT(obj) = object.weight;
    GET_OBJ_TIMER(obj) = object.timer;
    obj->obj_flags.bitvector = object.bitvector;

    for (j = 0; j < MAX_OBJ_AFFECT; j++)
      obj->affected[j] = object.affected[j];

    return (obj);
  } else
    return (NULL);
}

/* This procedure removes the '\r\n' from a string so that it may be
   saved to a file.  Use it only on buffers, not on the orginal
   strings. */
void strip_string(char *buffer)
{
  register char *ptr, *str;

  ptr = buffer;
  str = ptr;

  while ((*str = *ptr)) {
    str++;
    ptr++;
    if (*ptr == '\r')
      ptr++;
  }
}

int Obj_to_store(struct obj_data *obj, FILE *fp, int locate)
{
  int counter2;
  struct extra_descr_data *ex_desc;
  char buf1[MAX_STRING_LENGTH +1];
  char flags[65];
  struct obj_data *temp = NULL;

  if (GET_OBJ_VNUM(obj) != NOTHING)
    temp=read_object(GET_OBJ_VNUM(obj), VIRTUAL);
  else {
    temp = create_obj();
    temp->item_number = -1;
  }

  if (obj->action_description) {
    strcpy(buf1, obj->action_description);
    strip_string(buf1);
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
  if (GET_OBJ_EXTRA(obj) != GET_OBJ_EXTRA(temp)) {
    sprintascii(flags, GET_OBJ_EXTRA(obj));
    fprintf(fp, "Flag: %s\n", flags);
  }

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
  if (TEST_OBJN(bitvector)) {
    sprintascii(flags, obj->obj_flags.bitvector);
    fprintf(fp, "Perm: %s\n", flags);
  }
  if (TEST_OBJN(wear_flags)) {
    sprintascii(flags, GET_OBJ_WEAR(obj));
    fprintf(fp, "Wear: %s\n", flags);
  }

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
        strip_string(buf1);
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


/*
 * AutoEQ by Burkhard Knopf <burkhard.knopf@informatik.tu-clausthal.de>
 */

void auto_equip(struct char_data *ch, struct obj_data *obj, int location)
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
        /*
         * Check the characters's alignment to prevent them from being
         * zapped through the auto-equipping.
         */
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
  char filename[50];
  FILE *fl;

  if (!get_filename(filename, sizeof(filename), CRASH_FILE, name))
    return (0);

  if (!(fl = fopen(filename, "r"))) {
    if (errno != ENOENT)  /* if it fails but NOT because of no file */
      log("SYSERR: deleting crash file %s (1): %s", filename, strerror(errno));
    return (0);
  }
  fclose(fl);

  /* if it fails, NOT because of no file */
  if (remove(filename) < 0 && errno != ENOENT)
    log("SYSERR: deleting crash file %s (2): %s", filename, strerror(errno));

  return (1);
}


int Crash_delete_crashfile(struct char_data *ch)
{
  char fname[MAX_INPUT_LENGTH];
  struct rent_info rent;
  int numread;
  FILE *fl;
  int rentcode,timed,netcost,gold,account,nitems;
  char line[MAX_INPUT_LENGTH];

  if (!get_filename(fname, sizeof(fname), CRASH_FILE, GET_NAME(ch)))
    return (0);
  
  if (!(fl = fopen(fname, "r"))) {
    if (errno != ENOENT)  /* if it fails, NOT because of no file */
      log("SYSERR: checking for crash file %s (3): %s", fname, strerror(errno));
    return (0);
  }
  numread = get_line(fl,line);
  fclose(fl);
  if (numread == FALSE)
    return (0);
  sscanf(line,"%d %d %d %d %d %d",&rentcode,&timed,&netcost,&gold,
         &account,&nitems);
  
  if (rentcode == RENT_CRASH)
    Crash_delete_file(GET_NAME(ch));

  if (rent.rentcode == RENT_CRASH)
    Crash_delete_file(GET_NAME(ch));

  return (1);
}


int Crash_clean_file(char *name)
{
  char fname[MAX_STRING_LENGTH], filetype[20];
  struct rent_info rent;
  int numread;
  FILE *fl;
  int rentcode, timed, netcost, gold, account, nitems;
  char line[MAX_STRING_LENGTH];

  if (!get_filename(fname, sizeof(fname), CRASH_FILE, name))
    return (0);

  /*
   * open for write so that permission problems will be flagged now, at boot
   * time.
   */
  if (!(fl = fopen(fname, "r"))) {
    if (errno != ENOENT)  /* if it fails, NOT because of no file */
      log("SYSERR: OPENING OBJECT FILE %s (4): %s", fname, strerror(errno));
    return (0);
  }

  numread = get_line(fl,line);
  fclose(fl);
  if (numread == FALSE)
    return (0);
  sscanf(line, "%d %d %d %d %d %d",&rentcode,&timed,&netcost,
         &gold,&account,&nitems);

  rentcode = rent.rentcode;
  timed = rent.time;
  
  if ((rentcode == RENT_CRASH) ||
      (rentcode == RENT_FORCED) || (rentcode == RENT_TIMEDOUT)) {
    if (timed < time(0) - (crash_file_timeout * SECS_PER_REAL_DAY)) {
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
      return (1);
    }
    /* Must retrieve rented items w/in 30 days */
  } else if (rentcode == RENT_RENTED)
    if (timed < time(0) - (rent_file_timeout * SECS_PER_REAL_DAY)) {
      Crash_delete_file(name);
      log("    Deleting %s's rent file.", name);
      return (1);
    }
  return (0);
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
  char fname[MAX_INPUT_LENGTH], buf[MAX_STRING_LENGTH];
  /*   struct obj_file_elem object; */
  struct obj_data *obj;
  struct rent_info rent;
  int rentcode,timed,netcost,gold,account,nitems;
  int nr;
  char line[MAX_STRING_LENGTH];
  int numread;
  
  if (!get_filename(fname, sizeof(fname), CRASH_FILE, name))
    return;
  
  if (!(fl = fopen(fname, "r"))) {
    send_to_char(ch, "%s has no rent file.\r\n", name);
    return;
  }
  sprintf(buf, "%s\r\n", fname);
  
  numread = get_line(fl, line);

  /* Oops, can't get the data, punt. */
  if (numread == FALSE) {
    send_to_char(ch, "Error reading rent information.\r\n");
    fclose(fl);
    return;
  }
  
  sscanf(line,"%d %d %d %d %d %d",&rentcode,&timed,&netcost,
         &gold,&account,&nitems);

  rentcode=rent.rentcode;

  switch (rentcode) {
  case RENT_RENTED:
    strcat(buf, "Rent\r\n");
    break;
  case RENT_CRASH:
    strcat(buf, "Crash\r\n");
    break;
  case RENT_CRYO:
    strcat(buf, "Cryo\r\n");
    break;
  case RENT_TIMEDOUT:
  case RENT_FORCED:
    strcat(buf, "TimedOut\r\n");
    break;
  default:
    strcat(buf, "Undef\r\n");
    break;
  }

  while(get_line(fl, line)) {
    if(*line == '#') { /* swell - its an item */
      sscanf(line,"#%d",&nr);
      if(nr != NOTHING) {  /* then we can dispense with it easily */
        obj=read_object(nr,VIRTUAL);
        if (!obj)
          continue;
        sprintf(buf,"%s[%5d] (%5dau) %-20s\r\n",buf,
                nr, GET_OBJ_RENT(obj),
                obj->short_description);
        extract_obj(obj);
      } else { /* its nothing, and a unique item. bleh. partial parse.*/
        /* Policy states that all NOTHING objs should be rent 0 */
      }
    }
  }

  page_string(ch->desc,buf,0);
  fclose(fl);
}


int Crash_write_rentcode(struct char_data *ch, FILE *fl, struct rent_info *rent)
{

  if(fprintf(fl,"%d %d %d %d %d %d\r\n",rent->rentcode, rent->time,
              rent->net_cost_per_diem,rent->gold,rent->account,rent->nitems) < 1) {
    perror("Syserr: Writing rent code");
    return (0);
  }

  return (1);
}


/*
 * Return values:
 *  0 - successful load, keep char in rent room.
 *  1 - load failure or load of crash items -- put char in temple.
 *  2 - rented equipment lost (no $)
 */
int Crash_load(struct char_data *ch)
{
  return (Crash_load_objs(ch));
}



int Crash_save(struct obj_data *obj, FILE *fp, int location)
{
  struct obj_data *tmp;
  int result;

  if (obj) {
    Crash_save(obj->next_content, fp, location);
    Crash_save(obj->contains, fp, MIN(0, location) - 1);

    result = Obj_to_store(obj, fp, location);

    for (tmp = obj->in_obj; tmp; tmp = tmp->in_obj)
      GET_OBJ_WEIGHT(tmp) -= GET_OBJ_WEIGHT(obj);

    if (!result)
      return (0);
  }
  return (TRUE);
}


void Crash_restore_weight(struct obj_data *obj)
{
  if (obj) {
    Crash_restore_weight(obj->contains);
    Crash_restore_weight(obj->next_content);
    if (obj->in_obj)
      GET_OBJ_WEIGHT(obj->in_obj) += GET_OBJ_WEIGHT(obj);
  }
}

/*
 * Get !RENT items from equipment to inventory and
 * extract !RENT out of worn containers.
 */
void Crash_extract_norent_eq(struct char_data *ch)
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

void Crash_extract_objs(struct obj_data *obj)
{
  if (obj) {
    Crash_extract_objs(obj->contains);
    Crash_extract_objs(obj->next_content);
    extract_obj(obj);
  }
}


int Crash_is_unrentable(struct obj_data *obj)
{
  if (!obj)
    return (0);

  if (OBJ_FLAGGED(obj, ITEM_NORENT) ||
      GET_OBJ_RENT(obj) < 0 ||
      GET_OBJ_RNUM(obj) == NOTHING ||
      GET_OBJ_TYPE(obj) == ITEM_KEY) {
 log("Crash_is_unrentable: removing object %s", obj->short_description);
    return (1);
  }

  return (0);
}


void Crash_extract_norents(struct obj_data *obj)
{
  if (obj) {
    Crash_extract_norents(obj->contains);
    Crash_extract_norents(obj->next_content);
    if (Crash_is_unrentable(obj))
      extract_obj(obj);
  }
}


void Crash_extract_expensive(struct obj_data *obj)
{
  struct obj_data *tobj, *max;

  max = obj;
  for (tobj = obj; tobj; tobj = tobj->next_content)
    if (GET_OBJ_RENT(tobj) > GET_OBJ_RENT(max))
      max = tobj;
  extract_obj(max);
}



void Crash_calculate_rent(struct obj_data *obj, int *cost)
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
  struct rent_info rent;
  int j;
  FILE *fp;

  if (IS_NPC(ch))
    return;

  if (!get_filename(buf, sizeof(buf), CRASH_FILE, GET_NAME(ch)))
    return;
  
  if (!(fp = fopen(buf, "w")))
    return;

  rent.rentcode = RENT_CRASH;
  rent.time = time(0);

  fprintf(fp,"%d %d %d %d %d %d\r\n",rent.rentcode,rent.time,
           rent.net_cost_per_diem,rent.gold,rent.account,rent.nitems);

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
  REMOVE_BIT(PLR_FLAGS(ch), PLR_CRASH);
}


void Crash_idlesave(struct char_data *ch)
{
  char buf[MAX_INPUT_LENGTH];
  struct rent_info rent;
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
  rent.net_cost_per_diem = cost;

  rent.rentcode = RENT_TIMEDOUT;
  rent.time = time(0);
  rent.gold = GET_GOLD(ch);
  rent.account = GET_BANK_GOLD(ch);

  fprintf(fp,"%d %d %d %d %d %d\r\n",rent.rentcode,rent.time,
           rent.net_cost_per_diem,rent.gold,rent.account,rent.nitems);
  
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
  struct rent_info rent;
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

  rent.net_cost_per_diem = cost;
  rent.rentcode = RENT_RENTED;
  rent.time = time(0);
  rent.gold = GET_GOLD(ch);
  rent.account = GET_BANK_GOLD(ch);

  fprintf(fp,"%d %d %d %d %d %d\r\n",rent.rentcode,rent.time,
           rent.net_cost_per_diem,rent.gold,rent.account,rent.nitems);

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


void Crash_cryosave(struct char_data *ch, int cost)
{
  char buf[MAX_INPUT_LENGTH];
  struct rent_info rent;
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

  rent.rentcode = RENT_CRYO;
  rent.time = time(0);
  rent.gold = GET_GOLD(ch);
  rent.account = GET_BANK_GOLD(ch);
  rent.net_cost_per_diem = 0;

  fprintf(fp,"%d %d %d %d %d %d\r\n",rent.rentcode,rent.time,
           rent.net_cost_per_diem,rent.gold,rent.account,rent.nitems);


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
  SET_BIT(PLR_FLAGS(ch), PLR_CRYO);
}


/* ************************************************************************
* Routines used for the receptionist                                     * 
************************************************************************* */

void Crash_rent_deadline(struct char_data *ch, struct char_data *recep,
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

int Crash_report_unrentables(struct char_data *ch, struct char_data *recep,
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



void Crash_report_rent(struct char_data *ch, struct char_data *recep,
                       struct obj_data *obj, long *cost, long *nitems, 
                       int display, int factor)
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



int Crash_offer_rent(struct char_data *ch, struct char_data *receptionist,
                     int display, int factor)
{
  char buf[MAX_INPUT_LENGTH];
  int i;
  long totalcost = 0, numitems = 0, norent;

  norent = Crash_report_unrentables(ch, receptionist, ch->carrying);
  for (i = 0; i < NUM_WEARS; i++)
    norent += Crash_report_unrentables(ch, receptionist, GET_EQ(ch, i));

  if (norent)
    return (0);

  totalcost = min_rent_cost * factor;

  Crash_report_rent(ch, receptionist, ch->carrying, &totalcost, &numitems, display, factor);

  for (i = 0; i < NUM_WEARS; i++)
    Crash_report_rent(ch, receptionist, GET_EQ(ch, i), &totalcost, &numitems, display, factor);

  if (!numitems) {
    act("$n tells you, 'But you are not carrying anything!  Just quit!'",
        FALSE, receptionist, 0, ch, TO_VICT);
    return (0);
  }
  if (numitems > max_obj_save) {
    sprintf(buf, "$n tells you, 'Sorry, but I cannot store more than %d items.'",
            max_obj_save);
    act(buf, FALSE, receptionist, 0, ch, TO_VICT);
    return (0);
  }
  if (display) {
    sprintf(buf, "$n tells you, 'Plus, my %d coin fee..'",
            min_rent_cost * factor);
    act(buf, FALSE, receptionist, 0, ch, TO_VICT);
    sprintf(buf, "$n tells you, 'For a total of %ld coins%s.'",
            totalcost, (factor == RENT_FACTOR ? " per day" : ""));
    act(buf, FALSE, receptionist, 0, ch, TO_VICT);
    if (totalcost > GET_GOLD(ch) + GET_BANK_GOLD(ch)) {
      act("$n tells you, '...which I see you can't afford.'",
          FALSE, receptionist, 0, ch, TO_VICT);
      return (0);
    } else if (factor == RENT_FACTOR)
      Crash_rent_deadline(ch, receptionist, totalcost);
  }
  return (totalcost);
}

int gen_receptionist(struct char_data *ch, struct char_data *recep,
		         int cmd, char *arg, int mode)
{
  int cost;
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

  if (free_rent) {
    act("$n tells you, 'Rent is free here.  Just quit, and your objects will be saved!'",
	FALSE, recep, 0, ch, TO_VICT);
    return (1);
  }

  if (CMD_IS("rent")) {
    char buf[128];

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
      SET_BIT(PLR_FLAGS(ch), PLR_CRYO);
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
        REMOVE_BIT(PLR_FLAGS(d->character), PLR_CRASH);
      }
    }
  }
}


int Crash_load_objs(struct char_data *ch) {
  FILE *fl;
  char fname[MAX_STRING_LENGTH];
  char line[256];
  int t[10],i,num_of_days;
  int orig_rent_code;
  struct obj_data *temp;
  int locate=0, nr,cost,num_objs=0;
  struct obj_data *cont_row[MAX_BAG_ROWS];
  int rentcode,timed,netcost,gold,account,nitems;

  if (!get_filename(fname, sizeof(fname), CRASH_FILE, GET_NAME(ch)))
    return 1;

  for (i = 0; i < MAX_BAG_ROWS; i++)
    cont_row[i] = NULL;

  if (!(fl = fopen(fname, "r"))) {
    if (errno != ENOENT) { /* if it fails, NOT because of no file */
      char buf[MAX_STRING_LENGTH];
      sprintf(buf, "SYSERR: READING OBJECT FILE %s (5)", fname);
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
    char str[64];
    sprintf(str, "%d", SECS_PER_REAL_DAY);
    num_of_days = (int)((float) (time(0) - timed) / (float)atoi(str));
    cost = (int) (netcost * num_of_days);
    if (cost > GET_GOLD(ch) + GET_BANK_GOLD(ch)) {
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

  temp = NULL;
  while (get_line(fl, line)) {
    char tag[6];
    int num;

    /* first, we get the number. Not too hard. */
    if(*line == '$' && line[1] == '~') {
      if (temp)
        num_objs += handle_obj(temp, ch, locate, cont_row);
      break;
    }
    if (*line == '#') {
      if (sscanf(line, "#%d", &nr) != 1) {
        continue;
      } else {
        if (temp)
          num_objs += handle_obj(temp, ch, locate, cont_row);
        temp = NULL;
        locate = 0;
      }
      /* we have the number, check it, load obj. */
      if (nr == NOTHING) {   /* then it is unique */
        temp = create_obj();
        temp->item_number=NOTHING;
      } else if (nr < 0) {
        continue;
      } else {
        if(nr >= 999999) 
          continue;
        if(real_object(nr) != NOTHING) {
          temp=read_object(nr,VIRTUAL);
          if (!temp) {
            continue;
          }
        } else {
          log("Nonexistent object %d found in rent file.", nr);
          continue;
        }
      }
    }

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
      if (!strcmp(tag, "Flag"))
        GET_OBJ_EXTRA(temp) = asciiflag_conv(line);
      break;
    case 'L':
      if(!strcmp(tag, "Loc "))
        locate = num;
      break;
    case 'N':
      if (!strcmp(tag, "Name"))
        temp->name = strdup(line);
      break;
    case 'P':
      if (!strcmp(tag, "Perm"))
        temp->obj_flags.bitvector = asciiflag_conv(line);
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
      if (!strcmp(tag, "Wear"))
        GET_OBJ_WEAR(temp) = asciiflag_conv(line);
      else if (!strcmp(tag, "Wght"))
        GET_OBJ_WEIGHT(temp) = num;
      break;
    case 'V':
      if (!strcmp(tag, "Vals")) {
        sscanf(line, "%d %d %d %d", &t[0], &t[1], &t[2], &t[3]);
        for (i = 0; i < 4; i++)
          GET_OBJ_VAL(temp, i) = t[i];
      }
      break;
    }
  }
 
  /* Little hoarding check. -gg 3/1/98 */
 mudlog(NRM, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE, "%s (level %d) has %d objects (max %d).", 
         GET_NAME(ch), GET_LEVEL(ch), num_objs, max_obj_save);

  fclose(fl);

  if ((orig_rent_code == RENT_RENTED) || (orig_rent_code == RENT_CRYO))
    return 0;
  else
    return 1;
}


int handle_obj(struct obj_data *temp, struct char_data *ch, int locate, struct obj_data **cont_row)
{
  int j;
  struct obj_data *obj1;

  if (!temp)  /* this should never happen, but.... */
    return (0);

  auto_equip(ch, temp, locate);

  /* 
     what to do with a new loaded item:

     if there's a list with <locate> less than 1 below this:
     (equipped items are assumed to have <locate>==0 here) then its
     container has disappeared from the file   *gasp*
     -> put all the list back to ch's inventory
     if there's a list of contents with <locate> 1 below this:
     check if it's a container
     - if so: get it from ch, fill it, and give it back to ch (this way the
     container has its correct weight before modifying ch)
     - if not: the container is missing -> put all the list to ch's inventory

     for items with negative <locate>:
     if there's already a list of contents with the same <locate> put obj to it
     if not, start a new list

     Confused? Well maybe you can think of some better text to be put here ...

     since <locate> for contents is < 0 the list indices are switched to
     non-negative
  */

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

  return (1);
}

