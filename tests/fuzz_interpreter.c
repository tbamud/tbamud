#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"

extern FILE *logfile;

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
  char *buf;
  char first_arg[MAX_INPUT_LENGTH];
  char second_arg[MAX_INPUT_LENGTH];
  size_t mid;

  logfile = stderr;

  if (!data)
    return 0;
  if (size > 1024)
    return 0;

  buf = malloc(size + 1);
  if (!buf)
    return 0;
  memcpy(buf, data, size);
  buf[size] = '\0';

  (void)is_number(buf);
  (void)delete_doubledollar(buf);
  (void)any_one_arg(buf, first_arg);
  (void)one_word(buf, second_arg);

  mid = size / 2;
  buf[mid] = '\0';
  (void)is_abbrev(buf, buf + mid);

  free(buf);
  logfile = NULL;

  return 0;
}
