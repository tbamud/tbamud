/**
* @file utils.c
* Various utility functions used within the core mud code.
*
* Part of the core tbaMUD source code distribution, which is a derivative
* of, and continuation of, CircleMUD.
*
* All rights reserved.  See license for complete information.
* Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University
* CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.
*/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "db.h"
#include "comm.h"
#include "modify.h"
#include "screen.h"
#include "spells.h"
#include "handler.h"
#include "interpreter.h"
#include "class.h"


/** Aportable random number function.
 * @param from The lower bounds of the random number.
 * @param to The upper bounds of the random number. */
int rand_number(int from, int to)
{
  /* error checking in case people call this incorrectly */
  if (from > to) {
    int tmp = from;
    from = to;
    to = tmp;
    log("SYSERR: rand_number() should be called with lowest, then highest. (%d, %d), not (%d, %d).", from, to, to, from);
  }

  /* This should always be of the form: ((float)(to - from + 1) * rand() /
   * (float)(RAND_MAX + from) + from); If you are using rand() due to historical
   * non-randomness of the lower bits in older implementations.  We always use
   * circle_random() though, which shouldn't have that problem. Mean and
   * standard deviation of both are identical (within the realm of statistical
   * identity) if the rand() implementation is non-broken. */
  return ((circle_random() % (to - from + 1)) + from);
}

/** Simulates a single dice roll from one to many of a certain sized die.
 * @param num The number of dice to roll.
 * @param size The number of sides each die has, and hence the number range
 * of the die. */
int dice(int num, int size)
{
  int sum = 0;

  if (size <= 0 || num <= 0)
    return (0);

  while (num-- > 0)
    sum += rand_number(1, size);

  return (sum);
}

/** Return the smaller number. Original note: Be wary of sign issues with this.
 * @param a The first number.
 * @param b The second number. */
int MIN(int a, int b)
{
  return (a < b ? a : b);
}

/** Return the larger number. Original note: Be wary of sign issues with this.
 * @param a The first number.
 * @param b The second number. */
int MAX(int a, int b)
{
  return (a > b ? a : b);
}

/** Used to capitalize a string. Will not change any mud specific color codes.
 * @param txt The string to capitalize. */
char *CAP(char *txt)
{
  char *p = txt;

  /* Skip all preceeding color codes and ANSI codes */
  while ((*p == '\t' && *(p+1)) || (*p == '\x1B' && *(p+1) == '[')) {
    if (*p == '\t') p += 2;  /* Skip \t sign and color letter/number */
    else {
      p += 2;                          /* Skip the CSI section of the ANSI code */
      while (*p && !isalpha(*p)) p++;  /* Skip until a 'letter' is found */
      if (*p) p++;                     /* Skip the letter */
    }
  }

  if (*p)
    *p = UPPER(*p);
  return (txt);
}

#if !defined(HAVE_STRLCPY)
/** A 'strlcpy' function in the same fashion as 'strdup' below. This copies up
 * to totalsize - 1 bytes from the source string, placing them and a trailing
 * NUL into the destination string. Returns the total length of the string it
 * tried to copy, not including the trailing NUL.  So a '>= totalsize' test
 * says it was truncated. (Note that you may have _expected_ truncation
 * because you only wanted a few characters from the source string.) Portable
 * function, in case your system does not have strlcpy. */
size_t strlcpy(char *dest, const char *source, size_t totalsize)
{
  strncpy(dest, source, totalsize - 1);	/* strncpy: OK (we must assume 'totalsize' is correct) */
  dest[totalsize - 1] = '\0';
  return strlen(source);
}
#endif

#if !defined(HAVE_STRDUP)
/** Create a duplicate of a string function. Portable. */
char *strdup(const char *source)
{
  char *new_z;

  CREATE(new_z, char, strlen(source) + 1);
  return (strcpy(new_z, source)); /* strcpy: OK */
}
#endif

/** Strips "\\r\\n" from just the end of a string. Will not remove internal
 * "\\r\\n" values to the string.
 * @post Replaces any "\\r\\n" values at the end of the string with null.
 * @param txt The writable string to prune. */
void prune_crlf(char *txt)
{
  int i = strlen(txt) - 1;

  while (txt[i] == '\n' || txt[i] == '\r')
    txt[i--] = '\0';
}

#ifndef str_cmp
/** a portable, case-insensitive version of strcmp(). Returns: 0 if equal, > 0
 * if arg1 > arg2, or < 0 if arg1 < arg2. Scan until strings are found
 * different or we reach the end of both. */
int str_cmp(const char *arg1, const char *arg2)
{
  int chk, i;

  if (arg1 == NULL || arg2 == NULL) {
    log("SYSERR: str_cmp() passed a NULL pointer, %p or %p.", (void *)arg1, (void *)arg2);
    return (0);
  }

  for (i = 0; arg1[i] || arg2[i]; i++)
    if ((chk = LOWER(arg1[i]) - LOWER(arg2[i])) != 0)
      return (chk);	/* not equal */

  return (0);
}
#endif

#ifndef strn_cmp
/** a portable, case-insensitive version of strncmp(). Returns: 0 if equal, > 0
 * if arg1 > arg2, or < 0 if arg1 < arg2. Scan until strings are found
 * different, the end of both, or n is reached. */
int strn_cmp(const char *arg1, const char *arg2, int n)
{
  int chk, i;

  if (arg1 == NULL || arg2 == NULL) {
    log("SYSERR: strn_cmp() passed a NULL pointer, %p or %p.", (void *)arg1, (void *)arg2);
    return (0);
  }

  for (i = 0; (arg1[i] || arg2[i]) && (n > 0); i++, n--)
    if ((chk = LOWER(arg1[i]) - LOWER(arg2[i])) != 0)
      return (chk);	/* not equal */

  return (0);
}
#endif

/** New variable argument log() function; logs messages to disk.
 * Works the same as the old for previously written code but is very nice
 * if new code wishes to implment printf style log messages without the need
 * to make prior sprintf calls.
 * @param format The message to log. Standard printf formatting and variable
 * arguments are allowed.
 * @param args The comma delimited, variable substitutions to make in str. */
void basic_mud_vlog(const char *format, va_list args)
{
  time_t ct = time(0);
  char timestr[21];
  int i;
  
  if (logfile == NULL) {
    puts("SYSERR: Using log() before stream was initialized!");
    return;
  }

  if (format == NULL)
    format = "SYSERR: log() received a NULL format.";

  for (i=0;i<21;i++) timestr[i]=0;
  strftime(timestr, sizeof(timestr), "%b %d %H:%M:%S %Y", localtime(&ct));

  fprintf(logfile, "%-20.20s :: ", timestr);
  vfprintf(logfile, format, args);
  fputc('\n', logfile);
  fflush(logfile);
}

/** Log messages directly to syslog on disk, no display to in game immortals.
 * Supports variable string modification arguments, a la printf. Most likely
 * any calls to plain old log() have been redirected, via macro, to this
 * function.
 * @param format The message to log. Standard printf formatting and variable
 * arguments are allowed.
 * @param ... The comma delimited, variable substitutions to make in str. */
void basic_mud_log(const char *format, ...)
{
  va_list args;

  va_start(args, format);
  basic_mud_vlog(format, args);
  va_end(args);
}

/** Essentially the touch command. Create an empty file or update the modified
 * time of a file.
 * @param path The filepath to "touch." This filepath is relative to the /lib
 * directory relative to the root of the mud distribution. */
int touch(const char *path)
{
  FILE *fl;

  if (!(fl = fopen(path, "a"))) {
    log("SYSERR: %s: %s", path, strerror(errno));
    return (-1);
  } else {
    fclose(fl);
    return (0);
  }
}

/** Log mud messages to a file & to online imm's syslogs.
 * @param type The minimum syslog level that needs be set to see this message.
 * OFF, BRF, NRM and CMP are the values from lowest to highest. Using mudlog
 * with type = OFF should be avoided as every imm will see the message even
 * if they have syslog turned off.
 * @param level Minimum character level needed to see this message.
 * @param file TRUE log this to the syslog file, FALSE do not log this to disk.
 * @param str The message to log. Standard printf formatting and variable
 * arguments are allowed.
 * @param ... The comma delimited, variable substitutions to make in str. */
void mudlog(int type, int level, int file, const char *str, ...)
{
  char buf[MAX_STRING_LENGTH];
  struct descriptor_data *i;
  va_list args;

  if (str == NULL)
    return;	/* eh, oh well. */

  if (file) {
    va_start(args, str);
    basic_mud_vlog(str, args);
    va_end(args);
  }

  if (level < 0)
    return;

  strcpy(buf, "[ ");	/* strcpy: OK */
  va_start(args, str);
  vsnprintf(buf + 2, sizeof(buf) - 6, str, args);
  va_end(args);
  strcat(buf, " ]\r\n");	/* strcat: OK */

  for (i = descriptor_list; i; i = i->next) {
    if (STATE(i) != CON_PLAYING || IS_NPC(i->character)) /* switch */
      continue;
    if (GET_LEVEL(i->character) < level)
      continue;
    if (PLR_FLAGGED(i->character, PLR_WRITING))
      continue;
    if (type > (PRF_FLAGGED(i->character, PRF_LOG1) ? 1 : 0) + (PRF_FLAGGED(i->character, PRF_LOG2) ? 2 : 0))
      continue;

    send_to_char(i->character, "%s%s%s", CCGRN(i->character, C_NRM), buf, CCNRM(i->character, C_NRM));
  }
}



/** Take a bitvector and return a human readable
 * description of which bits are set in it.
 * @pre The final element in the names array must contain a one character
 * string consisting of a single newline character "\\n". Caller of function is
 * responsible for creating the memory buffer for the result string.
 * @param[in] bitvector The bitvector to test for set bits.
 * @param[in] names An array of human readable strings describing each possible
 * bit. The final element in this array must be a string made of a single
 * newline character (eg "\\n").
 * If you don't have a 'const' array for the names param, cast it as such.
 * @param[out] result Holds the names of the set bits in bitvector. The bit
 * names will be delimited by a single space.
 * Caller of sprintbit is responsible for creating the buffer for result.
 * Will be set to "NOBITS" if no bits are set in bitvector (ie bitvector = 0).
 * @param[in] reslen The length of the available memory in the result buffer.
 * Ideally, results will be large enough to hold the description of every bit
 * that could possibly be set in bitvector. */
size_t sprintbit(bitvector_t bitvector, const char *names[], char *result, size_t reslen)
{
  size_t len = 0;
  int nlen;
  long nr;

  *result = '\0';

  for (nr = 0; bitvector && len < reslen; bitvector >>= 1) {
    if (IS_SET(bitvector, 1)) {
      nlen = snprintf(result + len, reslen - len, "%s ", *names[nr] != '\n' ? names[nr] : "UNDEFINED");
      if (len + nlen >= reslen || nlen < 0)
        break;
      len += nlen;
    }

    if (*names[nr] != '\n')
      nr++;
  }

  if (!*result)
    len = strlcpy(result, "NOBITS ", reslen);

  return (len);
}

/** Return the human readable name of a defined type.
 * @pre The final element in the names array must contain a one character
 * string consisting of a single newline character "\\n". Caller of function is
 * responsible for creating the memory buffer for the result string.
 * @param[in] type The type number to be translated.
 * @param[in] names An array of human readable strings describing each possible
 * bit. The final element in this array must be a string made of a single
 * newline character (eg "\\n").
 * @param[out] result Holds the translated name of the type.
 * Caller of sprintbit is responsible for creating the buffer for result.
 * Will be set to "UNDEFINED" if the type is greater than the number of names
 * available.
 * @param[in] reslen The length of the available memory in the result buffer. */
size_t sprinttype(int type, const char *names[], char *result, size_t reslen)
{
  int nr = 0;

  while (type && *names[nr] != '\n') {
    type--;
    nr++;
  }

  return strlcpy(result, *names[nr] != '\n' ? names[nr] : "UNDEFINED", reslen);
}

/** Take a bitarray and return a human readable description of which bits are
 * set in it.
 * @pre The final element in the names array must contain a one character
 * string consisting of a single newline character "\\n". Caller of function is
 * responsible for creating the memory buffer for the result string large enough
 * to hold all possible bit translations. There is no error checking for
 * possible array overflow for result.
 * @param[in] bitvector The bitarray in which to test for set bits.
 * @param[in] names An array of human readable strings describing each possible
 * bit. The final element in this array must be a string made of a single
 * newline character (eg "\\n").
 * If you don't have a 'const' array for the names param, cast it as such.
 * @param[in] maxar The number of 'bytes' in the bitarray. This number will
 * usually be pre-defined for the particular bitarray you are using.
 * @param[out] result Holds the names of the set bits in bitarray. The bit
 * names are delimited by a single space. Ideally, results will be large enough
 * to hold the description of every bit that could possibly be set in bitvector.
 * Will be set to "NOBITS" if no bits are set in bitarray (ie all bits in the
 * bitarray are equal to 0).
 */
void sprintbitarray(int bitvector[], const char *names[], int maxar, char *result)
{
  int nr, teller, found = FALSE;

  *result = '\0';

  for(teller = 0; teller < maxar && !found; teller++)
  {
    for (nr = 0; nr < 32 && !found; nr++)
    {
      if (IS_SET_AR(bitvector, (teller*32)+nr))
      {
        if (*names[(teller*32)+nr] != '\n')
        {
          if (*names[(teller*32)+nr] != '\0')
          {
            strcat(result, names[(teller*32)+nr]);
            strcat(result, " ");
          }
        }
        else
        {
          strcat(result, "UNDEFINED ");
        }
      }
      if (*names[(teller*32)+nr] == '\n')
        found = TRUE;
    }
  }

  if (!*result)
    strcpy(result, "NOBITS ");
}

/** Calculate the REAL time passed between two time invervals.
 * @todo Recommend making this function foresightedly useful by calculating
 * real months and years, too.
 * @param t2 The later time.
 * @param t1 The earlier time. */
struct time_info_data *real_time_passed(time_t t2, time_t t1)
{
  long secs;
  static struct time_info_data now;

  secs = t2 - t1;

  now.hours = (secs / SECS_PER_REAL_HOUR) % 24;	/* 0..23 hours */
  secs -= SECS_PER_REAL_HOUR * now.hours;

  now.day = (secs / SECS_PER_REAL_DAY);	/* 0..34 days  */
  /* secs -= SECS_PER_REAL_DAY * now.day; - Not used. */

  now.month = -1;
  now.year = -1;

  return (&now);
}

/** Calculate the MUD time passed between two time invervals.
 * @param t2 The later time.
 * @param t1 The earlier time. */
struct time_info_data *mud_time_passed(time_t t2, time_t t1)
{
  long secs;
  static struct time_info_data now;

  secs = t2 - t1;

  now.hours = (secs / SECS_PER_MUD_HOUR) % 24;	/* 0..23 hours */
  secs -= SECS_PER_MUD_HOUR * now.hours;

  now.day = (secs / SECS_PER_MUD_DAY) % 35;	/* 0..34 days  */
  secs -= SECS_PER_MUD_DAY * now.day;

  now.month = (secs / SECS_PER_MUD_MONTH) % 17;	/* 0..16 months */
  secs -= SECS_PER_MUD_MONTH * now.month;

  now.year = (secs / SECS_PER_MUD_YEAR);	/* 0..XX? years */

  return (&now);
}

/** Translate the current mud time to real seconds (in type time_t).
 * @param now The current mud time to translate into a real time unit. */
time_t mud_time_to_secs(struct time_info_data *now)
{
  time_t when = 0;

  when += now->year  * SECS_PER_MUD_YEAR;
  when += now->month * SECS_PER_MUD_MONTH;
  when += now->day   * SECS_PER_MUD_DAY;
  when += now->hours * SECS_PER_MUD_HOUR;
  return (time(NULL) - when);
}

/** Calculate a player's MUD age.
 * @todo The minimum starting age of 17 is hardcoded in this function. Recommend
 * changing the minimum age to a property (variable) external to this function.
 * @param ch A valid player character. */
struct time_info_data *age(struct char_data *ch)
{
  static struct time_info_data player_age;

  player_age = *mud_time_passed(time(0), ch->player.time.birth);

  player_age.year += 17;	/* All players start at 17 */

  return (&player_age);
}

/** Check if making ch follow victim will create an illegal follow loop. In
 * essence, this prevents someone from following a character in a group that
 * is already being lead by the character.
 * @param ch The character trying to follow.
 * @param victim The character being followed. */
bool circle_follow(struct char_data *ch, struct char_data *victim)
{
  struct char_data *k;

  for (k = victim; k; k = k->master) {
    if (k == ch)
      return (TRUE);
  }

  return (FALSE);
}

/** Call on a character (NPC or PC) to stop them from following someone and
 * to break any charm affect.
 * @todo Make the messages returned from the broken charm affect more
 * understandable.
 * @pre ch MUST be following someone, else core dump.
 * @post The charm affect (AFF_CHARM) will be removed from the character and
 * the character will stop following the "master" they were following.
 * @param ch The character (NPC or PC) to stop from following.
 * */
void stop_follower(struct char_data *ch)
{
  struct follow_type *j, *k;

  /* Makes sure this function is not called when it shouldn't be called. */
  if (ch->master == NULL) {
    core_dump();
    return;
  }

  if (AFF_FLAGGED(ch, AFF_CHARM)) {
    act("You realize that $N is a jerk!", FALSE, ch, 0, ch->master, TO_CHAR);
    act("$n realizes that $N is a jerk!", FALSE, ch, 0, ch->master, TO_NOTVICT);
    act("$n hates your guts!", FALSE, ch, 0, ch->master, TO_VICT);
    if (affected_by_spell(ch, SPELL_CHARM))
      affect_from_char(ch, SPELL_CHARM);
  } else {
    act("You stop following $N.", FALSE, ch, 0, ch->master, TO_CHAR);
    act("$n stops following $N.", TRUE, ch, 0, ch->master, TO_NOTVICT);
    if (CAN_SEE(ch->master, ch))
      act("$n stops following you.", TRUE, ch, 0, ch->master, TO_VICT);
  }

  if (ch->master->followers->follower == ch) {	/* Head of follower-list? */
    k = ch->master->followers;
    ch->master->followers = k->next;
    free(k);
  } else {			/* locate follower who is not head of list */
    for (k = ch->master->followers; k->next->follower != ch; k = k->next);

    j = k->next;
    k->next = j->next;
    free(j);
  }

  ch->master = NULL;
  REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_CHARM);
}

/** Finds the number of follows that are following, and charmed by, the
 * character (PC or NPC).
 * @param ch The character to check for charmed followers.
 */
int num_followers_charmed(struct char_data *ch)
{
  struct follow_type *lackey;
  int total = 0;

  for (lackey = ch->followers; lackey; lackey = lackey->next)
    if (AFF_FLAGGED(lackey->follower, AFF_CHARM) && lackey->follower->master == ch)
      total++;

  return (total);
}

/** Called when a character that follows/is followed dies. If the character
 * is the leader of a group, it stops everyone in the group from following
 * them. Despite the title, this function does not actually perform the kill on
 * the character passed in as the argument.
 * @param ch The character (NPC or PC) to stop from following.
 * */
void die_follower(struct char_data *ch)
{
  struct follow_type *j, *k;

  if (ch->master)
    stop_follower(ch);

  for (k = ch->followers; k; k = j) {
    j = k->next;
    stop_follower(k->follower);
  }
}

/** Adds a new follower to a group.
 * @todo Maybe make circle_follow an inherent part of this function?
 * @pre Make sure to call circle_follow first. ch may also not already
 * be following anyone, otherwise core dump.
 * @param ch The character to follow.
 * @param leader The character to be followed. */
void add_follower(struct char_data *ch, struct char_data *leader)
{
  struct follow_type *k;

  if (ch->master) {
    core_dump();
    return;
  }

  ch->master = leader;

  CREATE(k, struct follow_type, 1);

  k->follower = ch;
  k->next = leader->followers;
  leader->followers = k;

  act("You now follow $N.", FALSE, ch, 0, leader, TO_CHAR);
  if (CAN_SEE(leader, ch))
    act("$n starts following you.", TRUE, ch, 0, leader, TO_VICT);
  act("$n starts to follow $N.", TRUE, ch, 0, leader, TO_NOTVICT);
}

/** Reads the next non-blank line off of the input stream. Empty lines are
 * skipped. Lines which begin with '*' are considered to be comments and are
 * skipped.
 * @pre Caller must allocate memory for buf.
 * @post If a there is a line to be read, the newline character is removed from
 * the file line ending and the string is returned. Else a null string is
 * returned in buf.
 * @param[in] fl The file to be read from.
 * @param[out] buf The next non-blank line read from the file. Buffer given must
 * be at least READ_SIZE (256) characters large. */
int get_line(FILE *fl, char *buf)
{
  char temp[READ_SIZE];
  int lines = 0;
  int sl;

  do {
    if (!fgets(temp, READ_SIZE, fl))
      return (0);
    lines++;
  } while (*temp == '*' || *temp == '\n' || *temp == '\r');

  /* Last line of file doesn't always have a \n, but it should. */
  sl = strlen(temp);
  while (sl > 0 && (temp[sl - 1] == '\n' || temp[sl - 1] == '\r'))
    temp[--sl] = '\0';

  strcpy(buf, temp); /* strcpy: OK, if buf >= READ_SIZE (256) */
  return (lines);
}

/** Create the full path, relative to the library path, of the player type
 * file to open.
 * @todo Make the return type bool.
 * @pre Caller is responsible for allocating memory buffer for the created
 * file name.
 * @post The potential file path to open is created. This function does not
 * actually open any file descriptors.
 * @param[out] filename Buffer to store the full path of the file to open.
 * @param[in] fbufsize The maximum size of filename, and the maximum size
 * of the path that can be written to it.
 * @param[in] mode What type of files can be created. Currently, recognized
 * modes are CRASH_FILE, ETEXT_FILE, SCRIPT_VARS_FILE and PLR_FILE.
 * @param[in] orig_name The player name to create the filepath (of type mode)
 * for. */
int get_filename(char *filename, size_t fbufsize, int mode, const char *orig_name)
{
  const char *prefix, *middle, *suffix;
  char name[PATH_MAX], *ptr;

  if (orig_name == NULL || *orig_name == '\0' || filename == NULL) {
    log("SYSERR: NULL pointer or empty string passed to get_filename(), %p or %p.",
		(const void *)orig_name, (void *)filename);
    return (0);
  }

  switch (mode) {
  case CRASH_FILE:
    prefix = LIB_PLROBJS;
    suffix = SUF_OBJS;
    break;
  case ETEXT_FILE:
    prefix = LIB_PLRTEXT;
    suffix = SUF_TEXT;
    break;
  case SCRIPT_VARS_FILE:
    prefix = LIB_PLRVARS;
    suffix = SUF_MEM;
    break;
  case PLR_FILE:
    prefix = LIB_PLRFILES;
    suffix = SUF_PLR;
    break;
  default:
    return (0);
  }

  strlcpy(name, orig_name, sizeof(name));
  for (ptr = name; *ptr; ptr++)
    *ptr = LOWER(*ptr);

  switch (LOWER(*name)) {
  case 'a':  case 'b':  case 'c':  case 'd':  case 'e':
    middle = "A-E";
    break;
  case 'f':  case 'g':  case 'h':  case 'i':  case 'j':
    middle = "F-J";
    break;
  case 'k':  case 'l':  case 'm':  case 'n':  case 'o':
    middle = "K-O";
    break;
  case 'p':  case 'q':  case 'r':  case 's':  case 't':
    middle = "P-T";
    break;
  case 'u':  case 'v':  case 'w':  case 'x':  case 'y':  case 'z':
    middle = "U-Z";
    break;
  default:
    middle = "ZZZ";
    break;
  }

  snprintf(filename, fbufsize, "%s%s"SLASH"%s.%s", prefix, middle, name, suffix);
  return (1);
}

/** Calculate the number of player characters (PCs) in the room. Any NPC (mob)
 * is not returned in the count.
 * @param room The room to check for PCs. */
int num_pc_in_room(struct room_data *room)
{
  int i = 0;
  struct char_data *ch;

  for (ch = room->people; ch != NULL; ch = ch->next_in_room)
    if (!IS_NPC(ch))
      i++;

  return (i);
}


/** This function (derived from basic fork() abort() idea by Erwin S Andreasen)
 * causes your MUD to dump core (assuming you can) but continue running. The
 * core dump will allow post-mortem debugging that is less severe than assert();
 * Don't call this directly as core_dump_unix() but as simply 'core_dump()' so
 * that it will be excluded from systems not supporting them. You still want to
 * call abort() or exit(1) for non-recoverable errors, of course. Wonder if
 * flushing streams includes sockets?
 * @param who The file in which this call was made.
 * @param line The line at which this call was made. */
void core_dump_real(const char *who, int line)
{
  log("SYSERR: Assertion failed at %s:%d!", who, line);

#if 1	/* By default, let's not litter. */
#if defined(CIRCLE_UNIX)
  /* These would be duplicated otherwise...make very sure. */
  fflush(stdout);
  fflush(stderr);
  fflush(logfile);
  /* Everything, just in case, for the systems that support it. */
  fflush(NULL);

  /* Kill the child so the debugger or script doesn't think the MUD crashed.
   * The 'autorun' script would otherwise run it again. */
  if (fork() == 0)
    abort();
#endif
#endif
}

/** Count the number bytes taken up by color codes in a string that will be
 * empty space once the color codes are converted and made non-printable.
 * @param string The string in which to check for color codes. */
int count_color_chars(char *string)
{
  int i, len;
  int num = 0;

	if (!string || !*string)
		return 0;

	len = strlen(string);
  for (i = 0; i < len; i++) {
    while (string[i] == '\t') {
      if (string[i + 1] == '\t')
        num++;
      else
        num += 2;
      i += 2;
    }
  }
  return num;
}

/* Not the prettiest thing I've ever written but it does the task which
 * is counting all characters in a string which are not part of the
 * protocol system. This is with the exception of detailed MXP codes. */
int count_non_protocol_chars(char * str)
{
  int count = 0;
  char *string = str;
  
  while (*string) {
    if (*string == '\r' || *string == '\n') {
      string++;
      continue;
    }
    if (*string == '@' || *string == '\t') {
      string++;
      if (*string != '[' && *string != '<' && *string != '>' && *string != '(' && *string != ')')
        string++;
      else if (*string == '[') {
        while (*string && *string != ']')
          string++;
        string++;
      } else
        string++;
      continue;
    }
    count++;
    string++;
  }
  
  return count;
}

/** Tests to see if a room is dark. Rules (unless overridden by ROOM_DARK):
 * Inside and City rooms are always lit. Outside rooms are dark at sunset and
 * night.
 * @todo Make the return value a bool.
 * @param room The real room to test for. */
int room_is_dark(room_rnum room)
{
  if (!VALID_ROOM_RNUM(room)) {
    log("room_is_dark: Invalid room rnum %d. (0-%d)", room, top_of_world);
    return (FALSE);
  }

  if (world[room].light)
    return (FALSE);

  if (ROOM_FLAGGED(room, ROOM_DARK))
    return (TRUE);

  if (SECT(room) == SECT_INSIDE || SECT(room) == SECT_CITY)
    return (FALSE);

  if (weather_info.sunlight == SUN_SET || weather_info.sunlight == SUN_DARK)
    return (TRUE);

  return (FALSE);
}

/** Calculates the Levenshtein distance between two strings. Currently used
 * by the mud to make suggestions to the player when commands are mistyped.
 * This function is most useful when an index of possible choices are available
 * and the results of this function are constrained and used to help narrow
 * down the possible choices. For more information about Levenshtein distance,
 * recommend doing an internet or wikipedia search.
 * @param s1 The input string.
 * @param s2 The string to be compared to. */
int levenshtein_distance(const char *s1, const char *s2)
{
  int **d, i, j;
  int s1_len = strlen(s1), s2_len = strlen(s2);

  CREATE(d, int *, s1_len + 1);

  for (i = 0; i <= s1_len; i++) {
    CREATE(d[i], int, s2_len + 1);
    d[i][0] = i;
  }

  for (j = 0; j <= s2_len; j++)
    d[0][j] = j;
  for (i = 1; i <= s1_len; i++)
    for (j = 1; j <= s2_len; j++)
      d[i][j] = MIN(d[i - 1][j] + 1, MIN(d[i][j - 1] + 1,
      d[i - 1][j - 1] + ((s1[i - 1] == s2[j - 1]) ? 0 : 1)));

  i = d[s1_len][s2_len];

  for (j = 0; j <= s1_len; j++)
    free(d[j]);
  free(d);

  return i;
}

/** Removes a character from a piece of furniture. Unlike some of the other
 * _from_ functions, this does not place the character into NOWHERE.
 * @post ch is unattached from the furniture object.
 * @param ch The character to remove from the furniture object.
 */
void char_from_furniture(struct char_data *ch)
{
  struct obj_data *furniture;
  struct char_data *tempch;

  if (!SITTING(ch))
    return;

  if (!(furniture = SITTING(ch))){
    log("SYSERR: No furniture for char in char_from_furniture.");
    SITTING(ch) = NULL;
    NEXT_SITTING(ch) = NULL;
    return;
  }

  if (!(tempch = OBJ_SAT_IN_BY(furniture))){
    log("SYSERR: Char from furniture, but no furniture!");
    SITTING(ch) = NULL;
    NEXT_SITTING(ch) = NULL;
    GET_OBJ_VAL(furniture, 1) = 0;
    return;
  }

  if (tempch == ch){
    if (!NEXT_SITTING(ch)) {
      OBJ_SAT_IN_BY(furniture) = NULL;
    } else {
      OBJ_SAT_IN_BY(furniture) = NEXT_SITTING(ch);
    }
  } else {
    for (tempch = OBJ_SAT_IN_BY(furniture); tempch; tempch = NEXT_SITTING(tempch)) {
      if (NEXT_SITTING(tempch) == ch) {
        NEXT_SITTING(tempch) = NEXT_SITTING(ch);
      }
    }
  }
  GET_OBJ_VAL(furniture, 1) -= 1;
  SITTING(ch) = NULL;
  NEXT_SITTING(ch) = NULL;


  if (GET_OBJ_VAL(furniture, 1) < 1){
    OBJ_SAT_IN_BY(furniture) = NULL;
    GET_OBJ_VAL(furniture, 1) = 0;
  }

 return;
}


/* column_list
   The list is output in a fixed format, and only the number of columns can be adjusted
   This function will output the list to the player
   Vars:
     ch          - the player
     num_cols    - the desired number of columns
     list        - a pointer to a list of strings
     list_length - So we can work with lists that don't end with /n
     show_nums   - when set to TRUE, it will show a number before the list entry.
*/
void column_list(struct char_data *ch, int num_cols, const char **list, int list_length, bool show_nums)
{
   size_t max_len = 0, len = 0, temp_len;
   int num_per_col, col_width, r, c, i, offset = 0;
   char buf[MAX_STRING_LENGTH];

   *buf='\0';
   /* Work out the longest list item */
   for (i=0; i<list_length; i++)
     if (max_len < strlen(list[i]))
       max_len = strlen(list[i]);

   /* auto columns case */
   if (num_cols == 0) {
	   num_cols = (IS_NPC(ch) ? 80 : GET_SCREEN_WIDTH(ch)) / (max_len + (show_nums ? 5 : 1));
   }

   /* Ensure that the number of columns is in the range 1-10 */
   num_cols = MIN(MAX(num_cols,1), 10);

   /* Work out the longest list item */
   for (i=0; i<list_length; i++)
     if (max_len < strlen(list[i]))
       max_len = strlen(list[i]);

   /* Calculate the width of each column */
   if (IS_NPC(ch))   col_width = 80 / num_cols;
   else              col_width = (GET_SCREEN_WIDTH(ch)) / num_cols;

   if (show_nums) col_width-=4;

   if (col_width < 0 || (size_t)col_width < max_len)
     log("Warning: columns too narrow for correct output to %s in simple_column_list (utils.c)", GET_NAME(ch));

   /* Calculate how many list items there should be per column */
   num_per_col = (list_length / num_cols) + ((list_length % num_cols) ? 1 : 0);

   /* Fill 'buf' with the columnised list */
   for (r=0; r<num_per_col; r++)
   {
     for (c=0; c<num_cols; c++)
     {
       offset = (c*num_per_col)+r;
       if (offset < list_length)
       {
         if (show_nums)
           temp_len = snprintf(buf+len, sizeof(buf) - len, "%2d) %-*s", offset+1, col_width, list[(offset)]);
         else
           temp_len = snprintf(buf+len, sizeof(buf) - len, "%-*s", col_width, list[(offset)]);
         len += temp_len;
       }
     }
     temp_len = snprintf(buf+len, sizeof(buf) - len, "\r\n");
     len += temp_len;
   }

   if (len >= sizeof(buf))
     snprintf((buf + MAX_STRING_LENGTH) - 22, 22, "\r\n*** OVERFLOW ***\r\n");

   /* Send the list to the player */
   page_string(ch->desc, buf, TRUE);
}


/**
 * Search through a string array of flags for a particular flag.
 * @param flag_list An array of flag name strings. The final element must
 * be a string made up of a single newline.
 * @param flag_name The name to search in flag_list.
 */
int get_flag_by_name(const char *flag_list[], char *flag_name)
{
  int i=0;
  for (;flag_list[i] && *flag_list[i] && strcmp(flag_list[i], "\n") != 0; i++)
    if (!strcmp(flag_list[i], flag_name))
      return (i);
  return (NOFLAG);
}

/**
 * Reads a certain number of lines from the begining of a file, like performing
 * a 'head'.
 * @pre Expects an already open file and the user to supply enough memory
 * in the output buffer to hold the lines read from the file. Assumes the
 * file is a text file. Expects buf to be nulled out if the entire buf is
 * to be used, otherwise, appends file information beyond the first null
 * character. lines_to_read is assumed to be a positive number.
 * @post Rewinds the file pointer to the beginning of the file. If buf is
 * too small to handle the requested output, **OVERFLOW** is appended to the
 * buffer.
 * @param[in] file A pointer to an already successfully opened file.
 * @param[out] buf Buffer to hold the data read from the file. Will not
 * overwrite preexisting information in a non-null string.
 * @param[in] bufsize The total size of the buffer.
 * @param[in] lines_to_read The number of lines to be read from the front of
 * the file.
 */
int file_head( FILE *file, char *buf, size_t bufsize, int lines_to_read )
{
  /* Local variables */
  int lines_read = 0;   /* The number of lines read so far. */
  char line[READ_SIZE]; /* Retrieval buffer for file. */
  size_t buflen;        /* Amount of previous existing data in buffer. */
  int readstatus = 1;   /* Are we at the end of the file? */
  int n = 0;            /* Return value from snprintf. */
  const char *overflow = "\r\n**OVERFLOW**\r\n"; /* Appended if overflow. */

  /* Quick check for bad arguments. */
  if (lines_to_read <= 0)
  {
    return lines_to_read;
  }

  /* Initialize local variables not already initialized. */
  buflen  = strlen(buf);

  /* Read from the front of the file. */
  rewind(file);

  while ( (lines_read < lines_to_read) &&
      (readstatus > 0) && (buflen < bufsize) )
  {
    /* Don't use get_line to set lines_read because get_line will return
     * the number of comments skipped during reading. */
    readstatus = get_line( file, line );

    if (readstatus > 0)
    {
      n = snprintf( buf + buflen, bufsize - buflen, "%s\r\n", line);
      buflen += n;
      lines_read++;
    }
  }

  /* Check to see if we had a potential buffer overflow. */
  if (buflen >= bufsize)
  {
    /* We should never see this case, but... */
    if ( (strlen(overflow) + 1) >= bufsize )
    {
      core_dump();
      snprintf( buf, bufsize, "%s", overflow);
    }
    else
    {
      /* Append the overflow statement to the buffer. */
      snprintf( buf + buflen - strlen(overflow) - 1, strlen(overflow) + 1, "%s", overflow);
    }
  }

  rewind(file);

  /* Return the number of lines. */
  return lines_read;
}

/**
 * Reads a certain number of lines from the end of the file, like performing
 * a 'tail'.
 * @pre Expects an already open file and the user to supply enough memory
 * in the output buffer to hold the lines read from the file. Assumes the
 * file is a text file. Expects buf to be nulled out if the entire buf is
 * to be used, otherwise, appends file information beyond the first null
 * character in buf. lines_to_read is assumed to be a positive number.
 * @post Rewinds the file pointer to the beginning of the file. If buf is
 * too small to handle the requested output, **OVERFLOW** is appended to the
 * buffer.
 * @param[in] file A pointer to an already successfully opened file.
 * @param[out] buf Buffer to hold the data read from the file. Will not
 * overwrite preexisting information in a non-null string.
 * @param[in] bufsize The total size of the buffer.
 * @param[in] lines_to_read The number of lines to be read from the back of
 * the file.
 */
int file_tail( FILE *file, char *buf, size_t bufsize, int lines_to_read )
{
  /* Local variables */
  int lines_read = 0;   /* The number of lines read so far. */
  int total_lines = 0;  /* The total number of lines in the file. */
  char c;               /* Used to fast forward the file. */
  char line[READ_SIZE]; /* Retrieval buffer for file. */
  size_t buflen;        /* Amount of previous existing data in buffer. */
  int readstatus = 1;   /* Are we at the end of the file? */
  int n = 0;            /* Return value from snprintf. */
  const char *overflow = "\r\n**OVERFLOW**\r\n"; /* Appended if overflow. */

  /* Quick check for bad arguments. */
  if (lines_to_read <= 0)
  {
    return lines_to_read;
  }

  /* Initialize local variables not already initialized. */
  buflen  = strlen(buf);
  total_lines = file_numlines(file); /* Side effect: file is rewound. */

  /* Fast forward to the location we should start reading from */
  while (((lines_to_read + lines_read) < total_lines))
  {
    do {
      c = fgetc(file);
    } while(c != '\n');

    lines_read++;
  }

  /* We reuse the lines_read counter. */
  lines_read = 0;

  /** From here on, we perform just like file_head */
  while ( (lines_read < lines_to_read) &&
      (readstatus > 0) && (buflen < bufsize) )
  {
    /* Don't use get_line to set lines_read because get_line will return
     * the number of comments skipped during reading. */
    readstatus = get_line( file, line );

    if (readstatus > 0)
    {
      n = snprintf( buf + buflen, bufsize - buflen, "%s\r\n", line);
      buflen += n;
      lines_read++;
    }
  }

  /* Check to see if we had a potential buffer overflow. */
  if (buflen >= bufsize)
  {
    /* We should never see this case, but... */
    if ( (strlen(overflow) + 1) >= bufsize )
    {
      core_dump();
      snprintf( buf, bufsize, "%s", overflow);
    }
    else
    {
      /* Append the overflow statement to the buffer. */
      snprintf( buf + buflen - strlen(overflow) - 1, strlen(overflow) + 1, "%s", overflow);
    }
  }

  rewind(file);

  /* Return the number of lines read. */
  return lines_read;

}

/** Returns the byte size of a file. We assume size_t to be a large enough type
 * to handle all of the file sizes in the mud, and so do not make SIZE_MAX
 * checks.
 * @pre file parameter must already be opened.
 * @post file will be rewound.
 * @param file The file to determine the size of.
 */
size_t file_sizeof( FILE *file )
{
  size_t numbytes = 0;

  rewind(file);

  /* It would be so much easier to do a byte count if an fseek SEEK_END and
   * ftell pair of calls was portable for text files, but all information
   * I've found says that getting a file size from ftell for text files is
   * not portable. Oh well, this method should be extremely fast for the
   * relatively small filesizes in the mud, and portable, too. */
  while (!feof(file))
  {
    fgetc(file);
    numbytes++;
  }

  rewind(file);

  return numbytes;
}

/** Returns the number of newlines "\\n" in a file, which we equate to number of
 * lines. We assume the int type more than adequate to count the number of lines
 * and do not make checks for overrunning INT_MAX.
 * @pre file parameter must already be opened.
 * @post file will be rewound.
 * @param file The file to determine the size of.
 */
int file_numlines( FILE *file )
{
  int numlines = 0;
  char c;

  rewind(file);

  while (!feof(file))
  {
    c = fgetc(file);
    if (c == '\n')
    {
      numlines++;
    }
  }

  rewind(file);

  return numlines;
}


/** A string converter designed to deal with the compile sensitive IDXTYPE.
 * Relies on the friendlier strtol function.
 * @pre Assumes that NOWHERE, NOTHING, NOBODY, NOFLAG, etc are all equal.
 * @param str_to_conv A string of characters to attempt to convert to an
 * IDXTYPE number.
 */
IDXTYPE atoidx( const char *str_to_conv )
{
  long int result;

  /* Check for errors */
  errno = 0;

  result = strtol(str_to_conv, NULL, 10);

  if ( errno || (result > IDXTYPE_MAX) || (result < 0) )
    return NOWHERE; /* All of the NO* settings should be the same */
  else
    return (IDXTYPE) result;
}

#define isspace_ignoretabs(c) ((c)!='\t' && isspace(c))

/*
   strfrmt (String Format) function
   Used by automap/map system
   Re-formats a string to fit within a particular size box.
   Recognises @ color codes, and if a line ends in one color, the
   next line will start with the same color.
   Ends every line with \tn to prevent color bleeds.
*/
char *strfrmt(char *str, int w, int h, int justify, int hpad, int vpad)
{
  static char ret[MAX_STRING_LENGTH];
  char line[MAX_INPUT_LENGTH];
  char *sp = str;
  char *lp = line;
  char *rp = ret;
  char *wp;
  int wlen = 0, llen = 0, lcount = 0;
  char last_color='n';
  bool new_line_started = FALSE;

  memset(line, '\0', MAX_INPUT_LENGTH);
  /* Nomalize spaces and newlines */
  /* Split into lines, including convert \\ into \r\n */
  while(*sp) {
    /* eat leading space */
    while(*sp && isspace_ignoretabs(*sp)) sp++;
    /* word begins */
    wp = sp;
    wlen = 0;
    while(*sp) { /* Find the end of the word */
      if(isspace_ignoretabs(*sp)) break;
      if(*sp=='\\' && sp[1] && sp[1]=='\\') {
        if(sp!=wp)
          break; /* Finish dealing with the current word */
        sp += 2; /* Eat the marker and any trailing space */
        while(*sp && isspace_ignoretabs(*sp)) sp++;
        wp = sp;
        /* Start a new line */
        if(hpad)
          for(; llen < w; llen++)
            *lp++ = ' ';
        *lp++ = '\r';
        *lp++ = '\n';
        *lp++ = '\0';
        rp += sprintf(rp, "%s", line);
        llen = 0;
        lcount++;
        lp = line;
      } else if (*sp=='`'||*sp=='$'||*sp=='#') {
        if (sp[1] && (sp[1]==*sp))
          wlen++; /* One printable char here */
        sp += 2; /* Eat the whole code regardless */
      } else if (*sp=='\t'&&sp[1]) {
        char MXPcode = sp[1]=='[' ? ']' : sp[1]=='<' ? '>' : '\0';
	
  if (!MXPcode)
	   last_color = sp[1];
 
        sp += 2; /* Eat the code */
        if (MXPcode)
        {
           while (*sp!='\0'&&*sp!=MXPcode)
             ++sp; /* Eat the rest of the code */
        } 
      } else {
        wlen++;
        sp++;
      }
    }
    if(llen + wlen + (lp==line ? 0 : 1) > w) {
      /* Start a new line */
      if(hpad)
        for(; llen < w; llen++)
          *lp++ = ' ';
      *lp++ = '\t';  /* 'normal' color */
      *lp++ = 'n';
      *lp++ = '\r'; /* New line */
      *lp++ = '\n';
      *lp++ = '\0';
      sprintf(rp, "%s", line);
      rp += strlen(line);
      llen = 0;
      lcount++;
      lp = line;
      if (last_color != 'n') {
        *lp++ = '\t';  /* restore previous color */
        *lp++ = last_color;
        new_line_started = TRUE;
      }
    }
    /* add word to line */
    if (lp!=line && new_line_started!=TRUE) {
      *lp++ = ' ';
      llen++;
    }
    new_line_started = FALSE;
    llen += wlen ;
    for( ; wp!=sp ; *lp++ = *wp++);
  }
  /* Copy over the last line */
  if(lp!=line) {
    if(hpad)
      for(; llen < w; llen++)
        *lp++ = ' ';
    *lp++ = '\r';
    *lp++ = '\n';
    *lp++ = '\0';
    sprintf(rp, "%s", line);
    rp += strlen(line);
    lcount++;
  }
  if(vpad) {
    while(lcount < h) {
      if(hpad) {
        memset(rp, ' ', w);
        rp += w;
      }
      *rp++ = '\r';
      *rp++ = '\n';
      lcount++;
    }
    *rp = '\0';
  }
  return ret;
}

/**
   Takes two long strings (multiple lines) and joins them side-by-side.
   Used by the automap/map system
   @param str1 The string to be displayed on the left.
   @param str2 The string to be displayed on the right.
   @param joiner ???.
*/
char *strpaste(char *str1, char *str2, char *joiner)
{
  static char ret[MAX_STRING_LENGTH+1];
  char *sp1 = str1;
  char *sp2 = str2;
  char *rp = ret;
  int jlen = strlen(joiner);

  while((rp - ret) < MAX_STRING_LENGTH && (*sp1 || *sp2)) {
     /* Copy line from str1 */
    while((rp - ret) < MAX_STRING_LENGTH && *sp1 && !ISNEWL(*sp1))
      *rp++ = *sp1++;
    /* Eat the newline */
    if(*sp1) {
      if(sp1[1] && sp1[1]!=sp1[0] && ISNEWL(sp1[1]))
        sp1++;
      sp1++;
    }

    /* Add the joiner */
    if((rp - ret) + jlen >= MAX_STRING_LENGTH)
      break;
    strcpy(rp, joiner);
    rp += jlen;

     /* Copy line from str2 */
    while((rp - ret) < MAX_STRING_LENGTH && *sp2 && !ISNEWL(*sp2))
      *rp++ = *sp2++;
    /* Eat the newline */
    if(*sp2) {
      if(sp2[1] && sp2[1]!=sp2[0] && ISNEWL(sp2[1]))
        sp2++;
      sp2++;
    }

    /* Add the newline */
    if((rp - ret) + 2 >= MAX_STRING_LENGTH)
      break;
    *rp++ = '\r';
    *rp++ = '\n';
  }
  /* Close off the string */
  *rp = '\0';
  return ret;
}

/* Create a blank affect struct */
void new_affect(struct affected_type *af)
{
  int i;
  af->spell     = 0;
  af->duration  = 0;
  af->modifier  = 0;
  af->location  = APPLY_NONE;
  for (i=0; i<AF_ARRAY_MAX; i++) af->bitvector[i]=0;
}

/* Handy function to get class ID number by name (abbreviations allowed) */
int get_class_by_name(char *classname)
{
    int i;
    for (i=0; i<NUM_CLASSES; i++)
      if (is_abbrev(classname, pc_class_types[i])) return(i);

    return (-1);
}

char * convert_from_tabs(char * string)
{
  static char buf[MAX_STRING_LENGTH * 8];
  
  strcpy(buf, string);
  parse_tab(buf);
  return(buf);
}
