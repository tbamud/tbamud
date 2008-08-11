/**************************************************************************
*  File: act.social.c                                      Part of tbaMUD *
*  Usage: Functions to handle socials.                                    *
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
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "screen.h"
#include "spells.h"
#include "act.h"

/* local defined functions for local use */
/* do_action and do_gmote utility function */
static int find_action(int cmd);


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
      send_to_char(ch, "Erm, no.\r\n");
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

/* this function adds in the loaded socials and assigns them a command # */
void create_command_list(void)
{
  int i, j, k;
  struct social_messg temp;

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
	complete_cmd_info[k] = cmd_info[i];
  log("Command info rebuilt, %d total commands.", k);
}

void free_command_list(void)
{
  free(complete_cmd_info);
  complete_cmd_info = NULL;
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

static int find_action(int cmd)
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
    snprintf(buf, sizeof(buf), "Gemote: $n%s", argument);
    act(buf, FALSE, ch, 0, vict, TO_GMOTE);
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
    snprintf(buf, sizeof(buf), "Gemote: %s", action->others_no_arg);
  } else if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_WORLD))) {
    send_to_char(ch, "%s\r\n", action->not_found);
    return;
  } else if (vict == ch) {
    if(!action->others_auto || !*action->others_auto) {
      send_to_char(ch, "%s\r\n", action->char_auto);
      return;
    }
    snprintf(buf, sizeof(buf), "Gemote: %s", action->others_auto);
  } else {
    if (GET_POS(vict) < action->min_victim_position) {
      act("$N is not in a proper position for that.",
           FALSE, ch, 0, vict, TO_CHAR | TO_SLEEP);
      return;
    }
    snprintf(buf, sizeof(buf), "Gemote: %s", action->others_found);
  }
  act(buf, FALSE, ch, 0, vict, TO_GMOTE);
}
