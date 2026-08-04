/**
 * @file dg_olc_syntax.c
 * @brief Core tokenisation and classification of DG scripts.
 *
 * @details Holds the growable buffers, the structural command tables and the
 * line parser shared by trigedit's highlighting and formatting.
 *
 * @note DG Scripts Code Syntax Format and Highlighting.
 * Developed by Victor Augusto Borges Dias de Almeida (aka Stoneheart) in Brazil.
 * Created and tested on BrMUD Engine, ported to tbaMUD.
 * BrMUD: Tormenta - play.brmud.com.br 4000
 *
 * @author Victor Augusto Borges Dias de Almeida (Stoneheart) (BrMUD Engine, 2002-2026).
 * @copyright Victor Augusto Borges Dias de Almeida (C) 2002 - 2026.
 * @date 2026-06-26
 */

#include <stdint.h>

#include "conf.h"
#include "sysdep.h"
#include "structs.h"

#include "dg_olc_syntax.h"
#include "dg_scripts.h"
#include "oasis.h"
#include "utils.h"

/**
 * @brief Table of the DG Scripts flow control keywords, terminated by an entry with name = NULL.
 */
static const struct dg_command_spec control_commands[] = {
    { "if",         true,   true  },
    { "elseif",     true,   true  },
    { "else",       false,  false },
    { "end",        false,  false },
    { "while",      true,   true  },
    { "switch",     true,   true  },
    { "case",       true,   true  },
    { "default",    false,  false },
    { "break",      false,  false },
    { "done",       false,  false },
    { NULL,         false,  false }
};

/**
 * @brief Table of the generic script commands, terminated by name = NULL.
 * @details Holds the commands that are not specific to mobs, objects or rooms.
 */
static const struct dg_command_spec generic_commands[] = {
    { "eval",       true,   true  },
    { "nop",        true,   false },
    { "extract",    true,   false },
    { "dg_letter",  true,   false },
    { "makeuid",    true,   false },
    { "dg_cast",    true,   false },
    { "dg_affect",  true,   false },
    { "global",     true,   false },
    { "context",    true,   false },
    { "remote",     true,   false },
    { "rdelete",    true,   false },
    { "return",     true,   true  },
    { "set",        true,   false },
    { "unset",      true,   false },
    { "wait",       true,   false },
    { "attach",     true,   false },
    { "detach",     true,   false },
    { "halt",       false,  false },
    { NULL,         false,  false }
};

/**
 * @brief Duplicates a string into a freshly allocated buffer.
 *
 * @param source Text to duplicate; NULL returns NULL.
 * @return New buffer holding the copy, or NULL if source is null or the allocation fails.
 */
char *dg_syntax_str_dup(const char *source)
{
    char *copy;
    size_t length;

    if (!source) {
        return NULL;
    }

    length = strlen(source) + 1;
    /* The cast keeps MEMORY_DEBUG builds quiet: zmalloc.h redefines malloc()
     * to a function returning unsigned char *. */
    copy = (char *)malloc(length);

    if (copy) {
        memcpy(copy, source, length);
    }

    return copy;
}

/**
 * @brief Ensures room for required bytes, including the null terminator.
 *
 * @details Does nothing when the current capacity is already enough.
 * Otherwise doubles the capacity starting at 128 until it reaches required
 * (guarded against size_t overflow) and reallocates buffer->data.
 *
 * @param buffer Buffer to grow.
 * @param required Total bytes needed (including the null terminator).
 * @return true if the buffer already had or now has enough room; false if the reallocation fails.
 */
static bool syntax_buffer_reserve(struct syntax_text_buffer *buffer, size_t required)
{
    if (required <= buffer->capacity) {
        return true;
    }

    size_t capacity = buffer->capacity ? buffer->capacity : 128;

    while (capacity < required) {
        if (capacity > SIZE_MAX / 2) {
            capacity = required;
            break;
        }
        capacity *= 2;
    }

    char *expanded = (char *)realloc(buffer->data, capacity);

    if (!expanded) {
        return false;
    }

    buffer->data = expanded;
    buffer->capacity = capacity;

    return true;
}

/**
 * @brief Appends len bytes to the buffer and keeps it null-terminated.
 *
 * @param buffer Destination buffer.
 * @param text Start of the bytes to copy (does not need to be null-terminated).
 * @param len Number of bytes to copy from text.
 * @return true if the bytes were appended; false on invalid argument, overflow or allocation failure.
 */
bool dg_syntax_buffer_append_n(struct syntax_text_buffer *buffer, const char *text, size_t len)
{
    if (!buffer || !text || len > SIZE_MAX - buffer->length - 1
        || !syntax_buffer_reserve(buffer, buffer->length + len + 1)) {
        return false;
    }

    memcpy(buffer->data + buffer->length, text, len);
    buffer->length += len;
    buffer->data[buffer->length] = '\0';

    return true;
}

/**
 * @brief Appends a whole string to the buffer.
 *
 * @param buffer Destination buffer.
 * @param text Null-terminated text to copy.
 * @return true if the text was appended; false if text is null or dg_syntax_buffer_append_n() fails.
 */
bool dg_syntax_buffer_append(struct syntax_text_buffer *buffer, const char *text)
{
    return text && dg_syntax_buffer_append_n(buffer, text, strlen(text));
}

/**
 * @brief Copies a range of characters into a null-terminated buffer.
 *
 * @param start Start of the range to copy.
 * @param len Number of characters to copy.
 * @return New buffer with the null-terminated copy, or NULL if start is null, len is invalid or the allocation fails.
 */
char *dg_syntax_copy_text_range(const char *start, size_t len)
{
    if (!start || len == SIZE_MAX) {
        return NULL;
    }

    char *copy = (char *)malloc(len + 1);

    if (!copy) {
        return NULL;
    }

    memcpy(copy, start, len);
    copy[len] = '\0';
    return copy;
}

/**
 * @brief Copies the command token of a parsed line into a null-terminated buffer.
 *
 * @details Shared by the classification (which needs the command to query the
 * dispatchers) and by the format_script() error messages (which only need a
 * readable excerpt). When the token does not fit in dest the copy is
 * truncated and the function returns false.
 *
 * @param parsed Line already analysed by dg_syntax_parse_line().
 * @param dest Destination buffer.
 * @param dest_size Size of dest in bytes, including the null terminator.
 * @return true if the whole token fit; false if it was truncated or some argument is invalid.
 */
bool dg_syntax_copy_command(const struct dg_parsed_line *parsed, char *dest, size_t dest_size)
{
    if (!parsed || !parsed->first || !dest || !dest_size) {
        return false;
    }

    size_t length = parsed->command_length < dest_size ? parsed->command_length : dest_size - 1;

    memcpy(dest, parsed->first, length);
    dest[length] = '\0';

    return length == parsed->command_length;
}

/**
 * @brief Compares a token that is not null-terminated against a command name.
 *
 * @param token Start of the token within the original line.
 * @param token_length Length of the token.
 * @param name Null-terminated command name to compare against.
 * @return true if token and name have the same length and contents.
 */
bool dg_syntax_token_equals(const char *token, size_t token_length, const char *name)
{
    return token && name && strlen(name) == token_length && !strn_cmp(token, name, token_length);
}

/**
 * @brief Looks a token up in a table of control or generic commands.
 *
 * @param commands Table to search (control_commands[] or generic_commands[]), terminated by name = NULL.
 * @param token Start of the token to look for.
 * @param token_length Length of the token.
 * @return Pointer to the matching entry, or NULL if none matches.
 */
static const struct dg_command_spec *find_command_spec(
    const struct dg_command_spec *commands, const char *token, size_t token_length)
{
    for (size_t i = 0; commands[i].name; i++) {
        if (dg_syntax_token_equals(token, token_length, commands[i].name)) {
            return &commands[i];
        }
    }

    return NULL;
}

/**
 * @brief Returns the type of the trigger being edited, or DG_TRIGGER_ANY outside trigedit.
 *
 * @param d Descriptor being inspected.
 * @return MOB_TRIGGER, OBJ_TRIGGER or WLD_TRIGGER if d is editing a trigger
 * with a valid attach_type; DG_TRIGGER_ANY otherwise.
 */
int dg_syntax_descriptor_trigger_type(const struct descriptor_data *d)
{
    if (d && d->olc && OLC_TRIG(d)
    && OLC_TRIG(d)->attach_type >= MOB_TRIGGER
    && OLC_TRIG(d)->attach_type <= WLD_TRIGGER) {
        return OLC_TRIG(d)->attach_type;
    }

    return DG_TRIGGER_ANY;
}

/**
 * @brief Queries the real dispatchers to classify a specific command.
 *
 * @details Uses dg_mob_command_exists()/dg_obj_command_exists()/
 * dg_wld_command_exists()/dg_player_command_exists() (the very dispatch
 * tables used at runtime, not a list of its own) to decide the category;
 * when trigger_type is a specific type, only the matching table (plus the
 * player/social one) is queried.
 *
 * @param command Command word to classify.
 * @param trigger_type MOB_TRIGGER, OBJ_TRIGGER, WLD_TRIGGER or DG_TRIGGER_ANY.
 * @return The matching category, or DG_LINE_UNKNOWN if no dispatcher recognises the command.
 */
static enum dg_line_kind classify_entity_command(const char *command, int trigger_type)
{
    if (!command || !*command) {
        return DG_LINE_UNKNOWN;
    }

    if ((trigger_type == DG_TRIGGER_ANY || trigger_type == MOB_TRIGGER)
        && dg_mob_command_exists(command)) {
        return DG_LINE_MOB;
    }

    if ((trigger_type == DG_TRIGGER_ANY || trigger_type == OBJ_TRIGGER)
        && dg_obj_command_exists(command)) {
        return DG_LINE_OBJ;
    }

    if ((trigger_type == DG_TRIGGER_ANY || trigger_type == WLD_TRIGGER)
        && dg_wld_command_exists(command)) {
        return DG_LINE_WLD;
    }

    if ((trigger_type == DG_TRIGGER_ANY || trigger_type == MOB_TRIGGER)
        && dg_player_command_exists(command)) {
        return DG_LINE_PLAYER;
    }

    return DG_LINE_UNKNOWN;
}

/**
 * @brief Splits off the first token and classifies the line without touching the source.
 *
 * @details Skips leading spaces and recognises empty and comment lines (first
 * non-space character `*`) before extracting the command. Classification
 * follows this order: control_commands[], then generic_commands[], then the
 * `%` prefix (dynamic command), then classify_entity_command().
 *
 * @param line Script line to analyse.
 * @param trigger_type MOB_TRIGGER, OBJ_TRIGGER, WLD_TRIGGER or DG_TRIGGER_ANY.
 * @param parsed Output struct, filled in with the result.
 * @return false only if line or parsed are null; true in every other case, even with kind DG_LINE_UNKNOWN.
 */
bool dg_syntax_parse_line(const char *line, int trigger_type, struct dg_parsed_line *parsed)
{
    char command[MAX_INPUT_LENGTH];

    if (!line || !parsed) {
        return false;
    }

    memset(parsed, 0, sizeof(*parsed));
    parsed->first = line;

    while (isspace((unsigned char)*parsed->first)) {
        parsed->first++;
    }

    if (!*parsed->first) {
        parsed->kind = DG_LINE_EMPTY;
        parsed->argument = parsed->first;
        return true;
    }

    if (*parsed->first == '*') {
        parsed->kind = DG_LINE_COMMENT;
        parsed->argument = parsed->first + strlen(parsed->first);
        return true;
    }

    while (parsed->first[parsed->command_length]
        && !isspace((unsigned char)parsed->first[parsed->command_length])) {
        parsed->command_length++;
    }

    parsed->argument = parsed->first + parsed->command_length;

    while (*parsed->argument && isspace((unsigned char)*parsed->argument)) {
        parsed->argument++;
    }

    parsed->spec = find_command_spec(control_commands, parsed->first, parsed->command_length);

    if (parsed->spec) {
        parsed->kind = DG_LINE_CONTROL;
        return true;
    }

    parsed->spec = find_command_spec(generic_commands, parsed->first, parsed->command_length);

    if (parsed->spec) {
        parsed->kind = DG_LINE_GENERIC;
        return true;
    }

    if (*parsed->first == '%') {
        parsed->kind = DG_LINE_DYNAMIC;
        return true;
    }

    if (!dg_syntax_copy_command(parsed, command, sizeof(command))) {
        /* No DG command comes close to MAX_INPUT_LENGTH: if it did not fit,
         * it does not exist. */
        parsed->kind = DG_LINE_UNKNOWN;
        return true;
    }

    parsed->kind = classify_entity_command(command, trigger_type);

    return true;
}
