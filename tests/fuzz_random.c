#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"

#define RAND_BOUND_SPAN 100001U
#define RAND_BOUND_BIAS 50000
#define DICE_BOUND_SPAN 256U
#define DICE_BOUND_BIAS 32

static uint32_t read_u32(const uint8_t *data, size_t size, size_t offset)
{
  uint32_t v = 0;
  size_t i;

  for (i = 0; i < 4 && offset + i < size; i++)
    v = (v << 8) | data[offset + i];

  return v;
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
  uint32_t seed, from_raw, to_raw, num_raw, sides_raw;
  int from, to, low, high, r, num, sides, d;

  if (!data)
    return 0;

  seed = read_u32(data, size, 0);
  from_raw = read_u32(data, size, 4);
  to_raw = read_u32(data, size, 8);
  num_raw = read_u32(data, size, 12);
  sides_raw = read_u32(data, size, 16);

  circle_srandom(seed);
  (void)circle_random();
  (void)circle_random();

  from = (int)(from_raw % RAND_BOUND_SPAN) - RAND_BOUND_BIAS;
  to = (int)(to_raw % RAND_BOUND_SPAN) - RAND_BOUND_BIAS;
  if (from > to) {
    int tmp = from;
    from = to;
    to = tmp;
  }
  r = rand_number(from, to);
  low = from;
  high = to;
  if (r < low || r > high)
    abort();

  num = (int)(num_raw % DICE_BOUND_SPAN) - DICE_BOUND_BIAS;
  sides = (int)(sides_raw % DICE_BOUND_SPAN) - DICE_BOUND_BIAS;
  d = dice(num, sides);
  if (num <= 0 || sides <= 0) {
    if (d != 0)
      abort();
  } else if (d < num || d > num * sides)
    abort();

  return 0;
}
