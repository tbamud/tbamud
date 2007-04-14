/**************************************************************************
*  File: interpreter.c                                     Part of tbaMUD *
*  Usage: Parse user commands, search for specials, call ACMD functions.  *
*                                                                         *
*  All rights reserved.  See license for complete information.            *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
**************************************************************************/

#define __INTERPRETER_C__

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "comm.h"
#include "interpreter.h"
#include "db.h"
#include "utils.h"
#include "spells.h"
#include "handler.h"
#include "mail.h"
#include "screen.h"
#include "genolc.h"
#include "oasis.h"
#include "improved-edit.h"
#include "dg_scripts.h"
#include "constants.h"

/* external variables */
extern room_rnum r_mortal_start_room;
extern room_rnum r_immort_start_room;
extern room_rnum r_frozen_start_room;
extern const char *class_menu;
extern char *motd;
extern char *imotd;
extern char *background;
extern struct player_index_element *player_table;
extern int top_of_p_table;
extern int circle_restrict;
extern int no_specials;
extern int selfdelete_fastwipe;

/* external functions */
void echo_on(struct descriptor_data *d);
void echo_off(struct descriptor_data *d);
void do_start(struct char_data *ch);
int parse_class(char arg);
int special(struct char_data *ch, int cmd, char *arg);
int isbanned(char *hostname);
int valid_name(char *newname);
void read_aliases(struct char_data *ch);
void delete_aliases(const char *charname);
void remove_player(int pfilepos);

/* local functions */
int perform_dupe_check(struct descriptor_data *d);
struct alias_data *find_alias(struct alias_data *alias_list, char *str);
void free_alias(struct alias_data *a);
void perform_complex_alias(struct txt_q *input_q, char *orig, struct alias_data *a);
int perform_alias(struct descriptor_data *d, char *orig, size_t maxlen);
int reserved_word(char *argument);
int _parse_name(char *arg, char *name);
int enter_player_game (struct descriptor_data *d);
void write_aliases(struct char_data *ch);

/* prototypes for all do_x functions. */
ACMD(do_action);
ACMD(do_advance);
ACMD(do_aedit);
ACMD(do_alias);
ACMD(do_assist);
ACMD(do_astat);
ACMD(do_at);
ACMD(do_attach);
ACMD(do_backstab);
ACMD(do_ban);
ACMD(do_bash);
ACMD(do_cast);
ACMD(do_changelog);
ACMD(do_checkloadstatus);
ACMD(do_commands);
ACMD(do_consider);
ACMD(do_copyover);
ACMD(do_credits);
ACMD(do_date);
ACMD(do_dc);
ACMD(do_detach);
ACMD(do_diagnose);
ACMD(do_dig);
ACMD(do_display);
ACMD(do_drink);
ACMD(do_drop);
ACMD(do_eat);
ACMD(do_echo);
ACMD(do_enter);
ACMD(do_equipment);
ACMD(do_examine);
ACMD(do_exit);
ACMD(do_exits);
ACMD(do_export_zone);
ACMD(do_file);
ACMD(do_flee);
ACMD(do_follow);
ACMD(do_force);
ACMD(do_gecho);
ACMD(do_gen_comm);
ACMD(do_gen_door);
ACMD(do_gen_ps);
ACMD(do_gen_tog);
ACMD(do_gen_write);
ACMD(do_get);
ACMD(do_give);
ACMD(do_gold);
ACMD(do_goto);
ACMD(do_grab);
ACMD(do_group);
ACMD(do_gsay);
ACMD(do_hcontrol);
ACMD(do_help);
ACMD(do_hindex);
ACMD(do_history);
ACMD(do_helpcheck);
ACMD(do_hide);
ACMD(do_hit);
ACMD(do_house);
ACMD(do_inventory);
ACMD(do_invis);
ACMD(do_kick);
ACMD(do_kill);
ACMD(do_last);
ACMD(do_leave);
ACMD(do_levels);
ACMD(do_links);
ACMD(do_load);
ACMD(do_look);
ACMD(do_masound);
ACMD(do_mat);
ACMD(do_mdamage);
ACMD(do_mdoor);
ACMD(do_mecho);
ACMD(do_mechoaround);
ACMD(do_mfollow);
ACMD(do_mforce);
ACMD(do_mgoto);
ACMD(do_mhunt);
ACMD(do_mjunk);
ACMD(do_mkill);
ACMD(do_mload);
/* ACMD(do_move); -- interpreter.h */
ACMD(do_mpurge);
ACMD(do_msend);
ACMD(do_mteleport);
ACMD(do_mremember);
ACMD(do_mforget);
ACMD(do_mtransform);
ACMD(do_mzoneecho);
ACMD(do_mrecho);
ACMD(do_not_here);
ACMD(do_order);
ACMD(do_page);
ACMD(do_peace);
ACMD(do_plist);
ACMD(do_pour);
ACMD(do_practice);
ACMD(do_purge);
ACMD(do_put);
ACMD(do_qcomm);
ACMD(do_quit);
ACMD(do_reboot);
ACMD(do_remove);
ACMD(do_reply);
ACMD(do_report);
ACMD(do_rescue);
ACMD(do_rest);
ACMD(do_restore);
ACMD(do_return);
ACMD(do_room_copy);
ACMD(do_save);
ACMD(do_saveall);
ACMD(do_say);
ACMD(do_score);
ACMD(do_send);
ACMD(do_set);
ACMD(do_show);
ACMD(do_show_save_list);
ACMD(do_shutdown);
ACMD(do_sit);
ACMD(do_skillset);
ACMD(do_sleep);
ACMD(do_sneak);
ACMD(do_snoop);
ACMD(do_spec_comm);
ACMD(do_split);
ACMD(do_stand);
ACMD(do_stat);
ACMD(do_steal);
ACMD(do_switch);
ACMD(do_teleport);
ACMD(do_tell);
ACMD(do_time);
ACMD(do_title);
ACMD(do_tlist);
ACMD(do_toggle);
ACMD(do_track);
ACMD(do_trans);
ACMD(do_tstat);
ACMD(do_unban);
ACMD(do_ungroup);
ACMD(do_use);
ACMD(do_users);
ACMD(do_vdelete);
ACMD(do_visible);
ACMD(do_vnum);
ACMD(do_vstat);
ACMD(do_wake);
ACMD(do_wear);
ACMD(do_weather);
ACMD(do_where);
ACMD(do_who);
ACMD(do_wield);
ACMD(do_wizlock);
ACMD(do_wiznet);
ACMD(do_wizutil);
ACMD(do_write);
ACMD(do_zcheck);
ACMD(do_zreset);
ACMD(do_zpurge);

struct command_info *complete_cmd_info;

/* This is the Master Command List. You can put new commands in, take commands 
 * out, change the order they appear in, etc.  You can adjust the "priority" 
 * of commands simply by changing the order they appear in the command list.
 * (For example, if you want "as" to mean "assist" instead of "ask", just put 
 * "assist" above "ask" in the Master Command List. In general, utility 
 * commands such as "at" should have high priority; infrequently used and 
 * dangerously destructive commands should have low priority. */

cpp_extern const struct command_info cmd_info[] = {
  { "RESERVED", "", 0, 0, 0, 0 }, /* this must be first -- for specprocs */

  /* directions must come before other commands but after RESERVED */
  { "north"    , "n"       , POS_STANDING, do_move     , 0, SCMD_NORTH },
  { "east"     , "e"       , POS_STANDING, do_move     , 0, SCMD_EAST },
  { "south"    , "s"       , POS_STANDING, do_move     , 0, SCMD_SOUTH },
  { "west"     , "w"       , POS_STANDING, do_move     , 0, SCMD_WEST },
  { "up"       , "u"       , POS_STANDING, do_move     , 0, SCMD_UP },
  { "down"     , "d"       , POS_STANDING, do_move     , 0, SCMD_DOWN },

  /* now, the main list */
  { "at"       , "at"      , POS_DEAD    , do_at       , LVL_IMMORT, 0 },
  { "advance"  , "adv"     , POS_DEAD    , do_advance  , LVL_GOD, 0 },
  { "aedit"    , "aed"     , POS_DEAD    , do_oasis_aedit, LVL_GOD, 0 },
  { "alias"    , "ali"     , POS_DEAD    , do_alias    , 0, 0 },
  { "afk"      , "afk"     , POS_DEAD    , do_gen_tog  , 0, SCMD_AFK },
  { "assist"   , "as"      , POS_FIGHTING, do_assist   , 1, 0 },
  { "ask"      , "ask"     , POS_RESTING , do_spec_comm, 0, SCMD_ASK },
  { "astat"    , "ast"     , POS_DEAD    , do_astat    , 0, 0 },
  { "attach"   , "attach"  , POS_DEAD    , do_attach   , LVL_BUILDER, 0 },
  { "auction"  , "auc"     , POS_SLEEPING, do_gen_comm , 0, SCMD_AUCTION },
  { "autoexits" , "autoex"  , POS_DEAD    , do_gen_tog  , 0, SCMD_AUTOEXIT },

  { "backstab" , "ba"      , POS_STANDING, do_backstab , 1, 0 },
  { "ban"      , "ban"     , POS_DEAD    , do_ban      , LVL_GRGOD, 0 },
  { "balance"  , "bal"     , POS_STANDING, do_not_here , 1, 0 },
  { "bash"     , "bas"     , POS_FIGHTING, do_bash     , 1, 0 },
  { "brief"    , "br"      , POS_DEAD    , do_gen_tog  , 0, SCMD_BRIEF },
  { "buildwalk", "buildwalk", POS_STANDING, do_gen_tog,   LVL_BUILDER, SCMD_BUILDWALK },
  { "buy"      , "bu"      , POS_STANDING, do_not_here , 0, 0 },
  { "bug"      , "bug"     , POS_DEAD    , do_gen_write, 0, SCMD_BUG },

  { "cast"     , "c"       , POS_SITTING , do_cast     , 1, 0 },
  { "cedit"    , "cedit"   , POS_DEAD    , do_oasis_cedit, LVL_IMPL, 0 },
  { "changelog", "cha"     , POS_DEAD    , do_changelog, LVL_IMPL, 0 },
  { "check"    , "ch"      , POS_STANDING, do_not_here , 1, 0 },
  { "checkload", "checkl"  , POS_DEAD    , do_checkloadstatus, LVL_GOD, 0 },
  { "close"    , "cl"      , POS_SITTING , do_gen_door , 0, SCMD_CLOSE },
  { "clear"    , "cle"     , POS_DEAD    , do_gen_ps   , 0, SCMD_CLEAR },
  { "cls"      , "cls"     , POS_DEAD    , do_gen_ps   , 0, SCMD_CLEAR },
  { "clsolc"   , "clsolc"  , POS_DEAD    , do_gen_tog  , 0, SCMD_CLS },
  { "consider" , "con"     , POS_RESTING , do_consider , 0, 0 },
  { "commands" , "com"     , POS_DEAD    , do_commands , 0, SCMD_COMMANDS },
  { "compact"  , "comp"    , POS_DEAD    , do_gen_tog  , 0, SCMD_COMPACT },
  { "copyover" , "copyover", POS_DEAD    , do_copyover , LVL_GRGOD, 0 },
  { "credits"  , "cred"    , POS_DEAD    , do_gen_ps   , 0, SCMD_CREDITS },

  { "date"     , "da"      , POS_DEAD    , do_date     , LVL_IMMORT, SCMD_DATE },
  { "dc"       , "dc"      , POS_DEAD    , do_dc       , LVL_GOD, 0 },
  { "deposit"  , "depo"    , POS_STANDING, do_not_here , 1, 0 },
  { "detach"   , "detach"  , POS_DEAD    , do_detach   , LVL_BUILDER, 0 },
  { "diagnose" , "diag"    , POS_RESTING , do_diagnose , 0, 0 },
  { "dig"      , "dig"     , POS_DEAD    , do_dig      , LVL_BUILDER, 0 },
  { "display"  , "disp"    , POS_DEAD    , do_display  , 0, 0 },
  { "donate"   , "don"     , POS_RESTING , do_drop     , 0, SCMD_DONATE },
  { "drink"    , "dri"     , POS_RESTING , do_drink    , 0, SCMD_DRINK },
  { "drop"     , "dro"     , POS_RESTING , do_drop     , 0, SCMD_DROP },

  { "eat"      , "ea"      , POS_RESTING , do_eat      , 0, SCMD_EAT },
  { "echo"     , "ec"      , POS_SLEEPING, do_echo     , LVL_IMMORT, SCMD_ECHO },
  { "emote"    , "em"      , POS_RESTING , do_echo     , 1, SCMD_EMOTE },
  { ":"        , ":"       , POS_RESTING, do_echo      , 1, SCMD_EMOTE },
  { "enter"    , "ent"     , POS_STANDING, do_enter    , 0, 0 },
  { "equipment", "eq"      , POS_SLEEPING, do_equipment, 0, 0 },
  { "exits"    , "ex"      , POS_RESTING , do_exits    , 0, 0 },
  { "examine"  , "exa"     , POS_SITTING , do_examine  , 0, 0 },
  { "export"   , "export"  , POS_DEAD    , do_export_zone, LVL_IMPL, 0 },

  { "force"    , "force"   , POS_SLEEPING, do_force    , LVL_GOD, 0 },
  { "fill"     , "fil"     , POS_STANDING, do_pour     , 0, SCMD_FILL },
  { "file"     , "file"    , POS_SLEEPING, do_file     , LVL_GOD, 0 },
  { "flee"     , "fl"      , POS_FIGHTING, do_flee     , 1, 0 },
  { "follow"   , "fol"     , POS_RESTING , do_follow   , 0, 0 },
  { "freeze"   , "freeze"  , POS_DEAD    , do_wizutil  , LVL_GRGOD, SCMD_FREEZE },

  { "get"      , "g"       , POS_RESTING , do_get      , 0, 0 },
  { "gecho"    , "gecho"   , POS_DEAD    , do_gecho    , LVL_GOD, 0 },
  { "gemote"   , "gem"     , POS_SLEEPING, do_gen_comm , 0, SCMD_GEMOTE },
  { "give"     , "giv"     , POS_RESTING , do_give     , 0, 0 },
  { "goto"     , "go"      , POS_SLEEPING, do_goto     , LVL_IMMORT, 0 },
  { "gold"     , "gol"     , POS_RESTING , do_gold     , 0, 0 },
  { "gossip"   , "gos"     , POS_SLEEPING, do_gen_comm , 0, SCMD_GOSSIP },
  { "group"    , "gr"      , POS_RESTING , do_group    , 1, 0 },
  { "grab"     , "grab"    , POS_RESTING , do_grab     , 0, 0 },
  { "grats"    , "grat"    , POS_SLEEPING, do_gen_comm , 0, SCMD_GRATZ },
  { "gsay"     , "gsay"    , POS_SLEEPING, do_gsay     , 0, 0 },
  { "gtell"    , "gt"      , POS_SLEEPING, do_gsay     , 0, 0 },

  { "help"     , "h"       , POS_DEAD    , do_help     , 0, 0 },
  { "hedit"    , "hedit"   , POS_DEAD    , do_oasis_hedit, LVL_GOD , 0 },
  { "hindex"   , "hind"    , POS_DEAD    , do_hindex   , 0, 0 },
  { "helpcheck", "helpch"  , POS_DEAD    , do_helpcheck, LVL_IMPL, 0 },
  { "hide"     , "hi"      , POS_RESTING , do_hide     , 1, 0 },
  { "handbook" , "handb"   , POS_DEAD    , do_gen_ps   , LVL_IMMORT, SCMD_HANDBOOK },
  { "hcontrol" , "hcontrol", POS_DEAD    , do_hcontrol , LVL_GRGOD, 0 },
  { "history"  , "history" , POS_DEAD    , do_history, 0, 0},
  { "hit"      , "hit"     , POS_FIGHTING, do_hit      , 0, SCMD_HIT },
  { "hold"     , "hold"    , POS_RESTING , do_grab     , 1, 0 },
  { "holler"   , "holler"  , POS_RESTING , do_gen_comm , 1, SCMD_HOLLER },
  { "holylight", "holy"    , POS_DEAD    , do_gen_tog  , LVL_IMMORT, SCMD_HOLYLIGHT },
  { "house"    , "house"   , POS_RESTING , do_house    , 0, 0 },

  { "inventory", "i"       , POS_DEAD    , do_inventory, 0, 0 },
  { "idea"     , "id"      , POS_DEAD    , do_gen_write, 0, SCMD_IDEA },
  { "imotd"    , "imo"     , POS_DEAD    , do_gen_ps   , LVL_IMMORT, SCMD_IMOTD },
  { "immlist"  , "imm"     , POS_DEAD    , do_gen_ps   , 0, SCMD_IMMLIST },
  { "info"     , "info"    , POS_SLEEPING, do_gen_ps   , 0, SCMD_INFO },
  { "invis"    , "invi"    , POS_DEAD    , do_invis    , LVL_IMMORT, 0 },

  { "junk"     , "j"       , POS_RESTING , do_drop     , 0, SCMD_JUNK },

  { "kill"     , "k"       , POS_FIGHTING, do_kill     , 0, 0 },
  { "kick"     , "ki"      , POS_FIGHTING, do_kick     , 1, 0 },

  { "look"     , "l"       , POS_RESTING , do_look     , 0, SCMD_LOOK },
  { "last"     , "last"    , POS_DEAD    , do_last     , LVL_GOD, 0 },
  { "leave"    , "lea"     , POS_STANDING, do_leave    , 0, 0 },
  { "levels"   , "lev"     , POS_DEAD    , do_levels   , 0, 0 },
  { "list"     , "lis"     , POS_STANDING, do_not_here , 0, 0 },
  { "links"    , "lin"     , POS_STANDING, do_links    , LVL_GOD, 0 },
  { "lock"     , "loc"     , POS_SITTING , do_gen_door , 0, SCMD_LOCK },
  { "load"     , "load"     , POS_DEAD    , do_load     , LVL_BUILDER, 0 },

  { "motd"     , "motd"    , POS_DEAD    , do_gen_ps   , 0, SCMD_MOTD },
  { "mail"     , "mail"    , POS_STANDING, do_not_here , 1, 0 },
  { "medit"    , "med"     , POS_DEAD    , do_oasis_medit, LVL_BUILDER, 0 },
  { "mlist"    , "mlist"   , POS_DEAD    , do_oasis_list , LVL_BUILDER, SCMD_OASIS_MLIST },
  { "mute"     , "mute"    , POS_DEAD    , do_wizutil  , LVL_GOD, SCMD_SQUELCH },

  { "news"     , "news"    , POS_SLEEPING, do_gen_ps   , 0, SCMD_NEWS },
  { "noauction", "noauction",POS_DEAD    , do_gen_tog  , 0, SCMD_NOAUCTION },
  { "nogossip" , "nogossip", POS_DEAD    , do_gen_tog  , 0, SCMD_NOGOSSIP },
  { "nograts"  , "nograts" , POS_DEAD    , do_gen_tog  , 0, SCMD_NOGRATZ },
  { "nohassle" , "nohassle", POS_DEAD    , do_gen_tog  , LVL_IMMORT, SCMD_NOHASSLE },
  { "norepeat" , "norepeat", POS_DEAD    , do_gen_tog  , 0, SCMD_NOREPEAT },
  { "noshout"  , "noshout" , POS_SLEEPING, do_gen_tog  , 1, SCMD_NOSHOUT },
  { "nosummon" , "nosummon", POS_DEAD    , do_gen_tog  , 1, SCMD_NOSUMMON },
  { "notell"   , "notell"  , POS_DEAD    , do_gen_tog  , 1, SCMD_NOTELL },
  { "notitle"  , "notitle" , POS_DEAD    , do_wizutil  , LVL_GOD, SCMD_NOTITLE },
  { "nowiz"    , "nowiz"   , POS_DEAD    , do_gen_tog  , LVL_IMMORT, SCMD_NOWIZ },

  { "open"     , "o"       , POS_SITTING , do_gen_door , 0, SCMD_OPEN },
  { "order"    , "ord"     , POS_RESTING , do_order    , 1, 0 },
  { "offer"    , "off"     , POS_STANDING, do_not_here , 1, 0 },
  { "olc"      , "olc"     , POS_DEAD    , do_show_save_list, LVL_BUILDER, 0 },
  { "olist"    , "olist"   , POS_DEAD    , do_oasis_list, LVL_BUILDER, SCMD_OASIS_OLIST },
  { "oedit"    , "oedit"   , POS_DEAD    , do_oasis_oedit, LVL_BUILDER, 0 },

  { "put"      , "p"       , POS_RESTING , do_put      , 0, 0 },
  { "peace"    , "pe"      , POS_DEAD    , do_peace    , LVL_BUILDER, 0 },
  { "pick"     , "pi"      , POS_STANDING, do_gen_door , 1, SCMD_PICK },
  { "practice" , "pr"     , POS_RESTING , do_practice , 1, 0 },
  { "page"     , "pag"     , POS_DEAD    , do_page     , 1, 0 },
  { "pardon"   , "pardon"  , POS_DEAD    , do_wizutil  , LVL_GOD, SCMD_PARDON },
  { "plist"    , "plist"   , POS_DEAD    , do_plist    , LVL_GOD, 0 },
  { "policy"   , "pol"     , POS_DEAD    , do_gen_ps   , 0, SCMD_POLICIES },
  { "pour"     , "pour"    , POS_STANDING, do_pour     , 0, SCMD_POUR },
  { "prompt"   , "pro"     , POS_DEAD    , do_display  , 0, 0 },
  { "purge"    , "purge"   , POS_DEAD    , do_purge    , LVL_BUILDER, 0 },

  { "quaff"    , "qua"     , POS_RESTING , do_use      , 0, SCMD_QUAFF },
  { "qecho"    , "qec"     , POS_DEAD    , do_qcomm    , LVL_GOD, SCMD_QECHO },
  { "quest"    , "que"     , POS_DEAD    , do_gen_tog  , 0, SCMD_QUEST },
  { "qui"      , "qui"     , POS_DEAD    , do_quit     , 0, 0 },
  { "quit"     , "quit"    , POS_DEAD    , do_quit     , 0, SCMD_QUIT },
  { "qsay"     , "qsay"    , POS_RESTING , do_qcomm    , 0, SCMD_QSAY },

  { "reply"    , "r"       , POS_SLEEPING, do_reply    , 0, 0 },
  { "rest"     , "res"     , POS_RESTING , do_rest     , 0, 0 },
  { "read"     , "rea"     , POS_RESTING , do_look     , 0, SCMD_READ },
  { "reload"   , "reload"  , POS_DEAD    , do_reboot   , LVL_IMPL, 0 },
  { "recite"   , "reci"    , POS_RESTING , do_use      , 0, SCMD_RECITE },
  { "receive"  , "rece"    , POS_STANDING, do_not_here , 1, 0 },
  { "remove"   , "rem"     , POS_RESTING , do_remove   , 0, 0 },
  { "rent"     , "rent"    , POS_STANDING, do_not_here , 1, 0 },
  { "report"   , "repo"    , POS_RESTING , do_report   , 0, 0 },
  { "reroll"   , "rero"    , POS_DEAD    , do_wizutil  , LVL_GRGOD, SCMD_REROLL },
  { "rescue"   , "resc"    , POS_FIGHTING, do_rescue   , 1, 0 },
  { "restore"  , "resto"   , POS_DEAD    , do_restore  , LVL_GOD, 0 },
  { "return"   , "retu"    , POS_DEAD    , do_return   , 0, 0 },
  { "redit"    , "redit"   , POS_DEAD    , do_oasis_redit, LVL_BUILDER, 0 },
  { "rlist"    , "rlist"   , POS_DEAD    , do_oasis_list, LVL_BUILDER, SCMD_OASIS_RLIST },
  { "rclone"   , "rclone"  , POS_DEAD    , do_room_copy, LVL_BUILDER, 0 },
  { "roomflags", "roomflags", POS_DEAD   , do_gen_tog  , LVL_IMMORT, SCMD_SHOWVNUMS },

  { "say"      , "s"       , POS_RESTING , do_say      , 0, 0 },
  { "score"    , "sc"      , POS_DEAD    , do_score    , 0, 0 },
  { "sit"      , "si"      , POS_RESTING , do_sit      , 0, 0 },
  { "'"        , "'"       , POS_RESTING , do_say      , 0, 0 },
  { "save"     , "sav"     , POS_SLEEPING, do_save     , 0, 0 },
  { "saveall"  , "saveall" , POS_DEAD    , do_saveall  , LVL_BUILDER, 0},
  { "sell"     , "sell"    , POS_STANDING, do_not_here , 0, 0 },
  { "sedit"    , "sedit"   , POS_DEAD    , do_oasis_sedit, LVL_BUILDER, 0 },
  { "send"     , "send"    , POS_SLEEPING, do_send     , LVL_GOD, 0 },
  { "set"      , "set"     , POS_DEAD    , do_set      , LVL_IMMORT, 0 },
  { "shout"    , "sho"     , POS_RESTING , do_gen_comm , 0, SCMD_SHOUT },
  { "show"     , "show"    , POS_DEAD    , do_show     , LVL_IMMORT, 0 },
  { "shutdow"  , "shutdow" , POS_DEAD    , do_shutdown , LVL_IMPL, 0 },
  { "shutdown" , "shutdown", POS_DEAD    , do_shutdown , LVL_IMPL, SCMD_SHUTDOWN },
  { "sip"      , "sip"     , POS_RESTING , do_drink    , 0, SCMD_SIP },
  { "skillset" , "skillset", POS_SLEEPING, do_skillset , LVL_GRGOD, 0 },
  { "sleep"    , "sl"      , POS_SLEEPING, do_sleep    , 0, 0 },
  { "slist"    , "slist"   , POS_SLEEPING, do_oasis_list, LVL_BUILDER, SCMD_OASIS_SLIST },
  { "sneak"    , "sneak"   , POS_STANDING, do_sneak    , 1, 0 },
  { "snoop"    , "snoop"   , POS_DEAD    , do_snoop    , LVL_GOD, 0 },
  { "socials"  , "socials" , POS_DEAD    , do_commands , 0, SCMD_SOCIALS },
  { "split"    , "split"   , POS_SITTING , do_split    , 1, 0 },
  { "stand"    , "st"      , POS_RESTING , do_stand    , 0, 0 },
  { "stat"     , "stat"    , POS_DEAD    , do_stat     , LVL_IMMORT, 0 },
  { "steal"    , "ste"     , POS_STANDING, do_steal    , 1, 0 },
  { "switch"   , "switch"  , POS_DEAD    , do_switch   , LVL_GOD, 0 },

  { "tell"     , "t"       , POS_DEAD    , do_tell     , 0, 0 },
  { "take"     , "ta"      , POS_RESTING , do_get      , 0, 0 },
  { "taste"    , "tas"     , POS_RESTING , do_eat      , 0, SCMD_TASTE },
  { "teleport" , "tele"    , POS_DEAD    , do_teleport , LVL_GOD, 0 },
  { "tedit"    , "tedit"   , POS_DEAD    , do_tedit    , LVL_GOD, 0 },  /* XXX: Oasisify */
  { "thaw"     , "thaw"    , POS_DEAD    , do_wizutil  , LVL_GRGOD, SCMD_THAW },
  { "title"    , "title"   , POS_DEAD    , do_title    , 0, 0 },
  { "time"     , "time"    , POS_DEAD    , do_time     , 0, 0 },
  { "toggle"   , "toggle"  , POS_DEAD    , do_toggle   , 0, 0 },
  { "track"    , "track"   , POS_STANDING, do_track    , 0, 0 },
  { "transfer" , "transfer", POS_SLEEPING, do_trans    , LVL_GOD, 0 },
  { "trigedit" , "trigedit", POS_DEAD    , do_oasis_trigedit, LVL_BUILDER, 0 },
  { "typo"     , "typo"    , POS_DEAD    , do_gen_write, 0, SCMD_TYPO },
  { "tlist"    , "tlist"   , POS_DEAD    , do_oasis_list, LVL_BUILDER, SCMD_OASIS_TLIST },
  { "tstat"    , "tstat"   , POS_DEAD    , do_tstat    , LVL_BUILDER, 0 },

  { "unlock"   , "unlock"  , POS_SITTING , do_gen_door , 0, SCMD_UNLOCK },
  { "ungroup"  , "ungroup" , POS_DEAD    , do_ungroup  , 0, 0 },
  { "unban"    , "unban"   , POS_DEAD    , do_unban    , LVL_GRGOD, 0 },
  { "unaffect" , "unaffect", POS_DEAD    , do_wizutil  , LVL_GOD, SCMD_UNAFFECT },
  { "uptime"   , "uptime"  , POS_DEAD    , do_date     , LVL_GOD, SCMD_UPTIME },
  { "use"      , "use"     , POS_SITTING , do_use      , 1, SCMD_USE },
  { "users"    , "users"   , POS_DEAD    , do_users    , LVL_GOD, 0 },

  { "value"    , "val"     , POS_STANDING, do_not_here , 0, 0 },
  { "version"  , "ver"     , POS_DEAD    , do_gen_ps   , 0, SCMD_VERSION },
  { "visible"  , "vis"     , POS_RESTING , do_visible  , 1, 0 },
  { "vnum"     , "vnum"    , POS_DEAD    , do_vnum     , LVL_IMMORT, 0 },
  { "vstat"    , "vstat"   , POS_DEAD    , do_vstat    , LVL_IMMORT, 0 },
  { "vdelete"  , "vdelete" , POS_DEAD    , do_vdelete  , LVL_BUILDER, 0 },

  { "wake"     , "wake"    , POS_SLEEPING, do_wake     , 0, 0 },
  { "wear"     , "wea"     , POS_RESTING , do_wear     , 0, 0 },
  { "weather"  , "weather" , POS_RESTING , do_weather  , 0, 0 },
  { "who"      , "wh"      , POS_DEAD    , do_who      , 0, 0 },
  { "whoami"   , "whoami"  , POS_DEAD    , do_gen_ps   , 0, SCMD_WHOAMI },
  { "where"    , "where"   , POS_RESTING , do_where    , 1, 0 },
  { "whisper"  , "whisper" , POS_RESTING , do_spec_comm, 0, SCMD_WHISPER },
  { "wield"    , "wie"     , POS_RESTING , do_wield    , 0, 0 },
  { "withdraw" , "withdraw", POS_STANDING, do_not_here , 1, 0 },
  { "wiznet"   , "wiz"     , POS_DEAD    , do_wiznet   , LVL_IMMORT, 0 },
  { ";"        , ";"       , POS_DEAD    , do_wiznet   , LVL_IMMORT, 0 },
  { "wizhelp"  , "wizhelp" , POS_SLEEPING, do_commands , LVL_IMMORT, SCMD_WIZHELP },
  { "wizlist"  , "wizlist" , POS_DEAD    , do_gen_ps   , 0, SCMD_WIZLIST },
  { "wizlock"  , "wizlock" , POS_DEAD    , do_wizlock  , LVL_IMPL, 0 },
  { "write"    , "write"   , POS_STANDING, do_write    , 1, 0 },

  { "zreset"   , "zreset"  , POS_DEAD    , do_zreset   , LVL_BUILDER, 0 },
  { "zedit"    , "zedit"   , POS_DEAD    , do_oasis_zedit, LVL_BUILDER, 0 },
  { "zlist"    , "zlist"   , POS_DEAD    , do_oasis_list, LVL_BUILDER, SCMD_OASIS_ZLIST },
  { "zcheck"   , "zcheck"  , POS_DEAD    , do_zcheck   , LVL_GOD, 0 },
  { "zpurge"   , "zpurge"  , POS_DEAD    , do_zpurge   , LVL_BUILDER, 0 },

  /* DG trigger commands */
  { "masound"  , "masound" , POS_DEAD    , do_masound  , -1, 0 },
  { "mkill"    , "mkill"   , POS_STANDING, do_mkill    , -1, 0 },
  { "mjunk"    , "mjunk"   , POS_SITTING , do_mjunk    , -1, 0 },
  { "mdamage"  , "mdamage" , POS_DEAD    , do_mdamage  , -1, 0 },
  { "mdoor"    , "mdoor"   , POS_DEAD    , do_mdoor    , -1, 0 },
  { "mecho"    , "mecho"   , POS_DEAD    , do_mecho    , -1, 0 },
  { "mrecho"   , "mrecho"  , POS_DEAD    , do_mrecho   , -1, 0 },
  { "mechoaround", "mechoaround", POS_DEAD, do_mechoaround, -1, 0 },
  { "msend"    , "msend"   , POS_DEAD    , do_msend    , -1, 0 },
  { "mload"    , "mload"   , POS_DEAD    , do_mload    , -1, 0 },
  { "mpurge"   , "mpurge"  , POS_DEAD    , do_mpurge   , -1, 0 },
  { "mgoto"    , "mgoto"   , POS_DEAD    , do_mgoto    , -1, 0 },
  { "mat"      , "mat"     , POS_DEAD    , do_mat      , -1, 0 },
  { "mteleport", "mteleport", POS_DEAD   , do_mteleport, -1, 0 },
  { "mforce"   , "mforce"  , POS_DEAD    , do_mforce   , -1, 0 },
  { "mhunt"    , "mhunt"   , POS_DEAD    , do_mhunt    , -1, 0 },
  { "mremember", "mremember", POS_DEAD   , do_mremember, -1, 0 },
  { "mforget"  , "mforget" , POS_DEAD    , do_mforget  , -1, 0 },
  { "mtransform", "mtransform", POS_DEAD , do_mtransform,-1, 0 },
  { "mzoneecho", "mzoneecho", POS_DEAD   , do_mzoneecho, -1, 0 },
  { "mfollow"  , "mfollow" , POS_DEAD    , do_mfollow  , -1, 0 },

  { "\n", "zzzzzzz", 0, 0, 0, 0 } };	/* this must be last */

const char *fill[] =
{
  "in",
  "from",
  "with",
  "the",
  "on",
  "at",
  "to",
  "\n"
};

const char *reserved[] =
{
  "a",
  "an",
  "self",
  "me",
  "all",
  "room",
  "someone",
  "something",
  "\n"
};

/* This is the actual command interpreter called from game_loop() in comm.c
 * It makes sure you are the proper level and position to execute the command,
 * then calls the appropriate function. */
void command_interpreter(struct char_data *ch, char *argument)
{
  int cmd, length;
  char *line;
  char arg[MAX_INPUT_LENGTH];

  REMOVE_BIT(AFF_FLAGS(ch), AFF_HIDE);

  /* just drop to next line for hitting CR */
  skip_spaces(&argument);
  if (!*argument)
    return;

  /* special case to handle one-character, non-alphanumeric commands; requested
   * by many people so "'hi" or ";godnet test" is possible. Patch sent by Eric 
   * Green and Stefan Wasilewski. */
  if (!isalpha(*argument)) {
    arg[0] = argument[0];
    arg[1] = '\0';
    line = argument + 1;
  } else
    line = any_one_arg(argument, arg);

  /* Since all command triggers check for valid_dg_target before acting, the levelcheck
   * here has been removed. Otherwise, find the command. */
  {
    int cont;                                            /* continue the command checks */
    cont = command_wtrigger(ch, arg, line);              /* any world triggers ? */
    if (!cont) cont = command_mtrigger(ch, arg, line);   /* any mobile triggers ? */
    if (!cont) cont = command_otrigger(ch, arg, line);   /* any object triggers ? */
    if (cont) return;                                    /* yes, command trigger took over */
  }

  for (length = strlen(arg), cmd = 0; *complete_cmd_info[cmd].command != '\n'; cmd++)
    if(complete_cmd_info[cmd].command_pointer != do_action &&
       !strncmp(complete_cmd_info[cmd].command, arg, length))
      if (GET_LEVEL(ch) >= complete_cmd_info[cmd].minimum_level)
        break;

  /* it's not a 'real' command, so it's a social */

  if(*complete_cmd_info[cmd].command == '\n')
    for (length = strlen(arg), cmd = 0; *complete_cmd_info[cmd].command != '\n'; cmd++)
      if (complete_cmd_info[cmd].command_pointer == do_action &&
          !strncmp(complete_cmd_info[cmd].command, arg, length))
        if (GET_LEVEL(ch) >= complete_cmd_info[cmd].minimum_level)
          break;

  if (*complete_cmd_info[cmd].command == '\n') {
    int found = 0;
    send_to_char(ch, "Huh!?!\r\n");

    for (cmd = 0; *cmd_info[cmd].command != '\n'; cmd++) {
      if (*arg != *cmd_info[cmd].command || cmd_info[cmd].minimum_level > GET_LEVEL(ch))
        continue;

      if (levenshtein_distance(arg, (char *) cmd_info[cmd].command) <= 2) {
        if (!found) {   
          send_to_char(ch, "\r\nDid you mean:\r\n");
          found = 1;
        }
        send_to_char(ch, "  %s\r\n", cmd_info[cmd].command);
      }
    }
  }
  else if (!IS_NPC(ch) && PLR_FLAGGED(ch, PLR_FROZEN) && GET_LEVEL(ch) < LVL_IMPL)
    send_to_char(ch, "You try, but the mind-numbing cold prevents you...\r\n");
  else if (complete_cmd_info[cmd].command_pointer == NULL)
    send_to_char(ch, "Sorry, that command hasn't been implemented yet.\r\n");
  else if (IS_NPC(ch) && complete_cmd_info[cmd].minimum_level >= LVL_IMMORT)
    send_to_char(ch, "You can't use immortal commands while switched.\r\n");
  else if (GET_POS(ch) < complete_cmd_info[cmd].minimum_position)
    switch (GET_POS(ch)) {
    case POS_DEAD:
      send_to_char(ch, "Lie still; you are DEAD!!! :-(\r\n");
      break;
    case POS_INCAP:
    case POS_MORTALLYW:
      send_to_char(ch, "You are in a pretty bad shape, unable to do anything!\r\n");
      break;
    case POS_STUNNED:
      send_to_char(ch, "All you can do right now is think about the stars!\r\n");
      break;
    case POS_SLEEPING:
      send_to_char(ch, "In your dreams, or what?\r\n");
      break;
    case POS_RESTING:
      send_to_char(ch, "Nah... You feel too relaxed to do that..\r\n");
      break;
    case POS_SITTING:
      send_to_char(ch, "Maybe you should get on your feet first?\r\n");
      break;
    case POS_FIGHTING:
      send_to_char(ch, "No way!  You're fighting for your life!\r\n");
      break;
  } else if (no_specials || !special(ch, cmd, line))
    ((*complete_cmd_info[cmd].command_pointer) (ch, line, cmd, complete_cmd_info[cmd].subcmd));
}

/* Routines to handle aliasing. */
struct alias_data *find_alias(struct alias_data *alias_list, char *str)
{
  while (alias_list != NULL) {
    if (*str == *alias_list->alias)	/* hey, every little bit counts :-) */
      if (!strcmp(str, alias_list->alias))
	return (alias_list);

    alias_list = alias_list->next;
  }

  return (NULL);
}

void free_alias(struct alias_data *a)
{
  if (a->alias)
    free(a->alias);
  if (a->replacement)
    free(a->replacement);
  free(a);
}

/* The interface to the outside world: do_alias */
ACMD(do_alias)
{
  char arg[MAX_INPUT_LENGTH];
  char *repl;
  struct alias_data *a, *temp;

  if (IS_NPC(ch))
    return;

  repl = any_one_arg(argument, arg);

  if (!*arg) {			/* no argument specified -- list currently defined aliases */
    send_to_char(ch, "Currently defined aliases:\r\n");
    if ((a = GET_ALIASES(ch)) == NULL)
      send_to_char(ch, " None.\r\n");
    else {
      while (a != NULL) {
	send_to_char(ch, "%-15s %s\r\n", a->alias, a->replacement);
	a = a->next;
      }
    }
  } else {			/* otherwise, add or remove aliases */
    /* is this an alias we've already defined? */
    if ((a = find_alias(GET_ALIASES(ch), arg)) != NULL) {
      REMOVE_FROM_LIST(a, GET_ALIASES(ch), next);
      free_alias(a);
    }
    /* if no replacement string is specified, assume we want to delete */
    if (!*repl) {
      if (a == NULL)
	send_to_char(ch, "No such alias.\r\n");
      else
	send_to_char(ch, "Alias deleted.\r\n");
    } else {			/* otherwise, either add or redefine an alias */
      if (!str_cmp(arg, "alias")) {
	send_to_char(ch, "You can't alias 'alias'.\r\n");
	return;
      }
      CREATE(a, struct alias_data, 1);
      a->alias = strdup(arg);
      delete_doubledollar(repl);
      a->replacement = strdup(repl);
      if (strchr(repl, ALIAS_SEP_CHAR) || strchr(repl, ALIAS_VAR_CHAR))
	a->type = ALIAS_COMPLEX;
      else
	a->type = ALIAS_SIMPLE;
      a->next = GET_ALIASES(ch);
      GET_ALIASES(ch) = a;
      save_char(ch);
      send_to_char(ch, "Alias ready.\r\n");
    }
  }
}

/* Valid numeric replacements are only $1 .. $9 (makes parsing a little easier,
 * and it's not that much of a limitation anyway.)  Also valid is "$*", which 
 * stands for the entire original line after the alias. ";" is used to delimit 
 * commands. */
#define NUM_TOKENS       9

void perform_complex_alias(struct txt_q *input_q, char *orig, struct alias_data *a)
{
  struct txt_q temp_queue;
  char *tokens[NUM_TOKENS], *temp, *write_point;
  char buf2[MAX_RAW_INPUT_LENGTH], buf[MAX_RAW_INPUT_LENGTH];	/* raw? */
  int num_of_tokens = 0, num;

  /* First, parse the original string */
  strcpy(buf2, orig);	/* strcpy: OK (orig:MAX_INPUT_LENGTH < buf2:MAX_RAW_INPUT_LENGTH) */
  temp = strtok(buf2, " ");
  while (temp != NULL && num_of_tokens < NUM_TOKENS) {
    tokens[num_of_tokens++] = temp;
    temp = strtok(NULL, " ");
  }

  /* initialize */
  write_point = buf;
  temp_queue.head = temp_queue.tail = NULL;

  /* now parse the alias */
  for (temp = a->replacement; *temp; temp++) {
    if (*temp == ALIAS_SEP_CHAR) {
      *write_point = '\0';
      buf[MAX_INPUT_LENGTH - 1] = '\0';
      write_to_q(buf, &temp_queue, 1);
      write_point = buf;
    } else if (*temp == ALIAS_VAR_CHAR) {
      temp++;
      if ((num = *temp - '1') < num_of_tokens && num >= 0) {
	strcpy(write_point, tokens[num]);	/* strcpy: OK */
	write_point += strlen(tokens[num]);
      } else if (*temp == ALIAS_GLOB_CHAR) {
	strcpy(write_point, orig);		/* strcpy: OK */
	write_point += strlen(orig);
      } else if ((*(write_point++) = *temp) == '$')	/* redouble $ for act safety */
	*(write_point++) = '$';
    } else
      *(write_point++) = *temp;
  }

  *write_point = '\0';
  buf[MAX_INPUT_LENGTH - 1] = '\0';
  write_to_q(buf, &temp_queue, 1);

  /* push our temp_queue on to the _front_ of the input queue */
  if (input_q->head == NULL)
    *input_q = temp_queue;
  else {
    temp_queue.tail->next = input_q->head;
    input_q->head = temp_queue.head;
  }
}

/* Given a character and a string, perform alias replacement on it.
 * Return values:
 *   0: String was modified in place; call command_interpreter immediately.
 *   1: String was _not_ modified in place; rather, the expanded aliases
 *      have been placed at the front of the character's input queue. */
int perform_alias(struct descriptor_data *d, char *orig, size_t maxlen)
{
  char first_arg[MAX_INPUT_LENGTH], *ptr;
  struct alias_data *a, *tmp;

  /* Mobs don't have alaises. */
  if (IS_NPC(d->character))
    return (0);

  /* bail out immediately if the guy doesn't have any aliases */
  if ((tmp = GET_ALIASES(d->character)) == NULL)
    return (0);

  /* find the alias we're supposed to match */
  ptr = any_one_arg(orig, first_arg);

  /* bail out if it's null */
  if (!*first_arg)
    return (0);

  /* if the first arg is not an alias, return without doing anything */
  if ((a = find_alias(tmp, first_arg)) == NULL)
    return (0);

  if (a->type == ALIAS_SIMPLE) {
    strlcpy(orig, a->replacement, maxlen);
    return (0);
  } else {
    perform_complex_alias(&d->input, ptr, a);
    return (1);
  }
}

/* Various other parsing utilities. */

/* Searches an array of strings for a target string.  "exact" can be 0 or non-0,
 * depending on whether or not the match must be exact for it to be returned.  
 * Returns -1 if not found; 0..n otherwise.  Array must be terminated with a 
 * '\n' so it knows to stop searching. */
int search_block(char *arg, const char **list, int exact)
{
  int i, l;

  /*  We used to have \r as the first character on certain array items to
   *  prevent the explicit choice of that point.  It seems a bit silly to
   *  dump control characters into arrays to prevent that, so we'll just
   *  check in here to see if the first character of the argument is '!',
   *  and if so, just blindly return a '-1' for not found. - ae. */
  if (*arg == '!')
    return (-1);

  /* Make into lower case, and get length of string */
  for (l = 0; *(arg + l); l++)
    *(arg + l) = LOWER(*(arg + l));

  if (exact) {
    for (i = 0; **(list + i) != '\n'; i++)
      if (!strcmp(arg, *(list + i)))
	return (i);
  } else {
    if (!l)
      l = 1;			/* Avoid "" to match the first available
				 * string */
    for (i = 0; **(list + i) != '\n'; i++)
      if (!strncmp(arg, *(list + i), l))
	return (i);
  }

  return (-1);
}

int is_number(const char *str)
{
  if(*str == '-')
     str++;
  if(!*str)
     return (0);
  while (*str)
    if (!isdigit(*(str++)))
      return (0);

  return (1);
}

/* Function to skip over the leading spaces of a string. */
void skip_spaces(char **string)
{
  for (; **string && isspace(**string); (*string)++);
}

/* Given a string, change all instances of double dollar signs ($$) to single 
 * dollar signs ($).  When strings come in, all $'s are changed to $$'s to 
 * avoid having users be able to crash the system if the inputted string is 
 * eventually sent to act().  If you are using user input to produce screen 
 * output AND YOU ARE SURE IT WILL NOT BE SENT THROUGH THE act() FUNCTION 
 * (i.e., do_gecho, do_title, but NOT do_say), you can call 
 * delete_doubledollar() to make the output look correct. 
 * Modifies the string in-place. */
char *delete_doubledollar(char *string)
{
  char *ddread, *ddwrite;

  /* If the string has no dollar signs, return immediately */
  if ((ddwrite = strchr(string, '$')) == NULL)
    return (string);

  /* Start from the location of the first dollar sign */
  ddread = ddwrite;


  while (*ddread)   /* Until we reach the end of the string... */
    if ((*(ddwrite++) = *(ddread++)) == '$') /* copy one char */
      if (*ddread == '$')
	ddread++; /* skip if we saw 2 $'s in a row */

  *ddwrite = '\0';

  return (string);
}

int fill_word(char *argument)
{
  return (search_block(argument, fill, TRUE) >= 0);
}

int reserved_word(char *argument)
{
  return (search_block(argument, reserved, TRUE) >= 0);
}

/* Copy the first non-fill-word, space-delimited argument of 'argument'
 * to 'first_arg'; return a pointer to the remainder of the string. */
char *one_argument(char *argument, char *first_arg)
{
  char *begin = first_arg;

  if (!argument) {
    log("SYSERR: one_argument received a NULL pointer!");
    *first_arg = '\0';
    return (NULL);
  }

  do {
    skip_spaces(&argument);

    first_arg = begin;
    while (*argument && !isspace(*argument)) {
      *(first_arg++) = LOWER(*argument);
      argument++;
    }

    *first_arg = '\0';
  } while (fill_word(begin));

  return (argument);
}

/* one_word is like any_one_arg, except that words in quotes ("") are 
 * considered one word. No longer ignores fill words.  -dak */
char *one_word(char *argument, char *first_arg)
{
    skip_spaces(&argument);

    if (*argument == '\"') {
      argument++;
      while (*argument && *argument != '\"') {
        *(first_arg++) = LOWER(*argument);
        argument++;
      }
      argument++;
    } else {
      while (*argument && !isspace(*argument)) {
        *(first_arg++) = LOWER(*argument);
        argument++;
      }
    }

    *first_arg = '\0';
  return (argument);
}

/* Same as one_argument except that it doesn't ignore fill words. */
char *any_one_arg(char *argument, char *first_arg)
{
  skip_spaces(&argument);

  while (*argument && !isspace(*argument)) {
    *(first_arg++) = LOWER(*argument);
    argument++;
  }

  *first_arg = '\0';

  return (argument);
}

/* Same as one_argument except that it takes two args and returns the rest;
 * ignores fill words */
char *two_arguments(char *argument, char *first_arg, char *second_arg)
{
  return (one_argument(one_argument(argument, first_arg), second_arg)); /* :-) */
}

/* Determine if a given string is an abbreviation of another.
 * Returns 1 if arg1 is an abbreviation of arg2. */
int is_abbrev(const char *arg1, const char *arg2)
{
  if (!*arg1)
    return (0);

  for (; *arg1 && *arg2; arg1++, arg2++)
    if (LOWER(*arg1) != LOWER(*arg2))
      return (0);

  if (!*arg1)
    return (1);
  else
    return (0);
}

/* Return first space-delimited token in arg1; remainder of string in arg2.
 * NOTE: Requires sizeof(arg2) >= sizeof(string) */
void half_chop(char *string, char *arg1, char *arg2)
{
  char *temp;

  temp = any_one_arg(string, arg1);
  skip_spaces(&temp);
  strcpy(arg2, temp);	/* strcpy: OK (documentation) */
}

/* Used in specprocs, mostly.  (Exactly) matches "command" to cmd number */
int find_command(const char *command)
{
  int cmd;

  for (cmd = 0; *complete_cmd_info[cmd].command != '\n'; cmd++)
    if (!strcmp(complete_cmd_info[cmd].command, command))
      return (cmd);

  return (-1);
}

int special(struct char_data *ch, int cmd, char *arg)
{
  struct obj_data *i;
  struct char_data *k;
  int j;

  /* special in room? */
  if (GET_ROOM_SPEC(IN_ROOM(ch)) != NULL)
    if (GET_ROOM_SPEC(IN_ROOM(ch)) (ch, world + IN_ROOM(ch), cmd, arg))
      return (1);

  /* special in equipment list? */
  for (j = 0; j < NUM_WEARS; j++)
    if (GET_EQ(ch, j) && GET_OBJ_SPEC(GET_EQ(ch, j)) != NULL)
      if (GET_OBJ_SPEC(GET_EQ(ch, j)) (ch, GET_EQ(ch, j), cmd, arg))
	return (1);

  /* special in inventory? */
  for (i = ch->carrying; i; i = i->next_content)
    if (GET_OBJ_SPEC(i) != NULL)
      if (GET_OBJ_SPEC(i) (ch, i, cmd, arg))
	return (1);

  /* special in mobile present? */
  for (k = world[IN_ROOM(ch)].people; k; k = k->next_in_room)
    if (!MOB_FLAGGED(k, MOB_NOTDEADYET))
      if (GET_MOB_SPEC(k) && GET_MOB_SPEC(k) (ch, k, cmd, arg))
	return (1);

  /* special in object present? */
  for (i = world[IN_ROOM(ch)].contents; i; i = i->next_content)
    if (GET_OBJ_SPEC(i) != NULL)
      if (GET_OBJ_SPEC(i) (ch, i, cmd, arg))
	return (1);

  return (0);
}

/* Stuff for controlling the non-playing sockets (get name, pwd etc).
 * This function needs to die. */
int _parse_name(char *arg, char *name)
{
  int i;

  skip_spaces(&arg);
  for (i = 0; (*name = *arg); arg++, i++, name++)
    if (!isalpha(*arg))
      return (1);

  if (!i)
    return (1);

  return (0);
}

#define RECON		1
#define USURP		2
#define UNSWITCH	3

/* This function seems a bit over-extended. */
int perform_dupe_check(struct descriptor_data *d)
{
  struct descriptor_data *k, *next_k;
  struct char_data *target = NULL, *ch, *next_ch;
  int mode = 0;
  int pref_temp = 0; /* for "last" log */
  int id = GET_IDNUM(d->character);

  /* Now that this descriptor has successfully logged in, disconnect all
   * other descriptors controlling a character with the same ID number. */

  for (k = descriptor_list; k; k = next_k) {
    next_k = k->next;

    if (k == d)
      continue;

    if (k->original && (GET_IDNUM(k->original) == id)) {
      /* Original descriptor was switched, booting it and restoring normal body control. */

      write_to_output(d, "\r\nMultiple login detected -- disconnecting.\r\n");
      STATE(k) = CON_CLOSE;
      pref_temp=GET_PREF(k->character);
      if (!target) {
	target = k->original;
	mode = UNSWITCH;
      }
      if (k->character)
	k->character->desc = NULL;
      k->character = NULL;
      k->original = NULL;
    } else if (k->character && GET_IDNUM(k->character) == id && k->original) {
      /* Character taking over their own body, while an immortal was switched to it. */

      do_return(k->character, NULL, 0, 0);
    } else if (k->character && GET_IDNUM(k->character) == id) {
      /* Character taking over their own body. */
      pref_temp=GET_PREF(k->character);

      if (!target && STATE(k) == CON_PLAYING) {
	write_to_output(k, "\r\nThis body has been usurped!\r\n");
	target = k->character;
	mode = USURP;
      }
      k->character->desc = NULL;
      k->character = NULL;
      k->original = NULL;
      write_to_output(k, "\r\nMultiple login detected -- disconnecting.\r\n");
      STATE(k) = CON_CLOSE;
    }
  }

 /* Now, go through the character list, deleting all characters that are not 
  * already marked for deletion from the above step (i.e., in the CON_HANGUP 
  * state), and have not already been selected as a target for switching into.
  * In addition, if we haven't already found a target, choose one if one is 
  * available (while still deleting the other duplicates, though theoretically 
  * none should be able to exist). */
  for (ch = character_list; ch; ch = next_ch) {
    next_ch = ch->next;

    if (IS_NPC(ch))
      continue;
    if (GET_IDNUM(ch) != id)
      continue;

    /* ignore chars with descriptors (already handled by above step) */
    if (ch->desc)
      continue;

    /* don't extract the target char we've found one already */
    if (ch == target)
      continue;

    /* we don't already have a target and found a candidate for switching */
    if (!target) {
      target = ch;
      mode = RECON;
      pref_temp = GET_PREF(ch);
      continue;
    }

    /* we've found a duplicate - blow him away, dumping his eq in limbo. */
    if (IN_ROOM(ch) != NOWHERE)
      char_from_room(ch);
    char_to_room(ch, 1);
    extract_char(ch);
  }

  /* no target for switching into was found - allow login to continue */
  if (!target) {
    GET_PREF(d->character)= rand_number(1, 128000);
    GET_HOST(d->character)= strdup(d->host);
    return 0;
  }

  if (GET_HOST(target)) free(GET_HOST(target));
  GET_HOST(target) = strdup(d->host);

  GET_PREF(target) = pref_temp;
  add_llog_entry(target, LAST_RECONNECT);

  /* Okay, we've found a target.  Connect d to target. */
  free_char(d->character); /* get rid of the old char */
  d->character = target;
  d->character->desc = d;
  d->original = NULL;
  d->character->char_specials.timer = 0;
  REMOVE_BIT(PLR_FLAGS(d->character), PLR_MAILING | PLR_WRITING);
  REMOVE_BIT(AFF_FLAGS(d->character), AFF_GROUP);
  STATE(d) = CON_PLAYING;

  switch (mode) {
  case RECON:
    write_to_output(d, "Reconnecting.\r\n");
    act("$n has reconnected.", TRUE, d->character, 0, 0, TO_ROOM);

    if (GET_INVIS_LEV(d->character))
      mudlog(BRF, MAX(LVL_IMMORT, GET_INVIS_LEV(d->character)), TRUE, "%s has reconnected. (invis %d)", GET_NAME(d->character), GET_INVIS_LEV(d->character));
    else
      mudlog(BRF, LVL_IMMORT, TRUE, "%s has reconnected.", GET_NAME(d->character));

    if (has_mail(GET_IDNUM(d->character)))
      write_to_output(d, "You have mail waiting.\r\n");
    break;
  case USURP:
    write_to_output(d, "You take over your own body, already in use!\r\n");
    act("$n suddenly keels over in pain, surrounded by a white aura...\r\n"
	"$n's body has been taken over by a new spirit!",
	TRUE, d->character, 0, 0, TO_ROOM);
    mudlog(NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(d->character)), TRUE,
	"%s has re-logged in ... disconnecting old socket.", GET_NAME(d->character));
    break;
  case UNSWITCH:
    write_to_output(d, "Reconnecting to unswitched char.");
    mudlog(NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(d->character)), TRUE, "%s [%s] has reconnected.", GET_NAME(d->character), d->host);
    break;
  }

  return (1);
}

/* load the player, put them in the right room - used by copyover_recover too */
int enter_player_game (struct descriptor_data *d)
{

    int load_result;
    room_vnum load_room;

      reset_char(d->character);
      /* See if there might be some aliases in the old alias file. Only do this 
       * if there were no aliases in the pfile. */
		if (GET_ALIASES(d->character) == NULL)
		{
      read_aliases(d->character);
      /* delete the old file - player will be saved in a second. */
      delete_aliases(GET_NAME(d->character));
    }

      if (PLR_FLAGGED(d->character, PLR_INVSTART))
	GET_INVIS_LEV(d->character) = GET_LEVEL(d->character);

      /* We have to place the character in a room before equipping them
       * or equip_char() will gripe about the person in NOWHERE. */
      if ((load_room = GET_LOADROOM(d->character)) != NOWHERE)
	load_room = real_room(load_room);

      /* If char was saved with NOWHERE, or real_room above failed... */
      if (load_room == NOWHERE) {
	if (GET_LEVEL(d->character) >= LVL_IMMORT)
	  load_room = r_immort_start_room;
	else
	  load_room = r_mortal_start_room;
      }

      if (PLR_FLAGGED(d->character, PLR_FROZEN))
	load_room = r_frozen_start_room;

      /* copyover */
      GET_ID(d->character) = GET_IDNUM(d->character);
      /* find_char helper */
      add_to_lookup_table(GET_ID(d->character), (void *)d->character);

      /* After moving saving of variables to the player file, this should only 
       * be called in case nothing was found in the pfile. If something was 
       * found, SCRIPT(ch) will be set. */
      if (!SCRIPT(d->character))
        read_saved_vars(d->character);

      d->character->next = character_list;
      character_list = d->character;
      char_to_room(d->character, load_room);
      load_result = Crash_load(d->character);
      save_char(d->character);

    return load_result;
}

/* deal with newcomers and other non-playing sockets */
void nanny(struct descriptor_data *d, char *arg)
{
  int load_result;	/* Overloaded variable */
  int player_i;

  /* OasisOLC states */
  struct {
    int state;
    void (*func)(struct descriptor_data *, char *);
  } olc_functions[] = {
    { CON_OEDIT, oedit_parse },
    { CON_ZEDIT, zedit_parse },
    { CON_SEDIT, sedit_parse },
    { CON_MEDIT, medit_parse },
    { CON_REDIT, redit_parse },
    { CON_CEDIT, cedit_parse },
    { CON_TRIGEDIT, trigedit_parse },
    { CON_AEDIT, aedit_parse },
    { CON_HEDIT,    hedit_parse },
    { -1, NULL }
  };

  skip_spaces(&arg);

  /* Quick check for the OLC states. */
  for (player_i = 0; olc_functions[player_i].state >= 0; player_i++)
    if (STATE(d) == olc_functions[player_i].state) {
      /* send context-sensitive help if need be */
      if (context_help(d, arg)) return;

      (*olc_functions[player_i].func)(d, arg);
      return;
    }

  /* Not in OLC. */
  switch (STATE(d)) {
  case CON_GET_NAME:		/* wait for input of name */
    if (d->character == NULL) {
      CREATE(d->character, struct char_data, 1);
      clear_char(d->character);
      CREATE(d->character->player_specials, struct player_special_data, 1);
		  GET_HOST(d->character) = strdup(d->host);
      d->character->desc = d;
    }
    if (!*arg)
      STATE(d) = CON_CLOSE;
    else {
      char buf[MAX_INPUT_LENGTH], tmp_name[MAX_INPUT_LENGTH];

      if ((_parse_name(arg, tmp_name)) || strlen(tmp_name) < 2 ||
	  strlen(tmp_name) > MAX_NAME_LENGTH || !valid_name(tmp_name) ||
	  fill_word(strcpy(buf, tmp_name)) || reserved_word(buf)) {	/* strcpy: OK (mutual MAX_INPUT_LENGTH) */
	write_to_output(d, "Invalid name, please try another.\r\nName: ");
	return;
      }
      if ((player_i = load_char(tmp_name, d->character)) > -1) {
	GET_PFILEPOS(d->character) = player_i;

	if (PLR_FLAGGED(d->character, PLR_DELETED)) {
          /* Make sure old files are removed so the new player doesn't get the 
	   * deleted player's equipment. */
	   if ((player_i = get_ptable_by_name(tmp_name)) >= 0)
	   remove_player(player_i);

          /* We get a false positive from the original deleted character. */
	  free_char(d->character);
	  /* Check for multiple creations. */
	  if (!valid_name(tmp_name)) {
	    write_to_output(d, "Invalid name, please try another.\r\nName: ");
	    return;
	  }
	  CREATE(d->character, struct char_data, 1);
	  clear_char(d->character);
	  CREATE(d->character->player_specials, struct player_special_data, 1);
	  GET_HOST(d->character) = strdup(d->host);

	  d->character->desc = d;
	  CREATE(d->character->player.name, char, strlen(tmp_name) + 1);
	  strcpy(d->character->player.name, CAP(tmp_name));	/* strcpy: OK (size checked above) */
	  GET_PFILEPOS(d->character) = player_i;
	  write_to_output(d, "Did I get that right, %s (Y/N)? ", tmp_name);
	  STATE(d) = CON_NAME_CNFRM;
	} else {
	  /* undo it just in case they are set */
	  REMOVE_BIT(PLR_FLAGS(d->character),
		     PLR_WRITING | PLR_MAILING | PLR_CRYO);
	  REMOVE_BIT(AFF_FLAGS(d->character), AFF_GROUP);
          d->character->player.time.logon = time(0);
	  write_to_output(d, "Password: ");
	  echo_off(d);
	  d->idle_tics = 0;
	  STATE(d) = CON_PASSWORD;
	}
      } else {
	/* player unknown -- make new character */

	/* Check for multiple creations of a character. */
	if (!valid_name(tmp_name)) {
	  write_to_output(d, "Invalid name, please try another.\r\nName: ");
	  return;
	}
	CREATE(d->character->player.name, char, strlen(tmp_name) + 1);
	strcpy(d->character->player.name, CAP(tmp_name));	/* strcpy: OK (size checked above) */

	write_to_output(d, "Did I get that right, %s (Y/N)? ", tmp_name);
	STATE(d) = CON_NAME_CNFRM;
      }
    }
    break;

  case CON_NAME_CNFRM:		/* wait for conf. of new name    */
    if (UPPER(*arg) == 'Y') {
      if (isbanned(d->host) >= BAN_NEW) {
	mudlog(NRM, LVL_GOD, TRUE, "Request for new char %s denied from [%s] (siteban)", GET_PC_NAME(d->character), d->host);
	write_to_output(d, "Sorry, new characters are not allowed from your site!\r\n");
	STATE(d) = CON_CLOSE;
	return;
      }
      if (circle_restrict) {
	write_to_output(d, "Sorry, new players can't be created at the moment.\r\n");
	mudlog(NRM, LVL_GOD, TRUE, "Request for new char %s denied from [%s] (wizlock)", GET_PC_NAME(d->character), d->host);
	STATE(d) = CON_CLOSE;
	return;
      }
      write_to_output(d, "New character.\r\nGive me a password for %s: ", GET_PC_NAME(d->character));
      echo_off(d);
      STATE(d) = CON_NEWPASSWD;
    } else if (*arg == 'n' || *arg == 'N') {
      write_to_output(d, "Okay, what IS it, then? ");
      free(d->character->player.name);
      d->character->player.name = NULL;
      STATE(d) = CON_GET_NAME;
    } else
      write_to_output(d, "Please type Yes or No: ");
    break;

  case CON_PASSWORD:		/* get pwd for known player      */
    /* To really prevent duping correctly, the player's record should be reloaded 
     * from disk at this point (after the password has been typed).  However I'm 
     * afraid that trying to load a character over an already loaded character is 
     * going to cause some problem down the road that I can't see at the moment.  
     * So to compensate, I'm going to (1) add a 15 or 20-second time limit for 
     * entering a password, and (2) re-add the code to cut off duplicates when a 
     * player quits.  JE 6 Feb 96 */

    echo_on(d);    /* turn echo back on */

    /* New echo_on() eats the return on telnet. Extra space better than none. */
    write_to_output(d, "\r\n");

    if (!*arg)
      STATE(d) = CON_CLOSE;
    else {
      if (strncmp(CRYPT(arg, GET_PASSWD(d->character)), GET_PASSWD(d->character), MAX_PWD_LENGTH)) {
	mudlog(BRF, LVL_GOD, TRUE, "Bad PW: %s [%s]", GET_NAME(d->character), d->host);
	GET_BAD_PWS(d->character)++;
	save_char(d->character);
	if (++(d->bad_pws) >= CONFIG_MAX_BAD_PWS) {	/* 3 strikes and you're out. */
	  write_to_output(d, "Wrong password... disconnecting.\r\n");
	  STATE(d) = CON_CLOSE;
	} else {
	  write_to_output(d, "Wrong password.\r\nPassword: ");
	  echo_off(d);
	}
	return;
      }

      /* Password was correct. */
      load_result = GET_BAD_PWS(d->character);
      GET_BAD_PWS(d->character) = 0;
      d->bad_pws = 0;

      if (isbanned(d->host) == BAN_SELECT &&
	  !PLR_FLAGGED(d->character, PLR_SITEOK)) {
	write_to_output(d, "Sorry, this char has not been cleared for login from your site!\r\n");
	STATE(d) = CON_CLOSE;
	mudlog(NRM, LVL_GOD, TRUE, "Connection attempt for %s denied from %s", GET_NAME(d->character), d->host);
	return;
      }
      if (GET_LEVEL(d->character) < circle_restrict) {
	write_to_output(d, "The game is temporarily restricted.. try again later.\r\n");
	STATE(d) = CON_CLOSE;
	mudlog(NRM, LVL_GOD, TRUE, "Request for login denied for %s [%s] (wizlock)", GET_NAME(d->character), d->host);
	return;
      }
      /* check and make sure no other copies of this player are logged in */
      if (perform_dupe_check(d))
	return;

      if (GET_LEVEL(d->character) >= LVL_IMMORT)
	write_to_output(d, "%s", imotd);
      else
	write_to_output(d, "%s", motd);

      if (GET_INVIS_LEV(d->character))
        mudlog(BRF, MAX(LVL_IMMORT, GET_INVIS_LEV(d->character)), TRUE, "%s has connected. (invis %d)", GET_NAME(d->character), GET_INVIS_LEV(d->character));
      else
        mudlog(BRF, LVL_IMMORT, TRUE, "%s has connected.", GET_NAME(d->character));

      if (load_result) {
        write_to_output(d, "\r\n\r\n\007\007\007"
		"%s%d LOGIN FAILURE%s SINCE LAST SUCCESSFUL LOGIN.%s\r\n",
		CCRED(d->character, C_SPR), load_result,
		(load_result > 1) ? "S" : "", CCNRM(d->character, C_SPR));
	GET_BAD_PWS(d->character) = 0;
      }
      write_to_output(d, "\r\n*** PRESS RETURN: ");
      STATE(d) = CON_RMOTD;
    }
    break;

  case CON_NEWPASSWD:
  case CON_CHPWD_GETNEW:
    if (!*arg || strlen(arg) > MAX_PWD_LENGTH || strlen(arg) < 3 ||
	!str_cmp(arg, GET_PC_NAME(d->character))) {
      write_to_output(d, "\r\nIllegal password.\r\nPassword: ");
      return;
    }
    strncpy(GET_PASSWD(d->character), CRYPT(arg, GET_PC_NAME(d->character)), MAX_PWD_LENGTH);	/* strncpy: OK (G_P:MAX_PWD_LENGTH+1) */
    *(GET_PASSWD(d->character) + MAX_PWD_LENGTH) = '\0';

    write_to_output(d, "\r\nPlease retype password: ");
    if (STATE(d) == CON_NEWPASSWD)
      STATE(d) = CON_CNFPASSWD;
    else
      STATE(d) = CON_CHPWD_VRFY;
    break;

  case CON_CNFPASSWD:
  case CON_CHPWD_VRFY:
    if (strncmp(CRYPT(arg, GET_PASSWD(d->character)), GET_PASSWD(d->character),
		MAX_PWD_LENGTH)) {
      write_to_output(d, "\r\nPasswords don't match... start over.\r\nPassword: ");
      if (STATE(d) == CON_CNFPASSWD)
	STATE(d) = CON_NEWPASSWD;
      else
	STATE(d) = CON_CHPWD_GETNEW;
      return;
    }
    echo_on(d);

    if (STATE(d) == CON_CNFPASSWD) {
      write_to_output(d, "\r\nWhat is your sex (M/F)? ");
      STATE(d) = CON_QSEX;
    } else {
      save_char(d->character);
      write_to_output(d, "\r\nDone.\r\n%s", CONFIG_MENU);
      STATE(d) = CON_MENU;
    }
    break;

  case CON_QSEX:		/* query sex of new user         */
    switch (*arg) {
    case 'm':
    case 'M':
      d->character->player.sex = SEX_MALE;
      break;
    case 'f':
    case 'F':
      d->character->player.sex = SEX_FEMALE;
      break;
    default:
      write_to_output(d, "That is not a sex..\r\n"
		"What IS your sex? ");
      return;
    }

    write_to_output(d, "%s\r\nClass: ", class_menu);
    STATE(d) = CON_QCLASS;
    break;

  case CON_QCLASS:
    load_result = parse_class(*arg);
    if (load_result == CLASS_UNDEFINED) {
      write_to_output(d, "\r\nThat's not a class.\r\nClass: ");
      return;
    } else
      GET_CLASS(d->character) = load_result;

      if (d->olc) {
        free(d->olc);
        d->olc = NULL;
      }
      if (GET_PFILEPOS(d->character) < 0)
      GET_PFILEPOS(d->character) = create_entry(GET_PC_NAME(d->character));
    /* Now GET_NAME() will work properly. */
    init_char(d->character);
    save_char(d->character);
    save_player_index();
    write_to_output(d, "%s\r\n*** PRESS RETURN: ", motd);
    STATE(d) = CON_RMOTD;
    /* make sure the last log is updated correctly. */
    GET_PREF(d->character)= rand_number(1, 128000);
    GET_HOST(d->character)= strdup(d->host);

    mudlog(NRM, LVL_IMMORT, TRUE, "%s [%s] new player.", GET_NAME(d->character), d->host);
    break;

  case CON_RMOTD:		/* read CR after printing motd   */
    write_to_output(d, "%s", CONFIG_MENU);
    add_llog_entry(d->character, LAST_CONNECT);
    STATE(d) = CON_MENU;
    break;

  case CON_MENU: {		/* get selection from main menu  */

    switch (*arg) {
    case '0':
      write_to_output(d, "Goodbye.\r\n");
      add_llog_entry(d->character, LAST_QUIT);
      STATE(d) = CON_CLOSE;
      break;

    case '1':
      load_result = enter_player_game(d);
      send_to_char(d->character, "%s", CONFIG_WELC_MESSG);

      /* Clear their load room if it's not persistant. */
      if (!PLR_FLAGGED(d->character, PLR_LOADROOM))
        GET_LOADROOM(d->character) = NOWHERE;
      save_char(d->character);

      greet_mtrigger(d->character, -1);
      greet_memory_mtrigger(d->character);

      act("$n has entered the game.", TRUE, d->character, 0, 0, TO_ROOM);

      STATE(d) = CON_PLAYING;
      if (GET_LEVEL(d->character) == 0) {
	do_start(d->character);
	send_to_char(d->character, "%s", CONFIG_START_MESSG);
      }
      look_at_room(d->character, 0);
      if (has_mail(GET_IDNUM(d->character)))
	send_to_char(d->character, "You have mail waiting.\r\n");
      if (load_result == 2) {	/* rented items lost */
	send_to_char(d->character, "\r\n\007You could not afford your rent!\r\n"
		"Your possesions have been donated to the Salvation Army!\r\n");
      }
      d->has_prompt = 0;
      /* We've updated to 3.1 - some bits might be set wrongly: */
      REMOVE_BIT(PRF_FLAGS(d->character), PRF_BUILDWALK);
      break;

    case '2':
      if (d->character->player.description) {
	write_to_output(d, "Current description:\r\n%s", d->character->player.description);
	/* Don't free this now... so that the old description gets loaded as the 
	 * current buffer in the editor.  Do setup the ABORT buffer here, however. */
	d->backstr = strdup(d->character->player.description);
      }
      write_to_output(d, "Enter the new text you'd like others to see when they look at you.\r\n");
      send_editor_help(d);
      d->str = &d->character->player.description;
      d->max_str = PLR_DESC_LENGTH;
      STATE(d) = CON_PLR_DESC;
      break;

    case '3':
      page_string(d, background, 0);
      STATE(d) = CON_RMOTD;
      break;

    case '4':
      write_to_output(d, "\r\nEnter your old password: ");
      echo_off(d);
      STATE(d) = CON_CHPWD_GETOLD;
      break;

    case '5':
      write_to_output(d, "\r\nEnter your password for verification: ");
      echo_off(d);
      STATE(d) = CON_DELCNF1;
      break;

    default:
      write_to_output(d, "\r\nThat's not a menu choice!\r\n%s", CONFIG_MENU);
      break;
    }
    break;
  }

  case CON_CHPWD_GETOLD:
    if (strncmp(CRYPT(arg, GET_PASSWD(d->character)), GET_PASSWD(d->character), MAX_PWD_LENGTH)) {
      echo_on(d);
      write_to_output(d, "\r\nIncorrect password.\r\n%s", CONFIG_MENU);
      STATE(d) = CON_MENU;
    } else {
      write_to_output(d, "\r\nEnter a new password: ");
      STATE(d) = CON_CHPWD_GETNEW;
    }
    return;

  case CON_DELCNF1:
    echo_on(d);
    if (strncmp(CRYPT(arg, GET_PASSWD(d->character)), GET_PASSWD(d->character), MAX_PWD_LENGTH)) {
      write_to_output(d, "\r\nIncorrect password.\r\n%s", CONFIG_MENU);
      STATE(d) = CON_MENU;
    } else {
      write_to_output(d, "\r\nYOU ARE ABOUT TO DELETE THIS CHARACTER PERMANENTLY.\r\n"
		"ARE YOU ABSOLUTELY SURE?\r\n\r\n"
		"Please type \"yes\" to confirm: ");
      STATE(d) = CON_DELCNF2;
    }
    break;

  case CON_DELCNF2:
    if (!strcmp(arg, "yes") || !strcmp(arg, "YES")) {
      if (PLR_FLAGGED(d->character, PLR_FROZEN)) {
	write_to_output(d, "You try to kill yourself, but the ice stops you.\r\n"
		"Character not deleted.\r\n\r\n");
	STATE(d) = CON_CLOSE;
	return;
      }
      if (GET_LEVEL(d->character) < LVL_GRGOD)
	SET_BIT(PLR_FLAGS(d->character), PLR_DELETED);
      save_char(d->character);
      Crash_delete_file(GET_NAME(d->character));
      /* If the selfdelete_fastwipe flag is set (in config.c), remove all the 
       * player's immediately. */
      if (selfdelete_fastwipe)
        if ((player_i = get_ptable_by_name(GET_NAME(d->character))) >= 0) {
          SET_BIT(player_table[player_i].flags, PINDEX_SELFDELETE);
          remove_player(player_i);
        }

      delete_aliases(GET_NAME(d->character));
      delete_variables(GET_NAME(d->character));
      write_to_output(d, "Character '%s' deleted! Goodbye.\r\n", GET_NAME(d->character));
      mudlog(NRM, LVL_GOD, TRUE, "%s (lev %d) has self-deleted.", GET_NAME(d->character), GET_LEVEL(d->character));
      STATE(d) = CON_CLOSE;
      return;
    } else {
      write_to_output(d, "\r\nCharacter not deleted.\r\n%s", CONFIG_MENU);
      STATE(d) = CON_MENU;
    }
    break;

  /* It is possible, if enough pulses are missed, to kick someone off while they
   * are at the password prompt. We'll let the game_loop()axe them. */
  case CON_CLOSE:
    break;

  default:
    log("SYSERR: Nanny: illegal state of con'ness (%d) for '%s'; closing connection.",
	STATE(d), d->character ? GET_NAME(d->character) : "<unknown>");
    STATE(d) = CON_DISCONNECT;	/* Safest to do. */
    break;
  }
}
