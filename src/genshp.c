/**************************************************************************
*  File: genshp.c                                          Part of tbaMUD *
*  Usage: Generic OLC Library - Shops.                                    *
*                                                                         *
*  Copyright 1996 by Harvey Gilpin, 1997-2001 by George Greer.            *
**************************************************************************/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "db.h"
#include "shop.h"
#include "genolc.h"
#include "genshp.h"
#include "genzon.h"
#include "genmob.h"	/* for save_mobiles */

/* NOTE (gg): Didn't modify sedit much. Don't consider it as 'recent' as the
 * other editors with regard to updates or style. */

/* local (file scope) functions */
static void copy_shop_list(IDXTYPE **tlist, IDXTYPE *flist);
static void copy_shop_type_list(struct shop_buy_data **tlist, struct shop_buy_data *flist);
static void free_shop_strings(struct shop_data *shop);
static void free_shop_type_list(struct shop_buy_data **list);

void copy_shop(struct shop_data *tshop, struct shop_data *fshop, int free_old_strings)
{
  /* Copy basic information over. */
  S_NUM(tshop) = S_NUM(fshop);
  S_KEEPER(tshop) = S_KEEPER(fshop);
  S_OPEN1(tshop) = S_OPEN1(fshop);
  S_CLOSE1(tshop) = S_CLOSE1(fshop);
  S_OPEN2(tshop) = S_OPEN2(fshop);
  S_CLOSE2(tshop) = S_CLOSE2(fshop);
  S_BANK(tshop) = S_BANK(fshop);
  S_BROKE_TEMPER(tshop) = S_BROKE_TEMPER(fshop);
  S_BITVECTOR(tshop) = S_BITVECTOR(fshop);
  S_NOTRADE(tshop) = S_NOTRADE(fshop);
  S_SORT(tshop) = S_SORT(fshop);
  S_BUYPROFIT(tshop) = S_BUYPROFIT(fshop);
  S_SELLPROFIT(tshop) = S_SELLPROFIT(fshop);
  S_FUNC(tshop) = S_FUNC(fshop);

  /* Copy lists over. */
  copy_shop_list(&(S_ROOMS(tshop)), S_ROOMS(fshop));
  copy_shop_list(&(S_PRODUCTS(tshop)), S_PRODUCTS(fshop));
  copy_shop_type_list(&(tshop->type), fshop->type);

  /* Copy notification strings over. */
  if (free_old_strings)
    free_shop_strings(tshop);
  S_NOITEM1(tshop) = str_udup(S_NOITEM1(fshop));
  S_NOITEM2(tshop) = str_udup(S_NOITEM2(fshop));
  S_NOCASH1(tshop) = str_udup(S_NOCASH1(fshop));
  S_NOCASH2(tshop) = str_udup(S_NOCASH2(fshop));
  S_NOBUY(tshop) = str_udup(S_NOBUY(fshop));
  S_BUY(tshop) = str_udup(S_BUY(fshop));
  S_SELL(tshop) = str_udup(S_SELL(fshop));

}

/* Copy a 'NOTHING' terminated integer array list. */
static void copy_shop_list(IDXTYPE **tlist, IDXTYPE *flist)
{
  int num_items, i;

  if (*tlist)
    free(*tlist);

  /* Count number of entries. */
  for (i = 0; flist[i] != NOTHING; i++);
  num_items = i + 1;

  /* Make space for entries. */
  CREATE(*tlist, IDXTYPE, num_items);

  /* Copy entries over. */
  for (i = 0; i < num_items; i++)
    (*tlist)[i] = flist[i];
}

/* Copy a -1 terminated (in the type field) shop_buy_data array list. */
static void copy_shop_type_list(struct shop_buy_data **tlist, struct shop_buy_data *flist)
{
  int num_items, i;

  if (*tlist)
    free_shop_type_list(tlist);

  /* Count number of entries. */
  for (i = 0; BUY_TYPE(flist[i]) != NOTHING; i++);
  num_items = i + 1;

  /* Make space for entries. */
  CREATE(*tlist, struct shop_buy_data, num_items);

  /* Copy entries over. */
  for (i = 0; i < num_items; i++) {
    (*tlist)[i].type = flist[i].type;
    if (BUY_WORD(flist[i]))
      BUY_WORD((*tlist)[i]) = strdup(BUY_WORD(flist[i]));
  }
}

void remove_shop_from_type_list(struct shop_buy_data **list, int num)
{
  int i, num_items;
  struct shop_buy_data *nlist;

  /* Count number of entries. */
  for (i = 0; (*list)[i].type != NOTHING; i++);

  if (num < 0 || num >= i)
    return;
  num_items = i;

  CREATE(nlist, struct shop_buy_data, num_items);

  for (i = 0; i < num_items; i++)
    nlist[i] = (i < num) ? (*list)[i] : (*list)[i + 1];

  free(BUY_WORD((*list)[num]));
  free(*list);
  *list = nlist;
}

void add_shop_to_type_list(struct shop_buy_data **list, struct shop_buy_data *newl)
{
  int i, num_items;
  struct shop_buy_data *nlist;

  /* Count number of entries. */
  for (i = 0; (*list)[i].type != NOTHING; i++);
  num_items = i;

  /* Make a new list and slot in the new entry. */
  CREATE(nlist, struct shop_buy_data, num_items + 2);

  for (i = 0; i < num_items; i++)
    nlist[i] = (*list)[i];
  nlist[num_items] = *newl;
  nlist[num_items + 1].type = NOTHING;

  /* Out with the old, in with the new. */
  free(*list);
  *list = nlist;
}

void add_shop_to_int_list(IDXTYPE **list, IDXTYPE newi)
{
  IDXTYPE i, num_items, *nlist;

  /* Count number of entries. */
  for (i = 0; (*list)[i] != NOTHING; i++);
  num_items = i;

  /* Make a new list and slot in the new entry. */
  CREATE(nlist, IDXTYPE, num_items + 2);

  for (i = 0; i < num_items; i++)
    nlist[i] = (*list)[i];
  nlist[num_items] = newi;
  nlist[num_items + 1] = NOTHING;

  /* Out with the old, in with the new. */
  free(*list);
  *list = nlist;
}

void remove_shop_from_int_list(IDXTYPE **list, IDXTYPE num)
{
  IDXTYPE i, num_items, *nlist;

  /* Count number of entries. */
  for (i = 0; (*list)[i] != NOTHING; i++);

#if CIRCLE_UNSIGNED_INDEX
  if (num >= i)
#else
  if (num < 0 || num >= i)
#endif
    return;
  num_items = i;

  CREATE(nlist, IDXTYPE, num_items);

  for (i = 0; i < num_items; i++)
    nlist[i] = (i < num) ? (*list)[i] : (*list)[i + 1];

  free(*list);
  *list = nlist;
}

/* Free all the notice character strings in a shop structure. */
static void free_shop_strings(struct shop_data *shop)
{
  if (S_NOITEM1(shop)) {
    free(S_NOITEM1(shop));
    S_NOITEM1(shop) = NULL;
  }
  if (S_NOITEM2(shop)) {
    free(S_NOITEM2(shop));
    S_NOITEM2(shop) = NULL;
  }
  if (S_NOCASH1(shop)) {
    free(S_NOCASH1(shop));
    S_NOCASH1(shop) = NULL;
  }
  if (S_NOCASH2(shop)) {
    free(S_NOCASH2(shop));
    S_NOCASH2(shop) = NULL;
  }
  if (S_NOBUY(shop)) {
    free(S_NOBUY(shop));
    S_NOBUY(shop) = NULL;
  }
  if (S_BUY(shop)) {
    free(S_BUY(shop));
    S_BUY(shop) = NULL;
  }
  if (S_SELL(shop)) {
    free(S_SELL(shop));
    S_SELL(shop) = NULL;
  }
}

/* Free a type list and all the strings it contains. */
static void free_shop_type_list(struct shop_buy_data **list)
{
  int i;

  for (i = 0; (*list)[i].type != NOTHING; i++)
    if (BUY_WORD((*list)[i]))
      free(BUY_WORD((*list)[i]));

  free(*list);
  *list = NULL;
}

/* Free up the whole shop structure and it's content. */
void free_shop(struct shop_data *shop)
{
  free_shop_strings(shop);
  free_shop_type_list(&(S_NAMELISTS(shop)));
  free(S_ROOMS(shop));
  free(S_PRODUCTS(shop));
  free(shop);
}

/* Remove a shop from shop_index.
 *
 * shop_index is an array of shop_data, not of pointers, so free_shop() is the
 * wrong tool -- it ends with free(shop), right for a heap-allocated one
 * (OLC_SHOP) and wrong for an array element. The contents are released
 * individually here, with the same helpers free_shop() uses.
 *
 * top_shop is a LAST INDEX, not a count, so the guard is `> top_shop` and the
 * shift stops one short of it. real_shop() binary-searches this array by vnum,
 * so it has to stay sorted -- which a straight compaction preserves.
 *
 * The keeper needs the most care, and NOT the way delete_quest handles a
 * questmaster -- review showed that shape is unsafe here.
 * assign_the_shopkeepers() displaces the mob's own spec proc into SHOP_FUNC
 * and points mob_index at shop_keeper. It runs once, at boot, and its guard
 * stashes the proc in only the FIRST shop it walks for a given keeper, so a
 * mob keeping two shops has its proc in the lower-rnum one and NULL in the
 * other. Simply dropping it when other shops remain therefore destroys the
 * only copy: delete the shops in ascending order and the mob's own proc is
 * gone for good. It is handed to a surviving shop instead. delete_quest
 * needs none of this because add_quest re-stashes on every add. */
int delete_shop(shop_rnum rnum)
{
  shop_rnum i, inherit = NOWHERE;
  zone_rnum rznum, kzone;
  mob_rnum keeper;
  struct char_data *mob;
  SPECIAL (*tempfunc);
  int shops_remaining = 0;

  if (rnum == NOWHERE || rnum > top_shop)
    return FALSE;

  rznum = real_zone_by_thing(SHOP_NUM(rnum));
  keeper = SHOP_KEEPER(rnum);
  tempfunc = SHOP_FUNC(rnum);

  log("GenOLC: delete_shop: Deleting shop #%d.", SHOP_NUM(rnum));

  free_shop_strings(&shop_index[rnum]);
  free_shop_type_list(&(S_NAMELISTS(&shop_index[rnum])));
  free(S_ROOMS(&shop_index[rnum]));
  free(S_PRODUCTS(&shop_index[rnum]));

  for (i = rnum; i < top_shop; i++)
    shop_index[i] = shop_index[i + 1];

  top_shop--;

  if (top_shop >= 0)
    RECREATE(shop_index, struct shop_data, top_shop + 1);
  else {
    free(shop_index);
    shop_index = NULL;
  }

  if (keeper != NOBODY) {
    for (i = 0; i <= top_shop; i++)
      if (SHOP_KEEPER(i) == keeper) {
        shops_remaining++;
        if (inherit == NOWHERE && SHOP_FUNC(i) == NULL)
          inherit = i;
      }

    if (shops_remaining) {
      /* Somebody else still keeps a shop for this mob, so it stays a
       * shopkeeper -- but the proc it had before it became one may have
       * been living in the record just freed. Give it to a survivor that
       * has none. */
      if (tempfunc && inherit != NOWHERE)
        SHOP_FUNC(inherit) = tempfunc;
    } else {
      /* Its last shop. Hand the proc back, but only if shop_keeper is
       * still what is there to replace: a keeper reassigned since boot
       * should keep the reassignment rather than have it overwritten. */
      if (mob_index[keeper].func == shop_keeper)
        mob_index[keeper].func = tempfunc;

      /* And it is not a shopkeeper any more. MOB_SPEC left set over a NULL
       * func makes mobile_activity log "Attempting to call non-existing mob
       * function" -- once for every copy that spawns, and again after every
       * reboot, because it strips the bit from the live mob and never from
       * the prototype the next copy is read from. So: the prototype, the
       * copies already walking around, and the .mob file they are loaded
       * from, which is the only one of the three that survives a reboot. */
      if (mob_index[keeper].func == NULL &&
          MOB_FLAGGED(&mob_proto[keeper], MOB_SPEC)) {
        REMOVE_BIT_AR(MOB_FLAGS(&mob_proto[keeper]), MOB_SPEC);
        for (mob = character_list; mob; mob = mob->next)
          if (IS_NPC(mob) && GET_MOB_RNUM(mob) == keeper)
            REMOVE_BIT_AR(MOB_FLAGS(mob), MOB_SPEC);

        kzone = real_zone_by_thing(mob_index[keeper].vnum);
        if (kzone != NOWHERE) {
          add_to_save_list(zone_table[kzone].number, SL_MOB);
          /* Written now only when it is the zone this delete is already
           * writing. Keepers do not have to live in their shop's zone,
           * and reaching across to write a file the builder may have no
           * right to edit is not this command's business -- that one is
           * queued for the next save instead. The in-memory correction
           * above happens either way. */
          if (kzone == rznum)
            save_mobiles(kzone);
        }
      }
    }
  }

  if (rznum != NOWHERE)
    add_to_save_list(zone_table[rznum].number, SL_SHP);
  else
    mudlog(BRF, LVL_BUILDER, TRUE,
           "SYSERR: GenOLC: delete_shop: Cannot determine shop zone.");

  return TRUE;
}

/* Returns the real number of the shop with given virtual number. We take so
 * good care to keep it sorted - let's use it. - Welcor */
shop_rnum real_shop(shop_vnum vnum)
{
  int bot, top, mid;

  bot = 0;
  top = top_shop;

  /* perform binary search on shop_table */
  while (bot <= top) {
    mid = (bot + top) / 2;
    if (SHOP_NUM(mid) == vnum)
      return (mid);
    if (SHOP_NUM(mid) > vnum)
      top = mid - 1;
    else
      bot = mid + 1;
  }
  return NOWHERE;
}

/* Generic string modifier for shop keeper messages. */
void modify_shop_string(char **str, char *new_s)
{

  char buf[MAX_STRING_LENGTH];
  char *pointer;

  /* Check the '%s' is present, if not, add it. */
  if (*new_s != '%') {
    snprintf(buf, sizeof(buf), "%%s %s", new_s);
    pointer = buf;
  } else
    pointer = new_s;

  if (*str)
    free(*str);
  *str = strdup(pointer);
}

/* How many shops name this mobile as their keeper. */
static int shops_kept_by(mob_rnum keeper)
{
  shop_rnum i;
  int n = 0;

  if (keeper == NOBODY)
    return 0;

  for (i = 0; i <= top_shop; i++)
    if (SHOP_KEEPER(i) == keeper)
      n++;

  return n;
}

/* Put both mobiles right after a shop's keeper has been changed.
 *
 * The rule the boot establishes is that a mobile runs shop_keeper exactly
 * while some shop names it, and assign_the_shopkeepers displaces whatever proc
 * the mobile already had into SHOP_FUNC so it can be handed back. Changing a
 * keeper through sedit used to install shop_keeper on the new mobile and do
 * nothing at all about the old one, which left it answering `list` and `buy`
 * with "Sorry, but you cannot do that here!" -- a shopkeeper for no shop.
 *
 * `oldfunc` is the proc this shop was holding for its previous keeper, read
 * before add_shop overwrote the record. It is only the mobile's own proc when
 * THIS is the shop holding it: assign_the_shopkeepers stashes it in the first
 * shop that names the mobile and leaves the rest NULL. So when the mobile
 * still keeps others, the proc has to move to one of them rather than be
 * dropped, or releasing shops in one order destroys it and the other order
 * does not.
 *
 * MOB_SPEC is not SET on the incoming keeper: special() dispatches on the
 * index entry and never reads the flag, and assign_the_shopkeepers does not
 * set it either, so setting it here would diverge from what a reboot gives.
 * It IS cleared off a mobile released with nothing left to dispatch, and
 * written out -- see below for why the write is not optional.
 *
 * Returns the mobile it released, or NOBODY. */
mob_rnum reassign_shopkeeper(shop_vnum vnum, mob_rnum oldkeeper, SPECIAL(*oldfunc))
{
  shop_rnum rshop = real_shop(vnum), i;
  mob_rnum newkeeper, released = NOBODY;

  if (rshop == NOWHERE)
    return NOBODY;

  newkeeper = SHOP_KEEPER(rshop);
  if (newkeeper == oldkeeper)
    return NOBODY;

  /* The mobile that is no longer this shop's keeper. */
  if (oldkeeper != NOBODY && oldkeeper <= top_of_mobt) {
    if (shops_kept_by(oldkeeper) == 0) {
      /* Nothing names it now, so it stops being a shopkeeper and takes its
       * own proc back. */
      if (mob_index[oldkeeper].func == shop_keeper)
        mob_index[oldkeeper].func = oldfunc;

      /* And with nothing left to dispatch it is not a SPEC mobile either.
       * Stock shopkeepers carry MOB_SPEC in their .mob entry and no other
       * proc, so this is the ordinary case rather than the odd one. Left
       * set, mobile_activity logs "Attempting to call non-existing mob
       * function" for every copy that spawns -- and strips the bit from the
       * LIVE mobile only, so the prototype hands it out again on the next
       * respawn, and the file hands it out again after every reboot.
       *
       * All three therefore, and the write is what makes it stick: mutating
       * the prototype and leaving the file alone would put the change on
       * disk at whatever later moment somebody saved that zone for an
       * unrelated reason. Written here only when the keeper lives in the
       * zone this shop is already in; anywhere else it is queued, because
       * reaching across to write a zone the builder may not own would also
       * flush whatever else that zone had pending. */
      if (mob_index[oldkeeper].func == NULL &&
          MOB_FLAGGED(&mob_proto[oldkeeper], MOB_SPEC)) {
        struct char_data *mob;
        zone_rnum kzone;

        REMOVE_BIT_AR(MOB_FLAGS(&mob_proto[oldkeeper]), MOB_SPEC);
        for (mob = character_list; mob; mob = mob->next)
          if (IS_NPC(mob) && GET_MOB_RNUM(mob) == oldkeeper)
            REMOVE_BIT_AR(MOB_FLAGS(mob), MOB_SPEC);

        kzone = real_zone_by_thing(mob_index[oldkeeper].vnum);
        if (kzone != NOWHERE) {
          add_to_save_list(zone_table[kzone].number, SL_MOB);
          if (kzone == real_zone_by_thing(vnum))
            save_mobiles(kzone);
        }
        released = oldkeeper;
      }
    } else if (oldfunc) {
      /* It still keeps others, so it stays as it is -- but if the proc was
       * being held HERE it has to move somewhere that survives. */
      for (i = 0; i <= top_shop; i++)
        if (SHOP_KEEPER(i) == oldkeeper && SHOP_FUNC(i) == NULL) {
          SHOP_FUNC(i) = oldfunc;
          break;
        }
    }
  }

  /* And the mobile that now is. SHOP_FUNC is written unconditionally: this
   * record still holds whatever add_shop copied out of the editor, which is
   * the OLD keeper's proc, and leaving it there makes the shop run an
   * unrelated mobile's spec proc. */
  if (newkeeper != NOBODY && newkeeper <= top_of_mobt) {
    SHOP_FUNC(rshop) = mob_index[newkeeper].func != shop_keeper
                         ? mob_index[newkeeper].func : NULL;
    mob_index[newkeeper].func = shop_keeper;
  } else
    SHOP_FUNC(rshop) = NULL;

  return released;
}

int add_shop(struct shop_data *nshp)
{
  shop_rnum rshop;
  int found = 0;
  zone_rnum rznum = real_zone_by_thing(S_NUM(nshp));

  /* The shop already exists, just update it. */
  if ((rshop = real_shop(S_NUM(nshp))) != NOWHERE) {
   /* free old strings. They're not used in any other place -- Welcor */
   copy_shop(&shop_index[rshop], nshp, TRUE);
    if (rznum != NOWHERE)
      add_to_save_list(zone_table[rznum].number, SL_SHP);
    else
      mudlog(BRF, LVL_BUILDER, TRUE, "SYSERR: GenOLC: Cannot determine shop zone.");
    return rshop;
  }

  top_shop++;
  RECREATE(shop_index, struct shop_data, top_shop + 1);

  for (rshop = top_shop; rshop > 0; rshop--) {
    if (nshp->vnum > SHOP_NUM(rshop - 1)) {
      found = rshop;

      /* Make a "nofree" variant and remove these later. */
      shop_index[rshop].in_room = NULL;
      shop_index[rshop].producing = NULL;
      shop_index[rshop].type = NULL;
      /* don't free old strings - they're still in use -- Welcor */
      copy_shop(&shop_index[rshop], nshp, FALSE);
      break;
    }
    shop_index[rshop] = shop_index[rshop - 1];
  }

  if (!found) {
    /* Make a "nofree" variant and remove these later. */
    shop_index[rshop].in_room = NULL;
    shop_index[rshop].producing = NULL;
    shop_index[rshop].type = NULL;
    /* don't free old strings - they're still in use -- Welcor */
    copy_shop(&shop_index[0], nshp, FALSE);
  }

  if (rznum != NOWHERE)
    add_to_save_list(zone_table[rznum].number, SL_SHP);
  else
    mudlog(BRF, LVL_BUILDER, TRUE, "SYSERR: GenOLC: Cannot determine shop zone.");

  return rshop;
}

int save_shops(zone_rnum zone_num)
{
  int i, j, rshop, num_shops = 0;
  FILE *shop_file;
  char fname[128], oldname[128], buf[MAX_STRING_LENGTH];
  struct shop_data *shop;

#if CIRCLE_UNSIGNED_INDEX
  if (zone_num == NOWHERE || zone_num > top_of_zone_table) {
#else
  if (zone_num < 0 || zone_num > top_of_zone_table) {
#endif
    log("SYSERR: GenOLC: save_shops: Invalid real zone number %d. (0-%d)", zone_num, top_of_zone_table);
    return FALSE;
  }

  snprintf(fname, sizeof(fname), "%s/%d.new", SHP_PREFIX, zone_table[zone_num].number);
  if (!(shop_file = fopen(fname, "w"))) {
    mudlog(BRF, LVL_GOD, TRUE, "SYSERR: OLC: Cannot open shop file!");
    return FALSE;
  } else if (fprintf(shop_file, "CircleMUD v3.0 Shop File~\n") < 0) {
    mudlog(BRF, LVL_GOD, TRUE, "SYSERR: OLC: Cannot write to shop file!");
    fclose(shop_file);
    return FALSE;
  }
  /* Search database for shops in this zone. */
  for (i = genolc_zone_bottom(zone_num); i <= zone_table[zone_num].top; i++) {
    if ((rshop = real_shop(i)) != NOWHERE) {
      fprintf(shop_file, "#%d~\n", i);
      shop = shop_index + rshop;

      /* Save the products. */
      for (j = 0; S_PRODUCT(shop, j) != NOTHING; j++)
	fprintf(shop_file, "%d\n", obj_index[S_PRODUCT(shop, j)].vnum);
      fprintf(shop_file, "-1\n");

      /* Save the rates. */
      fprintf(shop_file, "%1.2f\n"
                         "%1.2f\n",
                         S_BUYPROFIT(shop),
                         S_SELLPROFIT(shop));

      /* Save the buy types and namelists. */
      for (j = 0;S_BUYTYPE(shop, j) != NOTHING; j++)
        fprintf(shop_file, "%d%s\n",
                S_BUYTYPE(shop, j),
		S_BUYWORD(shop, j) ? S_BUYWORD(shop, j) : "");
      fprintf(shop_file, "-1\n");

      /* Save messages. Added some defaults as sanity checks. */
      sprintf(buf,
	      "%s~\n"
	      "%s~\n"
	      "%s~\n"
	      "%s~\n"
	      "%s~\n"
	      "%s~\n"
	      "%s~\n"
	      "%d\n"
	      "%ld\n"
	      "%d\n"
	      "%d\n",
	      S_NOITEM1(shop) ? S_NOITEM1(shop) : "%s Ke?!",
	      S_NOITEM2(shop) ? S_NOITEM2(shop) : "%s Ke?!",
	      S_NOBUY(shop) ? S_NOBUY(shop) : "%s Ke?!",
	      S_NOCASH1(shop) ? S_NOCASH1(shop) : "%s Ke?!",
	      S_NOCASH2(shop) ? S_NOCASH2(shop) : "%s Ke?!",
	      S_BUY(shop) ? S_BUY(shop) : "%s Ke?! %d?",
	      S_SELL(shop) ? S_SELL(shop) : "%s Ke?! %d?",
	      S_BROKE_TEMPER(shop),
	      S_BITVECTOR(shop),
	      S_KEEPER(shop) == NOBODY ? -1 : mob_index[S_KEEPER(shop)].vnum,
	      S_NOTRADE(shop)
	      );
        
        fputs(convert_from_tabs(buf), shop_file);

      /* Save the rooms. */
      for (j = 0;S_ROOM(shop, j) != NOWHERE; j++)
        fprintf(shop_file, "%d\n", S_ROOM(shop, j));
      fprintf(shop_file, "-1\n");

      /* Save open/closing times. */
      fprintf(shop_file, "%d\n%d\n%d\n%d\n", S_OPEN1(shop), S_CLOSE1(shop),
          S_OPEN2(shop), S_CLOSE2(shop));
      num_shops++;
    }
  }
  fprintf(shop_file, "$~\n");
  fclose(shop_file);
  snprintf(oldname, sizeof(oldname), "%s/%d.shp", SHP_PREFIX, zone_table[zone_num].number);
  remove(oldname);
  rename(fname, oldname);

  if (num_shops > 0)
    create_world_index(zone_table[zone_num].number, "shp");

  if (in_save_list(zone_table[zone_num].number, SL_SHP))
    remove_from_save_list(zone_table[zone_num].number, SL_SHP);
  return TRUE;
}
