/**************************************************************************
*  File: hsedit.c                                          Part of tbaMUD *
*  Usage: Oasis OLC - Houses.                                             *
*                                                                         *
* Copyright 1996 Harvey Gilpin. 1997-2001 George Greer.                   *
* Copyright 2007-2010 Stefan Cole and tbaMUD                              *
**************************************************************************/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "comm.h"
#include "utils.h"
#include "handler.h"
#include "interpreter.h"
#include "db.h"
#include "boards.h"
#include "oasis.h"
#include "genolc.h"
#include "genzon.h"
#include "constants.h"
#include "house.h"
#include "screen.h"

/* Internal (static) functions */
static void hsedit_setup_new(struct descriptor_data *d);
static void hsedit_setup_existing(struct descriptor_data *d, struct house_control_data *hse);
static void hsedit_save_to_disk(struct descriptor_data *d);
static void hsedit_save_internally(struct descriptor_data *d);
static void hedit_delete_house(struct descriptor_data *d, int house_vnum);
static void hsedit_disp_flags_menu(struct descriptor_data * d);
static void hsedit_owner_menu(struct descriptor_data *d);
static void hsedit_dir_menu(struct descriptor_data *d);
static void hsedit_disp_type_menu(struct descriptor_data *d);
static void hsedit_disp_guest_menu(struct descriptor_data *d);
static char *hsedit_list_guests(struct house_control_data *thishouse, char *guestlist);
static void hsedit_disp_menu(struct descriptor_data * d);

/* External Globals */
extern struct house_control_data *house_control;

/*------------------------------------------------------------------------*/
static void hsedit_setup_new(struct descriptor_data *d)
{
  OLC_HOUSE(d) = new_house();

  OLC_HOUSE(d)->vnum = OLC_NUM(d);
  OLC_HOUSE(d)->built_by = GET_IDNUM(d->character);

  OLC_VAL(d) = 0;
  hsedit_disp_menu(d);
}

/*------------------------------------------------------------------------*/
static void hsedit_setup_existing(struct descriptor_data *d, struct house_control_data *hse)
{
  struct house_control_data *house;
  struct guest_data *g, *next_g;
  int i;

  /*. Build a copy of the house .*/
  CREATE (house, struct house_control_data, 1);

  /* allocate space for all strings  */

  /* Copy all variables */
  house->vnum           = hse->vnum;
  house->atrium         = hse->atrium;
  house->owner          = hse->owner;
  house->exit_num       = hse->exit_num;
  house->built_on       = hse->built_on;
  house->built_by       = hse->built_by;
  house->mode           = hse->mode;
  house->last_payment   = hse->last_payment;
  house->receptionist   = hse->receptionist;
  house->next           = NULL;

  /* Copy the house flags */
  for (i=0; i < HS_ARRAY_MAX; i++)
    house->house_flags[i] = hse->house_flags[i];

  /* Copy the guest list too */
  for (g=hse->guests; g; g = next_g) {
    next_g = g->next;
    add_house_guest(house, g->id);
  }

  /*. Attach house copy to players descriptor .*/
  OLC_HOUSE(d) = house;
  OLC_VAL(d) = 0;
  hsedit_disp_menu(d);
}

/*-----------------------------------------------------------1-------------*/
static void hsedit_save_to_disk(struct descriptor_data *d)
{
  /* Save all houses */
  House_save_control();
}

/*-----------------------------------------------------------1-------------*/
static void hsedit_save_internally(struct descriptor_data *d)
{
  struct house_control_data *hse;
  struct guest_data *g, *next_g;
  int i;

  hse = find_house(OLC_NUM(d));
  if (hse != NULL)
  {
    /* This house VNUM is already in the list */
    /* Replace the old data                   */
    *hse = *OLC_HOUSE(d);
    hse->vnum           = OLC_HOUSE(d)->vnum;
    hse->atrium         = OLC_HOUSE(d)->atrium;
    hse->owner          = OLC_HOUSE(d)->owner;
    hse->exit_num       = OLC_HOUSE(d)->exit_num;
    hse->built_on       = OLC_HOUSE(d)->built_on;
    hse->built_by       = OLC_HOUSE(d)->built_by;
    hse->mode           = OLC_HOUSE(d)->mode;
    hse->last_payment   = OLC_HOUSE(d)->last_payment;
    hse->receptionist   = OLC_HOUSE(d)->receptionist;

    /* Copy flags */
    for (i=0; i<HS_ARRAY_MAX; i++)
      hse->house_flags[i] = OLC_HOUSE(d)->house_flags[i];

    /* Replace guest list with new one */
    free_house_guests(hse);
    for (g=OLC_HOUSE(d)->guests; g; g = next_g) {
      next_g = g->next;
      add_house_guest(hse, g->id);
    }
  } else {
    /*. House doesn't exist, hafta add it .*/
    if (count_houses() < MAX_HOUSES) {
      hse = add_house(OLC_HOUSE(d));
    } else {
      send_to_char(d->character, "MAX House limit reached - Unable to save this house!");
      mudlog(NRM, ADMLVL_IMPL, TRUE, "HSEDIT: Max houses limit reached - Unable to save OLC data");
      hse = NULL;
    }
  }
  /* The new house is stored - now to ensure the roomsflags are correct */
  if (hse) {
    set_house(OLC_HOUSE(d));
  }
//  olc_add_to_save_list(zone_table[OLC_ZNUM(d)].number, OLC_SAVE_HOUSE);
}

/*------------------------------------------------------------------------*/
static void hedit_delete_house(struct descriptor_data *d, int house_vnum)
{
  struct house_control_data *hse, *next_hse = NULL;
  room_rnum real_atrium, real_house;

  if ((hse = find_house(house_vnum)) == NULL) {
    mudlog(BRF, ADMLVL_IMPL, TRUE, "SYSERR: hsedit: Invalid house vnum in hedit_delete_house\r\n");
    cleanup_olc(d, CLEANUP_STRUCTS);
    return;
  }

  if ((real_atrium = real_room(hse->atrium)) == NOWHERE)
    log("SYSERR: House %d had invalid atrium %d!", house_vnum, hse->atrium);
  else
    REMOVE_BIT_AR(ROOM_FLAGS(real_atrium), ROOM_ATRIUM);

  if ((real_house = real_room(hse->vnum)) == NOWHERE)
    log("SYSERR: House %d had invalid vnum %d!", house_vnum, hse->vnum);
  else {
    REMOVE_BIT_AR(ROOM_FLAGS(real_house), ROOM_HOUSE);
    REMOVE_BIT_AR(ROOM_FLAGS(real_house), ROOM_PRIVATE);
    REMOVE_BIT_AR(ROOM_FLAGS(real_house), ROOM_HOUSE_CRASH);
  }

  House_delete_file(hse->vnum);

  delete_house_control(hse);

  send_to_char(d->character, "House deleted.\r\n");
  House_save_control();

  /*
   * Now, reset the ROOM_ATRIUM flag on all existing houses' atriums,
   * just in case the house we just deleted shared an atrium with another
   * house.  --JE 9/19/94
   */
  for (hse = house_control; hse; hse = next_hse) {
    next_hse = hse->next;
    if ((real_atrium = real_room(hse->atrium)) != NOWHERE)
      SET_BIT_AR(ROOM_FLAGS(real_atrium), ROOM_ATRIUM);
  }

  cleanup_olc(d, CLEANUP_ALL);
}

/**************************************************************************
 HSEdit House OLC Menu functions
**************************************************************************/

static void hsedit_disp_flags_menu(struct descriptor_data * d)
{
  int counter, columns = 0;
  char buf[MAX_STRING_LENGTH];
  struct char_data *ch = d->character;

  clear_screen(d);
  for (counter = 0; counter < HOUSE_NUM_FLAGS; counter++)
  {
    send_to_char(d->character, "%s%2d%s) %-20.20s ",
       QBGRN, counter + 1, QNRM, house_bits[counter]);
    if(!(++columns % 2))
      send_to_char(d->character, "\r\n");
  }
  sprintbitarray(OLC_HOUSE(d)->house_flags, house_bits, HS_ARRAY_MAX, buf);
  send_to_char(d->character,
     "\r\nHouse flags: %s%s%s\r\n"
     "Enter house flags, 0 to quit : ", QCYN, buf, QNRM);

  OLC_MODE(d) = HSEDIT_FLAGS;
}

/*------------------------------------------------------------------------*/
static void hsedit_owner_menu(struct descriptor_data *d)
{
  struct house_control_data *house;
  struct char_data *ch = d->character;
  char ownr[MAX_NAME_LENGTH+2];

  house = OLC_HOUSE(d);

  sprintf(ownr, "%s", (get_name_by_id(house->owner) == NULL) ? "<Not Set!>" : CAP(get_name_by_id(house->owner)));

  send_to_char(ch,
   "%s1%s) Owner Name : %s%s%s\r\n"
   "%s2%s) Owner ID   : %s%ld%s\r\n"
     "%sQ%s) Back to main menu\r\n"
     "Enter choice : ",
   QBCYN, QNRM, QYEL, ownr, QNRM,
   QBCYN, QNRM, QYEL, house->owner, QNRM,
   QBCYN, QNRM
  );

  OLC_MODE(d) = HSEDIT_OWNER_MENU;
}

/*------------------------------------------------------------------------*/
static void hsedit_dir_menu(struct descriptor_data *d)
{
  struct house_control_data *house;
  struct char_data *ch = d->character;
  int house_rnum, newroom[NUM_OF_DIRS], i;
  char dir[20];

  house = OLC_HOUSE(d);
  house_rnum = real_room(house->vnum);

  if (house_rnum == NOWHERE)
  {
    /* Should never reach here, as hsedit command specified vnum, but just in case */
    send_to_char(ch,
      "%sWARNING%s: %sYou cannot set an atium direction before selecting a valid room vnum%s\r\n"
      "(Press Enter)\r\n", QBRED, QNRM, QYEL, QNRM );

    OLC_MODE(d) = HSEDIT_NOVNUM;
  } else {
    /* Grab exit rooms */
    for(i=0;i<DIR_COUNT;i++)
    {
      if (world[house_rnum].dir_option[i])
        newroom[i] = world[house_rnum].dir_option[i]->to_room;
      else
        newroom[i] = NOWHERE;
    }

    for (i=0; i<DIR_COUNT; i++) {
      sprintf(dir, "%s", dirs[i]);
      send_to_char(ch,
          "%s%d%s) %-10s  : (%s%s%s)\r\n",
          QBCYN, (i+1), QNRM, CAP(dir), QCYN,
          (newroom[i] == NOWHERE ? "<No Room!>" : world[(newroom[i])].name), QNRM);
    }
    send_to_char(ch,
        "%sQ%s) Back to main menu\r\n"
        "Enter atrium direction : ", QBCYN, QNRM );

    OLC_MODE(d) = HSEDIT_DIR_MENU;
  }
}

/*------------------------------------------------------------------------*/
static void hsedit_disp_type_menu(struct descriptor_data *d)
{
  int counter, columns = 0;
  struct char_data *ch = d->character;

  clear_screen(d);
  for (counter = 0; counter < NUM_HOUSE_TYPES; counter++) {
    send_to_char(d->character, "%s%2d%s) %-20.20s ",
           QBCYN, counter, QNRM, house_types[counter]);
    if(!(++columns % 2))
      send_to_char(d->character, "\r\n");
  }
  send_to_char(d->character, "\r\nEnter house type : ");
  OLC_MODE(d) = HSEDIT_TYPE;
}

/*------------------------------------------------------------------------*/
static void hsedit_disp_guest_menu(struct descriptor_data *d)
{
  char not_set[128];
  struct char_data *ch = d->character;
  struct house_control_data *house;
  struct guest_data *g, *next_g;
  int count=1;

  house = OLC_HOUSE(d);

  sprintf(not_set, "%s<Not Set!>%s", QYEL, QNRM);

  for (g=OLC_HOUSE(d)->guests; g; g = next_g) {
    next_g = g->next;
    send_to_char(ch,
        "%s%2d%s) %s%s%s (%sID: %ld%s)\r\n",
        QBCYN, count++, QNRM,
        QYEL, get_name_by_id(g->id) == NULL ? not_set : get_name_by_id(g->id), QNRM,
        QCYN, (g->id) < 1 ? 0 : (g->id), QNRM);
  }
  send_to_char(ch,
     "%sA%s) Add a guest\r\n"
     "%sD%s) Delete a guest\r\n"
     "%sC%s) Clear guest list\r\n"
     "%sQ%s) Back to main menu\r\n"
     "Enter selection (A/D/C/Q): ",
     QBCYN, QNRM,
     QBCYN, QNRM,
     QBCYN, QNRM,
     QBCYN, QNRM);

  OLC_MODE(d) = HSEDIT_GUEST_MENU;
}

/*------------------------------------------------------------------------*/
static char *hsedit_list_guests(struct house_control_data *thishouse, char *guestlist)
{
  int num_printed;
  char *temp;
  struct guest_data *g, *next_g = NULL;

  if (count_house_guests(thishouse) == 0) {
    sprintf(guestlist, "<None!>");
    return(guestlist);
  }

  for (num_printed = 0, g = thishouse->guests; g; g = next_g) {
    next_g = g->next;
    /* Avoid <UNDEF>. -gg 6/21/98 */
    if ((temp = get_name_by_id(g->id)) == NULL)
      continue;

    num_printed++;
    sprintf(guestlist, "%s%c%s ", guestlist, UPPER(*temp), temp + 1);
  }

  if (num_printed == 0)
    sprintf(guestlist, "all dead");

  return(guestlist);
}

/*------------------------------------------------------------------------*/
/* the main menu */
static void hsedit_disp_menu(struct descriptor_data * d)
{
  char buf[MAX_STRING_LENGTH], buf1[MAX_STRING_LENGTH], mob_name[MAX_INPUT_LENGTH];
  char built_on[128], last_pay[128], buf2[MAX_STRING_LENGTH], atrium[20];
  char *timestr, no_name[128];
  struct char_data *ch = d->character;
  struct house_control_data *house;
  mob_rnum recep;

  clear_screen(d);
  house = OLC_HOUSE(d);

  if (house->built_on) {
    timestr = asctime(localtime(&(house->built_on)));
    *(timestr + 10) = '\0';
    strlcpy(built_on, timestr, sizeof(built_on));
  } else
    strcpy(built_on, "Unknown");   /* strcpy: OK (for 'strlen("Unknown") < 128') */

  if (house->last_payment) {
    timestr = asctime(localtime(&(house->last_payment)));
    *(timestr + 10) = '\0';
    strlcpy(last_pay, timestr, sizeof(last_pay));
  } else
    strcpy(last_pay, "None");   /* strcpy: OK (for 'strlen("None") < 128') */

  if (house->atrium == NOWHERE) {
    sprintf(atrium, "<Not Set!>");
  } else {
    sprintf(atrium, "%d", house->atrium);
  }

  *buf2 = '\0';
  sprintbitarray((house->house_flags), house_bits, HS_ARRAY_MAX, buf1);
  sprintf(no_name, "%s<Nobody!>%s", QCYN, QNRM);
  recep = real_mobile(house->receptionist);
  sprintf(mob_name, "%s", (recep == NOBODY) ? "<Not Set!>" : mob_proto[(recep)].player.short_descr);
  sprintf(buf,
   "%s-- House OLC Editor for tbaMUD --%s\r\n"
   "-- House number : %s[%s%d%s]%s     House zone: %s[%s%d%s]%s\r\n"
   "%s1%s) Owner       : %s%ld%s -- %s%s%s\r\n"
   "%s2%s) Atrium      : %s%s%s\r\n"
   "%s3%s) Direction   : %s%s%s\r\n"
   "%s4%s) House Type  : %s%s%s\r\n"
   "%s5%s) Built on    : %s%s%s\r\n"
   "%s6%s) Payment     : %s%s%s\r\n"
   "%s7%s) Guests      : %s%s%s\r\n"
   "%s8%s) Flags       : %s%s%s\r\n"
   "%s9%s) Receptionist: %s[%s%d%s] %s%s\r\n"
   "%sX%s) Delete this house\r\n"
   "%sQ%s) Quit\r\n"
   "Enter choice : ",

      QGRN, QNRM,
      QCYN, QYEL, OLC_NUM(d), QCYN, QNRM,
      QCYN, QYEL, zone_table[OLC_ZNUM(d)].number, QCYN, QNRM,
      QBCYN, QNRM, QYEL, house->owner, QNRM, QCYN, get_name_by_id(house->owner) == NULL ? no_name : get_name_by_id(house->owner), QNRM,
      QBCYN, QNRM, QYEL, atrium, QNRM,
      QBCYN, QNRM, QCYN, ((house->exit_num >= 0) && (house->exit_num <= DIR_COUNT)) ? dirs[(house->exit_num)] : "<None!>", QNRM,
      QBCYN, QNRM, QCYN, house_types[(house->mode)], QNRM,
      QBCYN, QNRM, QYEL, built_on, QNRM,
      QBCYN, QNRM, QYEL, last_pay, QNRM,
      QBCYN, QNRM, QYEL, hsedit_list_guests(house, buf2), QNRM,
      QBCYN, QNRM, QYEL, buf1, QNRM,
      QBCYN, QNRM, QCYN, QYEL, house->receptionist, QCYN, mob_name, QNRM,
      QBCYN, QNRM,
      QBCYN, QNRM
  );
  send_to_char(d->character, "%s", buf);

  OLC_MODE(d) = HSEDIT_MAIN_MENU;
}

/**************************************************************************
  The main loop
**************************************************************************/
void hsedit_parse(struct descriptor_data * d, char *arg)
{
  int number=0, id=0, i, room_rnum;
  char *tmp;
  bool found=FALSE;

  mudlog(CMP, ADMLVL_IMPL, FALSE, "(LOG) hsedit_parse: OLC mode %d", OLC_MODE(d));

  switch (OLC_MODE(d)) {
  case HSEDIT_CONFIRM_SAVESTRING:
    switch (*arg) {
    case 'y':
    case 'Y':
      hsedit_save_internally(d);
      mudlog(CMP, ADMLVL_BUILDER, TRUE, "OLC: %s edits house %d", GET_NAME(d->character), OLC_NUM(d));
      if (CONFIG_OLC_SAVE) {
        hsedit_save_to_disk(d);
        write_to_output(d, "House saved to disk.\r\n");
      } else
        write_to_output(d, "House saved to memory.\r\n");
      /*. Do NOT free strings! just the room structure .*/
      cleanup_olc(d, CLEANUP_STRUCTS);
      break;
    case 'n':
    case 'N':
      /* free everything up, including strings etc */
      cleanup_olc(d, CLEANUP_ALL);
      break;
    default:
      send_to_char(d->character, "Invalid choice!\r\n");
      send_to_char(d->character, "Do you wish to save this house internally? : ");
      break;
    }
    return;

  case HSEDIT_MAIN_MENU:
    switch (*arg) {
    case 'q':
    case 'Q':
      if (OLC_VAL(d))
      { /*. Something has been modified .*/
        send_to_char(d->character, "Do you wish to save this house internally? : ");
        OLC_MODE(d) = HSEDIT_CONFIRM_SAVESTRING;
      } else
        cleanup_olc(d, CLEANUP_ALL);
      return;
    case '1':
      hsedit_owner_menu(d);
      break;

    case '2':
      if ((OLC_HOUSE(d)->vnum == NOWHERE) || (real_room(OLC_HOUSE(d)->vnum) == NOWHERE))
      {
        send_to_char(d->character, "ERROR: Invalid house VNUM\r\n(Press Enter)\r\n");
        mudlog(NRM, ADMLVL_GRGOD, TRUE, "SYSERR: Invalid house VNUM in hsedit");
      }
      else
      {
        send_to_char(d->character, "Enter atrium room vnum:");
        OLC_MODE(d) = HSEDIT_ATRIUM;
      }
      break;

    case '3':
      if ((OLC_HOUSE(d)->vnum == NOWHERE) || (real_room(OLC_HOUSE(d)->vnum) == NOWHERE))
      {
        send_to_char(d->character, "ERROR: Invalid house VNUM\r\n(Press Enter)\r\n");
        mudlog(NRM, ADMLVL_GRGOD, TRUE, "SYSERR: Invalid house VNUM in hsedit");
      }
      else
      {
        hsedit_dir_menu(d);
      }
      break;

    case '4':
      hsedit_disp_type_menu(d);
      break;

    case '5':
      send_to_char(d->character, "Set build date to now? (Y/N):");
      OLC_MODE(d) = HSEDIT_BUILD_DATE;
      break;

    case '6':
      send_to_char(d->character, "Set last payment as now? (Y/N): ");
      OLC_MODE(d) = HSEDIT_PAYMENT;
      break;

    case '7':
      hsedit_disp_guest_menu(d);
      break;

    case '8':
      hsedit_disp_flags_menu(d);
      break;

    case '9':
      if (OLC_HOUSE(d)->mode == HOUSE_SHOP)
        send_to_char(d->character, "Enter the VNUM of the shopkeeper mobile: ");
      else
        send_to_char(d->character, "Enter the VNUM of the receptionist mobile: ");
      OLC_MODE(d) = HSEDIT_RECEP;
      break;

    case 'x':
    case 'X':
      send_to_char(d->character, "Are you sure you want to delete this house? (Y/N) : ");
      OLC_MODE(d) = HSEDIT_DELETE;
      break;

    default:
      send_to_char(d->character, "Invalid choice!\r\n");
      hsedit_disp_menu(d);
      break;
    }
    return;

  case HSEDIT_OWNER_MENU:
    switch (*arg)
    {
      case '1':
        send_to_char(d->character, "Enter the name of the owner : ");
        OLC_MODE(d) = HSEDIT_OWNER_NAME;
        break;

      case '2':
        send_to_char(d->character, "Enter the user id of the owner : ");
        OLC_MODE(d) = HSEDIT_OWNER_ID;
        break;

      case 'Q':
        hsedit_disp_menu(d);
        break;
    }
    return;
    break;

  case HSEDIT_OWNER_NAME:
    if ((id = get_id_by_name(arg)) < 0)
    {
      send_to_char(d->character, "There is no such player.\r\n");
      hsedit_owner_menu(d);
      return;
    }
    else
    {
      OLC_HOUSE(d)->owner = id;
    }
    break;

  case HSEDIT_OWNER_ID:
    id = atoi(arg);
    if ((tmp = get_name_by_id(id)) == NULL)
    {
      send_to_char(d->character, "There is no such player.\r\n");
      hsedit_owner_menu(d);
      return;
    }
    else
    {
      OLC_HOUSE(d)->owner = id;
    }
    break;

  case HSEDIT_ATRIUM:
    number = atoi(arg);
    if (number == 0)
    {
      /* '0' chosen - go back to main menu */
      hsedit_disp_menu(d);
      return;
    }
    room_rnum = real_room(OLC_HOUSE(d)->vnum);
    if (real_room(number) == NOWHERE)
    {
      send_to_char(d->character, "Room VNUM does not exist.\r\nEnter a valid room VNUM for this atrium (0 to exit) : ");
      return;
    }
    else
    {
      for (i=0; i<DIR_COUNT; i++)
      {
        if (world[room_rnum].dir_option[i])
        {
          if (world[room_rnum].dir_option[i]->to_room == real_room(number))
          {
            found=TRUE;
            id = i;
          }
        }
      }

      if (found == FALSE)
      {
        send_to_char(d->character, "Atrium MUST be an adjoining room.\r\nEnter a valid room VNUM for this atrium (0 to exit) : ");
        return;
      }
      else
      {
        OLC_HOUSE(d)->atrium = number;
        OLC_HOUSE(d)->exit_num = id;
      }
    }
    break;

  case HSEDIT_DIR_MENU:

    number = atoi(arg)-1;

    if ((*arg == 'q') || (*arg == 'Q') || (number == -1))
    {
      hsedit_disp_menu(d);
      return;
    }
    if ((number < 0) || (number > 5))
    {
      send_to_char(d->character, "Invalid choice, Please select a direction (1-6, Q to quit) : ");
      return;
    }
    id = real_room(OLC_HOUSE(d)->vnum);
    if (!(world[id].dir_option[number]))
    {
      send_to_char(d->character, "%sYou cannot set the atrium to a room that doesn't exist!%s\r\n", CBRED(d->character, C_NRM), CCNRM(d->character, C_NRM));
      hsedit_dir_menu(d);
      return;
    }
    else if ((world[id].dir_option[number]->to_room) == NOWHERE)
    {
      send_to_char(d->character, "%sYou cannot set the atrium to nowhere!%s\r\n", CBRED(d->character, C_NRM), CCNRM(d->character, C_NRM));
      hsedit_dir_menu(d);
      return;
    }
    else
    {
      OLC_HOUSE(d)->exit_num = number;

      room_rnum = world[id].dir_option[number]->to_room;
      OLC_HOUSE(d)->atrium = world[room_rnum].number;
    }
    break;

  case HSEDIT_NOVNUM:
    /* Just an 'enter' keypress - don't do anything */
    break;

  case HSEDIT_BUILD_DATE:
    switch (*arg)
    {
      case 'y':
      case 'Y':
        OLC_HOUSE(d)->built_on = time(0);
        break;
      case 'n':
      case 'N':
        send_to_char(d->character, "Build Date not changed\r\n");
        hsedit_disp_menu(d);
        return;
        break;
    }
    break;

  case HSEDIT_DELETE:
    switch (*arg)
    {
      case 'y':
      case 'Y':
        hedit_delete_house(d, OLC_HOUSE(d)->vnum);
        break;
      case 'n':
      case 'N':
        send_to_char(d->character, "House not deleted!\r\n");
        hsedit_disp_menu(d);
        return;
        break;
    }
    break;

  case HSEDIT_BUILDER:
    if ((id = get_id_by_name(arg)) < 0)
    {
      send_to_char(d->character, "No such player.\r\n");
      return;
    }
    else {
      OLC_HOUSE(d)->built_by = id;
      send_to_char(d->character, "Builder changed.\r\n");
    }
    break;

  case HSEDIT_PAYMENT:
    switch (*arg)
    {
      case 'y':
      case 'Y':
        OLC_HOUSE(d)->last_payment = time(0);
        break;
      case 'n':
      case 'N':
        send_to_char(d->character, "Last Payment Date not changed\r\n");
        hsedit_disp_menu(d);
        return;
        break;
    }
    break;

  case HSEDIT_GUEST_MENU:
    switch (*arg)
    {
      case 'a':
      case 'A':
        if (count_house_guests(OLC_HOUSE(d)) > (MAX_GUESTS-1))
        {
          send_to_char(d->character, "%sGuest List Full! - delete some before adding more%s\r\nEnter selection (A/D/C/Q) : ", CBRED(d->character, C_NRM), CCNRM(d->character, C_NRM));
        }
        else
        {
          send_to_char(d->character, "Name of guest to add: ");
          OLC_MODE(d) = HSEDIT_GUEST_ADD;
        }
        break;

      case 'd':
      case 'D':
        if (count_house_guests(OLC_HOUSE(d)) < 1)
        {
          send_to_char(d->character, "%sGuest List Empty! - add a guest before trying to delete one%s\r\nEnter selection (A/D/C/Q) : ", CBRED(d->character, C_NRM), CCNRM(d->character, C_NRM));
        }
        else
        {
          send_to_char(d->character, "Name of guest to delete : ");
          OLC_MODE(d) = HSEDIT_GUEST_DELETE;
        }
        break;

      case 'c':
      case 'C':
        send_to_char(d->character, "Clear guest list? (Y/N) : ");
        OLC_MODE(d) = HSEDIT_GUEST_CLEAR;
        break;

      case 'q':
      case 'Q':
        hsedit_disp_menu(d);
        break;

      default:
        send_to_char(d->character, "Invalid choice!\r\n\r\n");
        hsedit_disp_guest_menu(d);
        break;
    }
    return;
    break;

  case HSEDIT_GUEST_ADD:
    if ((id = get_id_by_name(arg)) < 0)
    {
      send_to_char(d->character, "No such player.\r\n");
      hsedit_disp_guest_menu(d);
      return;
    }
    else if (id == OLC_HOUSE(d)->owner)
    {
      send_to_char(d->character, "House owner should not be in the guest list!\r\n");
      hsedit_disp_guest_menu(d);
      return;
    }
    else {
      struct guest_data *g, *next_g = NULL;
      for (g = OLC_HOUSE(d)->guests; g; g = next_g)
      {
        next_g = g->next;
        if (g->id == id)
        {
          send_to_char(d->character, "That player is already in the guest list!.\r\n");
          hsedit_disp_guest_menu(d);
          return;
        }
      }
      add_house_guest(OLC_HOUSE(d), id);
      send_to_char(d->character, "Guest added.\r\n");
      OLC_VAL(d) = 1;
      hsedit_disp_guest_menu(d);
      return;
    }
    break;

  case HSEDIT_GUEST_DELETE:
    if ((id = get_id_by_name(arg)) < 0)
    {
      send_to_char(d->character, "No such player.\r\n");
      hsedit_disp_guest_menu(d);
      return;
    }
    else {
      struct guest_data *g, *next_g = NULL;
      for (g = OLC_HOUSE(d)->guests; g; g = next_g)
      {
        next_g = g->next;
        if (g->id == id)
          found = TRUE;
      }
      if (!found) {
        send_to_char(d->character, "That player isn't in the guest list!\r\n");
        hsedit_disp_guest_menu(d);
        return;
      }
      remove_house_guest(OLC_HOUSE(d), id);
      send_to_char(d->character, "Guest deleted.\r\n");
      OLC_VAL(d) = 1;
      hsedit_disp_guest_menu(d);
      return;
    }
    break;

  case HSEDIT_GUEST_CLEAR:
    switch (*arg)
    {
      case 'n':
      case 'N':
        send_to_char(d->character, "Invalid choice!");
        return;

      case 'y':
      case 'Y':
        free_house_guests(OLC_HOUSE(d));
        send_to_char(d->character, "Guest List Cleared!");
        OLC_VAL(d) = 1;
        hsedit_disp_guest_menu(d);
        break;

      default:
        send_to_char(d->character, "Invalid choice!\r\nClear Guest List? (Y/N) : ");
        return;
    }
    break;

  case HSEDIT_TYPE:
    number = atoi(arg);
    if (number < 0 || number >= NUM_HOUSE_TYPES) {
      send_to_char(d->character, "Invalid choice!");
      hsedit_disp_type_menu(d);
      return;
    } else
      OLC_HOUSE(d)->mode = number;
    break;

  case HSEDIT_FLAGS:
    number = atoi(arg);
    if ((number < 0) || (number > HOUSE_NUM_FLAGS)) {
      send_to_char(d->character, "That's not a valid choice!\r\n");
      hsedit_disp_flags_menu(d);
    } else {
      if (number == 0)
        break;
      else {
        /* toggle bits */
        if (IS_SET_AR(OLC_HOUSE(d)->house_flags, (number - 1)))
          REMOVE_BIT_AR(OLC_HOUSE(d)->house_flags, (number - 1));
        else
          SET_BIT_AR(OLC_HOUSE(d)->house_flags, (number - 1));
        hsedit_disp_flags_menu(d);
      }
    }
    return;

  case HSEDIT_RECEP:
    number = atoi(arg);
    if (number == 0)
    {
      /* '0' chosen - go back to main menu */
      hsedit_disp_menu(d);
      return;
    }
    if (real_mobile(number) == NOBODY)
    {
      send_to_char(d->character, "Mob VNUM does not exist.\r\nEnter a valid mobile VNUM (0 to exit) : ");
      return;
    }
    OLC_HOUSE(d)->receptionist = number;
    break;

  default:
    /* we should never get here */
    mudlog(BRF,ADMLVL_BUILDER,TRUE,"SYSERR: Reached default case in parse_hsedit");
    break;
  }
  /*. If we get this far, something has been changed .*/
  OLC_VAL(d) = 1;
  hsedit_disp_menu(d);
}

void hsedit_string_cleanup(struct descriptor_data *d, int terminator)
{
  switch (OLC_MODE(d)) {
    /* There are no strings to be edited in houses - if there are any added later, add them here */
  }
}

ACMD(do_oasis_hsedit)
{
  int number = NOWHERE, save = 0;
  struct descriptor_data *d;
  char *buf3;
  char buf1[MAX_STRING_LENGTH];
  char buf2[MAX_STRING_LENGTH];
  struct house_control_data *hse;


/****************************************************************************/
/** Parse any arguments.                                                   **/
/****************************************************************************/
  buf3 = two_arguments(argument, buf1, buf2);


/****************************************************************************/
/** If there aren't any arguments...grab the number of the current room... **/
/****************************************************************************/
  if (!*buf1) {
    number = GET_ROOM_VNUM(IN_ROOM(ch));
  } else if (!isdigit(*buf1)) {
    if (str_cmp("save", buf1) != 0) {
      send_to_char(ch, "Yikes!  Stop that, someone will get hurt!\r\n");
      return;
    }
    save = TRUE;

    if (is_number(buf2))
      number = atoi(buf2);
    else if (GET_OLC_ZONE(ch) > 0) {
      zone_rnum zlok;

      if ((zlok = real_zone(GET_OLC_ZONE(ch))) == NOWHERE)
        number = NOWHERE;
      else
        number = genolc_zone_bottom(zlok);
    }

    if (number == NOWHERE) {
      send_to_char(ch, "Save which zone?\r\n");
      return;
    }
  }


/****************************************************************************/
/** If a numeric argument was given, get it.                               **/
/****************************************************************************/
  if (number == NOWHERE)
    number = atoi(buf1);


/****************************************************************************/
/** Check that whatever it is isn't already being edited.                  **/
/****************************************************************************/
  for (d = descriptor_list; d; d = d->next) {
    if (STATE(d) == CON_HSEDIT) {
      if (d->olc && OLC_NUM(d) == number) {
        send_to_char(ch, "That house is currently being edited by %s.\r\n",
          PERS(d->character, ch));
        return;
      }
    }
  }


/****************************************************************************/
/** Point d to the builder's descriptor (for easier typing later).         **/
/****************************************************************************/
  d = ch->desc;


/****************************************************************************/
/** Give the descriptor an OLC structure.                                  **/
/****************************************************************************/
  if (d->olc) {
    mudlog(BRF, ADMLVL_IMMORT, TRUE, "SYSERR: do_oasis_hsedit: Player already had olc structure.");
    free(d->olc);
  }

  CREATE(d->olc, struct oasis_olc_data, 1);


/****************************************************************************/
/** Find the zone.                                                         **/
/****************************************************************************/
  OLC_ZNUM(d) = save ? real_zone(number) : real_zone_by_thing(number);
  if (OLC_ZNUM(d) == NOWHERE) {
    send_to_char(ch, "Sorry, there is no zone for that number!\r\n");


/**************************************************************************/
/** Free the descriptor's OLC structure.                                 **/
/**************************************************************************/
    free(d->olc);
    d->olc = NULL;
    return;
  }


/****************************************************************************/
/** Everyone but IMPLs can only edit zones they have been assigned.        **/
/****************************************************************************/
  if (!can_edit_zone(ch, OLC_ZNUM(d))) {
    send_to_char(ch, " You do not have permission to edit zone %d. Try zone %d.\r\n", zone_table[OLC_ZNUM(d)].number, GET_OLC_ZONE(ch));mudlog(BRF, ADMLVL_IMPL, TRUE, "OLC: %s tried to edit zone %d allowed zone %d",
      GET_NAME(ch), zone_table[OLC_ZNUM(d)].number, GET_OLC_ZONE(ch));


/**************************************************************************/
/** Free the descriptor's OLC structure.                                 **/
/**************************************************************************/
    free(d->olc);
    d->olc = NULL;
    return;
  }


/****************************************************************************/
/** If we need to save, save the houses.                                  **/
/****************************************************************************/
  if (save) {
    send_to_char(ch, "Saving all houses in zone %d.\r\n",
      zone_table[OLC_ZNUM(d)].number);
    mudlog(CMP, MAX(ADMLVL_BUILDER, GET_INVIS_LEV(ch)), TRUE,
      "OLC: %s saves house info for zone %d.", GET_NAME(ch),
      zone_table[OLC_ZNUM(d)].number);


/**************************************************************************/
/** Save all the houses in memory before we start.                       **/
/**************************************************************************/
    House_save_control();


/**************************************************************************/
/** Free the descriptor's OLC structure.                                 **/
/**************************************************************************/
    free(d->olc);
    d->olc = NULL;
    return;
  }

  OLC_NUM(d) = number;


/****************************************************************************/
/** If this is a new house, setup a new house, otherwise setup the        **/
/** existing house.                                                       **/
/****************************************************************************/
  if ((hse = find_house(number)) == NULL)
  {
    /* Do a quick check to ensure there is room for more */
    if (count_houses() >= MAX_HOUSES)
    {
      send_to_char(ch, "MAX houses limit reached (%d) - Unable to create more.\r\n", MAX_HOUSES);
      mudlog(NRM, ADMLVL_IMPL, TRUE, "HSEDIT: MAX houses limit reached (%d)\r\n", MAX_HOUSES);
      return;
    }
    else
    {
      hsedit_setup_new(d);
    }
  }
  else
  {
    hsedit_setup_existing(d, hse);
  }

  STATE(d) = CON_HSEDIT;


/****************************************************************************/
  /** Send the OLC message to the players in the same room as the
builder.   **/

/****************************************************************************/
  act("$n starts using OLC.", TRUE, d->character, 0, 0, TO_ROOM);
  SET_BIT_AR(PLR_FLAGS(ch), PLR_WRITING);


/****************************************************************************/
/** Log the OLC message.                                                   **/
/****************************************************************************/
  mudlog(CMP, ADMLVL_IMMORT, TRUE, "OLC: (hsedit) %s starts editing zone %d allowed zone %d",
    GET_NAME(ch), zone_table[OLC_ZNUM(d)].number, GET_OLC_ZONE(ch));
}
