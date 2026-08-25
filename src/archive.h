/**
* @file archive.h
* Archive containers written in-process: tar, gzip and zip.
*
* Part of the core tbaMUD source code distribution, which is a derivative
* of, and continuation of, CircleMUD.
*
* Written so that `export` does not have to shell out. Nothing here needs
* an external program, a temporary file or a shell, which is what makes
* the command work identically on every platform the MUD builds on.
*/
#ifndef _ARCHIVE_H_
#define _ARCHIVE_H_

/** A growable byte buffer. Zero it, append to it, free it. */
struct byte_buf {
  unsigned char *data;
  size_t len;
  size_t cap;
};

void buf_init(struct byte_buf *b);
void buf_free(struct byte_buf *b);
void buf_write(struct byte_buf *b, const void *bytes, size_t n);

/** One member of an archive. */
struct archive_member {
  const char *name;
  const unsigned char *data;
  size_t len;
};

/** CRC-32/ISO-HDLC, as used by both gzip and zip. */
unsigned long archive_crc32(const unsigned char *data, size_t len);

/**
 * Compress to a raw DEFLATE stream (RFC 1951), fixed Huffman codes.
 * Data that will not compress is emitted in stored blocks instead, so the
 * result never exceeds the input by more than the framing.
 */
void archive_deflate(struct byte_buf *out, const unsigned char *data, size_t len);

/** USTAR archive of every member, stamped with mtime. */
void archive_tar(struct byte_buf *out, const struct archive_member *members,
                 int count, time_t mtime);

/** gzip container around a deflate stream of data. */
void archive_gzip(struct byte_buf *out, const unsigned char *data, size_t len,
                  time_t mtime);

/** ZIP archive; members are deflated unless stored is smaller. */
void archive_zip(struct byte_buf *out, const struct archive_member *members,
                 int count, time_t when);

/** Create a directory, whatever the platform calls it. Returns 0 on
 * success, or if it already exists. */
int archive_mkdir(const char *path);

#endif /* _ARCHIVE_H_ */
