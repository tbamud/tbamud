/* ************************************************************************
*   File: act.social.c                                  Part of CircleMUD *
*  Usage: Functions to handle socials                                     *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include "conf.h"
#include "sysdep.h"


#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"

/* local functions */
char *fread_action(FILE *fl, int nr);
int find_action(int cmd);
ACMD(do_action);
ACMD(do_insult);
void boot_social_messages(void);
void free_social_messages(void);
void free_action(struct social_messg *mess);
void free_command_list(void);
void create_command_list(void);
ACMD(do_gmote);

ACMD(do_action)
{
  char arg[MAX_INPUT_LENGTH], part[MAX_INPUT_LENGTH];
  int act_nr;
  struct social_messg *action;
  struct char_data *vict;
  struct obj_data *targ;

  if ((act_nr = find_action(cmd)) < 0) {
    send_to_char(ch, "That action is not supported.\r\n");
    return;
  }

  action = &soc_mess_list[act_nr];

  if (!argument || !*argument) {
    send_to_char(ch, "%s\r\n", action->char_no_arg);
    act(action->others_no_arg, action->hide, ch, 0, 0, TO_ROOM);
    return;
  }

  two_arguments(argument, arg, part);

  if ((!action->char_body_found) && (*part)) {
    send_to_char(ch, "Sorry, this social does not support body parts.\r\n");
    return;
  }

  if (!action->char_found) 
    *arg = '\0';

  if (action->char_found && argument)
    one_argument(argument, arg);
  else
    *arg = '\0';

  vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM);
  if (!vict) {
    if (action->char_obj_found) {
      targ = get_obj_in_list_vis(ch, arg, NULL, ch->carrying);
      if (!targ) targ = get_obj_in_list_vis(ch, arg, NULL, world[IN_ROOM(ch)].contents);
      if (targ) {
        act(action->char_obj_found, action->hide, ch, targ, 0, TO_CHAR);
        act(action->others_obj_found, action->hide, ch, targ, 0, TO_ROOM);
        return;
      }
    }
    if (action->not_found)
      send_to_char(ch, "%s\r\n", action->not_found);
    else
      send_to_char(ch, "I don't see anything by that name here.\r\n");
    return;
  } 
  
  if (vict == ch) {
    if (action->char_auto)
      send_to_char(ch, "%s\r\n", action->char_auto);
    else
      send_to_char(ch, "Erm, no.");
    act(action->others_auto, action->hide, ch, 0, 0, TO_ROOM);
    return;
  } 
  
  if (GET_POS(vict) < action->min_victim_position)
    act("$N is not in a proper position for that.", FALSE, ch, 0, vict, TO_CHAR | TO_SLEEP);
  else {
    if (*part) {
      act(action->char_body_found, 0, ch, (struct obj_data *)part, vict, TO_CHAR | TO_SLEEP);
      act(action->others_body_found, action->hide, ch, (struct obj_data *)part, vict, TO_NOTVICT);
      act(action->vict_body_found, action->hide, ch, (struct obj_data *)part, vict, TO_VICT);
    } else {
      act(action->char_found, 0, ch, 0, vict, TO_CHAR | TO_SLEEP);
      act(action->others_found, action->hide, ch, 0, vict, TO_NOTVICT);
      act(action->vict_found, action->hide, ch, 0, vict, TO_VICT);
    }
  }
}



ACMD(do_insult)
{
  char arg[MAX_INPUT_LENGTH];
  struct char_data *victim;

  one_argument(argument, arg);

  if (*arg) {
    if (!(victim = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM)))
      send_to_char(ch, "Can't hear you!\r\n");
    else {
      if (victim != ch) {
	send_to_char(ch, "You insult %s.\r\n", GET_NAME(victim));

	switch (rand_number(0, 2)) {
	case 0:
	  if (GET_SEX(ch) == SEX_MALE) {
	    if (GET_SEX(victim) == SEX_MALE)
	      act("$n accuses you of fighting like a woman!", FALSE, ch, 0, victim, TO_VICT);
	    else
	      act("$n says that women can't fight.", FALSE, ch, 0, victim, TO_VICT);
	  } else {		/* Ch == Woman */
	    if (GET_SEX(victim) == SEX_MALE)
	      act("$n accuses you of having the smallest... (brain?)",
		  FALSE, ch, 0, victim, TO_VICT);
	    else
	      act("$n tells you that you'd lose a beauty contest against a troll.",
		  FALSE, ch, 0, victim, TO_VICT);
	  }
	  break;
	case 1:
	  act("$n calls your mother a bitch!", FALSE, ch, 0, victim, TO_VICT);
	  break;
	default:
	  act("$n tells you to get lost!", FALSE, ch, 0, victim, TO_VICT);
	  break;
	}			/* end switch */

	act("$n insults $N.", TRUE, ch, 0, victim, TO_NOTVICT);
      } else {			/* ch == victim */
	send_to_char(ch, "You feel insulted.\r\n");
      }
    }
  } else
    send_to_char(ch, "I'm sure you don't want to insult *everybody*...\r\n");
}



void boot_social_messages(void)
{
  FILE *fl;
  int nr = 0, hide, min_char_pos, min_pos, min_lvl, curr_soc = -1;
  char next_soc[MAX_STRING_LENGTH], sorted[MAX_INPUT_LENGTH];

  if (CONFIG_NEW_SOCIALS == TRUE) {
    /* open social file */
    if (!(fl = fopen(SOCMESS_FILE_NEW, "r"))) {
      log("SYSERR: can't open socials file '%s': %s", SOCMESS_FILE_NEW, strerror(errno));
      exit(1);
    }
    /* count socials */
    *next_soc = '\0';
    while (!feof(fl)) {
      fgets(next_soc, MAX_STRING_LENGTH, fl);
      if (*next_soc == '~') top_of_socialt++;
    }
  } else { /* old style */

    /* open social file */
    if (!(fl = fopen(SOCMESS_FILE, "r"))) {
      log("SYSERR: can't open socials file '%s': %s", SOCMESS_FILE, strerror(errno));
      exit(1);
    }
    /* count socials */
    while (!feof(fl)) {
      fgets(next_soc, MAX_STRING_LENGTH, fl);
      if (*next_soc == '\n' || *next_soc == '\r') top_of_socialt++; /* all socials are followed by a blank line */
    }
  }

  log("Social table contains %d socials.", top_of_socialt);
  rewind(fl);
  
  CREATE(soc_mess_list, struct social_messg, top_of_socialt + 1);

  /* now read 'em */
  for (;;) {
    fscanf(fl, " %s ", next_soc);
    if (*next_soc == '$') break;

    if (CONFIG_NEW_SOCIALS == TRUE) {
      if (fscanf(fl, " %s %d %d %d %d \n",
  		sorted, &hide, &min_char_pos, &min_pos, &min_lvl) != 5) {
      log("SYSERR: format error in social file near social '%s'", next_soc);
      exit(1);
    }
      curr_soc++;
      soc_mess_list[curr_soc].command = strdup(next_soc+1);
      soc_mess_list[curr_soc].sort_as = strdup(sorted);
      soc_mess_list[curr_soc].hide = hide;
      soc_mess_list[curr_soc].min_char_position = min_char_pos;
      soc_mess_list[curr_soc].min_victim_position = min_pos;
      soc_mess_list[curr_soc].min_level_char = min_lvl;
    } else {  /* old style */
      if (fscanf(fl, " %d %d \n", &hide, &min_pos) != 2) {
        log("SYSERR: format error in social file near social '%s'", next_soc);
        exit(1);
      }
      curr_soc++;
      soc_mess_list[curr_soc].command = strdup(next_soc);
      soc_mess_list[curr_soc].sort_as = strdup(next_soc);
      soc_mess_list[curr_soc].hide = hide;
      soc_mess_list[curr_soc].min_char_position = POS_RESTING;
      soc_mess_list[curr_soc].min_victim_position = min_pos;
      soc_mess_list[curr_soc].min_level_char = 0;
    }

#ifdef CIRCLE_ACORN
    if (fgetc(fl) != '\n')
      log("SYSERR: Acorn bug workaround failed.");
#endif

    soc_mess_list[curr_soc].char_no_arg = fread_action(fl, nr);
    soc_mess_list[curr_soc].others_no_arg = fread_action(fl, nr);
    soc_mess_list[curr_soc].char_found = fread_action(fl, nr);

    /* if no char_found, the rest is to be ignored */
    if (CONFIG_NEW_SOCIALS == FALSE && !soc_mess_list[curr_soc].char_found)
      continue;

    soc_mess_list[curr_soc].others_found = fread_action(fl, nr);
    soc_mess_list[curr_soc].vict_found = fread_action(fl, nr);
    soc_mess_list[curr_soc].not_found = fread_action(fl, nr);
    soc_mess_list[curr_soc].char_auto = fread_action(fl, nr);
    soc_mess_list[curr_soc].others_auto = fread_action(fl, nr);

    if (CONFIG_NEW_SOCIALS == FALSE) 
      continue;

    soc_mess_list[curr_soc].char_body_found = fread_action(fl, nr);
    soc_mess_list[curr_soc].others_body_found = fread_action(fl, nr);
    soc_mess_list[curr_soc].vict_body_found = fread_action(fl, nr);
    soc_mess_list[curr_soc].char_obj_found = fread_action(fl, nr);
    soc_mess_list[curr_soc].others_obj_found = fread_action(fl, nr);
  }

  /* close file & set top */
  fclose(fl);
  assert(curr_soc <= top_of_socialt);
  top_of_socialt = curr_soc;
}

/* this function adds in the loaded socials and assigns them a command # */
void create_command_list(void)
{
  int i, j, k;
  struct social_messg temp;
  extern struct command_info cmd_info[];

  /* free up old command list */
  if (complete_cmd_info) 
    free_command_list();

  /* re check the sort on the socials */
  for (j = 0; j < top_of_socialt; j++) {
    k = j;
    for (i = j + 1; i <= top_of_socialt; i++)
      if (str_cmp(soc_mess_list[i].sort_as, soc_mess_list[k].sort_as) < 0)
        k = i;
    if (j != k) {
      temp = soc_mess_list[j];
      soc_mess_list[j] = soc_mess_list[k];
      soc_mess_list[k] = temp;
    }
  }

  /* count the commands in the command list */
  i = 0;
  while(*cmd_info[i].command != '\n') i++;
  i++;

  CREATE(complete_cmd_info, struct command_info, top_of_socialt + i + 2);

  /* this loop sorts the socials and commands together into one big list */
  i = 0;
  j = 0;
  k = 0;
  while ((*cmd_info[i].command != '\n') || (j <= top_of_socialt))  {
    if ((i < RESERVE_CMDS) || (j > top_of_socialt) || 
	(str_cmp(cmd_info[i].sort_as, soc_mess_list[j].sort_as) < 1))
      complete_cmd_info[k++] = cmd_info[i++];
    else {
      soc_mess_list[j].act_nr		= k;
      complete_cmd_info[k].command		= soc_mess_list[j].command;
      complete_cmd_info[k].sort_as		= soc_mess_list[j].sort_as;
      complete_cmd_info[k].minimum_position	= soc_mess_list[j].min_char_position;
      complete_cmd_info[k].command_pointer	= do_action;
      complete_cmd_info[k].minimum_level    	= soc_mess_list[j++].min_level_char;
      complete_cmd_info[k++].subcmd		= 0;
    }
  }
  complete_cmd_info[k].command		= strdup("\n");
  complete_cmd_info[k].sort_as		= strdup("zzzzzzz");
  complete_cmd_info[k].minimum_position = 0;
  complete_cmd_info[k].command_pointer	= 0;
  complete_cmd_info[k].minimum_level	= 0;
  complete_cmd_info[k].subcmd		= 0;
  log("Command info rebuilt, %d total commands.", k);
}

void free_command_list(void) 
{
  int i;
  
  for (i = 0;*complete_cmd_info[i].command !='\n';i++);
  
  free((char *)complete_cmd_info[i].command); /* special case, the terminator */
  free((char *)complete_cmd_info[i].sort_as);
  free(complete_cmd_info);
  complete_cmd_info = NULL;
}

char *fread_action(FILE *fl, int nr)
{
  char buf[MAX_STRING_LENGTH];

  fgets(buf, MAX_STRING_LENGTH, fl);
  if (feof(fl)) {
    log("SYSERR: fread_action: unexpected EOF near action #%d", nr);
    exit(1);
  }
  if (*buf == '#')
    return (NULL);

  buf[strlen(buf) - 1] = '\0';
  return (strdup(buf));
}

void free_social_messages(void) 
{
  struct social_messg *mess;
  int i;
  
  for (i = 0;i <= top_of_socialt;i++)  {
    mess = &soc_mess_list[i];
    free_action(mess);
  }
  free(soc_mess_list);
  
}  

void free_action(struct social_messg *mess)  {
  if (mess->command) free(mess->command);
  if (mess->sort_as) free(mess->sort_as);
  if (mess->char_no_arg) free(mess->char_no_arg);
  if (mess->others_no_arg) free(mess->others_no_arg);
  if (mess->char_found) free(mess->char_found);
  if (mess->others_found) free(mess->others_found);
  if (mess->vict_found) free(mess->vict_found);
  if (mess->char_body_found) free(mess->char_body_found);
  if (mess->others_body_found) free(mess->others_body_found);
  if (mess->vict_body_found) free(mess->vict_body_found);
  if (mess->not_found) free(mess->not_found);
  if (mess->char_auto) free(mess->char_auto);
  if (mess->others_auto) free(mess->others_auto);
  if (mess->char_obj_found) free(mess->char_obj_found);
  if (mess->others_obj_found) free(mess->others_obj_found);
  memset(mess, 0, sizeof(struct social_messg));
}

int find_action(int cmd)
{
  int bot, top, mid;

  bot = 0;
  top = top_of_socialt;

  if (top < 0)
    return (-1);

  for (;;) {
    mid = (bot + top) / 2;

    if (soc_mess_list[mid].act_nr == cmd)
      return (mid);
    if (bot >= top)
      return (-1);

    if (soc_mess_list[mid].act_nr > cmd)
      top = --mid;
    else
      bot = ++mid;
  }
}

ACMD(do_gmote)
{
  int act_nr, length;
  char arg[MAX_INPUT_LENGTH], buf[MAX_INPUT_LENGTH];
  struct social_messg *action;
  struct char_data *vict = NULL;

  half_chop(argument, buf, arg);

  if(subcmd)
    for (length = strlen(buf), cmd = 0; *complete_cmd_info[cmd].command != '\n'; cmd++)
      if (!strncmp(complete_cmd_info[cmd].command, buf, length))
        break;

  if ((act_nr = find_action(cmd)) < 0) {
    send_to_char(ch, "That's not a social!\r\n");
    return;
  }

  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_SOUNDPROOF)) { 
    send_to_char(ch, "The walls seem to absorb your actions.\r\n");
    return;
  }

action = &soc_mess_list[act_nr];

  if (!action->char_found)
    *arg = '\0';

  if (!*arg) {
    if(!action->others_no_arg || !*action->others_no_arg) {
      send_to_char(ch, "Who are you going to do that to?\r\n");
      return;
    }
    snprintf(buf, sizeof(buf), "Gossip: %s", action->others_no_arg);
  } else if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_WORLD))) {
    send_to_char(ch, "%s\r\n", action->not_found);
    return;
  } else if (vict == ch) {
    if(!action->others_auto || !*action->others_auto) {
      send_to_char(ch, "%s\r\n", action->char_auto);
      return;
    }
    snprintf(buf, sizeof(buf), "Gossip: %s", action->others_auto);
  } else {
    if (GET_POS(vict) < action->min_victim_position) {
      act("$N is not in a proper position for that.",
           FALSE, ch, 0, vict, TO_CHAR | TO_SLEEP);
      return;
    }
    snprintf(buf, sizeof(buf), "Gossip: %s", action->others_found);
  }
  act(buf, FALSE, ch, 0, vict, TO_GMOTE);
}
