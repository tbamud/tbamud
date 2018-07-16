/**************************************************************************
*  File: improved-edit.c                                   Part of tbaMUD *
*  Usage: Routines specific to the improved editor.                       *
**************************************************************************/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "db.h"
#include "comm.h"
#include "interpreter.h"
#include "improved-edit.h"
#include "dg_scripts.h"
#include "modify.h"


void send_editor_help(struct descriptor_data *d)
{
  if (using_improved_editor)
    write_to_output(d, "Instructions: /s to save, /h for more options.\r\n");
  else
    write_to_output(d, "Instructions: Type @ on a line by itself to end.\r\n");
}

#if CONFIG_IMPROVED_EDITOR

int improved_editor_execute(struct descriptor_data *d, char *str)
{
  char actions[MAX_INPUT_LENGTH];

  if (*str != '/')
    return STRINGADD_OK;

  strncpy(actions, str + 2, sizeof(actions) - 1);
  actions[sizeof(actions) - 1] = '\0';
  *str = '\0';

  switch (str[1]) {
  case 'a':
    return STRINGADD_ABORT;
  case 'c':
    if (*(d->str)) {
      free(*d->str);
      *(d->str) = NULL;
      write_to_output(d, "Current buffer cleared.\r\n");
    } else
      write_to_output(d, "Current buffer empty.\r\n");
    break;
  case 'd':
    parse_edit_action(PARSE_DELETE, actions, d);
    break;
  case 'e':
    parse_edit_action(PARSE_EDIT, actions, d);
    break;
  case 'f':
    if (*(d->str))
      parse_edit_action(PARSE_FORMAT, actions, d);
    else
      write_to_output(d, "Current buffer empty.\r\n");
    break;
  case 'i':
    if (*(d->str))
      parse_edit_action(PARSE_INSERT, actions, d);
    else
      write_to_output(d, "Current buffer empty.\r\n");
    break;
  case 'h':
    parse_edit_action(PARSE_HELP, actions, d);
    break;
  case 'l':
    if (*d->str)
      parse_edit_action(PARSE_LIST_NORM, actions, d);
    else
      write_to_output(d, "Current buffer empty.\r\n");
    break;
  case 'n':
    if (*d->str)
      parse_edit_action(PARSE_LIST_NUM, actions, d);
    else
      write_to_output(d, "Current buffer empty.\r\n");
    break;
  case 'r':
    parse_edit_action(PARSE_REPLACE, actions, d);
    break;
  case 's':
    return STRINGADD_SAVE;
  case 't':
    parse_edit_action(PARSE_TOGGLE, actions, d);
    break;
  default:
    write_to_output(d, "Invalid option.\r\n");
    break;
  }
  return STRINGADD_ACTION;
}

/* Handle some editor commands. */
void parse_edit_action(int command, char *string, struct descriptor_data *d)
{
  int indent = 0, rep_all = 0, flags = 0, replaced, i, line_low, line_high, j = 0;
  unsigned int total_len;
  char *s, *t, temp;
  char buf[MAX_STRING_LENGTH];
  char buf2[MAX_STRING_LENGTH - 1];

  switch (command) {
  case PARSE_HELP:
    write_to_output(d,
            "Editor command formats: /<letter>\r\n\r\n"
            "/a         -  aborts editor\r\n"
            "/c         -  clears buffer\r\n"
            "/d#        -  deletes a line #\r\n"
            "/e# <text> -  changes the line at # with <text>\r\n"
            "/f         -  formats text\r\n"
            "/fi        -  indented formatting of text\r\n"
            "/h         -  list text editor commands\r\n"
            "/i# <text> -  inserts <text> before line #\r\n"
            "/l         -  lists buffer\r\n"
            "/n         -  lists buffer with line numbers\r\n"
            "/r 'a' 'b' -  replace 1st occurence of text <a> in buffer with text <b>\r\n"
            "/ra 'a' 'b'-  replace all occurences of text <a> within buffer with text <b>\r\n"
            "              usage: /r[a] 'pattern' 'replacement'\r\n"
            "/t         -  toggles '@' and tabs\r\n"
            "/s         -  saves text\r\n");
    break;
  case PARSE_TOGGLE:
    if (!*d->str) {
      write_to_output(d, "No string.\r\n");        
      break;
    }
    bool has_at = FALSE;
    for (char* c = *d->str; *c; ++c) {
      if (*c == '@') {
        if (*(++c) != '@') {
          has_at = TRUE;
          break;
        }
      }
    }
    if (has_at) {
      parse_at(*d->str);
      write_to_output(d, "Toggling (at) into (tab) Characters...\r\n");  
    } else {
      parse_tab(*d->str);
      write_to_output(d, "Toggling (tab) into (at) Characters...\r\n"); 
    }
  break;
  case PARSE_FORMAT:
    if (STATE(d) == CON_TRIGEDIT) {
      write_to_output(d, "Script %sformatted.\r\n", format_script(d) ? "": "not ");
      return;
    }
    while (isalpha(string[j]) && j < 2) {
      if (string[j++] == 'i' && !indent) {
        indent = TRUE;
        flags += FORMAT_INDENT;
      }
    }
    switch (sscanf((indent ? string + 1 : string), " %d - %d ", &line_low, &line_high))
    {
    case -1:
    case 0:
      line_low = 1;
      line_high = 999999;
      break;
    case 1:
      line_high = line_low;
      break;
    case 2:
      if (line_high < line_low) {
        write_to_output(d, "That range is invalid.\\r\\n");
        return;
      }
    break;
    }
    /* in case line_low is negative or zero */
    line_low = MAX(1, line_low);

    format_text(d->str, flags, d, d->max_str, line_low, line_high);
    write_to_output(d, "Text formatted with%s indent.\r\n", (indent ? "" : "out"));
    break;
  case PARSE_REPLACE:
    while (isalpha(string[j]) && j < 2)
      if (string[j++] == 'a' && !indent)
        rep_all = 1;

    if ((s = strtok(string, "'")) == NULL) {
      write_to_output(d, "Invalid format.\r\n");
      return;
    } else if ((s = strtok(NULL, "'")) == NULL) {
      write_to_output(d, "Target string must be enclosed in single quotes.\r\n");
      return;
    } else if ((t = strtok(NULL, "'")) == NULL) {
      write_to_output(d, "No replacement string.\r\n");
      return;
    } else if ((t = strtok(NULL, "'")) == NULL) {
      write_to_output(d, "Replacement string must be enclosed in single quotes.\r\n");
      return;
      /*wb's fix for empty buffer replacement crashing */
    } else if ((!*d->str)) {
      return;
    } else if ((total_len = ((strlen(t) - strlen(s)) + strlen(*d->str))) <= d->max_str) {
      if ((replaced = replace_str(d->str, s, t, rep_all, d->max_str)) > 0) {
        write_to_output(d, "Replaced %d occurence%sof '%s' with '%s'.\r\n", replaced, ((replaced != 1) ? "s " : " "), s, t);
      } else if (replaced == 0) {
        write_to_output(d, "String '%s' not found.\r\n", s);
      } else
        write_to_output(d, "ERROR: Replacement string causes buffer overflow, aborted replace.\r\n");
    } else
      write_to_output(d, "Not enough space left in buffer.\r\n");
    break;
  case PARSE_DELETE:
    switch (sscanf(string, " %d - %d ", &line_low, &line_high)) {
    case 0:
      write_to_output(d, "You must specify a line number or range to delete.\r\n");
      return;
    case 1:
      line_high = line_low;
      break;
    case 2:
      if (line_high < line_low) {
        write_to_output(d, "That range is invalid.\r\n");
        return;
      }
      break;
    }

    i = 1;
    total_len = 1;
    if ((s = *d->str) == NULL) {
      write_to_output(d, "Buffer is empty.\r\n");
      return;
    } else if (line_low > 0) {
      while (s && i < line_low)
        if ((s = strchr(s, '\n')) != NULL) {
          i++;
          s++;
        }
      if (s == NULL || i < line_low) {
        write_to_output(d, "Line(s) out of range; not deleting.\r\n");
        return;
      }
      t = s;
      while (s && i < line_high)
        if ((s = strchr(s, '\n')) != NULL) {
          i++;
          total_len++;
          s++;
        }
      if (s && (s = strchr(s, '\n')) != NULL) {
        while (*(++s))
          *(t++) = *s;
      } else
        total_len--;
      *t = '\0';
      RECREATE(*d->str, char, strlen(*d->str) + 3);

      write_to_output(d, "%d line%sdeleted.\r\n", total_len, (total_len != 1 ? "s " : " "));
    } else {
      write_to_output(d, "Invalid, line numbers to delete must be higher than 0.\r\n");
      return;
    }
    break;
  case PARSE_LIST_NORM:
    /* Note: Rv's buf, buf1, buf2, and arg variables are defined to 32k so they
     * are ok for what we do here. */
    *buf = '\0';
    if (*string)
      switch (sscanf(string, " %d - %d ", &line_low, &line_high)) {
      case 0:
        line_low = 1;
        line_high = 999999;
        break;
      case 1:
        line_high = line_low;
        break;
    } else {
      line_low = 1;
      line_high = 999999;
    }

    if (line_low < 1) {
      write_to_output(d, "Line numbers must be greater than 0.\r\n");
      return;
    } else if (line_high < line_low) {
      write_to_output(d, "That range is invalid.\r\n");
      return;
    }
    *buf = '\0';
    if (line_high < 999999 || line_low > 1)
      snprintf(buf, sizeof(buf), "Current buffer range [%d - %d]:\r\n", line_low, line_high);
    i = 1;
    total_len = 0;
    s = *d->str;
    while (s && (i < line_low))
      if ((s = strchr(s, '\n')) != NULL) {
        i++;
        s++;
      }
    if (i < line_low || s == NULL) {
      write_to_output(d, "Line(s) out of range; no buffer listing.\r\n");
      return;
    }
    t = s;
    while (s && i <= line_high)
      if ((s = strchr(s, '\n')) != NULL) {
        i++;
        total_len++;
        s++;
      }
    if (s) {
      temp = *s;
      *s = '\0';
      strncat(buf, t, sizeof(buf) - strlen(buf) - 1);
      *s = temp;
    } else
      strncat(buf, t, sizeof(buf) - strlen(buf) - 1);
    /* This is kind of annoying...but some people like it. */
    snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), "\r\n%d line%sshown.\r\n", total_len, (total_len != 1) ? "s " : " ");
    page_string(d, buf, TRUE);
    break;
  case PARSE_LIST_NUM:
    /* Note: Rv's buf, buf1, buf2, and arg variables are defined to 32k so they
     * are probably ok for what we do here. */
    *buf = '\0';
    if (*string)
      switch (sscanf(string, " %d - %d ", &line_low, &line_high)) {
      case 0:
        line_low = 1;
        line_high = 999999;
        break;
      case 1:
        line_high = line_low;
        break;
    } else {
      line_low = 1;
      line_high = 999999;
    }

    if (line_low < 1) {
      write_to_output(d, "Line numbers must be greater than 0.\r\n");
      return;
    }
    if (line_high < line_low) {
      write_to_output(d, "That range is invalid.\r\n");
      return;
    }
    *buf = '\0';
    i = 1;
    total_len = 0;
    s = *d->str;
    while (s && i < line_low)
      if ((s = strchr(s, '\n')) != NULL) {
        i++;
        s++;
      }
    if (i < line_low || s == NULL) {
      write_to_output(d, "Line(s) out of range; no buffer listing.\r\n");
      return;
    }
    t = s;
    while (s && i <= line_high)
      if ((s = strchr(s, '\n')) != NULL) {
        i++;
        total_len++;
        s++;
        temp = *s;
        *s = '\0';
        char buf3[9];
        sprintf(buf3, "%4d: ", (i - 1));
        strncat(buf, buf3, sizeof(buf) - strlen(buf) - 1);
        strncat(buf, t, sizeof(buf) - strlen(buf) - 1);
        *s = temp;
        t = s;
      }
    if (s && t) {
      temp = *s;
      *s = '\0';
      strncat(buf, t, sizeof(buf) - strlen(buf) - 1);
      *s = temp;
    } else if (t)
      strncat(buf, t, sizeof(buf) - strlen(buf) - 1);

    page_string(d, buf, TRUE);
    break;

  case PARSE_INSERT:
    half_chop(string, buf, buf2);
    if (*buf == '\0') {
      write_to_output(d, "You must specify a line number before which to insert text.\r\n");
      return;
    }
    line_low = atoi(buf);
    strncat(buf2, "\r\n", sizeof(buf2) - strlen(buf2) - 1);

    i = 1;
    *buf = '\0';
    if ((s = *d->str) == NULL) {
      write_to_output(d, "Buffer is empty, nowhere to insert.\r\n");
      return;
    }
    if (line_low > 0) {
      while (s && (i < line_low))
      if ((s = strchr(s, '\n')) != NULL) {
        i++;
        s++;
      }
      if (i < line_low || s == NULL) {
        write_to_output(d, "Line number out of range; insert aborted.\r\n");
        return;
      }
      temp = *s;
      *s = '\0';
      if ((strlen(*d->str) + strlen(buf2) + strlen(s + 1) + 3) > d->max_str) {
        *s = temp;
        write_to_output(d, "Insert text pushes buffer over maximum size, insert aborted.\r\n");
        return;
      }
      if (*d->str && **d->str)
        strncat(buf, *d->str, sizeof(buf) - strlen(buf) - 1);
      *s = temp;
      strncat(buf, buf2, sizeof(buf) - strlen(buf) - 1);
      if (s && *s)
        strncat(buf, s, sizeof(buf) - strlen(buf) - 1);
      RECREATE(*d->str, char, strlen(buf) + 3);

      strcpy(*d->str, buf);
      write_to_output(d, "Line inserted.\r\n");
    } else {
      write_to_output(d, "Line number must be higher than 0.\r\n");
      return;
    }
    break;

  case PARSE_EDIT:
    half_chop(string, buf, buf2);
    if (*buf == '\0') {
      write_to_output(d, "You must specify a line number at which to change text.\r\n");
      return;
    }
    line_low = atoi(buf);
    strncat(buf2, "\r\n", sizeof(buf2) - strlen(buf2) - 1);

    i = 1;
    *buf = '\0';
    if ((s = *d->str) == NULL) {
      write_to_output(d, "Buffer is empty, nothing to change.\r\n");
      return;
    }
    if (line_low > 0) {
      /* Loop through the text counting \n characters until we get to the line. */
      while (s && i < line_low)
        if ((s = strchr(s, '\n')) != NULL) {
          i++;
          s++;
        }
      /* Make sure that there was a THAT line in the text. */
      if (s == NULL || i < line_low) {
        write_to_output(d, "Line number out of range; change aborted.\r\n");
        return;
      }
      /* If s is the same as *d->str that means I'm at the beginning of the
       * message text and I don't need to put that into the changed buffer. */
      if (s != *d->str) {
        /* First things first .. we get this part into the buffer. */
        temp = *s;
        *s = '\0';
        /* Put the first 'good' half of the text into storage. */
        strncat(buf, *d->str, sizeof(buf) - strlen(buf) - 1);
        *s = temp;
      }
      /* Put the new 'good' line into place. */
      strncat(buf, buf2, sizeof(buf) - strlen(buf) - 1);
      if ((s = strchr(s, '\n')) != NULL) {
        /* This means that we are at the END of the line, we want out of there,
         * but we want s to point to the beginning of the line. AFTER the line 
         * we want edited. */
        s++;
        /* Now put the last 'good' half of buffer into storage. */
        strncat(buf, s, sizeof(buf) - strlen(buf) - 1);
      }
      /* Check for buffer overflow. */
      if (strlen(buf) > d->max_str) {
        write_to_output(d, "Change causes new length to exceed buffer maximum size, aborted.\r\n");
        return;
      }
      /* Change the size of the REAL buffer to fit the new text. */
      RECREATE(*d->str, char, strlen(buf) + 3);
      strcpy(*d->str, buf);
      write_to_output(d, "Line changed.\r\n");
    } else {
      write_to_output(d, "Line number must be higher than 0.\r\n");
      return;
    }
    break;
  default:
    write_to_output(d, "Invalid option.\r\n");
    mudlog(BRF, LVL_IMPL, TRUE, "SYSERR: invalid command passed to parse_edit_action");
    return;
  }
}

/* Re-formats message type formatted char *. (for strings edited with d->str) 
 * (mostly olc and mail). */
int format_text(char **ptr_string, int mode, struct descriptor_data *d, unsigned int maxlen, int low, int high)
{
  int line_chars, cap_next = TRUE, cap_next_next = FALSE, color_chars = 0, i, pass_line = 0;
  char *flow, *start = NULL, temp;
  char formatted[MAX_STRING_LENGTH] = "";
  char str[MAX_STRING_LENGTH];

  /* Fix memory overrun. */
  if (d->max_str > MAX_STRING_LENGTH) {
    log("SYSERR: format_text: max_str is greater than buffer size.");
    return 0;
  }

  /* XXX: Want to make sure the string doesn't grow either... */
  if ((flow = *ptr_string) == NULL)
    return 0;

  strncpy(str, flow, sizeof(str) - 1);

  for (i = 0; i < low - 1; i++) {
    start = strtok(str, "\n");
    if (!start) {
      write_to_output(d, "There aren't that many lines!\r\n");
      return 0;
    }
    strncat(formatted, strcat(start, "\n"), sizeof(formatted) - strlen(formatted) - 1);
    flow = strstr(flow, "\n");
    strncpy(str, ++flow, sizeof(str) - 1);
  }

  if (IS_SET(mode, FORMAT_INDENT)) {
    strncat(formatted, "   ", sizeof(formatted) - strlen(formatted) - 1);
    line_chars = 3;
  } else {
    line_chars = 0;
  }

  while (*flow && i < high) {
    while (*flow && strchr("\n\r\f\v ", *flow)) {
      if (*flow == '\n' && !pass_line)
        if (i++ >= high) {
          pass_line = 1;
          break;
        }
      flow++;
    }

    if (*flow) {
      start = flow;
      while (*flow && !strchr("\n\r\f\v .?!", *flow)) {
        if (*flow == '\t') {
          if (*(flow + 1) == '\t')
            color_chars++;
          else if (*(flow + 1) == '[')
            color_chars += 7;
          else
            color_chars += 2;
          flow++;
        }
        flow++;
      }

      if (cap_next_next) {
        cap_next_next = FALSE;
        cap_next = TRUE;
      }

      /* This is so that if we stopped on a sentence, we move off the sentence 
       * delimiter. */
      while (strchr(".!?", *flow)) {
        cap_next_next = TRUE;
        flow++;
      }

      /* Special case: if we're at the end of the last line, and the last
       * character is a delimiter, the flow++ above will have *flow pointing
       * to the \r (or \n) character after the delimiter. Thus *flow will be 
       * non-null, and an extra (blank) line might be added erroneously. We 
       * fix it by skipping the newline characters in between. - Welcor */
      if (strchr("\n\r", *flow)) {
        *flow = '\0';  /* terminate 'start' string */
        flow++;        /* we know this is safe     */
        if (*flow == '\n' && i++ >= high)
          pass_line = 1;

        while (*flow && strchr("\n\r", *flow) && !pass_line) {
          flow++;      /* skip to next non-delimiter */
          if (*flow == '\n' && i++ >= high)
            pass_line = 1;
        }
        temp = *flow;  /* save this char             */
     } else {
        temp = *flow;
        *flow = '\0';
      }

      if (line_chars + strlen(start) + 1 - color_chars > PAGE_WIDTH) {
        strncat(formatted, "\r\n", sizeof(formatted) - strlen(formatted) - 1);
        line_chars = 0;
        color_chars = count_color_chars(start);
      }

      if (!cap_next) {
        if (line_chars > 0) {
          strncat(formatted, " ", sizeof(formatted) - strlen(formatted) - 1);
          line_chars++;
        }
      } else {
        cap_next = FALSE;
        CAP(start);
      }

      line_chars += strlen(start);
      strncat(formatted, start, sizeof(formatted) - strlen(formatted) - 1);

      *flow = temp;
    }

    if (cap_next_next && *flow) {
      if (line_chars + 3 - color_chars > PAGE_WIDTH) {
        strncat(formatted, "\r\n", sizeof(formatted) - strlen(formatted) - 1);
        line_chars = 0;
        color_chars = count_color_chars(start);
      } else if (*flow == '\"' || *flow == '\'') {
        char buf[MAX_STRING_LENGTH - 1];
        snprintf(buf, sizeof(buf), "%c  ", *flow);
        strncat(formatted, buf, sizeof(formatted) - strlen(formatted) - 1);
        flow++;
        line_chars++;
      } else {
        strncat(formatted, "  ", sizeof(formatted) - strlen(formatted) - 1);
        line_chars += 2;
      }
    }
  }
  if (*flow)
    strncat(formatted, "\r\n", sizeof(formatted) - strlen(formatted) - 1);
  strncat(formatted, flow, sizeof(formatted) - strlen(formatted) - 1);
  if (!*flow)
    strncat(formatted, "\r\n", sizeof(formatted) - strlen(formatted) - 1);

  int len = MIN(maxlen, strlen(formatted) + 1);
  RECREATE(*ptr_string, char, len);
  strncpy(*ptr_string, formatted, len - 1);
  (*ptr_string)[len - 1] = '\0';
  return 1;
}

int replace_str(char **string, char *pattern, char *replacement, int rep_all, unsigned int max_size)
{
  char *replace_buffer = NULL;
  char *flow, *jetsam, temp;
  int len, i;

  if ((strlen(*string) - strlen(pattern)) + strlen(replacement) > max_size)
    return -1;

  CREATE(replace_buffer, char, max_size);
  i = 0;
  jetsam = *string;
  flow = *string;
  *replace_buffer = '\0';

  if (rep_all) {
    while ((flow = (char *)strstr(flow, pattern)) != NULL) {
      i++;
      temp = *flow;
      *flow = '\0';
      if ((strlen(replace_buffer) + strlen(jetsam) + strlen(replacement)) > max_size) {
        i = -1;
        break;
      }
      strncat(replace_buffer, jetsam, max_size - strlen(replace_buffer) -1);
      strncat(replace_buffer, replacement, max_size - strlen(replace_buffer) - 1);
      *flow = temp;
      flow += strlen(pattern);
      jetsam = flow;
    }
    strncat(replace_buffer, jetsam, max_size - strlen(replace_buffer) - 1);
  } else {
    if ((flow = (char *)strstr(*string, pattern)) != NULL) {
      i++;
      flow += strlen(pattern);
      len = ((char *)flow - (char *)*string) - strlen(pattern);
      strncpy(replace_buffer, *string, len < max_size - 1 ? len : max_size - 1);
      replace_buffer[max_size - 1] = '\0';
      strncat(replace_buffer, replacement, max_size - strlen(replace_buffer) - 1);
      strncat(replace_buffer, flow, max_size - strlen(replace_buffer) - 1);
    }
  }

  if (i <= 0)
    return 0;
  else {
    RECREATE(*string, char, strlen(replace_buffer) + 3);
    strcpy(*string, replace_buffer);
  }
  free(replace_buffer);
  return i;
}

#endif
