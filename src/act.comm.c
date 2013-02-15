/**************************************************************************
*  File: act.comm.c                                        Part of tbaMUD *
*  Usage: Player-level communication commands.                            *
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
#include "improved-edit.h"
#include "dg_scripts.h"
#include "act.h"
#include "modify.h"

static bool legal_communication(char * arg);

static bool legal_communication(char * arg) 
{
  while (*arg) {
    if (*arg == '@') {
      arg++;
      if (*arg == '(' || *arg == ')' || *arg == '<' || *arg == '>')
        return FALSE; 
    }
    arg++;
  }
  return TRUE;
}

ACMD(do_say)
{
  skip_spaces(&argument);

  if (!*argument)
    send_to_char(ch, "Yes, but WHAT do you want to say?\r\n");
  else {
    char buf[MAX_INPUT_LENGTH + 14], *msg;
    struct char_data *vict;
 
    if (CONFIG_SPECIAL_IN_COMM && legal_communication(argument))
      parse_at(argument);

    snprintf(buf, sizeof(buf), "$n\tn says, '%s'", argument);
    msg = act(buf, FALSE, ch, 0, 0, TO_ROOM | DG_NO_TRIG);

    for (vict = world[IN_ROOM(ch)].people; vict; vict = vict->next_in_room)
      if (vict != ch && GET_POS(vict) > POS_SLEEPING)
        add_history(vict, msg, HIST_SAY);

    if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_NOREPEAT))
      send_to_char(ch, "%s", CONFIG_OK);
    else {
      sprintf(buf, "You say, '%s'", argument);
      msg = act(buf, FALSE, ch, 0, 0, TO_CHAR | DG_NO_TRIG);
      add_history(ch, msg, HIST_SAY);
    }
  }

  /* Trigger check. */
  speech_mtrigger(ch, argument);
  speech_wtrigger(ch, argument);
}

ACMD(do_gsay)
{
  skip_spaces(&argument);
  
  if (!GROUP(ch)) {
    send_to_char(ch, "But you are not a member of a group!\r\n");
    return;
  }
  if (!*argument)
    send_to_char(ch, "Yes, but WHAT do you want to group-say?\r\n");
  else {
		
    if (CONFIG_SPECIAL_IN_COMM && legal_communication(argument))
      parse_at(argument);		
		
    send_to_group(ch, ch->group, "%s%s%s says, '%s'%s\r\n", CCGRN(ch, C_NRM), CCGRN(ch, C_NRM), GET_NAME(ch), argument, CCNRM(ch, C_NRM));
	
    if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_NOREPEAT))
      send_to_char(ch, "%s", CONFIG_OK);
    else
      send_to_char(ch, "%sYou group-say, '%s'%s\r\n", CCGRN(ch, C_NRM), argument, CCNRM(ch, C_NRM));
	}
}

static void perform_tell(struct char_data *ch, struct char_data *vict, char *arg)
{
  char buf[MAX_STRING_LENGTH], *msg;

  snprintf(buf, sizeof(buf), "%s$n tells you, '%s'%s", CCRED(vict, C_NRM), arg, CCNRM(vict, C_NRM));
  msg = act(buf, FALSE, ch, 0, vict, TO_VICT | TO_SLEEP);
  add_history(vict, msg, HIST_TELL);

  if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_NOREPEAT))
    send_to_char(ch, "%s", CONFIG_OK);
  else {
    snprintf(buf, sizeof(buf), "%sYou tell $N, '%s'%s", CCRED(ch, C_NRM), arg, CCNRM(ch, C_NRM));
    msg = act(buf, FALSE, ch, 0, vict, TO_CHAR | TO_SLEEP);     
    add_history(ch, msg, HIST_TELL);
  }

  if (!IS_NPC(vict) && !IS_NPC(ch))
    GET_LAST_TELL(vict) = GET_IDNUM(ch);
}

static int is_tell_ok(struct char_data *ch, struct char_data *vict)
{
  if (!ch)
    log("SYSERR: is_tell_ok called with no characters");
  else if (!vict)
    send_to_char(ch, "%s", CONFIG_NOPERSON);
  else if (ch == vict)
    send_to_char(ch, "You try to tell yourself something.\r\n");
  else if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_NOTELL))
    send_to_char(ch, "You can't tell other people while you have notell on.\r\n");
  else if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_SOUNDPROOF) && (GET_LEVEL(ch) < LVL_GOD))
    send_to_char(ch, "The walls seem to absorb your words.\r\n");
  else if (!IS_NPC(vict) && !vict->desc)        /* linkless */
    act("$E's linkless at the moment.", FALSE, ch, 0, vict, TO_CHAR | TO_SLEEP);
  else if (PLR_FLAGGED(vict, PLR_WRITING))
    act("$E's writing a message right now; try again later.", FALSE, ch, 0, vict, TO_CHAR | TO_SLEEP);
  else if ((!IS_NPC(vict) && PRF_FLAGGED(vict, PRF_NOTELL)) || (ROOM_FLAGGED(IN_ROOM(vict), ROOM_SOUNDPROOF) && (GET_LEVEL(ch) < LVL_GOD)))
    act("$E can't hear you.", FALSE, ch, 0, vict, TO_CHAR | TO_SLEEP);
  else
    return (TRUE);

  return (FALSE);
}

/* Yes, do_tell probably could be combined with whisper and ask, but it is
 * called frequently, and should IMHO be kept as tight as possible. */
ACMD(do_tell)
{
  struct char_data *vict = NULL;
  char buf[MAX_INPUT_LENGTH], buf2[MAX_INPUT_LENGTH];

  half_chop(argument, buf, buf2);

  if (!*buf || !*buf2)
    send_to_char(ch, "Who do you wish to tell what??\r\n");
  else if (!strcmp(buf, "m-w")) {
#ifdef CIRCLE_WINDOWS
   /* getpid() is not portable */
    send_to_char(ch, "Sorry, that is not available in the windows port.\r\n");
#else /* all other configurations */
    int i;
    char word[MAX_INPUT_LENGTH], *p, *q;

    if (last_webster_teller != -1L) {
      if (GET_IDNUM(ch) == last_webster_teller) {
        send_to_char(ch, "You are still waiting for a response.\r\n");
        return;
      } else {
        send_to_char(ch, "Hold on, m-w is busy. Try again in a couple of seconds.\r\n");
        return;
      }
    }

    /* Only a-z and +/- allowed. */
    for (p = buf2, q = word; *p ; p++)
      if ((LOWER(*p) <= 'z' && LOWER(*p) >= 'a') || (*p == '+') || (*p == '-'))
        *q++ = *p;

    *q = '\0';

    if (!*word) {
      send_to_char(ch, "Sorry, only letters and +/- are allowed characters.\r\n");
      return;
    }
    snprintf(buf, sizeof(buf), "../bin/webster %s %d &", word, (int) getpid());
    i = system(buf);
    last_webster_teller = GET_IDNUM(ch);
    send_to_char(ch, "You look up '%s' in Merriam-Webster.\r\n", word);
#endif /* platform specific part */
  } else if (GET_LEVEL(ch) < LVL_IMMORT && !(vict = get_player_vis(ch, buf, NULL, FIND_CHAR_WORLD)))
    send_to_char(ch, "%s", CONFIG_NOPERSON);
  else if (GET_LEVEL(ch) >= LVL_IMMORT && !(vict = get_char_vis(ch, buf, NULL, FIND_CHAR_WORLD)))
    send_to_char(ch, "%s", CONFIG_NOPERSON);
  else if (is_tell_ok(ch, vict)) {
    if (CONFIG_SPECIAL_IN_COMM && legal_communication(argument))
      parse_at(buf2);
    perform_tell(ch, vict, buf2);
	}
}

ACMD(do_reply)
{
  struct char_data *tch = character_list;

  if (IS_NPC(ch))
    return;

  skip_spaces(&argument);

  if (GET_LAST_TELL(ch) == NOBODY)
    send_to_char(ch, "You have nobody to reply to!\r\n");
  else if (!*argument)
    send_to_char(ch, "What is your reply?\r\n");
  else {
    /* Make sure the person you're replying to is still playing by searching
     * for them.  Note, now last tell is stored as player IDnum instead of
     * a pointer, which is much better because it's safer, plus will still
     * work if someone logs out and back in again. A descriptor list based 
     * search would be faster although we could not find link dead people.  
     * Not that they can hear tells anyway. :) -gg 2/24/98 */
    while (tch && (IS_NPC(tch) || GET_IDNUM(tch) != GET_LAST_TELL(ch)))
      tch = tch->next;

    if (!tch)
      send_to_char(ch, "They are no longer playing.\r\n");
    else if (is_tell_ok(ch, tch)) {
      if (CONFIG_SPECIAL_IN_COMM && legal_communication(argument))
        parse_at(argument);
      perform_tell(ch, tch, argument);
		}
  }
}

ACMD(do_spec_comm)
{
  char buf[MAX_INPUT_LENGTH], buf2[MAX_INPUT_LENGTH];
  struct char_data *vict;
  const char *action_sing, *action_plur, *action_others;

  switch (subcmd) {
  case SCMD_WHISPER:
    action_sing = "whisper to";
    action_plur = "whispers to";
    action_others = "$n whispers something to $N.";
    break;

  case SCMD_ASK:
    action_sing = "ask";
    action_plur = "asks";
    action_others = "$n asks $N a question.";
    break;

  default:
    action_sing = "oops";
    action_plur = "oopses";
    action_others = "$n is tongue-tied trying to speak with $N.";
    break;
  }

  half_chop(argument, buf, buf2);

  if (!*buf || !*buf2)
    send_to_char(ch, "Whom do you want to %s.. and what??\r\n", action_sing);
  else if (!(vict = get_char_vis(ch, buf, NULL, FIND_CHAR_ROOM)))
    send_to_char(ch, "%s", CONFIG_NOPERSON);
  else if (vict == ch)
    send_to_char(ch, "You can't get your mouth close enough to your ear...\r\n");
  else {
    char buf1[MAX_STRING_LENGTH];

    if (CONFIG_SPECIAL_IN_COMM && legal_communication(argument))
      parse_at(buf2);

    snprintf(buf1, sizeof(buf1), "$n %s you, '%s'", action_plur, buf2);
    act(buf1, FALSE, ch, 0, vict, TO_VICT);

    if ((!IS_NPC(ch)) && (PRF_FLAGGED(ch, PRF_NOREPEAT))) 
      send_to_char(ch, "%s", CONFIG_OK);
    else
      send_to_char(ch, "You %s %s, '%s'\r\n", action_sing, GET_NAME(vict), buf2);
    act(action_others, FALSE, ch, 0, vict, TO_NOTVICT);
  }
}

ACMD(do_write)
{
  struct obj_data *paper, *pen = NULL;
  char *papername, *penname;
  char buf1[MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH];

  papername = buf1;
  penname = buf2;

  two_arguments(argument, papername, penname);

  if (!ch->desc)
    return;

  if (!*papername) {
    /* Nothing was delivered. */
    send_to_char(ch, "Write?  With what?  ON what?  What are you trying to do?!?\r\n");
    return;
  }
  if (*penname) {
    /* Nothing was delivered. */
    if (!(paper = get_obj_in_list_vis(ch, papername, NULL, ch->carrying))) {
      send_to_char(ch, "You have no %s.\r\n", papername);
      return;
    }
    if (!(pen = get_obj_in_list_vis(ch, penname, NULL, ch->carrying))) {
      send_to_char(ch, "You have no %s.\r\n", penname);
      return;
    }
  } else { /* There was one arg.. let's see what we can find. */
    if (!(paper = get_obj_in_list_vis(ch, papername, NULL, ch->carrying))) {
      send_to_char(ch, "There is no %s in your inventory.\r\n", papername);
      return;
    }
    if (GET_OBJ_TYPE(paper) == ITEM_PEN) { /* Oops, a pen. */
      pen = paper;
      paper = NULL;
    } else if (GET_OBJ_TYPE(paper) != ITEM_NOTE) {
      send_to_char(ch, "That thing has nothing to do with writing.\r\n");
      return;
    }

    /* One object was found.. now for the other one. */
    if (!GET_EQ(ch, WEAR_HOLD)) {
      send_to_char(ch, "You can't write with %s %s alone.\r\n", AN(papername), papername);
      return;
    }
    if (!CAN_SEE_OBJ(ch, GET_EQ(ch, WEAR_HOLD))) {
      send_to_char(ch, "The stuff in your hand is invisible!  Yeech!!\r\n");
      return;
    }
    if (pen)
      paper = GET_EQ(ch, WEAR_HOLD);
    else
      pen = GET_EQ(ch, WEAR_HOLD);
  }

  /* Now let's see what kind of stuff we've found. */
  if (GET_OBJ_TYPE(pen) != ITEM_PEN)
    act("$p is no good for writing with.", FALSE, ch, pen, 0, TO_CHAR);
  else if (GET_OBJ_TYPE(paper) != ITEM_NOTE)
    act("You can't write on $p.", FALSE, ch, paper, 0, TO_CHAR);
  else {
    char *backstr = NULL;

    /* Something on it, display it as that's in input buffer. */
    if (paper->action_description) {
      backstr = strdup(paper->action_description);
      send_to_char(ch, "There's something written on it already:\r\n");
      send_to_char(ch, "%s", paper->action_description);
    }

    /* We can write. */
    act("$n begins to jot down a note.", TRUE, ch, 0, 0, TO_ROOM);
    send_editor_help(ch->desc);
    string_write(ch->desc, &paper->action_description, MAX_NOTE_LENGTH, 0, backstr);
  }
}

ACMD(do_page)
{
  struct descriptor_data *d;
  struct char_data *vict;
  char buf2[MAX_INPUT_LENGTH], arg[MAX_INPUT_LENGTH];

  half_chop(argument, arg, buf2);

  if (IS_NPC(ch))
    send_to_char(ch, "Monsters can't page.. go away.\r\n");
  else if (!*arg)
    send_to_char(ch, "Whom do you wish to page?\r\n");
  else {
    char buf[MAX_STRING_LENGTH];

    snprintf(buf, sizeof(buf), "\007\007*$n* %s", buf2);
    if (!str_cmp(arg, "all")) {
      if (GET_LEVEL(ch) > LVL_GOD) {
	for (d = descriptor_list; d; d = d->next)
	  if (STATE(d) == CON_PLAYING && d->character)
	    act(buf, FALSE, ch, 0, d->character, TO_VICT);
      } else
	send_to_char(ch, "You will never be godly enough to do that!\r\n");
      return;
    }
    if ((vict = get_char_vis(ch, arg, NULL, FIND_CHAR_WORLD))) {
      act(buf, FALSE, ch, 0, vict, TO_VICT);
      if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_NOREPEAT))
	send_to_char(ch, "%s", CONFIG_OK);
      else
	act(buf, FALSE, ch, 0, vict, TO_CHAR);
    } else
      send_to_char(ch, "There is no such person in the game!\r\n");
  }
}

/* Generalized communication function by Fred C. Merkel (Torg). */
ACMD(do_gen_comm)
{
  struct descriptor_data *i;
  char color_on[24];
  char buf1[MAX_INPUT_LENGTH], buf2[MAX_INPUT_LENGTH], *msg;
  bool emoting = FALSE;

  /* Array of flags which must _not_ be set in order for comm to be heard. */
  int channels[] = {
    0,
    PRF_NOSHOUT,
    PRF_NOGOSS,
    PRF_NOAUCT,
    PRF_NOGRATZ,
    PRF_NOGOSS,
    0
  };

  int hist_type[] = {
    HIST_HOLLER,
    HIST_SHOUT,
    HIST_GOSSIP,
    HIST_AUCTION,
    HIST_GRATS,
  };

  /* com_msgs: [0] Message if you can't perform the action because of noshout
   *           [1] name of the action
   *           [2] message if you're not on the channel
   *           [3] a color string. */
  const char *com_msgs[][4] = {
    {"You cannot holler!!\r\n",
      "holler",
      "",
      KYEL},

    {"You cannot shout!!\r\n",
      "shout",
      "Turn off your noshout flag first!\r\n",
      KYEL},

    {"You cannot gossip!!\r\n",
      "gossip",
      "You aren't even on the channel!\r\n",
      KYEL},

    {"You cannot auction!!\r\n",
      "auction",
      "You aren't even on the channel!\r\n",
      KMAG},

    {"You cannot congratulate!\r\n",
      "congrat",
      "You aren't even on the channel!\r\n",
      KGRN},

    {"You cannot gossip your emotions!\r\n",
      "gossip",
      "You aren't even on the channel!\r\n",
      KYEL}
  };

  if (PLR_FLAGGED(ch, PLR_NOSHOUT)) {
    send_to_char(ch, "%s", com_msgs[subcmd][0]);
    return;
  }
  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_SOUNDPROOF) && (GET_LEVEL(ch) < LVL_GOD)) {
    send_to_char(ch, "The walls seem to absorb your words.\r\n");
    return;
  }


  if (subcmd == SCMD_GEMOTE) {
    if (!*argument)
      send_to_char(ch, "Gemote? Yes? Gemote what?\r\n");
    else
      do_gmote(ch, argument, 0, 1);
    return;
  }

  /* Level_can_shout defined in config.c. */
  if (GET_LEVEL(ch) < CONFIG_LEVEL_CAN_SHOUT) {
    send_to_char(ch, "You must be at least level %d before you can %s.\r\n", CONFIG_LEVEL_CAN_SHOUT, com_msgs[subcmd][1]);
    return;
  }
  /* Make sure the char is on the channel. */
  if (!IS_NPC(ch) && PRF_FLAGGED(ch, channels[subcmd])) {
    send_to_char(ch, "%s", com_msgs[subcmd][2]);
    return;
  }

  /* skip leading spaces */
  skip_spaces(&argument);

  /* Make sure that there is something there to say! */
  if (!*argument) {
    send_to_char(ch, "Yes, %s, fine, %s we must, but WHAT???\r\n", com_msgs[subcmd][1], com_msgs[subcmd][1]);
    return;
  }
  if (subcmd == SCMD_HOLLER) {
    if (GET_MOVE(ch) < CONFIG_HOLLER_MOVE_COST) {
      send_to_char(ch, "You're too exhausted to holler.\r\n");
      return;
    } else
      GET_MOVE(ch) -= CONFIG_HOLLER_MOVE_COST;
  }
  /* Set up the color on code. */
  strlcpy(color_on, com_msgs[subcmd][3], sizeof(color_on));

  /* First, set up strings to be given to the communicator. */
  if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_NOREPEAT))
    send_to_char(ch, "%s", CONFIG_OK);
  else {
		if (CONFIG_SPECIAL_IN_COMM && legal_communication(argument))
      parse_at(argument);
      
    snprintf(buf1, sizeof(buf1), "%sYou %s, '%s%s'%s", COLOR_LEV(ch) >= C_CMP ? color_on : "",
        com_msgs[subcmd][1], argument, COLOR_LEV(ch) >= C_CMP ? color_on : "", CCNRM(ch, C_CMP));
    
    msg = act(buf1, FALSE, ch, 0, 0, TO_CHAR | TO_SLEEP);
    add_history(ch, msg, hist_type[subcmd]);
  }
  if (!emoting)
    snprintf(buf1, sizeof(buf1), "$n %ss, '%s'", com_msgs[subcmd][1], argument);

  /* Now send all the strings out. */
  for (i = descriptor_list; i; i = i->next) {
    if (STATE(i) != CON_PLAYING || i == ch->desc || !i->character )
      continue;
    if (!IS_NPC(ch) && (PRF_FLAGGED(i->character, channels[subcmd]) || PLR_FLAGGED(i->character, PLR_WRITING)))
      continue;

    if (ROOM_FLAGGED(IN_ROOM(i->character), ROOM_SOUNDPROOF) && (GET_LEVEL(ch) < LVL_GOD))
      continue;

    if (subcmd == SCMD_SHOUT && ((world[IN_ROOM(ch)].zone != world[IN_ROOM(i->character)].zone) ||
         !AWAKE(i->character)))
      continue;

    snprintf(buf2, sizeof(buf2), "%s%s%s", (COLOR_LEV(i->character) >= C_NRM) ? color_on : "", buf1, KNRM); 
    msg = act(buf2, FALSE, ch, 0, i->character, TO_VICT | TO_SLEEP);
    add_history(i->character, msg, hist_type[subcmd]);
  }
}

ACMD(do_qcomm)
{
  if (!PRF_FLAGGED(ch, PRF_QUEST)) {
    send_to_char(ch, "You aren't even part of the quest!\r\n");
    return;
  }
  skip_spaces(&argument);

  if (!*argument)
    send_to_char(ch, "%c%s?  Yes, fine, %s we must, but WHAT??\r\n", UPPER(*CMD_NAME), CMD_NAME + 1, CMD_NAME);
  else {
    char buf[MAX_STRING_LENGTH];
    struct descriptor_data *i;
    
    if (CONFIG_SPECIAL_IN_COMM && legal_communication(argument))
      parse_at(argument);

    if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_NOREPEAT))
      send_to_char(ch, "%s", CONFIG_OK);
    else if (subcmd == SCMD_QSAY) {
      snprintf(buf, sizeof(buf), "You quest-say, '%s'", argument);
      act(buf, FALSE, ch, 0, argument, TO_CHAR);
    } else
      act(argument, FALSE, ch, 0, argument, TO_CHAR);

    if (subcmd == SCMD_QSAY)
      snprintf(buf, sizeof(buf), "$n quest-says, '%s'", argument);
    else {
      strlcpy(buf, argument, sizeof(buf));
      mudlog(CMP, MAX(LVL_BUILDER, GET_INVIS_LEV(ch)), TRUE, "(GC) %s qechoed: %s", GET_NAME(ch), argument);
    }
    for (i = descriptor_list; i; i = i->next)
      if (STATE(i) == CON_PLAYING && i != ch->desc && PRF_FLAGGED(i->character, PRF_QUEST))
        act(buf, 0, ch, 0, i->character, TO_VICT | TO_SLEEP);
  }
}
