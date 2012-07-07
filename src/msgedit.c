/**************************************************************************
*  File: msgedit.c                                       Part of tbaMUD   *
*  Usage: Handling of loading/saving messages and the olc editor.         *
*                                                                         *
*  By Vatiken. Copyright 2012 by Joseph Arnusch                           *
**************************************************************************/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "screen.h"
#include "spells.h"
#include "db.h"
#include "msgedit.h"
#include "oasis.h"
#include "genolc.h"
#include "interpreter.h"
#include "modify.h"

/* Statics */
static void free_messages_type(struct msg_type *msg);
static void msgedit_main_menu(struct descriptor_data * d);
static void copy_message_strings(struct message_type *tmsg, struct message_type * fmsg);
static void copy_message_list(struct message_list *to, struct message_list *from);

static void free_messages_type(struct msg_type *msg)
{
  if (msg->attacker_msg) {	free(msg->attacker_msg); msg->attacker_msg = NULL; }
  if (msg->victim_msg)	 {	free(msg->victim_msg); msg->victim_msg = NULL; }
  if (msg->room_msg)		 {  free(msg->room_msg); msg->room_msg = NULL; }
}

void free_message_list(struct message_list * mlist)
{
  struct message_type * msg, * orig;
  
  msg = mlist->msg;
  
  while (msg) {
    orig = msg;
    
    free_messages_type(&msg->die_msg);
    free_messages_type(&msg->miss_msg);
    free_messages_type(&msg->hit_msg);
    free_messages_type(&msg->god_msg);
  
    msg = msg->next;
    free(orig);
  }
  free (mlist);
}

void free_messages(void)
{
  int i;

  for (i = 0; i < MAX_MESSAGES; i++)
    while (fight_messages[i].msg) {
      struct message_type *former = fight_messages[i].msg;

      free_messages_type(&former->die_msg);
      free_messages_type(&former->miss_msg);
      free_messages_type(&former->hit_msg);
      free_messages_type(&former->god_msg);

      fight_messages[i].msg = fight_messages[i].msg->next;
      free(former);
    }
}

void load_messages(void)
{
  FILE *fl;
  int i, type;
  struct message_type *messages;
  char chk[128], *buf;

  if (!(fl = fopen(MESS_FILE, "r"))) {
    log("SYSERR: Error reading combat message file %s: %s", MESS_FILE, strerror(errno));
    exit(1);
  }

  for (i = 0; i < MAX_MESSAGES; i++) {
    fight_messages[i].a_type = 0;
    fight_messages[i].number_of_attacks = 0;
    fight_messages[i].msg = NULL;
  }

  while (!feof(fl)) {
    buf = fgets(chk, 128, fl);
    while (!feof(fl) && (*chk == '\n' || *chk == '*'))
      buf = fgets(chk, 128, fl);

    while (*chk == 'M') {
      buf = fgets(chk, 128, fl);
      sscanf(chk, " %d\n", &type);
      for (i = 0; (i < MAX_MESSAGES) && (fight_messages[i].a_type != type) &&
         (fight_messages[i].a_type); i++);
      if (i >= MAX_MESSAGES) {
        log("SYSERR: Too many combat messages.  Increase MAX_MESSAGES and recompile.");
        exit(1);
      }
      CREATE(messages, struct message_type, 1);
      fight_messages[i].number_of_attacks++;
      fight_messages[i].a_type = type;
      messages->next = fight_messages[i].msg;
      fight_messages[i].msg = messages;

      messages->die_msg.attacker_msg = fread_action(fl, i);
      messages->die_msg.victim_msg = fread_action(fl, i);
      messages->die_msg.room_msg = fread_action(fl, i);
      messages->miss_msg.attacker_msg = fread_action(fl, i);
      messages->miss_msg.victim_msg = fread_action(fl, i);
      messages->miss_msg.room_msg = fread_action(fl, i);
      messages->hit_msg.attacker_msg = fread_action(fl, i);
      messages->hit_msg.victim_msg = fread_action(fl, i);
      messages->hit_msg.room_msg = fread_action(fl, i);
      messages->god_msg.attacker_msg = fread_action(fl, i);
      messages->god_msg.victim_msg = fread_action(fl, i);
      messages->god_msg.room_msg = fread_action(fl, i);
      buf  = fgets(chk, 128, fl);
      while (!feof(fl) && (*chk == '\n' || *chk == '*'))
        buf  = fgets(chk, 128, fl);
    }
  }
  fclose(fl);
  log("Loaded %d Combat Messages...", i);
}

static void show_messages(struct char_data *ch)
{
  int i, half = MAX_MESSAGES / 2, count = 0;
  char buf[MAX_STRING_LENGTH];
  int len;
  
  len = snprintf(buf, sizeof(buf), "\t1Message List:\tn \r\n");
  
  for (i = 0; i < MAX_MESSAGES / 2; i++, half++)
    if (fight_messages[i].msg != NULL && len < sizeof(buf)) {
      count += fight_messages[i].number_of_attacks;
      len += snprintf(buf + len, sizeof(buf) - len, "%-2d) [%-3d] %d, %-18s%s", i, fight_messages[i].a_type, fight_messages[i].number_of_attacks, fight_messages[i].a_type < TOP_SPELL_DEFINE ? spell_info[fight_messages[i].a_type].name : "Unknown", half < MAX_MESSAGES && fight_messages[half].msg ? "   " : "\r\n");
      if (half < MAX_MESSAGES && fight_messages[half].msg)
        len += snprintf(buf + len, sizeof(buf) - len, "%-2d) [%-3d] %d, %-18s\r\n", half, fight_messages[half].a_type, fight_messages[half].number_of_attacks, fight_messages[half].a_type < TOP_SPELL_DEFINE ? spell_info[fight_messages[half].a_type].name : "Unknown");
    }
    
  len += snprintf(buf + len, sizeof(buf) - len, "Total Messages: %d\r\n", count);  
  page_string(ch->desc, buf, TRUE);
}

#define PRINT_MSG(msg) (msg == NULL ? "#" : msg)

void save_messages_to_disk(void)
{
  FILE *fp;
  int i;
  struct message_type *msg;

  if (!(fp = fopen(MESS_FILE, "w"))) {
    log("SYSERR: Error writing combat message file %s: %s", MESS_FILE, strerror(errno));
    exit(1);
  }
 
  fprintf(fp, "* TBAMUD 3.64 Combat Message File\n");
 
  for (i = 0; i < MAX_MESSAGES; i++) {
    if (fight_messages[i].msg == NULL)
      continue;
    if (fight_messages[i].a_type > 0 && fight_messages[i].a_type < TOP_SPELL_DEFINE)
      fprintf(fp, "* %s %d\n", PRINT_MSG(spell_info[fight_messages[i].a_type].name), fight_messages[i].a_type);
    else
      fprintf(fp, "* %d\n", fight_messages[i].a_type);
             
    for (msg = fight_messages[i].msg; msg; msg = msg->next){
      fprintf(fp, "M\n"
                  "%d\n"
                  "%s\n"
                  "%s\n" 
                  "%s\n" 
                  "%s\n" 
                  "%s\n" 
                  "%s\n" 
                  "%s\n" 
                  "%s\n" 
                  "%s\n" 
                  "%s\n" 
                  "%s\n" 
                  "%s\n"
                  "\n",
                  fight_messages[i].a_type,
                  PRINT_MSG(msg->die_msg.attacker_msg),
                  PRINT_MSG(msg->die_msg.victim_msg),
                  PRINT_MSG(msg->die_msg.room_msg),
                  PRINT_MSG(msg->miss_msg.attacker_msg),
                  PRINT_MSG(msg->miss_msg.victim_msg),
                  PRINT_MSG(msg->miss_msg.room_msg),
                  PRINT_MSG(msg->hit_msg.attacker_msg),
                  PRINT_MSG(msg->hit_msg.victim_msg),
                  PRINT_MSG(msg->hit_msg.room_msg),
                  PRINT_MSG(msg->god_msg.attacker_msg),
                  PRINT_MSG(msg->god_msg.victim_msg),
                  PRINT_MSG(msg->god_msg.room_msg));
    }
  }  
  
  fclose(fp);
}

static void msgedit_setup(struct descriptor_data *d)
{
  CREATE(OLC_MSG_LIST(d), struct message_list, 1);
  OLC_MSG_LIST(d)->msg = NULL;  
    
  copy_message_list(OLC_MSG_LIST(d), &fight_messages[OLC_NUM(d)]);
  OLC_MSG(d) = OLC_MSG_LIST(d)->msg;
}

static void copy_message_list(struct message_list *to, struct message_list *from) 
{
  struct message_type * msg, * tmp_msg, * orig;
  
  to->a_type = from->a_type;
  to->number_of_attacks = from->number_of_attacks;
  
  /* Lets free any messages in *to just in case */
  tmp_msg = to->msg;
  
  while (tmp_msg) {
    msg = tmp_msg;
    
    free_messages_type(&msg->die_msg);
    free_messages_type(&msg->miss_msg);
    free_messages_type(&msg->hit_msg);
    free_messages_type(&msg->god_msg);
    
    tmp_msg = msg->next;
    free(msg);
  }
  to->msg = NULL;
  
  /* Now lets copy */
  if (from->msg == NULL) {
    CREATE(msg, struct message_type, 1);
    copy_message_strings(msg, NULL);
    msg->next = NULL;
    to->msg = msg;
    return;    
  }
  
  for (tmp_msg = from->msg, orig = NULL; tmp_msg; tmp_msg = tmp_msg->next)
  {
    CREATE(msg, struct message_type, 1);
    copy_message_strings(msg, tmp_msg);

    msg->next = orig;
    orig = msg;    
  }
  to->msg = orig;
}

static void copy_message_strings(struct message_type *tmsg, struct message_type * fmsg)
{
  tmsg->die_msg.attacker_msg = fmsg && fmsg->die_msg.attacker_msg ? strdup(fmsg->die_msg.attacker_msg) : NULL;
  tmsg->die_msg.victim_msg = fmsg && fmsg->die_msg.victim_msg ? strdup(fmsg->die_msg.victim_msg) : NULL;
  tmsg->die_msg.room_msg = fmsg && fmsg->die_msg.room_msg ? strdup(fmsg->die_msg.room_msg) : NULL;
  
  tmsg->miss_msg.attacker_msg = fmsg && fmsg->miss_msg.attacker_msg ? strdup(fmsg->miss_msg.attacker_msg) : NULL;
  tmsg->miss_msg.victim_msg = fmsg && fmsg->miss_msg.victim_msg ? strdup(fmsg->miss_msg.victim_msg) : NULL;
  tmsg->miss_msg.room_msg = fmsg && fmsg->miss_msg.room_msg ? strdup(fmsg->miss_msg.room_msg) : NULL;
  
  tmsg->hit_msg.attacker_msg = fmsg && fmsg->hit_msg.attacker_msg ? strdup(fmsg->hit_msg.attacker_msg) : NULL;
  tmsg->hit_msg.victim_msg = fmsg && fmsg->hit_msg.victim_msg ? strdup(fmsg->hit_msg.victim_msg) : NULL;
  tmsg->hit_msg.room_msg = fmsg && fmsg->hit_msg.room_msg ? strdup(fmsg->hit_msg.room_msg) : NULL; 
  
  tmsg->god_msg.attacker_msg = fmsg && fmsg->god_msg.attacker_msg ? strdup(fmsg->god_msg.attacker_msg) : NULL;
  tmsg->god_msg.victim_msg = fmsg && fmsg->god_msg.victim_msg ? strdup(fmsg->god_msg.victim_msg) : NULL;
  tmsg->god_msg.room_msg = fmsg && fmsg->god_msg.room_msg ? strdup(fmsg->god_msg.room_msg) : NULL;     
}

ACMD(do_msgedit)
{
  int num;
  struct descriptor_data *d;
  
  if (!*argument) {
    show_messages(ch);
    return;  
  }
  
  if ((num = atoi(argument)) < 0) {
    send_to_char(ch, "You must select a message # between 0 and %d.\r\n", MAX_MESSAGES);
    return;
  }
  
  if (num >= MAX_MESSAGES) {
    send_to_char(ch, "You must select a message # between 0 and %d.\r\n", MAX_MESSAGES - 1);
    return;
  }
  
  for (d = descriptor_list; d; d = d->next)
    if (STATE(d) == CON_MSGEDIT) {
      if (OLC_MSG_LIST(d) && OLC_NUM(d) == num) {
        send_to_char(ch, "Someone is already editing that message.\r\n");
        return;
      }
    }
 
  /* Retrieve the player's descriptor. */
  d = ch->desc;

  /* Give the descriptor an OLC structure. */
  if (d->olc) {
    mudlog(BRF, LVL_IMMORT, TRUE, "SYSERR: do_msg_edit: Player already had olc structure.");
    free(d->olc);
  }

  /* Create the OLC structure. */
  CREATE(d->olc, struct oasis_olc_data, 1);
 
  OLC_NUM(d) = num;
  OLC_VAL(d) = 0;
  msgedit_setup(d);
  
  msgedit_main_menu(ch->desc);
  STATE(d) = CON_MSGEDIT;
  
  act("$n starts using OLC.", TRUE, d->character, 0, 0, TO_ROOM);
  SET_BIT_AR(PLR_FLAGS(ch), PLR_WRITING);

  mudlog(CMP, LVL_IMMORT, TRUE, "OLC: %s starts editing message %d",
    GET_NAME(ch), OLC_NUM(d));
}

static void msgedit_main_menu(struct descriptor_data * d)
{
  get_char_colors(d->character);
  
  write_to_output(d, "%sMsg Edit: %s[%s%dx%d%s] [%s$n: Attacker | $N: Victim%s]%s\r\n", cyn, grn, yel, OLC_NUM(d), OLC_MSG_LIST(d)->number_of_attacks, grn, yel, grn, nrm);
  write_to_output(d, "%s1%s) %sAction Type: %s%d %s[%s%s%s]%s\r\n", grn, yel, cyn, yel, OLC_MSG_LIST(d)->a_type,  grn, yel, OLC_MSG_LIST(d)->a_type < TOP_SPELL_DEFINE ? spell_info[OLC_MSG_LIST(d)->a_type].name : "Unknown", grn, nrm);
   
  write_to_output(d, "   %sDeath Messages:\r\n"
                     "%sA%s) CHAR : %s %s\r\n"
                     "%sB%s) VICT : %s %s\r\n"
                     "%sC%s) ROOM : %s %s\r\n", 
                     cyn,
                     grn, yel, nrm, PRINT_MSG(OLC_MSG(d)->die_msg.attacker_msg),
                     grn, yel, nrm, PRINT_MSG(OLC_MSG(d)->die_msg.victim_msg),
                     grn, yel, nrm, PRINT_MSG(OLC_MSG(d)->die_msg.room_msg));
  
  write_to_output(d, "   %sMiss Messages:\r\n"
                     "%sD%s) CHAR : %s %s\r\n"
                     "%sE%s) VICT : %s %s\r\n"
                     "%sF%s) ROOM : %s %s\r\n", 
                     cyn,
                     grn, yel, nrm, PRINT_MSG(OLC_MSG(d)->miss_msg.attacker_msg),
                     grn, yel, nrm, PRINT_MSG(OLC_MSG(d)->miss_msg.victim_msg),
                     grn, yel, nrm, PRINT_MSG(OLC_MSG(d)->miss_msg.room_msg));
    
  write_to_output(d, "   %sHit Messages:\r\n"
                     "%sG%s) CHAR : %s %s\r\n"
                     "%sH%s) VICT : %s %s\r\n"
                     "%sI%s) ROOM : %s %s\r\n", 
                     cyn,
                     grn, yel, nrm, PRINT_MSG(OLC_MSG(d)->hit_msg.attacker_msg),
                     grn, yel, nrm, PRINT_MSG(OLC_MSG(d)->hit_msg.victim_msg),
                     grn, yel, nrm, PRINT_MSG(OLC_MSG(d)->hit_msg.room_msg));
      
  write_to_output(d, "   %sGod Messages:\r\n"
                     "%sJ%s) CHAR : %s %s\r\n"
                     "%sK%s) VICT : %s %s\r\n"
                     "%sL%s) ROOM : %s %s\r\n", 
                     cyn,
                     grn, yel, nrm, PRINT_MSG(OLC_MSG(d)->god_msg.attacker_msg),
                     grn, yel, nrm, PRINT_MSG(OLC_MSG(d)->god_msg.victim_msg),
                     grn, yel, nrm, PRINT_MSG(OLC_MSG(d)->god_msg.room_msg));
  
  write_to_output(d, "\r\n%sN%s)%s %s", grn, yel, nrm, OLC_MSG(d)->next ? "Next" : "New");
  if (OLC_MSG(d) != OLC_MSG_LIST(d)->msg)
    write_to_output(d, " %sP%s)%s Previous", grn, yel, nrm);
  if (OLC_VAL(d))
    write_to_output(d, " %sS%s)%s Save", grn, yel, nrm);
  write_to_output(d, " %sQ%s)%s Quit\r\n"
                     "Enter Selection : ", grn, yel, nrm);
  OLC_MODE(d) = MSGEDIT_MAIN_MENU;  
}

void msgedit_parse(struct descriptor_data *d, char *arg)
{
  struct message_type * temp;
  static bool quit = FALSE;
  
  switch (OLC_MODE(d)) {
    case MSGEDIT_MAIN_MENU:
      if (!*arg) {
        write_to_output(d, "Enter Option : ");
        return;
      }
      switch (*arg) {
        case '1':
          write_to_output(d, "Enter Action Type : ");
          OLC_MODE(d) = MSGEDIT_TYPE;
        return;
        case 'A':
        case 'a':
          write_to_output(d, "Example: You kill $N!\r\n");
          write_to_output(d, "Enter new string : ");
          OLC_MODE(d) = MSGEDIT_DEATH_CHAR;
        return;
        case 'B':
        case 'b':
          write_to_output(d, "Example: $n kills you!\r\n");
          write_to_output(d, "Enter new string : ");
          OLC_MODE(d) = MSGEDIT_DEATH_VICT;
        return;
        case 'C':
        case 'c':
          write_to_output(d, "Example: $n kills $N!\r\n");
          write_to_output(d, "Enter new string : ");
          OLC_MODE(d) = MSGEDIT_DEATH_ROOM;
        return;
        case 'D':
        case 'd':
          write_to_output(d, "Example: You miss $N!\r\n");
          write_to_output(d, "Enter new string : ");
          OLC_MODE(d) = MSGEDIT_MISS_CHAR;
        return;
        case 'E':
        case 'e':
          write_to_output(d, "Example: $n misses you!\r\n");
          write_to_output(d, "Enter new string : ");
          OLC_MODE(d) = MSGEDIT_MISS_VICT;
        return;
        case 'F':
        case 'f':
          write_to_output(d, "Example: $n misses $N!\r\n");
          write_to_output(d, "Enter new string : ");
          OLC_MODE(d) = MSGEDIT_MISS_ROOM;
        return;
        case 'G':
        case 'g':
          write_to_output(d, "Example: You hit $N!\r\n");
          write_to_output(d, "Enter new string : ");
          OLC_MODE(d) = MSGEDIT_HIT_CHAR;
        return;
        case 'H':
        case 'h':
          write_to_output(d, "Example: $n hits you!\r\n");
          write_to_output(d, "Enter new string : ");
          OLC_MODE(d) = MSGEDIT_HIT_VICT;
        return;
        case 'I':
        case 'i':
          write_to_output(d, "Example: $n hits $N!\r\n");
          write_to_output(d, "Enter new string : ");
          OLC_MODE(d) = MSGEDIT_HIT_ROOM;
        return;
        case 'J':
        case 'j':
          write_to_output(d, "Example: You can't hit $N!\r\n");
          write_to_output(d, "Enter new string : ");
          OLC_MODE(d) = MSGEDIT_GOD_CHAR;
        return;
        case 'K':
        case 'k':
          write_to_output(d, "Example: $n can't hit you!\r\n");
          write_to_output(d, "Enter new string : ");
          OLC_MODE(d) = MSGEDIT_GOD_VICT;
        return;
        case 'L':
        case 'l':
          write_to_output(d, "Example: $n can't hit $N!\r\n");
          write_to_output(d, "Enter new string : ");
          OLC_MODE(d) = MSGEDIT_GOD_ROOM;
        return;
        case 'N':
        case 'n':
          if (OLC_MSG(d)->next == NULL) {
            temp = OLC_MSG(d);
            CREATE(OLC_MSG(d), struct message_type, 1);
            copy_message_strings(OLC_MSG(d), NULL);
            OLC_MSG_LIST(d)->number_of_attacks++;
            temp->next = OLC_MSG(d);
          } else
            OLC_MSG(d) = OLC_MSG(d)->next;
          
          msgedit_main_menu(d);
        return;
        case 'P':
        case 'p':
          if (OLC_MSG(d) == OLC_MSG_LIST(d)->msg) {
            msgedit_main_menu(d);
            return;
          }
          temp = OLC_MSG(d);
          for (OLC_MSG(d) = OLC_MSG_LIST(d)->msg; OLC_MSG(d); OLC_MSG(d) = OLC_MSG(d)->next)
            if (OLC_MSG(d)->next == temp)
              break;
          
          msgedit_main_menu(d);
        return;
        case 'S':
        case 's':
          write_to_output(d, "Do you wish to save? Y/N : ");
          OLC_MODE(d) = MSGEDIT_CONFIRM_SAVE;
        return;
        case 'Q':
        case 'q':
          if (OLC_VAL(d)) {
            OLC_MODE(d) = MSGEDIT_CONFIRM_SAVE;
            quit = TRUE;
            write_to_output(d, "Do you wish to save? Y/N : ");
            return;
          }
          write_to_output(d, "Exiting message editor.\r\n");
          cleanup_olc(d, CLEANUP_ALL);
        return;
      }
    break;
    case MSGEDIT_CONFIRM_SAVE:
      if (*arg && (*arg == 'Y' || *arg == 'y')) {
        copy_message_list(&fight_messages[OLC_NUM(d)], OLC_MSG_LIST(d));
        save_messages_to_disk();
        OLC_VAL(d) = 0;
        write_to_output(d, "Messages saved.\r\n");
      } else
        write_to_output(d, "Save aborted.\r\n");
  
      if (quit) {
        quit = FALSE;
        write_to_output(d, "Exiting message editor.\r\n");
        cleanup_olc(d, CLEANUP_ALL);     
        return;     
      } 
        
      msgedit_main_menu(d);
    return;
    case MSGEDIT_TYPE:
      OLC_MSG_LIST(d)->a_type = LIMIT(atoi(arg), 0, 500);
    break;
    case MSGEDIT_DEATH_CHAR:
      if (!genolc_checkstring(d, arg))
        break;
      delete_doubledollar(arg);
          
      if (OLC_MSG(d)->die_msg.attacker_msg)
        free(OLC_MSG(d)->die_msg.attacker_msg);
            
      OLC_MSG(d)->die_msg.attacker_msg = strdup(arg);    
    break;
    case MSGEDIT_DEATH_VICT:
      if (!genolc_checkstring(d, arg))
        break;
      delete_doubledollar(arg);
          
      if (OLC_MSG(d)->die_msg.victim_msg)
        free(OLC_MSG(d)->die_msg.victim_msg);
            
      OLC_MSG(d)->die_msg.victim_msg = strdup(arg);    
    break;
    case MSGEDIT_DEATH_ROOM:
      if (!genolc_checkstring(d, arg))
        break;
      delete_doubledollar(arg);
          
      if (OLC_MSG(d)->die_msg.room_msg)
        free(OLC_MSG(d)->die_msg.room_msg);
            
      OLC_MSG(d)->die_msg.room_msg = strdup(arg);    
    break;
    case MSGEDIT_MISS_CHAR:
      if (!genolc_checkstring(d, arg))
        break;
      delete_doubledollar(arg);
          
      if (OLC_MSG(d)->miss_msg.attacker_msg)
        free(OLC_MSG(d)->miss_msg.attacker_msg);
            
      OLC_MSG(d)->miss_msg.attacker_msg = strdup(arg);    
    break;
    case MSGEDIT_MISS_VICT:
      if (!genolc_checkstring(d, arg))
        break;
      delete_doubledollar(arg);
          
      if (OLC_MSG(d)->miss_msg.victim_msg)
        free(OLC_MSG(d)->miss_msg.victim_msg);
            
      OLC_MSG(d)->miss_msg.victim_msg = strdup(arg);    
    break;
    case MSGEDIT_MISS_ROOM:
      if (!genolc_checkstring(d, arg))
        break;
      delete_doubledollar(arg);
          
      if (OLC_MSG(d)->miss_msg.room_msg)
        free(OLC_MSG(d)->miss_msg.room_msg);
            
      OLC_MSG(d)->miss_msg.room_msg = strdup(arg);    
    break;
    case MSGEDIT_HIT_CHAR:
      if (!genolc_checkstring(d, arg))
        break;
      delete_doubledollar(arg);
          
      if (OLC_MSG(d)->hit_msg.attacker_msg)
        free(OLC_MSG(d)->hit_msg.attacker_msg);
            
      OLC_MSG(d)->hit_msg.attacker_msg = strdup(arg);    
    break;
    case MSGEDIT_HIT_VICT:
      if (!genolc_checkstring(d, arg))
        break;
      delete_doubledollar(arg);
          
      if (OLC_MSG(d)->hit_msg.victim_msg)
        free(OLC_MSG(d)->hit_msg.victim_msg);
            
      OLC_MSG(d)->hit_msg.victim_msg = strdup(arg);    
    break;
    case MSGEDIT_HIT_ROOM:
      if (!genolc_checkstring(d, arg))
        break;
      delete_doubledollar(arg);
          
      if (OLC_MSG(d)->hit_msg.room_msg)
        free(OLC_MSG(d)->hit_msg.room_msg);
            
      OLC_MSG(d)->hit_msg.room_msg = strdup(arg);    
    break;
    case MSGEDIT_GOD_CHAR:
      if (!genolc_checkstring(d, arg))
        break;
      delete_doubledollar(arg);
          
      if (OLC_MSG(d)->god_msg.attacker_msg)
        free(OLC_MSG(d)->god_msg.attacker_msg);
            
      OLC_MSG(d)->god_msg.attacker_msg = strdup(arg);    
    break;
    case MSGEDIT_GOD_VICT:
      if (!genolc_checkstring(d, arg))
        break;
      delete_doubledollar(arg);
          
      if (OLC_MSG(d)->god_msg.victim_msg)
        free(OLC_MSG(d)->god_msg.victim_msg);
            
      OLC_MSG(d)->god_msg.victim_msg = strdup(arg);    
    break;
    case MSGEDIT_GOD_ROOM:
      if (!genolc_checkstring(d, arg))
        break;
      delete_doubledollar(arg);
          
      if (OLC_MSG(d)->god_msg.room_msg)
        free(OLC_MSG(d)->god_msg.room_msg);
            
      OLC_MSG(d)->god_msg.room_msg = strdup(arg);    
    break;
  } 
  
  OLC_VAL(d) = 1;  
  msgedit_main_menu(d);
}
