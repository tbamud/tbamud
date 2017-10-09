/**************************************************************************
*  File: boards.c                                          Part of tbaMUD *
*  Usage: Handling of multiple bulletin boards.                           *
*                                                                         *
*  All rights reserved.  See license for complete information.            *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
**************************************************************************/

/* FEATURES & INSTALLATION INSTRUCTIONS
 * - Arbitrary number of boards handled by one set of generalized routines.
 *   Adding a new board is as easy as adding another entry to an array.
 * - Safe removal of messages while other messages are being written.
 *
 * TO ADD A NEW BOARD, simply follow our easy 4-step program:
 * 1 - Create a new board object in the object files.
 * 2 - Increase the NUM_OF_BOARDS constant in boards.h.
 * 3 - Add a new line to the board_info array below.  The fields are:
 * 	Board's virtual number.
 * 	Min level one must be to look at this board or read messages on it.
 * 	Min level one must be to post a message to the board.
 * 	Min level one must be to remove other people's messages from this
 * 	  board (but you can always remove your own message).
 * 	Filename of this board, in quotes.
 * 	Last field must always be 0.
 * 4 - In spec_assign.c, find the section which assigns the special procedure
 *     gen_board to the other bulletin boards, and add your new one in a
 *     similar fashion. */

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "boards.h"
#include "interpreter.h"
#include "handler.h"
#include "improved-edit.h"
#include "modify.h"

/* Board appearance order. */
#define	NEWEST_AT_TOP	FALSE

/* Format: vnum, read lvl, write lvl, remove lvl, filename, 0 at end. Be sure 
 * to also change NUM_OF_BOARDS in board.h*/
struct board_info_type board_info[NUM_OF_BOARDS] = {
  {3099, 0, 0, LVL_GOD, LIB_ETC "board.mortal", 0},
  {3098, LVL_IMMORT, LVL_IMMORT, LVL_GRGOD, LIB_ETC "board.immortal", 0},
  {3097, LVL_IMMORT, LVL_GRGOD, LVL_IMPL, LIB_ETC "board.freeze", 0},
  {3096, 0, 0, LVL_IMMORT, LIB_ETC "board.social", 0},
  {1226, 0, 0, LVL_IMPL, LIB_ETC "board.builder", 0},
  {1227, 0, 0, LVL_IMPL, LIB_ETC "board.staff", 0},
  {1228, 0, 0, LVL_IMPL, LIB_ETC "board.advertising", 0},
};

/* local (file scope) global variables */
static char *msg_storage[INDEX_SIZE];
static int msg_storage_taken[INDEX_SIZE];
static int num_of_msgs[NUM_OF_BOARDS];
static struct board_msginfo msg_index[NUM_OF_BOARDS][MAX_BOARD_MESSAGES];

/* local static utility functions */
static int find_slot(void);
static int find_board(struct char_data *ch);
static void init_boards(void);
static void board_reset_board(int board_type);
static void board_clear_board(int board_type);

static int find_slot(void)
{
  int i;

  for (i = 0; i < INDEX_SIZE; i++)
    if (!msg_storage_taken[i]) {
      msg_storage_taken[i] = 1;
      return (i);
    }
  return (-1);
}

/* search the room ch is standing in to find which board he's looking at */
static int find_board(struct char_data *ch)
{
  struct obj_data *obj;
  int i;

  for (obj = world[IN_ROOM(ch)].contents; obj; obj = obj->next_content)
    for (i = 0; i < NUM_OF_BOARDS; i++)
      if (BOARD_RNUM(i) == GET_OBJ_RNUM(obj))
	return (i);

  if (GET_LEVEL(ch) >= LVL_IMMORT)
    for (obj = ch->carrying; obj; obj = obj->next_content)
      for (i = 0; i < NUM_OF_BOARDS; i++)
        if (BOARD_RNUM(i) == GET_OBJ_RNUM(obj))
          return (i);

  return (-1);
}

static void init_boards(void)
{
  int i, j, fatal_error = 0;

  for (i = 0; i < INDEX_SIZE; i++) {
    msg_storage[i] = 0;
    msg_storage_taken[i] = 0;
  }

  for (i = 0; i < NUM_OF_BOARDS; i++) {
    if ((BOARD_RNUM(i) = real_object(BOARD_VNUM(i))) == NOTHING) {
      log("SYSERR: Fatal board error: board vnum %d does not exist!",
	      BOARD_VNUM(i));
      fatal_error = 1;
    }
    num_of_msgs[i] = 0;
    for (j = 0; j < MAX_BOARD_MESSAGES; j++) {
      memset((char *) &(msg_index[i][j]), 0, sizeof(struct board_msginfo));
      msg_index[i][j].slot_num = -1;
    }
    board_load_board(i);
  }

  if (fatal_error)
    exit(1);
}

SPECIAL(gen_board)
{
  int board_type;
  static int loaded = 0;
  struct obj_data *board = (struct obj_data *)me;

  /* These were originally globals for some unknown reason. */
  int ACMD_READ, ACMD_LOOK, ACMD_EXAMINE, ACMD_WRITE, ACMD_REMOVE;
  
  if (!loaded) {
    init_boards();
    loaded = 1;
  }
  if (!ch->desc)
    return (0);

  ACMD_READ = find_command("read");
  ACMD_WRITE = find_command("write");
  ACMD_REMOVE = find_command("remove");
  ACMD_LOOK = find_command("look");
  ACMD_EXAMINE = find_command("examine");

  if (cmd != ACMD_WRITE && cmd != ACMD_LOOK && cmd != ACMD_EXAMINE &&
      cmd != ACMD_READ && cmd != ACMD_REMOVE)
    return (0);

  if ((board_type = find_board(ch)) == -1) {
    log("SYSERR:  degenerate board!  (what the hell...)");
    return (0);
  }
  if (cmd == ACMD_WRITE)
    return (board_write_message(board_type, ch, argument, board));
  else if (cmd == ACMD_LOOK || cmd == ACMD_EXAMINE)
    return (board_show_board(board_type, ch, argument, board));
  else if (cmd == ACMD_READ)
    return (board_display_msg(board_type, ch, argument, board));
  else if (cmd == ACMD_REMOVE)
    return (board_remove_msg(board_type, ch, argument, board));
  else
    return (0);
}

int board_write_message(int board_type, struct char_data *ch, char *arg, struct obj_data *board)
{
  time_t ct;
  char buf[MAX_INPUT_LENGTH], buf2[MAX_NAME_LENGTH + 3], tmstr[MAX_STRING_LENGTH];

  if (GET_LEVEL(ch) < WRITE_LVL(board_type)) {
    send_to_char(ch, "You are not holy enough to write on this board.\r\n");
    return (1);
  }
  if (num_of_msgs[board_type] >= MAX_BOARD_MESSAGES) {
    send_to_char(ch, "The board is full.\r\n");
    return (1);
  }
  if ((NEW_MSG_INDEX(board_type).slot_num = find_slot()) == -1) {
    send_to_char(ch, "The board is malfunctioning - sorry.\r\n");
    log("SYSERR: Board: failed to find empty slot on write.");
    return (1);
  }
  /* skip blanks */
  skip_spaces(&arg);
  delete_doubledollar(arg);

  /* JE Truncate headline at 80 chars if it's longer than that. */
  arg[80] = '\0';

  if (!*arg) {
    send_to_char(ch, "We must have a headline!\r\n");
    return (1);
  }
  ct = time(0);
  strftime(tmstr, sizeof(tmstr), "%a %b %d %Y", localtime(&ct));

  snprintf(buf2, sizeof(buf2), "(%s)", GET_NAME(ch));
  snprintf(buf, sizeof(buf), "%s %-12s :: %s", tmstr, buf2, arg);
  NEW_MSG_INDEX(board_type).heading = strdup(buf);
  NEW_MSG_INDEX(board_type).level = GET_LEVEL(ch);

  send_to_char(ch, "Write your message.\r\n");
  send_editor_help(ch->desc);
  act("$n starts to write a message.", TRUE, ch, 0, 0, TO_ROOM);

  string_write(ch->desc, &(msg_storage[NEW_MSG_INDEX(board_type).slot_num]),
		MAX_MESSAGE_LENGTH, board_type + BOARD_MAGIC, NULL);

  num_of_msgs[board_type]++;
  return (1);
}

int board_show_board(int board_type, struct char_data *ch, char *arg, struct obj_data *board)
{
  int i;
  char tmp[MAX_STRING_LENGTH], buf[MAX_STRING_LENGTH];

  if (!ch->desc)
    return (0);

  one_argument(arg, tmp);

  if (!*tmp || !isname(tmp, board->name))
    return (0);

  if (GET_LEVEL(ch) < READ_LVL(board_type)) {
    send_to_char(ch, "You try but fail to understand the holy words.\r\n");
    return (1);
  }
  act("$n studies the board.", TRUE, ch, 0, 0, TO_ROOM);

  if (!num_of_msgs[board_type])
    send_to_char(ch, "This is a bulletin board.  Usage: READ/REMOVE <messg #>, WRITE <header>.\r\nThe board is empty.\r\n");
  else {
    size_t len = 0;
    int nlen;

    len = snprintf(buf, sizeof(buf),
		"This is a bulletin board.  Usage: READ/REMOVE <messg #>, WRITE <header>.\r\n"
		"You will need to look at the board to save your message.\r\n"
		"There are %d messages on the board.\r\n",
		num_of_msgs[board_type]);
#if NEWEST_AT_TOP
    for (i = num_of_msgs[board_type] - 1; i >= 0; i--) {
      if (!MSG_HEADING(board_type, i))
        goto fubar;

      nlen = snprintf(buf + len, sizeof(buf) - len, "%-2d : %s\r\n", num_of_msgs[board_type] - i, MSG_HEADING(board_type, i));
      if (len + nlen >= sizeof(buf) || nlen < 0)
        break;
      len += nlen;
    }
#else
    for (i = 0; i < num_of_msgs[board_type]; i++) {
      if (!MSG_HEADING(board_type, i))
        goto fubar;

      nlen = snprintf(buf + len, sizeof(buf) - len, "%-2d : %s\r\n", i + 1, MSG_HEADING(board_type, i));
      if (len + nlen >= sizeof(buf) || nlen < 0)
        break;
      len += nlen;
    }
#endif
    page_string(ch->desc, buf, TRUE);
  }
  return (1);

fubar:
  log("SYSERR: Board %d is fubar'd.", board_type);
  send_to_char(ch, "Sorry, the board isn't working.\r\n");
  return (1);
}

int board_display_msg(int board_type, struct char_data *ch, char *arg, struct obj_data *board)
{
  char number[MAX_INPUT_LENGTH], buffer[MAX_STRING_LENGTH];
  int msg, ind;

  one_argument(arg, number);
  if (!*number)
    return (0);
  if (isname(number, board->name))	/* so "read board" works */
    return (board_show_board(board_type, ch, arg, board));
  if (!is_number(number))	/* read 2.mail, look 2.sword */
    return (0);
  if (!(msg = atoi(number)))
    return (0);

  if (GET_LEVEL(ch) < READ_LVL(board_type)) {
    send_to_char(ch, "You try but fail to understand the holy words.\r\n");
    return (1);
  }
  if (!num_of_msgs[board_type]) {
    send_to_char(ch, "The board is empty!\r\n");
    return (1);
  }
  if (msg < 1 || msg > num_of_msgs[board_type]) {
    send_to_char(ch, "That message exists only in your imagination.\r\n");
    return (1);
  }
#if NEWEST_AT_TOP
  ind = num_of_msgs[board_type] - msg;
#else
  ind = msg - 1;
#endif
  if (MSG_SLOTNUM(board_type, ind) < 0 ||
      MSG_SLOTNUM(board_type, ind) >= INDEX_SIZE) {
    send_to_char(ch, "Sorry, the board is not working.\r\n");
    log("SYSERR: Board is screwed up. (Room #%d)", GET_ROOM_VNUM(IN_ROOM(ch)));
    return (1);
  }
  if (!(MSG_HEADING(board_type, ind))) {
    send_to_char(ch, "That message appears to be screwed up.\r\n");
    return (1);
  }
  if (!(msg_storage[MSG_SLOTNUM(board_type, ind)])) {
    send_to_char(ch, "That message seems to be empty.\r\n");
    return (1);
  }
  snprintf(buffer, sizeof(buffer), "Message %d : %s\r\n\r\n%s\r\n", msg,
	  MSG_HEADING(board_type, ind),
	  msg_storage[MSG_SLOTNUM(board_type, ind)]);

  page_string(ch->desc, buffer, TRUE);

  return (1);
}

int board_remove_msg(int board_type, struct char_data *ch, char *arg, struct obj_data *board)
{
  int ind, msg, slot_num;
  char number[MAX_INPUT_LENGTH], buf[MAX_INPUT_LENGTH];
  struct descriptor_data *d;

  one_argument(arg, number);

  if (!*number || !is_number(number))
    return (0);
  if (!(msg = atoi(number)))
    return (0);

  if (!num_of_msgs[board_type]) {
    send_to_char(ch, "The board is empty!\r\n");
    return (1);
  }
  if (msg < 1 || msg > num_of_msgs[board_type]) {
    send_to_char(ch, "That message exists only in your imagination.\r\n");
    return (1);
  }
#if NEWEST_AT_TOP
  ind = num_of_msgs[board_type] - msg;
#else
  ind = msg - 1;
#endif
  if (!MSG_HEADING(board_type, ind)) {
    send_to_char(ch, "That message appears to be screwed up.\r\n");
    return (1);
  }
  snprintf(buf, sizeof(buf), "(%s)", GET_NAME(ch));
  if (GET_LEVEL(ch) < REMOVE_LVL(board_type) &&
      !(strstr(MSG_HEADING(board_type, ind), buf))) {
    send_to_char(ch, "You are not holy enough to remove other people's messages.\r\n");
    return (1);
  }
  if (GET_LEVEL(ch) < MSG_LEVEL(board_type, ind)) {
    send_to_char(ch, "You can't remove a message holier than yourself.\r\n");
    return (1);
  }
  slot_num = MSG_SLOTNUM(board_type, ind);
  if (slot_num < 0 || slot_num >= INDEX_SIZE) {
    send_to_char(ch, "That message is majorly screwed up.\r\n");
    log("SYSERR: The board is seriously screwed up. (Room #%d)", GET_ROOM_VNUM(IN_ROOM(ch)));
    return (1);
  }
  for (d = descriptor_list; d; d = d->next)
    if (STATE(d) == CON_PLAYING && d->str == &(msg_storage[slot_num])) {
      send_to_char(ch, "At least wait until the author is finished before removing it!\r\n");
      return (1);
    }
  if (msg_storage[slot_num])
    free(msg_storage[slot_num]);
  msg_storage[slot_num] = 0;
  msg_storage_taken[slot_num] = 0;
  if (MSG_HEADING(board_type, ind))
    free(MSG_HEADING(board_type, ind));

  for (; ind < num_of_msgs[board_type] - 1; ind++) {
    MSG_HEADING(board_type, ind) = MSG_HEADING(board_type, ind + 1);
    MSG_SLOTNUM(board_type, ind) = MSG_SLOTNUM(board_type, ind + 1);
    MSG_LEVEL(board_type, ind) = MSG_LEVEL(board_type, ind + 1);
  }
  num_of_msgs[board_type]--;

  send_to_char(ch, "Message removed.\r\n");
  snprintf(buf, sizeof(buf), "$n just removed message %d.", msg);
  act(buf, FALSE, ch, 0, 0, TO_ROOM);
  board_save_board(board_type);

  return (1);
}

void board_save_board(int board_type)
{
  FILE *fl;
  int i, j;
  char *tmp1, *tmp2 = NULL;

  if (!num_of_msgs[board_type]) {
    remove(FILENAME(board_type));
    return;
  }
  if (!(fl = fopen(FILENAME(board_type), "wb"))) {
    perror("SYSERR: Error writing board");
    return;
  }
  j = fwrite(&(num_of_msgs[board_type]), sizeof(int), 1, fl);

  for (i = 0; i < num_of_msgs[board_type]; i++) {
    if ((tmp1 = MSG_HEADING(board_type, i)) != NULL)
      msg_index[board_type][i].heading_len = strlen(tmp1) + 1;
    else
      msg_index[board_type][i].heading_len = 0;

    if (MSG_SLOTNUM(board_type, i) < 0 ||
	MSG_SLOTNUM(board_type, i) >= INDEX_SIZE ||
	(!(tmp2 = msg_storage[MSG_SLOTNUM(board_type, i)])))
      msg_index[board_type][i].message_len = 0;
    else
      msg_index[board_type][i].message_len = strlen(tmp2) + 1;

    j = fwrite(&(msg_index[board_type][i]), sizeof(struct board_msginfo), 1, fl);
    if (tmp1)
      j = fwrite(tmp1, sizeof(char), msg_index[board_type][i].heading_len, fl);
    if (tmp2)
      j = fwrite(tmp2, sizeof(char), msg_index[board_type][i].message_len, fl);
  }

  fclose(fl);
}

void board_load_board(int board_type)
{
  FILE *fl;
  int i, j, len1, len2;
  char *tmp1, *tmp2;

  if (!(fl = fopen(FILENAME(board_type), "rb"))) {
    if (errno != ENOENT)
      perror("SYSERR: Error reading board");
    return;
  }
  if (fread(&(num_of_msgs[board_type]), sizeof(int), 1, fl) != 1)
    return;
  if (num_of_msgs[board_type] < 1 || num_of_msgs[board_type] > MAX_BOARD_MESSAGES) {
    log("SYSERR: Board file %d corrupt.  Resetting.", board_type);
    board_reset_board(board_type);
    return;
  }
  for (i = 0; i < num_of_msgs[board_type]; i++) {
    j = fread(&(msg_index[board_type][i]), sizeof(struct board_msginfo), 1, fl);
    if ((len1 = msg_index[board_type][i].heading_len) <= 0) {
      log("SYSERR: Board file %d corrupt!  Resetting.", board_type);
      board_reset_board(board_type);
      return;
    }
    CREATE(tmp1, char, len1);
    j = fread(tmp1, sizeof(char), len1, fl);
    MSG_HEADING(board_type, i) = tmp1;

    if ((MSG_SLOTNUM(board_type, i) = find_slot()) == -1) {
      log("SYSERR: Out of slots booting board %d!  Resetting...", board_type);
      board_reset_board(board_type);
      return;
    }
    if ((len2 = msg_index[board_type][i].message_len) > 0) {
      CREATE(tmp2, char, len2);
      j = fread(tmp2, sizeof(char), len2, fl);
      msg_storage[MSG_SLOTNUM(board_type, i)] = tmp2;
    } else
      msg_storage[MSG_SLOTNUM(board_type, i)] = NULL;
  }

  fclose(fl);
}

/* When shutting down, clear all boards. */
void board_clear_all(void)
{
  int i;

  for (i = 0; i < NUM_OF_BOARDS; i++)
    board_clear_board(i);
}

/* Clear the in-memory structures. */
void board_clear_board(int board_type)
{
  int i;

  for (i = 0; i < MAX_BOARD_MESSAGES; i++) {
    if (MSG_SLOTNUM(board_type, i) == -1)
      continue; /* don't try to free non-existant slots */
    if (MSG_HEADING(board_type, i))
      free(MSG_HEADING(board_type, i));
    if (msg_storage[MSG_SLOTNUM(board_type, i)])
      free(msg_storage[MSG_SLOTNUM(board_type, i)]);
    msg_storage_taken[MSG_SLOTNUM(board_type, i)] = 0;
    memset((char *)&(msg_index[board_type][i]),0,sizeof(struct board_msginfo));
    msg_index[board_type][i].slot_num = -1;
  }
  num_of_msgs[board_type] = 0;
}

/* Destroy the on-disk and in-memory board. */
static void board_reset_board(int board_type)
{
  board_clear_board(board_type);
  remove(FILENAME(board_type));
}
