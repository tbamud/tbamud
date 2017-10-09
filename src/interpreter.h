/**
* @file interpreter.h
* Public procs, macro defs, subcommand defines for the command intepreter.
*
* Part of the core tbaMUD source code distribution, which is a derivative
* of, and continuation of, CircleMUD.
*
* All rights reserved.  See license for complete information.
* Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University
* CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.
*/
#ifndef _INTERPRETER_H_
#define _INTERPRETER_H_


/* List of external function prototypes.
 * @todo Organize the functions into modules. */

#define CMD_NAME (complete_cmd_info[cmd].command)
#define CMD_IS(cmd_name) (!strcmp(cmd_name, complete_cmd_info[cmd].command))
#define IS_MOVE(cmdnum) (complete_cmd_info[cmdnum].command_pointer == do_move)

void sort_commands(void);
void	command_interpreter(struct char_data *ch, char *argument);
int	search_block(char *arg, const char **list, int exact);
char	*one_argument(char *argument, char *first_arg);
char	*one_word(char *argument, char *first_arg);
char	*any_one_arg(char *argument, char *first_arg);
char	*two_arguments(char *argument, char *first_arg, char *second_arg);
int	fill_word(char *argument);
int reserved_word(char *argument);
void	half_chop(char *string, char *arg1, char *arg2);
void	nanny(struct descriptor_data *d, char *arg);
int	is_abbrev(const char *arg1, const char *arg2);
int	is_number(const char *str);
int	find_command(const char *command);
void	skip_spaces(char **string);
char	*delete_doubledollar(char *string);
int special(struct char_data *ch, int cmd, char *arg);
void free_alias(struct alias_data *a);
int perform_alias(struct descriptor_data *d, char *orig, size_t maxlen);
int enter_player_game (struct descriptor_data *d);
/* ACMDs available through interpreter.c */
ACMD(do_alias);


/* for compatibility with 2.20: */
#define argument_interpreter(a, b, c) two_arguments(a, b, c)

/* WARNING: if you have added diagonal directions and have them at the
 * beginning of the command list.. change this value to 11 or 15 (depending)
 * reserve these commands to come straight from the cmd list then start
 * sorting */
#define RESERVE_CMDS                7

struct command_info {
   const char *command;
   const char *sort_as;
   byte minimum_position;
   void	(*command_pointer)
	   (struct char_data *ch, char *argument, int cmd, int subcmd);
   sh_int minimum_level;
   int	subcmd;
};

struct mob_script_command_t {
  const char *command_name;
  void (*command_pointer)
         (struct char_data *ch, char *argument, int cmd, int subcmd);
  int subcmd;
};

struct alias_data {
  char *alias;
  char *replacement;
  int type;
  struct alias_data *next;
};

#define ALIAS_SIMPLE	0
#define ALIAS_COMPLEX	1

#define ALIAS_SEP_CHAR	';'
#define ALIAS_VAR_CHAR	'$'
#define ALIAS_GLOB_CHAR	'*'

/* SUBCOMMANDS: You can define these however you want to, and the definitions
 * of the subcommands are independent from function to function.*/
/* directions */

/* do_move
 *
 * Make sure the SCMD_XX directions are mapped
 * to the cardinal directions.
 */
#define SCMD_NORTH   NORTH
#define SCMD_EAST    EAST
#define SCMD_SOUTH   SOUTH
#define SCMD_WEST    WEST
#define SCMD_UP      UP
#define SCMD_DOWN    DOWN
#define SCMD_NW      NORTHWEST
#define SCMD_NE      NORTHEAST
#define SCMD_SE      SOUTHEAST
#define SCMD_SW      SOUTHWEST

/** @deprecated all old do_poof stuff is deprecated and unused. */
#define SCMD_POOFIN   0
/** @deprecated all old do_poof stuff is deprecated and unused. */
#define SCMD_POOFOUT  1

/* do_oasis_Xlist */
#define SCMD_OASIS_RLIST       0
#define SCMD_OASIS_MLIST       1
#define SCMD_OASIS_OLIST       2
#define SCMD_OASIS_SLIST       3
#define SCMD_OASIS_ZLIST       4
#define SCMD_OASIS_TLIST       5
#define SCMD_OASIS_QLIST       6

/* Necessary for CMD_IS macro.  Borland needs the structure defined first
 * so it has been moved down here. */

extern int *cmd_sort_info;
extern struct command_info *complete_cmd_info;
extern const struct command_info cmd_info[];

#endif /* _INTERPRETER_H_ */
