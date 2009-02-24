/**
* @file comm.h
* Configuration of various aspects of tbaMUD operation.
*
* Part of the core tbaMUD source code distribution, which is a derivative
* of, and continuation of, CircleMUD.
*
* All rights reserved.  See license for complete information.
* Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University
* CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.
*
*/
#ifndef _CONFIG_H_
#define _CONFIG_H_

#ifndef __CONFIG_C__
/* Global variable declarations, all settable by cedit */
extern int pk_allowed;
extern int script_players;
extern int pt_allowed;
extern int level_can_shout;
extern int holler_move_cost;
extern int tunnel_size;
extern int max_exp_gain;
extern int max_exp_loss;
extern int max_npc_corpse_time;
extern int max_pc_corpse_time;
extern int idle_void;
extern int idle_rent_time;
extern int idle_max_level;
extern int dts_are_dumps;
extern int load_into_inventory;
extern const char *OK;
extern const char *NOPERSON;
extern const char *NOEFFECT;
extern int track_through_doors;
extern int no_mort_to_immort;
extern int free_rent;
extern int max_obj_save;
extern int min_rent_cost;
extern int auto_save;
extern int autosave_time;
extern int crash_file_timeout;
extern int rent_file_timeout;
/* Room Numbers */
extern room_vnum mortal_start_room;
extern room_vnum immort_start_room;
extern room_vnum frozen_start_room;
extern room_vnum donation_room_1;
extern room_vnum donation_room_2;
extern room_vnum donation_room_3;
/* Game Operation settings */
extern ush_int DFLT_PORT;
extern const char *DFLT_IP;
extern const char *DFLT_DIR;
extern const char *LOGNAME;
extern int max_playing;
extern int max_filesize;
extern int max_bad_pws;
extern int siteok_everyone;
extern int nameserver_is_slow;
extern int auto_save_olc;
extern int use_new_socials;
extern const char *MENU;
extern const char *WELC_MESSG;
extern const char *START_MESSG;
extern int use_autowiz;
extern int min_wizlist_lev;
extern int display_closed_doors;
/* Automap and map options */
extern int map_option;
extern int default_map_size;
extern int default_minimap_size;

extern int medit_advanced_stats;
/*
 * Variables not controlled by cedit
 */
/* Game operation settings. */
extern int bitwarning;
extern int bitsavetodisk;
extern int auto_pwipe;
extern struct pclean_criteria_data pclean_criteria[];
extern int selfdelete_fastwipe;

#endif /* __CONFIG_C__ */

#endif /* _CONFIG_H_*/
