/**
* @file act.h
* Header file for the core act* c files.
*
* Part of the core tbaMUD source code distribution, which is a derivative
* of, and continuation of, CircleMUD.
*
* All rights reserved.  See license for complete information.
* Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University
* CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.
*
* @todo Utility functions that could easily be moved elsewhere have been
* marked. Suggest a review of all utility functions (aka. non ACMDs) and
* determine if the utility functions should be placed into a lower level
* (non-ACMD focused) shared module.
*
*/
#ifndef _ACT_H_
#define _ACT_H_

#include "utils.h" /* for the ACMD macro */

/*****************************************************************************
 * Begin Functions and defines for act.comm.c
 ****************************************************************************/
/* functions with subcommands */
/* do_gen_comm */
ACMD(do_gen_comm);
#define SCMD_HOLLER   0
#define SCMD_SHOUT    1
#define SCMD_GOSSIP   2
#define SCMD_AUCTION  3
#define SCMD_GRATZ    4
#define SCMD_GEMOTE   5
/* do_qcomm */
ACMD(do_qcomm);
#define SCMD_QSAY     0
#define SCMD_QECHO    1
/* do_spec_com */
ACMD(do_spec_comm);
#define SCMD_WHISPER  0
#define SCMD_ASK      1
/* functions without subcommands */
ACMD(do_say);
ACMD(do_gsay);
ACMD(do_page);
ACMD(do_reply);
ACMD(do_tell);
ACMD(do_write);
/*****************************************************************************
 * Begin Functions and defines for act.informative.c
 ****************************************************************************/
/* Utility Functions */
/** @todo Move to a utility library */
char *find_exdesc(char *word, struct extra_descr_data *list);
/** @todo Move to a mud centric string utility library */
void space_to_minus(char *str);
/** @todo Move to a help module? */
int search_help(const char *argument, int level);
void free_history(struct char_data *ch, int type);
void free_recent_players(void);
/* functions with subcommands */
/* do_commands */
ACMD(do_commands);
#define SCMD_COMMANDS 0
#define SCMD_SOCIALS  1
#define SCMD_WIZHELP  2
/* do_gen_ps */
ACMD(do_gen_ps);
#define SCMD_INFO      0
#define SCMD_HANDBOOK  1
#define SCMD_CREDITS   2
#define SCMD_NEWS      3
#define SCMD_WIZLIST   4
#define SCMD_POLICIES  5
#define SCMD_VERSION   6
#define SCMD_IMMLIST   7
#define SCMD_MOTD      8
#define SCMD_IMOTD     9
#define SCMD_CLEAR     10
#define SCMD_WHOAMI    11
/* do_look */
ACMD(do_look);
#define SCMD_LOOK 0
#define SCMD_READ 1
/* functions without subcommands */
ACMD(do_areas);
ACMD(do_consider);
ACMD(do_diagnose);
ACMD(do_equipment);
ACMD(do_examine);
ACMD(do_exits);
ACMD(do_gold);
ACMD(do_help);
ACMD(do_history);
ACMD(do_inventory);
ACMD(do_levels);
ACMD(do_scan);
ACMD(do_score);
ACMD(do_time);
ACMD(do_toggle);
ACMD(do_users);
ACMD(do_weather);
ACMD(do_where);
ACMD(do_who);
ACMD(do_whois);

/*****************************************************************************
 * Begin Functions and defines for act.item.c
 ****************************************************************************/
/* Utility Functions */
/** @todo Compare with needs of find_eq_pos_script. */
int find_eq_pos(struct char_data *ch, struct obj_data *obj, char *arg);
void name_from_drinkcon(struct obj_data *obj);
void name_to_drinkcon(struct obj_data *obj, int type);
void weight_change_object(struct obj_data *obj, int weight);
/* functions with subcommands */
/* do_drop */
ACMD(do_drop);
#define SCMD_DROP   0
#define SCMD_JUNK   1
#define SCMD_DONATE 2
/* do_eat */
ACMD(do_eat);
#define SCMD_EAT    0
#define SCMD_TASTE  1
#define SCMD_DRINK  2
#define SCMD_SIP    3
/* do_pour */
ACMD(do_pour);
#define SCMD_POUR  0
#define SCMD_FILL  1
/* functions without subcommands */
ACMD(do_drink);
ACMD(do_get);
ACMD(do_give);
ACMD(do_grab);
ACMD(do_put);
ACMD(do_remove);
ACMD(do_sac);
ACMD(do_wear);
ACMD(do_wield);


/*****************************************************************************
 * Begin Functions and defines for act.movement.c
 ****************************************************************************/
/* Functions with subcommands */
/* do_gen_door */
ACMD(do_gen_door);
#define SCMD_OPEN       0
#define SCMD_CLOSE      1
#define SCMD_UNLOCK     2
#define SCMD_LOCK       3
#define SCMD_PICK       4
/* Functions without subcommands */
ACMD(do_enter);
ACMD(do_follow);
ACMD(do_leave);
ACMD(do_move);
ACMD(do_rest);
ACMD(do_sit);
ACMD(do_sleep);
ACMD(do_stand);
ACMD(do_wake);
/* Global variables from act.movement.c */
#ifndef __ACT_MOVEMENT_C__
extern const char *cmd_door[];
#endif /* __ACT_MOVEMENT_C__ */


/*****************************************************************************
 * Begin Functions and defines for act.offensive.c
 ****************************************************************************/
/* Functions with subcommands */
/* do_hit */
ACMD(do_hit);
#define SCMD_HIT    0
/* Functions without subcommands */
ACMD(do_assist);
ACMD(do_bash);
ACMD(do_backstab);
ACMD(do_flee);
ACMD(do_kick);
ACMD(do_kill);
ACMD(do_order);
ACMD(do_rescue);
ACMD(do_whirlwind);

/*****************************************************************************
 * Begin Functions and defines for act.other.c
 ****************************************************************************/
/* Functions with subcommands */
/* do_gen_tog */
ACMD(do_gen_tog);
#define SCMD_NOSUMMON    0
#define SCMD_NOHASSLE    1
#define SCMD_BRIEF       2
#define SCMD_COMPACT     3
#define SCMD_NOTELL      4
#define SCMD_NOAUCTION   5
#define SCMD_NOSHOUT     6
#define SCMD_NOGOSSIP    7
#define SCMD_NOGRATZ     8
#define SCMD_NOWIZ       9
#define SCMD_QUEST       10
#define SCMD_SHOWVNUMS   11
#define SCMD_NOREPEAT    12
#define SCMD_HOLYLIGHT   13
#define SCMD_SLOWNS      14
#define SCMD_AUTOEXIT    15
#define SCMD_TRACK       16
#define SCMD_CLS         17
#define SCMD_BUILDWALK   18
#define SCMD_AFK         19
#define SCMD_AUTOLOOT    20
#define SCMD_AUTOGOLD    21
#define SCMD_AUTOSPLIT   22
#define SCMD_AUTOSAC     23
#define SCMD_AUTOASSIST  24
#define SCMD_AUTOMAP     25
#define SCMD_AUTOKEY     26
#define SCMD_AUTODOOR    27
#define SCMD_COLOR       28
#define SCMD_SYSLOG      29
#define SCMD_WIMPY       30
#define SCMD_PAGELENGTH  31
#define SCMD_SCREENWIDTH 32

/* do_quit */
ACMD(do_quit);
#define SCMD_QUI  0
#define SCMD_QUIT 1
/* do_use */
ACMD(do_use);
#define SCMD_USE  0
#define SCMD_QUAFF  1
#define SCMD_RECITE 2
/* Functions without subcommands */
ACMD(do_display);
ACMD(do_group);
ACMD(do_happyhour);
ACMD(do_hide);
ACMD(do_not_here);
ACMD(do_practice);
ACMD(do_report);
ACMD(do_save);
ACMD(do_sneak);
ACMD(do_split);
ACMD(do_steal);
ACMD(do_title);
ACMD(do_visible);


/*****************************************************************************
 * Begin Functions and defines for act.social.c
 ****************************************************************************/
/* Utility Functions */
void free_social_messages(void);
/** @todo free_action should be moved to a utility function module. */
void free_action(struct social_messg *mess);
/** @todo command list functions probably belong in interpreter */
void free_command_list(void);
/** @todo command list functions probably belong in interpreter */
void create_command_list(void);
/* Functions without subcommands */
ACMD(do_action);
ACMD(do_gmote);



/*****************************************************************************
 * Begin Functions and defines for act.wizard.c
 ****************************************************************************/
/* Utility Functions */
/** @todo should probably be moved to a more general file handler module */
void clean_llog_entries(void);
/** @todo This should be moved to a more general utility file */
int script_command_interpreter(struct char_data *ch, char *arg);
room_rnum find_target_room(struct char_data *ch, char *rawroomstr);
void perform_immort_vis(struct char_data *ch);
void snoop_check(struct char_data *ch);
bool change_player_name(struct char_data *ch, struct char_data *vict, char *new_name);
bool AddRecentPlayer(char *chname, char *chhost, bool newplr, bool cpyplr);
/* Functions with subcommands */
/* do_date */
ACMD(do_date);
#define SCMD_DATE   0
#define SCMD_UPTIME 1
/* do_echo */
ACMD(do_echo);
#define SCMD_ECHO   0
#define SCMD_EMOTE  1
/* do_last */
ACMD(do_last);
#define SCMD_LIST_ALL 1
/* do_shutdown */
ACMD(do_shutdown);
#define SCMD_SHUTDOW   0
#define SCMD_SHUTDOWN  1
/* do_wizutil */
ACMD(do_wizutil);
#define SCMD_REROLL   0
#define SCMD_PARDON   1
#define SCMD_NOTITLE  2
#define SCMD_MUTE     3
#define SCMD_FREEZE   4
#define SCMD_THAW     5
#define SCMD_UNAFFECT 6
/* Functions without subcommands */
ACMD(do_advance);
ACMD(do_at);
ACMD(do_checkloadstatus);
ACMD(do_copyover);
ACMD(do_dc);
ACMD(do_changelog);
ACMD(do_file);
ACMD(do_force);
ACMD(do_gecho);
ACMD(do_goto);
ACMD(do_invis);
ACMD(do_links);
ACMD(do_load);
ACMD(do_oset);
ACMD(do_peace);
ACMD(do_plist);
ACMD(do_purge);
ACMD(do_recent);
ACMD(do_restore);
void return_to_char(struct char_data * ch);
ACMD(do_return);
ACMD(do_saveall);
ACMD(do_send);
ACMD(do_set);
ACMD(do_show);
ACMD(do_snoop);
ACMD(do_stat);
ACMD(do_switch);
ACMD(do_teleport);
ACMD(do_trans);
ACMD(do_vnum);
ACMD(do_vstat);
ACMD(do_wizlock);
ACMD(do_wiznet);
ACMD(do_wizupdate);
ACMD(do_zcheck);
ACMD(do_zlock);
ACMD(do_zpurge);
ACMD(do_zreset);
ACMD(do_zunlock);

#endif /* _ACT_H_ */
