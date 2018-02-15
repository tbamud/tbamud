/**************************************************************************
*  File: prefedit.c                                        Part of tbaMUD *
*  Usage: Player-level OLC for setting preferences and toggles            *
*                                                                         *
*  Created by Jamdog for tbaMUD 3.59                                      *
*  All rights reserved.  See license for complete information.            *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
**************************************************************************/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "comm.h"
#include "utils.h"
#include "handler.h"
#include "interpreter.h"
#include "db.h"
#include "oasis.h"
#include "prefedit.h"
#include "screen.h"

/* Internal (static) functions */
static void prefedit_setup(struct descriptor_data *d, struct char_data *vict);
static void prefedit_save_to_char(struct descriptor_data *d);
static void prefedit_disp_main_menu(struct descriptor_data *d);
static void prefedit_disp_toggles_menu(struct descriptor_data *d);
static void prefedit_disp_prompt_menu(struct descriptor_data *d);
static void prefedit_disp_color_menu(struct descriptor_data *d);
static void prefedit_disp_syslog_menu(struct descriptor_data *d);

/* Note: there is no setup_new, as you can ONLY edit an existing player */
/*       vict is normally = d->character, except when imps edit players */
static void prefedit_setup(struct descriptor_data *d, struct char_data *vict)
{
  int i;
  struct prefs_data *toggles;

  if (!vict) vict = d->character;

  /*. Build a copy of the player's toggles .*/
  CREATE (toggles, struct prefs_data, 1);

  /* no strings to allocate space for */
  for (i=0; i<PR_ARRAY_MAX; i++)
    toggles->pref_flags[i] = PRF_FLAGS(vict)[i];

  toggles->wimp_level   = GET_WIMP_LEV(vict);
  toggles->page_length  = GET_PAGE_LENGTH(vict);
  toggles->screen_width = GET_SCREEN_WIDTH(vict);

  toggles->ch = vict;

  /*. Attach toggles copy to editors descriptor .*/
  OLC_PREFS(d) = toggles;
  OLC_VAL(d) = 0;
  prefedit_disp_main_menu(d);
}

static void prefedit_save_to_char(struct descriptor_data *d)
{
  int i;
  struct char_data *vict;

  vict = PREFEDIT_GET_CHAR;

  if (vict && vict->desc && IS_PLAYING(vict->desc))
  {
    for (i=0; i<PR_ARRAY_MAX; i++)
      PRF_FLAGS(vict)[i]  = OLC_PREFS(d)->pref_flags[i];

    GET_WIMP_LEV(vict)     = OLC_PREFS(d)->wimp_level;
    GET_PAGE_LENGTH(vict)  = OLC_PREFS(d)->page_length;
    GET_SCREEN_WIDTH(vict) = OLC_PREFS(d)->screen_width;

    save_char(vict);
  }
  else
  {
  if (!vict) {
    mudlog(BRF, LVL_BUILDER, TRUE, "SYSERR: Unable to save toggles (no vict)");
    send_to_char(d->character, "Unable to save toggles (no vict)");
  } else if (!vict->desc) {
    mudlog(BRF, LVL_BUILDER, TRUE, "SYSERR: Unable to save toggles (no vict descriptor)");
    send_to_char(d->character, "Unable to save toggles (no vict descriptor)");
  } else if (!IS_PLAYING(vict->desc)) {
    mudlog(BRF, LVL_BUILDER, TRUE, "SYSERR: Unable to save toggles (vict not playing)");
    send_to_char(d->character, "Unable to save toggles (vict not playing)");
  } else {
    mudlog(BRF, LVL_BUILDER, TRUE, "SYSERR: Unable to save toggles (unknown reason)");
    send_to_char(d->character, "Unable to save toggles (unknown reason)");
    }
  }
}

static void prefedit_disp_main_menu(struct descriptor_data *d)
{
  struct char_data *vict;
  char prompt_string[10], color_string[10], syslog_string[10];
  const char *multi_types[] = { "Off", "Brief", "Normal", "Complete", "\n" };

  /* Set up the required variables and strings */
  vict = PREFEDIT_GET_CHAR;

  sprintf(prompt_string, "%s%s%s", PREFEDIT_FLAGGED(PRF_DISPHP) ? "H" : "",   PREFEDIT_FLAGGED(PRF_DISPMANA) ? "M" : "",
                                   PREFEDIT_FLAGGED(PRF_DISPMOVE) ? "V" : "" );

  sprintf(color_string, "%s", multi_types[(PREFEDIT_FLAGGED(PRF_COLOR_1) ? 1 : 0) + (PREFEDIT_FLAGGED(PRF_COLOR_2) ? 2 : 0)]);

  send_to_char(d->character, "\r\n%sPreferences for %s%s\r\n",
                              CCYEL(d->character, C_NRM),
                              GET_NAME(vict),
                              CCNRM(d->character, C_NRM) );


  /* The mortal preferences section of the actual menu */
  send_to_char(d->character, "\r\n"
                             "%sPreferences\r\n"
                             "%sP%s) Prompt : %s[%s%-3s%s]         %sL%s) Pagelength : %s[%s%-3d%s]\r\n"
                             "%sC%s) Color  : %s[%s%-8s%s]    %sS%s) Screenwidth: %s[%s%-3d%s]\r\n"
                             "%sW%s) Wimpy  : %s[%s%-4d%s]%s\r\n",
             CCWHT(d->character, C_NRM),
/* Line 1 - prompt and pagelength */
             CBYEL(d->character, C_NRM), CCNRM(d->character, C_NRM), CCCYN(d->character, C_NRM), CCYEL(d->character, C_NRM),
             prompt_string, CCCYN(d->character, C_NRM), CBYEL(d->character, C_NRM), CCNRM(d->character, C_NRM),
             CCCYN(d->character, C_NRM), CCYEL(d->character, C_NRM), PREFEDIT_GET_PAGELENGTH, CCCYN(d->character, C_NRM),
/* Line 2 - color and screenwidth */
             CBYEL(d->character, C_NRM), CCNRM(d->character, C_NRM), CCCYN(d->character, C_NRM), CCYEL(d->character, C_NRM),
             color_string, CCCYN(d->character, C_NRM), CBYEL(d->character, C_NRM), CCNRM(d->character, C_NRM),
             CCCYN(d->character, C_NRM), CCYEL(d->character, C_NRM), PREFEDIT_GET_SCREENWIDTH, CCCYN(d->character, C_NRM),
/* Line 2 - wimpy                 */
             CBYEL(d->character, C_NRM), CCNRM(d->character, C_NRM), CCCYN(d->character, C_NRM), CCYEL(d->character, C_NRM),
             PREFEDIT_GET_WIMP_LEV, CCCYN(d->character, C_NRM), CCNRM(d->character, C_NRM)
             );

  send_to_char(d->character, "%sT%s) Toggle Preferences...\r\n",
             CBYEL(d->character, C_NRM), CCNRM(d->character, C_NRM) );

  /* Imm Prefs */
  if (GET_LEVEL(PREFEDIT_GET_CHAR) >= LVL_IMMORT)
  {
    sprintf(syslog_string, "%s", multi_types[((PREFEDIT_FLAGGED(PRF_LOG1) ? 1 : 0)+ (PREFEDIT_FLAGGED(PRF_LOG2) ? 2 : 0))] );

    send_to_char(d->character, "\r\n"
                               "%sImmortal Preferences\r\n"
                               "%s1%s) Syslog Level %s[%s%8s%s]   %s4%s) ClsOLC    %s[%s%3s%s]\r\n"
                               "%s2%s) Show Flags   %s[%s%3s%s]        %s5%s) No WizNet %s[%s%3s%s]\r\n"
                               "%s3%s) No Hassle    %s[%s%3s%s]        %s6%s) Holylight %s[%s%3s%s]\r\n",
             CBWHT(d->character, C_NRM),
/* Line 1 - syslog and clsolc */
             CBYEL(d->character, C_NRM), CCNRM(d->character, C_NRM), CCCYN(d->character, C_NRM), CCYEL(d->character, C_NRM),
             syslog_string, CCCYN(d->character, C_NRM), CBYEL(d->character, C_NRM), CCNRM(d->character, C_NRM),
             CCCYN(d->character, C_NRM), CCYEL(d->character, C_NRM), ONOFF(PREFEDIT_FLAGGED(PRF_CLS)), CCCYN(d->character, C_NRM),
/* Line 2 - show vnums and nowiz */
             CBYEL(d->character, C_NRM), CCNRM(d->character, C_NRM), CCCYN(d->character, C_NRM), CCYEL(d->character, C_NRM),
             ONOFF(PREFEDIT_FLAGGED(PRF_SHOWVNUMS)), CCCYN(d->character, C_NRM), CBYEL(d->character, C_NRM), CCNRM(d->character, C_NRM),
             CCCYN(d->character, C_NRM), CCYEL(d->character, C_NRM), ONOFF(PREFEDIT_FLAGGED(PRF_NOWIZ)), CCCYN(d->character, C_NRM),
/* Line 3 - nohassle and holylight */
             CBYEL(d->character, C_NRM), CCNRM(d->character, C_NRM), CCCYN(d->character, C_NRM), CCYEL(d->character, C_NRM),
             ONOFF(PREFEDIT_FLAGGED(PRF_NOHASSLE)), CCCYN(d->character, C_NRM), CBYEL(d->character, C_NRM), CCNRM(d->character, C_NRM),
             CCCYN(d->character, C_NRM), CCYEL(d->character, C_NRM), ONOFF(PREFEDIT_FLAGGED(PRF_HOLYLIGHT)), CCCYN(d->character, C_NRM)
             );
    if (GET_LEVEL(PREFEDIT_GET_CHAR) == LVL_IMPL)
      send_to_char(d->character, "%s7%s) Zone Resets  %s[%s%3s%s]\r\n",
             CBYEL(d->character, C_NRM), CCNRM(d->character, C_NRM), CCCYN(d->character, C_NRM), CCYEL(d->character, C_NRM),
             ONOFF(PREFEDIT_FLAGGED(PRF_ZONERESETS)), CCCYN(d->character, C_NRM));
  }

/* Finishing Off */
  send_to_char(d->character, "\r\n"
                             "%sD%s) Restore all default values\r\n"
                             "%sQ%s) Quit\r\n"
                             "\r\n",
             CBYEL(d->character, C_NRM), CCNRM(d->character, C_NRM),
             CBYEL(d->character, C_NRM), CCNRM(d->character, C_NRM)
             );


  /* Bottom of the menu */

  OLC_MODE(d) = PREFEDIT_MAIN_MENU;
}

static void prefedit_disp_toggles_menu(struct descriptor_data *d)
{
  struct char_data *vict;

  /* Set up the required variables and strings */
  vict = OLC_PREFS(d)->ch;

  /* Top of the menu */
  send_to_char(d->character, "Toggle preferences for %s%-20s\r\n",
             CBGRN(d->character, C_NRM), GET_NAME(vict));

  send_to_char(d->character, "\r\n"
                             "%sAuto-flags                 Channels\r\n",
             CBWHT(d->character, C_NRM));

  /* The top section of the actual menu */
  send_to_char(d->character, "%s1%s) Autoexits    %s[%s%3s%s]      %sA%s) Gossip   %s[%s%3s%s]\r\n"
                             "%s2%s) Autoloot     %s[%s%3s%s]      %sB%s) Shout    %s[%s%3s%s]\r\n"
                             "%s3%s) Autogold     %s[%s%3s%s]      %sC%s) Tell     %s[%s%3s%s]\r\n"
                             "%s4%s) Autosac      %s[%s%3s%s]      %sD%s) Auction  %s[%s%3s%s]\r\n"
                             "%s5%s) Autoassist   %s[%s%3s%s]      %sE%s) Gratz    %s[%s%3s%s]\r\n"
                             "%s6%s) Autosplit    %s[%s%3s%s]\r\n",
/* Line 1 - autoexits and gossip */
             CBYEL(d->character, C_NRM), CCNRM(d->character, C_NRM), CCCYN(d->character, C_NRM), PREFEDIT_FLAGGED(PRF_AUTOEXIT) ? CBGRN(d->character, C_NRM) : CBRED(d->character, C_NRM),
             ONOFF(PREFEDIT_FLAGGED(PRF_AUTOEXIT)), CCCYN(d->character, C_NRM), CBYEL(d->character, C_NRM), CCNRM(d->character, C_NRM), CCCYN(d->character, C_NRM),
             PREFEDIT_FLAGGED(PRF_NOGOSS) ? CBRED(d->character, C_NRM) : CBGRN(d->character, C_NRM), ONOFF(!PREFEDIT_FLAGGED(PRF_NOGOSS)), CCCYN(d->character, C_NRM),
/* Line 2 - autoloot and shout */
             CBYEL(d->character, C_NRM), CCNRM(d->character, C_NRM), CCCYN(d->character, C_NRM), PREFEDIT_FLAGGED(PRF_AUTOLOOT) ? CBGRN(d->character, C_NRM) : CBRED(d->character, C_NRM),
             ONOFF(PREFEDIT_FLAGGED(PRF_AUTOLOOT)), CCCYN(d->character, C_NRM), CBYEL(d->character, C_NRM), CCNRM(d->character, C_NRM), CCCYN(d->character, C_NRM),
             PREFEDIT_FLAGGED(PRF_NOSHOUT) ? CBRED(d->character, C_NRM) : CBGRN(d->character, C_NRM), ONOFF(!PREFEDIT_FLAGGED(PRF_NOSHOUT)), CCCYN(d->character, C_NRM),
/* Line 3 - autogold and tell */
             CBYEL(d->character, C_NRM), CCNRM(d->character, C_NRM), CCCYN(d->character, C_NRM), PREFEDIT_FLAGGED(PRF_AUTOGOLD) ? CBGRN(d->character, C_NRM) : CBRED(d->character, C_NRM),
             ONOFF(PREFEDIT_FLAGGED(PRF_AUTOGOLD)), CCCYN(d->character, C_NRM), CBYEL(d->character, C_NRM), CCNRM(d->character, C_NRM), CCCYN(d->character, C_NRM),
             PREFEDIT_FLAGGED(PRF_NOTELL) ? CBRED(d->character, C_NRM) : CBGRN(d->character, C_NRM), ONOFF(!PREFEDIT_FLAGGED(PRF_NOTELL)), CCCYN(d->character, C_NRM),
/* Line 4 - autosac and auction */
             CBYEL(d->character, C_NRM), CCNRM(d->character, C_NRM), CCCYN(d->character, C_NRM), PREFEDIT_FLAGGED(PRF_AUTOSAC) ? CBGRN(d->character, C_NRM) : CBRED(d->character, C_NRM),
             ONOFF(PREFEDIT_FLAGGED(PRF_AUTOSAC)), CCCYN(d->character, C_NRM), CBYEL(d->character, C_NRM), CCNRM(d->character, C_NRM), CCCYN(d->character, C_NRM),
             PREFEDIT_FLAGGED(PRF_NOAUCT) ? CBRED(d->character, C_NRM) : CBGRN(d->character, C_NRM), ONOFF(!PREFEDIT_FLAGGED(PRF_NOAUCT)), CCCYN(d->character, C_NRM),
/* Line 5 - autoassist and grats */
             CBYEL(d->character, C_NRM), CCNRM(d->character, C_NRM), CCCYN(d->character, C_NRM), PREFEDIT_FLAGGED(PRF_AUTOASSIST) ? CBGRN(d->character, C_NRM) : CBRED(d->character, C_NRM),
             ONOFF(PREFEDIT_FLAGGED(PRF_AUTOASSIST)), CCCYN(d->character, C_NRM), CBYEL(d->character, C_NRM), CCNRM(d->character, C_NRM), CCCYN(d->character, C_NRM),
             PREFEDIT_FLAGGED(PRF_NOGRATZ) ? CBRED(d->character, C_NRM) : CBGRN(d->character, C_NRM), ONOFF(!PREFEDIT_FLAGGED(PRF_NOGRATZ)), CCCYN(d->character, C_NRM),
/* Line 6 - autosplit */
             CBYEL(d->character, C_NRM), CCNRM(d->character, C_NRM), CCCYN(d->character, C_NRM), PREFEDIT_FLAGGED(PRF_AUTOSPLIT) ? CBGRN(d->character, C_NRM) : CBRED(d->character, C_NRM),
             ONOFF(PREFEDIT_FLAGGED(PRF_AUTOSPLIT)), CCCYN(d->character, C_NRM)
             );

  send_to_char(d->character, "%s7%s) Automap      %s[%s%3s%s]\r\n"
                             "%s8%s) Autokey      %s[%s%3s%s]\r\n"
                             "%s9%s) Autodoor     %s[%s%3s%s]\r\n",
/* Line 7 - automap */
             CBYEL(d->character, C_NRM), CCNRM(d->character, C_NRM), CCCYN(d->character, C_NRM), PREFEDIT_FLAGGED(PRF_AUTOMAP) ? CBGRN(d->character, C_NRM) : CBRED(d->character, C_NRM),
             ONOFF(PREFEDIT_FLAGGED(PRF_AUTOMAP)), CCCYN(d->character, C_NRM),
/* Line 8 - autokey */
             CBYEL(d->character, C_NRM), CCNRM(d->character, C_NRM), CCCYN(d->character, C_NRM), PREFEDIT_FLAGGED(PRF_AUTOKEY) ? CBGRN(d->character, C_NRM) : CBRED(d->character, C_NRM),
             ONOFF(PREFEDIT_FLAGGED(PRF_AUTOKEY)), CCCYN(d->character, C_NRM),
/* Line 9 - autodoor */
             CBYEL(d->character, C_NRM), CCNRM(d->character, C_NRM), CCCYN(d->character, C_NRM), PREFEDIT_FLAGGED(PRF_AUTODOOR) ? CBGRN(d->character, C_NRM) : CBRED(d->character, C_NRM),
             ONOFF(PREFEDIT_FLAGGED(PRF_AUTODOOR)), CCCYN(d->character, C_NRM)
             );

  /* The bottom section of the toggles menu */
  send_to_char(d->character, "\r\n"
                             "%sOther Flags\r\n"
                             "%sF%s) No Summon    %s[%s%3s%s]      %sH%s) Brief    %s[%s%3s%s]\r\n"
                             "%sG%s) No Repeat    %s[%s%3s%s]      %sI%s) Compact  %s[%s%3s%s]\r\n",
             CBWHT(d->character, C_NRM),
/* Line 10 - nosummon and brief */
             CBYEL(d->character, C_NRM), CCNRM(d->character, C_NRM), CCCYN(d->character, C_NRM), CCYEL(d->character, C_NRM),
             ONOFF(!PREFEDIT_FLAGGED(PRF_SUMMONABLE)), CCCYN(d->character, C_NRM), CBYEL(d->character, C_NRM), CCNRM(d->character, C_NRM),
             CCCYN(d->character, C_NRM), CCYEL(d->character, C_NRM), ONOFF(PREFEDIT_FLAGGED(PRF_BRIEF)), CCCYN(d->character, C_NRM),
/* Line 11 - norepeat and compact */
             CBYEL(d->character, C_NRM), CCNRM(d->character, C_NRM), CCCYN(d->character, C_NRM), CCYEL(d->character, C_NRM),
             ONOFF(PREFEDIT_FLAGGED(PRF_NOREPEAT)), CCCYN(d->character, C_NRM), CBYEL(d->character, C_NRM), CCNRM(d->character, C_NRM),
             CCCYN(d->character, C_NRM), CCYEL(d->character, C_NRM), ONOFF(PREFEDIT_FLAGGED(PRF_COMPACT)), CCCYN(d->character, C_NRM)
             );

  /* The bottom section of the toggles menu */
  send_to_char(d->character, "\r\n"
                             "%sProtocol Settings:\r\n"
                             "%sJ%s) Xterm 256    %s[%s%3s%s]      %sM%s) MXP      %s[%s%3s%s]\r\n"
                             "%sK%s) ANSI         %s[%s%3s%s]      %sN%s) MSDP     %s[%s%3s%s]\r\n"
                             "%sL%s) Charset      %s[%s%3s%s]      %sO%s) ATCP     %s[%s%3s%s]\r\n"
                             "%sP%s) UTF-8        %s[%s%3s%s]      %sR%s) MSP      %s[%s%3s%s]\r\n"
                             "\r\n",
             CBWHT(d->character, C_NRM),
/* Line 12 - 256 and mxp */
             CBYEL(d->character, C_NRM), CCNRM(d->character, C_NRM), CCCYN(d->character, C_NRM), CCYEL(d->character, C_NRM),
             ONOFF(d->pProtocol->pVariables[eMSDP_XTERM_256_COLORS]->ValueInt), CCCYN(d->character, C_NRM), CBYEL(d->character, C_NRM), CCNRM(d->character, C_NRM),
             CCCYN(d->character, C_NRM), CCYEL(d->character, C_NRM), ONOFF(d->pProtocol->pVariables[eMSDP_MXP]->ValueInt), CCCYN(d->character, C_NRM),
/* Line 13 - ansi and msdp */
             CBYEL(d->character, C_NRM), CCNRM(d->character, C_NRM), CCCYN(d->character, C_NRM), CCYEL(d->character, C_NRM),
             ONOFF(d->pProtocol->pVariables[eMSDP_ANSI_COLORS]->ValueInt), CCCYN(d->character, C_NRM), CBYEL(d->character, C_NRM), CCNRM(d->character, C_NRM),
             CCCYN(d->character, C_NRM), CCYEL(d->character, C_NRM), ONOFF(d->pProtocol->bMSDP), CCCYN(d->character, C_NRM),
/* Line 14 - charset and atcp */
             CBYEL(d->character, C_NRM), CCNRM(d->character, C_NRM), CCCYN(d->character, C_NRM), CCYEL(d->character, C_NRM),
             ONOFF(d->pProtocol->bCHARSET), CCCYN(d->character, C_NRM), CBYEL(d->character, C_NRM), CCNRM(d->character, C_NRM),
             CCCYN(d->character, C_NRM), CCYEL(d->character, C_NRM), ONOFF(d->pProtocol->bATCP), CCCYN(d->character, C_NRM),
/* Line 15 - utf-8 and msp */
             CBYEL(d->character, C_NRM), CCNRM(d->character, C_NRM), CCCYN(d->character, C_NRM), CCYEL(d->character, C_NRM),
             ONOFF(d->pProtocol->pVariables[eMSDP_UTF_8]->ValueInt), CCCYN(d->character, C_NRM), CBYEL(d->character, C_NRM), CCNRM(d->character, C_NRM),
             CCCYN(d->character, C_NRM), CCYEL(d->character, C_NRM), ONOFF(d->pProtocol->bMSP), CCCYN(d->character, C_NRM)
             );
/* Finishing Off */
  send_to_char(d->character, "%sQ%s) Quit toggle preferences...\r\n",
             CBYEL(d->character, C_NRM), CCNRM(d->character, C_NRM) );

  OLC_MODE(d) = PREFEDIT_TOGGLE_MENU;
}

static void prefedit_disp_prompt_menu(struct descriptor_data *d)
{
  char prompt_string[7];

  if (PREFEDIT_FLAGGED(PRF_DISPAUTO))
    sprintf(prompt_string, "<Auto>");
  else
    sprintf(prompt_string, "%s%s%s", PREFEDIT_FLAGGED(PRF_DISPHP) ? "H" : "",   PREFEDIT_FLAGGED(PRF_DISPMANA) ? "M" : "",
                                     PREFEDIT_FLAGGED(PRF_DISPMOVE) ? "V" : "");

  send_to_char(d->character, "%sPrompt Settings\r\n"
                             "%s1%s) Toggle HP\r\n"
                             "%s2%s) Toggle Mana\r\n"
                             "%s3%s) Toggle Moves\r\n"
                             "%s4%s) Toggle auto flag\r\n\r\n"
                             "%sCurrent Prompt: %s%s%s\r\n\r\n"
                             "%s0%s) Quit (to main menu)\r\n",
                             CBWHT(d->character, C_NRM), CBYEL(d->character, C_NRM), CCNRM(d->character, C_NRM),
                             CBYEL(d->character, C_NRM), CCNRM(d->character, C_NRM),
                             CBYEL(d->character, C_NRM), CCNRM(d->character, C_NRM),
                             CBYEL(d->character, C_NRM), CCNRM(d->character, C_NRM),
                             CCNRM(d->character, C_NRM), CCCYN(d->character, C_NRM), prompt_string, CCNRM(d->character, C_NRM),
                             CBYEL(d->character, C_NRM), CCNRM(d->character, C_NRM) );

  send_to_char(d->character, "Enter Choice :");
  OLC_MODE(d) = PREFEDIT_PROMPT;
}

static void prefedit_disp_color_menu(struct descriptor_data *d)
{
  send_to_char(d->character, "%sColor level\r\n"
                             "%s1%s) Off      %s(do not display any color - monochrome)%s\r\n"
                             "%s2%s) Brief    %s(show minimal color where necessary)%s\r\n"
                             "%s3%s) Normal   %s(show game-enhancing color)%s\r\n"
                             "%s4%s) On       %s(show all colors whenever possible)%s\r\n",
                             CBWHT(d->character, C_NRM),
                             CBYEL(d->character, C_NRM), CCNRM(d->character, C_NRM), CCYEL(d->character, C_NRM), CCNRM(d->character, C_NRM),
                             CBYEL(d->character, C_NRM), CCNRM(d->character, C_NRM), CCYEL(d->character, C_NRM), CCNRM(d->character, C_NRM),
                             CBYEL(d->character, C_NRM), CCNRM(d->character, C_NRM), CCYEL(d->character, C_NRM), CCNRM(d->character, C_NRM),
                             CBYEL(d->character, C_NRM), CCNRM(d->character, C_NRM), CCYEL(d->character, C_NRM), CCNRM(d->character, C_NRM) );

  send_to_char(d->character, "Enter Choice :");
  OLC_MODE(d) = PREFEDIT_COLOR;
}

static void prefedit_disp_syslog_menu(struct descriptor_data *d)
{
  send_to_char(d->character, "%sSyslog level\r\n"
                             "%s1%s) Off      %s(do not display any logs or error messages)%s\r\n"
                             "%s2%s) Brief    %s(show only important warnings or errors)%s\r\n"
                             "%s3%s) Normal   %s(show all warnings and errors)%s\r\n"
                             "%s4%s) Complete %s(show all logged information for your level)%s\r\n",
                             CBWHT(d->character, C_NRM),
                             CBYEL(d->character, C_NRM), CCNRM(d->character, C_NRM), CCYEL(d->character, C_NRM), CCNRM(d->character, C_NRM),
                             CBYEL(d->character, C_NRM), CCNRM(d->character, C_NRM), CCYEL(d->character, C_NRM), CCNRM(d->character, C_NRM),
                             CBYEL(d->character, C_NRM), CCNRM(d->character, C_NRM), CCYEL(d->character, C_NRM), CCNRM(d->character, C_NRM),
                             CBYEL(d->character, C_NRM), CCNRM(d->character, C_NRM), CCYEL(d->character, C_NRM), CCNRM(d->character, C_NRM) );

  send_to_char(d->character, "Enter Choice :");
  OLC_MODE(d) = PREFEDIT_SYSLOG;
}

void prefedit_parse(struct descriptor_data * d, char *arg)
{
  int number;

  switch (OLC_MODE(d)) {
  case PREFEDIT_CONFIRM_SAVE:
    switch (*arg) {
    case 'y':
    case 'Y':
      prefedit_save_to_char(d);
      mudlog(CMP, MAX(LVL_BUILDER, GET_INVIS_LEV(d->character)), TRUE, "OLC: %s edits toggles for %s", 
        GET_NAME(d->character), GET_NAME(OLC_PREFS(d)->ch));
      /*. No strings to save - cleanup all .*/
      cleanup_olc(d, CLEANUP_ALL);
      break;
    case 'n':
    case 'N':
      /* don't save to char, just free everything up */
      cleanup_olc(d, CLEANUP_ALL);
      break;
    default:
      send_to_char(d->character, "Invalid choice!\r\n");
      send_to_char(d->character, "Do you wish to save these toggle settings? : ");
      break;
    }
    return;

  case PREFEDIT_MAIN_MENU:
    switch (*arg) {
    case 'q':
    case 'Q':
      if (OLC_VAL(d))
      { /*. Something has been modified .*/
        send_to_char(d->character, "Do you wish to save these toggle settings? : ");
        OLC_MODE(d) = PREFEDIT_CONFIRM_SAVE;
      } else
        cleanup_olc(d, CLEANUP_ALL);
      return;

    case 'p':
    case 'P':
      prefedit_disp_prompt_menu(d);
      return;

    case 'c':
    case 'C':
      prefedit_disp_color_menu(d);
      return;

    case 'l':
    case 'L':
      send_to_char(d->character, "Enter number of lines per page (10-60): ");
      OLC_MODE(d) = PREFEDIT_PAGELENGTH;
      return;

    case 's':
    case 'S':
      send_to_char(d->character, "Enter number of columns per page (40-120): ");
      OLC_MODE(d) = PREFEDIT_SCREENWIDTH;
      return;

    case 'w':
    case 'W':
      send_to_char(d->character, "Enter HP at which to flee (0-%d): ", MIN(GET_MAX_HIT(d->character)/2, 500));
      OLC_MODE(d) = PREFEDIT_WIMPY;
      return;

    case 't':
    case 'T':
      prefedit_disp_toggles_menu(d);
      return;

    case 'd':
    case 'D':
      prefedit_Restore_Defaults(d);
      break;

    /* Below this point are Imm-only toggles */
    case '1':
      if (GET_LEVEL(PREFEDIT_GET_CHAR) < LVL_IMMORT)
      {
        send_to_char(d->character, "%sInvalid choice!%s\r\n", CBRED(d->character, C_NRM), CCNRM(d->character, C_NRM));
        prefedit_disp_main_menu(d);
      }
      else
      {
        prefedit_disp_syslog_menu(d);
        return;
      }
      break;

    case '2':
      if (GET_LEVEL(PREFEDIT_GET_CHAR) < LVL_IMMORT)
      {
        send_to_char(d->character, "%sInvalid choice!%s\r\n", CBRED(d->character, C_NRM), CCNRM(d->character, C_NRM));
        prefedit_disp_main_menu(d);
      }
      else
      {
        TOGGLE_BIT_AR(PREFEDIT_GET_FLAGS, PRF_SHOWVNUMS);
      }
      break;

    case '3':
      if (GET_LEVEL(PREFEDIT_GET_CHAR) < LVL_IMMORT)
      {
        send_to_char(d->character, "%sInvalid choice!%s\r\n", CBRED(d->character, C_NRM), CCNRM(d->character, C_NRM));
        prefedit_disp_main_menu(d);
      }
      else
      {
        TOGGLE_BIT_AR(PREFEDIT_GET_FLAGS, PRF_NOHASSLE);
      }
      break;

    case '4':
      if (GET_LEVEL(PREFEDIT_GET_CHAR) < LVL_IMMORT)
      {
        send_to_char(d->character, "%sInvalid choice!%s\r\n", CBRED(d->character, C_NRM), CCNRM(d->character, C_NRM));
        prefedit_disp_main_menu(d);
      }
      else
      {
        TOGGLE_BIT_AR(PREFEDIT_GET_FLAGS, PRF_CLS);
      }
      break;

    case '5':
      if (GET_LEVEL(PREFEDIT_GET_CHAR) < LVL_IMMORT)
      {
        send_to_char(d->character, "%sInvalid choice!%s\r\n", CBRED(d->character, C_NRM), CCNRM(d->character, C_NRM));
        prefedit_disp_main_menu(d);
      }
      else
      {
        TOGGLE_BIT_AR(PREFEDIT_GET_FLAGS, PRF_NOWIZ);
      }
      break;

    case '6':
      if (GET_LEVEL(PREFEDIT_GET_CHAR) < LVL_IMMORT)
      {
        send_to_char(d->character, "%sInvalid choice!%s\r\n", CBRED(d->character, C_NRM), CCNRM(d->character, C_NRM));
        prefedit_disp_main_menu(d);
      }
      else
      {
        TOGGLE_BIT_AR(PREFEDIT_GET_FLAGS, PRF_HOLYLIGHT);
      }
      break;

    case '7':
      if (GET_LEVEL(PREFEDIT_GET_CHAR) < LVL_IMPL)
      {
        send_to_char(d->character, "%sInvalid choice!%s\r\n", CBRED(d->character, C_NRM), CCNRM(d->character, C_NRM));
        prefedit_disp_main_menu(d);
      }
      else
      {
        TOGGLE_BIT_AR(PREFEDIT_GET_FLAGS, PRF_ZONERESETS);
      }
      break;

    default:
        send_to_char(d->character, "%sInvalid choice!%s\r\n", CBRED(d->character, C_NRM), CCNRM(d->character, C_NRM));
      prefedit_disp_main_menu(d);
      break;
    }
    break;

  case PREFEDIT_PAGELENGTH:
    number = atoi(arg);
    OLC_PREFS(d)->page_length = MAX(10, MIN(number, 60));
    break;

  case PREFEDIT_SCREENWIDTH:
    number = atoi(arg);
    OLC_PREFS(d)->screen_width = MAX(40, MIN(number, 120));
    break;

  case PREFEDIT_WIMPY:
    number = atoi(arg);
    OLC_PREFS(d)->wimp_level = MAX(0, MIN(number, 500));
    break;

  case PREFEDIT_COLOR:
    number = atoi(arg) - 1;
    if ((number < 0) || (number > 3)) {
      send_to_char(d->character, "%sThat's not a valid choice!%s\r\n", CBRED(d->character, C_NRM), CCNRM(d->character, C_NRM));
      prefedit_disp_color_menu(d);
      return;
    }
    REMOVE_BIT_AR(PREFEDIT_GET_FLAGS, PRF_COLOR_1);
    REMOVE_BIT_AR(PREFEDIT_GET_FLAGS, PRF_COLOR_2);
    if (number % 2)   SET_BIT_AR(PREFEDIT_GET_FLAGS, PRF_COLOR_1);
    if (number >= 2)  SET_BIT_AR(PREFEDIT_GET_FLAGS, PRF_COLOR_2);

    break;

  case PREFEDIT_TOGGLE_MENU:
    switch (*arg) {
      case 'q':
      case 'Q':
      case 'x' :
      case 'X' : prefedit_disp_main_menu(d);
                 return;

      case '1':
        TOGGLE_BIT_AR(PREFEDIT_GET_FLAGS, PRF_AUTOEXIT);
        break;

      case '2':
        TOGGLE_BIT_AR(PREFEDIT_GET_FLAGS, PRF_AUTOLOOT);
        break;

      case '3':
        TOGGLE_BIT_AR(PREFEDIT_GET_FLAGS, PRF_AUTOGOLD);
        break;

      case '4':
        TOGGLE_BIT_AR(PREFEDIT_GET_FLAGS, PRF_AUTOSAC);
        break;

      case '5':
        TOGGLE_BIT_AR(PREFEDIT_GET_FLAGS, PRF_AUTOASSIST);
        break;

      case '6':
        TOGGLE_BIT_AR(PREFEDIT_GET_FLAGS, PRF_AUTOSPLIT);
        break;

      case '7':
        TOGGLE_BIT_AR(PREFEDIT_GET_FLAGS, PRF_AUTOMAP);
        break;

      case '8':
        TOGGLE_BIT_AR(PREFEDIT_GET_FLAGS, PRF_AUTOKEY);
        break;

      case '9':
        TOGGLE_BIT_AR(PREFEDIT_GET_FLAGS, PRF_AUTODOOR);
        break;

      case 'a':
      case 'A':
        TOGGLE_BIT_AR(PREFEDIT_GET_FLAGS, PRF_NOGOSS);
        break;

      case 'b':
      case 'B':
        TOGGLE_BIT_AR(PREFEDIT_GET_FLAGS, PRF_NOSHOUT);
        break;

      case 'c':
      case 'C':
        TOGGLE_BIT_AR(PREFEDIT_GET_FLAGS, PRF_NOTELL);
        break;

      case 'd':
      case 'D':
        TOGGLE_BIT_AR(PREFEDIT_GET_FLAGS, PRF_NOAUCT);
        break;

      case 'e':
      case 'E':
        TOGGLE_BIT_AR(PREFEDIT_GET_FLAGS, PRF_NOGRATZ);
        break;

      case 'f':
      case 'F':
        TOGGLE_BIT_AR(PREFEDIT_GET_FLAGS, PRF_SUMMONABLE);
        break;

      case 'g':
      case 'G':
        TOGGLE_BIT_AR(PREFEDIT_GET_FLAGS, PRF_NOREPEAT);
        break;

      case 'h':
      case 'H':
        TOGGLE_BIT_AR(PREFEDIT_GET_FLAGS, PRF_BRIEF);
        break;

      case 'i':
      case 'I':
        TOGGLE_BIT_AR(PREFEDIT_GET_FLAGS, PRF_COMPACT);
        break;
        
      case 'j':
      case 'J':
        TOGGLE_VAR(d->pProtocol->pVariables[eMSDP_XTERM_256_COLORS]->ValueInt);
        break;
        
      case 'k':
      case 'K':
        TOGGLE_VAR(d->pProtocol->pVariables[eMSDP_ANSI_COLORS]->ValueInt);
        break;        

      case 'l':
      case 'L':
        TOGGLE_VAR(d->pProtocol->bCHARSET);
        break;        
        
      case 'm':
      case 'M':
        TOGGLE_VAR(d->pProtocol->pVariables[eMSDP_MXP]->ValueInt);
        break;

      case 'n':
      case 'N':
        TOGGLE_VAR(d->pProtocol->bMSDP);
        break;
        
      case 'o':
      case 'O':
        TOGGLE_VAR(d->pProtocol->bATCP);
        break;      

      case 'p':
      case 'P':
        TOGGLE_VAR(d->pProtocol->pVariables[eMSDP_UTF_8]->ValueInt);
        break;

      case 'r':
      case 'R':
        TOGGLE_VAR(d->pProtocol->bMSP);
        break;   

      default  : send_to_char(d->character, "Invalid Choice, try again (Q to Quit to main menu): ");
                 return;
    }
    /* Set the 'value has changed' flag thing */
    OLC_VAL(d) = 1;
    prefedit_disp_toggles_menu(d);

    return;

  case PREFEDIT_SYSLOG:
    number = atoi(arg) - 1;
    if ((number < 0) || (number > 3)) {
      send_to_char(d->character, "%sThat's not a valid choice!%s\r\n", CBRED(d->character, C_NRM), CCNRM(d->character, C_NRM));
      prefedit_disp_color_menu(d);
      return;
    }

    if ((number % 2) == 1) SET_BIT_AR(PREFEDIT_GET_FLAGS, PRF_LOG1);
    else                   REMOVE_BIT_AR(PREFEDIT_GET_FLAGS, PRF_LOG1);

    if (number >= 2) SET_BIT_AR(PREFEDIT_GET_FLAGS, PRF_LOG2);
    else             REMOVE_BIT_AR(PREFEDIT_GET_FLAGS, PRF_LOG2);

    break;

  /* Sub-menu's and flag toggle menu's */
  case PREFEDIT_PROMPT:
    number = atoi(arg);
    if ((number < 0) || (number > 7)) {
      send_to_char(d->character, "%sThat's not a valid choice!%s\r\n", CBRED(d->character, C_NRM), CCNRM(d->character, C_NRM));
      prefedit_disp_prompt_menu(d);
    } else {
      if (number == 0)
        break;
      else
      {
        /* toggle bits */
        if (number == 1)
        {
          if (PREFEDIT_FLAGGED(PRF_DISPHP))
            REMOVE_BIT_AR(PREFEDIT_GET_FLAGS, PRF_DISPHP);
          else
            SET_BIT_AR(PREFEDIT_GET_FLAGS, PRF_DISPHP);
        }
        else if (number == 2)
        {
          if (PREFEDIT_FLAGGED(PRF_DISPMANA))
            REMOVE_BIT_AR(PREFEDIT_GET_FLAGS, PRF_DISPMANA);
          else
            SET_BIT_AR(PREFEDIT_GET_FLAGS, PRF_DISPMANA);
        }
        else if (number == 3)
        {
          if (PREFEDIT_FLAGGED(PRF_DISPMOVE))
            REMOVE_BIT_AR(PREFEDIT_GET_FLAGS, PRF_DISPMOVE);
          else
            SET_BIT_AR(PREFEDIT_GET_FLAGS, PRF_DISPMOVE);
        }
        else if (number == 4)
        {
          if (PREFEDIT_FLAGGED(PRF_DISPAUTO))
            REMOVE_BIT_AR(PREFEDIT_GET_FLAGS, PRF_DISPAUTO);
          else
            SET_BIT_AR(PREFEDIT_GET_FLAGS, PRF_DISPAUTO);
        }
        prefedit_disp_prompt_menu(d);
      }
    }
    return;

  default:
    /* we should never get here */
    mudlog(BRF,LVL_BUILDER,TRUE,"SYSERR: Reached default case in parse_prefedit");
    break;
  }
  /*. If we get this far, something has be changed .*/
  OLC_VAL(d) = 1;
  prefedit_disp_main_menu(d);
}

void prefedit_Restore_Defaults(struct descriptor_data *d)
{
  /* Let's do toggles one at a time */
  /* PRF_BRIEF      - Off */
  if (PREFEDIT_FLAGGED(PRF_BRIEF))
     REMOVE_BIT_AR(PREFEDIT_GET_FLAGS, PRF_BRIEF);

  /* PRF_COMPACT    - Off */
  if (PREFEDIT_FLAGGED(PRF_COMPACT))
     REMOVE_BIT_AR(PREFEDIT_GET_FLAGS, PRF_COMPACT);

  /* PRF_NOSHOUT       - Off */
  if (PREFEDIT_FLAGGED(PRF_NOSHOUT))
     REMOVE_BIT_AR(PREFEDIT_GET_FLAGS, PRF_NOSHOUT);

  /* PRF_NOTELL     - Off */
  if (PREFEDIT_FLAGGED(PRF_NOTELL))
     REMOVE_BIT_AR(PREFEDIT_GET_FLAGS, PRF_NOTELL);

  /* PRF_DISPHP     - On */
  if (!PREFEDIT_FLAGGED(PRF_DISPHP))
     SET_BIT_AR(PREFEDIT_GET_FLAGS, PRF_DISPHP);

  /* PRF_DISPMANA   - On */
  if (!PREFEDIT_FLAGGED(PRF_DISPMANA))
     SET_BIT_AR(PREFEDIT_GET_FLAGS, PRF_DISPMANA);

  /* PRF_DISPMOVE   - On */
  if (!PREFEDIT_FLAGGED(PRF_DISPMOVE))
     SET_BIT_AR(PREFEDIT_GET_FLAGS, PRF_DISPMOVE);

  /* PRF_AUTOEXIT   - On */
  if (!PREFEDIT_FLAGGED(PRF_AUTOEXIT))
     SET_BIT_AR(PREFEDIT_GET_FLAGS, PRF_AUTOEXIT);

  /* PRF_NOHASSLE   - On for Imms */
  if (!PREFEDIT_FLAGGED(PRF_NOHASSLE) && GET_LEVEL(PREFEDIT_GET_CHAR) > LVL_IMMORT)
     SET_BIT_AR(PREFEDIT_GET_FLAGS, PRF_NOHASSLE);
  else
     REMOVE_BIT_AR(PREFEDIT_GET_FLAGS, PRF_NOHASSLE);

  /* PRF_QUEST      - Off */
  if (PREFEDIT_FLAGGED(PRF_QUEST))
     REMOVE_BIT_AR(PREFEDIT_GET_FLAGS, PRF_QUEST);

  /* PRF_SUMMONABLE - Off */
  if (PREFEDIT_FLAGGED(PRF_SUMMONABLE))
     REMOVE_BIT_AR(PREFEDIT_GET_FLAGS, PRF_SUMMONABLE);

  /* PRF_NOREPEAT   - Off */
  if (PREFEDIT_FLAGGED(PRF_NOREPEAT))
     REMOVE_BIT_AR(PREFEDIT_GET_FLAGS, PRF_NOREPEAT);

  /* PRF_HOLYLIGHT  - On for Imms */
  if (!PREFEDIT_FLAGGED(PRF_HOLYLIGHT) && GET_LEVEL(PREFEDIT_GET_CHAR) > LVL_IMMORT)
     SET_BIT_AR(PREFEDIT_GET_FLAGS, PRF_HOLYLIGHT);
  else
     REMOVE_BIT_AR(PREFEDIT_GET_FLAGS, PRF_HOLYLIGHT);

  /* PRF_COLOR      - On (Complete) */
  if (!PREFEDIT_FLAGGED(PRF_COLOR_1))
     SET_BIT_AR(PREFEDIT_GET_FLAGS, PRF_COLOR_1);
  if (!PREFEDIT_FLAGGED(PRF_COLOR_2))
     SET_BIT_AR(PREFEDIT_GET_FLAGS, PRF_COLOR_2);

  /* PRF_NOWIZ      - Off */
  if (PREFEDIT_FLAGGED(PRF_NOWIZ))
     REMOVE_BIT_AR(PREFEDIT_GET_FLAGS, PRF_NOWIZ);

  /* PRF_LOG1       - Off */
  if (PREFEDIT_FLAGGED(PRF_LOG1))
     REMOVE_BIT_AR(PREFEDIT_GET_FLAGS, PRF_LOG1);

  /* PRF_LOG2       - Off */
  if (PREFEDIT_FLAGGED(PRF_LOG2))
     REMOVE_BIT_AR(PREFEDIT_GET_FLAGS, PRF_LOG2);

  /* PRF_NOAUCT     - Off */
  if (PREFEDIT_FLAGGED(PRF_NOAUCT))
     REMOVE_BIT_AR(PREFEDIT_GET_FLAGS, PRF_NOAUCT);

  /* PRF_NOGOSS     - Off */
  if (PREFEDIT_FLAGGED(PRF_NOGOSS))
     REMOVE_BIT_AR(PREFEDIT_GET_FLAGS, PRF_NOGOSS);

  /* PRF_NOGRATZ    - Off */
  if (PREFEDIT_FLAGGED(PRF_NOGRATZ))
     REMOVE_BIT_AR(PREFEDIT_GET_FLAGS, PRF_NOGRATZ);

  /* PRF_SHOWVNUMS  - On for Imms */
  if (!PREFEDIT_FLAGGED(PRF_SHOWVNUMS) && GET_LEVEL(PREFEDIT_GET_CHAR) > LVL_IMMORT)
     SET_BIT_AR(PREFEDIT_GET_FLAGS, PRF_SHOWVNUMS);
  else
     REMOVE_BIT_AR(PREFEDIT_GET_FLAGS, PRF_SHOWVNUMS);

  /* PRF_DISPAUTO   - Off */
  if (PREFEDIT_FLAGGED(PRF_DISPAUTO))
     REMOVE_BIT_AR(PREFEDIT_GET_FLAGS, PRF_DISPAUTO);

  /* PRF_CLS - Off */
  if (PREFEDIT_FLAGGED(PRF_CLS))
     REMOVE_BIT_AR(PREFEDIT_GET_FLAGS, PRF_CLS);

  /* PRF_AFK - Off */
  if (PREFEDIT_FLAGGED(PRF_AFK))
     REMOVE_BIT_AR(PREFEDIT_GET_FLAGS, PRF_AFK);

  /* PRF_AUTOLOOT   - On */
  if (!PREFEDIT_FLAGGED(PRF_AUTOLOOT))
     SET_BIT_AR(PREFEDIT_GET_FLAGS, PRF_AUTOLOOT);

  /* PRF_AUTOGOLD   - On */
  if (!PREFEDIT_FLAGGED(PRF_AUTOGOLD))
     SET_BIT_AR(PREFEDIT_GET_FLAGS, PRF_AUTOGOLD);

  /* PRF_AUTOSPLIT  - Off */
  if (PREFEDIT_FLAGGED(PRF_AUTOSPLIT))
     REMOVE_BIT_AR(PREFEDIT_GET_FLAGS, PRF_AUTOSPLIT);

  /* PRF_AUTOSAC    - Off */
  if (PREFEDIT_FLAGGED(PRF_AUTOSAC))
     REMOVE_BIT_AR(PREFEDIT_GET_FLAGS, PRF_AUTOSAC);

  /* PRF_AUTOASSIST - Off */
  if (PREFEDIT_FLAGGED(PRF_AUTOASSIST))
     REMOVE_BIT_AR(PREFEDIT_GET_FLAGS, PRF_AUTOASSIST);

  /* PRF_AUTOMAP    - On */
  if (PREFEDIT_FLAGGED(PRF_AUTOMAP))
     SET_BIT_AR(PREFEDIT_GET_FLAGS, PRF_AUTOMAP);

  /* PRF_AUTOKEY    - On */
  if (PREFEDIT_FLAGGED(PRF_AUTOKEY))
     SET_BIT_AR(PREFEDIT_GET_FLAGS, PRF_AUTOKEY);

  /* PRF_AUTODOOR   - On */
  if (PREFEDIT_FLAGGED(PRF_AUTODOOR))
     SET_BIT_AR(PREFEDIT_GET_FLAGS, PRF_AUTODOOR);

  /* Other (non-toggle) options */
  PREFEDIT_GET_WIMP_LEV   = 0;   /* Wimpy off by default */
  PREFEDIT_GET_PAGELENGTH = 22;  /* Default telnet screen is 22 lines   */
  PREFEDIT_GET_SCREENWIDTH = 80; /* Default telnet screen is 80 columns */
}

ACMD(do_oasis_prefedit)
{
  struct descriptor_data *d;
  struct char_data *vict;
  char *buf3;
  char buf[MAX_STRING_LENGTH];
  char buf1[MAX_STRING_LENGTH];
  char buf2[MAX_STRING_LENGTH];

  /****************************************************************************/
  /** Parse any arguments.                                                   **/
  /****************************************************************************/
  buf3 = two_arguments(argument, buf1, buf2);

  /****************************************************************************/
  /** If there aren't any arguments...well...they can only modify their      **/
  /** own toggles, can't they?                                               **/
  /****************************************************************************/
  if (!*buf1) {
    vict = ch;
  }
  else if (GET_LEVEL(ch) >= LVL_IMPL)
  {
    if ((vict = get_player_vis(ch, buf1, NULL, FIND_CHAR_WORLD)) == NULL)
    {
      send_to_char(ch, "There is no-one here by that name.\r\n");
      return;
    }
  }
  else
  {
      send_to_char(ch, "You can't do that!\r\n");
      return;
  }

  if (IS_NPC(vict))
  {
      send_to_char(ch, "Don't be ridiculous! Mobs don't have toggles.\r\n");
      return;
  }

  /****************************************************************************/
  /** Check that whatever it is isn't already being edited.                  **/
  /****************************************************************************/
  for (d = descriptor_list; d; d = d->next) {
    if (STATE(d) == CON_PREFEDIT) {
      if (d->olc && OLC_PREFS(d)->ch == vict) {
        if (ch == vict)
          send_to_char(ch, "Your preferences are currently being edited by %s.\r\n", PERS(d->character, ch));
        else
          sprintf(buf, "$S$u preferences are currently being edited by %s.", PERS(d->character, ch));
          act(buf, FALSE, ch, 0, vict, TO_CHAR);
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
    mudlog(BRF, LVL_IMMORT, TRUE, "SYSERR: do_oasis_prefedit: Player already had olc structure.");
    free(d->olc);
  }

  CREATE(d->olc, struct oasis_olc_data, 1);

  OLC_NUM(d) = 0;

  /****************************************************************************/
  /** If this is a new quest, setup a new quest, otherwise setup the       **/
  /** existing quest.                                                       **/
  /****************************************************************************/
  prefedit_setup(d, vict);

  STATE(d) = CON_PREFEDIT;

  /****************************************************************************/
  /** Send the OLC message to the players in the same room as the builder.   **/
  /****************************************************************************/
  act("$n starts editing toggles.", TRUE, d->character, 0, 0, TO_ROOM);
  SET_BIT_AR(PLR_FLAGS(ch), PLR_WRITING);

  /****************************************************************************/
  /** Log the OLC message.                                                   **/
  /****************************************************************************/
/* No need - done elsewhere */
//  mudlog(CMP, LVL_IMMORT, TRUE, "OLC: (prefedit) %s starts editing toggles for %s", GET_NAME(ch), GET_NAME(vict));
}

