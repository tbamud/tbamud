#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
  char *buf, *left, *right, *trimmed;
  size_t mid;

  if (!data)
    return 0;
  if (size > 1024)
    return 0;

  buf = malloc(size + 1);
  if (!buf)
    return 0;
  memcpy(buf, data, size);
  buf[size] = '\0';

  prune_crlf(buf);
  (void)str_cmp(buf, buf);
  (void)strn_cmp(buf, buf, (int)size);
  (void)count_color_chars(buf);
  (void)count_non_protocol_chars(buf);

  if (size > 0) {
    trimmed = right_trim_whitespace(buf);
    free(trimmed);
  }

  mid = size / 2;
  left = malloc(mid + 1);
  right = malloc((size - mid) + 1);
  if (left && right) {
    memcpy(left, data, mid);
    left[mid] = '\0';
    memcpy(right, data + mid, size - mid);
    right[size - mid] = '\0';
    if (size <= 128)
      (void)levenshtein_distance(left, right);
    if (*right != '\0')
      remove_from_string(left, right);
  }
  free(left);
  free(right);
  free(buf);

  return 0;
}
