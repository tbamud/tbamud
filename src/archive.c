/**************************************************************************
*   File: archive.c                                     Part of tbaMUD    *
*  Usage: tar, gzip and zip containers, written in-process.               *
*                                                                         *
*  All rights reserved.  See license for complete information.            *
**************************************************************************/

/* Everything here works on plain bytes and needs no external program, so
 * `export` can build an archive on any platform the MUD builds on. The
 * compressor is DEFLATE (RFC 1951) with the fixed Huffman code tables:
 * a few percent worse than gzip's dynamic tables, and none of the code
 * that building and emitting those tables would take.
 *
 * 32-bit quantities are held in unsigned long and masked, since the MUD
 * does not assume a C99 <stdint.h>. */

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"
#include "archive.h"

#if defined(CIRCLE_WINDOWS) && !defined(__CYGWIN__)
#include <direct.h>
#else
#include <sys/stat.h>
#include <sys/types.h>
#endif

#define MASK32(x) ((x) & 0xFFFFFFFFUL)

/* ************************************************************************
*  Byte buffers                                                           *
********************************************************************** */

void buf_init(struct byte_buf *b)
{
  b->data = NULL;
  b->len = 0;
  b->cap = 0;
}

void buf_free(struct byte_buf *b)
{
  if (b->data)
    free(b->data);
  buf_init(b);
}

static void buf_reserve(struct byte_buf *b, size_t extra)
{
  size_t want = b->len + extra;
  unsigned char *bigger;

  if (want <= b->cap)
    return;

  if (b->cap == 0)
    b->cap = 1024;
  while (b->cap < want)
    b->cap *= 2;

  if (!(bigger = (unsigned char *) realloc(b->data, b->cap))) {
    perror("SYSERR: archive buffer");
    abort();
  }
  b->data = bigger;
}

void buf_write(struct byte_buf *b, const void *bytes, size_t n)
{
  if (n == 0)
    return;
  buf_reserve(b, n);
  memcpy(b->data + b->len, bytes, n);
  b->len += n;
}

static void buf_byte(struct byte_buf *b, unsigned char c)
{
  buf_reserve(b, 1);
  b->data[b->len++] = c;
}

/** Little-endian 16 and 32 bit fields, as both containers store them. */
static void buf_le16(struct byte_buf *b, unsigned long v)
{
  buf_byte(b, (unsigned char) (v & 0xFF));
  buf_byte(b, (unsigned char) ((v >> 8) & 0xFF));
}

static void buf_le32(struct byte_buf *b, unsigned long v)
{
  buf_le16(b, v & 0xFFFF);
  buf_le16(b, (v >> 16) & 0xFFFF);
}

/* ************************************************************************
*  CRC-32                                                                 *
********************************************************************** */

static unsigned long crc_table[256];
static int crc_table_built = FALSE;

static void build_crc_table(void)
{
  unsigned long c;
  int i, k;

  for (i = 0; i < 256; i++) {
    c = (unsigned long) i;
    for (k = 0; k < 8; k++)
      c = (c & 1) ? (0xEDB88320UL ^ (c >> 1)) : (c >> 1);
    crc_table[i] = c;
  }
  crc_table_built = TRUE;
}

unsigned long archive_crc32(const unsigned char *data, size_t len)
{
  unsigned long c = 0xFFFFFFFFUL;
  size_t i;

  if (!crc_table_built)
    build_crc_table();

  for (i = 0; i < len; i++)
    c = crc_table[(c ^ data[i]) & 0xFF] ^ (c >> 8);

  return MASK32(c ^ 0xFFFFFFFFUL);
}

/* ************************************************************************
*  DEFLATE, fixed Huffman codes                                           *
********************************************************************** */

#define DEF_WINDOW    32768
#define DEF_MIN_MATCH     3
#define DEF_MAX_MATCH   258
#define DEF_HASH_BITS    15
#define DEF_HASH_SIZE (1 << DEF_HASH_BITS)
/* Longest hash chain walked per position: more finds longer matches at a
 * cost, and this is around zlib's default level. */
#define DEF_MAX_CHAIN   128
#define DEF_NONE        0xFFFFFFFFUL

/* RFC 1951 3.2.5: length codes 257-285, distance codes 0-29. */
static const int length_base[29] = {
  3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31, 35, 43, 51,
  59, 67, 83, 99, 115, 131, 163, 195, 227, 258
};
static const int length_extra[29] = {
  0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4,
  5, 5, 5, 5, 0
};
static const int dist_base[30] = {
  1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193, 257, 385,
  513, 769, 1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577
};
static const int dist_extra[30] = {
  0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10,
  10, 11, 11, 12, 12, 13, 13
};

/* Bits leave least-significant first, but a Huffman code's own bits leave
 * most-significant first (RFC 1951 3.1.1). */
struct bit_writer {
  struct byte_buf *out;
  unsigned long acc;
  int nbits;
};

static void bw_bits(struct bit_writer *w, unsigned long value, int count)
{
  w->acc |= MASK32(value << w->nbits);
  w->nbits += count;
  while (w->nbits >= 8) {
    buf_byte(w->out, (unsigned char) (w->acc & 0xFF));
    w->acc >>= 8;
    w->nbits -= 8;
  }
}

static void bw_code(struct bit_writer *w, unsigned long code, int len)
{
  int i;

  for (i = len - 1; i >= 0; i--)
    bw_bits(w, (code >> i) & 1, 1);
}

static void bw_finish(struct bit_writer *w)
{
  if (w->nbits > 0)
    buf_byte(w->out, (unsigned char) (w->acc & 0xFF));
  w->acc = 0;
  w->nbits = 0;
}

/** The fixed literal/length alphabet (RFC 1951 3.2.6). */
static void fixed_code(int sym, unsigned long *code, int *bits)
{
  if (sym <= 143) {
    *code = 0x30UL + sym;
    *bits = 8;
  } else if (sym <= 255) {
    *code = 0x190UL + sym - 144;
    *bits = 9;
  } else if (sym <= 279) {
    *code = (unsigned long) (sym - 256);
    *bits = 7;
  } else {
    *code = 0xC0UL + sym - 280;
    *bits = 8;
  }
}

static int length_code_of(int len)
{
  int i;

  for (i = 28; i >= 0; i--)
    if (length_base[i] <= len)
      return i;
  return 0;
}

static int dist_code_of(int dist)
{
  int i;

  for (i = 29; i >= 0; i--)
    if (dist_base[i] <= dist)
      return i;
  return 0;
}

static unsigned long def_hash(const unsigned char *d, size_t i)
{
  unsigned long h = ((unsigned long) d[i] << 10) ^ ((unsigned long) d[i + 1] << 5)
                  ^ (unsigned long) d[i + 2];

  return (MASK32(h * 0x9E3779B1UL) >> (32 - DEF_HASH_BITS)) % DEF_HASH_SIZE;
}

/** One stored block per 65535 bytes, for data that will not compress. */
static void deflate_stored(struct byte_buf *out, const unsigned char *data, size_t len)
{
  size_t done = 0;

  do {
    size_t take = len - done;
    int last;

    if (take > 0xFFFF)
      take = 0xFFFF;
    last = (done + take == len);

    buf_byte(out, (unsigned char) (last ? 1 : 0));
    buf_le16(out, take);
    buf_le16(out, (~take) & 0xFFFF);
    buf_write(out, data + done, take);
    done += take;
  } while (done < len);
}

void archive_deflate(struct byte_buf *out, const unsigned char *data, size_t len)
{
  struct bit_writer w;
  struct byte_buf tried;
  unsigned long *head, *prev;
  unsigned long code;
  size_t i;
  int bits;

  if (len < DEF_MIN_MATCH) {
    deflate_stored(out, data, len);
    return;
  }

  buf_init(&tried);
  w.out = &tried;
  w.acc = 0;
  w.nbits = 0;

  bw_bits(&w, 1, 1);            /* BFINAL */
  bw_bits(&w, 1, 2);            /* BTYPE = 01, fixed Huffman */

  CREATE(head, unsigned long, DEF_HASH_SIZE);
  CREATE(prev, unsigned long, len);
  for (i = 0; i < DEF_HASH_SIZE; i++)
    head[i] = DEF_NONE;
  for (i = 0; i < len; i++)
    prev[i] = DEF_NONE;

  i = 0;
  while (i < len) {
    size_t best_len = 0, best_dist = 0;

    if (i + DEF_MIN_MATCH <= len) {
      unsigned long candidate = head[def_hash(data, i)];
      size_t limit = (i > DEF_WINDOW) ? i - DEF_WINDOW : 0;
      int chain = 0;

      while (candidate != DEF_NONE && (size_t) candidate >= limit && chain < DEF_MAX_CHAIN) {
        size_t c = (size_t) candidate;
        size_t max = len - i;

        if (max > DEF_MAX_MATCH)
          max = DEF_MAX_MATCH;

        /* Cheap rejection before the full compare. */
        if (max > best_len && data[c + best_len] == data[i + best_len]) {
          size_t l = 0;

          while (l < max && data[c + l] == data[i + l])
            l++;
          if (l > best_len) {
            best_len = l;
            best_dist = i - c;
            if (l == max)
              break;
          }
        }
        candidate = prev[c];
        chain++;
      }
    }

    if (best_len >= DEF_MIN_MATCH) {
      int lc = length_code_of((int) best_len);
      int dc = dist_code_of((int) best_dist);
      size_t k;

      fixed_code(257 + lc, &code, &bits);
      bw_code(&w, code, bits);
      if (length_extra[lc] > 0)
        bw_bits(&w, (unsigned long) (best_len - length_base[lc]), length_extra[lc]);

      bw_code(&w, (unsigned long) dc, 5);
      if (dist_extra[dc] > 0)
        bw_bits(&w, (unsigned long) (best_dist - dist_base[dc]), dist_extra[dc]);

      /* Every position the match covers still has to enter the chains, or
       * later matches will not find them. */
      for (k = i; k < i + best_len; k++)
        if (k + DEF_MIN_MATCH <= len) {
          unsigned long h = def_hash(data, k);
          prev[k] = head[h];
          head[h] = (unsigned long) k;
        }
      i += best_len;
    } else {
      fixed_code((int) data[i], &code, &bits);
      bw_code(&w, code, bits);
      if (i + DEF_MIN_MATCH <= len) {
        unsigned long h = def_hash(data, i);
        prev[i] = head[h];
        head[h] = (unsigned long) i;
      }
      i++;
    }
  }

  fixed_code(256, &code, &bits); /* end of block */
  bw_code(&w, code, bits);
  bw_finish(&w);

  free(head);
  free(prev);

  /* Never do worse than not compressing at all. */
  if (tried.len >= len + 5)
    deflate_stored(out, data, len);
  else
    buf_write(out, tried.data, tried.len);

  buf_free(&tried);
}

/* ************************************************************************
*  tar and gzip                                                           *
********************************************************************** */

/** An octal header field: zero padded to width-1 digits, then a NUL. */
static void tar_octal(char *field, int width, unsigned long value)
{
  char tmp[32];
  int digits = width - 1;
  int n;

  snprintf(tmp, sizeof(tmp), "%0*lo", digits, value);
  n = strlen(tmp);
  /* Values here are sizes and permissions and cannot overflow the field,
   * but keep the low digits rather than smash the header if one ever does. */
  memcpy(field, tmp + (n > digits ? n - digits : 0), digits);
  field[digits] = '\0';
}

void archive_tar(struct byte_buf *out, const struct archive_member *members,
                 int count, time_t mtime)
{
  static const unsigned char zeros[512] = { 0 };
  unsigned char header[512];
  unsigned long sum;
  size_t pad;
  int i, k;

  for (i = 0; i < count; i++) {
    memset(header, 0, sizeof(header));
    strncpy((char *) header, members[i].name, 99);
    tar_octal((char *) header + 100, 8, 0644);           /* mode */
    tar_octal((char *) header + 108, 8, 0);              /* uid */
    tar_octal((char *) header + 116, 8, 0);              /* gid */
    tar_octal((char *) header + 124, 12, (unsigned long) members[i].len);
    tar_octal((char *) header + 136, 12, (unsigned long) mtime);
    header[156] = '0';                                   /* regular file */
    memcpy(header + 257, "ustar", 6);
    memcpy(header + 263, "00", 2);

    /* The checksum is computed with its own field read as spaces. */
    memset(header + 148, ' ', 8);
    sum = 0;
    for (k = 0; k < 512; k++)
      sum += header[k];
    tar_octal((char *) header + 148, 7, sum);
    header[154] = '\0';
    header[155] = ' ';

    buf_write(out, header, sizeof(header));
    buf_write(out, members[i].data, members[i].len);
    pad = (512 - members[i].len % 512) % 512;
    if (pad)
      buf_write(out, zeros, pad);
  }

  /* Two zero blocks end the archive. */
  buf_write(out, zeros, sizeof(zeros));
  buf_write(out, zeros, sizeof(zeros));
}

void archive_gzip(struct byte_buf *out, const unsigned char *data, size_t len,
                  time_t mtime)
{
  static const unsigned char magic[4] = { 0x1f, 0x8b, 0x08, 0x00 };

  buf_write(out, magic, sizeof(magic));
  buf_le32(out, (unsigned long) mtime);
  buf_byte(out, 0x00);                  /* no extra flags */
  buf_byte(out, 0xff);                  /* OS unknown */

  archive_deflate(out, data, len);

  buf_le32(out, archive_crc32(data, len));
  buf_le32(out, MASK32((unsigned long) len));
}

/* ************************************************************************
*  zip                                                                    *
********************************************************************** */

/** Pack a local time into the MS-DOS date and time fields zip stores. */
static void dos_stamp(time_t when, unsigned long *date, unsigned long *time_of_day)
{
  struct tm *t = localtime(&when);
  int year = t ? t->tm_year + 1900 : 1980;

  if (year < 1980)
    year = 1980;
  if (year > 2107)
    year = 2107;

  *date = ((unsigned long) (year - 1980) << 9)
        | ((unsigned long) (t ? t->tm_mon + 1 : 1) << 5)
        | (unsigned long) (t ? t->tm_mday : 1);
  *time_of_day = ((unsigned long) (t ? t->tm_hour : 0) << 11)
        | ((unsigned long) (t ? t->tm_min : 0) << 5)
        | (unsigned long) ((t ? t->tm_sec : 0) / 2);
}

void archive_zip(struct byte_buf *out, const struct archive_member *members,
                 int count, time_t when)
{
  struct byte_buf central, deflated;
  unsigned long dos_date, dos_time, central_offset;
  int i;

  dos_stamp(when, &dos_date, &dos_time);
  buf_init(&central);

  for (i = 0; i < count; i++) {
    unsigned long crc = archive_crc32(members[i].data, members[i].len);
    unsigned long offset = (unsigned long) out->len;
    unsigned long method, stored_len;
    const unsigned char *body;
    size_t name_len = strlen(members[i].name);

    buf_init(&deflated);
    archive_deflate(&deflated, members[i].data, members[i].len);
    if (deflated.len < members[i].len) {
      method = 8;                       /* raw deflate stream */
      body = deflated.data;
      stored_len = (unsigned long) deflated.len;
    } else {
      method = 0;                       /* stored */
      body = members[i].data;
      stored_len = (unsigned long) members[i].len;
    }

    buf_le32(out, 0x04034b50UL);        /* local file header */
    buf_le16(out, 20);                  /* version needed */
    buf_le16(out, 0);                   /* flags */
    buf_le16(out, method);
    buf_le16(out, dos_time);
    buf_le16(out, dos_date);
    buf_le32(out, crc);
    buf_le32(out, stored_len);
    buf_le32(out, (unsigned long) members[i].len);
    buf_le16(out, name_len);
    buf_le16(out, 0);                   /* extra length */
    buf_write(out, members[i].name, name_len);
    buf_write(out, body, stored_len);

    buf_le32(&central, 0x02014b50UL);   /* central directory entry */
    buf_le16(&central, 20);             /* version made by */
    buf_le16(&central, 20);             /* version needed */
    buf_le16(&central, 0);              /* flags */
    buf_le16(&central, method);
    buf_le16(&central, dos_time);
    buf_le16(&central, dos_date);
    buf_le32(&central, crc);
    buf_le32(&central, stored_len);
    buf_le32(&central, (unsigned long) members[i].len);
    buf_le16(&central, name_len);
    buf_le16(&central, 0);              /* extra */
    buf_le16(&central, 0);              /* comment */
    buf_le16(&central, 0);              /* disk number */
    buf_le16(&central, 0);              /* internal attributes */
    buf_le32(&central, 0);              /* external attributes */
    buf_le32(&central, offset);
    buf_write(&central, members[i].name, name_len);

    buf_free(&deflated);
  }

  central_offset = (unsigned long) out->len;
  buf_write(out, central.data, central.len);

  buf_le32(out, 0x06054b50UL);          /* end of central directory */
  buf_le16(out, 0);                     /* this disk */
  buf_le16(out, 0);                     /* disk with the directory */
  buf_le16(out, count);
  buf_le16(out, count);
  buf_le32(out, (unsigned long) central.len);
  buf_le32(out, central_offset);
  buf_le16(out, 0);                     /* comment length */

  buf_free(&central);
}

/* ************************************************************************
*  Platform odds and ends                                                 *
********************************************************************** */

int archive_mkdir(const char *path)
{
#if defined(CIRCLE_WINDOWS) && !defined(__CYGWIN__)
  if (_mkdir(path) == 0 || errno == EEXIST)
    return 0;
#else
  if (mkdir(path, 0775) == 0 || errno == EEXIST)
    return 0;
#endif
  return -1;
}
