/**************************************************************************
*  File: context_help.c                                    Part of tbaMUD *
*  Usage: Handles the context sensitive help system.                      *
*  By Welcor.                                                             *
**************************************************************************/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "interpreter.h"
#include "oasis.h"
#include "dg_olc.h"

/* Extern functions */
ACMD(do_help);

/* local global */
char *context_help_list[NUM_CONTEXTS];

/* If you add more olc options, be sure to add a case for it here. - Welcor */
int find_context(struct descriptor_data *d)
{
  switch STATE(d) {
    case CON_TRIGEDIT: return find_context_trigedit(d);
    case CON_REDIT:    return find_context_redit(d);
    case CON_MEDIT:    return find_context_medit(d);
    case CON_OEDIT:    return find_context_oedit(d);
    case CON_ZEDIT:    return find_context_zedit(d);
    case CON_SEDIT:    return find_context_sedit(d);
    default:           return NOTHING;
  }
}

int find_context_oedit(struct descriptor_data *d)
{
  switch (OLC_MODE(d)) {
    case OEDIT_MAIN_MENU:             return CONTEXT_OEDIT_MAIN_MENU;
    case OEDIT_KEYWORD:               return CONTEXT_OEDIT_KEYWORD;
    case OEDIT_SHORTDESC:             return CONTEXT_OEDIT_SHORTDESC;
    case OEDIT_LONGDESC:              return CONTEXT_OEDIT_LONGDESC;
    case OEDIT_ACTDESC:               return CONTEXT_OEDIT_ACTDESC;
    case OEDIT_TYPE:                  return CONTEXT_OEDIT_TYPE;
    case OEDIT_EXTRAS:                return CONTEXT_OEDIT_EXTRAS;
    case OEDIT_WEAR:                  return CONTEXT_OEDIT_WEAR;
    case OEDIT_WEIGHT:                return CONTEXT_OEDIT_WEIGHT;
    case OEDIT_COST:                  return CONTEXT_OEDIT_COST;
    case OEDIT_COSTPERDAY:            return CONTEXT_OEDIT_COSTPERDAY;
    case OEDIT_TIMER:                 return CONTEXT_OEDIT_TIMER;
    case OEDIT_VALUE_1:               return CONTEXT_OEDIT_VALUE_1;
    case OEDIT_VALUE_2:               return CONTEXT_OEDIT_VALUE_2;
    case OEDIT_VALUE_3:               return CONTEXT_OEDIT_VALUE_3;
    case OEDIT_VALUE_4:               return CONTEXT_OEDIT_VALUE_4;
    case OEDIT_APPLY:                 return CONTEXT_OEDIT_APPLY;
    case OEDIT_APPLYMOD:              return CONTEXT_OEDIT_APPLYMOD;
    case OEDIT_EXTRADESC_KEY:         return CONTEXT_OEDIT_EXTRADESC_KEY;
    case OEDIT_CONFIRM_SAVEDB:        return CONTEXT_OEDIT_CONFIRM_SAVEDB;
    case OEDIT_CONFIRM_SAVESTRING:    return CONTEXT_OEDIT_CONFIRM_SAVESTRING;
    case OEDIT_PROMPT_APPLY:          return CONTEXT_OEDIT_PROMPT_APPLY;
    case OEDIT_EXTRADESC_DESCRIPTION: return CONTEXT_OEDIT_EXTRADESC_DESCRIPTION;
    case OEDIT_EXTRADESC_MENU:        return CONTEXT_OEDIT_EXTRADESC_MENU;
    case OEDIT_LEVEL:                 return CONTEXT_OEDIT_LEVEL;
    case OEDIT_PERM:                  return CONTEXT_OEDIT_PERM;
    case OLC_SCRIPT_EDIT:             return find_context_script_edit(d);
    default: return NOTHING;
  }
}

int find_context_redit(struct descriptor_data *d)
{
  switch (OLC_MODE(d)) {
    case REDIT_MAIN_MENU:            return CONTEXT_REDIT_MAIN_MENU;
    case REDIT_NAME:                 return CONTEXT_REDIT_NAME;
    case REDIT_DESC:                 return CONTEXT_REDIT_DESC;
    case REDIT_FLAGS:                return CONTEXT_REDIT_FLAGS;
    case REDIT_SECTOR:               return CONTEXT_REDIT_SECTOR;
    case REDIT_EXIT_MENU:            return CONTEXT_REDIT_EXIT_MENU;
    case REDIT_CONFIRM_SAVEDB:       return CONTEXT_REDIT_CONFIRM_SAVEDB;
    case REDIT_CONFIRM_SAVESTRING:   return CONTEXT_REDIT_CONFIRM_SAVESTRING;
    case REDIT_EXIT_NUMBER:          return CONTEXT_REDIT_EXIT_NUMBER;
    case REDIT_EXIT_DESCRIPTION:     return CONTEXT_REDIT_EXIT_DESCRIPTION;
    case REDIT_EXIT_KEYWORD:         return CONTEXT_REDIT_EXIT_KEYWORD;
    case REDIT_EXIT_KEY:             return CONTEXT_REDIT_EXIT_KEY;
    case REDIT_EXIT_DOORFLAGS:       return CONTEXT_REDIT_EXIT_DOORFLAGS;
    case REDIT_EXTRADESC_MENU:       return CONTEXT_REDIT_EXTRADESC_MENU;
    case REDIT_EXTRADESC_KEY:        return CONTEXT_REDIT_EXTRADESC_KEY;
    case REDIT_EXTRADESC_DESCRIPTION:return CONTEXT_REDIT_EXTRADESC_DESCRIPTION;
    case OLC_SCRIPT_EDIT:            return find_context_script_edit(d);
    default: return NOTHING;
  }
}

int find_context_zedit(struct descriptor_data *d)
{
  switch (OLC_MODE(d)) {
    case ZEDIT_MAIN_MENU:         return CONTEXT_ZEDIT_MAIN_MENU;
    case ZEDIT_DELETE_ENTRY:      return CONTEXT_ZEDIT_DELETE_ENTRY;
    case ZEDIT_NEW_ENTRY:         return CONTEXT_ZEDIT_NEW_ENTRY;
    case ZEDIT_CHANGE_ENTRY:      return CONTEXT_ZEDIT_CHANGE_ENTRY;
    case ZEDIT_COMMAND_TYPE:      return CONTEXT_ZEDIT_COMMAND_TYPE;
    case ZEDIT_IF_FLAG:           return CONTEXT_ZEDIT_IF_FLAG;
    case ZEDIT_ARG1:              return CONTEXT_ZEDIT_ARG1;
    case ZEDIT_ARG2:              return CONTEXT_ZEDIT_ARG2;
    case ZEDIT_ARG3:              return CONTEXT_ZEDIT_ARG3;
    case ZEDIT_ZONE_NAME:         return CONTEXT_ZEDIT_ZONE_NAME;
    case ZEDIT_ZONE_LIFE:         return CONTEXT_ZEDIT_ZONE_LIFE;
    case ZEDIT_ZONE_BOT:          return CONTEXT_ZEDIT_ZONE_BOT;
    case ZEDIT_ZONE_TOP:          return CONTEXT_ZEDIT_ZONE_TOP;
    case ZEDIT_ZONE_RESET:        return CONTEXT_ZEDIT_ZONE_RESET;
    case ZEDIT_CONFIRM_SAVESTRING:return CONTEXT_ZEDIT_CONFIRM_SAVESTRING;
    case ZEDIT_SARG1:             return CONTEXT_ZEDIT_SARG1;
    case ZEDIT_SARG2:             return CONTEXT_ZEDIT_SARG2;
    default: return NOTHING;
  }
}

int find_context_medit(struct descriptor_data *d)
{
  switch (OLC_MODE(d)) {
    case MEDIT_MAIN_MENU:         return CONTEXT_MEDIT_MAIN_MENU;
    case MEDIT_KEYWORD:           return CONTEXT_MEDIT_KEYWORD;
    case MEDIT_S_DESC:            return CONTEXT_MEDIT_S_DESC;
    case MEDIT_L_DESC:            return CONTEXT_MEDIT_L_DESC;
    case MEDIT_D_DESC:            return CONTEXT_MEDIT_D_DESC;
    case MEDIT_NPC_FLAGS:         return CONTEXT_MEDIT_NPC_FLAGS;
    case MEDIT_AFF_FLAGS:         return CONTEXT_MEDIT_AFF_FLAGS;
    case MEDIT_CONFIRM_SAVESTRING:return CONTEXT_MEDIT_CONFIRM_SAVESTRING;
    case MEDIT_SEX:               return CONTEXT_MEDIT_SEX;
    case MEDIT_HITROLL:           return CONTEXT_MEDIT_HITROLL;
    case MEDIT_DAMROLL:           return CONTEXT_MEDIT_DAMROLL;
    case MEDIT_NDD:               return CONTEXT_MEDIT_NDD;
    case MEDIT_SDD:               return CONTEXT_MEDIT_SDD;
    case MEDIT_NUM_HP_DICE:       return CONTEXT_MEDIT_NUM_HP_DICE;
    case MEDIT_SIZE_HP_DICE:      return CONTEXT_MEDIT_SIZE_HP_DICE;
    case MEDIT_ADD_HP:            return CONTEXT_MEDIT_ADD_HP;
    case MEDIT_AC:                return CONTEXT_MEDIT_AC;
    case MEDIT_EXP:               return CONTEXT_MEDIT_EXP;
    case MEDIT_GOLD:              return CONTEXT_MEDIT_GOLD;
    case MEDIT_POS:               return CONTEXT_MEDIT_POS;
    case MEDIT_DEFAULT_POS:       return CONTEXT_MEDIT_DEFAULT_POS;
    case MEDIT_ATTACK:            return CONTEXT_MEDIT_ATTACK;
    case MEDIT_LEVEL:             return CONTEXT_MEDIT_LEVEL;
    case MEDIT_ALIGNMENT:         return CONTEXT_MEDIT_ALIGNMENT;
    case OLC_SCRIPT_EDIT:         return find_context_script_edit(d);
    default: return NOTHING;
  }
}

int find_context_sedit(struct descriptor_data *d)
{
  switch (OLC_MODE(d)) {
    case SEDIT_MAIN_MENU:         return CONTEXT_SEDIT_MAIN_MENU;
    case SEDIT_CONFIRM_SAVESTRING:return CONTEXT_SEDIT_CONFIRM_SAVESTRING;
    case SEDIT_NOITEM1:           return CONTEXT_SEDIT_NOITEM1;
    case SEDIT_NOITEM2:           return CONTEXT_SEDIT_NOITEM2;
    case SEDIT_NOCASH1:           return CONTEXT_SEDIT_NOCASH1;
    case SEDIT_NOCASH2:           return CONTEXT_SEDIT_NOCASH2;
    case SEDIT_NOBUY:             return CONTEXT_SEDIT_NOBUY;
    case SEDIT_BUY:               return CONTEXT_SEDIT_BUY;
    case SEDIT_SELL:              return CONTEXT_SEDIT_SELL;
    case SEDIT_PRODUCTS_MENU:     return CONTEXT_SEDIT_PRODUCTS_MENU;
    case SEDIT_ROOMS_MENU:        return CONTEXT_SEDIT_ROOMS_MENU;
    case SEDIT_NAMELIST_MENU:     return CONTEXT_SEDIT_NAMELIST_MENU;
    case SEDIT_NAMELIST:          return CONTEXT_SEDIT_NAMELIST;
    case SEDIT_OPEN1:             return CONTEXT_SEDIT_OPEN1;
    case SEDIT_OPEN2:             return CONTEXT_SEDIT_OPEN2;
    case SEDIT_CLOSE1:            return CONTEXT_SEDIT_CLOSE1;
    case SEDIT_CLOSE2:            return CONTEXT_SEDIT_CLOSE2;
    case SEDIT_KEEPER:            return CONTEXT_SEDIT_KEEPER;
    case SEDIT_BUY_PROFIT:        return CONTEXT_SEDIT_BUY_PROFIT;
    case SEDIT_SELL_PROFIT:       return CONTEXT_SEDIT_SELL_PROFIT;
    case SEDIT_TYPE_MENU:         return CONTEXT_SEDIT_TYPE_MENU;
    case SEDIT_DELETE_TYPE:       return CONTEXT_SEDIT_DELETE_TYPE;
    case SEDIT_DELETE_PRODUCT:    return CONTEXT_SEDIT_DELETE_PRODUCT;
    case SEDIT_NEW_PRODUCT:       return CONTEXT_SEDIT_NEW_PRODUCT;
    case SEDIT_DELETE_ROOM:       return CONTEXT_SEDIT_DELETE_ROOM;
    case SEDIT_NEW_ROOM:          return CONTEXT_SEDIT_NEW_ROOM;
    case SEDIT_SHOP_FLAGS:        return CONTEXT_SEDIT_SHOP_FLAGS;
    case SEDIT_NOTRADE:           return CONTEXT_SEDIT_NOTRADE;
    default: return NOTHING;
  }
}

int find_context_trigedit(struct descriptor_data *d)
{
  switch (OLC_MODE(d)) {
    case TRIGEDIT_MAIN_MENU:         return CONTEXT_TRIGEDIT_MAIN_MENU;
    case TRIGEDIT_TRIGTYPE:          return CONTEXT_TRIGEDIT_TRIGTYPE;
    case TRIGEDIT_CONFIRM_SAVESTRING:return CONTEXT_TRIGEDIT_CONFIRM_SAVESTRING;
    case TRIGEDIT_NAME:              return CONTEXT_TRIGEDIT_NAME;
    case TRIGEDIT_INTENDED:          return CONTEXT_TRIGEDIT_INTENDED;
    case TRIGEDIT_TYPES:             return CONTEXT_TRIGEDIT_TYPES;
    case TRIGEDIT_COMMANDS:          return CONTEXT_TRIGEDIT_COMMANDS;
    case TRIGEDIT_NARG:              return CONTEXT_TRIGEDIT_NARG;
    case TRIGEDIT_ARGUMENT:          return CONTEXT_TRIGEDIT_ARGUMENT;
    default:                         return NOTHING;
  }
}

int find_context_script_edit(struct descriptor_data *d)
{
  switch (OLC_SCRIPT_EDIT_MODE(d)) {
    case SCRIPT_MAIN_MENU:  return CONTEXT_SCRIPT_MAIN_MENU;
    case SCRIPT_NEW_TRIGGER:return CONTEXT_SCRIPT_NEW_TRIGGER;
    case SCRIPT_DEL_TRIGGER:return CONTEXT_SCRIPT_DEL_TRIGGER;
    default:                return NOTHING;
  }
}

char *NO_HELP = "No help available (yet)!\r\n";
#define FIND_HELP_CHAR '*'

int context_help(struct descriptor_data *d, char *arg)
{
  int context = NOTHING;
  char actbuf[MAX_INPUT_LENGTH], *tmp;
  /* skip if context help isn't wanted */
  if (strncmp(arg, CONTEXT_HELP_STRING, strlen(CONTEXT_HELP_STRING)))
    return FALSE;

  tmp = one_argument(arg, actbuf); /* the totally useless 'help' string.. */
  skip_spaces(&tmp);

  if (!*tmp)
    context = find_context(d);

  if (context != NOTHING && context < NUM_CONTEXTS && *context_help_list[context]) {
    if (*context_help_list[context] == FIND_HELP_CHAR) {
      strncpy(actbuf, context_help_list[context]+1, sizeof(actbuf)-1);
      do_help(d->character, actbuf, 0, 0);
    } else {
      write_to_output(d, "\r\n%s\r\n>  ", context_help_list[context]);
    }
    return TRUE;
  }
  strncpy(actbuf, tmp, sizeof(actbuf)-1);
  do_help(d->character, actbuf, 0, 0);
  return TRUE;
}

void boot_context_help(void) {
  int i, num;
  FILE *fl;
  char line[READ_SIZE];

  fl = fopen(CONTEXT_HELP_FILE, "r");

  /* init to 'no help' string */
  for (i=0;i < NUM_CONTEXTS;i++)
      context_help_list[i] = NO_HELP;

  if (!fl) {
    log("No context help found : %s", strerror(errno));
    return;
  }
  while (get_line(fl, line)) {
    if (sscanf(line, "#%d *", &num) == 1 && num >= 0 && num < NUM_CONTEXTS) /* got a number, now get the text */
      context_help_list[num] = fread_string(fl, "Context sensitive help");
  }

  fclose(fl);
}

void free_context_help(void) {
  int i;

  for (i = 0; i < NUM_CONTEXTS; i++)
    if (context_help_list[i] && context_help_list[i] != NO_HELP)
      free(context_help_list[i]);
}
