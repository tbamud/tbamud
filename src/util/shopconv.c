/* ************************************************************************
*  file:  shopconv.c                                       Part of tbaMUD *
*  Usage: code to convert 2.20 shop files to 3.0 shop files               *
*  Written by Jeff Fink                                                   *
************************************************************************* */

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"
#include "db.h"
#include "shop.h"

void basic_mud_log(const char *x, ...)
{
  puts(x);
}

/* Answers NULL if the file ran out or the string would not fit, having said
 * which; the caller must check.  Exiting here instead would leave main() no
 * chance to put the original file back, and it is main() that renamed it.
 * An empty string comes back as an allocated "" rather than NULL, so the two
 * cases stay apart -- boot_the_shops_conv() dereferences what it is given. */
char *fread_string(FILE * fl, const char *error)
{
  char buf[MAX_STRING_LENGTH], tmp[512], *rslt, *point;
  int flag;

  *buf = '\0';

  do {
    if (!fgets(tmp, sizeof(tmp), fl)) {
      printf("fread_string: format error at or near %s\n", error);
      return (NULL);
    }
    if (strlen(tmp) + strlen(buf) > MAX_STRING_LENGTH) {
      printf("SYSERR: fread_string: string too large (shopconv.c)");
      return (NULL);
    } else
      strcat(buf, tmp);

    /* Walk back over trailing whitespace to the terminator.  The old scan
     * started at buf + strlen(buf) - 2, which is before buf for a line
     * shorter than two characters, and read *point after stepping below buf
     * -- a lone "~" line was enough to run off the front of the array. */
    point = buf + strlen(buf);
    while (point > buf && isspace((unsigned char) *(point - 1)))
      point--;
    if ((flag = (point > buf && *(point - 1) == '~')))
      *(point - 1) = '\0';
  } while (!flag);

  /* do the allocate boogie  */

  CREATE(rslt, char, strlen(buf) + 1);
  strcpy(rslt, buf);
  return (rslt);
}


/* system() answers -1 when the command could not be started at all, and the
 * shell's exit status otherwise.  Every use here renames the file the next
 * step is about to open, so a failure that goes unreported turns into a
 * confusing error further on -- or, worse, leaves the .tmp holding the only
 * copy of the shop file. */
static int run(const char *cmd)
{
  int status = system(cmd);

  if (status == -1) {
    perror(cmd);
    return (0);
  }
  if (status != 0) {
    /* Where system() answers a wait status rather than a plain exit code the
     * code sits in the high byte, so a `mv' that exited 1 would otherwise be
     * reported as 256.  Platforms with no <sys/wait.h> never define these
     * macros, and there system() already hands back the exit code itself. */
#ifdef WEXITSTATUS
    if (WIFEXITED(status))
      fprintf(stderr, "shopconv: `%s' failed (status %d).\n", cmd,
              WEXITSTATUS(status));
    else
      fprintf(stderr, "shopconv: `%s' did not complete.\n", cmd);
#else
    fprintf(stderr, "shopconv: `%s' failed (status %d).\n", cmd, status);
#endif
    return (0);
  }
  return (1);
}

/* main() renames the shop file out of the way before it reads it, so every
 * path that gives up after that point has to put it back -- otherwise the
 * only copy is left sitting in the .tmp under a name nobody looks for. */
static int restore(const char *fn)
{
  char cmd[512];

  snprintf(cmd, sizeof(cmd), "mv %s.tmp %s", fn, fn);
  if (!run(cmd)) {
    fprintf(stderr, "shopconv: %s.tmp still holds the original %s.\n", fn, fn);
    return (0);
  }
  return (1);
}


int do_list(FILE * shop_f, FILE * newshop_f, int max)
{
  int count, temp;
  char buf[MAX_STRING_LENGTH];

  for (count = 0; count < max; count++) {
    /* A failed conversion leaves temp untouched, so the old loop ran to
     * `max` re-testing the same stale value and never advanced the file. */
    if (fscanf(shop_f, "%d", &temp) != 1) {
      fprintf(stderr, "shopconv: expected %d list entries, file ran out after %d.\n",
              max, count);
      return (0);
    }
    if (fgets(buf, MAX_STRING_LENGTH - 1, shop_f) == NULL) {
      fprintf(stderr, "shopconv: unexpected end of file inside a list.\n");
      return (0);
    }
    if (temp > 0)
      fprintf(newshop_f, "%d%s", temp, buf);
  }

  fprintf(newshop_f, "-1\n");
  return (1);
}


int do_float(FILE * shop_f, FILE * newshop_f)
{
  float f;
  size_t len;
  /* %f has no width of its own and prints the whole integer part, so the
   * width is the magnitude, not the format: near FLT_MAX it is 46
   * characters.  %f writes `digits + 7' characters plus the NUL, so char[20]
   * held twelve integer digits and overflowed from thirteen -- about 1e12 --
   * straight off an fscanf("%f") of a shop file, exactly the kind of file
   * MUDs pass round.  Sized for the widest float there is, and written
   * with snprintf so the bound comes from the array either way. */
  char str[64];

  /* f is uninitialised until this succeeds, and printing it is the next
   * thing that happens. */
  if (fscanf(shop_f, "%f \n", &f) != 1) {
    fprintf(stderr, "shopconv: expected a number.\n");
    return (0);
  }
  snprintf(str, sizeof(str), "%f", f);

  /* Trim trailing zeros, keeping one after the point.  Indexing from
   * strlen() twice per iteration underflowed on a string shorter than two
   * characters -- "inf" and "nan" do not reach the loop body, but nothing
   * here said so. */
  len = strlen(str);
  while (len > 2 && str[len - 1] == '0' && str[len - 2] != '.')
    str[--len] = '\0';
  fprintf(newshop_f, "%s \n", str);
  return (1);
}


int do_int(FILE * shop_f, FILE * newshop_f)
{
  int i;

  if (fscanf(shop_f, "%d \n", &i) != 1) {
    fprintf(stderr, "shopconv: expected a number.\n");
    return (0);
  }
  fprintf(newshop_f, "%d \n", i);
  return (1);
}


int do_string(FILE * shop_f, FILE * newshop_f, char *msg)
{
  char *ptr;

  if ((ptr = fread_string(shop_f, msg)) == NULL)
    return (0);
  fprintf(newshop_f, "%s~\n", ptr);
  free(ptr);
  return (1);
}


/* boot_the_shops_conv() results.  Anything other than CONV_DONE leaves the
 * file as it was; only CONV_ERROR is a failure worth an exit status, since
 * being handed a file that is already v3.0 is not an error. */
#define CONV_DONE	0	/* converted                             */
#define CONV_ALREADY	1	/* already v3.0, nothing to do           */
#define CONV_ERROR	2	/* malformed input, file left untouched  */

static int boot_the_shops_conv(FILE * shop_f, FILE * newshop_f, char *filename)
{
  char *buf, buf2[150];
  int temp, count;

  sprintf(buf2, "beginning of shop file %s", filename);
  fprintf(newshop_f, "CircleMUD %s Shop File~\n", VERSION3_TAG);
  for (;;) {
    if ((buf = fread_string(shop_f, buf2)) == NULL)
      return (CONV_ERROR);
    if (*buf == '#') {		/* New shop */
      sscanf(buf, "#%d\n", &temp);
      sprintf(buf2, "shop #%d in shop file %s", temp, filename);
      fprintf(newshop_f, "#%d~\n", temp);
      free(buf);		/* Plug memory leak! */
      printf("   #%d\n", temp);

      /* Any of these failing means the file is not what it claims to be.
       * CONV_ERROR leaves the file alone, as CONV_ALREADY does, but is
       * the one main() reports in its exit status. */
      if (!do_list(shop_f, newshop_f, MAX_PROD))	/* Produced Items */
	return (CONV_ERROR);

      if (!do_float(shop_f, newshop_f))	/* Ratios */
	return (CONV_ERROR);
      if (!do_float(shop_f, newshop_f))
	return (CONV_ERROR);

      if (!do_list(shop_f, newshop_f, MAX_TRADE))	/* Bought Items */
	return (CONV_ERROR);

      for (count = 0; count < 7; count++)	/* Keeper msgs */
	if (!do_string(shop_f, newshop_f, buf2))
	  return (CONV_ERROR);

      for (count = 0; count < 5; count++)	/* Misc   */
	if (!do_int(shop_f, newshop_f))
	  return (CONV_ERROR);
      fprintf(newshop_f, "-1\n");
      for (count = 0; count < 4; count++)	/* Open/Close     */
	if (!do_int(shop_f, newshop_f))
	  return (CONV_ERROR);

    } else {
      if (*buf == '$') {	/* EOF */
	free(buf);		/* Plug memory leak! */
	fprintf(newshop_f, "$~\n");
	break;
      } else if (strstr(buf, VERSION3_TAG)) {
	printf("%s: New format detected, conversion aborted!\n", filename);
	free(buf);		/* Plug memory leak! */
	return (CONV_ALREADY);
      } else
	free(buf);		/* Anything else: skipped, not kept */
    }
  }
  return (CONV_DONE);
}

int main(int argc, char *argv[])
{
  FILE *sfp, *nsfp;
  char fn[120], part[256];
  int result, index, status = 0;

  if (argc < 2) {
    printf("Usage: shopconv <file1> [file2] [file3] ...\n");
    exit(1);
  }
  for (index = 1; index < argc; index++) {
    sprintf(fn, "%s", argv[index]);
    sprintf(part, "mv %s %s.tmp", fn, fn);
    if (!run(part)) {
      status = 1;
      continue;
    }
    sprintf(part, "%s.tmp", fn);
    sfp = fopen(part, "r");
    if (sfp == NULL) {
      perror(part);
      restore(fn);
      status = 1;
    } else {
      if ((nsfp = fopen(fn, "w")) == NULL) {
	      printf("Error writing to %s.\n", fn);
	      fclose(sfp);
	      restore(fn);
	      status = 1;
	      continue;
      }
      printf("%s:\n", fn);
      result = boot_the_shops_conv(sfp, nsfp, fn);
      fclose(nsfp);
      fclose(sfp);
      if (result != CONV_DONE) {
	      if (result == CONV_ERROR)
	        status = 1;
	      if (!restore(fn))
	        status = 1;
      } else {
	      sprintf(part, "mv %s.tmp %s.bak", fn, fn);
	      /* Only a rename that actually happened leaves the original safe,
	       * so it is what "Done!" is reporting on. */
	      if (run(part))
	        printf("Done!\n");
	      else
	        status = 1;
      }
    }
  }

  return (status);
}
