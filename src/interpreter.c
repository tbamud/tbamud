/**************************************************************************
*  File: interpreter.c                                     Part of tbaMUD *
*  Usage: Parse user commands, search for specials, call ACMD functions.  *
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
#include "db.h"
#include "spells.h"
#include "handler.h"
#include "mail.h"
#include "screen.h"
#include "genolc.h"
#include "oasis.h"
#include "improved-edit.h"
#include "py_triggers.h"
#include "constants.h"
#include "act.h" /* ACMDs located within the act*.c files */
#include "ban.h"
#include "class.h"
#include "species.h"
#include "graph.h"
#include "hedit.h"
#include "house.h"
#include "config.h"
#include "quest.h"
#include "asciimap.h"
#include "prefedit.h"
#include "ibt.h"
#include "mud_event.h"
#include "modify.h" /* to ensure page_string is available */
#include "accounts.h"

/* local (file scope) functions */
static int perform_dupe_check(struct descriptor_data *d);
static struct alias_data *find_alias(struct alias_data *alias_list, char *str);
static void perform_complex_alias(struct txt_q *input_q, char *orig, struct alias_data *a);
static int _parse_name(char *arg, char *name);
static bool perform_new_char_dupe_check(struct descriptor_data *d);
/* sort_commands utility */
static int sort_commands_helper(const void *a, const void *b);
static void show_species_menu(struct descriptor_data *d);
static bool is_creation_state(int state);
static void show_stat_pref_prompt(struct descriptor_data *d);
static int ability_from_pref_arg(const char *arg);
static bool parse_stat_preference(char *input, ubyte *order, ubyte *count);

/* globals defined here, used here and elsewhere */
int *cmd_sort_info = NULL;

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
  { "northwest", "northw"  , POS_STANDING, do_move     , 0, SCMD_NW },
  { "nw"       , "nw"      , POS_STANDING, do_move     , 0, SCMD_NW },
  { "northeast", "northe"  , POS_STANDING, do_move     , 0, SCMD_NE },
  { "ne"       , "ne"      , POS_STANDING, do_move     , 0,  SCMD_NE },
  { "southeast", "southe"  , POS_STANDING, do_move     , 0, SCMD_SE },
  { "se"       , "se"      , POS_STANDING, do_move     , 0, SCMD_SE },
  { "southwest", "southw"  , POS_STANDING, do_move     , 0, SCMD_SW },
  { "sw"       , "sw"      , POS_STANDING, do_move     , 0, SCMD_SW },
  
  /* now, the main list */
  { "audit"    , "aud"     , POS_DEAD    , do_audit    , LVL_IMMORT, 0 },
  { "at"       , "at"      , POS_DEAD    , do_at       , LVL_IMMORT, 0 },
  { "advance"  , "adv"     , POS_DEAD    , do_advance  , LVL_GRGOD, 0 },
  { "aedit"    , "aed"     , POS_DEAD    , do_oasis_aedit, LVL_GOD, 0 },
  { "alias"    , "ali"     , POS_DEAD    , do_alias    , 0, 0 },
  { "afk"      , "afk"     , POS_DEAD    , do_gen_tog  , 0, SCMD_AFK },
  { "areas"    , "are"     , POS_DEAD    , do_areas    , 0, 0 },
  { "assist"   , "as"      , POS_FIGHTING, do_assist   , 1, 0 },
  { "ask"      , "ask"     , POS_RESTING , do_spec_comm, 0, SCMD_ASK },
  { "astat"    , "ast"     , POS_DEAD    , do_astat    , 0, 0 },
  { "attach"   , "attach"  , POS_DEAD    , do_attach   , LVL_BUILDER, 0 },
  { "autoexits" , "autoex"  , POS_DEAD    , do_gen_tog , 0, SCMD_AUTOEXIT },
  { "autoassist","autoass" , POS_DEAD    , do_gen_tog , 0, SCMD_AUTOASSIST },
  { "autodoor" , "autodoor", POS_DEAD    , do_gen_tog , 0, SCMD_AUTODOOR },
  { "autokey"  , "autokey" , POS_DEAD    , do_gen_tog , 0, SCMD_AUTOKEY },
  { "autoloot" , "autoloot", POS_DEAD    , do_gen_tog , 0, SCMD_AUTOLOOT },
  { "automap"  , "automap" , POS_DEAD    , do_gen_tog , 0, SCMD_AUTOMAP },
  { "autosplit", "autospl" , POS_DEAD    , do_gen_tog , 0, SCMD_AUTOSPLIT },

  { "backstab" , "ba"      , POS_STANDING, do_backstab , 1, 0 },
  { "ban"      , "ban"     , POS_DEAD    , do_ban      , LVL_GRGOD, 0 },
  { "bandage"  , "band"    , POS_RESTING , do_bandage  , 1, 0 },
  { "balance"  , "bal"     , POS_STANDING, do_not_here , 1, 0 },
  { "bash"     , "bas"     , POS_FIGHTING, do_bash     , 1, 0 },
  { "brief"    , "br"      , POS_DEAD    , do_gen_tog  , 0, SCMD_BRIEF },
  { "buildwalk", "buildwalk", POS_STANDING, do_gen_tog , LVL_BUILDER, SCMD_BUILDWALK },
  { "buy"      , "bu"      , POS_STANDING, do_not_here , 0, 0 },
  { "bug"      , "bug"     , POS_DEAD    , do_ibt      , 0, SCMD_BUG },

  { "cast"     , "c"       , POS_SITTING , do_cast     , 1, 0 },
  { "cedit"    , "cedit"   , POS_DEAD    , do_oasis_cedit, LVL_IMPL, 0 },
  { "change"   , "chang"   , POS_SLEEPING , do_change   , 0, 0 },
  { "changelog", "cha"     , POS_DEAD    , do_changelog, LVL_IMPL, 0 },
  { "check"    , "ch"      , POS_STANDING, do_not_here , 1, 0 },
  { "checkload", "checkl"  , POS_DEAD    , do_checkloadstatus, LVL_GOD, 0 },
  { "close"    , "cl"      , POS_SITTING , do_gen_door , 0, SCMD_CLOSE },
  { "clear"    , "cle"     , POS_DEAD    , do_gen_ps   , 0, SCMD_CLEAR },
  { "cls"      , "cls"     , POS_DEAD    , do_gen_ps   , 0, SCMD_CLEAR },
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
  { "dismount" , "dism"    , POS_STANDING, do_dismount , 0, 0 },
  { "display"  , "disp"    , POS_DEAD    , do_display  , 0, 0 },
  { "drink"    , "dri"     , POS_RESTING , do_drink    , 0, SCMD_DRINK },
  { "drop"     , "dro"     , POS_RESTING , do_drop     , 0, SCMD_DROP },

  { "eat"      , "ea"      , POS_RESTING , do_eat      , 0, SCMD_EAT },
  { "echo"     , "ec"      , POS_SLEEPING, do_echo     , LVL_IMMORT, SCMD_ECHO },
  { "emote"    , "em"      , POS_RESTING , do_emote     , 0, SCMD_EMOTE },
  { "hemote"   , "hem"     , POS_RESTING , do_hemote    , 0, SCMD_HEMOTE },
  { "enter"    , "ent"     , POS_STANDING, do_enter    , 0, 0 },
  { "equipment", "eq"      , POS_SLEEPING, do_equipment, 0, 0 },
  { "exits"    , "ex"      , POS_RESTING , do_exits    , 0, 0 },
  { "examine"  , "exa"     , POS_SITTING , do_examine  , 0, 0 },
  { "export"   , "export"  , POS_DEAD    , do_export_zone, LVL_IMPL, 0 },

  { "feel"     , "fee"     , POS_SLEEPING, do_feel    , 0, 0 },
  { "force"    , "force"   , POS_SLEEPING, do_force    , LVL_GOD, 0 },
  { "fill"     , "fil"     , POS_STANDING, do_pour     , 0, SCMD_FILL },
  { "file"     , "file"    , POS_SLEEPING, do_file     , LVL_GOD, 0 },
  { "flee"     , "fl"      , POS_FIGHTING, do_flee     , 1, 0 },
  { "follow"   , "fol"     , POS_RESTING , do_follow   , 0, 0 },
  { "freeze"   , "freeze"  , POS_DEAD    , do_wizutil  , LVL_GRGOD, SCMD_FREEZE },

  { "get"      , "g"       , POS_RESTING , do_get      , 0, 0 },
  { "gecho"    , "gecho"   , POS_DEAD    , do_gecho    , LVL_GOD, 0 },
  { "gemote"   , "gem"     , POS_SLEEPING, do_gen_comm , LVL_IMMORT, SCMD_GEMOTE },
  { "give"     , "giv"     , POS_RESTING , do_give     , 0, 0 },
  { "goto"     , "go"      , POS_SLEEPING, do_goto     , LVL_IMMORT, 0 },
  { "coins"    , "coin"    , POS_RESTING , do_coins    , 0, 0 },
  { "group"    , "gr"      , POS_RESTING , do_group    , 1, 0 },
  { "grab"     , "grab"    , POS_RESTING , do_grab     , 0, 0 },

  { "help"     , "h"       , POS_DEAD    , do_help     , 0, 0 },
  { "hedit"    , "hedit"   , POS_DEAD    , do_oasis_hedit, LVL_GOD , 0 },
  { "helpcheck", "helpch"  , POS_DEAD    , do_helpcheck, LVL_GOD, 0 },
  { "hide"     , "hi"      , POS_RESTING , do_hide     , 1, 0 },
  { "hindex"   , "hind"    , POS_DEAD    , do_hindex   , 0, 0 },
  { "handbook" , "handb"   , POS_DEAD    , do_gen_ps   , LVL_IMMORT, SCMD_HANDBOOK },
  { "hcontrol" , "hcontrol", POS_DEAD    , do_hcontrol , LVL_GRGOD, 0 },
  { "history"  , "history" , POS_DEAD    , do_history, 0, 0},
  { "hit"      , "hit"     , POS_FIGHTING, do_hit      , 0, SCMD_HIT },
  { "hold"     , "hold"    , POS_RESTING , do_grab     , 1, 0 },
  { "holylight", "holy"    , POS_DEAD    , do_gen_tog  , LVL_IMMORT, SCMD_HOLYLIGHT },
  { "house"    , "house"   , POS_RESTING , do_house    , 0, 0 },
  { "hitch"    , "hitc"    , POS_STANDING, do_hitch    , 0, 0 },

  { "inventory", "i"       , POS_DEAD    , do_inventory, 0, 0 },
  { "idea"     , "ide"      , POS_DEAD    , do_ibt      , 0, SCMD_IDEA },
  { "imotd"    , "imo"     , POS_DEAD    , do_gen_ps   , LVL_IMMORT, SCMD_IMOTD },
  { "immlist"  , "imm"     , POS_DEAD    , do_gen_ps   , 0, SCMD_IMMLIST },
  { "info"     , "info"    , POS_SLEEPING, do_gen_ps   , 0, SCMD_INFO },
  { "invis"    , "invi"    , POS_DEAD    , do_invis    , LVL_IMMORT, 0 },

  { "junk"     , "j"       , POS_RESTING , do_drop     , 0, SCMD_JUNK },

  { "kill"     , "k"       , POS_FIGHTING, do_kill     , 0, 0 },
  { "kick"     , "ki"      , POS_FIGHTING, do_kick     , 1, 0 },

  { "look"     , "l"       , POS_RESTING , do_look     , 0, SCMD_LOOK },
  { "lower"    , "low"     , POS_SITTING , do_raise_lower_hood, 0, SCMD_LOWER_HOOD },
  { "last"     , "last"    , POS_DEAD    , do_last     , LVL_GOD, 0 },
  { "leave"    , "lea"     , POS_STANDING, do_leave    , 0, 0 },
  { "list"     , "lis"     , POS_STANDING, do_not_here , 0, 0 },
  { "mount"    , "mou"     , POS_STANDING, do_mount    , 0, 0 },
  { "listen"   , "lisn"    , POS_RESTING , do_listen   , 0, 0 },
  { "links"    , "lin"     , POS_STANDING, do_links    , LVL_GOD, 0 },
  { "lock"     , "loc"     , POS_SITTING , do_gen_door , 0, SCMD_LOCK },
  { "load"     , "load"     , POS_DEAD    , do_load     , LVL_BUILDER, 0 },

  { "motd"     , "motd"    , POS_DEAD    , do_gen_ps   , 0, SCMD_MOTD },
  { "mail"     , "mail"    , POS_STANDING, do_not_here , 1, 0 },
  { "map"      , "map"     , POS_STANDING, do_map      , 1, 0 },
  { "medit"    , "med"     , POS_DEAD    , do_oasis_medit, LVL_BUILDER, 0 },
  { "mlist"    , "mlist"   , POS_DEAD    , do_oasis_list, LVL_BUILDER, SCMD_OASIS_MLIST },
  { "mcopy"    , "mcopy"   , POS_DEAD    , do_oasis_copy, LVL_GOD, CON_MEDIT },
  { "mcreate"  , "mcreate" , POS_DEAD    , do_mcreate  , LVL_BUILDER, 0 },
  { "mset"     , "mset"    , POS_DEAD    , do_mset     , LVL_BUILDER, 0 },
  { "msave"    , "msav"    , POS_DEAD    , do_msave,     LVL_BUILDER, 0 },
  { "msgedit"  , "msgedit" , POS_DEAD    , do_msgedit,   LVL_GOD, 0 },
  { "mute"     , "mute"    , POS_DEAD    , do_wizutil  , LVL_GOD, SCMD_MUTE },

  { "news"     , "news"    , POS_SLEEPING, do_gen_ps   , 0, SCMD_NEWS },
  { "nohassle" , "nohassle", POS_DEAD    , do_gen_tog  , LVL_IMMORT, SCMD_NOHASSLE },
  { "norepeat" , "norepeat", POS_DEAD    , do_gen_tog  , 0, SCMD_NOREPEAT },
  { "noshout"  , "noshout" , POS_SLEEPING, do_gen_tog  , 1, SCMD_NOSHOUT },
  { "nosummon" , "nosummon", POS_DEAD    , do_gen_tog  , 1, SCMD_NOSUMMON },
  { "notell"   , "notell"  , POS_DEAD    , do_gen_tog  , 1, SCMD_NOTELL },
  { "nowiz"    , "nowiz"   , POS_DEAD    , do_gen_tog  , LVL_IMMORT, SCMD_NOWIZ },

  { "open"     , "o"       , POS_SITTING , do_gen_door , 0, SCMD_OPEN },
  { "order"    , "ord"     , POS_RESTING , do_order    , 1, 0 },
  { "olc"      , "olc"     , POS_DEAD    , do_show_save_list, LVL_BUILDER, 0 },
  { "olist"    , "olist"   , POS_DEAD    , do_oasis_list, LVL_BUILDER, SCMD_OASIS_OLIST },
  { "oedit"    , "oedit"   , POS_DEAD    , do_oasis_oedit, LVL_BUILDER, 0 },
  { "ocreate"  , "ocreate" , POS_DEAD    , do_ocreate  , LVL_BUILDER, 0 },
  { "ooc"      , "oo"      , POS_RESTING , do_ooc      , 0, 0 },
  { "osave"    , "osave"   , POS_DEAD    , do_osave    , LVL_BUILDER, 0 },
  { "oset"     , "oset"    , POS_DEAD    , do_oset,        LVL_BUILDER, 0 },  
  { "ocopy"    , "ocopy"   , POS_DEAD    , do_oasis_copy, LVL_GOD, CON_OEDIT },

  { "put"      , "p"       , POS_RESTING , do_put      , 0, 0 },
  { "pack"     , "pac"     , POS_RESTING , do_pack     , 0, 0 },
  { "peace"    , "pe"      , POS_DEAD    , do_peace    , LVL_BUILDER, 0 },
  { "pemote"   , "pem"     , POS_SLEEPING, do_pemote   , 0, SCMD_PEMOTE },
  { "phemote"  , "phem"    , POS_SLEEPING, do_phemote  , 0, SCMD_PHEMOTE },
  { "pick"     , "pi"      , POS_STANDING, do_gen_door , 1, SCMD_PICK },
  { "page"     , "pag"     , POS_DEAD    , do_page     , 1, 0 },
  { "pardon"   , "pardon"  , POS_DEAD    , do_wizutil  , LVL_GOD, SCMD_PARDON },
{ "perception","per"     , POS_RESTING , do_scan     , 0, 0 },
  { "palm"     , "palm"    , POS_STANDING, do_palm     , 1, 0 },
  { "plist"    , "plist"   , POS_DEAD    , do_plist    , LVL_GOD, 0 },
  { "policy"   , "pol"     , POS_DEAD    , do_gen_ps   , 0, SCMD_POLICIES },
  { "pour"     , "pour"    , POS_STANDING, do_pour     , 0, SCMD_POUR },
  { "prompt"   , "pro"     , POS_DEAD    , do_display  , 0, 0 },
  { "prefedit" , "pre"     , POS_DEAD    , do_oasis_prefedit , 0, 0 },
  { "purge"    , "purge"   , POS_DEAD    , do_purge    , LVL_BUILDER, 0 },

  { "qcreate"  , "qcreate" , POS_DEAD    , do_qcreate  , LVL_BUILDER, 0 },
  { "qedit"    , "qedit"   , POS_DEAD    , do_oasis_qedit, LVL_BUILDER, 0 },
  { "qlist"    , "qlist"   , POS_DEAD    , do_oasis_list, LVL_BUILDER, SCMD_OASIS_QLIST },
  { "qsave"    , "qsav"    , POS_DEAD    , do_qsave    , LVL_BUILDER, 0 },
  { "qset"     , "qset"    , POS_DEAD    , do_qset     , LVL_BUILDER, 0 },
  { "quaff"    , "qua"     , POS_RESTING , do_use      , 0, SCMD_QUAFF },
  { "qecho"    , "qec"     , POS_DEAD    , do_qcomm    , LVL_GOD, SCMD_QECHO },
  { "quest"    , "que"     , POS_DEAD    , do_quest    , 0, 0 },
  { "qui"      , "qui"     , POS_DEAD    , do_quit     , 0, 0 },
  { "quit"     , "quit"    , POS_DEAD    , do_quit     , 0, SCMD_QUIT },

  { "raise"    , "rai"     , POS_SITTING , do_raise_lower_hood, 0, SCMD_RAISE_HOOD },
  { "reply"    , "r"       , POS_SLEEPING, do_reply    , LVL_IMMORT, 0 },
  { "rest"     , "res"     , POS_RESTING , do_rest     , 0, 0 },
  { "read"     , "rea"     , POS_RESTING , do_look     , 0, SCMD_READ },
  { "reload"   , "reload"  , POS_DEAD    , do_reboot   , LVL_IMPL, 0 },
  { "recite"   , "reci"    , POS_RESTING , do_use      , 0, SCMD_RECITE },
  { "receive"  , "rece"    , POS_STANDING, do_not_here , 1, 0 },
  { "recent"   , "recent"  , POS_DEAD    , do_recent   , LVL_IMMORT, 0 },
  { "remove"   , "rem"     , POS_RESTING , do_remove   , 0, 0 },
  { "report"   , "repo"    , POS_RESTING , do_report   , 0, 0 },
  { "reroll"   , "rero"    , POS_DEAD    , do_reroll   , 0, 0 },
  { "rescue"   , "resc"    , POS_FIGHTING, do_rescue   , 1, 0 },
  { "restore"  , "resto"   , POS_DEAD    , do_restore  , LVL_GOD, 0 },
  { "return"   , "retu"    , POS_DEAD    , do_return   , 0, 0 },
  { "redit"    , "redit"   , POS_DEAD    , do_oasis_redit, LVL_BUILDER, 0 },
  { "rcreate"  , "rcreate" , POS_DEAD    , do_rcreate  , LVL_BUILDER, 0 },
  { "rset"     , "rset"    , POS_DEAD    , do_rset     , LVL_BUILDER, 0 },
  { "rlist"    , "rlist"   , POS_DEAD    , do_oasis_list, LVL_BUILDER, SCMD_OASIS_RLIST },
  { "rcopy"    , "rcopy"   , POS_DEAD    , do_oasis_copy, LVL_GOD, CON_REDIT },
  { "roomflags", "roomflags", POS_DEAD   , do_gen_tog  , LVL_IMMORT, SCMD_SHOWVNUMS },
  { "rsave"    , "rsave"   , POS_DEAD    , do_rsave    , LVL_BUILDER, 0 },

  { "say"      , "s"       , POS_RESTING , do_say      , 0, 0 },
  { "score"    , "sc"      , POS_DEAD    , do_score    , 0, 0 },
  { "scan"     , "sca"     , POS_RESTING , do_scan     , 0, 0 },
  { "scopy"    , "scopy"   , POS_DEAD    , do_oasis_copy, LVL_GOD, CON_SEDIT },
  { "sit"      , "si"      , POS_RESTING , do_sit      , 0, 0 },
  { "'"        , "'"       , POS_RESTING , do_say      , 0, 0 },
  { "save"     , "sav"     , POS_SLEEPING, do_save     , 0, 0 },
  { "saveall"  , "saveall" , POS_DEAD    , do_saveall  , LVL_BUILDER, 0},
  { "sell"     , "sell"    , POS_STANDING, do_not_here , 0, 0 },
  { "sedit"    , "sedit"   , POS_DEAD    , do_oasis_sedit, LVL_BUILDER, 0 },
  { "send"     , "send"    , POS_SLEEPING, do_send     , LVL_GOD, 0 },
  { "set"      , "set"     , POS_DEAD    , do_set      , LVL_IMMORT, 0 },
  { "shout"    , "sho"     , POS_RESTING , do_gen_comm , 0, SCMD_SHOUT },
  { "skills"   , "sk"      , POS_SLEEPING, do_skills   , 0, 0 },
  { "forage"   , "for"     , POS_STANDING, do_forage   , 0, 0 },
  { "skin"     , "skin"    , POS_STANDING, do_skin, 0, 0 },
  { "show"     , "show"    , POS_DEAD    , do_show     , LVL_IMMORT, 0 },
  { "shutdow"  , "shutdow" , POS_DEAD    , do_shutdown , LVL_IMPL, 0 },
  { "shutdown" , "shutdown", POS_DEAD    , do_shutdown , LVL_IMPL, SCMD_SHUTDOWN },
  { "sip"      , "sip"     , POS_RESTING , do_drink    , 0, SCMD_SIP },
  { "sleep"    , "sl"      , POS_SLEEPING, do_sleep    , 0, 0 },
  { "slip"     , "slip"    , POS_STANDING, do_slip     , 1, 0 },
  { "slist"    , "slist"   , POS_SLEEPING, do_oasis_list, LVL_BUILDER, SCMD_OASIS_SLIST },
  { "sneak"    , "sneak"   , POS_STANDING, do_sneak    , 1, 0 },
  { "snoop"    , "snoop"   , POS_DEAD    , do_snoop    , LVL_GOD, 0 },
  { "socials"  , "socials" , POS_DEAD    , do_commands , 0, SCMD_SOCIALS },
  { "split"    , "split"   , POS_SITTING , do_split    , 1, 0 },
  { "stand"    , "st"      , POS_RESTING , do_stand    , 0, 0 },
  { "stat"     , "stat"    , POS_DEAD    , do_stat     , LVL_IMMORT, 0 },
  { "steal"    , "ste"     , POS_STANDING, do_steal    , 1, 0 },
  { "switch"   , "switch"  , POS_DEAD    , do_switch   , LVL_GOD, 0 },

  { "tell"     , "t"       , POS_DEAD    , do_tell     , LVL_IMMORT, 0 },
  { "talk"     , "talk"    , POS_SITTING , do_talk     , 0, 0 },
  { "take"     , "ta"      , POS_RESTING , do_get      , 0, 0 },
  { "taste"    , "tas"     , POS_RESTING , do_eat      , 0, SCMD_TASTE },
  { "teleport" , "tele"    , POS_DEAD    , do_teleport , LVL_BUILDER, 0 },
  { "tedit"    , "tedit"   , POS_DEAD    , do_tedit    , LVL_GOD, 0 },  /* XXX: Oasisify */
  { "thaw"     , "thaw"    , POS_DEAD    , do_wizutil  , LVL_GRGOD, SCMD_THAW },
  { "think"    , "thin"    , POS_RESTING , do_think    , 0, 0 },
  { "time"     , "time"    , POS_DEAD    , do_time     , 0, 0 },
  { "toggle"   , "toggle"  , POS_DEAD    , do_toggle   , 0, 0 },
  { "track"    , "track"   , POS_STANDING, do_track    , 0, 0 },
  { "transfer" , "transfer", POS_SLEEPING, do_trans    , LVL_GOD, 0 },
  { "trigedit" , "trigedit", POS_DEAD    , do_oasis_trigedit, LVL_BUILDER, 0 },
  { "typo"     , "typo"    , POS_DEAD    , do_ibt      , 0, SCMD_TYPO },
  { "tlist"    , "tlist"   , POS_DEAD    , do_oasis_list, LVL_BUILDER, SCMD_OASIS_TLIST },
  { "pylist"   , "pylist"  , POS_DEAD    , do_pylist   , LVL_BUILDER, 0 },
  { "tcopy"    , "tcopy"   , POS_DEAD    , do_oasis_copy, LVL_GOD, CON_TRIGEDIT },
  { "tstat"    , "tstat"   , POS_DEAD    , do_tstat    , LVL_BUILDER, 0 },

  { "unlock"   , "unlock"  , POS_SITTING , do_gen_door , 0, SCMD_UNLOCK },
  { "unban"    , "unban"   , POS_DEAD    , do_unban    , LVL_GRGOD, 0 },
  { "unaffect" , "unaffect", POS_DEAD    , do_wizutil  , LVL_GOD, SCMD_UNAFFECT },
  { "unhitch"  , "unh"     , POS_STANDING, do_unhitch  , 0, 0 },
  { "unfollow" , "unf"     , POS_RESTING , do_unfollow , 0, 0 },
  { "unpack"   , "unpa"    , POS_RESTING , do_unpack   , 0, 0 },
  { "uptime"   , "uptime"  , POS_DEAD    , do_date     , LVL_GOD, SCMD_UPTIME },
  { "use"      , "use"     , POS_SITTING , do_use      , 1, SCMD_USE },
  { "users"    , "users"   , POS_DEAD    , do_users    , LVL_GOD, 0 },

  { "value"    , "val"     , POS_STANDING, do_not_here , 0, 0 },
  { "version"  , "ver"     , POS_DEAD    , do_gen_ps   , 0, SCMD_VERSION },
  { "view"     , "vie"     , POS_STANDING, do_not_here , 0, 0 },
  { "visible"  , "vis"     , POS_RESTING , do_visible  , 1, 0 },
  { "vnum"     , "vnum"    , POS_DEAD    , do_vnum     , LVL_IMMORT, 0 },
  { "vstat"    , "vstat"   , POS_DEAD    , do_vstat    , LVL_IMMORT, 0 },
  { "vdelete"  , "vdelete" , POS_DEAD    , do_vdelete  , LVL_BUILDER, 0 },

  { "wake"     , "wake"    , POS_SLEEPING, do_wake     , 0, 0 },
  { "wear"     , "wea"     , POS_RESTING , do_wear     , 0, 0 },
  { "weather"  , "weather" , POS_RESTING , do_weather  , 0, 0 },
  { "who"      , "wh"      , POS_DEAD    , do_who      , 0, 0 },
  { "whois"    , "whoi"    , POS_DEAD    , do_whois    , 0, 0 },
  { "whoami"   , "whoami"  , POS_DEAD    , do_gen_ps   , 0, SCMD_WHOAMI },
  { "where"    , "where"   , POS_RESTING , do_where    , 1, 0 },
  { "whirlwind", "whirl"   , POS_FIGHTING, do_whirlwind, 0, 0 },
  { "whisper"  , "whisper" , POS_RESTING , do_spec_comm, 0, SCMD_WHISPER },
  { "wield"    , "wie"     , POS_RESTING , do_wield    , 0, 0 },
  { "withdraw" , "withdraw", POS_STANDING, do_not_here , 1, 0 },
  { "wiznet"   , "wiz"     , POS_DEAD    , do_wiznet   , LVL_IMMORT, 0 },
  { ";"        , ";"       , POS_DEAD    , do_wiznet   , LVL_IMMORT, 0 },
  { "wizhelp"  , "wizhelp" , POS_DEAD    , do_wizhelp  , LVL_IMMORT, 0 },
  { "wizlist"  , "wizlist" , POS_DEAD    , do_gen_ps   , 0, SCMD_WIZLIST },
  { "wizupdate", "wizupde" , POS_DEAD    , do_wizupdate, LVL_GRGOD, 0 },
  { "wizlock"  , "wizlock" , POS_DEAD    , do_wizlock  , LVL_IMPL, 0 },
  { "write"    , "write"   , POS_STANDING, do_write    , 1, 0 },

  { "zoneresets", "zoner" ,  POS_DEAD    , do_gen_tog , LVL_IMPL, SCMD_ZONERESETS },
  { "zreset"   , "zreset"  , POS_DEAD    , do_zreset   , LVL_BUILDER, 0 },
  { "zedit"    , "zedit"   , POS_DEAD    , do_oasis_zedit, LVL_BUILDER, 0 },
  { "zlist"    , "zlist"   , POS_DEAD    , do_oasis_list, LVL_BUILDER, SCMD_OASIS_ZLIST },
  { "zlock"    , "zlock"   , POS_DEAD    , do_zlock    , LVL_GOD, 0 },
  { "zunlock"  , "zunlock" , POS_DEAD    , do_zunlock  , LVL_GOD, 0 },
  { "zcheck"   , "zcheck"  , POS_DEAD    , do_zcheck   , LVL_BUILDER, 0 },
  { "zpurge"   , "zpurge"  , POS_DEAD    , do_zpurge   , LVL_BUILDER, 0 },

  { "\n", "zzzzzzz", 0, 0, 0, 0 } };    /* this must be last */


  /* Thanks to Melzaren for this change to allow DG Scripts to be attachable
   *to player's while still disallowing them to manually use the DG-Commands. */
static const struct mob_script_command_t mob_script_commands[] = {

  /* DG trigger commands. minimum_level should be set to -1. */
  { "masound"  , do_masound  , 0 },
  { "mkill"    , do_mkill    , 0 },
  { "mjunk"    , do_mjunk    , 0 },
  { "mdamage"  , do_mdamage  , 0 },
  { "mdoor"    , do_mdoor    , 0 },
  { "mecho"    , do_mecho    , 0 },
  { "mrecho"   , do_mrecho   , 0 },
  { "mechoaround", do_mechoaround , 0 },
  { "msend"    , do_msend    , 0 },
  { "mload"    , do_mload    , 0 },
  { "mpurge"   , do_mpurge   , 0 },
  { "mgoto"    , do_mgoto    , 0 },
  { "mat"      , do_mat      , 0 },
  { "mteleport", do_mteleport, 0 },
  { "mforce"   , do_mforce   , 0 },
  { "mhunt"    , do_mhunt    , 0 },
  { "mremember", do_mremember, 0 },
  { "mforget"  , do_mforget  , 0 },
  { "mtransform", do_mtransform , 0 },
  { "mzoneecho", do_mzoneecho, 0 },
  { "mfollow"  , do_mfollow  , 0 },
  { "mlog"     , do_mlog     , 0 },
  { "\n" , do_not_here , 0 } };

int script_command_interpreter(struct char_data *ch, char *arg) {
  /* DG trigger commands */

  int i;
  char first_arg[MAX_INPUT_LENGTH];
  char *line;

  skip_spaces(&arg);
  if (!*arg)
  return 0;

  line = any_one_arg(arg, first_arg);

  for (i = 0; *mob_script_commands[i].command_name != '\n'; i++)
  if (!str_cmp(first_arg, mob_script_commands[i].command_name))
  break; // NB - only allow full matches.

  if (*mob_script_commands[i].command_name == '\n')
  return 0; // no matching commands.

  /* Poiner to the command? */
  ((*mob_script_commands[i].command_pointer) (ch, line, 0,
  mob_script_commands[i].subcmd));
  return 1; // We took care of execution. Let caller know.
}

static const char *fill[] =
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

static const char *reserved[] =
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

static int sort_commands_helper(const void *a, const void *b)
{
  return strcmp(complete_cmd_info[*(const int *)a].sort_as,
                complete_cmd_info[*(const int *)b].sort_as);
}

void sort_commands(void)
{
  int a, num_of_cmds = 0;

  while (complete_cmd_info[num_of_cmds].command[0] != '\n')
    num_of_cmds++;
  num_of_cmds++;  /* \n */

  CREATE(cmd_sort_info, int, num_of_cmds);

  for (a = 0; a < num_of_cmds; a++)
    cmd_sort_info[a] = a;

  /* Don't sort the RESERVED or \n entries. */
  qsort(cmd_sort_info + 1, num_of_cmds - 2, sizeof(int), sort_commands_helper);
}


/* This is the actual command interpreter called from game_loop() in comm.c
 * It makes sure you are the proper level and position to execute the command,
 * then calls the appropriate function. */
void command_interpreter(struct char_data *ch, char *argument)
{
  int cmd, length;
  char *line;
  char arg[MAX_INPUT_LENGTH];

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

  if (!is_abbrev(arg, "change"))
    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_HIDE);

  /* Since all command triggers check for valid_dg_target before acting, the levelcheck
   * here has been removed. Otherwise, find the command. */
  {
    int cont;                                            /* continue the command checks */
    cont = command_wtrigger(ch, arg, line);              /* any world triggers ? */
    if (!cont) cont = command_mtrigger(ch, arg, line);   /* any mobile triggers ? */
    if (!cont) cont = command_otrigger(ch, arg, line);   /* any object triggers ? */
    if (cont) return;                                    /* yes, command trigger took over */
  }

  /* Allow IMPLs to switch into mobs to test the commands. */
  if (IS_NPC(ch) && ch->desc && GET_LEVEL(ch->desc->original) >= LVL_IMPL) {
    if (script_command_interpreter(ch, argument))
      return;
  }

  for (length = strlen(arg), cmd = 0; *complete_cmd_info[cmd].command != '\n'; cmd++)
    if (complete_cmd_info[cmd].command_pointer != do_action &&
        !strncmp(complete_cmd_info[cmd].command, arg, length))
      if (GET_LEVEL(ch) >= complete_cmd_info[cmd].minimum_level)
        break;

  /* it's not a 'real' command, so it's a social */

  if (*complete_cmd_info[cmd].command == '\n')
    for (length = strlen(arg), cmd = 0; *complete_cmd_info[cmd].command != '\n'; cmd++)
      if (complete_cmd_info[cmd].command_pointer == do_action &&
          !strncmp(complete_cmd_info[cmd].command, arg, length))
        if (GET_LEVEL(ch) >= complete_cmd_info[cmd].minimum_level)
          break;

  if (*complete_cmd_info[cmd].command == '\n') {
    int found = 0;
    send_to_char(ch, "%s", CONFIG_HUH);

    for (cmd = 0; *cmd_info[cmd].command != '\n'; cmd++) {
      if (*arg != *cmd_info[cmd].command || cmd_info[cmd].minimum_level > GET_LEVEL(ch))
        continue;

      /* Only apply levenshtein counts if the command is not a trigger command. */
      if ((levenshtein_distance(arg, cmd_info[cmd].command) <= 2) &&
          (cmd_info[cmd].minimum_level >= 0)) {
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
    }
  else if (no_specials || !special(ch, cmd, line)) {
    /* Execute the command */
    ((*complete_cmd_info[cmd].command_pointer)(ch, line, cmd, complete_cmd_info[cmd].subcmd));

    /* After successful execution, log any immortal command to log/godcmds. */
    if (!IS_NPC(ch) && GET_LEVEL(ch) >= LVL_IMMORT) {
      int rvnum = (IN_ROOM(ch) != NOWHERE) ? world[IN_ROOM(ch)].number : -1;
      const char *line_safe = (line && *line) ? line : "";
      godcmd_log("%s (lev %d, invis %d) in room %d: %s%s%s",
                 GET_NAME(ch),
                 GET_LEVEL(ch),
                 GET_INVIS_LEV(ch),
                 rvnum,
                 complete_cmd_info[cmd].command,
                 *line_safe ? " " : "",
                 line_safe);
    }
  }
}

/* Routines to handle aliasing. */
static struct alias_data *find_alias(struct alias_data *alias_list, char *str)
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

static void perform_complex_alias(struct txt_q *input_q, char *orig, struct alias_data *a)
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
        skip_spaces(&orig);
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
  for (; **string && **string != '\t' && isspace(**string); (*string)++);
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
static int _parse_name(char *arg, char *name)
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

static void show_account_character_list(struct descriptor_data *d)
{
  int i;

  if (!d || !d->account)
    return;

  write_to_output(d, "\r\nCharacter history:\r\n");
  if (d->account->pc_count == 0) {
    write_to_output(d, "  (none)\r\n");
    return;
  }

  for (i = 0; i < d->account->pc_count; i++) {
    int pfilepos = get_ptable_by_name(d->account->pc_list[i]);
    const char *status = "dead";

    if (pfilepos >= 0 && !IS_SET(player_table[pfilepos].flags, PINDEX_DELETED))
      status = "alive";

    write_to_output(d, "  %s (%s)\r\n", d->account->pc_list[i], status);
  }
  write_to_output(d, "\r\n*** PRESS RETURN: ");
}

void send_account_menu(struct descriptor_data *d)
{
  int has_pc;

  if (!d || !d->account)
    return;

  account_refresh_pc(d->account);
  has_pc = account_has_alive_pc(d->account);

  write_to_output(d,
    "\r\n"
    "                     .\r\n"
    "                    /=\\\\\r\n"
    "                   /===\\ \\\r\n"
    "                  /=====\\' \\\r\n"
    "                 /=======\\'' \\\r\n"
    "                /=========\\ ' '\\\r\n"
    "               /===========\\''   \\\r\n"
    "              /=============\\ ' '  \\\r\n"
    "             /===============\\   ''  \\\r\n"
    "            /=================\\' ' ' ' \\\r\n"
    "           /===================\\' ' '  ' \\\r\n"
    "          /=====================\\' '   ' ' \\\r\n"
    "         /=======================\\  '   ' /\r\n"
    "        /=========================\\   ' /\r\n"
    "       /===========================\\'  /\r\n"
    "      /=============| |=============\\/\r\n"
    "\r\n"
    "      -Pyramid of Ikaros, current day.\r\n"
    "\r\n");
  write_to_output(d, "\r\n Account: %s\r\n", d->account->name);
  if (has_pc)
    write_to_output(d, "\t(  C\t)) Connect to %s.\r\n", d->account->pc_name);
  else
    write_to_output(d, "\t(  R\t)) Create a new PC.\r\n");
  write_to_output(d, "\t(  L\t)) List previous characters.\r\n");
  write_to_output(d, "\t(  X\t)) Exit from Miranthas.\r\n\r\n"
                     "   Make your choice: ");
}

#define RECON		1
#define USURP		2
#define UNSWITCH	3

/* This function seems a bit over-extended. */
static int perform_dupe_check(struct descriptor_data *d)
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
    GET_PREF(d->character) = rand_number(1, 128000);
    if (GET_HOST(d->character))
      free(GET_HOST(d->character));
    GET_HOST(d->character) = strdup(d->host);
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
  REMOVE_BIT_AR(PLR_FLAGS(d->character), PLR_MAILING);
  REMOVE_BIT_AR(PLR_FLAGS(d->character), PLR_WRITING);
  STATE(d) = CON_PLAYING;
  MXPSendTag( d, "<VERSION>" );

  switch (mode) {
  case RECON:
    write_to_output(d, "Reconnecting.\r\n");
    act("$n has reconnected.", TRUE, d->character, 0, 0, TO_ROOM);
    mudlog(NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(d->character)), TRUE, "%s [%s] has reconnected.", GET_NAME(d->character), d->host);
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

/* New Char dupe-check called at the start of character creation */
static bool perform_new_char_dupe_check(struct descriptor_data *d)
{
  struct descriptor_data *k, *next_k;
  bool found = FALSE;

  /* Now that this descriptor has successfully logged in, disconnect all
   * other descriptors controlling a character with the same ID number. */

  for (k = descriptor_list; k; k = next_k) {
    next_k = k->next;

    if (k == d)
      continue;

   if (k->character == NULL)
      continue;

    /* Do the player names match? */
    if (!strcmp(GET_NAME(k->character), GET_NAME(d->character))) {
      /* Check the other character is still in creation? */
      if (is_creation_state(STATE(k))) {
        /* Boot the older one */
        k->character->desc = NULL;
        k->character = NULL;
        k->original = NULL;
        write_to_output(k, "\r\nMultiple login detected -- disconnecting.\r\n");
        STATE(k) = CON_CLOSE;

        mudlog(NRM, LVL_GOD, TRUE, "Multiple logins detected in char creation for %s.", GET_NAME(d->character));

        found = TRUE;
      } else {
        /* Something went VERY wrong, boot both chars */
        k->character->desc = NULL;
        k->character = NULL;
        k->original = NULL;
        write_to_output(k, "\r\nMultiple login detected -- disconnecting.\r\n");
        STATE(k) = CON_CLOSE;

        d->character->desc = NULL;
        d->character = NULL;
        d->original = NULL;
        write_to_output(d, "\r\nSorry, due to multiple connections, all your connections are being closed.\r\n");
        write_to_output(d, "\r\nPlease reconnect.\r\n");
        STATE(d) = CON_CLOSE;

        mudlog(NRM, LVL_GOD, TRUE, "SYSERR: Multiple logins with 1st in-game and the 2nd in char creation.");

        found = TRUE;
      }
    }
  }
  return (found);
}

static void show_species_menu(struct descriptor_data *d)
{
  int count = pc_species_count();

  write_to_output(d, "Select a species:\r\n");
  for (int i = 0; i < count; i++) {
    int species = pc_species_list[i];
    write_to_output(d, " %2d) %s\r\n", i + 1, species_types[species]);
  }
  write_to_output(d, "Species: ");
}

static void show_age_prompt(struct descriptor_data *d)
{
  write_to_output(d, "Age (%d-%d): ", MIN_CHAR_AGE, MAX_CHAR_AGE);
}

static bool is_creation_state(int state)
{
  switch (state) {
    case CON_GET_NAME:
    case CON_NAME_CNFRM:
    case CON_PASSWORD:
    case CON_NEWPASSWD:
    case CON_CNFPASSWD:
    case CON_QSEX:
    case CON_QSPECIES:
    case CON_QCLASS:
    case CON_QAGE:
    case CON_QSTAT_PREF:
    case CON_QSHORTDESC:
    case CON_PLR_DESC:
    case CON_PLR_BACKGROUND:
      return TRUE;
    default:
      return FALSE;
  }
}

static void show_stat_pref_prompt(struct descriptor_data *d)
{
  write_to_output(d,
    "\r\nEnter your stat preference, with the first stat being your preferred highest,\r\n"
    "followed by the others in descending order.\r\n"
    "If you list fewer than six, those listed get the highest rolls; the rest are FIFO.\r\n"
    "Example: strength dexterity constitution intelligence wisdom charisma\r\n"
    "   or:  str dex con int wis cha\r\n"
    "Press Enter to skip (first-in, first-out).\r\n"
    "Stat preference: ");
}

static int ability_from_pref_arg(const char *arg)
{
  if (!arg || !*arg)
    return -1;
  if (!str_cmp(arg, "str") || is_abbrev(arg, "strength"))
    return ABIL_STR;
  if (!str_cmp(arg, "dex") || is_abbrev(arg, "dexterity"))
    return ABIL_DEX;
  if (!str_cmp(arg, "con") || is_abbrev(arg, "constitution"))
    return ABIL_CON;
  if (!str_cmp(arg, "int") || is_abbrev(arg, "intelligence"))
    return ABIL_INT;
  if (!str_cmp(arg, "wis") || is_abbrev(arg, "wisdom"))
    return ABIL_WIS;
  if (!str_cmp(arg, "cha") || is_abbrev(arg, "charisma"))
    return ABIL_CHA;
  return -1;
}

static bool parse_stat_preference(char *input, ubyte *order, ubyte *count)
{
  char arg[MAX_INPUT_LENGTH];
  bool seen[NUM_ABILITIES] = { FALSE };

  if (!order || !count)
    return FALSE;

  *count = 0;
  skip_spaces(&input);
  if (!*input)
    return TRUE;

  if (!str_cmp(input, "none") || !str_cmp(input, "no") || !str_cmp(input, "skip"))
    return TRUE;

  while (*input) {
    size_t len;
    int ability;

    input = one_argument(input, arg);
    if (!*arg)
      break;

    len = strlen(arg);
    while (len > 0 && (arg[len - 1] == ',' || arg[len - 1] == '.')) {
      arg[len - 1] = '\0';
      len--;
    }

    if (!*arg)
      continue;

    ability = ability_from_pref_arg(arg);
    if (ability < 0 || ability >= NUM_ABILITIES)
      return FALSE;
    if (seen[ability])
      return FALSE;
    if (*count >= NUM_ABILITIES)
      return FALSE;

    order[*count] = (ubyte)ability;
    (*count)++;
    seen[ability] = TRUE;
  }

  return TRUE;
}

/* load the player, put them in the right room - used by copyover_recover too */
int enter_player_game (struct descriptor_data *d)
{
  int load_result;
  room_vnum saved_vnum;         /* cache pfile value BEFORE any mutations */
  room_rnum start_r = NOWHERE;

  if (!d || !d->character)
    return 0;

  if (!d->account && GET_ACCOUNT(d->character) && *GET_ACCOUNT(d->character))
    d->account = account_load(GET_ACCOUNT(d->character));

  /* Cache the pfile's saved load room FIRST. */
  saved_vnum = GET_LOADROOM(d->character);

  reset_char(d->character);

  if (PLR_FLAGGED(d->character, PLR_INVSTART))
    GET_INVIS_LEV(d->character) = GET_LEVEL(d->character);

  /* Resolve the room to enter (use cached vnum -> rnum). */
  if (saved_vnum != NOWHERE)
    start_r = real_room(saved_vnum);

  /* Fallbacks if invalid/missing. */
  if (start_r == NOWHERE) {
    if (PLR_FLAGGED(d->character, PLR_FROZEN))
      start_r = r_frozen_start_room;
    else if (GET_LEVEL(d->character) >= LVL_IMMORT)
      start_r = r_immort_start_room;
    else
      start_r = r_mortal_start_room;
  }

  /* DG Scripts setup */
  d->character->script_id = GET_IDNUM(d->character);
  add_to_lookup_table(d->character->script_id, (void *)d->character);
  if (!SCRIPT(d->character))
    read_saved_vars(d->character);

  /* Link character and place before equipping. */
  d->character->next = character_list;
  character_list = d->character;
  char_to_room(d->character, start_r);

  /* Load inventory/equipment */
  load_result = Crash_load(d->character);

  {
    static const struct {
      obj_vnum vnum;
      int wear_pos;
    } starting_wear[] = {
      { 153, WEAR_BACK },
      { 165, WEAR_BODY },
      { 166, WEAR_LEGS },
      { 167, WEAR_FEET },
    };
    static const struct {
      obj_vnum vnum;
      int qty;
    } starting_inv[] = {
      { 142, 3 },
      { 160, 1 },
      { 121, 1 },
    };
    int i;
    int has_eq = 0;

    if (GET_LEVEL(d->character) <= 1 && GET_EXP(d->character) <= 1 &&
        d->character->carrying == NULL) {
      for (i = 0; i < NUM_WEARS; i++) {
        if (GET_EQ(d->character, i)) {
          has_eq = 1;
          break;
        }
      }

      if (!has_eq) {
        for (i = 0; i < (int)(sizeof(starting_wear) / sizeof(starting_wear[0])); i++) {
          struct obj_data *obj = read_object(starting_wear[i].vnum, VIRTUAL);
          if (!obj) {
            log("SYSERR: enter_player_game: missing starting wear obj vnum %d",
                starting_wear[i].vnum);
            continue;
          }
          equip_char(d->character, obj, starting_wear[i].wear_pos);
        }

        for (i = 0; i < (int)(sizeof(starting_inv) / sizeof(starting_inv[0])); i++) {
          int qty = starting_inv[i].qty;
          while (qty-- > 0) {
            struct obj_data *obj = read_object(starting_inv[i].vnum, VIRTUAL);
            if (!obj) {
              log("SYSERR: enter_player_game: missing starting inventory obj vnum %d",
                  starting_inv[i].vnum);
              break;
            }
            obj_to_char(obj, d->character);
          }
        }

        add_coins_to_char(d->character, rand_number(800, 1100));
      }
    }
  }

  /* DO NOT save here — avoids clobbering Room with NOWHERE on login. */
  /* login_wtrigger can remain. */
  login_wtrigger(&world[IN_ROOM(d->character)], d->character);

  return load_result;
}

EVENTFUNC(get_protocols)
{
  struct descriptor_data *d;
  struct mud_event_data *pMudEvent;
  char buf[MAX_STRING_LENGTH];
  size_t len;

  if (event_obj == NULL)
    return 0;
  
  pMudEvent = (struct mud_event_data *) event_obj;
  d = (struct descriptor_data *) pMudEvent->pStruct;  
  
  /* Clear extra white space from the "protocol scroll" */
  write_to_output(d, "[H[J");

  len = snprintf(buf, MAX_STRING_LENGTH,   "\tO[\toClient\tO] \tw%s\tn | ", d->pProtocol->pVariables[eMSDP_CLIENT_ID]->pValueString);

  if (d->pProtocol->pVariables[eMSDP_XTERM_256_COLORS]->ValueInt)
    len += snprintf(buf + len, MAX_STRING_LENGTH - len, "\tO[\toColors\tO] \tw256\tn | ");
  else if (d->pProtocol->pVariables[eMSDP_ANSI_COLORS]->ValueInt)
    len += snprintf(buf + len, MAX_STRING_LENGTH - len, "\tO[\toColors\tO] \twAnsi\tn | ");
  else
    len += snprintf(buf + len, MAX_STRING_LENGTH - len, "[Colors] No Color | ");
 
  len += snprintf(buf + len, MAX_STRING_LENGTH - len,   "\tO[\toMXP\tO] \tw%s\tn | ", d->pProtocol->bMXP ? "Yes" : "No");
  len += snprintf(buf + len, MAX_STRING_LENGTH - len,   "\tO[\toMSDP\tO] \tw%s\tn | ", d->pProtocol->bMSDP ? "Yes" : "No");
  snprintf(buf + len, MAX_STRING_LENGTH - len,   "\tO[\toATCP\tO] \tw%s\tn\r\n\r\n", d->pProtocol->bATCP ? "Yes" : "No");
   
  write_to_output(d, buf, 0);
    
  write_to_output(d, GREETINGS, 0); 
  STATE(d) = CON_GET_CONNECT;
  return 0;
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
    { CON_HEDIT, hedit_parse },
    { CON_QEDIT, qedit_parse },
    { CON_PREFEDIT, prefedit_parse },
    { CON_IBTEDIT, ibtedit_parse },
    { CON_MSGEDIT, msgedit_parse },
    { -1, NULL }
  };

  skip_spaces(&arg);

  /* Quick check for the OLC states. */
  for (player_i = 0; olc_functions[player_i].state >= 0; player_i++)
    if (STATE(d) == olc_functions[player_i].state) {
      (*olc_functions[player_i].func)(d, arg);
      return;
    }

  /* Not in OLC. */
  switch (STATE(d)) {
  case CON_GET_PROTOCOL:
    write_to_output(d, "Collecting Protocol Information... Please Wait.\r\n"); 
    return;
  case CON_GET_CONNECT:
    if (!*arg) {
      write_to_output(d, GREETINGS, 0);
      return;
    }
    switch (UPPER(*arg)) {
      case 'C':
        write_to_output(d, "Account name: ");
        STATE(d) = CON_GET_ACCOUNT;
        return;
      case 'X':
        write_to_output(d, "Goodbye.\r\n");
        STATE(d) = CON_CLOSE;
        return;
      default:
        write_to_output(d,
          "That is an invalid selection.\r\n");
        write_to_output(d, GREETINGS, 0);
        return;
    }
  case CON_GET_ACCOUNT:
    if (!*arg) {
      STATE(d) = CON_CLOSE;
      break;
    } else {
      char buf[MAX_INPUT_LENGTH], tmp_name[MAX_INPUT_LENGTH];

      if ((_parse_name(arg, tmp_name)) || strlen(tmp_name) < 2 ||
          strlen(tmp_name) > MAX_NAME_LENGTH || !valid_name(tmp_name) ||
          fill_word(strcpy(buf, tmp_name)) || reserved_word(buf)) {	/* strcpy: OK (mutual MAX_INPUT_LENGTH) */
        write_to_output(d, "Invalid account name, please try another.\r\nAccount name: ");
        return;
      }

      if (d->account) {
        account_free(d->account);
        d->account = NULL;
      }

      d->account = account_load(tmp_name);
      if (d->account) {
        write_to_output(d, "Password: ");
        echo_off(d);
        d->idle_tics = 0;
        STATE(d) = CON_ACCOUNT_PASSWORD;
        return;
      }

      if ((player_i = get_ptable_by_name(tmp_name)) >= 0 &&
          !IS_SET(player_table[player_i].flags, PINDEX_DELETED)) {
        d->account = account_create(tmp_name);
        account_set_pc(d->account, player_table[player_i].name);
        write_to_output(d, "An existing character was found. Create an account to link it (\t(Y\t)/\t(N\t))? ");
        STATE(d) = CON_ACCOUNT_CNFRM;
        return;
      }

      d->account = account_create(tmp_name);
      write_to_output(d, "Create a new account %s (\t(Y\t)/\t(N\t))? ", tmp_name);
      STATE(d) = CON_ACCOUNT_CNFRM;
      return;
    }

  case CON_GET_NAME:		/* wait for input of name */
    if (!d->account) {
      STATE(d) = CON_CLOSE;
      return;
    }
    if (d->character == NULL) {
      CREATE(d->character, struct char_data, 1);
      clear_char(d->character);
      CREATE(d->character->player_specials, struct player_special_data, 1);

      new_mobile_data(d->character);

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
      if (account_has_pc(d->account, tmp_name)) {
        write_to_output(d, "That name has already been used, try something else.\r\nName: ");
        return;
      }

      if ((player_i = get_ptable_by_name(tmp_name)) >= 0) {
        if (IS_SET(player_table[player_i].flags, PINDEX_DELETED)) {
          remove_player(player_i);
        } else {
          write_to_output(d, "That name is already taken.\r\nName: ");
          return;
        }
      }

      if (GET_NAME(d->character))
        free(GET_NAME(d->character));
      CREATE(d->character->player.name, char, strlen(tmp_name) + 1);
      strcpy(d->character->player.name, CAP(tmp_name));	/* strcpy: OK (size checked above) */

      write_to_output(d, "Did I get that right, %s (\t(Y\t)/\t(N\t))? ", tmp_name);
      STATE(d) = CON_NAME_CNFRM;
    }
    break;

  case CON_ACCOUNT_CNFRM:
    if (UPPER(*arg) == 'Y') {
      if (isbanned(d->host) >= BAN_NEW) {
        mudlog(NRM, LVL_GOD, TRUE, "Request for new account %s denied from [%s] (siteban)",
               d->account ? d->account->name : "<unknown>", d->host);
        write_to_output(d, "Sorry, new accounts are not allowed from your site!\r\n");
        STATE(d) = CON_CLOSE;
        return;
      }
      if (circle_restrict) {
        write_to_output(d, "Sorry, new accounts can't be created at the moment.\r\n");
        mudlog(NRM, LVL_GOD, TRUE, "Request for new account %s denied from [%s] (wizlock)",
               d->account ? d->account->name : "<unknown>", d->host);
        STATE(d) = CON_CLOSE;
        return;
      }
      write_to_output(d, "New account.\r\nGive me a password: ");
      echo_off(d);
      STATE(d) = CON_ACCOUNT_NEWPASSWD;
    } else if (*arg == 'n' || *arg == 'N') {
      account_free(d->account);
      d->account = NULL;
      write_to_output(d, "Okay, what IS it, then?\r\nAccount name: ");
      STATE(d) = CON_GET_ACCOUNT;
    } else
      write_to_output(d, "Please type Yes or No: ");
    break;

  case CON_ACCOUNT_PASSWORD:
    echo_on(d);
    write_to_output(d, "\r\n");

    if (!*arg)
      STATE(d) = CON_CLOSE;
    else if (!d->account) {
      STATE(d) = CON_CLOSE;
    } else {
      if (strncmp(CRYPT(arg, d->account->passwd), d->account->passwd, MAX_PWD_LENGTH)) {
        mudlog(BRF, LVL_GOD, TRUE, "Bad account PW: %s [%s]", d->account->name, d->host);
        if (++(d->bad_pws) >= CONFIG_MAX_BAD_PWS) {
          write_to_output(d, "Wrong password... disconnecting.\r\n");
          STATE(d) = CON_CLOSE;
        } else {
          write_to_output(d, "Wrong password.\r\nPassword: ");
          echo_off(d);
        }
        return;
      }

      d->bad_pws = 0;
      send_account_menu(d);
      STATE(d) = CON_ACCOUNT_MENU;
    }
    break;

  case CON_ACCOUNT_NEWPASSWD:
    if (!*arg || strlen(arg) > MAX_PWD_LENGTH || strlen(arg) < 3 ||
        !str_cmp(arg, d->account ? d->account->name : "")) {
      write_to_output(d, "\r\nIllegal password.\r\nPassword: ");
      return;
    }
    strncpy(d->account->passwd, CRYPT(arg, d->account->name), MAX_PWD_LENGTH);
    d->account->passwd[MAX_PWD_LENGTH] = '\0';

    write_to_output(d, "\r\nPlease retype password: ");
    STATE(d) = CON_ACCOUNT_CNFPASSWD;
    break;

  case CON_ACCOUNT_CNFPASSWD:
    if (strncmp(CRYPT(arg, d->account->passwd), d->account->passwd, MAX_PWD_LENGTH)) {
      write_to_output(d, "\r\nPasswords don't match... start over.\r\nPassword: ");
      STATE(d) = CON_ACCOUNT_NEWPASSWD;
      return;
    }
    echo_on(d);

    write_to_output(d, "\r\nEmail address (optional, press Enter to skip): ");
    STATE(d) = CON_ACCOUNT_EMAIL;
    break;

  case CON_ACCOUNT_EMAIL:
    if (d->account) {
      if (d->account->email)
        free(d->account->email);
      d->account->email = NULL;
      if (*arg)
        d->account->email = strdup(arg);
      account_save(d->account);
    }
    write_to_output(d, "\r\nAccount created.\r\n");
    send_account_menu(d);
    STATE(d) = CON_ACCOUNT_MENU;
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
      perform_new_char_dupe_check(d);
      if (GET_ACCOUNT(d->character))
        free(GET_ACCOUNT(d->character));
      GET_ACCOUNT(d->character) = d->account ? strdup(d->account->name) : NULL;
      write_to_output(d, "New character.\r\nWhat is your sex (\t(M\t)/\t(F\t))? ");
      STATE(d) = CON_QSEX;
    } else if (*arg == 'n' || *arg == 'N') {
      write_to_output(d, "Okay, what IS it, then?\r\nName: ");
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

      /* Add to the list of 'recent' players (since last reboot) */
      if (AddRecentPlayer(GET_NAME(d->character), d->host, FALSE, FALSE) == FALSE)
      {
        mudlog(BRF, MAX(LVL_IMMORT, GET_INVIS_LEV(d->character)), TRUE, "Failure to AddRecentPlayer (returned FALSE).");
      }

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
      write_to_output(d, "\r\nWhat is your sex (\t(M\t)/\t(F\t))? ");
      STATE(d) = CON_QSEX;
    } else {
      save_char(d->character);
      write_to_output(d, "\r\nDone.\r\n");
      send_account_menu(d);
      STATE(d) = CON_ACCOUNT_MENU;
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

    show_species_menu(d);
    STATE(d) = CON_QSPECIES;
    break;

case CON_QSPECIES: {
  int choice = atoi(arg);
  int species = species_from_pc_choice(choice);

  if (species == SPECIES_UNDEFINED) {
    write_to_output(d, "\r\nThat's not a species.\r\n");
    show_species_menu(d);
    return;
  }

  GET_SPECIES(d->character) = species;
  write_to_output(d, "%s\r\nClass: ", class_menu);
  STATE(d) = CON_QCLASS;
  break;
}

case CON_QCLASS:
  load_result = parse_class(*arg);
  if (load_result == CLASS_UNDEFINED) {
    write_to_output(d, "\r\nThat's not a class.\r\nClass: ");
    return;
  } else {
    GET_CLASS(d->character) = load_result;
  }

  show_age_prompt(d);
  STATE(d) = CON_QAGE;
  return;

case CON_QAGE: {
  if (!is_number(arg)) {
    write_to_output(d, "\r\nPlease enter a number between %d and %d.\r\n",
                    MIN_CHAR_AGE, MAX_CHAR_AGE);
    show_age_prompt(d);
    return;
  }

  int age_years = atoi(arg);
  if (age_years < MIN_CHAR_AGE || age_years > MAX_CHAR_AGE) {
    write_to_output(d, "\r\nAge must be between %d and %d.\r\n",
                    MIN_CHAR_AGE, MAX_CHAR_AGE);
    show_age_prompt(d);
    return;
  }

  GET_ROLEPLAY_AGE(d->character) = age_years;
  GET_ROLEPLAY_AGE_YEAR(d->character) = time_info.year;

  show_stat_pref_prompt(d);
  STATE(d) = CON_QSTAT_PREF;
  return;
}

case CON_QSTAT_PREF: {
  ubyte order[NUM_ABILITIES];
  ubyte count = 0;

  if (!parse_stat_preference(arg, order, &count)) {
    write_to_output(d,
      "\r\nInvalid stat list. Please enter a valid order, or press Enter to skip.\r\n");
    show_stat_pref_prompt(d);
    return;
  }

  d->character->stat_pref_use = TRUE;
  d->character->stat_pref_count = count;
  for (int i = 0; i < NUM_ABILITIES; i++) {
    d->character->stat_pref_order[i] = (i < count) ? order[i] : 0;
  }

  /* Create player entry and initialize character now so file exists */
  if (d->olc) {
    free(d->olc);
    d->olc = NULL;
  }
  if (GET_PFILEPOS(d->character) < 0)
    GET_PFILEPOS(d->character) = create_entry(GET_PC_NAME(d->character));

  /* Initialize base stats, starting level, etc. */
  init_char(d->character);
  if (d->account && d->account->name) {
    if (GET_ACCOUNT(d->character))
      free(GET_ACCOUNT(d->character));
    GET_ACCOUNT(d->character) = strdup(d->account->name);
  }
  save_char(d->character);
  save_player_index();
  if (d->account) {
    account_set_pc(d->account, GET_NAME(d->character));
    account_save(d->account);
  }

  /* Log and register early so new players are tracked immediately */
  GET_PREF(d->character) = rand_number(1, 128000);
  GET_HOST(d->character) = strdup(d->host);
  mudlog(NRM, LVL_GOD, TRUE, "%s [%s] new player created (awaiting description).",
         GET_NAME(d->character), d->host);

  if (AddRecentPlayer(GET_NAME(d->character), d->host, TRUE, FALSE) == FALSE)
    mudlog(BRF, MAX(LVL_IMMORT, GET_INVIS_LEV(d->character)), TRUE,
           "Failure to AddRecentPlayer (returned FALSE).");

  /* === NEW: mandatory short description before main description === */
  write_to_output(d,
    "\r\nBefore entering the world, you must choose a short description.\r\n"
    "This is what others see in the room list and messages instead of your name.\r\n"
    "It should describe your appearance, not identity.\r\n\r\n"
    "Examples:\r\n"
    "  the tall, muscular man\r\n"
    "  the lanky, sharp-eyed elf\r\n"
    "  the short, bald dwarf\r\n\r\n"
    "Do not include your character's name here.\r\n"
    "Short description: ");

  STATE(d) = CON_QSHORTDESC;
  return;
}

  case CON_QSHORTDESC: {
      skip_spaces(&arg);

      if (!*arg) {
          write_to_output(d, "\r\nA short description cannot be empty.\r\n"
                            "Short description: ");
          return;
      }

      /* Do not allow their character name in the sdesc */
      if (str_str(arg, GET_NAME(d->character))) {
          write_to_output(d, "\r\nYour short description must not include your name.\r\n"
                            "Short description: ");
          return;
      }

      /* Enforce length limit (optional, but recommended) */
      if (strlen(arg) > MAX_NAME_LENGTH * 2) {
          write_to_output(d, "\r\nThat description is too long.\r\n"
                            "Short description: ");
          return;
      }

      /* Store as player's short description */
      if (GET_SHORT_DESC(d->character))
          free(GET_SHORT_DESC(d->character));

      GET_SHORT_DESC(d->character) = strdup(arg);

      /* Immediately save it to disk */
      save_char(d->character);
      save_player_index();

      /* Now transition to full description input */
      write_to_output(d,
        "\r\nBefore entering the world, please describe your character.\r\n"
        "Focus on what others can immediately see — height, build, complexion,\r\n"
        "facial structure, hair, eyes, and other physical details. Avoid names,\r\n"
        "clothing, or personal information that someone would not know meeting\r\n"
        "you for the first time.\r\n\r\n"
        "Example:\r\n"
        "  This broad-shouldered human stands with a relaxed but watchful bearing.\r\n"
        "  Weather and sun have darkened their skin, and faint scars trace the backs\r\n"
        "  of their hands. Their eyes are a pale, gray-green hue, steady and alert\r\n"
        "  beneath a low brow. Thick, uneven hair falls around a strong jaw and\r\n"
        "  angular features.\r\n\r\n");

      d->backstr = NULL;
      d->str = &d->character->player.description;
      d->max_str = PLR_DESC_LENGTH;
      STATE(d) = CON_PLR_DESC;

      send_editor_help(d);
      return;
  }

  case CON_PLR_DESC:
    /* Description accepted — save and continue */
    save_char(d->character);

    if (!GET_BACKGROUND(d->character) || !*GET_BACKGROUND(d->character)) {
      write_to_output(d,
        "\r\nBefore stepping into Miranthas, share a bit of your character's background.\r\n"
        "Guideline: aim for at least four lines that hint at where they came from,\r\n"
        "who shaped them, and why they now walk the Caleran region. Touch on things like:\r\n"
        "  - The city-state, tribe, or caravan that claimed them.\r\n"
        "  - Mentors, slavers, or patrons who left a mark.\r\n"
        "  - A defining hardship, triumph, oath, or secret.\r\n"
        "  - The goal, vengeance, or hope that drives them back into the wastes.\r\n"
        "\r\nExample 1:\r\n"
        "   Raised beneath the ziggurat of Caleran, I learned to barter gossip between\r\n"
        "   templars and gladiators just to stay alive. Freedom came when Kalak fell,\r\n"
        "   but the slave-scar on my shoulder still aches. I now search the desert\r\n"
        "   for the relic my clutch mates died protecting, hoping to buy their kin peace.\r\n"
        "\r\nExample 2:\r\n"
        "   I rode caravan outrider routes from Balic until giants shattered our train.\r\n"
        "   Two nights buried in silt taught me to whisper with the wind and trust only\r\n"
        "   my erdlu. I hunt the warlord who sold us out, yet coin and company on the\r\n"
        "   road must come first.\r\n"
        "\r\nExample 3:\r\n"
        "   Born outside Raam, I was tempered by obsidian shards and psionic murmurs.\r\n"
        "   A defiler ruined our oasis, so I swore to hound such spell-scars wherever\r\n"
        "   they bloom. Rumor of a hidden well near Caleran is the lone hope that guides me.\r\n"
        "\r\nType your background now. Use '/s' on a blank line when you finish.\r\n"
        "If you'd rather keep it secret, just save immediately and we'll note the mystery.\r\n\r\n");
      d->backstr = NULL;
      if (GET_BACKGROUND(d->character) && *GET_BACKGROUND(d->character))
        d->backstr = strdup(GET_BACKGROUND(d->character));
      d->str = &d->character->player.background;
      d->max_str = PLR_DESC_LENGTH;
      STATE(d) = CON_PLR_BACKGROUND;
      send_editor_help(d);
      return;
    }

    write_to_output(d, "%s\r\n*** PRESS RETURN: ", motd);
    STATE(d) = CON_RMOTD;

    /* Final log confirmation */
    mudlog(NRM, LVL_GOD, TRUE, "%s [%s] completed character creation.",
           GET_NAME(d->character), d->host);
    break;

  case CON_RMOTD:		/* read CR after printing motd   */
    if (!d->character) {
      send_account_menu(d);
      STATE(d) = CON_ACCOUNT_MENU;
      break;
    }
    if (!d->account && GET_ACCOUNT(d->character) && *GET_ACCOUNT(d->character))
      d->account = account_load(GET_ACCOUNT(d->character));
    add_llog_entry(d->character, LAST_CONNECT);

    load_result = enter_player_game(d);
    send_to_char(d->character, "%s", CONFIG_WELC_MESSG);

    save_char(d->character);

    greet_mtrigger(d->character, -1);
    greet_memory_mtrigger(d->character);

    act("$n has entered the game.", TRUE, d->character, 0, 0, TO_ROOM);

    STATE(d) = CON_PLAYING;
    MXPSendTag(d, "<VERSION>");

    if (GET_LEVEL(d->character) == 0) {
      do_start(d->character);
      send_to_char(d->character, "%s", CONFIG_START_MESSG);
    }

    look_at_room(d->character, 0);
    if (has_mail(GET_IDNUM(d->character)))
      send_to_char(d->character, "You have mail waiting.\r\n");

    d->has_prompt = 0;
    /* We've updated to 3.1 - some bits might be set wrongly: */
    REMOVE_BIT_AR(PRF_FLAGS(d->character), PRF_BUILDWALK);
    break;

  case CON_ACCOUNT_MENU: {
    int has_pc;

    if (!d->account) {
      STATE(d) = CON_CLOSE;
      break;
    }

    account_refresh_pc(d->account);
    has_pc = account_has_alive_pc(d->account);

    switch (*arg) {
    case 'X':
    case 'x':
      write_to_output(d, "Goodbye.\r\n");
      if (d->character)
        add_llog_entry(d->character, LAST_QUIT);
      STATE(d) = CON_CLOSE;
      break;
    case 'L':
    case 'l':
      show_account_character_list(d);
      STATE(d) = CON_ACCOUNT_LIST;
      break;
    case 'C':
    case 'c':
      if (has_pc) {
        if (d->character) {
          free_char(d->character);
          d->character = NULL;
        }
        CREATE(d->character, struct char_data, 1);
        clear_char(d->character);
        CREATE(d->character->player_specials, struct player_special_data, 1);
        new_mobile_data(d->character);

        GET_HOST(d->character) = strdup(d->host);
        d->character->desc = d;

        if ((player_i = load_char(d->account->pc_name, d->character)) < 0) {
          write_to_output(d, "Could not load that character.\r\n");
          send_account_menu(d);
          STATE(d) = CON_ACCOUNT_MENU;
          break;
        }
        GET_PFILEPOS(d->character) = player_i;

        if (GET_ACCOUNT(d->character) && str_cmp(GET_ACCOUNT(d->character), d->account->name)) {
          write_to_output(d, "That character does not belong to this account.\r\n");
          free_char(d->character);
          d->character = NULL;
          send_account_menu(d);
          STATE(d) = CON_ACCOUNT_MENU;
          break;
        }
        if (!GET_ACCOUNT(d->character) && d->account->name) {
          GET_ACCOUNT(d->character) = strdup(d->account->name);
          save_char(d->character);
        }

        /* undo it just in case they are set */
        REMOVE_BIT_AR(PLR_FLAGS(d->character), PLR_WRITING);
        REMOVE_BIT_AR(PLR_FLAGS(d->character), PLR_MAILING);
        REMOVE_BIT_AR(PLR_FLAGS(d->character), PLR_CRYO);
        d->character->player.time.logon = time(0);

        if (isbanned(d->host) == BAN_SELECT &&
            !PLR_FLAGGED(d->character, PLR_SITEOK)) {
          write_to_output(d, "Sorry, this char has not been cleared for login from your site!\r\n");
          STATE(d) = CON_CLOSE;
          mudlog(NRM, LVL_GOD, TRUE, "Connection attempt for %s denied from %s",
                 GET_NAME(d->character), d->host);
          return;
        }
        if (GET_LEVEL(d->character) < circle_restrict) {
          write_to_output(d, "The game is temporarily restricted.. try again later.\r\n");
          STATE(d) = CON_CLOSE;
          mudlog(NRM, LVL_GOD, TRUE, "Request for login denied for %s [%s] (wizlock)",
                 GET_NAME(d->character), d->host);
          return;
        }

        if (perform_dupe_check(d))
          return;

        if (GET_LEVEL(d->character) >= LVL_IMMORT)
          write_to_output(d, "%s", imotd);
        else
          write_to_output(d, "%s", motd);

        if (GET_INVIS_LEV(d->character))
          mudlog(BRF, MAX(LVL_IMMORT, GET_INVIS_LEV(d->character)), TRUE,
                 "%s has connected. (invis %d)", GET_NAME(d->character),
                 GET_INVIS_LEV(d->character));
        else
          mudlog(BRF, LVL_IMMORT, TRUE, "%s has connected.", GET_NAME(d->character));

        if (AddRecentPlayer(GET_NAME(d->character), d->host, FALSE, FALSE) == FALSE) {
          mudlog(BRF, MAX(LVL_IMMORT, GET_INVIS_LEV(d->character)), TRUE,
                 "Failure to AddRecentPlayer (returned FALSE).");
        }

        write_to_output(d, "\r\n*** PRESS RETURN: ");
        STATE(d) = CON_RMOTD;
      } else {
        write_to_output(d, "You do not have a character to connect.\r\n");
        send_account_menu(d);
        STATE(d) = CON_ACCOUNT_MENU;
      }
      break;
    case 'R':
    case 'r':
      if (has_pc) {
        write_to_output(d, "You already have a character. Delete it before creating another.\r\n");
        send_account_menu(d);
        STATE(d) = CON_ACCOUNT_MENU;
      } else {
        if (d->character) {
          free_char(d->character);
          d->character = NULL;
        }
        write_to_output(d, "By what name do you wish to be known? ");
        STATE(d) = CON_GET_NAME;
      }
      break;
    default:
      write_to_output(d, "\r\nThat's not a menu choice!\r\n");
      send_account_menu(d);
      break;
    }
    break;
  }

  case CON_MENU: {		/* get selection from main menu  */
    if (!d->account) {
      write_to_output(d, "No account loaded.\r\n");
      STATE(d) = CON_CLOSE;
      break;
    }
    send_account_menu(d);
    STATE(d) = CON_ACCOUNT_MENU;
    break;
  }

  case CON_ACCOUNT_LIST:
    send_account_menu(d);
    STATE(d) = CON_ACCOUNT_MENU;
    break;

  case CON_CHPWD_GETOLD:
    if (strncmp(CRYPT(arg, GET_PASSWD(d->character)), GET_PASSWD(d->character), MAX_PWD_LENGTH)) {
      echo_on(d);
      write_to_output(d, "\r\nIncorrect password.\r\n");
      send_account_menu(d);
      STATE(d) = CON_ACCOUNT_MENU;
    } else {
      write_to_output(d, "\r\nEnter a new password: ");
      STATE(d) = CON_CHPWD_GETNEW;
    }
    return;

  case CON_DELCNF1:
    echo_on(d);
    if (strncmp(CRYPT(arg, GET_PASSWD(d->character)), GET_PASSWD(d->character), MAX_PWD_LENGTH)) {
      write_to_output(d, "\r\nIncorrect password.\r\n");
      send_account_menu(d);
      STATE(d) = CON_ACCOUNT_MENU;
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
	SET_BIT_AR(PLR_FLAGS(d->character), PLR_DELETED);
      save_char(d->character);
      Crash_delete_file(GET_NAME(d->character));
      /* If the selfdelete_fastwipe flag is set (in config.c), remove all the
       * player's immediately. */
      if (selfdelete_fastwipe)
        if ((player_i = get_ptable_by_name(GET_NAME(d->character))) >= 0) {
          SET_BIT(player_table[player_i].flags, PINDEX_SELFDELETE);
          remove_player(player_i);
        }

      delete_variables(GET_NAME(d->character));
      write_to_output(d, "Character '%s' deleted! Goodbye.\r\n", GET_NAME(d->character));
      mudlog(NRM, MAX(LVL_GOD, GET_INVIS_LEV(d->character)), TRUE, "%s (lev %d) has self-deleted.",
       GET_NAME(d->character), GET_LEVEL(d->character));
      STATE(d) = CON_CLOSE;
      return;
    } else {
      write_to_output(d, "\r\nCharacter not deleted.\r\n");
      send_account_menu(d);
      STATE(d) = CON_ACCOUNT_MENU;
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
