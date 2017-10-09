/**************************************************************************
*  File: ban.c                                             Part of tbaMUD *
*  Usage: Banning/unbanning/checking sites and player names.              *
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
#include "handler.h"
#include "db.h"
#include "ban.h"

/* global variables locally defined, used externally */
struct ban_list_element *ban_list = NULL;
int num_invalid = 0;

/* Local (file) scope variables */
#define MAX_INVALID_NAMES 200
static char *invalid_list[MAX_INVALID_NAMES];

/* local utility functions */
static void write_ban_list(void);
static void _write_one_node(FILE *fp, struct ban_list_element *node);

static const char *ban_types[] = {
  "no",
  "new",
  "select",
  "all",
  "ERROR"
};

void load_banned(void)
{
  FILE *fl;
  int i, date;
  char site_name[BANNED_SITE_LENGTH + 1], ban_type[100];
  char name[MAX_NAME_LENGTH + 1];
  struct ban_list_element *next_node;

  ban_list = 0;

  if (!(fl = fopen(BAN_FILE, "r"))) {
    if (errno != ENOENT) {
      log("SYSERR: Unable to open banfile '%s': %s", BAN_FILE, strerror(errno));
    } else
      log("   Ban file '%s' doesn't exist.", BAN_FILE);
    return;
  }
  while (fscanf(fl, " %s %s %d %s ", ban_type, site_name, &date, name) == 4) {
    CREATE(next_node, struct ban_list_element, 1);
    strncpy(next_node->site, site_name, BANNED_SITE_LENGTH);	/* strncpy: OK (n_n->site:BANNED_SITE_LENGTH+1) */
    next_node->site[BANNED_SITE_LENGTH] = '\0';
    strncpy(next_node->name, name, MAX_NAME_LENGTH);	/* strncpy: OK (n_n->name:MAX_NAME_LENGTH+1) */
    next_node->name[MAX_NAME_LENGTH] = '\0';
    next_node->date = date;

    for (i = BAN_NOT; i <= BAN_ALL; i++)
      if (!strcmp(ban_type, ban_types[i]))
	next_node->type = i;

    next_node->next = ban_list;
    ban_list = next_node;
  }

  fclose(fl);
}

int isbanned(char *hostname)
{
  int i;
  struct ban_list_element *banned_node;
  char *nextchar;

  if (!hostname || !*hostname)
    return (0);

  i = 0;
  for (nextchar = hostname; *nextchar; nextchar++)
    *nextchar = LOWER(*nextchar);

  for (banned_node = ban_list; banned_node; banned_node = banned_node->next)
    if (strstr(hostname, banned_node->site))	/* if hostname is a substring */
      i = MAX(i, banned_node->type);

  return (i);
}

static void _write_one_node(FILE *fp, struct ban_list_element *node)
{
  if (node) {
    _write_one_node(fp, node->next);
    fprintf(fp, "%s %s %ld %s\n", ban_types[node->type],
	    node->site, (long) node->date, node->name);
  }
}

static void write_ban_list(void)
{
  FILE *fl;

  if (!(fl = fopen(BAN_FILE, "w"))) {
    perror("SYSERR: Unable to open '" BAN_FILE "' for writing");
    return;
  }
  _write_one_node(fl, ban_list);/* recursively write from end to start */
  fclose(fl);
  return;
}

#define BAN_LIST_FORMAT "%-25.25s  %-8.8s  %-15.15s  %-16.16s\r\n"
ACMD(do_ban)
{
  char flag[MAX_INPUT_LENGTH], site[MAX_INPUT_LENGTH], *nextchar;
  char timestr[16];
  int i;
  struct ban_list_element *ban_node;

  if (!*argument) {
    if (!ban_list) {
      send_to_char(ch, "No sites are banned.\r\n");
      return;
    }
    send_to_char(ch, BAN_LIST_FORMAT,
	    "Banned Site Name",
	    "Ban Type",
	    "Banned On",
	    "Banned By");
    send_to_char(ch, BAN_LIST_FORMAT,
	    "---------------------------------",
	    "---------------------------------",
	    "---------------------------------",
	    "---------------------------------");

    for (ban_node = ban_list; ban_node; ban_node = ban_node->next) {
      if (ban_node->date) {
        strftime(timestr, sizeof(timestr), "%a %b %d %Y", localtime(&(ban_node->date)));
      } else
	strcpy(timestr, "Unknown");	/* strcpy: OK (strlen("Unknown") < 16) */

      send_to_char(ch, BAN_LIST_FORMAT, ban_node->site, ban_types[ban_node->type], timestr, ban_node->name);
    }
    return;
  }

  two_arguments(argument, flag, site);
  if (!*site || !*flag) {
    send_to_char(ch, "Usage: ban {all | select | new} site_name\r\n");
    return;
  }
  if (!(!str_cmp(flag, "select") || !str_cmp(flag, "all") || !str_cmp(flag, "new"))) {
    send_to_char(ch, "Flag must be ALL, SELECT, or NEW.\r\n");
    return;
  }
  for (ban_node = ban_list; ban_node; ban_node = ban_node->next) {
    if (!str_cmp(ban_node->site, site)) {
      send_to_char(ch, "That site has already been banned -- unban it to change the ban type.\r\n");
      return;
    }
  }

  CREATE(ban_node, struct ban_list_element, 1);
  strncpy(ban_node->site, site, BANNED_SITE_LENGTH);	/* strncpy: OK (b_n->site:BANNED_SITE_LENGTH+1) */
  for (nextchar = ban_node->site; *nextchar; nextchar++)
    *nextchar = LOWER(*nextchar);
  ban_node->site[BANNED_SITE_LENGTH] = '\0';
  strncpy(ban_node->name, GET_NAME(ch), MAX_NAME_LENGTH);	/* strncpy: OK (b_n->size:MAX_NAME_LENGTH+1) */
  ban_node->name[MAX_NAME_LENGTH] = '\0';
  ban_node->date = time(0);

  for (i = BAN_NEW; i <= BAN_ALL; i++)
    if (!str_cmp(flag, ban_types[i]))
      ban_node->type = i;

  ban_node->next = ban_list;
  ban_list = ban_node;

  mudlog(NRM, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE, "%s has banned %s for %s players.",
	GET_NAME(ch), site, ban_types[ban_node->type]);
  send_to_char(ch, "Site banned.\r\n");
  write_ban_list();
}
#undef BAN_LIST_FORMAT

ACMD(do_unban)
{
  char site[MAX_INPUT_LENGTH];
  struct ban_list_element *ban_node, *temp;
  int found = 0;

  one_argument(argument, site);
  if (!*site) {
    send_to_char(ch, "A site to unban might help.\r\n");
    return;
  }
  ban_node = ban_list;
  while (ban_node && !found) {
    if (!str_cmp(ban_node->site, site))
      found = 1;
    else
      ban_node = ban_node->next;
  }

  if (!found) {
    send_to_char(ch, "That site is not currently banned.\r\n");
    return;
  }
  REMOVE_FROM_LIST(ban_node, ban_list, next);
  send_to_char(ch, "Site unbanned.\r\n");
  mudlog(NRM, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE, "%s removed the %s-player ban on %s.",
	GET_NAME(ch), ban_types[ban_node->type], ban_node->site);

  free(ban_node);
  write_ban_list();
}



/* Check for invalid names (i.e., profanity, etc.) Written by Sharon P Garza. */
int valid_name(char *newname)
{
  int i, vowels = 0;
  struct descriptor_data *dt;
  char tempname[MAX_INPUT_LENGTH];

  /* Make sure someone isn't trying to create this same name.  We want to do a 
   * 'str_cmp' so people can't do 'Bob' and 'BoB'.  The creating login will not
   * have a character name yet and other people sitting at the prompt won't 
   * have characters yet. New, unindexed characters (i.e., characters who are 
   * in the process of creating) will have an idnum of -1, set by clear_char() 
   * in db.c.  If someone is creating a character by the same name as the one 
   * we are checking, then the name is invalid, to prevent character duping.
   * THIS SHOULD FIX THE 'invalid name' if disconnected from OLC-bug - Welcor */
  for (dt = descriptor_list; dt; dt = dt->next)
    if (dt->character && GET_NAME(dt->character) && !str_cmp(GET_NAME(dt->character), newname))
      if (GET_IDNUM(dt->character) == -1)
        return (IS_PLAYING(dt));

  /* count vowels */
  for (i = 0; newname[i]; i++) {
    if (strchr("aeiouyAEIOUY", newname[i]))
      vowels++;
  }

  /* return invalid if no vowels */
  if (!vowels)
    return (0);

  /* check spaces */
  if (strchr(newname, ' '))
    return (0);

  /* return valid if list doesn't exist */
  if (num_invalid < 1)
    return (1);

  /* change to lowercase */
  strlcpy(tempname, newname, sizeof(tempname));
  for (i = 0; tempname[i]; i++)
    tempname[i] = LOWER(tempname[i]);

  /* Does the desired name contain a string in the invalid list? */
  for (i = 0; i < num_invalid; i++)
    if (strstr(tempname, invalid_list[i]))
      return (0);

  return (1);
}

void free_invalid_list(void)
{
  int invl;

  for (invl = 0; invl < num_invalid; invl++)
    free(invalid_list[invl]);

  num_invalid = 0;
}

void read_invalid_list(void)
{
  FILE *fp;
  char temp[256];

  if (!(fp = fopen(XNAME_FILE, "r"))) {
    perror("SYSERR: Unable to open '" XNAME_FILE "' for reading");
    return;
  }

  num_invalid = 0;
  while (get_line(fp, temp) && num_invalid < MAX_INVALID_NAMES)
    invalid_list[num_invalid++] = strdup(temp);

  if (num_invalid >= MAX_INVALID_NAMES) {
    log("SYSERR: Too many invalid names; change MAX_INVALID_NAMES in ban.c");
    exit(1);
  }

  fclose(fp);
}
