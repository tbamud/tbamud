/**************************************************************************
*  File: shop.c                                            Part of tbaMUD *
*  Usage: Shopkeepers, loading config files, spec procs.                  *
*                                                                         *
*  All rights reserved.  See license for complete information.            *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
*  By Jeff Fink.                                                          *
**************************************************************************/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "handler.h"
#include "db.h"
#include "interpreter.h"
#include "utils.h"
#include "shop.h"
#include "genshp.h"
#include "constants.h"
#include "act.h"
#include "modify.h"
#include "spells.h"  /* for skill_name() */
#include "screen.h"

/* Global variables definitions used externally */
/* Constant list for printing out who we sell to */
const char *trade_letters[] = {
        "Good",                 /* First, the alignment based ones */
        "Evil",
        "Neutral",
        "Magic User",           /* Then the class based ones */
        "Cleric",
        "Thief",
        "Warrior",
        "\n"
};

const char *shop_bits[] = {
        "WILL_FIGHT",
        "USES_BANK",
        "UNLIMITED_CASH",
        "\n"
};


/* local (file scope) function prototypes  */
static void push(struct stack_data *stack, int pushval); /**< @todo Move to utils.c */
static int top(struct stack_data *stack); /**< @todo Move to utils.c */
static int pop(struct stack_data *stack); /**< @todo Move to utils.c */
static char *list_object(struct obj_data *obj, int cnt, int oindex, int shop_nr, struct char_data *keeper, struct char_data *seller);
static void sort_keeper_objs(struct char_data *keeper, int shop_nr);
static char *read_shop_message(int mnum, room_vnum shr, FILE *shop_f, const char *why);
static int read_type_list(FILE *shop_f, struct shop_buy_data *list, int new_format, int max);
static int read_list(FILE *shop_f, struct shop_buy_data *list, int new_format, int max, int type);
static void shopping_list(char *arg, struct char_data *ch, struct char_data *keeper, int shop_nr);
static bool shopping_identify(char *arg, struct char_data *ch, struct char_data *keeper, int shop_nr);
static void shopping_value(char *arg, struct char_data *ch, struct char_data *keeper, int shop_nr);
static void shopping_sell(char *arg, struct char_data *ch, struct char_data *keeper, int shop_nr);
static struct obj_data *get_selling_obj(struct char_data *ch, char *name, struct char_data *keeper, int shop_nr, int msg);
static struct obj_data *slide_obj(struct obj_data *obj, struct char_data *keeper, int shop_nr);
static void shopping_buy(char *arg, struct char_data *ch, struct char_data *keeper, int shop_nr);
static struct obj_data *get_purchase_obj(struct char_data *ch, char *arg, struct char_data *keeper, int shop_nr, int msg);
static struct obj_data *get_hash_obj_vis(struct char_data *ch, char *name, struct obj_data *list);
static struct obj_data *get_slide_obj_vis(struct char_data *ch, char *name, struct obj_data *list);
static char *customer_string(int shop_nr, int detailed);
static void list_all_shops(struct char_data *ch);
static void list_detailed_shop(struct char_data *ch, int shop_nr);
static int is_ok_char(struct char_data *keeper, struct char_data *ch, int shop_nr);
static int is_open(struct char_data *keeper, int shop_nr, int msg);
static int is_ok(struct char_data *keeper, struct char_data *ch, int shop_nr);
static void evaluate_operation(struct stack_data *ops, struct stack_data *vals);
static int find_oper_num(char token);
static int evaluate_expression(struct obj_data *obj, char *expr);
static int trade_with(struct obj_data *item, int shop_nr);
static int same_obj(struct obj_data *obj1, struct obj_data *obj2);
static int shop_producing(struct obj_data *item, int shop_nr);
static int transaction_amt(char *arg);
static char *times_message(struct obj_data *obj, char *name, int num);
static int buy_price(struct obj_data *obj, int shop_nr, struct char_data *keeper, struct char_data *buyer);
static int sell_price(struct obj_data *obj, int shop_nr, struct char_data *keeper, struct char_data *seller);
static int ok_shop_room(int shop_nr, room_vnum room);
static int add_to_shop_list(struct shop_buy_data *list, int type, int *len, int *val);
static int end_read_list(struct shop_buy_data *list, int len, int error);
static void read_line(FILE *shop_f, const char *string, void *data);

/* Local file scope only variables */
static int cmd_say;
static int cmd_tell;
static int cmd_emote;
static int cmd_slap;
static int cmd_puke;

/* config arrays */
static const char *operator_str[] = {
        "[({",
        "])}",
        "|+",
        "&*",
        "^'"
} ;


static int is_ok_char(struct char_data *keeper, struct char_data *ch, int shop_nr)
{
  char buf[MAX_INPUT_LENGTH];

  if (!CAN_SEE(keeper, ch)) {
    char actbuf[MAX_INPUT_LENGTH] = MSG_NO_SEE_CHAR;
    do_say(keeper, actbuf, cmd_say, 0);
    return (FALSE);
  }
  if (IS_GOD(ch))
    return (TRUE);

  if ((IS_GOOD(ch) && NOTRADE_GOOD(shop_nr)) ||
      (IS_EVIL(ch) && NOTRADE_EVIL(shop_nr)) ||
      (IS_NEUTRAL(ch) && NOTRADE_NEUTRAL(shop_nr))) {
    snprintf(buf, sizeof(buf), "%s %s", GET_NAME(ch), MSG_NO_SELL_ALIGN);
    do_tell(keeper, buf, cmd_tell, 0);
    return (FALSE);
  }
  if (IS_NPC(ch))
    return (TRUE);

  if ((IS_MAGIC_USER(ch) && NOTRADE_MAGIC_USER(shop_nr)) ||
      (IS_CLERIC(ch) && NOTRADE_CLERIC(shop_nr)) ||
      (IS_THIEF(ch) && NOTRADE_THIEF(shop_nr)) ||
      (IS_WARRIOR(ch) && NOTRADE_WARRIOR(shop_nr))) {
    snprintf(buf, sizeof(buf), "%s %s", GET_NAME(ch), MSG_NO_SELL_CLASS);
    do_tell(keeper, buf, cmd_tell, 0);
    return (FALSE);
  }
  return (TRUE);
}

static int is_open(struct char_data *keeper, int shop_nr, int msg)
{
  char buf[MAX_INPUT_LENGTH];

  *buf = '\0';
  if (SHOP_OPEN1(shop_nr) > time_info.hours)
    strlcpy(buf, MSG_NOT_OPEN_YET, sizeof(buf));
  else if (SHOP_CLOSE1(shop_nr) < time_info.hours) {
    if (SHOP_OPEN2(shop_nr) > time_info.hours)
      strlcpy(buf, MSG_NOT_REOPEN_YET, sizeof(buf));
    else if (SHOP_CLOSE2(shop_nr) < time_info.hours)
      strlcpy(buf, MSG_CLOSED_FOR_DAY, sizeof(buf));
  }
  if (!*buf)
    return (TRUE);
  if (msg)
    do_say(keeper, buf, cmd_tell, 0);
  return (FALSE);
}

static int is_ok(struct char_data *keeper, struct char_data *ch, int shop_nr)
{
  if (is_open(keeper, shop_nr, TRUE))
    return (is_ok_char(keeper, ch, shop_nr));
  else
    return (FALSE);
}

static void push(struct stack_data *stack, int pushval)
{
  S_DATA(stack, S_LEN(stack)++) = pushval;
}

static int top(struct stack_data *stack)
{
  if (S_LEN(stack) > 0)
    return (S_DATA(stack, S_LEN(stack) - 1));
  else
    return (-1);
}

static int pop(struct stack_data *stack)
{
  if (S_LEN(stack) > 0)
    return (S_DATA(stack, --S_LEN(stack)));
  else {
    log("SYSERR: Illegal expression %d in shop keyword list.", S_LEN(stack));
    return (0);
  }
}

static void evaluate_operation(struct stack_data *ops, struct stack_data *vals)
{
  int oper;

  if ((oper = pop(ops)) == OPER_NOT)
    push(vals, !pop(vals));
  else {
    int val1 = pop(vals),
	val2 = pop(vals);

    /* Compiler would previously short-circuit these. */
    if (oper == OPER_AND)
      push(vals, val1 && val2);
    else if (oper == OPER_OR)
      push(vals, val1 || val2);
  }
}

static int find_oper_num(char token)
{
  int oindex;

  for (oindex = 0; oindex <= MAX_OPER; oindex++)
    if (strchr(operator_str[oindex], token))
      return (oindex);
  return (NOTHING);
}

static int evaluate_expression(struct obj_data *obj, char *expr)
{
  struct stack_data ops, vals;
  char *ptr, *end, name[MAX_STRING_LENGTH];
  int temp, eindex;

  if (!expr || !*expr)	/* Allows opening ( first. */
    return (TRUE);

  ops.len = vals.len = 0;
  ptr = expr;
  while (*ptr) {
    if (isspace(*ptr))
      ptr++;
    else {
      if ((temp = find_oper_num(*ptr)) == NOTHING) {
	end = ptr;
	while (*ptr && !isspace(*ptr) && find_oper_num(*ptr) == NOTHING)
	  ptr++;
	strncpy(name, end, ptr - end);	/* strncpy: OK (name/end:MAX_STRING_LENGTH) */
	name[ptr - end] = '\0';
	for (eindex = 0; *extra_bits[eindex] != '\n'; eindex++)
	  if (!str_cmp(name, extra_bits[eindex])) {
	    push(&vals, OBJ_FLAGGED(obj, eindex));
	    break;
	  }
	if (*extra_bits[eindex] == '\n')
	  push(&vals, isname(name, obj->name));
      } else {
	if (temp != OPER_OPEN_PAREN)
	  while (top(&ops) > temp)
	    evaluate_operation(&ops, &vals);

	if (temp == OPER_CLOSE_PAREN) {
	  if ((temp = pop(&ops)) != OPER_OPEN_PAREN) {
	    log("SYSERR: Illegal parenthesis in shop keyword expression.");
	    return (FALSE);
	  }
	} else
	  push(&ops, temp);
	ptr++;
      }
    }
  }
  while (top(&ops) != -1)
    evaluate_operation(&ops, &vals);
  temp = pop(&vals);
  if (top(&vals) != -1) {
    log("SYSERR: Extra operands left on shop keyword expression stack.");
    return (FALSE);
  }
  return (temp);
}

static int trade_with(struct obj_data *item, int shop_nr)
{
  int counter;

  if (GET_OBJ_COST(item) < 1)
    return (OBJECT_NOVAL);

  if (OBJ_FLAGGED(item, ITEM_NOSELL))
    return (OBJECT_NOTOK);

  for (counter = 0; SHOP_BUYTYPE(shop_nr, counter) != NOTHING; counter++)
    if (SHOP_BUYTYPE(shop_nr, counter) == GET_OBJ_TYPE(item)) {
      if (GET_OBJ_VAL(item, 2) == 0 &&
		(GET_OBJ_TYPE(item) == ITEM_WAND ||
		 GET_OBJ_TYPE(item) == ITEM_STAFF))
	return (OBJECT_DEAD);
      else if (evaluate_expression(item, SHOP_BUYWORD(shop_nr, counter)))
	return (OBJECT_OK);
    }
  return (OBJECT_NOTOK);
}

static int same_obj(struct obj_data *obj1, struct obj_data *obj2)
{
  int aindex;

  if (!obj1 || !obj2)
    return (obj1 == obj2);

  if (GET_OBJ_RNUM(obj1) != GET_OBJ_RNUM(obj2))
    return (FALSE);

  if (GET_OBJ_COST(obj1) != GET_OBJ_COST(obj2))
    return (FALSE);

  for (aindex = 0; aindex < MAX_OBJ_AFFECT; aindex++)
    if ((obj1->affected[aindex].location != obj2->affected[aindex].location) ||
	(obj1->affected[aindex].modifier != obj2->affected[aindex].modifier))
      return (FALSE);

  return (TRUE);
}

static int shop_producing(struct obj_data *item, int shop_nr)
{
  int counter;

  if (GET_OBJ_RNUM(item) == NOTHING)
    return (FALSE);

  for (counter = 0; SHOP_PRODUCT(shop_nr, counter) != NOTHING; counter++)
    if (same_obj(item, &obj_proto[SHOP_PRODUCT(shop_nr, counter)]))
      return (TRUE);
  return (FALSE);
}

static int transaction_amt(char *arg)
{
  char buf[MAX_INPUT_LENGTH];

  char *buywhat;

  /* If we have two arguments, it means 'buy 5 3', or buy 5 of #3. We don't do
   * that if we only have one argument, like 'buy 5', buy #5. By Andrey Fidrya */
  buywhat = one_argument(arg, buf);
  if (*buywhat && *buf && is_number(buf)) {
    strcpy(arg, arg + strlen(buf) + 1);	/* strcpy: OK (always smaller) */
    return (atoi(buf));
  }
  return (1);
}

static char *times_message(struct obj_data *obj, char *name, int num)
{
  static char buf[256];
  size_t len;
  char *ptr;

  if (obj)
    len = strlcpy(buf, obj->short_description, sizeof(buf));
  else {
    if ((ptr = strchr(name, '.')) == NULL)
      ptr = name;
    else
      ptr++;
    len = snprintf(buf, sizeof(buf), "%s %s", AN(ptr), ptr);
  }

  if (num > 1 && len < sizeof(buf))
    snprintf(buf + len, sizeof(buf) - len, " (x %d)", num);

  return (buf);
}

static struct obj_data *get_slide_obj_vis(struct char_data *ch, char *name, struct obj_data *list)
{
  struct obj_data *i, *last_match = NULL;
  int j, number;
  char tmpname[MAX_INPUT_LENGTH];
  char *tmp;

  strlcpy(tmpname, name, sizeof(tmpname));
  tmp = tmpname;
  if (!(number = get_number(&tmp)))
    return (NULL);

  for (i = list, j = 1; i && (j <= number); i = i->next_content)
    if (isname(tmp, i->name))
      if (CAN_SEE_OBJ(ch, i) && !same_obj(last_match, i)) {
	if (j == number)
	  return (i);
	last_match = i;
	j++;
      }
  return (NULL);
}

static struct obj_data *get_hash_obj_vis(struct char_data *ch, char *name, struct obj_data *list)
{
  struct obj_data *loop, *last_obj = NULL;
  int qindex;

  if (is_number(name))
    qindex = atoi(name);
  else if (is_number(name + 1))
    qindex = atoi(name + 1);
  else
    return (NULL);

  for (loop = list; loop; loop = loop->next_content)
    if (CAN_SEE_OBJ(ch, loop) && GET_OBJ_COST(loop) > 0)
      if (!same_obj(last_obj, loop)) {
	if (--qindex == 0)
	  return (loop);
	last_obj = loop;
      }
  return (NULL);
}

static struct obj_data *get_purchase_obj(struct char_data *ch, char *arg, struct char_data *keeper, int shop_nr, int msg)
{
  char name[MAX_INPUT_LENGTH];
  struct obj_data *obj;

  one_argument(arg, name);
  do {
    if (*name == '#' || is_number(name))
      obj = get_hash_obj_vis(ch, name, keeper->carrying);
    else
      obj = get_slide_obj_vis(ch, name, keeper->carrying);
    if (!obj) {
      if (msg) {
        char buf[MAX_INPUT_LENGTH];

	snprintf(buf, sizeof(buf), shop_index[shop_nr].no_such_item1, GET_NAME(ch));
	do_tell(keeper, buf, cmd_tell, 0);
      }
      return (NULL);
    }
    if (GET_OBJ_COST(obj) <= 0) {
      extract_obj(obj);
      obj = NULL;
    }
  } while (!obj);
  return (obj);
}

/* Shop purchase adjustment, based on charisma-difference from buyer to keeper.
   for i in `seq 15 -15`; do printf " * %3d: %6.4f\n" $i \
   `echo "scale=4; 1+$i/70" | bc`; done
   Shopkeeper higher charisma (markup)
   ^  15: 1.2142  14: 1.2000  13: 1.1857  12: 1.1714  11: 1.1571
   |  10: 1.1428   9: 1.1285   8: 1.1142   7: 1.1000   6: 1.0857
   |   5: 1.0714   4: 1.0571   3: 1.0428   2: 1.0285   1: 1.0142
   +   0: 1.0000
   |  -1: 0.9858  -2: 0.9715  -3: 0.9572  -4: 0.9429  -5: 0.9286
   |  -6: 0.9143  -7: 0.9000  -8: 0.8858  -9: 0.8715 -10: 0.8572
   v -11: 0.8429 -12: 0.8286 -13: 0.8143 -14: 0.8000 -15: 0.7858
   Player higher charisma (discount)
   Most mobiles have 11 charisma so an 18 charisma player would get a 10%
   discount beyond the basic price.  That assumes they put a lot of points
   into charisma, because on the flip side they'd get 11% inflation by
   having a 3. */
static int buy_price(struct obj_data *obj, int shop_nr, struct char_data *keeper, struct char_data *buyer)
{
  return (int) (GET_OBJ_COST(obj) * SHOP_BUYPROFIT(shop_nr)
	* (1 + (GET_CHA(keeper) - GET_CHA(buyer)) / (float)70));
}

/* When the shopkeeper is buying, we reverse the discount. Also make sure
   we don't buy for more than we sell for, to prevent infinite money-making. */
static int sell_price(struct obj_data *obj, int shop_nr, struct char_data *keeper, struct char_data *seller)
{
  float sell_cost_modifier = SHOP_SELLPROFIT(shop_nr) * (1 - (GET_CHA(keeper) - GET_CHA(seller)) / 70.0);
  float buy_cost_modifier = SHOP_BUYPROFIT(shop_nr) * (1 + (GET_CHA(keeper) - GET_CHA(seller)) / 70.0);

  if (sell_cost_modifier > buy_cost_modifier)
    sell_cost_modifier = buy_cost_modifier;

  return (int) (GET_OBJ_COST(obj) * sell_cost_modifier);
}

static void shopping_buy(char *arg, struct char_data *ch, struct char_data *keeper, int shop_nr)
{
  char tempstr[MAX_INPUT_LENGTH], tempbuf[MAX_INPUT_LENGTH];
  struct obj_data *obj, *last_obj = NULL;
  int goldamt = 0, buynum, bought = 0;

  if (!is_ok(keeper, ch, shop_nr))
    return;

  if (SHOP_SORT(shop_nr) < IS_CARRYING_N(keeper))
    sort_keeper_objs(keeper, shop_nr);

  if ((buynum = transaction_amt(arg)) < 0) {
    char buf[MAX_INPUT_LENGTH];

    snprintf(buf, sizeof(buf), "%s A negative amount?  Try selling me something.",
	    GET_NAME(ch));
    do_tell(keeper, buf, cmd_tell, 0);
    return;
  }
  if (!*arg || !buynum) {
    char buf[MAX_INPUT_LENGTH];

    snprintf(buf, sizeof(buf), "%s What do you want to buy??", GET_NAME(ch));
    do_tell(keeper, buf, cmd_tell, 0);
    return;
  }
  if (!(obj = get_purchase_obj(ch, arg, keeper, shop_nr, TRUE)))
    return;

  if (OBJ_FLAGGED(obj, ITEM_QUEST)) {
    if (GET_OBJ_COST(obj) > GET_QUESTPOINTS(ch) && !IS_GOD(ch)) {
      char actbuf[MAX_INPUT_LENGTH];
      snprintf(actbuf, sizeof(actbuf),
        "%s You haven't earned enough quest points for such an item.",
        GET_NAME(ch));
      do_tell(keeper, actbuf, cmd_tell, 0);
      return;
    }
  } else { /*has the player got enough gold? */
  if (buy_price(obj, shop_nr, keeper, ch) > GET_GOLD(ch) && !IS_GOD(ch)) {
    char actbuf[MAX_INPUT_LENGTH];

    snprintf(actbuf, sizeof(actbuf), shop_index[shop_nr].missing_cash2, GET_NAME(ch));
    do_tell(keeper, actbuf, cmd_tell, 0);

    switch (SHOP_BROKE_TEMPER(shop_nr)) {
    case 0:
      do_action(keeper, strcpy(actbuf, GET_NAME(ch)), cmd_puke, 0);	/* strcpy: OK (MAX_NAME_LENGTH < MAX_INPUT_LENGTH) */
      return;
    case 1:
      do_echo(keeper, strcpy(actbuf, "smokes on his joint."), cmd_emote, SCMD_EMOTE);	/* strcpy: OK */
      return;
    default:
      return;
    }
  }
  }
  
  if (IS_NPC(ch) || (!IS_NPC(ch) && !PRF_FLAGGED(ch, PRF_NOHASSLE))) {
	if (IS_CARRYING_N(ch) + 1 > CAN_CARRY_N(ch)) {
      send_to_char(ch, "%s: You can't carry any more items.\r\n", fname(obj->name));
	  return;
    }
    if (IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(obj) > CAN_CARRY_W(ch)) {
	  send_to_char(ch, "%s: You can't carry that much weight.\r\n", fname(obj->name));
      return;
	}
  }
  
  if (OBJ_FLAGGED(obj, ITEM_QUEST)) {
    while (obj &&
           (GET_QUESTPOINTS(ch) >= GET_OBJ_COST(obj) || IS_GOD(ch))
    && IS_CARRYING_N(ch) < CAN_CARRY_N(ch)
    && bought < buynum
    && IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(obj) <= CAN_CARRY_W(ch)) {
      bought++;
      /* Test if producing shop ! */
      if (shop_producing(obj, shop_nr)) {
        obj = read_object(GET_OBJ_RNUM(obj), REAL);
      } else {
        obj_from_char(obj);
        SHOP_SORT(shop_nr)--;
      }
      obj_to_char(obj, ch);

      goldamt += GET_OBJ_COST(obj);
      if (!IS_GOD(ch))
        GET_QUESTPOINTS(ch) -= GET_OBJ_COST(obj);

      last_obj = obj;
      obj = get_purchase_obj(ch, arg, keeper, shop_nr, FALSE);
      if (!same_obj(obj, last_obj))
        break;
    }
  } else {
  while (obj && (GET_GOLD(ch) >= buy_price(obj, shop_nr, keeper, ch) || IS_GOD(ch))
	 && IS_CARRYING_N(ch) < CAN_CARRY_N(ch) && bought < buynum
	 && IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(obj) <= CAN_CARRY_W(ch)) {
    int charged;

    bought++;
    /* Test if producing shop ! */
    if (shop_producing(obj, shop_nr))
      obj = read_object(GET_OBJ_RNUM(obj), REAL);
    else {
      obj_from_char(obj);
      SHOP_SORT(shop_nr)--;
    }
    obj_to_char(obj, ch);

    charged = buy_price(obj, shop_nr, keeper, ch);
    goldamt += charged;
    if (!IS_GOD(ch))
      decrease_gold(ch, charged);

    last_obj = obj;
    obj = get_purchase_obj(ch, arg, keeper, shop_nr, FALSE);
    if (!same_obj(obj, last_obj))
      break;
  }
  }
  if (bought < buynum) {
    char buf[MAX_INPUT_LENGTH];

    if (!obj || !same_obj(last_obj, obj))
      snprintf(buf, sizeof(buf), "%s I only have %d to sell you.", GET_NAME(ch), bought);
    else if (!OBJ_FLAGGED(obj, ITEM_QUEST) &&
      GET_GOLD(ch) < buy_price(obj, shop_nr, keeper, ch))
      snprintf(buf, sizeof(buf), "%s You can only afford %d.", GET_NAME(ch), bought);
    else if (OBJ_FLAGGED(obj, ITEM_QUEST) &&
      GET_QUESTPOINTS(ch) < GET_OBJ_COST(obj))
      snprintf(buf, sizeof(buf), "%s You only had sufficient quest points for %d.",
        GET_NAME(ch), bought);
    else if (IS_CARRYING_N(ch) >= CAN_CARRY_N(ch))
      snprintf(buf, sizeof(buf), "%s You can only hold %d.", GET_NAME(ch), bought);
    else if (IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(obj) > CAN_CARRY_W(ch))
      snprintf(buf, sizeof(buf), "%s You can only carry %d.", GET_NAME(ch), bought);
    else
      snprintf(buf, sizeof(buf), "%s Something screwy only gave you %d.", GET_NAME(ch), bought);
    do_tell(keeper, buf, cmd_tell, 0);
  }
  if (!IS_GOD(ch) && obj && !OBJ_FLAGGED(obj, ITEM_QUEST)) {
    increase_gold(keeper, goldamt);
    if (SHOP_USES_BANK(shop_nr))
      if (GET_GOLD(keeper) > MAX_OUTSIDE_BANK) {
        SHOP_BANK(shop_nr) += (GET_GOLD(keeper) - MAX_OUTSIDE_BANK);
        GET_GOLD(keeper) = MAX_OUTSIDE_BANK;
      }
  }
  strlcpy(tempstr, times_message(ch->carrying, 0, bought), sizeof(tempstr));

  snprintf(tempbuf, sizeof(tempbuf), "$n buys %s.", tempstr);
  act(tempbuf, FALSE, ch, obj, 0, TO_ROOM);

  if (obj && OBJ_FLAGGED(obj, ITEM_QUEST))
    snprintf(tempbuf, sizeof(tempbuf), "%s That has cost you %d quest points.", GET_NAME(ch), goldamt);
  else
    snprintf(tempbuf, sizeof(tempbuf), shop_index[shop_nr].message_buy, GET_NAME(ch), goldamt);

  do_tell(keeper, tempbuf, cmd_tell, 0);

  send_to_char(ch, "You now have %s.\r\n", tempstr);

}

static struct obj_data *get_selling_obj(struct char_data *ch, char *name, struct char_data *keeper, int shop_nr, int msg)
{
  char buf[MAX_INPUT_LENGTH];
  struct obj_data *obj;
  int result;

  if (!(obj = get_obj_in_list_vis(ch, name, NULL, ch->carrying))) {
    if (msg) {
      char tbuf[MAX_INPUT_LENGTH];

      snprintf(tbuf, sizeof(tbuf), shop_index[shop_nr].no_such_item2, GET_NAME(ch));
      do_tell(keeper, tbuf, cmd_tell, 0);
    }
    return (NULL);
  }
  if ((result = trade_with(obj, shop_nr)) == OBJECT_OK)
    return (obj);

  if (!msg)
    return (0);

  switch (result) {
  case OBJECT_NOVAL:
    snprintf(buf, sizeof(buf), "%s You've got to be kidding, that thing is worthless!", GET_NAME(ch));
    break;
  case OBJECT_NOTOK:
    snprintf(buf, sizeof(buf), shop_index[shop_nr].do_not_buy, GET_NAME(ch));
    break;
  case OBJECT_DEAD:
    snprintf(buf, sizeof(buf), "%s %s", GET_NAME(ch), MSG_NO_USED_WANDSTAFF);
    break;
  default:
    log("SYSERR: Illegal return value of %d from trade_with() (%s)", result, __FILE__);	/* Someone might rename it... */
    snprintf(buf, sizeof(buf), "%s An error has occurred.", GET_NAME(ch));
    break;
  }
  do_tell(keeper, buf, cmd_tell, 0);
  return (NULL);
}

/* This function is a slight hack!  To make sure that duplicate items are only
 * listed once on the "list", this function groups "identical" objects together
 * on the shopkeeper's inventory list.  The hack involves knowing how the list
 * is put together, and manipulating the order of the objects on the list. (But
 * since most of DIKU is not encapsulated, and information hiding is almost
 * never used, it isn't that big a deal). -JF */
static struct obj_data *slide_obj(struct obj_data *obj, struct char_data *keeper, int shop_nr)
{
  struct obj_data *loop;
  int temp;

  if (SHOP_SORT(shop_nr) < IS_CARRYING_N(keeper))
    sort_keeper_objs(keeper, shop_nr);

  /* Extract the object if it is identical to one produced */
  if (shop_producing(obj, shop_nr)) {
    temp = GET_OBJ_RNUM(obj);
    extract_obj(obj);
    return (&obj_proto[temp]);
  }
  SHOP_SORT(shop_nr)++;
  loop = keeper->carrying;
  obj_to_char(obj, keeper);
  keeper->carrying = loop;
  while (loop) {
    if (same_obj(obj, loop)) {
      obj->next_content = loop->next_content;
      loop->next_content = obj;
      return (obj);
    }
    loop = loop->next_content;
  }
  keeper->carrying = obj;
  return (obj);
}

static void sort_keeper_objs(struct char_data *keeper, int shop_nr)
{
  struct obj_data *list = NULL, *temp;

  while (SHOP_SORT(shop_nr) < IS_CARRYING_N(keeper)) {
    temp = keeper->carrying;
    obj_from_char(temp);
    temp->next_content = list;
    list = temp;
  }

  while (list) {
    temp = list;
    list = list->next_content;
    if (shop_producing(temp, shop_nr) &&
	!get_obj_in_list_num(GET_OBJ_RNUM(temp), keeper->carrying)) {
      obj_to_char(temp, keeper);
      SHOP_SORT(shop_nr)++;
    } else
      slide_obj(temp, keeper, shop_nr);
  }
}

static void shopping_sell(char *arg, struct char_data *ch, struct char_data *keeper, int shop_nr)
{
  char tempstr[MAX_INPUT_LENGTH], name[MAX_INPUT_LENGTH], tempbuf[MAX_INPUT_LENGTH];
  struct obj_data *obj;
  int sellnum, sold = 0, goldamt = 0;

  if (!(is_ok(keeper, ch, shop_nr)))
    return;

  if ((sellnum = transaction_amt(arg)) < 0) {
    char buf[MAX_INPUT_LENGTH];

    snprintf(buf, sizeof(buf), "%s A negative amount?  Try buying something.", GET_NAME(ch));
    do_tell(keeper, buf, cmd_tell, 0);
    return;
  }
  if (!*arg || !sellnum) {
    char buf[MAX_INPUT_LENGTH];

    snprintf(buf, sizeof(buf), "%s What do you want to sell??", GET_NAME(ch));
    do_tell(keeper, buf, cmd_tell, 0);
    return;
  }
  one_argument(arg, name);
  if (!(obj = get_selling_obj(ch, name, keeper, shop_nr, TRUE)))
    return;

  if (!IS_SET(SHOP_BITVECTOR(shop_nr), HAS_UNLIMITED_CASH) && GET_GOLD(keeper) + SHOP_BANK(shop_nr) < sell_price(obj, shop_nr, keeper, ch)) {
    char buf[MAX_INPUT_LENGTH];

    snprintf(buf, sizeof(buf), shop_index[shop_nr].missing_cash1, GET_NAME(ch));
    do_tell(keeper, buf, cmd_tell, 0);
    return;
  }
  while (obj && (IS_SET(SHOP_BITVECTOR(shop_nr), HAS_UNLIMITED_CASH) || GET_GOLD(keeper) + SHOP_BANK(shop_nr) >= sell_price(obj, shop_nr, keeper, ch)) && sold < sellnum) {
    int charged = sell_price(obj, shop_nr, keeper, ch);

    goldamt += charged;
    if (!IS_SET(SHOP_BITVECTOR(shop_nr), HAS_UNLIMITED_CASH))
      decrease_gold(keeper, charged);

    sold++;
    obj_from_char(obj);
    slide_obj(obj, keeper, shop_nr);	/* Seems we don't use return value. */
    obj = get_selling_obj(ch, name, keeper, shop_nr, FALSE);
  }

  if (sold < sellnum) {
    char buf[MAX_INPUT_LENGTH];

    if (!obj)
      snprintf(buf, sizeof(buf), "%s You only have %d of those.", GET_NAME(ch), sold);
    else if (GET_GOLD(keeper) + SHOP_BANK(shop_nr) < sell_price(obj, shop_nr, keeper, ch))
      snprintf(buf, sizeof(buf), "%s I can only afford to buy %d of those.", GET_NAME(ch), sold);
    else
      snprintf(buf, sizeof(buf), "%s Something really screwy made me buy %d.", GET_NAME(ch), sold);

    do_tell(keeper, buf, cmd_tell, 0);
  }
  increase_gold(ch, goldamt);

  strlcpy(tempstr, times_message(0, name, sold), sizeof(tempstr));
  snprintf(tempbuf, sizeof(tempbuf), "$n sells %s.", tempstr);
  act(tempbuf, FALSE, ch, obj, 0, TO_ROOM);

  snprintf(tempbuf, sizeof(tempbuf), shop_index[shop_nr].message_sell, GET_NAME(ch), goldamt);
  do_tell(keeper, tempbuf, cmd_tell, 0);

  send_to_char(ch, "The shopkeeper now has %s.\r\n", tempstr);

  if (GET_GOLD(keeper) < MIN_OUTSIDE_BANK) {
    goldamt = MIN(MAX_OUTSIDE_BANK - GET_GOLD(keeper), SHOP_BANK(shop_nr));
    SHOP_BANK(shop_nr) -= goldamt;
    increase_gold(keeper, goldamt);
  }
}

static void shopping_value(char *arg, struct char_data *ch, struct char_data *keeper, int shop_nr)
{
  char buf[MAX_STRING_LENGTH], name[MAX_INPUT_LENGTH];
  struct obj_data *obj;

  if (!is_ok(keeper, ch, shop_nr))
    return;

  if (!*arg) {
    snprintf(buf, sizeof(buf), "%s What do you want me to evaluate??", GET_NAME(ch));
    do_tell(keeper, buf, cmd_tell, 0);
    return;
  }
  one_argument(arg, name);
  if (!(obj = get_selling_obj(ch, name, keeper, shop_nr, TRUE)))
    return;

  snprintf(buf, sizeof(buf), "%s I'll give you %d gold coins for that!", GET_NAME(ch), sell_price(obj, shop_nr, keeper, ch));
  do_tell(keeper, buf, cmd_tell, 0);
}

static char *list_object(struct obj_data *obj, int cnt, int aindex, int shop_nr, struct char_data *keeper, struct char_data *ch)
{
  static char result[256];
  char	itemname[128],
	quantity[16];	/* "Unlimited" or "%d" */

  if (shop_producing(obj, shop_nr))
    strcpy(quantity, "Unlimited");	/* strcpy: OK (for 'quantity >= 10') */
  else
    sprintf(quantity, "%d", cnt);	/* sprintf: OK (for 'quantity >= 11', 32-bit int) */

  switch (GET_OBJ_TYPE(obj)) {
  case ITEM_DRINKCON:
    if (GET_OBJ_VAL(obj, 1))
      snprintf(itemname, sizeof(itemname), "%s of %s", obj->short_description, drinks[GET_OBJ_VAL(obj, 2)]);
    else
      strlcpy(itemname, obj->short_description, sizeof(itemname));
    break;

  case ITEM_WAND:
  case ITEM_STAFF:
    snprintf(itemname, sizeof(itemname), "%s%s", obj->short_description,
	GET_OBJ_VAL(obj, 2) < GET_OBJ_VAL(obj, 1) ? " (partially used)" : "");
    break;

  default:
    strlcpy(itemname, obj->short_description, sizeof(itemname));
    break;
  }
  CAP(itemname);

  snprintf(result, sizeof(result), " %2d)  %9s   %-*s %6d%s\r\n",
      aindex, quantity, count_color_chars(itemname)+48, itemname,
      buy_price(obj, shop_nr, keeper, ch), OBJ_FLAGGED(obj, ITEM_QUEST) ? " qp" : "");

  return (result);
}

static void shopping_list(char *arg, struct char_data *ch, struct char_data *keeper, int shop_nr)
{
  char buf[MAX_STRING_LENGTH], name[MAX_INPUT_LENGTH];
  struct obj_data *obj, *last_obj = NULL;
  int cnt = 0, lindex = 0, found = FALSE, has_quest = FALSE;
  size_t len;
  /* cnt is the number of that particular object available */
  /* has_quest indicates if the shopkeeper sells quest items */

  if (!is_ok(keeper, ch, shop_nr))
    return;

  if (SHOP_SORT(shop_nr) < IS_CARRYING_N(keeper))
    sort_keeper_objs(keeper, shop_nr);

  one_argument(arg, name);

  len = strlcpy(buf,   " ##   Available   Item                                               Cost\r\n"
      "----------------------------------------------------------------------------\r\n", sizeof(buf));
  if (keeper->carrying)
    for (obj = keeper->carrying; obj; obj = obj->next_content)
      if (CAN_SEE_OBJ(ch, obj) && GET_OBJ_COST(obj) > 0) {
	if (!last_obj) {
	  last_obj = obj;
	  cnt = 1;
	} else if (same_obj(last_obj, obj))
	  cnt++;
	else {
	  lindex++;
	  if (!*name || isname(name, last_obj->name)) {
	    strncat(buf, list_object(last_obj, cnt, lindex, shop_nr, keeper, ch), sizeof(buf) - len - 1);	/* strncat: OK */
            len = strlen(buf);
            if (len + 1 >= sizeof(buf))
              break;
            found = TRUE;
            if (OBJ_FLAGGED(last_obj, ITEM_QUEST))
              has_quest = TRUE;
	  }
	  cnt = 1;
	  last_obj = obj;
	}
      }
  lindex++;
  if (!last_obj)	/* we actually have nothing in our list for sale, period */
    send_to_char(ch, "Currently, there is nothing for sale.\r\n");
  else if (*name && !found)	/* nothing the char was looking for was found */
    send_to_char(ch, "Presently, none of those are for sale.\r\n");
  else {
    if (!*name || isname(name, last_obj->name))	/* show last obj */
      if (len < sizeof(buf))
        strncat(buf, list_object(last_obj, cnt, lindex, shop_nr, keeper, ch), sizeof(buf) - len - 1);	/* strncat: OK */
    page_string(ch->desc, buf, TRUE);
    if (has_quest)
      send_to_char(ch, "Items flagged \"qp\" require quest points to purchase.\r\n");
  }
}

static int ok_shop_room(int shop_nr, room_vnum room)
{
  int mindex;

  for (mindex = 0; SHOP_ROOM(shop_nr, mindex) != NOWHERE; mindex++)
    if (SHOP_ROOM(shop_nr, mindex) == room)
      return (TRUE);
  return (FALSE);
}

SPECIAL(shop_keeper)
{
  struct char_data *keeper = (struct char_data *)me;
  int shop_nr;

  for (shop_nr = 0; shop_nr <= top_shop; shop_nr++)
    if (SHOP_KEEPER(shop_nr) == keeper->nr)
      break;

  if (shop_nr > top_shop)
    return (FALSE);

  if (SHOP_FUNC(shop_nr))	/* Check secondary function */
    if ((SHOP_FUNC(shop_nr)) (ch, me, cmd, argument))
      return (TRUE);

  if (keeper == ch) {
    if (cmd)
      SHOP_SORT(shop_nr) = 0;	/* Safety in case "drop all" */
    return (FALSE);
  }
  if (!ok_shop_room(shop_nr, GET_ROOM_VNUM(IN_ROOM(ch))))
    return (0);

  if (!AWAKE(keeper))
    return (FALSE);

  if (CMD_IS("steal")) {
    char argm[MAX_INPUT_LENGTH];

    snprintf(argm, sizeof(argm), "$N shouts '%s'", MSG_NO_STEAL_HERE);
    act(argm, FALSE, ch, 0, keeper, TO_CHAR);

    do_action(keeper, GET_NAME(ch), cmd_slap, 0);
    return (TRUE);
  }

  if (CMD_IS("buy")) {
    shopping_buy(argument, ch, keeper, shop_nr);
    return (TRUE);
  } else if (CMD_IS("sell")) {
    shopping_sell(argument, ch, keeper, shop_nr);
    return (TRUE);
  } else if (CMD_IS("value")) {
    shopping_value(argument, ch, keeper, shop_nr);
    return (TRUE);
  } else if (CMD_IS("list")) {
    shopping_list(argument, ch, keeper, shop_nr);
    return (TRUE);
  } else if (CMD_IS("identify")) {
    return (shopping_identify(argument, ch, keeper, shop_nr));
  }
  return (FALSE);
}

int ok_damage_shopkeeper(struct char_data *ch, struct char_data *victim)
{
  int sindex;

  if (!IS_MOB(victim) || mob_index[GET_MOB_RNUM(victim)].func != shop_keeper)
    return (TRUE);

  /* Prevent "invincible" shopkeepers if they're charmed. */
  if (AFF_FLAGGED(victim, AFF_CHARM))
    return (TRUE);

  for (sindex = 0; sindex <= top_shop; sindex++)
    if (GET_MOB_RNUM(victim) == SHOP_KEEPER(sindex) && !SHOP_KILL_CHARS(sindex)) {
      char buf[MAX_INPUT_LENGTH];

      snprintf(buf, sizeof(buf), "%s %s", GET_NAME(ch), MSG_CANT_KILL_KEEPER);
      do_tell(victim, buf, cmd_tell, 0);

      do_action(victim, GET_NAME(ch), cmd_slap, 0);
      return (FALSE);
    }

  return (TRUE);
}

/* val == obj_vnum and obj_rnum (?) */
static int add_to_shop_list(struct shop_buy_data *list, int type, int *len, int *val)
{
  if (*val != NOTHING && *val >= 0) { /* necessary after changing to unsigned v/rnums -- Welcor */
    if (*len < MAX_SHOP_OBJ) {
      if (type == LIST_PRODUCE)
	*val = real_object(*val);
      if (*val != NOTHING) {
	BUY_TYPE(list[*len]) = *val;
	BUY_WORD(list[(*len)++]) = NULL;
      } else
	*val = NOTHING;
      return (FALSE);
    } else
      return (TRUE);
  }
  return (FALSE);
}

static int end_read_list(struct shop_buy_data *list, int len, int error)
{
  if (error)
    log("SYSERR: Raise MAX_SHOP_OBJ constant in shop.h to %d", len + error);
  BUY_WORD(list[len]) = NULL;
  BUY_TYPE(list[len++]) = NOTHING;
  return (len);
}

static void read_line(FILE *shop_f, const char *string, void *data)
{
  char buf[READ_SIZE];

  if (!get_line(shop_f, buf) || !sscanf(buf, string, data)) {
    log("SYSERR: Error in shop #%d, near '%s' with '%s'", SHOP_NUM(top_shop), buf, string);
    exit(1);
  }
}

static int read_list(FILE *shop_f, struct shop_buy_data *list, int new_format,
	          int max, int type)
{
  int count, temp, len = 0, error = 0;

  if (new_format) {
    for (;;) {
      read_line(shop_f, "%d", &temp);
      if (temp < 0)	/* Always "-1" the string. */
        break;
      error += add_to_shop_list(list, type, &len, &temp);
    }
  } else
    for (count = 0; count < max; count++) {
      read_line(shop_f, "%d", &temp);
      error += add_to_shop_list(list, type, &len, &temp);
    }
  return (end_read_list(list, len, error));
}

/* END_OF inefficient. */
static int read_type_list(FILE *shop_f, struct shop_buy_data *list,
		       int new_format, int max)
{
  int tindex, num, len = 0, error = 0;
  char *ptr, buf[MAX_STRING_LENGTH], *buf1;

  if (!new_format)
    return (read_list(shop_f, list, 0, max, LIST_TRADE));

  do {
    buf1 = fgets(buf, sizeof(buf), shop_f);
    if ((ptr = strchr(buf, ';')) != NULL)
      *ptr = '\0';
    else
      *(END_OF(buf) - 1) = '\0';

    num = -1;

    if (strncmp(buf, "-1", 2) != 0)
      for (tindex = 0; *item_types[tindex] != '\n'; tindex++)
        if (!strn_cmp(item_types[tindex], buf, strlen(item_types[tindex]))) {
          num = tindex;
          strcpy(buf, buf + strlen(item_types[tindex]));	/* strcpy: OK (always smaller) */
          break;
        }

    ptr = buf;
    if (num == -1) {
      sscanf(buf, "%d", &num);
      while (!isdigit(*ptr))
	ptr++;
      while (isdigit(*ptr))
	ptr++;
    }
    while (isspace(*ptr))
      ptr++;
    while (isspace(*(END_OF(ptr) - 1)))
      *(END_OF(ptr) - 1) = '\0';
    error += add_to_shop_list(list, LIST_TRADE, &len, &num);
    if (*ptr)
      BUY_WORD(list[len - 1]) = strdup(ptr);
  } while (num >= 0);
  return (end_read_list(list, len, error));
}

static char *read_shop_message(int mnum, room_vnum shr, FILE *shop_f, const char *why)
{
  int cht, ss = 0, ds = 0, err = 0;
  char *tbuf;

  if (!(tbuf = fread_string(shop_f, why)))
    return (NULL);

  for (cht = 0; tbuf[cht]; cht++) {
    if (tbuf[cht] != '%')
      continue;

    if (tbuf[cht + 1] == 's')
      ss++;
    else if (tbuf[cht + 1] == 'd' && (mnum == 5 || mnum == 6)) {
      if (ss == 0) {
        log("SYSERR: Shop #%d has %%d before %%s, message #%d.", shr, mnum);
        err++;
      }
      ds++;
    } else if (tbuf[cht + 1] != '%') {
      log("SYSERR: Shop #%d has invalid format '%%%c' in message #%d.", shr, tbuf[cht + 1], mnum);
      err++;
    }
  }

  if (ss > 1 || ds > 1) {
    log("SYSERR: Shop #%d has too many specifiers for message #%d. %%s=%d %%d=%d", shr, mnum, ss, ds);
    err++;
  }

  if (err) {
    free(tbuf);
    return (NULL);
  }
  return (tbuf);
}

void boot_the_shops(FILE *shop_f, char *filename, int rec_count)
{
  char *buf, buf2[256];
  int temp, count, new_format = FALSE;
  struct shop_buy_data list[MAX_SHOP_OBJ + 1];
  int done = FALSE;

  snprintf(buf2, sizeof(buf2), "beginning of shop file %s", filename);

  while (!done) {
    buf = fread_string(shop_f, buf2);
    if (*buf == '#') {		/* New shop */
      sscanf(buf, "#%d\n", &temp);
      snprintf(buf2, sizeof(buf2), "shop #%d in shop file %s", temp, filename);
      free(buf);		/* Plug memory leak! */
      top_shop++;
      if (!top_shop)
	CREATE(shop_index, struct shop_data, rec_count);
      SHOP_NUM(top_shop) = temp;
      temp = read_list(shop_f, list, new_format, MAX_PROD, LIST_PRODUCE);
      CREATE(shop_index[top_shop].producing, obj_vnum, temp);
      for (count = 0; count < temp; count++)
	SHOP_PRODUCT(top_shop, count) = BUY_TYPE(list[count]);

      read_line(shop_f, "%f", &SHOP_BUYPROFIT(top_shop));
      read_line(shop_f, "%f", &SHOP_SELLPROFIT(top_shop));

      temp = read_type_list(shop_f, list, new_format, MAX_TRADE);
      CREATE(shop_index[top_shop].type, struct shop_buy_data, temp);
      for (count = 0; count < temp; count++) {
	SHOP_BUYTYPE(top_shop, count) = BUY_TYPE(list[count]);
	SHOP_BUYWORD(top_shop, count) = BUY_WORD(list[count]);
      }

      shop_index[top_shop].no_such_item1 = read_shop_message(0, SHOP_NUM(top_shop), shop_f, buf2);
      shop_index[top_shop].no_such_item2 = read_shop_message(1, SHOP_NUM(top_shop), shop_f, buf2);
      shop_index[top_shop].do_not_buy = read_shop_message(2, SHOP_NUM(top_shop), shop_f, buf2);
      shop_index[top_shop].missing_cash1 = read_shop_message(3, SHOP_NUM(top_shop), shop_f, buf2);
      shop_index[top_shop].missing_cash2 = read_shop_message(4, SHOP_NUM(top_shop), shop_f, buf2);
      shop_index[top_shop].message_buy = read_shop_message(5, SHOP_NUM(top_shop), shop_f, buf2);
      shop_index[top_shop].message_sell = read_shop_message(6, SHOP_NUM(top_shop), shop_f, buf2);
      read_line(shop_f, "%d", &SHOP_BROKE_TEMPER(top_shop));
      read_line(shop_f, "%ld", &SHOP_BITVECTOR(top_shop));
      read_line(shop_f, "%hd", &SHOP_KEEPER(top_shop));

      SHOP_KEEPER(top_shop) = real_mobile(SHOP_KEEPER(top_shop));
      read_line(shop_f, "%d", &SHOP_TRADE_WITH(top_shop));

      temp = read_list(shop_f, list, new_format, 1, LIST_ROOM);
      CREATE(shop_index[top_shop].in_room, room_vnum, temp);
      for (count = 0; count < temp; count++)
	SHOP_ROOM(top_shop, count) = BUY_TYPE(list[count]);

      read_line(shop_f, "%d", &SHOP_OPEN1(top_shop));
      read_line(shop_f, "%d", &SHOP_CLOSE1(top_shop));
      read_line(shop_f, "%d", &SHOP_OPEN2(top_shop));
      read_line(shop_f, "%d", &SHOP_CLOSE2(top_shop));

      SHOP_BANK(top_shop) = 0;
      SHOP_SORT(top_shop) = 0;
      SHOP_FUNC(top_shop) = NULL;
    } else {
      if (*buf == '$')		/* EOF */
	done = TRUE;
      else if (strstr(buf, VERSION3_TAG))	/* New format marker */
	new_format = TRUE;
      free(buf);		/* Plug memory leak! */
    }
  }
}

void assign_the_shopkeepers(void)
{
  int cindex;

  cmd_say = find_command("say");
  cmd_tell = find_command("tell");
  cmd_emote = find_command("emote");
  cmd_slap = find_command("slap");
  cmd_puke = find_command("puke");

  for (cindex = 0; cindex <= top_shop; cindex++) {
    if (SHOP_KEEPER(cindex) == NOBODY)
      continue;

    if (SHOP_KEEPER(cindex) > top_of_mobt) {
    	log ("shop %d had mob out of bounds", cindex);
    	abort();
    }


    /* Having SHOP_FUNC() as 'shop_keeper' will cause infinite recursion. */
    if (mob_index[SHOP_KEEPER(cindex)].func && mob_index[SHOP_KEEPER(cindex)].func != shop_keeper)
      SHOP_FUNC(cindex) = mob_index[SHOP_KEEPER(cindex)].func;

    mob_index[SHOP_KEEPER(cindex)].func = shop_keeper;

  }
}

static char *customer_string(int shop_nr, int detailed)
{
  int sindex = 0, flag = 1, nlen;
  size_t len = 0;
  static char buf[256];

  while (*trade_letters[sindex] != '\n' && len + 1 < sizeof(buf)) {
    if (detailed) {
      if (!IS_SET(flag, SHOP_TRADE_WITH(shop_nr))) {
	nlen = snprintf(buf + len, sizeof(buf) - len, ", %s", trade_letters[sindex]);

        if (len + nlen >= sizeof(buf) || nlen < 0)
          break;

        len += nlen;
      }
    } else {
      buf[len++] = (IS_SET(flag, SHOP_TRADE_WITH(shop_nr)) ? '_' : *trade_letters[sindex]);
      buf[len] = '\0';

      if (len >= sizeof(buf))
        break;
    }

    sindex++;
    flag <<= 1;
  }

  buf[sizeof(buf) - 1] = '\0';
  return (buf);
}

/* END_OF inefficient */
static void list_all_shops(struct char_data *ch)
{
  const char *list_all_shops_header =
	" ##   Virtual   Where    Keeper    Buy   Sell   Customers\r\n"
	"---------------------------------------------------------\r\n";
  int shop_nr, headerlen = strlen(list_all_shops_header);
  size_t len = 0;
  char buf[MAX_STRING_LENGTH], buf1[16];

  *buf = '\0';
  for (shop_nr = 0; shop_nr <= top_shop && len < sizeof(buf); shop_nr++) {
    /* New page in page_string() mechanism, print the header again. */
    if (!(shop_nr % (PAGE_LENGTH - 2))) {
      /*
       * If we don't have enough room for the header, or all we have room left
       * for is the header, then don't add it and just quit now.
       */
      if (len + headerlen + 1 >= sizeof(buf))
        break;
      strcpy(buf + len, list_all_shops_header);	/* strcpy: OK (length checked above) */
      len += headerlen;
    }

    if (SHOP_KEEPER(shop_nr) == NOBODY)
      strcpy(buf1, "<NONE>");	/* strcpy: OK (for 'buf1 >= 7') */
    else
      sprintf(buf1, "%6d", mob_index[SHOP_KEEPER(shop_nr)].vnum);	/* sprintf: OK (for 'buf1 >= 11', 32-bit int) */

    len += snprintf(buf + len, sizeof(buf) - len,
        "%3d   %6d   %6d    %s   %3.2f   %3.2f    %s\r\n",
        shop_nr + 1, SHOP_NUM(shop_nr), SHOP_ROOM(shop_nr, 0), buf1,
        SHOP_SELLPROFIT(shop_nr), SHOP_BUYPROFIT(shop_nr),
        customer_string(shop_nr, FALSE));
  }

  page_string(ch->desc, buf, TRUE);
}

static void list_detailed_shop(struct char_data *ch, int shop_nr)
{
  struct char_data *k;
  int sindex, column, flag = 1, found = 0;
  /* char *ptrsave; */

  send_to_char(ch, "Vnum:       [%5d], Rnum: [%5d]\r\n", SHOP_NUM(shop_nr), shop_nr + 1);

  send_to_char(ch, "Rooms:      ");
  column = 12;	/* ^^^ strlen ^^^ */
  for (sindex = 0; SHOP_ROOM(shop_nr, sindex) != NOWHERE; sindex++) {
    char buf1[128];
    int linelen, temp;

    if (sindex) {
      send_to_char(ch, ", ");
      column += 2;
    }

    if ((temp = real_room(SHOP_ROOM(shop_nr, sindex))) != NOWHERE)
      linelen = snprintf(buf1, sizeof(buf1), "%s (#%d)", world[temp].name, GET_ROOM_VNUM(temp));
    else
      linelen = snprintf(buf1, sizeof(buf1), "<UNKNOWN> (#%d)", SHOP_ROOM(shop_nr, sindex));

    /* Implementing word-wrapping: assumes screen-size == 80 */
    if (linelen + column >= 78 && column >= 20) {
      send_to_char(ch, "\r\n            ");
      /* 12 is to line up with "Rooms:" printed first, and spaces above. */
      column = 12;
    }

    if (!send_to_char(ch, "%s", buf1))
      return;
    column += linelen;
  }
  if (!sindex)
    send_to_char(ch, "Rooms:      None!");

  send_to_char(ch, "\r\nShopkeeper: ");
  if (SHOP_KEEPER(shop_nr) != NOBODY) {
    send_to_char(ch, "%s (#%d), Special Function: %s\r\n",
	GET_NAME(&mob_proto[SHOP_KEEPER(shop_nr)]),
	mob_index[SHOP_KEEPER(shop_nr)].vnum,
	YESNO(SHOP_FUNC(shop_nr)));

    if ((k = get_char_num(SHOP_KEEPER(shop_nr))))
      send_to_char(ch, "Coins:      [%9d], Bank: [%9d] (Total: %d)\r\n",
	 GET_GOLD(k), SHOP_BANK(shop_nr), GET_GOLD(k) + SHOP_BANK(shop_nr));
  } else
    send_to_char(ch, "<NONE>\r\n");

  /* send_to_char(ch, "Customers:  %s\r\n", (ptrsave = customer_string(shop_nr, TRUE)) ? ptrsave : "None"); */
  send_to_char(ch, "Customers:  ");
  column = 12;  /* ^^^ strlen ^^^ */
  for (sindex = 0; *trade_letters[sindex] != '\n'; sindex++) {
    char buf1[128];
    int linelen;

    if (!IS_SET(flag, SHOP_TRADE_WITH(shop_nr))){
      if (sindex) {
        send_to_char(ch, ", ");
        column += 2;
      }
      linelen = snprintf(buf1, sizeof(buf1), "%s", trade_letters[sindex]);
      /* Implementing word-wrapping: assumes screen-size == 80 */
      if (linelen + column >= 78 && column >= 20) {
        send_to_char(ch, "\r\n            ");
        column = 12;
      }

      if (!send_to_char(ch, "%s", buf1))
        return;
      column += linelen;
      found = TRUE;
    }
    flag <<= 1; /* next flag */
  }
  send_to_char(ch, "%s\r\n", found ? "" : "Nobody!");

  send_to_char(ch, "Produces:   ");
  column = 12;	/* ^^^ strlen ^^^ */
  for (sindex = 0; SHOP_PRODUCT(shop_nr, sindex) != NOTHING; sindex++) {
    char buf1[128];
    int linelen;

    if (sindex) {
      send_to_char(ch, ", ");
      column += 2;
    }
    linelen = snprintf(buf1, sizeof(buf1), "%s (#%d)",
		obj_proto[SHOP_PRODUCT(shop_nr, sindex)].short_description,
		obj_index[SHOP_PRODUCT(shop_nr, sindex)].vnum);

    /* Implementing word-wrapping: assumes screen-size == 80 */
    if (linelen + column >= 78 && column >= 20) {
      send_to_char(ch, "\r\n            ");
      /* 12 is to line up with "Produces:" printed first, and spaces above. */
      column = 12;
    }

    if (!send_to_char(ch, "%s", buf1))
      return;
    column += linelen;
  }
  if (!sindex)
    send_to_char(ch, "Produces:   Nothing!");

  send_to_char(ch, "\r\nBuys:       ");
  column = 12;	/* ^^^ strlen ^^^ */
  for (sindex = 0; SHOP_BUYTYPE(shop_nr, sindex) != NOTHING; sindex++) {
    char buf1[128];
    size_t linelen;

    if (sindex) {
      send_to_char(ch, ", ");
      column += 2;
    }

    linelen = snprintf(buf1, sizeof(buf1), "%s (#%d) [%s]",
		item_types[SHOP_BUYTYPE(shop_nr, sindex)],
		SHOP_BUYTYPE(shop_nr, sindex),
		SHOP_BUYWORD(shop_nr, sindex) ? SHOP_BUYWORD(shop_nr, sindex) : "all");

    /* Implementing word-wrapping: assumes screen-size == 80 */
    if (linelen + column >= 78 && column >= 20) {
      send_to_char(ch, "\r\n            ");
      /* 12 is to line up with "Buys:" printed first, and spaces above. */
      column = 12;
    }

    if (!send_to_char(ch, "%s", buf1))
      return;
    column += linelen;
  }
  if (!sindex)
    send_to_char(ch, "Buys:       Nothing!");

  send_to_char(ch, "\r\nBuy at:     [%4.2f], Sell at: [%4.2f], Open: [%d-%d, %d-%d]\r\n",
	SHOP_SELLPROFIT(shop_nr), SHOP_BUYPROFIT(shop_nr), SHOP_OPEN1(shop_nr),
	SHOP_CLOSE1(shop_nr), SHOP_OPEN2(shop_nr), SHOP_CLOSE2(shop_nr));

  /* Need a local buffer. */
  {
    char buf1[128];
    sprintbit(SHOP_BITVECTOR(shop_nr), shop_bits, buf1, sizeof(buf1));
    send_to_char(ch, "Bits:       %s\r\n", buf1);
  }
}

void show_shops(struct char_data *ch, char *arg)
{
  int shop_nr;

  if (!*arg)
    list_all_shops(ch);
  else {
    if (!str_cmp(arg, ".")) {
      for (shop_nr = 0; shop_nr <= top_shop; shop_nr++)
	if (ok_shop_room(shop_nr, GET_ROOM_VNUM(IN_ROOM(ch))))
	  break;

      if (shop_nr > top_shop) {
	send_to_char(ch, "This isn't a shop!\r\n");
	return;
      }
    } else if (is_number(arg))
      shop_nr = real_shop(atoi(arg));
    else
      shop_nr = -1;

    if (shop_nr < 0 || shop_nr > top_shop) {
      send_to_char(ch, "Illegal shop number.\r\n");
      return;
    }
    list_detailed_shop(ch, shop_nr);
  }
}

void destroy_shops(void)
{
  ssize_t cnt, itr;

  if (!shop_index)
    return;

  for (cnt = 0; cnt <= top_shop; cnt++) {
    if (shop_index[cnt].no_such_item1)
      free(shop_index[cnt].no_such_item1);
    if (shop_index[cnt].no_such_item2)
      free(shop_index[cnt].no_such_item2);
    if (shop_index[cnt].missing_cash1)
      free(shop_index[cnt].missing_cash1);
    if (shop_index[cnt].missing_cash2)
      free(shop_index[cnt].missing_cash2);
    if (shop_index[cnt].do_not_buy)
      free(shop_index[cnt].do_not_buy);
    if (shop_index[cnt].message_buy)
      free(shop_index[cnt].message_buy);
    if (shop_index[cnt].message_sell)
      free(shop_index[cnt].message_sell);
    if (shop_index[cnt].in_room)
      free(shop_index[cnt].in_room);
    if (shop_index[cnt].producing)
      free(shop_index[cnt].producing);

    if (shop_index[cnt].type) {
      for (itr = 0; BUY_TYPE(shop_index[cnt].type[itr]) != NOTHING; itr++)
        if (BUY_WORD(shop_index[cnt].type[itr]))
          free(BUY_WORD(shop_index[cnt].type[itr]));
      free(shop_index[cnt].type);
    }
  }

  free(shop_index);
  shop_index = NULL;
  top_shop = -1;
}

bool shopping_identify(char *arg, struct char_data *ch, struct char_data *keeper, int shop_nr)
{
  char buf[MAX_STRING_LENGTH];
  struct obj_data *obj;
  int i, found;

  if (!is_ok(keeper, ch, shop_nr))
    return FALSE;

  if (SHOP_SORT(shop_nr) < IS_CARRYING_N(keeper))
    sort_keeper_objs(keeper, shop_nr);

  if (!*arg) {
    snprintf(buf, sizeof(buf), "%s What do you want to identify??", GET_NAME(ch));
    do_tell(keeper, buf, cmd_tell, 0);
    return TRUE;
  }
  if (!(obj = get_purchase_obj(ch, arg, keeper, shop_nr, TRUE)))
    return FALSE;

  send_to_char(ch, "Name: %s\r\n", (obj->short_description) ? obj->short_description : "<None>");
  sprinttype(GET_OBJ_TYPE(obj), item_types, buf, sizeof(buf));
  send_to_char(ch, "Type: %s\r\n", buf);
  send_to_char(ch, "Weight: %d, Cost to Sell: %s%d%s, Cost to Buy: %s%d%s\r\n",
		GET_OBJ_WEIGHT(obj),
		QYEL, sell_price(obj, shop_nr, keeper, ch), QNRM,
		QYEL, buy_price(obj, shop_nr, keeper, ch), QNRM);
		
  sprintbitarray(GET_OBJ_WEAR(obj), wear_bits, TW_ARRAY_MAX, buf);
  send_to_char(ch, "Can be worn on: %s\r\n", buf);

      switch (GET_OBJ_TYPE(obj)) {
        case ITEM_LIGHT:
          if (GET_OBJ_VAL(obj, 2) == -1)
            send_to_char(ch, "Hours Remaining: (Infinite)\r\n");
          else if (GET_OBJ_VAL(obj, 2) == 0)
            send_to_char(ch, "Hours Remaining: None!\r\n");
          else
            send_to_char(ch, "Hours Remaining: %d\r\n", GET_OBJ_VAL(obj, 2));
          break;
        case ITEM_SCROLL:
        case ITEM_POTION:
          send_to_char(ch, "Spells: %s, %s, %s\r\n",
                  skill_name(GET_OBJ_VAL(obj, 1)),
                  skill_name(GET_OBJ_VAL(obj, 2)),
                  skill_name(GET_OBJ_VAL(obj, 3)));
          break;
        case ITEM_WAND:
        case ITEM_STAFF:
          send_to_char(ch, "Spell: %s\r\n", skill_name(GET_OBJ_VAL(obj, 3)));
          send_to_char(ch, "Charges: %d/%d\r\n", GET_OBJ_VAL(obj, 2), GET_OBJ_VAL(obj, 1));
          break;
        case ITEM_WEAPON:
            send_to_char(ch, "Damage Dice is '%dD%d' for an average per-round damage of %.1f.\r\n",
                        GET_OBJ_VAL(obj, 1), GET_OBJ_VAL(obj, 2),
                        ((GET_OBJ_VAL(obj, 2) + 1) / 2.0) * GET_OBJ_VAL(obj, 1));
            break;
        case ITEM_ARMOR:
          if(GET_OBJ_VAL(obj,1) == 0)
          {
            send_to_char(ch, "AC-apply: [%d]\r\n", GET_OBJ_VAL(obj, 0));
          }
          else
          {
            send_to_char(ch, "AC-apply: [%d] - This item has magical affects.\r\n", GET_OBJ_VAL(obj, 0));
          }
          break;
        case ITEM_CONTAINER:
          send_to_char(ch, "Capacity: %d/%d\r\n", GET_OBJ_WEIGHT(obj), GET_OBJ_VAL(obj, 0));
          break;
        case ITEM_DRINKCON:
        case ITEM_FOUNTAIN:
          send_to_char(ch, "Drinks: %d/%d\r\n", GET_OBJ_VAL(obj, 1), GET_OBJ_VAL(obj, 0));
          break;
        case ITEM_NOTE:
          send_to_char(ch, "\r\n");
          break;
        case ITEM_KEY:
          send_to_char(ch, "\r\n");
          break;
        case ITEM_FOOD:
          send_to_char(ch, "\r\n");
          break;
        case ITEM_MONEY:
          send_to_char(ch, "\r\n");
          break;
        case ITEM_WORN:
          if(GET_OBJ_VAL(obj,1) > 0)
            send_to_char(ch, "This item has magical affects.\r\n");
          else
            send_to_char(ch, "\r\n");
          break;
        default:
          send_to_char(ch, "\r\n");
          break;
      }

      found = 0;
      send_to_char(ch, "Affections:");
      for (i = 0; i < MAX_OBJ_AFFECT; i++)
        if (obj->affected[i].modifier) {
          sprinttype(obj->affected[i].location, apply_types, buf, sizeof(buf));
          send_to_char(ch, "%s %+d to %s", found++ ? "," : "", obj->affected[i].modifier, buf);
        }
      if (!found)
        send_to_char(ch, " None");

      send_to_char(ch, "\r\nExtra Flags: ");
      sprintbitarray(GET_OBJ_EXTRA(obj), extra_bits, EF_ARRAY_MAX, buf);
      send_to_char(ch, "%s\r\n", buf);

  return TRUE;
}
