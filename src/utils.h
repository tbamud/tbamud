/**
* @file utils.h
* Utility macros and prototypes of utility functions.
*
* Part of the core tbaMUD source code distribution, which is a derivative
* of, and continuation of, CircleMUD.
*
* All rights reserved.  See license for complete information.
* Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University
* CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.
*
* @todo Merge structs, random and other very generic functions and macros into
* the utils module.
* @todo Take more mud specific functions and function prototypes (follower
* functions, move functions, char_from_furniture) out of utils and declare /
* define elsewhere.
*/
#ifndef _UTILS_H_ /* Begin header file protection */
#define _UTILS_H_

/** Definition of the action command, for the do_ series of in game functions.
 * This macro is placed here (for now) because it's too general of a macro
 * to be first defined in interpreter.h. The reason for using a macro is
 * to allow for easier addition of parameters to the otherwise generic and
 * static function structure. */
#define ACMD(name)  \
   void name(struct char_data *ch, char *argument, int cmd, int subcmd)

/* external declarations and prototypes */

/** direct all log() references to basic_mud_log() function. */
#define log			basic_mud_log

/** Standard line size, used for many string limits. */
#define READ_SIZE	256

/* Public functions made available from utils.c. Documentation for all functions
 * are made available with the function definition. */
void basic_mud_log(const char *format, ...) __attribute__ ((format (printf, 1, 2)));
void basic_mud_vlog(const char *format, va_list args);
int touch(const char *path);
void mudlog(int type, int level, int file, const char *str, ...) __attribute__ ((format (printf, 4, 5)));
int	rand_number(int from, int to);
int	dice(int number, int size);
size_t	sprintbit(bitvector_t vektor, const char *names[], char *result, size_t reslen);
size_t	sprinttype(int type, const char *names[], char *result, size_t reslen);
void sprintbitarray(int bitvector[], const char *names[], int maxar, char *result);
int get_line(FILE *fl, char *buf);
int get_filename(char *filename, size_t fbufsize, int mode, const char *orig_name);
time_t mud_time_to_secs(struct time_info_data *now);
struct time_info_data *age(struct char_data *ch);
int num_pc_in_room(struct room_data *room);
void core_dump_real(const char *who, int line);
int count_color_chars(char *string);
int room_is_dark(room_rnum room);
int levenshtein_distance(const char *s1, const char *s2);
struct time_info_data *real_time_passed(time_t t2, time_t t1);
struct time_info_data *mud_time_passed(time_t t2, time_t t1);
void prune_crlf(char *txt);
void column_list(struct char_data *ch, int num_cols, const char **list, int list_length, bool show_nums);
int get_flag_by_name(const char *flag_list[], char *flag_name);
int file_head( FILE *file, char *buf, size_t bufsize, int lines_to_read );
int file_tail( FILE *file, char *buf, size_t bufsize, int lines_to_read );
size_t file_sizeof( FILE *file );
int file_numlines( FILE *file );
IDXTYPE atoidx( const char *str_to_conv );
char *strfrmt(char *str, int w, int h, int justify, int hpad, int vpad);
char *strpaste(char *str1, char *str2, char *joiner);
void new_affect(struct affected_type *af);
int get_class_by_name(char *classname);
char * convert_from_tabs(char * string);
int count_non_protocol_chars(char * str);

/* Public functions made available form weather.c */
void weather_and_time(int mode);

/** Creates a core dump for diagnostic purposes, but will keep (if it can)
 * the mud running after the core has been dumped. Call this in the place
 * of calling core_dump_real. */
#define core_dump() core_dump_real(__FILE__, __LINE__)

/* Only provide our versions if one isn't in the C library. These macro names
 * will be defined by sysdep.h if a strcasecmp or stricmp exists. */
#ifndef str_cmp
int	str_cmp(const char *arg1, const char *arg2);
#endif
#ifndef strn_cmp
int	strn_cmp(const char *arg1, const char *arg2, int n);
#endif

/* random functions in random.c */
void circle_srandom(unsigned long initial_seed);
unsigned long circle_random(void);

/* undefine MAX and MIN so that our functions are used instead */
#ifdef MAX
#undef MAX
#endif

#ifdef MIN
#undef MIN
#endif

int MAX(int a, int b);
int MIN(int a, int b);
char *CAP(char *txt);

/* Followers */
int	num_followers_charmed(struct char_data *ch);
void	die_follower(struct char_data *ch);
void	add_follower(struct char_data *ch, struct char_data *leader);
void	stop_follower(struct char_data *ch);
bool	circle_follow(struct char_data *ch, struct char_data *victim);

/* in act.informative.c */
void	look_at_room(struct char_data *ch, int mode);
void  add_history(struct char_data *ch, char *msg, int type);

/* in act.movmement.c */
int	do_simple_move(struct char_data *ch, int dir, int following);
int	perform_move(struct char_data *ch, int dir, int following);

/* in limits.c */
int	mana_gain(struct char_data *ch);
int	hit_gain(struct char_data *ch);
int	move_gain(struct char_data *ch);
void	set_title(struct char_data *ch, char *title);
void	gain_exp(struct char_data *ch, int gain);
void	gain_exp_regardless(struct char_data *ch, int gain);
void	gain_condition(struct char_data *ch, int condition, int value);
void	point_update(void);
void	update_pos(struct char_data *victim);
void run_autowiz(void);
int increase_gold(struct char_data *ch, int amt);
int decrease_gold(struct char_data *ch, int amt);
int increase_bank(struct char_data *ch, int amt);
int decrease_bank(struct char_data *ch, int amt);

/* in class.c */
void    advance_level(struct char_data *ch);

void char_from_furniture(struct char_data *ch);
/** What ch is currently sitting on. */
#define SITTING(ch)             ((ch)->char_specials.furniture)
/** Who is sitting next to ch, if anyone. */
#define NEXT_SITTING(ch)        ((ch)->char_specials.next_in_furniture)
/** Who is sitting on this obj */
#define OBJ_SAT_IN_BY(obj)      ((obj)->sitting_here)

/* various constants */

/* defines for mudlog() */
#define OFF	0  /**< Receive no mudlog messages. */
#define BRF	1  /**< Receive only the most important mudlog messages. */
#define NRM	2  /**< Receive the standard mudlog messages. */
#define CMP	3  /**< Receive every mudlog message. */

/* get_filename() types of files to open */
#define CRASH_FILE       0 /**< Open up a player crash save file */
#define ETEXT_FILE       1 /**< ???? */
#define SCRIPT_VARS_FILE 2 /**< Reference to a global variable file. */
#define PLR_FILE         3 /**< The standard player file */

#define MAX_FILES        4 /**< Max number of files types vailable */

/* breadth-first searching for graph function (tracking, etc) */
#define BFS_ERROR		(-1)       /**< Error in the search. */
#define BFS_ALREADY_THERE	(-2) /**< Area traversed already. */
#define BFS_NO_PATH		(-3)     /**< No path through here. */

/** Number of real life seconds per mud hour.
 * @todo The definitions based on SECS_PER_MUD_HOUR should be configurable.
 * See act.informative.c and utils.c for other places to change. */
#define SECS_PER_MUD_HOUR	75
/** Real life seconds in one mud day.
 * Current calculation = 30 real life minutes. */
#define SECS_PER_MUD_DAY	(24*SECS_PER_MUD_HOUR)
/** Real life seconds per mud month.
 * Current calculation = 17.5 real life hours */
#define SECS_PER_MUD_MONTH	(35*SECS_PER_MUD_DAY)
/** Real life seconds per mud month.
 * Current calculation ~= 12.4 real life days */
#define SECS_PER_MUD_YEAR	(17*SECS_PER_MUD_MONTH)

/** The number of seconds in a real minute. */
#define SECS_PER_REAL_MIN	60
/** The number of seconds in a real hour. */
#define SECS_PER_REAL_HOUR	(60*SECS_PER_REAL_MIN)
/** The number of seconds in a real day. */
#define SECS_PER_REAL_DAY	(24*SECS_PER_REAL_HOUR)
/** The number of seconds in a real year. */
#define SECS_PER_REAL_YEAR	(365*SECS_PER_REAL_DAY)

/* integer utils */
#define URANGE(a, b, c)          ((b) < (a) ? (a) : ((b) > (c) ? (c) : (b)))

/* Various string utils. */
/** If a is not null, FALSE or '0', return "YES"; if it is, return "NO" */
#define YESNO(a) ((a) ? "YES" : "NO")
/** If a is not null, FALSE or '0', return "ON"; if it is, return "OFF" */
#define ONOFF(a) ((a) ? "ON" : "OFF")

/** If c is an upper case letter, return the upper case. */
#define LOWER(c)   (((c)>='A'  && (c) <= 'Z') ? ((c)+('a'-'A')) : (c))
/** If c is a lower case letter, return the upper case. */
#define UPPER(c)   (((c)>='a'  && (c) <= 'z') ? ((c)+('A'-'a')) : (c) )

/** If ch is equal to either a newline or a carriage return, return 1,
 * else 0.
 * @todo Recommend using the ? operator for clarity. */
#define ISNEWL(ch) ((ch) == '\n' || (ch) == '\r')

/** If string begins a vowel (upper or lower case), return "an"; else return
 * "a". */
#define AN(string) (strchr("aeiouAEIOU", *string) ? "an" : "a")

/** A calloc based memory allocation macro.
 * @param result Pointer to created memory.
 * @param type The type of memory (int, struct char_data, etc.).
 * @param number How many of type to make. */
#define CREATE(result, type, number)  do {\
	if ((number) * sizeof(type) <= 0)	\
		log("SYSERR: Zero bytes or less requested at %s:%d.", __FILE__, __LINE__);	\
	if (!((result) = (type *) calloc ((number), sizeof(type))))	\
		{ perror("SYSERR: malloc failure"); abort(); } } while(0)

/** A realloc based memory reallocation macro. Reminder: realloc can reduce
 * the size of an array as well as increase it.
 * @param result Pointer to created memory.
 * @param type The type of memory (int, struct char_data, etc.).
 * @param number How many of type to make. */
#define RECREATE(result, type, number) do {\
  if (!((result) = (type *) realloc ((result), sizeof(type) * (number))))\
		{ perror("SYSERR: realloc failure"); abort(); } } while(0)

/** Remove an item from a linked list and reset the links.
 * If item is at the list head, change the head, else traverse the
 * list looking for the item before the one to be removed.
 * @pre Requires that a variable 'temp' be declared as the same type as the
 * list to be manipulated.
 * @post List pointers are correctly reset and item is no longer in the list.
 * item can now be changed, removed, etc independently from the list it was in.
 * @param item Pointer to item to remove from the list.
 * @param head Pointer to the head of the linked list.
 * @param next The variable name pointing to the next in the list.
 * */
#define REMOVE_FROM_LIST(item, head, next)	\
   if ((item) == (head))		\
      head = (item)->next;		\
   else {				\
      temp = head;			\
      while (temp && (temp->next != (item))) \
	 temp = temp->next;		\
      if (temp)				\
         temp->next = (item)->next;	\
   }					\

/* Connect 'link' to the end of a double-linked list
 * The new item becomes the last in the linked list, and the last
 * pointer is updated.
 * @param link  Pointer to item to remove from the list.
 * @param first Pointer to the first item of the linked list.
 * @param last  Pointer to the last item of the linked list.
 * @param next  The variable name pointing to the next in the list.
 * @param prev  The variable name pointing to the previous in the list.
 * */
#define LINK(link, first, last, next, prev)                     \
do                                                              \
{                                                               \
    if ( !(first) )                                             \
      (first)                   = (link);                       \
    else                                                        \
      (last)->next              = (link);                       \
    (link)->next                = NULL;                         \
    (link)->prev                = (last);                       \
    (last)                      = (link);                       \
} while(0)

/* Remove 'link' from a double-linked list
 * @post  item is removed from the list, but remains in memory, and must
   be free'd after unlinking.
 * @param link  Pointer to item to remove from the list.
 * @param first Pointer to the first item of the linked list.
 * @param last  Pointer to the last item of the linked list.
 * @param next  The variable name pointing to the next in the list.
 * @param prev  The variable name pointing to the previous in the list.
 * */
#define UNLINK(link, first, last, next, prev)                   \
do                                                              \
{                                                               \
    if ( !(link)->prev )                                        \
      (first)                   = (link)->next;                 \
    else                                                        \
      (link)->prev->next        = (link)->next;                 \
    if ( !(link)->next )                                        \
      (last)                    = (link)->prev;                 \
    else                                                        \
      (link)->next->prev        = (link)->prev;                 \
} while(0)

/* Free a pointer, and log if it was NULL
 * @param point The pointer to be free'd.
 * */
#define DISPOSE(point)                                          \
do                                                              \
{                                                               \
  if (!(point))                                                 \
  {                                                             \
        log( "SYSERR: Freeing null pointer %s:%d", __FILE__, __LINE__ ); \
  }                                                             \
  else free(point);                                             \
  point = NULL;                                                 \
} while(0)

/* String Utils */
/* Allocate memory for a string, and return a pointer
 * @param point The string to be copied.
 * */
#define STRALLOC(point)         (strdup(point))
/* Free allocated memory for a string
 * @param point The string to be free'd.
 * */
#define STRFREE(point)          DISPOSE(point)

/* basic bitvector utils */
/** Return the bitarray field number x is in. */
#define Q_FIELD(x)  ((int) (x) / 32)
/** Return the bit to set in a bitarray field. */
#define Q_BIT(x)    (1 << ((x) % 32))
/** 1 if bit is set in the bitarray represented by var, 0 if not. */
#define IS_SET_AR(var, bit)       ((var)[Q_FIELD(bit)] & Q_BIT(bit))
/** Set a specific bit in the bitarray represented by var to 1. */
#define SET_BIT_AR(var, bit)      ((var)[Q_FIELD(bit)] |= Q_BIT(bit))
/** Unset a specific bit in the bitarray represented by var to 0. */
#define REMOVE_BIT_AR(var, bit)   ((var)[Q_FIELD(bit)] &= ~Q_BIT(bit))
/** If bit is on in bitarray var, turn it off; if it is off, turn it on. */
#define TOGGLE_BIT_AR(var, bit)   ((var)[Q_FIELD(bit)] = (var)[Q_FIELD(bit)] ^ Q_BIT(bit))

/* Older, stock tbaMUD bit settings. */
/** 1 if bit is set in flag, 0 if it is not set. */
#define IS_SET(flag,bit)  ((flag) & (bit))
/** Set a specific bit in var to 1. */
#define SET_BIT(var,bit)  ((var) |= (bit))
/** Set a specific bit bit in var to 0. */
#define REMOVE_BIT(var,bit)  ((var) &= ~(bit))
/** If bit in var is off, turn it on; if it is on, turn it off. */
#define TOGGLE_BIT(var,bit) ((var) ^= (bit))

/* Accessing player specific data structures on a mobile is a very bad thing
 * to do.  Consider that changing these variables for a single mob will change
 * it for every other single mob in the game.  If we didn't specifically check
 * for it, 'wimpy' would be an extremely bad thing for a mob to do, as an
 * example.  If you really couldn't care less, change this to a '#if 0'. */
#if 1
/** Warn if accessing player_specials on a mob.
 * @todo Subtle bug in the var reporting, but works well for now. */
#define CHECK_PLAYER_SPECIAL(ch, var) \
	(*(((ch)->player_specials == &dummy_mob) ? (log("SYSERR: Mob using '"#var"' at %s:%d.", __FILE__, __LINE__), &(var)) : &(var)))
#else
#define CHECK_PLAYER_SPECIAL(ch, var)	(var)
#endif

/** The act flags on a mob. Synonomous with PLR_FLAGS. */
#define MOB_FLAGS(ch)	((ch)->char_specials.saved.act)
/** Player flags on a PC. Synonomous with MOB_FLAGS. */
#define PLR_FLAGS(ch)	((ch)->char_specials.saved.act)
/** Preference flags on a player (not to be used on mobs). */
#define PRF_FLAGS(ch) CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.pref))
/** Affect flags on the NPC or PC. */
#define AFF_FLAGS(ch)	((ch)->char_specials.saved.affected_by)
/** Room flags.
 * @param loc The real room number. */
#define ROOM_FLAGS(loc)	(world[(loc)].room_flags)
/** Zone flags.
 * @param rnum The real zone number. */
#define ZONE_FLAGS(rnum)       (zone_table[(rnum)].zone_flags)
/** Zone minimum level restriction.
 * @param rnum The real zone number. */
#define ZONE_MINLVL(rnum)      (zone_table[(rnum)].min_level)
/** Zone maximum level restriction.
 * @param rnum The real zone number. */
#define ZONE_MAXLVL(rnum)      (zone_table[(rnum)].max_level)

/** References the routine element for a spell. Currently unused. */
#define SPELL_ROUTINES(spl)	(spell_info[spl].routines)

/* IS_MOB() acts as a VALID_MOB_RNUM()-like function.*/
/** 1 if the character has the NPC bit set, 0 if the character does not.
 * Used to prevents NPCs and mobs from doing things they shouldn't, even
 * when mobs are possessed or charmed by a player. */
#define IS_NPC(ch)	(IS_SET_AR(MOB_FLAGS(ch), MOB_ISNPC))
/** 1 if the character is a real NPC, 0 if the character is not. */
#define IS_MOB(ch)	(IS_NPC(ch) && GET_MOB_RNUM(ch) <= top_of_mobt && \
				GET_MOB_RNUM(ch) != NOBODY)

/** 1 if ch is flagged an NPC and flag is set in the act bitarray, 0 if not. */
#define MOB_FLAGGED(ch, flag) (IS_NPC(ch) && IS_SET_AR(MOB_FLAGS(ch), (flag)))
/** 1 if ch is not flagged an NPC and flag is set in the act bitarray, 0 if
 * not. */
#define PLR_FLAGGED(ch, flag) (!IS_NPC(ch) && IS_SET_AR(PLR_FLAGS(ch), (flag)))
/** 1 if flag is set in the affect bitarray, 0 if not. */
#define AFF_FLAGGED(ch, flag) (IS_SET_AR(AFF_FLAGS(ch), (flag)))
/** 1 if flag is set in the preferences bitarray, 0 if not. */
#define PRF_FLAGGED(ch, flag) (IS_SET_AR(PRF_FLAGS(ch), (flag)))
/** 1 if flag is set in the room of loc, 0 if not. */
#define ROOM_FLAGGED(loc, flag) (IS_SET_AR(ROOM_FLAGS(loc), (flag)))
/** 1 if flag is set in the zone of rnum, 0 if not. */
#define ZONE_FLAGGED(rnum, flag)   (IS_SET_AR(zone_table[(rnum)].zone_flags, (flag)))
/** 1 if flag is set in the exit, 0 if not. */
#define EXIT_FLAGGED(exit, flag) (IS_SET((exit)->exit_info, (flag)))
/** 1 if flag is set in the affects bitarray of obj, 0 if not. */
#define OBJAFF_FLAGGED(obj, flag) (IS_SET_AR(GET_OBJ_AFFECT(obj), (flag)))
/** 1 if flag is set in the element of obj value, 0 if not. */
#define OBJVAL_FLAGGED(obj, flag) (IS_SET(GET_OBJ_VAL((obj), 1), (flag)))
/** 1 if flag is set in the wear bits of obj, 0 if not. */
#define OBJWEAR_FLAGGED(obj, flag) (IS_SET_AR(GET_OBJ_WEAR(obj), (flag)))
/** 1 if flag is set in the extra bits of obj, 0 if not. */
#define OBJ_FLAGGED(obj, flag) (IS_SET_AR(GET_OBJ_EXTRA(obj), (flag)))

/** 1 if spl has a flag set in routines, 0 if not. */
#define HAS_SPELL_ROUTINE(spl, flag) (IS_SET(SPELL_ROUTINES(spl), (flag)))

/** IS_AFFECTED for backwards compatibility */
#define IS_AFFECTED(ch, skill) (AFF_FLAGGED((ch), (skill)))

/** Toggle flag in ch PLR_FLAGS' turns on if off, or off if on. */
#define PLR_TOG_CHK(ch,flag) ((TOGGLE_BIT_AR(PLR_FLAGS(ch), (flag))) & Q_BIT(flag))
/** Toggle flag in ch PRF_FLAGS; turns on if off, or off if on. */
#define PRF_TOG_CHK(ch,flag) ((TOGGLE_BIT_AR(PRF_FLAGS(ch), (flag))) & Q_BIT(flag))

/** Checks to see if a PC or NPC is dead. */
#define DEAD(ch) (PLR_FLAGGED((ch), PLR_NOTDEADYET) || MOB_FLAGGED((ch), MOB_NOTDEADYET))

/* room utils */

/** Return the sector type for the room. If there is no sector type, return
 * SECT_INSIDE. */
#define SECT(room)	(VALID_ROOM_RNUM(room) ? \
				world[(room)].sector_type : SECT_INSIDE)

/** Return the zone number for this room */
#define GET_ROOM_ZONE(room)	(VALID_ROOM_RNUM(room) ? world[(room)].zone : NOWHERE)

/** TRUE if the room has no light, FALSE if not. */
#define IS_DARK(room)	room_is_dark((room))
/** TRUE if the room has light, FALSE if not. */
#define IS_LIGHT(room)  (!IS_DARK(room))

/** 1 if this is a valid room number, 0 if not. */
#define VALID_ROOM_RNUM(rnum)	((rnum) != NOWHERE && (rnum) <= top_of_world)
/** The room number if this is a valid room, NOWHERE if it is not */
#define GET_ROOM_VNUM(rnum) \
	((room_vnum)(VALID_ROOM_RNUM(rnum) ? world[(rnum)].number : NOWHERE))
/** Pointer to the room function, NULL if there is not one. */
#define GET_ROOM_SPEC(room) \
	(VALID_ROOM_RNUM(room) ? world[(room)].func : NULL)

/* char utils */
/** What room is PC/NPC in? */
#define IN_ROOM(ch)	((ch)->in_room)
/** What room was PC/NPC previously in? */
#define GET_WAS_IN(ch)	((ch)->was_in_room)
/** How old is PC/NPC, at last recorded time? */
#define GET_AGE(ch)     (age(ch)->year)

/** Name of PC. */
#define GET_PC_NAME(ch)	((ch)->player.name)
/** Name of PC or short_descr of NPC. */
#define GET_NAME(ch)    (IS_NPC(ch) ? \
			 (ch)->player.short_descr : GET_PC_NAME(ch))
/** Title of PC */
#define GET_TITLE(ch)   ((ch)->player.title)
/** Level of PC or NPC. */
#define GET_LEVEL(ch)   ((ch)->player.level)
/** Password of PC. */
#define GET_PASSWD(ch)	((ch)->player.passwd)
/** The player file position of PC. */
#define GET_PFILEPOS(ch)((ch)->pfilepos)

/** Gets the level of a player even if the player is switched.
 * @todo Make this the definition of GET_LEVEL. */
#define GET_REAL_LEVEL(ch) \
   (ch->desc && ch->desc->original ? GET_LEVEL(ch->desc->original) : \
    GET_LEVEL(ch))

/** Class of ch. */
#define GET_CLASS(ch)   ((ch)->player.chclass)
/** Height of ch. */
#define GET_HEIGHT(ch)	((ch)->player.height)
/** Weight of ch. */
#define GET_WEIGHT(ch)	((ch)->player.weight)
/** Sex of ch. */
#define GET_SEX(ch)	((ch)->player.sex)

/** Current strength of ch. */
#define GET_STR(ch)     ((ch)->aff_abils.str)
/** Current strength modifer of ch. */
#define GET_ADD(ch)     ((ch)->aff_abils.str_add)
/** Current dexterity of ch. */
#define GET_DEX(ch)     ((ch)->aff_abils.dex)
/** Current intelligence of ch. */
#define GET_INT(ch)     ((ch)->aff_abils.intel)
/** Current wisdom of ch. */
#define GET_WIS(ch)     ((ch)->aff_abils.wis)
/** Current constitution of ch. */
#define GET_CON(ch)     ((ch)->aff_abils.con)
/** Current charisma of ch. */
#define GET_CHA(ch)     ((ch)->aff_abils.cha)

/** Experience points of ch. */
#define GET_EXP(ch)	  ((ch)->points.exp)
/** Armor class of ch. */
#define GET_AC(ch)        ((ch)->points.armor)
/** Current hit points (health) of ch. */
#define GET_HIT(ch)	  ((ch)->points.hit)
/** Maximum hit points of ch. */
#define GET_MAX_HIT(ch)	  ((ch)->points.max_hit)
/** Current move points (stamina) of ch. */
#define GET_MOVE(ch)	  ((ch)->points.move)
/** Maximum move points (stamina) of ch. */
#define GET_MAX_MOVE(ch)  ((ch)->points.max_move)
/** Current mana points (magic) of ch. */
#define GET_MANA(ch)	  ((ch)->points.mana)
/** Maximum mana points (magic) of ch. */
#define GET_MAX_MANA(ch)  ((ch)->points.max_mana)
/** Gold on ch. */
#define GET_GOLD(ch)	  ((ch)->points.gold)
/** Gold in bank of ch. */
#define GET_BANK_GOLD(ch) ((ch)->points.bank_gold)
/** Current to-hit roll modifier for ch. */
#define GET_HITROLL(ch)	  ((ch)->points.hitroll)
/** Current damage roll modifier for ch. */
#define GET_DAMROLL(ch)   ((ch)->points.damroll)

/** Current position (standing, sitting) of ch. */
#define GET_POS(ch)	  ((ch)->char_specials.position)
/** Unique ID of ch. */
#define GET_IDNUM(ch)	  ((ch)->char_specials.saved.idnum)
/** Returns contents of id field from x. */
/** Warning: GET_ID is deprecated and you should use char_script_id, obj_script_id, room_script_id */
/** #define GET_ID(x)         ((x)->id) */
/** Weight carried by ch. */
#define IS_CARRYING_W(ch) ((ch)->char_specials.carry_weight)
/** Number of items carried by ch. */
#define IS_CARRYING_N(ch) ((ch)->char_specials.carry_items)
/** Who or what ch is fighting. */
#define FIGHTING(ch)	  ((ch)->char_specials.fighting)
/** Who or what the ch is hunting. */
#define HUNTING(ch)	  ((ch)->char_specials.hunting)
/** Saving throw i for character ch. */
#define GET_SAVE(ch, i)	  ((ch)->char_specials.saved.apply_saving_throw[i])
/** Alignment value for ch. */
#define GET_ALIGNMENT(ch) ((ch)->char_specials.saved.alignment)

/** Return condition i (DRUNK, HUNGER, THIRST) of ch. */
#define GET_COND(ch, i)		CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.conditions[(i)]))
/** The room to load player ch into. */
#define GET_LOADROOM(ch)	CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.load_room))
/** Number of skill practice sessions remaining for ch. */
#define GET_PRACTICES(ch)	CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.spells_to_learn))
/** Current invisibility level of ch. */
#define GET_INVIS_LEV(ch)	CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.invis_level))
/** Current wimpy level of ch. */
#define GET_WIMP_LEV(ch)	CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.wimp_level))
/** Current freeze level (god command) inflicted upon ch. */
#define GET_FREEZE_LEV(ch)	CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.freeze_level))
/** Current number of bad password attempts at logon. */
#define GET_BAD_PWS(ch)		CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.bad_pws))
/** Not used?
 * @deprecated Should not be used, as the talks field has been removed. */
#define GET_TALK(ch, i)		CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.talks[i]))
/** The poofin string for the ch. */
#define POOFIN(ch)		CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->poofin))
/** The poofout string for the ch. */
#define POOFOUT(ch)		CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->poofout))
/** The OLC zoon permission for ch.
 * @deprecated Currently unused? */
#define GET_OLC_ZONE(ch)	CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.olc_zone))
/** Currently unused?
 * @deprecated Currently unused? */
#define GET_LAST_OLC_TARG(ch)	CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->last_olc_targ))
/** Currently unused?
 * @deprecated Currently unused? */
#define GET_LAST_OLC_MODE(ch)	CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->last_olc_mode))
/** Retrieve command aliases for ch. */
#define GET_ALIASES(ch)		CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->aliases))
/** Who ch last spoke to with the 'tell' command. */
#define GET_LAST_TELL(ch)	CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->last_tell))
/** Get unique session id for ch. */
#define GET_PREF(ch)      ((ch)->pref)
/** Get host name or ip of ch. */
#define GET_HOST(ch)		CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->host))
#define GET_LAST_MOTD(ch)       CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.lastmotd))
#define GET_LAST_NEWS(ch)       CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.lastnews))
/** Get channel history i for ch. */
#define GET_HISTORY(ch, i)      CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.comm_hist[i]))
/** Return the page length (height) for ch. */
#define GET_PAGE_LENGTH(ch)     CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.page_length))
/** Return the page width for ch */
#define GET_SCREEN_WIDTH(ch)    CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.screen_width))
/* Autoquests data */
/** Return the number of questpoints ch has. */
#define GET_QUESTPOINTS(ch)     CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.questpoints))
/** Return the current quest that a player has assigned */
#define GET_QUEST(ch)           CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.current_quest))
/** Number of goals completed for this quest. */
#define GET_QUEST_COUNTER(ch)   CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.quest_counter))
/** Time remaining to complete the quest ch is currently on. */
#define GET_QUEST_TIME(ch)      CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.quest_time))
/** The number of quests completed by ch. */
#define GET_NUM_QUESTS(ch)      CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.num_completed_quests))
/** The type of quest ch is currently participating in. */
#define GET_QUEST_TYPE(ch)      (real_quest(GET_QUEST((ch))) != NOTHING ? aquest_table[real_quest(GET_QUEST((ch)))].type : AQ_UNDEFINED )

/** The current skill level of ch for skill i. */
#define GET_SKILL(ch, i)	CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.skills[i]))
/** Copy the current skill level i of ch to pct. */
#define SET_SKILL(ch, i, pct)	do { CHECK_PLAYER_SPECIAL((ch), (ch)->player_specials->saved.skills[i]) = pct; } while(0)

/** The player's default sector type when buildwalking */
#define GET_BUILDWALK_SECTOR(ch) CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->buildwalk_sector))

/** Get obj worn in position i on ch. */
#define GET_EQ(ch, i)		((ch)->equipment[i])

/** If ch is a mob, return the special function, else return NULL. */
#define GET_MOB_SPEC(ch)	(IS_MOB(ch) ? mob_index[(ch)->nr].func : NULL)
/** Get the real number of the mob instance. */
#define GET_MOB_RNUM(mob)	((mob)->nr)
/** If mob is a mob, return the virtual number of it. */
#define GET_MOB_VNUM(mob)	(IS_MOB(mob) ? \
				 mob_index[GET_MOB_RNUM(mob)].vnum : NOBODY)

/** Return the default position of ch. */
#define GET_DEFAULT_POS(ch)	((ch)->mob_specials.default_pos)
/** Return the memory of ch. */
#define MEMORY(ch)		((ch)->mob_specials.memory)

/** Return the equivalent strength of ch if ch has level 18 strength. */
#define STRENGTH_APPLY_INDEX(ch) \
        ( ((GET_ADD(ch) ==0) || (GET_STR(ch) != 18)) ? GET_STR(ch) :\
          (GET_ADD(ch) <= 50) ? 26 :( \
          (GET_ADD(ch) <= 75) ? 27 :( \
          (GET_ADD(ch) <= 90) ? 28 :( \
          (GET_ADD(ch) <= 99) ? 29 :  30 ) ) )                   \
        )

/** Return how much weight ch can carry. */
#define CAN_CARRY_W(ch) (str_app[STRENGTH_APPLY_INDEX(ch)].carry_w)
/** Return how many items ch can carry. */
#define CAN_CARRY_N(ch) (5 + (GET_DEX(ch) >> 1) + (GET_LEVEL(ch) >> 1))
/** Return whether or not ch is awake. */
#define AWAKE(ch) (GET_POS(ch) > POS_SLEEPING)
/** Defines if ch can see in general in the dark. */
#define CAN_SEE_IN_DARK(ch) \
   (AFF_FLAGGED(ch, AFF_INFRAVISION) || (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_HOLYLIGHT)))

/** Defines if ch is good. */
#define IS_GOOD(ch)    (GET_ALIGNMENT(ch) >= 350)
/** Defines if ch is evil. */
#define IS_EVIL(ch)    (GET_ALIGNMENT(ch) <= -350)
/** Defines if ch is neither good nor evil. */
#define IS_NEUTRAL(ch) (!IS_GOOD(ch) && !IS_EVIL(ch))

/** Old wait state function.
 * @deprecated Use GET_WAIT_STATE */
#define WAIT_STATE(ch, cycle) do { GET_WAIT_STATE(ch) = (cycle); } while(0)
/** Old check wait.
 * @deprecated Use GET_WAIT_STATE */
#define CHECK_WAIT(ch)                ((ch)->wait > 0)
/** Old mob wait check.
 * @deprecated Use GET_WAIT_STATE */
#define GET_MOB_WAIT(ch)      GET_WAIT_STATE(ch)
/** Use this macro to check the wait state of ch. */
#define GET_WAIT_STATE(ch)    ((ch)->wait)

/* Descriptor-based utils. */
/** Connected state of d. */
#define STATE(d)	((d)->connected)

/** Defines whether d is using an OLC or not. */
#define IS_IN_OLC(d)   ((STATE(d) >= FIRST_OLC_STATE) && (STATE(d) <= LAST_OLC_STATE))

/** Defines whether d is playing or not. */
#define IS_PLAYING(d)   (IS_IN_OLC(d) || STATE(d) == CON_PLAYING)

/** Defines if it is ok to send a message to ch. */
#define SENDOK(ch)	(((ch)->desc || SCRIPT_CHECK((ch), MTRIG_ACT)) && \
			(to_sleeping || AWAKE(ch)) && \
			!PLR_FLAGGED((ch), PLR_WRITING))

/* object utils */
/** Check for NOWHERE or the top array index? If using unsigned types, the top
 * array index will catch everything. If using signed types, NOTHING will
 * catch the majority of bad accesses. */
#define VALID_OBJ_RNUM(obj)	(GET_OBJ_RNUM(obj) <= top_of_objt && \
				 GET_OBJ_RNUM(obj) != NOTHING)

/** Level of obj. */
#define GET_OBJ_LEVEL(obj)      ((obj)->obj_flags.level)
/** Type of obj. */
#define GET_OBJ_TYPE(obj)	((obj)->obj_flags.type_flag)
/** Cost of obj. */
#define GET_OBJ_COST(obj)	((obj)->obj_flags.cost)
/** Cost per day to rent obj, if rent is turned on. */
#define GET_OBJ_RENT(obj)	((obj)->obj_flags.cost_per_day)
/** Affect flags on obj. */
#define GET_OBJ_AFFECT(obj)	((obj)->obj_flags.bitvector)
/** Extra flags bit array on obj. */
#define GET_OBJ_EXTRA(obj)	((obj)->obj_flags.extra_flags)
/** Extra flags field bit array field i on obj. */
#define GET_OBJ_EXTRA_AR(obj, i)   ((obj)->obj_flags.extra_flags[(i)])
/** Wear flags on obj. */
#define GET_OBJ_WEAR(obj)	((obj)->obj_flags.wear_flags)
/** Return value val for obj. */
#define GET_OBJ_VAL(obj, val)	((obj)->obj_flags.value[(val)])
/** Weight of obj. */
#define GET_OBJ_WEIGHT(obj)	((obj)->obj_flags.weight)
/** Current timer of obj. */
#define GET_OBJ_TIMER(obj)	((obj)->obj_flags.timer)
/** Real number of obj instance. */
#define GET_OBJ_RNUM(obj)	((obj)->item_number)
/** Virtual number of obj, or NOTHING if not a real obj. */
#define GET_OBJ_VNUM(obj)	(VALID_OBJ_RNUM(obj) ? \
				obj_index[GET_OBJ_RNUM(obj)].vnum : NOTHING)
/** Special function attached to obj, or NULL if nothing attached. */
#define GET_OBJ_SPEC(obj)	(VALID_OBJ_RNUM(obj) ? \
				obj_index[GET_OBJ_RNUM(obj)].func : NULL)

/** Defines if an obj is a corpse. */
#define IS_CORPSE(obj)		(GET_OBJ_TYPE(obj) == ITEM_CONTAINER && \
					GET_OBJ_VAL((obj), 3) == 1)

/** Can the obj be worn on body part? */
#define CAN_WEAR(obj, part)	OBJWEAR_FLAGGED((obj), (part))
/** Return short description of obj. */
#define GET_OBJ_SHORT(obj)      ((obj)->short_description)

/* Compound utilities and other macros. */
/** Used to compute version. To see if the code running is newer than 3.0pl13,
 * you would use: if _CIRCLEMUD > CIRCLEMUD_VERSION(3,0,13) */
#define CIRCLEMUD_VERSION(major, minor, patchlevel) \
       (((major) << 16) + ((minor) << 8) + (patchlevel))

/** Figures out possessive pronoun for ch. */
#define HSHR(ch) (GET_SEX(ch) ? (GET_SEX(ch)==SEX_MALE ? "his":"her") :"its")
/** Figures out third person, singular pronoun for ch. */
#define HSSH(ch) (GET_SEX(ch) ? (GET_SEX(ch)==SEX_MALE ? "he" :"she") : "it")
/** Figures out third person, objective pronoun for ch. */
#define HMHR(ch) (GET_SEX(ch) ? (GET_SEX(ch)==SEX_MALE ? "him":"her") : "it")

/** "An" or "A" for object (uppercased) */
#define ANA(obj) (strchr("aeiouAEIOU", *(obj)->name) ? "An" : "A")
/** "an" or "a" for object (lowercased) */
#define SANA(obj) (strchr("aeiouAEIOU", *(obj)->name) ? "an" : "a")
/** "an" or "a" for text (lowercased) */
#define TANA(obj) (strchr("aeiouAEIOU", *(obj)) ? "an" : "a")

/* Various macros building up to CAN_SEE */

/** Defines if there is enough light for sub to see in. */
#define LIGHT_OK(sub)	(!AFF_FLAGGED(sub, AFF_BLIND) && \
   (IS_LIGHT(IN_ROOM(sub)) || AFF_FLAGGED((sub), AFF_INFRAVISION) || \
   GET_LEVEL(sub) >= LVL_IMMORT))

/** Defines if sub character can see the invisible obj character. */
#define INVIS_OK(sub, obj) \
 ((!AFF_FLAGGED((obj),AFF_INVISIBLE) || AFF_FLAGGED(sub,AFF_DETECT_INVIS)) && \
 (!AFF_FLAGGED((obj), AFF_HIDE) || AFF_FLAGGED(sub, AFF_SENSE_LIFE)))

/** Defines if sub character can see obj character, assuming mortal only
 * settings. */
#define MORT_CAN_SEE(sub, obj) (LIGHT_OK(sub) && INVIS_OK(sub, obj))

/** Defines if sub character can see obj character, assuming immortal
 * and mortal settings. */
#define IMM_CAN_SEE(sub, obj) \
   (MORT_CAN_SEE(sub, obj) || (!IS_NPC(sub) && PRF_FLAGGED(sub, PRF_HOLYLIGHT)))

/** Is obj character the same as sub character? */
#define SELF(sub, obj)  ((sub) == (obj))

/** Can sub character see obj character? */
#define CAN_SEE(sub, obj) (SELF(sub, obj) || \
   ((GET_REAL_LEVEL(sub) >= (IS_NPC(obj) ? 0 : GET_INVIS_LEV(obj))) && \
   IMM_CAN_SEE(sub, obj)))
/* End of CAN_SEE */

/** Can the sub character see the obj if it is invisible? */
#define INVIS_OK_OBJ(sub, obj) \
  (!OBJ_FLAGGED((obj), ITEM_INVISIBLE) || AFF_FLAGGED((sub), AFF_DETECT_INVIS))

/** Is anyone carrying this object and if so, are they visible? */
#define CAN_SEE_OBJ_CARRIER(sub, obj) \
  ((!obj->carried_by || CAN_SEE(sub, obj->carried_by)) &&	\
   (!obj->worn_by || CAN_SEE(sub, obj->worn_by)))

/** Can sub character see the obj, using mortal only checks? */
#define MORT_CAN_SEE_OBJ(sub, obj) \
  (LIGHT_OK(sub) && INVIS_OK_OBJ(sub, obj) && CAN_SEE_OBJ_CARRIER(sub, obj))

/** Can sub character see the obj, using mortal and immortal checks? */
#define CAN_SEE_OBJ(sub, obj) \
   (MORT_CAN_SEE_OBJ(sub, obj) || (!IS_NPC(sub) && PRF_FLAGGED((sub), PRF_HOLYLIGHT)))

/** Can ch carry obj? */
#define CAN_CARRY_OBJ(ch,obj)  \
   (((IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(obj)) <= CAN_CARRY_W(ch)) &&   \
    ((IS_CARRYING_N(ch) + 1) <= CAN_CARRY_N(ch)))

/** Can ch pick up obj? */
#define CAN_GET_OBJ(ch, obj)   \
   (CAN_WEAR((obj), ITEM_WEAR_TAKE) && CAN_CARRY_OBJ((ch),(obj)) && \
    CAN_SEE_OBJ((ch),(obj)))

/** If vict can see ch, return ch name, else return "someone". */
#define PERS(ch, vict)   (CAN_SEE(vict, ch) ? GET_NAME(ch) : (GET_LEVEL(ch) > LVL_IMMORT ? "an immortal" : "someone"))

/** If vict can see obj, return obj short description, else return
 * "something". */
#define OBJS(obj, vict) (CAN_SEE_OBJ((vict), (obj)) ? \
	(obj)->short_description  : "something")

/** If vict can see obj, return obj name, else return "something". */
#define OBJN(obj, vict) (CAN_SEE_OBJ((vict), (obj)) ? \
	fname((obj)->name) : "something")

/** Does direction door exist in the same room as ch? */
#define EXIT(ch, door)  (world[IN_ROOM(ch)].dir_option[door])
/** Does room number have direction num? */
#define W_EXIT(room, num)     (world[(room)].dir_option[(num)])
/** Does room pointer have direction option num? */
#define R_EXIT(room, num)     ((room)->dir_option[(num)])

#define _2ND_EXIT(ch, door) (world[EXIT(ch, door)->to_room].dir_option[door])
#define _3RD_EXIT(ch, door) (world[_2ND_EXIT(ch, door)->to_room].dir_option[door])

/** Can ch walk through direction door. */
#define CAN_GO(ch, door) (EXIT(ch,door) && \
			 (EXIT(ch,door)->to_room != NOWHERE) && \
			 !IS_SET(EXIT(ch, door)->exit_info, EX_CLOSED))

/** True total number of directions available to move in. */
#define DIR_COUNT ((CONFIG_DIAGONAL_DIRS) ? 10 : 6)
 
/* Returns TRUE if the direction is a diagonal one */
#define IS_DIAGONAL(dir) (((dir) == NORTHWEST) || ((dir) == NORTHEAST) || \
		((dir) == SOUTHEAST) || ((dir) == SOUTHWEST) )

/** Return the class abbreviation for ch. */
#define CLASS_ABBR(ch) (IS_NPC(ch) ? "--" : class_abbrevs[(int)GET_CLASS(ch)])

/** 1 if ch is magic user class, 0 if not. */
#define IS_MAGIC_USER(ch)	(!IS_NPC(ch) && \
        (GET_CLASS(ch) == CLASS_MAGIC_USER))
/** 1 if ch is cleric class, 0 if not. */
#define IS_CLERIC(ch)		(!IS_NPC(ch) && \
        (GET_CLASS(ch) == CLASS_CLERIC))
/** 1 if ch is thief class, 0 if not. */
#define IS_THIEF(ch)		(!IS_NPC(ch) && \
        (GET_CLASS(ch) == CLASS_THIEF))
/** 1 if ch is warrior class, 0 if not. */
#define IS_WARRIOR(ch)		(!IS_NPC(ch) && \
        (GET_CLASS(ch) == CLASS_WARRIOR))

/** Defines if ch is outdoors or not. */
#define OUTSIDE(ch) (!ROOM_FLAGGED(IN_ROOM(ch), ROOM_INDOORS))

/* Group related defines */
#define GROUP(ch)            (ch->group)
#define GROUP_LEADER(group)  (group->leader)
#define GROUP_FLAGS(group)   (group->group_flags)

/* Happy-hour defines */
#define IS_HAPPYQP   (happy_data.qp_rate > 0)
#define IS_HAPPYEXP  (happy_data.exp_rate > 0)
#define IS_HAPPYGOLD (happy_data.gold_rate > 0)

#define HAPPY_EXP    happy_data.exp_rate
#define HAPPY_GOLD   happy_data.gold_rate
#define HAPPY_QP     happy_data.qp_rate

#define HAPPY_TIME   happy_data.ticks_left

#define IS_HAPPYHOUR ((IS_HAPPYEXP || IS_HAPPYGOLD || IS_HAPPYQP) && (HAPPY_TIME > 0))

/* OS compatibility */
#ifndef NULL
/** Just in case NULL is not defined. */
#define NULL (void *)0
#endif

#if !defined(FALSE)
/** Just in case FALSE is not defined. */
#define FALSE 0
#endif

#if !defined(TRUE)
/** Just in case TRUE is not defined. */
#define TRUE  (!FALSE)
#endif

#if !defined(YES)
/** In case YES is not defined. */
#define YES 1
#endif

#if !defined(NO)
/** In case NO is not defined. */
#define NO 0
#endif

/* defines for fseek */
#ifndef SEEK_SET
/** define for fseek */
#define SEEK_SET	0
/** define for fseek */
#define SEEK_CUR	1
/** define for fseek */
#define SEEK_END	2
#endif

/* NOCRYPT can be defined by an implementor manually in sysdep.h. CIRCLE_CRYPT
 * is a variable that the 'configure' script automatically sets when it
 * determines whether or not the system is capable of encrypting. */
#if defined(NOCRYPT) || !defined(CIRCLE_CRYPT)
/** When crypt is not defined. (NOTE: Player passwords will be plain text.) */
#define CRYPT(a,b) (a)
#else
/** When crypt is defined. Player passwords stored encrypted. */
#define CRYPT(a,b) ((char *) crypt((a),(b)))
#endif

/* Config macros */

/** Pointer to the config file. */
#define CONFIG_CONFFILE         config_info.CONFFILE

/** Player killing allowed or not? */
#define CONFIG_PK_ALLOWED       config_info.play.pk_allowed
/** Player thieving allowed or not? */
#define CONFIG_PT_ALLOWED       config_info.play.pt_allowed
/** What level to use the shout command? */
#define CONFIG_LEVEL_CAN_SHOUT  config_info.play.level_can_shout
/** How many move points does holler cost? */
#define CONFIG_HOLLER_MOVE_COST config_info.play.holler_move_cost
/** How many characters can fit in a room marked as tunnel? */
#define CONFIG_TUNNEL_SIZE      config_info.play.tunnel_size
/** What is the max experience that can be gained at once? */
#define CONFIG_MAX_EXP_GAIN     config_info.play.max_exp_gain
/** What is the max experience that can be lost at once? */
#define CONFIG_MAX_EXP_LOSS     config_info.play.max_exp_loss
/** How long will npc corpses last before decomposing? */
#define CONFIG_MAX_NPC_CORPSE_TIME config_info.play.max_npc_corpse_time
/** How long will pc corpses last before decomposing? */
#define CONFIG_MAX_PC_CORPSE_TIME  config_info.play.max_pc_corpse_time
/** How long can a pc be idled before being pulled into the void? */
#define CONFIG_IDLE_VOID        config_info.play.idle_void
/** How long until the idle pc is force rented? */
#define CONFIG_IDLE_RENT_TIME   config_info.play.idle_rent_time
/** What level and above is immune to idle outs? */
#define CONFIG_IDLE_MAX_LEVEL   config_info.play.idle_max_level
/** Are death traps dumps? */
#define CONFIG_DTS_ARE_DUMPS    config_info.play.dts_are_dumps
/** Should items crated with the load command be placed on ground or
 * in the creator's inventory? */
#define CONFIG_LOAD_INVENTORY   config_info.play.load_into_inventory
/** Get the track through doors setting. */
#define CONFIG_TRACK_T_DOORS    config_info.play.track_through_doors
/** Get the permission to level up from mortal to immortal. */
#define CONFIG_NO_MORT_TO_IMMORT config_info.play.no_mort_to_immort
/** Get the OK message. */
#define CONFIG_OK               config_info.play.OK
/** Get the HUH message. */
#define CONFIG_HUH              config_info.play.HUH
/** Get the NOPERSON message. */
#define CONFIG_NOPERSON         config_info.play.NOPERSON
/** Get the NOEFFECT message. */
#define CONFIG_NOEFFECT         config_info.play.NOEFFECT
/** Get the display closed doors setting. */
#define CONFIG_DISP_CLOSED_DOORS config_info.play.disp_closed_doors
/** Get the diagonal directions setting. */
#define CONFIG_DIAGONAL_DIRS    config_info.play.diagonal_dirs

/* Map/Automap options */
#define CONFIG_MAP             config_info.play.map_option
#define CONFIG_MAP_SIZE        config_info.play.map_size
#define CONFIG_MINIMAP_SIZE    config_info.play.minimap_size

/* DG Script Options */
#define CONFIG_SCRIPT_PLAYERS  config_info.play.script_players

/* Crash Saves */
/** Get free rent setting. */
#define CONFIG_FREE_RENT        config_info.csd.free_rent
/** Get max number of objects to save. */
#define CONFIG_MAX_OBJ_SAVE     config_info.csd.max_obj_save
/** Get minimum cost to rent. */
#define CONFIG_MIN_RENT_COST    config_info.csd.min_rent_cost
/** Get the auto save setting. */
#define CONFIG_AUTO_SAVE        config_info.csd.auto_save
/** Get the auto save frequency. */
#define CONFIG_AUTOSAVE_TIME    config_info.csd.autosave_time
/** Get the length of time to hold crash files. */
#define CONFIG_CRASH_TIMEOUT    config_info.csd.crash_file_timeout
/** Get legnth of time to hold rent files. */
#define CONFIG_RENT_TIMEOUT     config_info.csd.rent_file_timeout

/* Room Numbers */
/** Get the mortal start room. */
#define CONFIG_MORTAL_START     config_info.room_nums.mortal_start_room
/** Get the immortal start room. */
#define CONFIG_IMMORTAL_START   config_info.room_nums.immort_start_room
/** Get the frozen character start room. */
#define CONFIG_FROZEN_START     config_info.room_nums.frozen_start_room
/** Get the 1st donation room. */
#define CONFIG_DON_ROOM_1       config_info.room_nums.donation_room_1
/** Get the second donation room. */
#define CONFIG_DON_ROOM_2       config_info.room_nums.donation_room_2
/** Ge the third dontation room. */
#define CONFIG_DON_ROOM_3       config_info.room_nums.donation_room_3

/* Game Operation */
/** Get the default mud connection port. */
#define CONFIG_DFLT_PORT        config_info.operation.DFLT_PORT
/** Get the default mud ip address. */
#define CONFIG_DFLT_IP          config_info.operation.DFLT_IP
/** Get the max number of players allowed. */
#define CONFIG_MAX_PLAYING      config_info.operation.max_playing
/** Get the max filesize allowed. */
#define CONFIG_MAX_FILESIZE     config_info.operation.max_filesize
/** Get the max bad password attempts. */
#define CONFIG_MAX_BAD_PWS      config_info.operation.max_bad_pws
/** Get the siteok setting. */
#define CONFIG_SITEOK_ALL       config_info.operation.siteok_everyone
/** Get the auto-save-to-disk settings for OLC. */
#define CONFIG_OLC_SAVE         config_info.operation.auto_save_olc
/** Get the ability to use aedit or not. */
#define CONFIG_NEW_SOCIALS      config_info.operation.use_new_socials
/** Get the setting to resolve IPs or not. */
#define CONFIG_NS_IS_SLOW       config_info.operation.nameserver_is_slow
/** Default data directory. */
#define CONFIG_DFLT_DIR         config_info.operation.DFLT_DIR
/** Where is the default log file? */
#define CONFIG_LOGNAME          config_info.operation.LOGNAME
/** Get the text displayed in the opening menu. */
#define CONFIG_MENU             config_info.operation.MENU
/** Get the standard welcome message. */
#define CONFIG_WELC_MESSG       config_info.operation.WELC_MESSG
/** Get the standard new character message. */
#define CONFIG_START_MESSG      config_info.operation.START_MESSG
/** Should medit show the advnaced stats menu? */
#define CONFIG_MEDIT_ADVANCED   config_info.operation.medit_advanced
/** Does "bug resolve" autosave ? */
#define CONFIG_IBT_AUTOSAVE config_info.operation.ibt_autosave
/** Use the protocol negotiation system? */
#define CONFIG_PROTOCOL_NEGOTIATION config_info.operation.protocol_negotiation
/** Use the special character in comm channels? */
#define CONFIG_SPECIAL_IN_COMM config_info.operation.special_in_comm
/** Activate debug mode? */
#define CONFIG_DEBUG_MODE config_info.operation.debug_mode

/* Autowiz */
/** Use autowiz or not? */
#define CONFIG_USE_AUTOWIZ      config_info.autowiz.use_autowiz
/** What is the minimum level character to put on the wizlist? */
#define CONFIG_MIN_WIZLIST_LEV  config_info.autowiz.min_wizlist_lev

#endif /* _UTILS_H_ */
