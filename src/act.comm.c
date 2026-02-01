/**************************************************************************
*  File: act.comm.c                                        Part of tbaMUD *
*  Usage: Player-level communication commands.                            *
*                                                                         *
*  All rights reserved.  See license for complete information.            *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
**************************************************************************/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "screen.h"
#include "constants.h"
#include "spells.h"
#include "improved-edit.h"
#include "py_triggers.h"
#include "act.h"
#include "modify.h"
#include <ctype.h>
#include <string.h>
#include <strings.h> /* for strncasecmp on POSIX */

static bool legal_communication(char * arg);

static bool legal_communication(char * arg) 
{
  while (*arg) {
    if (*arg == '@') {
      arg++;
      if (*arg == '(' || *arg == ')' || *arg == '<' || *arg == '>')
        return FALSE; 
    }
    arg++;
  }
  return TRUE;
}

static int is_boundary_char(char c) {
  return c == '\0' || isspace((unsigned char)c) || ispunct((unsigned char)c);
}

/* Convert first-person phrases to second-person for self-facing messages. */
static void to_second_person_self(const char *in, char *out, size_t outlen) {
  struct { const char *from; const char *to; } map[] = {
    /* Longer patterns first to avoid partial matches */
    {"i'm",   "you're"},
    {"i’ve",  "you’ve"},
    {"i've",  "you've"},
    {"i’d",   "you’d"},
    {"i'd",   "you'd"},
    {"i’ll",  "you’ll"},
    {"i'll",  "you'll"},
    {"myself","yourself"},
    {"mine",  "yours"},
    {"my",    "your"},
    {"me",    "you"},
    {"i",     "you"}
  };
  const size_t nmap = sizeof(map)/sizeof(map[0]);

  size_t i = 0, o = 0;
  out[0] = '\0';

  while (in[i] && o + 1 < outlen) {
    int replaced = 0;

    if (i == 0 || is_boundary_char(in[i - 1])) {
      for (size_t k = 0; k < nmap; k++) {
        size_t lf = strlen(map[k].from);
        if (strncasecmp(in + i, map[k].from, lf) == 0 && is_boundary_char(in[i + lf])) {
          /* write replacement */
          size_t lt = strlen(map[k].to);
          if (o + lt < outlen) {
            memcpy(out + o, map[k].to, lt);
            o += lt;
            i += lf;
            replaced = 1;
          }
          break;
        }
      }
    }

    if (!replaced) {
      out[o++] = in[i++];
    }
  }

  /* NUL-terminate */
  if (o >= outlen) o = outlen - 1;
  out[o] = '\0';

  /* Trim trailing spaces */
  while (o && isspace((unsigned char)out[o - 1])) out[--o] = '\0';

  /* Ensure trailing sentence punctuation */
  if (o) {
    char last = out[o - 1];
    if (!(last == '.' || last == '!' || last == '?')) {
      if (o + 1 < outlen) {
        out[o++] = '.';
        out[o] = '\0';
      }
    }
  }
}

static void trim_whitespace(char *s) {
  char *start = s;
  while (*start && isspace((unsigned char)*start))
    start++;

  if (start != s)
    memmove(s, start, strlen(start) + 1);

  size_t len = strlen(s);
  while (len > 0 && isspace((unsigned char)s[len - 1]))
    s[--len] = '\0';
}

static bool parse_speech_adornments(struct char_data *ch, char **text,
                                    char *bracket_raw, size_t bracket_sz, bool *has_bracket,
                                    char *paren_raw, size_t paren_sz, bool *has_paren)
{
  char *p = *text;

  if (bracket_raw && bracket_sz > 0)
    bracket_raw[0] = '\0';
  if (paren_raw && paren_sz > 0)
    paren_raw[0] = '\0';
  if (has_bracket)
    *has_bracket = FALSE;
  if (has_paren)
    *has_paren = FALSE;

  while (TRUE) {
    skip_spaces(&p);

    if (*p == '[' && has_bracket && !*has_bracket) {
      const char *close = strchr(p, ']');
      if (!close) {
        send_to_char(ch, "You need a closing ']'.\r\n");
        return FALSE;
      }
      size_t len = (size_t)(close - p - 1);
      if (len >= bracket_sz)
        len = bracket_sz - 1;
      if (len > 0 && bracket_raw) {
        strncpy(bracket_raw, p + 1, len);
        bracket_raw[len] = '\0';
        trim_whitespace(bracket_raw);
      } else if (bracket_raw && bracket_sz > 0)
        bracket_raw[0] = '\0';
      *has_bracket = TRUE;
      p = (char *)close + 1;
      continue;
    }

    if (*p == '(' && has_paren && !*has_paren) {
      const char *close = strchr(p, ')');
      if (!close) {
        send_to_char(ch, "You need a closing ')'.\r\n");
        return FALSE;
      }
      size_t len = (size_t)(close - p - 1);
      if (len >= paren_sz)
        len = paren_sz - 1;
      if (len > 0 && paren_raw) {
        strncpy(paren_raw, p + 1, len);
        paren_raw[len] = '\0';
        trim_whitespace(paren_raw);
      } else if (paren_raw && paren_sz > 0)
        paren_raw[0] = '\0';
      *has_paren = TRUE;
      p = (char *)close + 1;
      continue;
    }

    break;
  }

  *text = p;
  return TRUE;
}

static void wrap_line(const char *src, char *dst, size_t dstsz, int width)
{
  size_t out = 0;
  int col = 0;
  const char *p = src;
  bool first_word = TRUE;

  if (!dst || dstsz == 0)
    return;

  dst[0] = '\0';

  while (*p && out < dstsz - 1) {
    while (*p && isspace((unsigned char)*p) && *p != '\n' && *p != '\r')
      p++;
    if (!*p)
      break;

    const char *word_start = p;
    size_t word_len = 0;
    while (*p && !isspace((unsigned char)*p))
      word_len++, p++;

    if (word_len == 0)
      continue;

    if (!first_word && col + 1 + (int)word_len > width) {
      if (out < dstsz - 2) {
        dst[out++] = '\r';
        dst[out++] = '\n';
      }
      col = 0;
      first_word = TRUE;
    }

    if (!first_word) {
      if (out < dstsz - 1) {
        dst[out++] = ' ';
        col++;
      }
    }

    size_t copy = MIN(word_len, dstsz - 1 - out);
    memcpy(dst + out, word_start, copy);
    out += copy;
    col += word_len;
    first_word = FALSE;

    while (*p && (*p == '\n' || *p == '\r')) {
      if (out < dstsz - 2) {
        dst[out++] = '\r';
        dst[out++] = '\n';
      }
      p++;
      col = 0;
      first_word = TRUE;
    }
  }

  dst[out] = '\0';
}

static void capitalize_leading_you(char *line)
{
  if (!line)
    return;
  if (strn_cmp(line, "you", 3) != 0)
    return;

  char next = line[3];
  if (next && !isspace((unsigned char)next) && next != ',' && next != ':' && next != ';')
    return;

  line[0] = UPPER(line[0]);
}

#define LISTEN_DC_TABLE             10
#define LISTEN_DC_TABLE_REMOTE      21
#define LISTEN_DC_TABLE_REMOTE_CLOSED 26
#define LISTEN_DC_WHISPER           15
#define LISTEN_DC_ROOM              18
#define LISTEN_DC_CLOSED            23
#define LISTEN_MASTERY_MIN          81

static void compose_history_entry(char *out, size_t outsz,
                                  const char *first_line,
                                  const char *speech)
{
  if (!out || outsz == 0)
    return;

  out[0] = '\0';
  if (first_line && *first_line)
    strlcpy(out, first_line, outsz);

  strlcat(out, "\r\n   \"", outsz);
  if (speech && *speech)
    strlcat(out, speech, outsz);
  strlcat(out, "\"", outsz);
}

static bool can_attempt_listen(struct char_data *ch)
{
  if (!ch)
    return FALSE;
  if (!AFF_FLAGGED(ch, AFF_LISTEN))
    return FALSE;
  if (GET_POS(ch) <= POS_SLEEPING)
    return FALSE;
  if (!GET_SKILL(ch, SKILL_PERCEPTION))
    return FALSE;
  return TRUE;
}

static int roll_listen_total(struct char_data *ch)
{
  int total = roll_skill_check(ch, SKILL_PERCEPTION, 0, NULL);

  if (FIGHTING(ch))
    total -= 4;

  return total;
}

static bool perform_listen_check(struct char_data *ch, int difficulty, bool require_mastery)
{
  bool success;

  if (!can_attempt_listen(ch))
    return FALSE;
  if (require_mastery && GET_SKILL(ch, SKILL_PERCEPTION) < LISTEN_MASTERY_MIN)
    return FALSE;

  success = (roll_listen_total(ch) >= difficulty);
  gain_skill(ch, "perception", success);
  return success;
}

static void deliver_listen_output(struct char_data *listener, const char *first_line, const char *speech)
{
  char wrapped_line[MAX_STRING_LENGTH];
  char hist_buf[MAX_STRING_LENGTH];

  wrap_line(first_line, wrapped_line, sizeof(wrapped_line), 80);
  send_to_char(listener, "%s\r\n   \"%s\"\r\n", wrapped_line, speech);

  compose_history_entry(hist_buf, sizeof(hist_buf), wrapped_line, speech);
  add_history(listener, hist_buf, HIST_SAY);
}

static void send_overheard_table(struct char_data *listener,
                                 struct char_data *speaker,
                                 const char *furn_name,
                                 const char *speech,
                                 const struct targeted_phrase *bracket_phrase,
                                 const struct targeted_phrase *paren_phrase)
{
  char prefix[MAX_STRING_LENGTH] = "";
  char suffix[MAX_STRING_LENGTH] = "";
  char first_line[MAX_STRING_LENGTH];
  const char *label = (furn_name && *furn_name) ? furn_name : "the table";

  if (bracket_phrase)
    render_targeted_phrase(speaker, bracket_phrase, FALSE, listener, prefix, sizeof(prefix));
  if (paren_phrase)
    render_targeted_phrase(speaker, paren_phrase, FALSE, listener, suffix, sizeof(suffix));

  strlcpy(first_line, "You overhear ", sizeof(first_line));
  if (*prefix) {
    char capped[MAX_STRING_LENGTH];
    strlcpy(capped, prefix, sizeof(capped));
    CAP(capped);
    strlcat(first_line, capped, sizeof(first_line));
    strlcat(first_line, ", ", sizeof(first_line));
  }

  strlcat(first_line, get_char_sdesc(speaker), sizeof(first_line));
  strlcat(first_line, " at ", sizeof(first_line));
  strlcat(first_line, label, sizeof(first_line));

  if (*suffix) {
    strlcat(first_line, ", ", sizeof(first_line));
    strlcat(first_line, suffix, sizeof(first_line));
  }

  strlcat(first_line, ":", sizeof(first_line));
  deliver_listen_output(listener, first_line, speech);
}

static void send_overheard_whisper(struct char_data *listener,
                                   struct char_data *speaker,
                                   struct char_data *vict,
                                   const char *speech,
                                   const struct targeted_phrase *bracket_phrase,
                                   const struct targeted_phrase *paren_phrase)
{
  char prefix[MAX_STRING_LENGTH] = "";
  char suffix[MAX_STRING_LENGTH] = "";
  char first_line[MAX_STRING_LENGTH];

  if (bracket_phrase)
    render_targeted_phrase(speaker, bracket_phrase, FALSE, listener, prefix, sizeof(prefix));
  if (paren_phrase)
    render_targeted_phrase(speaker, paren_phrase, FALSE, listener, suffix, sizeof(suffix));

  strlcpy(first_line, "You overhear ", sizeof(first_line));
  if (*prefix) {
    char capped[MAX_STRING_LENGTH];
    strlcpy(capped, prefix, sizeof(capped));
    CAP(capped);
    strlcat(first_line, capped, sizeof(first_line));
    strlcat(first_line, ", ", sizeof(first_line));
  }

  strlcat(first_line, get_char_sdesc(speaker), sizeof(first_line));
  strlcat(first_line, " whisper to ", sizeof(first_line));
  strlcat(first_line, get_char_sdesc(vict), sizeof(first_line));

  if (*suffix) {
    strlcat(first_line, ", ", sizeof(first_line));
    strlcat(first_line, suffix, sizeof(first_line));
  }

  strlcat(first_line, ":", sizeof(first_line));
  deliver_listen_output(listener, first_line, speech);
}

static void send_overheard_room(struct char_data *listener,
                                struct char_data *speaker,
                                const char *context_label,
                                const char *dir_name,
                                bool closed_door,
                                const char *speech,
                                const struct targeted_phrase *bracket_phrase,
                                const struct targeted_phrase *paren_phrase)
{
  char prefix[MAX_STRING_LENGTH] = "";
  char suffix[MAX_STRING_LENGTH] = "";
  char first_line[MAX_STRING_LENGTH];

  if (bracket_phrase)
    render_targeted_phrase(speaker, bracket_phrase, FALSE, listener, prefix, sizeof(prefix));
  if (paren_phrase)
    render_targeted_phrase(speaker, paren_phrase, FALSE, listener, suffix, sizeof(suffix));

  strlcpy(first_line, "You overhear ", sizeof(first_line));
  if (*prefix) {
    char capped[MAX_STRING_LENGTH];
    strlcpy(capped, prefix, sizeof(capped));
    CAP(capped);
    strlcat(first_line, capped, sizeof(first_line));
    strlcat(first_line, ", ", sizeof(first_line));
  }

  strlcat(first_line, get_char_sdesc(speaker), sizeof(first_line));
  if (context_label && *context_label) {
    strlcat(first_line, " at ", sizeof(first_line));
    strlcat(first_line, context_label, sizeof(first_line));
  }
  if (closed_door) {
    strlcat(first_line, " through a closed door to the ", sizeof(first_line));
  } else {
    strlcat(first_line, " from the ", sizeof(first_line));
  }
  strlcat(first_line, dir_name ? dir_name : "unknown", sizeof(first_line));

  if (*suffix) {
    strlcat(first_line, ", ", sizeof(first_line));
    strlcat(first_line, suffix, sizeof(first_line));
  }

  strlcat(first_line, ":", sizeof(first_line));
  deliver_listen_output(listener, first_line, speech);
}

static void notify_adjacent_listeners_internal(struct char_data *speaker,
                                               const char *speech,
                                               const struct targeted_phrase *bracket_phrase,
                                               const struct targeted_phrase *paren_phrase,
                                               int open_dc,
                                               int closed_dc,
                                               bool closed_requires_mastery,
                                               const char *context_label)
{
  room_rnum origin;

  if (!speaker || !speech || !*speech)
    return;

  origin = IN_ROOM(speaker);
  if (origin == NOWHERE)
    return;

  for (int dir = 0; dir < NUM_OF_DIRS; dir++) {
    struct room_direction_data *exit = world[origin].dir_option[dir];
    room_rnum other_room;
    bool closed_door;

    if (!exit || exit->to_room == NOWHERE)
      continue;

    other_room = exit->to_room;
    if (ROOM_FLAGGED(origin, ROOM_SOUNDPROOF) || ROOM_FLAGGED(other_room, ROOM_SOUNDPROOF))
      continue;

    closed_door = EXIT_FLAGGED(exit, EX_CLOSED) && EXIT_FLAGGED(exit, EX_ISDOOR);

    for (struct char_data *listener = world[other_room].people; listener; listener = listener->next_in_room) {
      if (!perform_listen_check(listener,
                                closed_door ? closed_dc : open_dc,
                                closed_requires_mastery && closed_door))
        continue;
      send_overheard_room(listener, speaker, context_label, dirs[dir], closed_door,
                          speech, bracket_phrase, paren_phrase);
    }
  }
}

static void notify_adjacent_listeners(struct char_data *speaker,
                                      const char *speech,
                                      const struct targeted_phrase *bracket_phrase,
                                      const struct targeted_phrase *paren_phrase)
{
  notify_adjacent_listeners_internal(speaker, speech,
                                     bracket_phrase, paren_phrase,
                                     LISTEN_DC_ROOM, LISTEN_DC_CLOSED, TRUE, NULL);
}

static void notify_adjacent_table_listeners(struct char_data *speaker,
                                            const char *furn_name,
                                            const char *speech,
                                            const struct targeted_phrase *bracket_phrase,
                                            const struct targeted_phrase *paren_phrase)
{
  if (!furn_name)
    furn_name = "the table";

  notify_adjacent_listeners_internal(speaker, speech,
                                     bracket_phrase, paren_phrase,
                                     LISTEN_DC_TABLE_REMOTE,
                                     LISTEN_DC_TABLE_REMOTE_CLOSED,
                                     TRUE,
                                     furn_name);
}

ACMD(do_say)
{
  char *p = argument;
  char bracket_raw[MAX_INPUT_LENGTH] = "";
  char paren_raw[MAX_INPUT_LENGTH] = "";
  char speech[MAX_INPUT_LENGTH];
  struct targeted_phrase bracket_phrase;
  struct targeted_phrase paren_phrase;
  bool has_bracket = FALSE;
  bool has_paren = FALSE;

  skip_spaces(&p);
  if (!parse_speech_adornments(ch, &p,
                               bracket_raw, sizeof(bracket_raw), &has_bracket,
                               paren_raw, sizeof(paren_raw), &has_paren))
    return;
  skip_spaces(&p);

  if (!*p) {
    send_to_char(ch, "Yes, but WHAT do you want to say?\r\n");
    return;
  }

  strlcpy(speech, p, sizeof(speech));

  if (CONFIG_SPECIAL_IN_COMM && legal_communication(speech))
    parse_at(speech);

  if (*bracket_raw) {
    if (!build_targeted_phrase(ch, bracket_raw, FALSE, &bracket_phrase))
      return;
    has_bracket = TRUE;
  }
  if (*paren_raw) {
    if (!build_targeted_phrase(ch, paren_raw, FALSE, &paren_phrase))
      return;
    has_paren = TRUE;
  }

  bool suppress_self = (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_NOREPEAT));
  bool use_say = (has_bracket || has_paren);

  for (struct char_data *vict = world[IN_ROOM(ch)].people; vict; vict = vict->next_in_room) {
    bool self = (vict == ch);

    if (self && suppress_self)
      continue;
    if (!self && GET_POS(vict) <= POS_SLEEPING)
      continue;

    char prefix[MAX_STRING_LENGTH] = "";
    char suffix[MAX_STRING_LENGTH] = "";
    char first_line[MAX_STRING_LENGTH];
    char speaker[MAX_INPUT_LENGTH];

    if (has_bracket)
      render_targeted_phrase(ch, &bracket_phrase, FALSE, vict, prefix, sizeof(prefix));
    if (has_paren)
      render_targeted_phrase(ch, &paren_phrase, FALSE, vict, suffix, sizeof(suffix));

    if (self)
      strlcpy(speaker, "you", sizeof(speaker));
    else
      strlcpy(speaker, PERS(ch, vict), sizeof(speaker));

    first_line[0] = '\0';
    strlcpy(first_line, "", sizeof(first_line));
    if (*prefix) {
      char capped[MAX_STRING_LENGTH];
      strlcpy(capped, prefix, sizeof(capped));
      CAP(capped);
      strlcpy(first_line, capped, sizeof(first_line));
      strlcat(first_line, ", ", sizeof(first_line));
      strlcat(first_line, speaker, sizeof(first_line));
    } else {
      strlcpy(first_line, speaker, sizeof(first_line));
    }

    strlcat(first_line, (self && use_say) ? " say" : " says", sizeof(first_line));

    if (*suffix) {
      strlcat(first_line, ", ", sizeof(first_line));
      strlcat(first_line, suffix, sizeof(first_line));
    }

    strlcat(first_line, ":", sizeof(first_line));
    if (self)
      capitalize_leading_you(first_line);
    char wrapped_line[MAX_STRING_LENGTH];
    wrap_line(first_line, wrapped_line, sizeof(wrapped_line), 80);
    send_to_char(vict, "%s\r\n   \"%s\"\r\n", wrapped_line, speech);

    if (!self || !suppress_self) {
      char hist_buf[MAX_STRING_LENGTH];
      compose_history_entry(hist_buf, sizeof(hist_buf), wrapped_line, speech);
      add_history(vict, hist_buf, HIST_SAY);
    }
  }

  notify_adjacent_listeners(ch, speech,
                            has_bracket ? &bracket_phrase : NULL,
                            has_paren ? &paren_phrase : NULL);

  if (suppress_self)
    send_to_char(ch, "%s", CONFIG_OK);

  speech_mtrigger(ch, speech);
  speech_wtrigger(ch, speech);
}

ACMD(do_talk)
{
  struct obj_data *furniture = SITTING(ch);
  int allowed_positions = 0;
  char *p = argument;
  char bracket_raw[MAX_INPUT_LENGTH] = "";
  char paren_raw[MAX_INPUT_LENGTH] = "";
  struct targeted_phrase bracket_phrase;
  struct targeted_phrase paren_phrase;
  bool has_bracket = FALSE, has_paren = FALSE;

  if (!furniture || GET_OBJ_TYPE(furniture) != ITEM_FURNITURE) {
    send_to_char(ch, "You need to be seated at a piece of furniture to talk there.\r\n");
    return;
  }

  if (GET_POS(ch) != POS_SITTING) {
    send_to_char(ch, "You need to be sitting first.\r\n");
    return;
  }

  allowed_positions = GET_OBJ_VAL(furniture, VAL_FURN_POSITIONS);
  if (allowed_positions > 0 && !(allowed_positions & (1 << 1))) {
    send_to_char(ch, "That furniture doesn't have any seats.\r\n");
    return;
  }

  skip_spaces(&p);
  if (!parse_speech_adornments(ch, &p,
                               bracket_raw, sizeof(bracket_raw), &has_bracket,
                               paren_raw, sizeof(paren_raw), &has_paren))
    return;
  skip_spaces(&p);

  if (!*p) {
    send_to_char(ch, "Talk what?\r\n");
    return;
  }

  char speech[MAX_INPUT_LENGTH];
  strlcpy(speech, p, sizeof(speech));

  if (CONFIG_SPECIAL_IN_COMM && legal_communication(speech))
    parse_at(speech);

  if (*bracket_raw) {
    if (!build_targeted_phrase(ch, bracket_raw, FALSE, &bracket_phrase))
      return;
    has_bracket = TRUE;
  }
  if (*paren_raw) {
    if (!build_targeted_phrase(ch, paren_raw, FALSE, &paren_phrase))
      return;
    has_paren = TRUE;
  }

  const char *furn_name = (furniture->short_description && *furniture->short_description)
                        ? furniture->short_description : "the furniture";

  bool suppress_self = (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_NOREPEAT));

  for (struct char_data *tch = OBJ_SAT_IN_BY(furniture); tch; tch = NEXT_SITTING(tch)) {
    if (tch == ch)
      continue;
    if (SITTING(tch) != furniture)
      continue;
    if (GET_POS(tch) != POS_SITTING)
      continue;
    if (GET_POS(tch) <= POS_SLEEPING)
      continue;

    char prefix[MAX_STRING_LENGTH] = "";
    char suffix[MAX_STRING_LENGTH] = "";
    char first_line[MAX_STRING_LENGTH];
    const char *speaker = PERS(ch, tch);

    if (has_bracket)
      render_targeted_phrase(ch, &bracket_phrase, FALSE, tch, prefix, sizeof(prefix));
    if (has_paren)
      render_targeted_phrase(ch, &paren_phrase, FALSE, tch, suffix, sizeof(suffix));

    first_line[0] = '\0';
    if (*prefix) {
      char capped[MAX_STRING_LENGTH];
      strlcpy(capped, prefix, sizeof(capped));
      CAP(capped);
      strlcpy(first_line, capped, sizeof(first_line));
      strlcat(first_line, ", ", sizeof(first_line));
      strlcat(first_line, speaker, sizeof(first_line));
    } else {
      strlcpy(first_line, speaker, sizeof(first_line));
    }

    strlcat(first_line, " says", sizeof(first_line));
    strlcat(first_line, ", ", sizeof(first_line));
    if (*suffix) {
      strlcat(first_line, suffix, sizeof(first_line));
      strlcat(first_line, ", ", sizeof(first_line));
    }
    char locbuf[MAX_INPUT_LENGTH];
    snprintf(locbuf, sizeof(locbuf), "at %s,", furn_name);
    strlcat(first_line, locbuf, sizeof(first_line));

    char wrapped_line[MAX_STRING_LENGTH];
    wrap_line(first_line, wrapped_line, sizeof(wrapped_line), 80);
    send_to_char(tch, "%s\r\n   \"%s\"\r\n", wrapped_line, speech);

    char hist_buf[MAX_STRING_LENGTH];
    compose_history_entry(hist_buf, sizeof(hist_buf), wrapped_line, speech);
    add_history(tch, hist_buf, HIST_SAY);
  }

  if (suppress_self)
    send_to_char(ch, "%s", CONFIG_OK);
  else {
    char prefix[MAX_STRING_LENGTH] = "";
    char suffix[MAX_STRING_LENGTH] = "";
    char first_line[MAX_STRING_LENGTH];

    if (has_bracket)
      render_targeted_phrase(ch, &bracket_phrase, FALSE, ch, prefix, sizeof(prefix));
    if (has_paren)
      render_targeted_phrase(ch, &paren_phrase, FALSE, ch, suffix, sizeof(suffix));

    if (*prefix) {
      char capped[MAX_STRING_LENGTH];
      strlcpy(capped, prefix, sizeof(capped));
      CAP(capped);
      strlcpy(first_line, capped, sizeof(first_line));
      strlcat(first_line, ", you", sizeof(first_line));
    } else {
      strlcpy(first_line, "you", sizeof(first_line));
    }

    strlcat(first_line, " say", sizeof(first_line));
    strlcat(first_line, ", ", sizeof(first_line));
    if (*suffix) {
      strlcat(first_line, suffix, sizeof(first_line));
      strlcat(first_line, ", ", sizeof(first_line));
    }
    char locbuf[MAX_INPUT_LENGTH];
    snprintf(locbuf, sizeof(locbuf), "at %s,", furn_name);
    strlcat(first_line, locbuf, sizeof(first_line));
    capitalize_leading_you(first_line);

    char wrapped_line[MAX_STRING_LENGTH];
    wrap_line(first_line, wrapped_line, sizeof(wrapped_line), 80);
    send_to_char(ch, "%s\r\n   \"%s\"\r\n", wrapped_line, speech);
    char hist_buf[MAX_STRING_LENGTH];
    compose_history_entry(hist_buf, sizeof(hist_buf), wrapped_line, speech);
    add_history(ch, hist_buf, HIST_SAY);
  }

  /* Notify others in the room (not seated at this furniture) with an action cue. */
  for (struct char_data *onlooker = world[IN_ROOM(ch)].people; onlooker; onlooker = onlooker->next_in_room) {
    if (onlooker == ch)
      continue;
    if (GET_POS(onlooker) <= POS_SLEEPING)
      continue;
    if (SITTING(onlooker) == furniture && GET_POS(onlooker) == POS_SITTING)
      continue; /* already heard the speech */

    if (perform_listen_check(onlooker, LISTEN_DC_TABLE, FALSE)) {
      send_overheard_table(onlooker, ch, furn_name, speech,
                           has_bracket ? &bracket_phrase : NULL,
                           has_paren ? &paren_phrase : NULL);
      continue;
    }

    char prefix[MAX_STRING_LENGTH] = "";
    char suffix[MAX_STRING_LENGTH] = "";
    char line[MAX_STRING_LENGTH];
    const char *speaker = PERS(ch, onlooker);

    if (has_bracket)
      render_targeted_phrase(ch, &bracket_phrase, FALSE, onlooker, prefix, sizeof(prefix));
    if (has_paren)
      render_targeted_phrase(ch, &paren_phrase, FALSE, onlooker, suffix, sizeof(suffix));

    line[0] = '\0';
    if (*prefix) {
      char capped[MAX_STRING_LENGTH];
      strlcpy(capped, prefix, sizeof(capped));
      CAP(capped);
      strlcpy(line, capped, sizeof(line));
      strlcat(line, ", ", sizeof(line));
      strlcat(line, speaker, sizeof(line));
    } else
      strlcpy(line, speaker, sizeof(line));

    strlcat(line, " says something at ", sizeof(line));
    strlcat(line, furn_name, sizeof(line));

    if (*suffix) {
      strlcat(line, ", ", sizeof(line));
      strlcat(line, suffix, sizeof(line));
    }
    strlcat(line, ".", sizeof(line));

    char wrapped_line[MAX_STRING_LENGTH];
    wrap_line(line, wrapped_line, sizeof(wrapped_line), 80);
    send_to_char(onlooker, "%s\r\n", wrapped_line);
  }

  speech_mtrigger(ch, speech);
  speech_wtrigger(ch, speech);

  notify_adjacent_table_listeners(ch, furn_name, speech,
                                  has_bracket ? &bracket_phrase : NULL,
                                  has_paren ? &paren_phrase : NULL);
}

ACMD(do_ooc)
{
  skip_spaces(&argument);

  if (!*argument)
    send_to_char(ch, "Yes, but WHAT do you want to say OOC?\r\n");
  else {
    char buf[MAX_INPUT_LENGTH + 14], *msg;
    struct char_data *vict;
 
    if (CONFIG_SPECIAL_IN_COMM && legal_communication(argument))
      parse_at(argument);

    snprintf(buf, sizeof(buf), "$n\tn says OOC: '%s'", argument);
    msg = act(buf, FALSE, ch, 0, 0, TO_ROOM | DG_NO_TRIG);

    for (vict = world[IN_ROOM(ch)].people; vict; vict = vict->next_in_room)
      if (vict != ch && GET_POS(vict) > POS_SLEEPING)
        add_history(vict, msg, HIST_SAY);

    if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_NOREPEAT))
      send_to_char(ch, "%s", CONFIG_OK);
    else {
      sprintf(buf, "You say OOC: '%s'", argument);
      msg = act(buf, FALSE, ch, 0, 0, TO_CHAR | DG_NO_TRIG);
      add_history(ch, msg, HIST_SAY);
    }
  }

  /* Trigger check. */
  speech_mtrigger(ch, argument);
  speech_wtrigger(ch, argument);
}

ACMD(do_feel)
{
  char raw[MAX_INPUT_LENGTH];
  char rendered[MAX_INPUT_LENGTH * 2];

  skip_spaces(&argument);

  if (!*argument) {
    send_to_char(ch, "Feel what?\r\n");
    return;
  }

  /* Keep user casing; just copy and convert perspective */
  strlcpy(raw, argument, sizeof(raw));
  to_second_person_self(raw, rendered, sizeof(rendered));

  /* Self-only echo */
  send_to_char(ch, "You feel %s\r\n", rendered);
}

ACMD(do_think)
{
  char thought[MAX_INPUT_LENGTH];
  char feeling[MAX_INPUT_LENGTH];
  char *p = argument;          /* must be mutable for skip_spaces */
  char *close;

  thought[0] = '\0';
  feeling[0] = '\0';

  skip_spaces(&p);

  if (!*p) {
    send_to_char(ch, "Think what?\r\n");
    return;
  }

  /* Optional leading "(...)" becomes the feeling block. */
  if (*p == '(') {
    close = strchr(p + 1, ')');
    if (close) {
      size_t len = (size_t)(close - (p + 1));
      if (len >= sizeof(feeling)) len = sizeof(feeling) - 1;
      strncpy(feeling, p + 1, len);
      feeling[len] = '\0';

      p = close + 1; /* move past ')' */
      while (*p && isspace((unsigned char)*p)) p++; /* skip spaces after ) */
    }
    /* If there's no closing ')', we ignore and treat entire line as thought. */
  }

  if (!*p) {
    send_to_char(ch, "Think what?\r\n");
    return;
  }

  /* The rest is the thought text */
  strlcpy(thought, p, sizeof(thought));
  delete_doubledollar(thought);
  if (*feeling) delete_doubledollar(feeling);

  /* Output (two lines; second indented two spaces) */
  if (*feeling) {
    send_to_char(ch, "You think, feeling %s,\r\n  \"%s\"\r\n", feeling, thought);
  } else {
    send_to_char(ch, "You think,\r\n  \"%s\"\r\n", thought);
  }
}

static void perform_tell(struct char_data *ch, struct char_data *vict, char *arg)
{
  char buf[MAX_STRING_LENGTH], *msg;

  snprintf(buf, sizeof(buf), "%s$n tells you, '%s'%s", CCRED(vict, C_NRM), arg, CCNRM(vict, C_NRM));
  msg = act(buf, FALSE, ch, 0, vict, TO_VICT | TO_SLEEP);
  add_history(vict, msg, HIST_TELL);

  if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_NOREPEAT))
    send_to_char(ch, "%s", CONFIG_OK);
  else {
    snprintf(buf, sizeof(buf), "%sYou tell $N, '%s'%s", CCRED(ch, C_NRM), arg, CCNRM(ch, C_NRM));
    msg = act(buf, FALSE, ch, 0, vict, TO_CHAR | TO_SLEEP);     
    add_history(ch, msg, HIST_TELL);
  }

  if (!IS_NPC(vict) && !IS_NPC(ch))
    GET_LAST_TELL(vict) = GET_IDNUM(ch);
}

static int is_tell_ok(struct char_data *ch, struct char_data *vict)
{
  if (!ch)
    log("SYSERR: is_tell_ok called with no characters");
  else if (!vict)
    send_to_char(ch, "%s", CONFIG_NOPERSON);
  else if (ch == vict)
    send_to_char(ch, "You try to tell yourself something.\r\n");
  else if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_SOUNDPROOF) && (GET_LEVEL(ch) < LVL_GOD))
    send_to_char(ch, "The walls seem to absorb your words.\r\n");
  else if (!IS_NPC(vict) && !vict->desc)        /* linkless */
    act("$E's linkless at the moment.", FALSE, ch, 0, vict, TO_CHAR | TO_SLEEP);
  else if (PLR_FLAGGED(vict, PLR_WRITING))
    act("$E's writing a message right now; try again later.", FALSE, ch, 0, vict, TO_CHAR | TO_SLEEP);
  else if ((!IS_NPC(vict)) || (ROOM_FLAGGED(IN_ROOM(vict), ROOM_SOUNDPROOF) && (GET_LEVEL(ch) < LVL_GOD)))
    act("$E can't hear you.", FALSE, ch, 0, vict, TO_CHAR | TO_SLEEP);
  else
    return (TRUE);

  return (FALSE);
}

/* Yes, do_tell probably could be combined with whisper and ask, but it is
 * called frequently, and should IMHO be kept as tight as possible. */
ACMD(do_tell)
{
  struct char_data *vict = NULL;
  char buf[MAX_INPUT_LENGTH + 25], buf2[MAX_INPUT_LENGTH];  // +25 to make room for constants

  half_chop(argument, buf, buf2);

  if (!*buf || !*buf2)
    send_to_char(ch, "Who do you wish to tell what??\r\n");
  else if (GET_LEVEL(ch) < LVL_IMMORT && !(vict = get_player_vis(ch, buf, NULL, FIND_CHAR_WORLD)))
    send_to_char(ch, "%s", CONFIG_NOPERSON);
  else if (GET_LEVEL(ch) >= LVL_IMMORT && !(vict = get_char_vis(ch, buf, NULL, FIND_CHAR_WORLD)))
    send_to_char(ch, "%s", CONFIG_NOPERSON);
  else if (is_tell_ok(ch, vict)) {
    if (CONFIG_SPECIAL_IN_COMM && legal_communication(argument))
      parse_at(buf2);
    perform_tell(ch, vict, buf2);
	}
}

ACMD(do_reply)
{
  struct char_data *tch = character_list;

  if (IS_NPC(ch))
    return;

  skip_spaces(&argument);

  if (GET_LAST_TELL(ch) == NOBODY)
    send_to_char(ch, "You have nobody to reply to!\r\n");
  else if (!*argument)
    send_to_char(ch, "What is your reply?\r\n");
  else {
    /* Make sure the person you're replying to is still playing by searching
     * for them.  Note, now last tell is stored as player IDnum instead of
     * a pointer, which is much better because it's safer, plus will still
     * work if someone logs out and back in again. A descriptor list based 
     * search would be faster although we could not find link dead people.  
     * Not that they can hear tells anyway. :) -gg 2/24/98 */
    while (tch && (IS_NPC(tch) || GET_IDNUM(tch) != GET_LAST_TELL(ch)))
      tch = tch->next;

    if (!tch)
      send_to_char(ch, "That player is no longer here.\r\n");
    else if (is_tell_ok(ch, tch)) {
      if (CONFIG_SPECIAL_IN_COMM && legal_communication(argument))
        parse_at(argument);
      perform_tell(ch, tch, argument);
		}
  }
}

ACMD(do_spec_comm)
{
  char buf[MAX_INPUT_LENGTH], buf2[MAX_INPUT_LENGTH];
  struct char_data *vict;
  const char *action_sing, *action_plur, *action_others;

  switch (subcmd) {
  case SCMD_WHISPER:
    action_sing = "whisper to";
    action_plur = "whispers to";
    action_others = "$n whispers something to $N.";
    break;

  case SCMD_ASK:
    action_sing = "ask";
    action_plur = "asks";
    action_others = "$n asks $N a question.";
    break;

  default:
    action_sing = "oops";
    action_plur = "oopses";
    action_others = "$n is tongue-tied trying to speak with $N.";
    break;
  }

  half_chop(argument, buf, buf2);

  if (!*buf || !*buf2)
    send_to_char(ch, "Whom do you want to %s.. and what??\r\n", action_sing);
  else if (!(vict = get_char_vis(ch, buf, NULL, FIND_CHAR_ROOM)))
    send_to_char(ch, "%s", CONFIG_NOPERSON);
  else if (vict == ch)
    send_to_char(ch, "You can't get your mouth close enough to your ear...\r\n");
  else {
    if (subcmd == SCMD_WHISPER) {
      char bracket_raw[MAX_INPUT_LENGTH] = "";
      char paren_raw[MAX_INPUT_LENGTH] = "";
      struct targeted_phrase bracket_phrase;
      struct targeted_phrase paren_phrase;
      bool has_bracket = FALSE, has_paren = FALSE;
      char speech[MAX_INPUT_LENGTH];
      char *p = buf2;

      skip_spaces(&p);
      if (!parse_speech_adornments(ch, &p,
                                   bracket_raw, sizeof(bracket_raw), &has_bracket,
                                   paren_raw, sizeof(paren_raw), &has_paren))
        return;
      skip_spaces(&p);

      if (!*p) {
        send_to_char(ch, "Whisper what?\r\n");
        return;
      }

      strlcpy(speech, p, sizeof(speech));

      if (CONFIG_SPECIAL_IN_COMM && legal_communication(speech))
        parse_at(speech);

      if (*bracket_raw) {
        if (!build_targeted_phrase(ch, bracket_raw, FALSE, &bracket_phrase))
          return;
        has_bracket = TRUE;
      }
      if (*paren_raw) {
        if (!build_targeted_phrase(ch, paren_raw, FALSE, &paren_phrase))
          return;
        has_paren = TRUE;
      }

      /* Message to victim */
      {
        char prefix[MAX_STRING_LENGTH] = "";
        char suffix[MAX_STRING_LENGTH] = "";
        char first_line[MAX_STRING_LENGTH];

        if (has_bracket)
          render_targeted_phrase(ch, &bracket_phrase, FALSE, vict, prefix, sizeof(prefix));
        if (has_paren)
          render_targeted_phrase(ch, &paren_phrase, FALSE, vict, suffix, sizeof(suffix));

        const char *speaker = get_char_sdesc(ch);
        if (*prefix) {
          char capped[MAX_STRING_LENGTH];
          strlcpy(capped, prefix, sizeof(capped));
          CAP(capped);
          strlcpy(first_line, capped, sizeof(first_line));
          strlcat(first_line, ", ", sizeof(first_line));
          strlcat(first_line, speaker ? speaker : PERS(ch, vict), sizeof(first_line));
        } else {
          strlcpy(first_line, speaker ? speaker : PERS(ch, vict), sizeof(first_line));
        }

        strlcat(first_line, " whispers to you", sizeof(first_line));
        if (*suffix) {
          strlcat(first_line, ", ", sizeof(first_line));
          strlcat(first_line, suffix, sizeof(first_line));
        }
        strlcat(first_line, ":", sizeof(first_line));

        char wrapped_line[MAX_STRING_LENGTH];
        wrap_line(first_line, wrapped_line, sizeof(wrapped_line), 80);
        send_to_char(vict, "%s\r\n   \"%s\"\r\n", wrapped_line, speech);
      }

      /* Message to self */
      if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_NOREPEAT)) {
        send_to_char(ch, "%s", CONFIG_OK);
      } else {
        char prefix[MAX_STRING_LENGTH] = "";
        char suffix[MAX_STRING_LENGTH] = "";
        char first_line[MAX_STRING_LENGTH];

        if (has_bracket)
          render_targeted_phrase(ch, &bracket_phrase, FALSE, ch, prefix, sizeof(prefix));
        if (has_paren)
          render_targeted_phrase(ch, &paren_phrase, FALSE, ch, suffix, sizeof(suffix));

        if (*prefix) {
          char capped[MAX_STRING_LENGTH];
          strlcpy(capped, prefix, sizeof(capped));
          CAP(capped);
          strlcpy(first_line, capped, sizeof(first_line));
          strlcat(first_line, ", you", sizeof(first_line));
        } else {
          strlcpy(first_line, "you", sizeof(first_line));
          CAP(first_line);
        }

        strlcat(first_line, " whisper to ", sizeof(first_line));
        strlcat(first_line, get_char_sdesc(vict), sizeof(first_line));
        if (*suffix) {
          strlcat(first_line, ", ", sizeof(first_line));
          strlcat(first_line, suffix, sizeof(first_line));
        }
        strlcat(first_line, ":", sizeof(first_line));

        char wrapped_line[MAX_STRING_LENGTH];
        wrap_line(first_line, wrapped_line, sizeof(wrapped_line), 80);
        send_to_char(ch, "%s\r\n   \"%s\"\r\n", wrapped_line, speech);
      }

      /* Onlookers */
      for (struct char_data *onlooker = world[IN_ROOM(ch)].people; onlooker; onlooker = onlooker->next_in_room) {
        if (onlooker == ch || onlooker == vict)
          continue;
        if (GET_POS(onlooker) <= POS_SLEEPING)
          continue;

        if (perform_listen_check(onlooker, LISTEN_DC_WHISPER, FALSE)) {
          send_overheard_whisper(onlooker, ch, vict, speech,
                                 has_bracket ? &bracket_phrase : NULL,
                                 has_paren ? &paren_phrase : NULL);
          continue;
        }

        char prefix[MAX_STRING_LENGTH] = "";
        char suffix[MAX_STRING_LENGTH] = "";
        char line[MAX_STRING_LENGTH];

        if (has_bracket)
          render_targeted_phrase(ch, &bracket_phrase, FALSE, onlooker, prefix, sizeof(prefix));
        if (has_paren)
          render_targeted_phrase(ch, &paren_phrase, FALSE, onlooker, suffix, sizeof(suffix));

        if (*prefix) {
          char capped[MAX_STRING_LENGTH];
          strlcpy(capped, prefix, sizeof(capped));
          CAP(capped);
          strlcpy(line, capped, sizeof(line));
          strlcat(line, ", ", sizeof(line));
          strlcat(line, PERS(ch, onlooker), sizeof(line));
        } else {
          strlcpy(line, PERS(ch, onlooker), sizeof(line));
        }

        strlcat(line, " whispers something to ", sizeof(line));
        strlcat(line, PERS(vict, onlooker), sizeof(line));

        if (*suffix) {
          strlcat(line, ", ", sizeof(line));
          strlcat(line, suffix, sizeof(line));
        }
        strlcat(line, ".", sizeof(line));

        char wrapped_line[MAX_STRING_LENGTH];
        wrap_line(line, wrapped_line, sizeof(wrapped_line), 80);
        send_to_char(onlooker, "%s\r\n", wrapped_line);
      }
    } else {
      char buf1[MAX_STRING_LENGTH];

      if (CONFIG_SPECIAL_IN_COMM && legal_communication(argument))
        parse_at(buf2);

      snprintf(buf1, sizeof(buf1), "$n %s you, '%s'", action_plur, buf2);
      act(buf1, FALSE, ch, 0, vict, TO_VICT);

      if ((!IS_NPC(ch)) && (PRF_FLAGGED(ch, PRF_NOREPEAT))) 
        send_to_char(ch, "%s", CONFIG_OK);
      else
        send_to_char(ch, "You %s %s, '%s'\r\n", action_sing, GET_NAME(vict), buf2);
      act(action_others, FALSE, ch, 0, vict, TO_NOTVICT);
    }
  }
}

ACMD(do_write)
{
  struct obj_data *paper, *pen = NULL;
  char *papername, *penname;
  char buf1[MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH];

  papername = buf1;
  penname = buf2;

  two_arguments(argument, papername, penname);

  if (!ch->desc)
    return;

  if (!*papername) {
    /* Nothing was delivered. */
    send_to_char(ch, "Write?  With what?  ON what?  What are you trying to do?!?\r\n");
    return;
  }
  if (*penname) {
    /* Nothing was delivered. */
    if (!(paper = get_obj_in_list_vis(ch, papername, NULL, ch->carrying))) {
      send_to_char(ch, "You have no %s.\r\n", papername);
      return;
    }
    if (!(pen = get_obj_in_list_vis(ch, penname, NULL, ch->carrying))) {
      send_to_char(ch, "You have no %s.\r\n", penname);
      return;
    }
  } else { /* There was one arg.. let's see what we can find. */
    if (!(paper = get_obj_in_list_vis(ch, papername, NULL, ch->carrying))) {
      send_to_char(ch, "There is no %s in your inventory.\r\n", papername);
      return;
    }
    if (GET_OBJ_TYPE(paper) == ITEM_PEN) { /* Oops, a pen. */
      pen = paper;
      paper = NULL;
    } else if (GET_OBJ_TYPE(paper) != ITEM_NOTE) {
      send_to_char(ch, "That thing has nothing to do with writing.\r\n");
      return;
    }

    /* One object was found.. now for the other one. */
    if (!GET_EQ(ch, WEAR_HOLD)) {
      send_to_char(ch, "You can't write with %s %s alone.\r\n", AN(papername), papername);
      return;
    }
    if (!CAN_SEE_OBJ(ch, GET_EQ(ch, WEAR_HOLD))) {
      send_to_char(ch, "The stuff in your hand is invisible!  Yeech!!\r\n");
      return;
    }
    if (pen)
      paper = GET_EQ(ch, WEAR_HOLD);
    else
      pen = GET_EQ(ch, WEAR_HOLD);
  }

  /* Now let's see what kind of stuff we've found. */
  if (GET_OBJ_TYPE(pen) != ITEM_PEN)
    act("$p is no good for writing with.", FALSE, ch, pen, 0, TO_CHAR);
  else if (GET_OBJ_TYPE(paper) != ITEM_NOTE)
    act("You can't write on $p.", FALSE, ch, paper, 0, TO_CHAR);
  else {
    char *backstr = NULL;

    /* Something on it, display it as that's in input buffer. */
    if (paper->main_description) {
      backstr = strdup(paper->main_description);
      send_to_char(ch, "There's something written on it already:\r\n");
      send_to_char(ch, "%s", paper->main_description);
    }

    /* We can write. */
    act("$n begins to jot down a note.", TRUE, ch, 0, 0, TO_ROOM);
    send_editor_help(ch->desc);
    string_write(ch->desc, &paper->main_description, MAX_NOTE_LENGTH, 0, backstr);
  }
}

ACMD(do_page)
{
  struct descriptor_data *d;
  struct char_data *vict;
  char buf2[MAX_INPUT_LENGTH], arg[MAX_INPUT_LENGTH];

  half_chop(argument, arg, buf2);

  if (IS_NPC(ch))
    send_to_char(ch, "Monsters can't page.. go away.\r\n");
  else if (!*arg)
    send_to_char(ch, "Whom do you wish to page?\r\n");
  else {
    char buf[MAX_STRING_LENGTH];

    snprintf(buf, sizeof(buf), "\007\007*$n* %s", buf2);
    if (!str_cmp(arg, "all")) {
      if (GET_LEVEL(ch) > LVL_GOD) {
	for (d = descriptor_list; d; d = d->next)
	  if (STATE(d) == CON_PLAYING && d->character)
	    act(buf, FALSE, ch, 0, d->character, TO_VICT);
      } else
	send_to_char(ch, "You will never be godly enough to do that!\r\n");
      return;
    }
    if ((vict = get_char_vis(ch, arg, NULL, FIND_CHAR_WORLD))) {
      act(buf, FALSE, ch, 0, vict, TO_VICT);
      if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_NOREPEAT))
	send_to_char(ch, "%s", CONFIG_OK);
      else
	act(buf, FALSE, ch, 0, vict, TO_CHAR);
    } else
      send_to_char(ch, "There is no such person in the game!\r\n");
  }
}

/* Generalized communication function by Fred C. Merkel (Torg). */
ACMD(do_gen_comm)
{
  struct descriptor_data *i;
  char color_on[24];
  char buf1[MAX_INPUT_LENGTH], buf2[MAX_INPUT_LENGTH + 50], *msg;   // + 50 to make room for color codes
  bool emoting = FALSE;

  /* Array of flags which must _not_ be set in order for comm to be heard. */
  int channels[] = {
    0,
    PRF_NOSHOUT,
    0
  };

  int hist_type[] = {
    HIST_SHOUT,
  };

  /* com_msgs: [0] Message if you can't perform the action because of noshout
   *           [1] name of the action
   *           [2] message if you're not on the channel
   *           [3] a color string. */
  const char *com_msgs[][4] = {

    {"You cannot shout!!\r\n",
      "shout",
      "Turn off your noshout flag first!\r\n",
      KYEL},

    {"You cannot gossip your emotions!\r\n",
      "gossip",
      "You aren't even on the channel!\r\n",
      KYEL}
  };

  if (PLR_FLAGGED(ch, PLR_NOSHOUT)) {
    send_to_char(ch, "%s", com_msgs[subcmd][0]);
    return;
  }
  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_SOUNDPROOF) && (GET_LEVEL(ch) < LVL_GOD)) {
    send_to_char(ch, "The walls seem to absorb your words.\r\n");
    return;
  }


  if (subcmd == SCMD_GEMOTE) {
    if (!*argument)
      send_to_char(ch, "Gemote? Yes? Gemote what?\r\n");
    else
      do_gmote(ch, argument, 0, 1);
    return;
  }

  /* Level_can_shout defined in config.c. */
  if (GET_LEVEL(ch) < CONFIG_LEVEL_CAN_SHOUT) {
    send_to_char(ch, "You must be at least level %d before you can %s.\r\n", CONFIG_LEVEL_CAN_SHOUT, com_msgs[subcmd][1]);
    return;
  }
  /* Make sure the char is on the channel. */
  if (!IS_NPC(ch) && PRF_FLAGGED(ch, channels[subcmd])) {
    send_to_char(ch, "%s", com_msgs[subcmd][2]);
    return;
  }

  /* skip leading spaces */
  skip_spaces(&argument);

  /* Make sure that there is something there to say! */
  if (!*argument) {
    send_to_char(ch, "Yes, %s, fine, %s we must, but WHAT???\r\n", com_msgs[subcmd][1], com_msgs[subcmd][1]);
    return;
  }
  /* Set up the color on code. */
  strlcpy(color_on, com_msgs[subcmd][3], sizeof(color_on));

  /* First, set up strings to be given to the communicator. */
  if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_NOREPEAT))
    send_to_char(ch, "%s", CONFIG_OK);
  else {
		if (CONFIG_SPECIAL_IN_COMM && legal_communication(argument))
      parse_at(argument);
      
    snprintf(buf1, sizeof(buf1), "%sYou %s, '%s%s'%s", COLOR_LEV(ch) >= C_CMP ? color_on : "",
        com_msgs[subcmd][1], argument, COLOR_LEV(ch) >= C_CMP ? color_on : "", CCNRM(ch, C_CMP));
    
    msg = act(buf1, FALSE, ch, 0, 0, TO_CHAR | TO_SLEEP);
    add_history(ch, msg, hist_type[subcmd]);
  }
  if (!emoting)
    snprintf(buf1, sizeof(buf1), "$n %ss, '%s'", com_msgs[subcmd][1], argument);

  /* Now send all the strings out. */
  for (i = descriptor_list; i; i = i->next) {
    if (STATE(i) != CON_PLAYING || i == ch->desc || !i->character )
      continue;
    if (!IS_NPC(ch) && (PRF_FLAGGED(i->character, channels[subcmd]) || PLR_FLAGGED(i->character, PLR_WRITING)))
      continue;

    if (ROOM_FLAGGED(IN_ROOM(i->character), ROOM_SOUNDPROOF) && (GET_LEVEL(ch) < LVL_GOD))
      continue;

    if (subcmd == SCMD_SHOUT && ((world[IN_ROOM(ch)].zone != world[IN_ROOM(i->character)].zone) ||
         !AWAKE(i->character)))
      continue;

    snprintf(buf2, sizeof(buf2), "%s%s%s", (COLOR_LEV(i->character) >= C_NRM) ? color_on : "", buf1, KNRM); 
    msg = act(buf2, FALSE, ch, 0, i->character, TO_VICT | TO_SLEEP);
    add_history(i->character, msg, hist_type[subcmd]);
  }
}

/* Currently used for qecho only */
ACMD(do_qcomm)
{
  if (!PRF_FLAGGED(ch, PRF_QUEST)) {
    send_to_char(ch, "You aren't even part of the quest!\r\n");
    return;
  }
  skip_spaces(&argument);

  if (!*argument)
    send_to_char(ch, "%c%s?  Yes, fine, %s we must, but WHAT??\r\n", UPPER(*CMD_NAME), CMD_NAME + 1, CMD_NAME);
  else {
    char buf[MAX_STRING_LENGTH];
    struct descriptor_data *i;
    
    if (CONFIG_SPECIAL_IN_COMM && legal_communication(argument))
      parse_at(argument);

    if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_NOREPEAT))
      send_to_char(ch, "%s", CONFIG_OK);
    else
      act(argument, FALSE, ch, 0, argument, TO_CHAR);

    strlcpy(buf, argument, sizeof(buf));
    mudlog(CMP, MAX(LVL_BUILDER, GET_INVIS_LEV(ch)), TRUE, "(GC) %s qechoed: %s", GET_NAME(ch), argument);
    for (i = descriptor_list; i; i = i->next)
      if (STATE(i) == CON_PLAYING && i != ch->desc && PRF_FLAGGED(i->character, PRF_QUEST))
        act(buf, 0, ch, 0, i->character, TO_VICT | TO_SLEEP);
  }
}
