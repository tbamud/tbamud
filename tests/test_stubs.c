/**
 * @file test_stubs.c
 * Stub definitions used by unit-test binaries.
 *
 * Every function here is declared __attribute__((weak)) so that a real
 * definition provided by a compiled source file (e.g. class.c providing
 * parse_class(), interpreter.c providing is_abbrev()) automatically wins
 * over the stub at link time.
 *
 * Global-variable stubs are plain definitions (zero-initialised by the
 * C standard for translation-unit scope).  They satisfy the extern
 * declarations in mud headers without conflicting with any source file
 * that is deliberately excluded from the test build.
 */

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "handler.h"
#include "interpreter.h"
#include "class.h"
#include "dg_scripts.h"
#include "protocol.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

/* =========================================================
 * Global variable stubs
 * ========================================================= */

/* comm.c */
FILE                      *logfile          = NULL;  /* tests init to stderr in setUp */
struct descriptor_data    *descriptor_list  = NULL;
int                        no_specials      = 0;
int                        circle_restrict  = 0;

/* db.c */
struct room_data          *world            = NULL;
room_rnum                  top_of_world     = 0;
struct weather_data        weather_info;          /* zero-init */
struct char_data          *character_list   = NULL;
struct index_data         *mob_index        = NULL;
struct index_data         *obj_index        = NULL;
mob_rnum                   top_of_mobt      = 0;
obj_rnum                   top_of_objt      = 0;
char                      *motd             = NULL;
char                      *imotd            = NULL;
char                      *GREETINGS        = NULL;
char                      *background       = NULL;
struct happyhour           happy_data;             /* zero-init */
struct player_index_element *player_table   = NULL;
struct player_special_data dummy_mob;              /* zero-init */
struct config_data         config_info;            /* zero-init */
time_t                     motdmod          = 0;
time_t                     newsmod          = 0;

/* interpreter needs some start-room vnum stubs */
ush_int r_mortal_start_room = 0;
ush_int r_immort_start_room = 0;
ush_int r_frozen_start_room = 0;

/* config.c */
int selfdelete_fastwipe = 0;

/* constants.c – only needed when class.c is NOT in the build */
__attribute__((weak)) const struct con_app_type con_app[26];
__attribute__((weak)) const struct wis_app_type wis_app[26];

/* class.c – only needed when class.c is NOT in the build */
__attribute__((weak)) const char *class_menu = "";
__attribute__((weak)) const char *pc_class_types[] = { "\n" };

/* =========================================================
 * Function stubs  (all weak so the real implementation wins)
 * ========================================================= */

/* ---------- comm.c ---------- */
__attribute__((weak))
size_t send_to_char(struct char_data *ch, const char *messg, ...)
{ (void)ch; (void)messg; return 0; }

__attribute__((weak))
char *act(const char *str, int hide_invisible, struct char_data *ch,
          struct obj_data *obj, void *vict_obj, int type)
{ (void)str; (void)hide_invisible; (void)ch;
  (void)obj; (void)vict_obj; (void)type; return NULL; }

__attribute__((weak))
void write_to_q(const char *txt, struct txt_q *queue, int aliased)
{ (void)txt; (void)queue; (void)aliased; }

__attribute__((weak))
size_t write_to_output(struct descriptor_data *d, const char *txt, ...)
{ (void)d; (void)txt; return 0; }

__attribute__((weak))
size_t vwrite_to_output(struct descriptor_data *d, const char *fmt, va_list args)
{ (void)d; (void)fmt; (void)args; return 0; }

__attribute__((weak))
void echo_off(struct descriptor_data *d) { (void)d; }

__attribute__((weak))
void echo_on(struct descriptor_data *d) { (void)d; }

/* ---------- modify.c ---------- */
__attribute__((weak))
void page_string(struct descriptor_data *d, char *str, int keep_internal)
{ (void)d; (void)str; (void)keep_internal; }

__attribute__((weak))
void parse_tab(char *str) { (void)str; }

/* ---------- handler.c ---------- */
__attribute__((weak))
bool affected_by_spell(struct char_data *ch, int spell)
{ (void)ch; (void)spell; return FALSE; }

__attribute__((weak))
void affect_from_char(struct char_data *ch, int type)
{ (void)ch; (void)type; }

__attribute__((weak))
void extract_char(struct char_data *ch) { (void)ch; }

__attribute__((weak))
void extract_char_final(struct char_data *ch) { (void)ch; }

__attribute__((weak))
void char_from_room(struct char_data *ch) { (void)ch; }

__attribute__((weak))
void char_to_room(struct char_data *ch, room_rnum room)
{ (void)ch; (void)room; }

__attribute__((weak))
void free_char(struct char_data *ch) { (void)ch; }

/* ---------- interpreter.c ---------- */
__attribute__((weak))
int is_abbrev(const char *arg1, const char *arg2)
{ (void)arg1; (void)arg2; return 0; }

__attribute__((weak))
int parse_class(char arg)
{ (void)arg; return CLASS_UNDEFINED; }

/* ---------- class.c ---------- */
__attribute__((weak))
void set_title(struct char_data *ch, char *title) { (void)ch; (void)title; }

__attribute__((weak))
void spell_level(int spell, int chclass, int level)
{ (void)spell; (void)chclass; (void)level; }

/* ---------- players.c ---------- */
__attribute__((weak))
void save_char(struct char_data *ch) { (void)ch; }

__attribute__((weak))
int create_entry(char *name) { (void)name; return 0; }

__attribute__((weak))
int load_char(const char *name, struct char_data *ch)
{ (void)name; (void)ch; return -1; }

__attribute__((weak))
void save_player_index(void) {}

__attribute__((weak))
void remove_player(int pfilepos) { (void)pfilepos; }

__attribute__((weak))
long get_ptable_by_name(const char *name) { (void)name; return -1; }

/* ---------- act.wizard.c ---------- */
__attribute__((weak))
void snoop_check(struct char_data *ch) { (void)ch; }

__attribute__((weak))
void add_llog_entry(struct char_data *ch, int type) { (void)ch; (void)type; }

/* ---------- db.c ---------- */
__attribute__((weak))
room_rnum real_room(room_vnum vnum) { (void)vnum; return NOWHERE; }

__attribute__((weak))
void clear_char(struct char_data *ch) { (void)ch; if (ch) memset(ch, 0, sizeof(*ch)); }

__attribute__((weak))
void reset_char(struct char_data *ch) { (void)ch; }

__attribute__((weak))
void init_char(struct char_data *ch) { (void)ch; }

__attribute__((weak))
void new_mobile_data(struct char_data *ch) { (void)ch; }

__attribute__((weak))
void free_char_from_db(struct char_data *ch) { (void)ch; }

__attribute__((weak))
void Crash_crashsave(struct char_data *ch) { (void)ch; }

__attribute__((weak))
int Crash_load(struct char_data *ch) { (void)ch; return 0; }

__attribute__((weak))
int Crash_delete_file(char *name) { (void)name; return 0; }

/* ---------- ban.c ---------- */
__attribute__((weak))
int isbanned(char *hostname) { (void)hostname; return 0; }

__attribute__((weak))
int valid_name(char *newname) { (void)newname; return 1; }

/* ---------- mail.c ---------- */
__attribute__((weak))
int has_mail(long recipient) { (void)recipient; return 0; }

/* ---------- improved-edit.c ---------- */
__attribute__((weak))
void send_editor_help(struct descriptor_data *d) { (void)d; }

/* ---------- dg_scripts.c ---------- */
__attribute__((weak))
void add_to_lookup_table(long uid, void *c) { (void)uid; (void)c; }

__attribute__((weak))
void delete_variables(const char *charname) { (void)charname; }

__attribute__((weak))
void read_saved_vars(struct char_data *ch) { (void)ch; }

/* ---------- dg_triggers.c ---------- */
__attribute__((weak))
int greet_mtrigger(struct char_data *actor, int dir)
{ (void)actor; (void)dir; return 1; }

__attribute__((weak))
void greet_memory_mtrigger(struct char_data *actor) { (void)actor; }

__attribute__((weak))
int login_wtrigger(struct room_data *room, struct char_data *actor)
{ (void)room; (void)actor; return 1; }

__attribute__((weak))
int command_mtrigger(struct char_data *actor, char *cmd, char *argument)
{ (void)actor; (void)cmd; (void)argument; return 0; }

__attribute__((weak))
int command_otrigger(struct char_data *actor, char *cmd, char *argument)
{ (void)actor; (void)cmd; (void)argument; return 0; }

__attribute__((weak))
int command_wtrigger(struct char_data *actor, char *cmd, char *argument)
{ (void)actor; (void)cmd; (void)argument; return 0; }

/* ---------- act.informative.c ---------- */
__attribute__((weak))
void look_at_room(struct char_data *ch, int ignore_brief)
{ (void)ch; (void)ignore_brief; }

/* ---------- protocol.c ---------- */
__attribute__((weak))
void MXPSendTag(descriptor_t *apDescriptor, const char *apTag)
{ (void)apDescriptor; (void)apTag; }

__attribute__((weak))
void AddRecentPlayer(char *charname, char *host, bool newplr, bool cpover)
{ (void)charname; (void)host; (void)newplr; (void)cpover; }

/* ---------- OLC parse functions ---------- */
__attribute__((weak))
void aedit_parse(struct descriptor_data *d, char *arg) { (void)d; (void)arg; }
__attribute__((weak))
void cedit_parse(struct descriptor_data *d, char *arg) { (void)d; (void)arg; }
__attribute__((weak))
void hedit_parse(struct descriptor_data *d, char *arg) { (void)d; (void)arg; }
__attribute__((weak))
void ibtedit_parse(struct descriptor_data *d, char *arg) { (void)d; (void)arg; }
__attribute__((weak))
void medit_parse(struct descriptor_data *d, char *arg) { (void)d; (void)arg; }
__attribute__((weak))
void msgedit_parse(struct descriptor_data *d, char *arg) { (void)d; (void)arg; }
__attribute__((weak))
void oedit_parse(struct descriptor_data *d, char *arg) { (void)d; (void)arg; }
__attribute__((weak))
void prefedit_parse(struct descriptor_data *d, char *arg) { (void)d; (void)arg; }
__attribute__((weak))
void qedit_parse(struct descriptor_data *d, char *arg) { (void)d; (void)arg; }
__attribute__((weak))
void redit_parse(struct descriptor_data *d, char *arg) { (void)d; (void)arg; }
__attribute__((weak))
void sedit_parse(struct descriptor_data *d, char *arg) { (void)d; (void)arg; }
__attribute__((weak))
void trigedit_parse(struct descriptor_data *d, char *arg) { (void)d; (void)arg; }
__attribute__((weak))
void zedit_parse(struct descriptor_data *d, char *arg) { (void)d; (void)arg; }

/* ---------- ACMD stubs for all do_* functions ----------
 * These are function-pointer entries in the cmd_info[] table in interpreter.c.
 * The table is DATA (not called at test time), but the linker still requires
 * every symbol to resolve.  Weak stubs satisfy the linker; the real do_*
 * implementations would override them if act*.c were compiled. */

#define STUB_ACMD(name) \
  __attribute__((weak)) ACMD(name) { (void)ch; (void)argument; (void)cmd; (void)subcmd; }

STUB_ACMD(do_action)
STUB_ACMD(do_advance)
STUB_ACMD(do_areas)
STUB_ACMD(do_assist)
STUB_ACMD(do_astat)
STUB_ACMD(do_at)
STUB_ACMD(do_attach)
STUB_ACMD(do_backstab)
STUB_ACMD(do_ban)
STUB_ACMD(do_bandage)
STUB_ACMD(do_bash)
STUB_ACMD(do_cast)
STUB_ACMD(do_changelog)
STUB_ACMD(do_checkloadstatus)
STUB_ACMD(do_commands)
STUB_ACMD(do_consider)
STUB_ACMD(do_copyover)
STUB_ACMD(do_date)
STUB_ACMD(do_dc)
STUB_ACMD(do_detach)
STUB_ACMD(do_diagnose)
STUB_ACMD(do_dig)
STUB_ACMD(do_display)
STUB_ACMD(do_drink)
STUB_ACMD(do_drop)
STUB_ACMD(do_eat)
STUB_ACMD(do_echo)
STUB_ACMD(do_enter)
STUB_ACMD(do_equipment)
STUB_ACMD(do_examine)
STUB_ACMD(do_exits)
STUB_ACMD(do_export_zone)
STUB_ACMD(do_file)
STUB_ACMD(do_flee)
STUB_ACMD(do_follow)
STUB_ACMD(do_force)
STUB_ACMD(do_gecho)
STUB_ACMD(do_gen_comm)
STUB_ACMD(do_gen_door)
STUB_ACMD(do_gen_ps)
STUB_ACMD(do_gen_tog)
STUB_ACMD(do_get)
STUB_ACMD(do_give)
STUB_ACMD(do_gold)
STUB_ACMD(do_goto)
STUB_ACMD(do_grab)
STUB_ACMD(do_group)
STUB_ACMD(do_gsay)
STUB_ACMD(do_happyhour)
STUB_ACMD(do_hcontrol)
STUB_ACMD(do_help)
STUB_ACMD(do_helpcheck)
STUB_ACMD(do_hide)
STUB_ACMD(do_hindex)
STUB_ACMD(do_history)
STUB_ACMD(do_hit)
STUB_ACMD(do_house)
STUB_ACMD(do_ibt)
STUB_ACMD(do_inventory)
STUB_ACMD(do_invis)
STUB_ACMD(do_kick)
STUB_ACMD(do_kill)
STUB_ACMD(do_last)
STUB_ACMD(do_leave)
STUB_ACMD(do_levels)
STUB_ACMD(do_links)
STUB_ACMD(do_load)
STUB_ACMD(do_look)
STUB_ACMD(do_map)
STUB_ACMD(do_masound)
STUB_ACMD(do_mat)
STUB_ACMD(do_mdamage)
STUB_ACMD(do_mdoor)
STUB_ACMD(do_mecho)
STUB_ACMD(do_mechoaround)
STUB_ACMD(do_mfollow)
STUB_ACMD(do_mforce)
STUB_ACMD(do_mforget)
STUB_ACMD(do_mgoto)
STUB_ACMD(do_mhunt)
STUB_ACMD(do_mjunk)
STUB_ACMD(do_mkill)
STUB_ACMD(do_mload)
STUB_ACMD(do_mlog)
STUB_ACMD(do_move)
STUB_ACMD(do_mpurge)
STUB_ACMD(do_mrecho)
STUB_ACMD(do_mremember)
STUB_ACMD(do_msend)
STUB_ACMD(do_msgedit)
STUB_ACMD(do_mteleport)
STUB_ACMD(do_mtransform)
STUB_ACMD(do_mzoneecho)
STUB_ACMD(do_not_here)
STUB_ACMD(do_oasis_aedit)
STUB_ACMD(do_oasis_cedit)
STUB_ACMD(do_oasis_copy)
STUB_ACMD(do_oasis_hedit)
STUB_ACMD(do_oasis_list)
STUB_ACMD(do_oasis_medit)
STUB_ACMD(do_oasis_oedit)
STUB_ACMD(do_oasis_prefedit)
STUB_ACMD(do_oasis_qedit)
STUB_ACMD(do_oasis_redit)
STUB_ACMD(do_oasis_sedit)
STUB_ACMD(do_oasis_trigedit)
STUB_ACMD(do_oasis_zedit)
STUB_ACMD(do_order)
STUB_ACMD(do_oset)
STUB_ACMD(do_page)
STUB_ACMD(do_peace)
STUB_ACMD(do_plist)
STUB_ACMD(do_pour)
STUB_ACMD(do_practice)
STUB_ACMD(do_purge)
STUB_ACMD(do_put)
STUB_ACMD(do_qcomm)
STUB_ACMD(do_quest)
STUB_ACMD(do_quit)
STUB_ACMD(do_reboot)
STUB_ACMD(do_recent)
STUB_ACMD(do_remove)
STUB_ACMD(do_reply)
STUB_ACMD(do_report)
STUB_ACMD(do_rescue)
STUB_ACMD(do_rest)
STUB_ACMD(do_restore)
STUB_ACMD(do_return)
STUB_ACMD(do_sac)
STUB_ACMD(do_save)
STUB_ACMD(do_saveall)
STUB_ACMD(do_say)
STUB_ACMD(do_scan)
STUB_ACMD(do_score)
STUB_ACMD(do_send)
STUB_ACMD(do_set)
STUB_ACMD(do_show)
STUB_ACMD(do_show_save_list)
STUB_ACMD(do_shutdown)
STUB_ACMD(do_sit)
STUB_ACMD(do_skillset)
STUB_ACMD(do_sleep)
STUB_ACMD(do_sneak)
STUB_ACMD(do_snoop)
STUB_ACMD(do_spec_comm)
STUB_ACMD(do_split)
STUB_ACMD(do_stand)
/* do_start has a different prototype than ACMD — it's called directly */
__attribute__((weak))
void do_start(struct char_data *ch) { (void)ch; }

STUB_ACMD(do_stat)
STUB_ACMD(do_steal)
STUB_ACMD(do_switch)
STUB_ACMD(do_tedit)
STUB_ACMD(do_teleport)
STUB_ACMD(do_tell)
STUB_ACMD(do_time)
STUB_ACMD(do_title)
STUB_ACMD(do_toggle)
STUB_ACMD(do_track)
STUB_ACMD(do_trans)
STUB_ACMD(do_tstat)
STUB_ACMD(do_unban)
STUB_ACMD(do_unfollow)
STUB_ACMD(do_use)
STUB_ACMD(do_users)
STUB_ACMD(do_vdelete)
STUB_ACMD(do_visible)
STUB_ACMD(do_vnum)
STUB_ACMD(do_vstat)
STUB_ACMD(do_wake)
STUB_ACMD(do_wear)
STUB_ACMD(do_weather)
STUB_ACMD(do_where)
STUB_ACMD(do_whirlwind)
STUB_ACMD(do_who)
STUB_ACMD(do_whois)
STUB_ACMD(do_wield)
STUB_ACMD(do_wizhelp)
STUB_ACMD(do_wizlock)
STUB_ACMD(do_wiznet)
STUB_ACMD(do_wizupdate)
STUB_ACMD(do_wizutil)
STUB_ACMD(do_write)
STUB_ACMD(do_zcheck)
STUB_ACMD(do_zlock)
STUB_ACMD(do_zpurge)
STUB_ACMD(do_zreset)
STUB_ACMD(do_zunlock)

#undef STUB_ACMD
