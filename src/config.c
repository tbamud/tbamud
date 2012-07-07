/**************************************************************************
*  File: config.c                                          Part of tbaMUD *
*  Usage: Configuration of various aspects of tbaMUD operation.           *
*                                                                         *
*  All rights reserved.  See license for complete information.            *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
**************************************************************************/

#define __CONFIG_C__

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "interpreter.h"	/* alias_data definition for structs.h */
#include "config.h"
#include "asciimap.h"

/* Update:  The following constants and variables are now the default values
 * for backwards compatibility with the new cedit game configurator.  If you
 * would not like to use the cedit command, you can change the values in
 * this file instead.  - Mythran */

/* Below are several constants which you can change to alter certain aspects
 * of the way tbaMUD acts.  Since this is a .c file, all you have to do
 * to change one of the constants (assuming you keep your object files around)
 * is change the constant in this file and type 'make'.  Make will recompile
 * this file and relink; you don't have to wait for the whole thing to recompile
 * as you do if you change a header file. I realize that it would be slightly
 * more efficient to have lots of #defines strewn about, so that, for example,
 * the autowiz code isn't compiled at all if you don't want to use autowiz.
 * However, the actual code for the various options is quite small, as is the
 * computational time in checking the option you've selected at run-time, so
 * I've decided the convenience of having all your options in this one file
 * outweighs the efficency of doing it the other way. */

/* YES / NO; TRUE / FALSE are all defined in utils.h */

/* Can Scripts be attached to players? */
int script_players = NO;

/* pk_allowed sets the tone of the entire game.  If pk_allowed is set to NO,
 * then players will not be allowed to kill, summon, charm, or sleep other
 * players, as well as a variety of other "asshole player" protections. However,
 * if you decide you want to have an all-out knock-down drag-out PK Mud, just
 * set pk_allowed to YES - and anything goes. */
int pk_allowed = NO;

/* Is playerthieving allowed? */
int pt_allowed = NO;

/* Minimum level a player must be to shout/holler/gossip/auction. */
int level_can_shout = 1;

/* Number of movement points it costs to holler. */
int holler_move_cost = 20;

/* How many people can get into a tunnel?  The default is two, but there is
 * also an alternate message in the case of one person being allowed. */
int tunnel_size = 2;

/* Exp change limits. */
int max_exp_gain = 100000;	/* max gainable per kill */
int max_exp_loss = 500000;	/* max losable per death */

/* Number of tics (usually 75 seconds) before PC/NPC corpses decompose. */
int max_npc_corpse_time = 5;
int max_pc_corpse_time = 10;

/* How many ticks before a player is sent to the void or idle-rented. */
int idle_void = 8;
int idle_rent_time = 48;

/* This level and up is immune to idling, LVL_IMPL+1 will disable it. */
int idle_max_level = LVL_GOD;

/* Should items in death traps automatically be junked? */
int dts_are_dumps = YES;

/* Whether you want items that immortals load to appear on the ground or not.
 * It is most likely best to set this to 'YES' so that something else doesn't
 * grab the item before the immortal does, but that also means people will be
 * able to carry around things like boards. */
int load_into_inventory = YES;

/* "okay" etc. */
const char *OK = "Okay.\r\n";
const char *NOPERSON = "No one by that name here.\r\n";
const char *NOEFFECT = "Nothing seems to happen.\r\n";

/* You can define or not define TRACK_THOUGH_DOORS, depending on whether or not
 * you want track to find paths which lead through closed or hidden doors. A
 * setting of 'NO' means to not go through the doors while 'YES' will pass
 * through doors to find the target. */
int track_through_doors = YES;

/* If you do not want mortals to level up to immortal once they have enough
 * experience, then set this to YES. Subtracting this from LVL_IMMORT gives
 * the top level that people can advance to in gain_exp() in limits.c */
int no_mort_to_immort = YES;

/* Are diagonal directions enabled?
* If set to NO, then only the 6 directions n,e,s,w,u,d are allowed */
int diagonal_dirs = NO;

/* RENT/CRASHSAVE OPTIONS */
/* Should the MUD allow you to 'rent' for free?  (i.e. if you just quit, your
 * objects are saved at no cost). */
int free_rent = YES;

/* Maximum number of items players are allowed to rent. */
int max_obj_save = 30;

/* Receptionist's surcharge on top of item costs. */
int min_rent_cost = 100;

/* Should the game automatically save people?  (i.e., save player data every 4
 * kills (on average), and Crash-save as defined below. If auto_save is YES,
 * then the 'save' command will be disabled to prevent item duplication via
 * game crashes. */
int auto_save = YES;

/* if auto_save (above) is yes, how often (in minutes) should the MUD Crash-save
 * people's objects?   Also, this number indicates how often the MUD will Crash-
 * save players' houses. */
int autosave_time = 5;

/* Lifetime of crashfiles and forced-rent (idlesave) files in days. */
int crash_file_timeout = 10;

/* Lifetime of normal rent files in days. */
int rent_file_timeout = 30;

/* Do you want to automatically wipe players who've been gone too long? */
int auto_pwipe = NO;

/* Autowipe deletion criteria. This struct holds information used to determine
 * which players to wipe when the mud boots.  The levels must be in ascending
 * order, with a descending level marking the end of the array.  A level -1
 * entry in the beginning is the case for players with the PLR_DELETED flag.
 * The values below match the stock purgeplay.c criteria.

   Detailed explanation by array element:
   * Element 0, level -1, days 0: Players with PLR_DELETED flag are always wiped
   * Element 1, level 0, days 0: Players at level 0 have created a character,
     but have never actually entered the game, so always wipe them.
   * Element 2, level 1, days 4: Players at level 1 are wiped if they haven't
     logged on in the past 4 days.
   * Element 3, level 4, days 7: Players level 2 through 4 are wiped if they
     haven't logged on in the past 7 days.
   * Element 4, level 10, days 30: Players level 5-10 get 30 days.
   * Element 5, level LVL_IMMORT - 1, days 60: All other mortals get 60 days.
   * Element 6, level LVL_IMPL, days 90: Immortals get 90 days.
   * Element 7: Because -2 is less than LVL_IMPL, this is assumed to be the end
     of the criteria.  The days entry is not used in this case. */
struct pclean_criteria_data pclean_criteria[] = {
/*	LEVEL		DAYS	*/
  {	0		,0	}, /* level 0 */
  {	1		,4	},
  {	4		,7	},
  {	10		,30	},
  {	LVL_IMMORT - 1	,60	}, /* highest mortal */
  {	LVL_IMPL	,90	}, /* all immortals */
  {	-1		,0	}  /* no more level checks */
};

/* Do you want players who self-delete to be wiped immediately with no backup? */
int selfdelete_fastwipe = YES;

/* ROOM NUMBERS */
/* Virtual number of room that mortals should enter at. */
room_vnum mortal_start_room = 3001;

/* Virtual number of room that immorts should enter at by default. */
room_vnum immort_start_room = 1204;

/* Virtual number of room that frozen players should enter at. */
room_vnum frozen_start_room = 1202;

/* Virtual numbers of donation rooms.  note: you must change code in do_drop of
 * act.item.c if you change the number of non-NOWHERE donation rooms. */
room_vnum donation_room_1 = 3063;
room_vnum donation_room_2 = 5510;
room_vnum donation_room_3 = 235;

/* GAME OPERATION OPTIONS */

/* If false stock world files will be converted to 128bit. If true the MUD will
 * exit with a warning when encountering stock world files. */
int bitwarning = FALSE;

/* If you want to look at normal world files but DO NOT want to save to 128bit
 * format, turn this to false. However, do not save through olc, or your
 * world files will be 128bit anyway. */
int bitsavetodisk = TRUE;

/* This is the default port on which the game should run if no port is given on
 * the command-line.  NOTE WELL: If you're using the 'autorun' script, the port
 * number there will override this setting. Change the PORT= line in autorun
 * instead of (or in addition to) changing this. */
ush_int DFLT_PORT = 4000;

/* IP address to which the MUD should bind.  This is only useful if you're
 * running Circle on a host that host more than one IP interface, and you only
 * want to bind to *one* of them instead of all of them. Setting this to NULL
 * (the default) causes Circle to bind to all interfaces on the host.
 * Otherwise, specify a numeric IP address in dotted quad format, and Circle
 * will only bind to that IP address.  (Of course, that IP address must be one
 * of your host's interfaces, or it won't work.) */
const char *DFLT_IP = NULL; /* bind to all interfaces */
/* const char *DFLT_IP = "192.168.1.1";  -- bind only to one interface */

/* Default directory to use as data directory. */
const char *DFLT_DIR = "lib";

/* What file to log messages to (ex: "log/syslog").  Setting this to NULL means
 * you want to log to stderr, which was the default in earlier versions of
 * Circle.  If you specify a file, you don't get messages to the screen. (Hint:
 * Try 'tail -f' if you have a UNIX machine.) */
const char *LOGNAME = NULL;
/* const char *LOGNAME = "log/syslog";  -- useful for Windows users */

/* Maximum number of players allowed before game starts to turn people away. */
int max_playing = 300;

/* Maximum size of bug, typo and idea files in bytes (to prevent bombing). */
int max_filesize = 50000;

/* Maximum number of password attempts before disconnection. */
int max_bad_pws = 3;

/* Rationale for enabling this, as explained by Naved:
 * Usually, when you select ban a site, it is because one or two people are
 * causing troubles while there are still many people from that site who you
 * want to still log on.  Right now if I want to add a new select ban, I need
 * to first add the ban, then SITEOK all the players from that site except for
 * the one or two who I don't want logging on.  Wouldn't it be more convenient
 * to just have to remove the SITEOK flags from those people I want to ban
 * rather than what is currently done? */
int siteok_everyone = TRUE;

/* Some nameservers are very slow and cause the game to lag terribly every time
 * someone logs in.  The lag is caused by the gethostbyaddr() function which is
 * responsible for resolving numeric IP addresses to alphabetic names.
 * Sometimes, nameservers can be so slow that the incredible lag caused by
 * gethostbyaddr() isn't worth the luxury of having names instead of numbers
 * for players' sitenames. If your nameserver is fast, set the variable below
 * to NO.  If your nameserver is slow, of it you would simply prefer to have
 * numbers instead of names for some other reason, set the variable to YES.
 * You can experiment with the setting of nameserver_is_slow on-line using the
 * SLOWNS command from within the MUD. */
int nameserver_is_slow = NO;

/* Will changes save automaticaly in OLC? */
int auto_save_olc = YES;

/* if you wish to enable Aedit, set this to YES. This will make the mud look
 * for a file called socials.new, which is in a different format than the
 * stock socials file. */
int use_new_socials = YES;

const char *MENU =
"\r\n"
"Welcome to tbaMUD!\r\n"
"\t(0\t)) Exit from tbaMUD.\r\n"
"\t(1\t)) Enter the game.\r\n"
"\t(2\t)) Enter description.\r\n"
"\t(3\t)) Read the background story.\r\n"
"\t(4\t)) Change password.\r\n"
"\t(5\t)) Delete this character.\r\n"
"\r\n"
"   Make your choice: ";

const char *WELC_MESSG =
"\r\n"
"Welcome to tbaMUD!  May your visit here be... Enlightening"
"\r\n\r\n";

const char *START_MESSG =
"Welcome.  This is your new tbaMUD character!  You can now earn gold,\r\n"
"gain experience, find weapons and equipment, and much more -- while\r\n"
"meeting people from around the world!\r\n";

/* AUTOWIZ OPTIONS */
/* Should the game automatically create a new wizlist/immlist every time someone
 * immorts, or is promoted to a higher (or lower) god level? NOTE: this only
 * works under UNIX systems. */
int use_autowiz = YES;

/* If yes, what is the lowest level which should be on the wizlist?  (All immort
 * levels below the level you specify will go on the immlist instead.) */
int min_wizlist_lev = LVL_GOD;

/* To mimic stock behavior set to NO. To allow mortals to see doors in exits
 * set to YES. */
int display_closed_doors = YES;

/* Automap and map options */
/* Default is to have automap and map command only enabled for immortals */
int map_option = MAP_IMM_ONLY;
int default_map_size = 6;
int default_minimap_size = 2;

/* Medit Stats menu - show 'advanced' options? */
int medit_advanced_stats = YES;

/* Does "bug resolve" autosave ? */
int ibt_autosave = YES;

/* Use the protocol negotiation system */
int protocol_negotiation = YES;

/* Use the special character in communication channels */
int special_in_comm = YES;

/* Current Debug Mode */
int debug_mode = OFF;
