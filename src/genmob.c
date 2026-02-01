/**************************************************************************
*  File: genmob.c                                          Part of tbaMUD *
*  Usage: Generic OLC Library - Mobiles.                                  *
*                                                                         *
*  Copyright 1996 by Harvey Gilpin, 1997-2001 by George Greer.            *
**************************************************************************/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "db.h"
#include "shop.h"
#include "handler.h"
#include "genolc.h"
#include "genmob.h"
#include "genzon.h"
#include "py_olc.h"
#include "spells.h"
#include "toml_utils.h"

/* local functions */
static void extract_mobile_all(mob_vnum vnum);

int add_mobile(struct char_data *mob, mob_vnum vnum)
{
  int rnum, i, found = FALSE, shop, cmd_no;
  zone_rnum zone;
  struct char_data *live_mob;

  if ((rnum = real_mobile(vnum)) != NOBODY) {
    /* Copy over the mobile and free() the old strings. */
    copy_mobile(&mob_proto[rnum], mob);

    /* Now re-point all existing mobile strings to here. */
    for (live_mob = character_list; live_mob; live_mob = live_mob->next)
      if (rnum == live_mob->nr)
        update_mobile_strings(live_mob, &mob_proto[rnum]);

    add_to_save_list(zone_table[real_zone_by_thing(vnum)].number, SL_MOB);
    log("GenOLC: add_mobile: Updated existing mobile #%d.", vnum);
    return rnum;
  }

  RECREATE(mob_proto, struct char_data, top_of_mobt + 2);
  RECREATE(mob_index, struct index_data, top_of_mobt + 2);
  top_of_mobt++;

  for (i = top_of_mobt; i > 0; i--) {
    if (vnum > mob_index[i - 1].vnum) {
      mob_proto[i] = *mob;
      mob_proto[i].nr = i;
      copy_mobile_strings(mob_proto + i, mob);
      mob_index[i].vnum = vnum;
      mob_index[i].number = 0;
      mob_index[i].func = 0;
      found = i;
      break;
    }
    mob_index[i] = mob_index[i - 1];
    mob_proto[i] = mob_proto[i - 1];
    mob_proto[i].nr++;
  }
  if (!found) {
    mob_proto[0] = *mob;
    mob_proto[0].nr = 0;
    copy_mobile_strings(&mob_proto[0], mob);
    mob_index[0].vnum = vnum;
    mob_index[0].number = 0;
    mob_index[0].func = 0;
  }

  log("GenOLC: add_mobile: Added mobile %d at index #%d.", vnum, found);

  /* Update live mobile rnums. */
  for (live_mob = character_list; live_mob; live_mob = live_mob->next)
    GET_MOB_RNUM(live_mob) += (GET_MOB_RNUM(live_mob) != NOTHING && GET_MOB_RNUM(live_mob) >= found);

  /* Update zone table. */
  for (zone = 0; zone <= top_of_zone_table; zone++)
    for (cmd_no = 0; ZCMD(zone, cmd_no).command != 'S'; cmd_no++)
      if (ZCMD(zone, cmd_no).command == 'M')
	ZCMD(zone, cmd_no).arg1 += (ZCMD(zone, cmd_no).arg1 >= found);

  /* Update shop keepers. */
  if (shop_index)
    for (shop = 0; shop <= top_shop; shop++)
      SHOP_KEEPER(shop) += (SHOP_KEEPER(shop) != NOTHING && SHOP_KEEPER(shop) >= found);

  add_to_save_list(zone_table[real_zone_by_thing(vnum)].number, SL_MOB);
  return found;
}

int copy_mobile(struct char_data *to, struct char_data *from)
{
  free_mobile_strings(to);
  *to = *from;
  check_mobile_strings(from);
  copy_mobile_strings(to, from);
  return TRUE;
}

static void extract_mobile_all(mob_vnum vnum)
{
  struct char_data *next, *ch;
  int i;

  for (ch = character_list; ch; ch = next) {
    next = ch->next;
    if (GET_MOB_VNUM(ch) == vnum) {
			if ((i = GET_MOB_RNUM(ch)) != NOBODY) {
	    if (ch->player.name && ch->player.name != mob_proto[i].player.name)
          free(ch->player.name);
				ch->player.name = NULL;
				
        if (ch->player.short_descr && ch->player.short_descr != mob_proto[i].player.short_descr)
          free(ch->player.short_descr);
				ch->player.short_descr = NULL;
				
        if (ch->player.long_descr && ch->player.long_descr != mob_proto[i].player.long_descr)
          free(ch->player.long_descr);
				ch->player.long_descr = NULL;
				
        if (ch->player.description && ch->player.description != mob_proto[i].player.description)
          free(ch->player.description);
				ch->player.description = NULL;
				
        if (ch->player.background && ch->player.background != mob_proto[i].player.background)
          free(ch->player.background);
        ch->player.background = NULL;

        if (ch->mob_specials.ex_description &&
            ch->mob_specials.ex_description != mob_proto[i].mob_specials.ex_description)
          free_ex_descriptions(ch->mob_specials.ex_description);
        ch->mob_specials.ex_description = NULL;
    
        /* free script proto list if it's not the prototype */
        if (ch->proto_script && ch->proto_script != mob_proto[i].proto_script)
          free_proto_script(ch, MOB_TRIGGER);			
				ch->proto_script = NULL;
			}
      extract_char(ch);
		}
  }
}

int delete_mobile(mob_rnum refpt)
{
  struct char_data *live_mob;
  struct char_data *proto;
  int counter, cmd_no;
  mob_vnum vnum;
  zone_rnum zone;

#if CIRCLE_UNSIGNED_INDEX
  if (refpt == NOBODY || refpt > top_of_mobt) {
#else
  if (refpt < 0 || refpt > top_of_mobt) {
#endif
    log("SYSERR: GenOLC: delete_mobile: Invalid rnum %d.", refpt);
    return NOBODY;
  }

  vnum = mob_index[refpt].vnum;
  proto = &mob_proto[refpt];
  
  extract_mobile_all(vnum);
  extract_char(proto);

  for (counter = refpt; counter < top_of_mobt; counter++) {
    mob_index[counter] = mob_index[counter + 1];
    mob_proto[counter] = mob_proto[counter + 1];
    mob_proto[counter].nr--;
  }

  top_of_mobt--;
  RECREATE(mob_index, struct index_data, top_of_mobt + 1);
  RECREATE(mob_proto, struct char_data, top_of_mobt + 1);

  /* Update live mobile rnums. */
  for (live_mob = character_list; live_mob; live_mob = live_mob->next)
    GET_MOB_RNUM(live_mob) -= (GET_MOB_RNUM(live_mob) >= refpt);

  /* Update zone table. */
  for (zone = 0; zone <= top_of_zone_table; zone++)
    for (cmd_no = 0; ZCMD(zone, cmd_no).command != 'S'; cmd_no++)
      if (ZCMD(zone, cmd_no).command == 'M'){
       if (ZCMD(zone, cmd_no).arg1 == refpt) {
        delete_zone_command(&zone_table[zone], cmd_no);
        } else
          ZCMD(zone, cmd_no).arg1 -= (ZCMD(zone, cmd_no).arg1 > refpt);
        }

  /* Update shop keepers. */
  if (shop_index)
    for (counter = 0; counter <= top_shop; counter++)
      SHOP_KEEPER(counter) -= (SHOP_KEEPER(counter) >= refpt);

  save_mobiles(real_zone_by_thing(vnum));

  return refpt;
}

int copy_mobile_strings(struct char_data *t, struct char_data *f)
{
  if (f->player.name)
    t->player.name = strdup(f->player.name);
  if (f->player.short_descr)
    t->player.short_descr = strdup(f->player.short_descr);
  if (f->player.long_descr)
    t->player.long_descr = strdup(f->player.long_descr);
  if (f->player.description)
    t->player.description = strdup(f->player.description);
  if (f->player.background)
    t->player.background = strdup(f->player.background);
  if (f->mob_specials.ex_description)
    copy_ex_descriptions(&t->mob_specials.ex_description, f->mob_specials.ex_description);
  return TRUE;
}

int update_mobile_strings(struct char_data *t, struct char_data *f)
{
  if (f->player.name)
    t->player.name = f->player.name;
  if (f->player.short_descr)
    t->player.short_descr = f->player.short_descr;
  if (f->player.long_descr)
    t->player.long_descr = f->player.long_descr;
  if (f->player.description)
    t->player.description = f->player.description;
  if (f->player.background)
    t->player.background = f->player.background;
  t->mob_specials.ex_description = f->mob_specials.ex_description;
  return TRUE;
}

int free_mobile_strings(struct char_data *mob)
{
  if (mob->player.name)
    free(mob->player.name);
  if (mob->player.short_descr)
    free(mob->player.short_descr);
  if (mob->player.long_descr)
    free(mob->player.long_descr);
  if (mob->player.description)
    free(mob->player.description);
  if (mob->player.background)
    free(mob->player.background);
  if (mob->mob_specials.ex_description) {
    free_ex_descriptions(mob->mob_specials.ex_description);
    mob->mob_specials.ex_description = NULL;
  }
  return TRUE;
}

/* Free a mobile structure that has been edited. Take care of existing mobiles
 * and their mob_proto! */
int free_mobile(struct char_data *mob)
{
  mob_rnum i;

  if (mob == NULL)
    return FALSE;

  /* Non-prototyped mobile.  Also known as new mobiles. */
  if ((i = GET_MOB_RNUM(mob)) == NOBODY) {
    free_mobile_strings(mob);
    /* free script proto list */
    free_proto_script(mob, MOB_TRIGGER);
   } else {	/* Prototyped mobile. */
    if (mob->player.name && mob->player.name != mob_proto[i].player.name)
      free(mob->player.name);
    if (mob->player.short_descr && mob->player.short_descr != mob_proto[i].player.short_descr)
      free(mob->player.short_descr);
    if (mob->player.long_descr && mob->player.long_descr != mob_proto[i].player.long_descr)
      free(mob->player.long_descr);
    if (mob->player.description && mob->player.description != mob_proto[i].player.description)
      free(mob->player.description);
    if (mob->player.background && mob->player.background != mob_proto[i].player.background)
      free(mob->player.background);
    if (mob->mob_specials.ex_description &&
        mob->mob_specials.ex_description != mob_proto[i].mob_specials.ex_description)
      free_ex_descriptions(mob->mob_specials.ex_description);
    /* free script proto list if it's not the prototype */
    if (mob->proto_script && mob->proto_script != mob_proto[i].proto_script)
      free_proto_script(mob, MOB_TRIGGER);
  }
  while (mob->affected)
    affect_remove(mob, mob->affected);

  /* free any assigned scripts */
  if (SCRIPT(mob))
    extract_script(mob, MOB_TRIGGER);

  free(mob);
  return TRUE;
}

int save_mobiles(zone_rnum rznum)
{
  zone_vnum vznum;
  FILE *mobfd;
  room_vnum i;
  mob_rnum rmob;
  int written;
  char mobfname[64], usedfname[64];

#if CIRCLE_UNSIGNED_INDEX
  if (rznum == NOWHERE || rznum > top_of_zone_table) {
#else
  if (rznum < 0 || rznum > top_of_zone_table) {
#endif
    log("SYSERR: GenOLC: save_mobiles: Invalid real zone number %d. (0-%d)", rznum, top_of_zone_table);
    return FALSE;
  }

  vznum = zone_table[rznum].number;
  snprintf(mobfname, sizeof(mobfname), "%s%d.new", MOB_PREFIX, vznum);
  if ((mobfd = fopen(mobfname, "w")) == NULL) {
    mudlog(BRF, LVL_GOD, TRUE, "SYSERR: GenOLC: Cannot open mob file for writing.");
    return FALSE;
  }

  for (i = genolc_zone_bottom(rznum); i <= zone_table[rznum].top; i++) {
    if ((rmob = real_mobile(i)) == NOBODY)
      continue;
    check_mobile_strings(&mob_proto[rmob]);
    if (write_mobile_record(i, &mob_proto[rmob], mobfd) < 0)
      log("SYSERR: GenOLC: Error writing mobile #%d.", i);
  }
  written = ftell(mobfd);
  fclose(mobfd);
  snprintf(usedfname, sizeof(usedfname), "%s%d.toml", MOB_PREFIX, vznum);
  remove(usedfname);
  rename(mobfname, usedfname);

  if (in_save_list(vznum, SL_MOB))
    remove_from_save_list(vznum, SL_MOB);
  log("GenOLC: '%s' saved, %d bytes written.", usedfname, written);
  return written;
}

int write_mobile_espec(mob_vnum mvnum, struct char_data *mob, FILE *fd)
{
  int count = 0;

  /* --- Ability scores (only write non-defaults) --- */
  if (GET_STR(mob) != 11) {
    fprintf(fd, "Str: %d\n", GET_STR(mob));
    count++;
  }
  if (GET_DEX(mob) != 11) {
    fprintf(fd, "Dex: %d\n", GET_DEX(mob));
    count++;
  }
  if (GET_INT(mob) != 11) {
    fprintf(fd, "Int: %d\n", GET_INT(mob));
    count++;
  }
  if (GET_WIS(mob) != 11) {
    fprintf(fd, "Wis: %d\n", GET_WIS(mob));
    count++;
  }
  if (GET_CON(mob) != 11) {
    fprintf(fd, "Con: %d\n", GET_CON(mob));
    count++;
  }
  if (GET_CHA(mob) != 11) {
    fprintf(fd, "Cha: %d\n", GET_CHA(mob));
    count++;
  }

  if (HAS_VALID_CLASS(mob)) {
    fprintf(fd, "Class: %d\n", (int)GET_CLASS(mob));
    count++;
  }
  if (HAS_VALID_SPECIES(mob)) {
    fprintf(fd, "Species: %d\n", (int)GET_SPECIES(mob));
    count++;
  }
  {
    int age_years = GET_ROLEPLAY_AGE(mob);
    if (age_years >= MIN_CHAR_AGE && age_years <= MAX_CHAR_AGE &&
        age_years != MIN_CHAR_AGE) {
      fprintf(fd, "Age: %d\n", age_years);
      count++;
    }
  }

  /* --- 5e-style saving throws --- */
  if (GET_SAVE(mob, ABIL_STR) != 0) {
    fprintf(fd, "SaveStr: %d\n", GET_SAVE(mob, ABIL_STR));
    count++;
  }
  if (GET_SAVE(mob, ABIL_DEX) != 0) {
    fprintf(fd, "SaveDex: %d\n", GET_SAVE(mob, ABIL_DEX));
    count++;
  }
  if (GET_SAVE(mob, ABIL_CON) != 0) {
    fprintf(fd, "SaveCon: %d\n", GET_SAVE(mob, ABIL_CON));
    count++;
  }
  if (GET_SAVE(mob, ABIL_INT) != 0) {
    fprintf(fd, "SaveInt: %d\n", GET_SAVE(mob, ABIL_INT));
    count++;
  }
  if (GET_SAVE(mob, ABIL_WIS) != 0) {
    fprintf(fd, "SaveWis: %d\n", GET_SAVE(mob, ABIL_WIS));
    count++;
  }
  if (GET_SAVE(mob, ABIL_CHA) != 0) {
    fprintf(fd, "SaveCha: %d\n", GET_SAVE(mob, ABIL_CHA));
    count++;
  }

  /* DO NOT print "E" here — handled by write_mobile_record() */
  return count;
}

int write_mobile_record(mob_vnum mvnum, struct char_data *mob, FILE *fd)
{
  char ldesc[MAX_STRING_LENGTH];
  char ddesc[MAX_STRING_LENGTH];
  char bdesc[MAX_STRING_LENGTH];
  char edesc_buf[MAX_STRING_LENGTH];
  int has_bdesc = 0;

  ldesc[MAX_STRING_LENGTH - 1] = '\0';
  ddesc[MAX_STRING_LENGTH - 1] = '\0';
  bdesc[MAX_STRING_LENGTH - 1] = '\0';
  strip_cr(strncpy(ldesc, GET_LDESC(mob), MAX_STRING_LENGTH - 1));
  strip_cr(strncpy(ddesc, GET_DDESC(mob), MAX_STRING_LENGTH - 1));
  if (GET_BDESC(mob)) {
    strip_cr(strncpy(bdesc, GET_BDESC(mob), MAX_STRING_LENGTH - 1));
    {
      const char *p;
      for (p = bdesc; *p; p++) {
        if (*p != ' ' && *p != '\t' && *p != '\r' && *p != '\n') {
          has_bdesc = 1;
          break;
        }
      }
    }
  } else
    bdesc[0] = '\0';

  fprintf(fd, "[[mob]]\n");
  fprintf(fd, "vnum = %d\n", mvnum);
  toml_write_kv_string(fd, "name", GET_NAME(mob));
  toml_write_kv_string(fd, "keywords", GET_KEYWORDS(mob));
  toml_write_kv_string(fd, "short", GET_SDESC(mob));
  toml_write_kv_string(fd, "long", ldesc);
  toml_write_kv_string(fd, "description", ddesc);
  if (has_bdesc)
    toml_write_kv_string_opt(fd, "background", bdesc);

  if (mob->mob_specials.ex_description) {
    struct extra_descr_data *xdesc;
    for (xdesc = mob->mob_specials.ex_description; xdesc; xdesc = xdesc->next) {
      if (!xdesc->keyword || !xdesc->description || !*xdesc->keyword || !*xdesc->description) {
        mudlog(BRF, LVL_IMMORT, TRUE, "SYSERR: GenOLC: write_mobile_record: Corrupt ex_desc!");
        continue;
      }
      strncpy(edesc_buf, xdesc->description, sizeof(edesc_buf) - 1);
      edesc_buf[sizeof(edesc_buf) - 1] = '\0';
      strip_cr(edesc_buf);
      fprintf(fd, "\n[[mob.extra_desc]]\n");
      toml_write_kv_string(fd, "keyword", xdesc->keyword);
      toml_write_kv_string(fd, "description", edesc_buf);
    }
  }

  fprintf(fd, "flags = [%d, %d, %d, %d]\n",
          MOB_FLAGS(mob)[0], MOB_FLAGS(mob)[1],
          MOB_FLAGS(mob)[2], MOB_FLAGS(mob)[3]);
  fprintf(fd, "aff_flags = [%d, %d, %d, %d]\n",
          AFF_FLAGS(mob)[0], AFF_FLAGS(mob)[1],
          AFF_FLAGS(mob)[2], AFF_FLAGS(mob)[3]);
  fprintf(fd, "alignment = %d\n", GET_ALIGNMENT(mob));
  toml_write_kv_string(fd, "mob_type", "enhanced");

  /* --- DG Scripts --- */
  script_save_to_disk(fd, mob, MOB_TRIGGER);

  fprintf(fd, "\n[mob.simple]\n");
  fprintf(fd, "level = %d\n", GET_LEVEL(mob));
  fprintf(fd, "hit_dice = %d\n", GET_HIT(mob));
  fprintf(fd, "mana_dice = %d\n", GET_MANA(mob));
  fprintf(fd, "stamina_dice = %d\n", GET_STAMINA(mob));
  fprintf(fd, "pos = %d\n", GET_POS(mob));
  fprintf(fd, "default_pos = %d\n", GET_DEFAULT_POS(mob));
  fprintf(fd, "sex = %d\n", GET_SEX(mob));

  fprintf(fd, "\n[mob.enhanced]\n");
  fprintf(fd, "class = %d\n", GET_CLASS(mob));
  fprintf(fd, "species = %d\n", GET_SPECIES(mob));
  fprintf(fd, "age = %d\n", GET_ROLEPLAY_AGE(mob));
  fprintf(fd, "attack_type = %d\n", mob->mob_specials.attack_type);

  fprintf(fd, "\n[mob.enhanced.abilities]\n");
  fprintf(fd, "str = %d\n", GET_STR(mob));
  fprintf(fd, "dex = %d\n", GET_DEX(mob));
  fprintf(fd, "con = %d\n", GET_CON(mob));
  fprintf(fd, "int = %d\n", GET_INT(mob));
  fprintf(fd, "wis = %d\n", GET_WIS(mob));
  fprintf(fd, "cha = %d\n", GET_CHA(mob));

  fprintf(fd, "\n[mob.enhanced.saving_throws]\n");
  fprintf(fd, "str = %d\n", GET_SAVE(mob, ABIL_STR));
  fprintf(fd, "dex = %d\n", GET_SAVE(mob, ABIL_DEX));
  fprintf(fd, "con = %d\n", GET_SAVE(mob, ABIL_CON));
  fprintf(fd, "int = %d\n", GET_SAVE(mob, ABIL_INT));
  fprintf(fd, "wis = %d\n", GET_SAVE(mob, ABIL_WIS));
  fprintf(fd, "cha = %d\n", GET_SAVE(mob, ABIL_CHA));

  /* Write NPC skills (if any set) */
  for (int s = 0; s < MAX_SKILLS; s++) {
    if (mob->mob_specials.skills[s] > 0) {
      fprintf(fd, "\n[[mob.enhanced.skills]]\n");
      fprintf(fd, "id = %d\n", s);
      fprintf(fd, "level = %d\n", mob->mob_specials.skills[s]);
    }
  }

  /* --- Loadout entries --- */
  for (struct mob_loadout *e = mob->proto_loadout; e; e = e->next) {
    fprintf(fd, "\n[[mob.loadout]]\n");
    fprintf(fd, "wear_pos = %d\n", (int)e->wear_pos);
    fprintf(fd, "vnum = %d\n", (int)e->vnum);
    fprintf(fd, "quantity = %d\n", MAX(1, e->quantity));
  }

  /* --- Skinning yields --- */
  {
    mob_rnum rmob = real_mobile(mvnum);
    struct skin_yield_entry *sy;

    if (rmob != NOBODY && mob_index[rmob].skin_yields) {
      for (sy = mob_index[rmob].skin_yields; sy; sy = sy->next) {
        fprintf(fd, "\n[[mob.skin_yield]]\n");
        fprintf(fd, "obj_vnum = %d\n", sy->obj_vnum);
        fprintf(fd, "dc = %d\n", sy->dc);
      }
    }
  }

#if CONFIG_GENOLC_MOBPROG
  if (write_mobile_mobprog(mvnum, mob, fd) < 0)
    log("SYSERR: GenOLC: Error writing MobProgs for mobile #%d.", mvnum);
#endif

  fputc('\n', fd);
  return TRUE;
}

void check_mobile_strings(struct char_data *mob)
{
  mob_vnum mvnum = mob_index[mob->nr].vnum;
  check_mobile_string(mvnum, &GET_NAME(mob), "npc name");
  check_mobile_string(mvnum, &GET_KEYWORDS(mob), "alias list");
  check_mobile_string(mvnum, &GET_SDESC(mob), "short description");
  check_mobile_string(mvnum, &GET_LDESC(mob), "long description");
  check_mobile_string(mvnum, &GET_DDESC(mob), "detailed description");
}

void check_mobile_string(mob_vnum i, char **string, const char *desc)
{
  if (*string == NULL || **string == '\0') {
    char smbuf[128];
    sprintf(smbuf, "GenOLC: Mob #%d has an invalid %s.", i, desc);
    mudlog(BRF, LVL_GOD, TRUE, "%s", smbuf);
    if (*string)
      free(*string);
    *string = strdup("An undefined string.\n");
  }
}
