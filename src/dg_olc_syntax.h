/**
 * @file dg_olc_syntax.h
 * @brief Public interface and shared structures of the DG script analyser.
 *
 * @details The declarations in the first section are used by the rest of the
 * MUD. The shared section is meant for the dg_olc_syntax*.c modules only; it
 * stays in this file as an architectural decision of the project.
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

#ifndef DG_OLC_SYNTAX_H
#define DG_OLC_SYNTAX_H

#include <stddef.h>

struct descriptor_data;

/* Public interface. */
char *command_syntax_highlighting(const char *string, int trigger_type);
void dg_olc_script_syntax_highlighting(struct descriptor_data *d, char *string);
int format_script(struct descriptor_data *d);

#define DG_TRIGGER_ANY              (-1)    /**< Accepts commands of any trigger type. */

#define DG_SHOW_COMMANDS_ON_MENU    1       /**< Show the script text on the trigedit main menu. */
#define DG_HIGHLIGHT_ON_MENU        1       /**< Apply semantic highlighting to the script shown on the menu. */
#define DG_LINE_LIMIT_ON_MENU       16      /**< Cap the number of script lines shown on the menu; 0 = no limit. */

/**
 * @brief Text buffer that grows on demand, used to build highlighted/formatted strings.
 *
 * @details Starts zeroed ({ NULL, 0, 0 }); syntax_buffer_reserve() reallocates
 * data in powers of two as needed. Always kept null-terminated by the
 * dg_syntax_buffer_append*() functions.
 */
struct syntax_text_buffer {
    char *data;      /**< Allocated buffer (may be NULL if nothing was written yet). */
    size_t length;   /**< Bytes used in data, not counting the null terminator. */
    size_t capacity; /**< Bytes allocated in data. */
};

/**
 * @brief Classification of a DG script line, used for colouring and validation.
 */
enum dg_line_kind {
    DG_LINE_EMPTY,    /**< Blank line, or whitespace only. */
    DG_LINE_COMMENT,  /**< Comment line; first non-space character is '*'. */
    DG_LINE_CONTROL,  /**< Flow control keyword (if/while/switch/case/...), see control_commands[]. */
    DG_LINE_GENERIC,  /**< Generic script command (eval/set/wait/...), see generic_commands[]. */
    DG_LINE_MOB,      /**< `m*` command recognised through dg_mob_command_exists(). */
    DG_LINE_OBJ,      /**< `o*` command recognised through dg_obj_command_exists(). */
    DG_LINE_WLD,      /**< `w*` command recognised through dg_wld_command_exists(). */
    DG_LINE_PLAYER,   /**< Normal player/social command recognised through dg_player_command_exists(). */
    DG_LINE_DYNAMIC,  /**< Line starting with '%'; the command is resolved at runtime by a variable. */
    DG_LINE_UNKNOWN   /**< No table nor dispatcher recognised the command. */
};

/**
 * @brief Describes how a control/generic command keyword must be validated and highlighted.
 *
 * @details Used by the control_commands[] and generic_commands[] tables,
 * searched by find_command_spec().
 */
struct dg_command_spec {
    const char *name;            /**< Keyword (e.g. "if", "eval"). */
    bool requires_argument;      /**< If true, format_script() reports an error when the argument is empty. */
    bool expression_argument;    /**< If true, the argument is treated as an expression. */
};

/**
 * @brief Result of tokenising a script line, filled in by dg_syntax_parse_line().
 */
struct dg_parsed_line {
    const char *first;          /**< Start of the command within the line, after leading spaces. */
    const char *argument;       /**< Start of the argument, after the command and the spaces that follow it. */
    size_t command_length;      /**< Length of the command token pointed to by first. */
    enum dg_line_kind kind;     /**< Line classification. */
    const struct dg_command_spec *spec;     /**< Matching specification, or NULL. */
};

char *dg_syntax_str_dup(const char *source);
char *dg_syntax_copy_text_range(const char *start, size_t len);
bool dg_syntax_buffer_append_n(struct syntax_text_buffer *buffer, const char *text, size_t len);
bool dg_syntax_buffer_append(struct syntax_text_buffer *buffer, const char *text);
bool dg_syntax_token_equals(const char *token, size_t token_length, const char *name);
bool dg_syntax_parse_line(const char *line, int trigger_type, struct dg_parsed_line *parsed);
bool dg_syntax_copy_command(const struct dg_parsed_line *parsed, char *dest, size_t dest_size);
int dg_syntax_descriptor_trigger_type(const struct descriptor_data *d);

#endif /* DG_OLC_SYNTAX_H */
