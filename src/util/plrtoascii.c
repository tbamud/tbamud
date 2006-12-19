/* ************************************************************************
*  file:  plrtoascii.c                                  Part of CircleMud *
*  Copyright (C) 1990, 1991 - see 'license.doc' for complete information. *
*  All Rights Reserved                                                    *
************************************************************************* */

#include "../conf.h"
#include "../sysdep.h"

#include "../structs.h"
#include "../utils.h"
#include "../db.h"
#include "../pfdefaults.h"

int sprintascii(char *out, bitvector_t bits);
int plr_filename(char *orig_name, char *filename);

void convert(char *filename)
{
  FILE *fl, *outfile, *index_file;
  struct char_file_u player;
  char index_name[40], outname[40], bits[127];
  int i;
  struct char_special_data_saved *csds;
  struct player_special_data_saved *psds;
  struct char_ability_data *cad;
  struct char_point_data *cpd;
  struct affected_type *aff;

  if (!(fl = fopen(filename, "r+"))) {
    perror("error opening playerfile");
    exit(1);
  }
  sprintf(index_name, "%s%s", LIB_PLRFILES, INDEX_FILE);
  if (!(index_file = fopen(index_name, "w"))) {
    perror("error opening index file");
    exit(1);
  }
  for (;;) {
    fread(&player, sizeof(struct char_file_u), 1, fl);
    if (feof(fl)) {
      fclose(fl);
      fclose(index_file);
      exit(1);
    }

    if (!plr_filename(player.name, outname))
      exit(1);
    printf("writing: %s\n", outname);

    fprintf(index_file, "%ld %s %d 0 %ld\n", 
	player.char_specials_saved.idnum, bits, player.level,
	player.last_logon);

    if (!(outfile = fopen(outname, "w"))) {
      printf("error opening output file");
      exit(1);
    }

/* char_file_u */
    if (player.name)
      fprintf(outfile, "Name: %s\n", player.name);
    if (player.pwd)
      fprintf(outfile, "Pass: %s\n", player.pwd);
    if (player.title)
      fprintf(outfile, "Titl: %s\n", player.title);
    if (player.description && *player.description)
      fprintf(outfile, "Desc:\n%s~\n", player.description);
    if (player.sex != PFDEF_SEX)
      fprintf(outfile, "Sex : %d\n", (int)player.sex); 
    if (player.chclass != PFDEF_CLASS)
      fprintf(outfile, "Clas: %d\n", (int)player.chclass); 
    if (player.level != PFDEF_LEVEL)
      fprintf(outfile, "Levl: %d\n", (int)player.level); 
    if (player.hometown != PFDEF_HOMETOWN)
      fprintf(outfile, "Home: %d\n", (int)player.hometown); 
    fprintf(outfile, "Brth: %d\n", (int)player.birth);
    fprintf(outfile, "Plyd: %d\n", (int)player.played);
    fprintf(outfile, "Last: %d\n", (int)player.last_logon);
    fprintf(outfile, "Host: %s\n", player.host);
    if (player.height != PFDEF_HEIGHT)
      fprintf(outfile, "Hite: %d\n", (int)player.height);
    if (player.weight != PFDEF_WEIGHT)
      fprintf(outfile, "Wate: %d\n", (int)player.weight);

/* char_special_data_saved */
    csds = &(player.char_specials_saved);
    if (csds->alignment != PFDEF_ALIGNMENT)
      fprintf(outfile, "Alin: %d\n", csds->alignment);
    fprintf(outfile, "Id  : %d\n", (int)csds->idnum);
    if (csds->act != PFDEF_PLRFLAGS)
      fprintf(outfile, "Act : %d\n", (int)csds->act);
    if (csds->affected_by != PFDEF_AFFFLAGS) {
      sprintascii(bits, csds->affected_by);
      fprintf(outfile, "Aff : %s\n", bits);
    }
    if (csds->apply_saving_throw[0] != PFDEF_SAVETHROW)
      fprintf(outfile, "Thr1: %d\n", csds->apply_saving_throw[0]);
    if (csds->apply_saving_throw[1] != PFDEF_SAVETHROW)
      fprintf(outfile, "Thr2: %d\n", csds->apply_saving_throw[1]);
    if (csds->apply_saving_throw[2] != PFDEF_SAVETHROW)
      fprintf(outfile, "Thr3: %d\n", csds->apply_saving_throw[2]);
    if (csds->apply_saving_throw[3] != PFDEF_SAVETHROW)
      fprintf(outfile, "Thr4: %d\n", csds->apply_saving_throw[3]);
    if (csds->apply_saving_throw[4] != PFDEF_SAVETHROW)
      fprintf(outfile, "Thr5: %d\n", csds->apply_saving_throw[4]);

/* player_special_data_saved */
    psds = &(player.player_specials_saved);
    if (player.level < LVL_IMMORT) {
      fprintf(outfile, "Skil:\n");
      for (i = 1; i <= MAX_SKILLS; i++) {
	if (psds->skills[i])
	  fprintf(outfile, "%d %d\n", i, (int)psds->skills[i]);
      }
      fprintf(outfile, "0 0\n");
    }
    if (psds->wimp_level != PFDEF_WIMPLEV)
      fprintf(outfile, "Wimp: %d\n", psds->wimp_level);
    if (psds->freeze_level != PFDEF_FREEZELEV)
      fprintf(outfile, "Frez: %d\n", (int)psds->freeze_level);
    if (psds->invis_level != PFDEF_INVISLEV)
      fprintf(outfile, "Invs: %d\n", (int)psds->invis_level);
    if (psds->load_room != PFDEF_LOADROOM)
      fprintf(outfile, "Room: %d\n", (int)psds->load_room);
    if (psds->pref != PFDEF_PREFFLAGS) {
      sprintascii(bits, psds->pref);
      fprintf(outfile, "Pref: %s\n", bits);
    }
    if (psds->conditions[FULL] && player.level < LVL_IMMORT &&
	psds->conditions[FULL] != PFDEF_HUNGER)
      fprintf(outfile, "Hung: %d\n", (int)psds->conditions[0]);
    if (psds->conditions[THIRST] && player.level < LVL_IMMORT &&
	psds->conditions[THIRST] != PFDEF_THIRST)
      fprintf(outfile, "Thir: %d\n", (int)psds->conditions[1]);
    if (psds->conditions[2] && player.level < LVL_IMMORT &&
	psds->conditions[DRUNK] != PFDEF_DRUNK)
      fprintf(outfile, "Drnk: %d\n", (int)psds->conditions[2]);
    if (psds->spells_to_learn != PFDEF_PRACTICES)
      fprintf(outfile, "Lern: %d\n", (int)psds->spells_to_learn);

/* char_ability_data */
    cad = &(player.abilities);
    if (cad->str != PFDEF_STR || cad->str_add != PFDEF_STRADD)
      fprintf(outfile, "Str : %d/%d\n", cad->str, cad->str_add);
    if (cad->intel != PFDEF_INT)
      fprintf(outfile, "Int : %d\n", cad->intel);
    if (cad->wis != PFDEF_WIS)
      fprintf(outfile, "Wis : %d\n", cad->wis);
    if (cad->dex != PFDEF_DEX)
      fprintf(outfile, "Dex : %d\n", cad->dex);
    if (cad->con != PFDEF_CON)
      fprintf(outfile, "Con : %d\n", cad->con);
    if (cad->cha != PFDEF_CHA)
      fprintf(outfile, "Cha : %d\n", cad->cha);

/* char_point_data */
    cpd = &(player.points);
    if (cpd->hit != PFDEF_HIT || cpd->max_hit != PFDEF_MAXHIT)
      fprintf(outfile, "Hit : %d/%d\n", cpd->hit, cpd->max_hit);
    if (cpd->mana != PFDEF_MANA || cpd->max_mana != PFDEF_MAXMANA)
      fprintf(outfile, "Mana: %d/%d\n", cpd->mana, cpd->max_mana);
    if (cpd->move != PFDEF_MOVE || cpd->max_move != PFDEF_MAXMOVE)
      fprintf(outfile, "Move: %d/%d\n", cpd->move, cpd->max_move);
    if (cpd->armor != PFDEF_AC)
      fprintf(outfile, "Ac  : %d\n", cpd->armor);
    if (cpd->gold != PFDEF_GOLD)
      fprintf(outfile, "Gold: %d\n", cpd->gold);
    if (cpd->bank_gold != PFDEF_BANK)
      fprintf(outfile, "Bank: %d\n", cpd->bank_gold);
    if (cpd->exp != PFDEF_EXP)
      fprintf(outfile, "Exp : %d\n", cpd->exp);
    if (cpd->hitroll != PFDEF_HITROLL)
      fprintf(outfile, "Hrol: %d\n", cpd->hitroll);
    if (cpd->damroll != PFDEF_DAMROLL)
      fprintf(outfile, "Drol: %d\n", cpd->damroll);

/* affected_type */
    fprintf(outfile, "Affs:\n");
    for (i = 0; i < MAX_AFFECT; i++) {
      aff = &(player.affected[i]);
      if (aff->type)
	fprintf(outfile, "%d %d %d %d %d\n", aff->type, aff->duration,
	  aff->modifier, aff->location, (int)aff->bitvector);
    }
    fprintf(outfile, "0 0 0 0 0\n");

    fclose(outfile);
  }
}


int main(int argc, char **argv)
{
  if (argc != 2)
    printf("Usage: %s playerfile-name\n", argv[0]);
  else
    convert(argv[1]);

  return 0;
}


int sprintascii(char *out, bitvector_t bits)
{
  int i, j = 0;
  /* 32 bits, don't just add letters to try to get more unless your bitvector_t is also as large. */
  char *flags = "abcdefghijklmnopqrstuvwxyzABCDEF";

  for (i = 0; flags[i] != '\0'; i++)
    if (bits & (1 << i))
      out[j++] = flags[i];

  if (j == 0) /* Didn't write anything. */
    out[j++] = '0';

  /* NUL terminate the output string. */
  out[j++] = '\0';
  return j;
}


int plr_filename(char *orig_name, char *filename)
{
  const char *middle;
  char name[64], *ptr;

  if (orig_name == NULL || *orig_name == '\0' || filename == NULL) {
    perror("error getting player file name");
    return (0);
  }

  strcpy(name, orig_name);
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

  sprintf(filename, "%s%s"SLASH"%s.%s", LIB_PLRFILES, middle, name, SUF_PLR);
  return (1);
}
