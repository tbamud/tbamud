/**
 * @file dg_olc_syntax_highlight.c
 * @brief Semantic highlighting of DG scripts inside trigedit.
 *
 * @details Colours commands, variables, strings, numbers, operators and
 * comments using the classification produced by the syntax core.
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

#include "conf.h"
#include "sysdep.h"
#include "structs.h"

#include "comm.h"
#include "dg_olc_syntax.h"
#include "dg_scripts.h"
#include "modify.h"
#include "utils.h"


#define DG_COLOR_CONTROL    "\tM"  /**< Flow control keywords. */
#define DG_COLOR_GENERIC    "\tC"  /**< Generic DG commands. */
#define DG_COLOR_MOB        "\tG"  /**< Mob-only commands. */
#define DG_COLOR_OBJ        "\tY"  /**< Object-only commands. */
#define DG_COLOR_WLD        "\tB"  /**< Room-only commands. */
#define DG_COLOR_PLAYER     "\tG"  /**< Normal and social commands. */
#define DG_COLOR_DYNAMIC    "\tm"  /**< Body of a command resolved by a variable. */
#define DG_COLOR_VARIABLE   "\to"  /**< Body of an expression between percent signs. */
#define DG_COLOR_DELIMITER  "\tm"  /**< Percent delimiters of commands and variables. */
#define DG_COLOR_STRING     "\ty"  /**< Strings delimited by double quotes. */
#define DG_COLOR_NUMBER     "\tY"  /**< Numeric values. */
#define DG_COLOR_OPERATOR   "\tC"  /**< Operators, in light cyan. */
#define DG_COLOR_COMMENT    "\tg"  /**< Script comments. */
#define DG_COLOR_ERROR      "\tR"  /**< Unknown or invalid command. */
#define DG_COLOR_RESET      "\tn"  /**< Restores the normal colour. */

/**
 * @brief An operator recognised by the highlighter, with its length precomputed.
 */
struct dg_syntax_operator {
    const char *text;  /**< Operator text. */
    size_t length;     /**< Length of text, without the null terminator. */
};

/**
 * @brief Returns the length of the DG operator starting at text, or zero.
 *
 * @details The table mirrors ops[] from eval_lhs_op_rhs() (dg_scripts.c) plus
 * the parentheses, and is ordered so that two-character operators come before
 * one-character ones - that way `!=` is never read as `!`.
 *
 * @param text Position to test.
 * @return Length of the recognised operator (1 or 2), or 0 if text does not start with a known operator.
 */
static size_t dg_operator_length(const char *text)
{
    static const struct dg_syntax_operator operators[] = {
        { "||", 2 },
        { "&&", 2 },
        { "==", 2 },
        { "!=", 2 },
        { "<=", 2 },
        { ">=", 2 },
        { "/=", 2 },
        { "+",  1 },
        { "-",  1 },
        { "/",  1 },
        { "*",  1 },
        { "!",  1 },
        { "<",  1 },
        { ">",  1 },
        { "(",  1 },
        { ")",  1 },
        { NULL, 0 }
    };

    for (size_t i = 0; operators[i].text; i++) {
        if (!strncmp(text, operators[i].text, operators[i].length)) {
            return operators[i].length;
        }
    }

    return 0;
}

/**
 * @brief Colours variables, strings, numbers and operators inside arguments.
 *
 * @details Does not interpret operators or numbers inside strings. Inside
 * variables it highlights parentheses and operators, plus the numbers found
 * between parentheses, restoring the variable colour after each token.
 * Treats `\` as an escape inside double quotes (copies the next character
 * without reinterpreting it); outside variables, operators are only coloured
 * when expression is true.
 *
 * @param output Output buffer the coloured argument is appended to.
 * @param argument Argument text to colour.
 * @param expression If true, recognises and colours operators (used by if/while/switch/return).
 * @return true if the argument was fully processed; false on allocation failure.
 */
static bool append_highlighted_argument(
    struct syntax_text_buffer *output,
    const char *argument,
    bool expression
) {
    bool in_quote = false, in_variable = false, in_variable_quote = false;
    size_t variable_parenthesis_depth = 0;

    for (const char *p = argument; *p;) {
        size_t operator_length;

        if (in_quote) {
            if (*p == '\\' && p[1]) {
                if (!dg_syntax_buffer_append_n(output, p, 2)) {
                    return false;
                }
                p += 2;
                continue;
            }

            if (!dg_syntax_buffer_append_n(output, p, 1)) {
                return false;
            }

            if (*p++ == '"') {
                in_quote = false;
                if (!dg_syntax_buffer_append(output, DG_COLOR_RESET)) {
                    return false;
                }
            }

            continue;
        }
        if (in_variable) {
            if (in_variable_quote) {
                if (*p == '\\' && p[1]) {
                    if (!dg_syntax_buffer_append_n(output, p, 2)) {
                        return false;
                    }
                    p += 2;
                    continue;
                }

                if (!dg_syntax_buffer_append_n(output, p, 1)) {
                    return false;
                }

                if (*p++ == '"') {
                    in_variable_quote = false;
                    if (!dg_syntax_buffer_append(output, DG_COLOR_VARIABLE)) {
                        return false;
                    }
                }

                continue;
            }

            if (*p == '%') {
                if (!dg_syntax_buffer_append(output, DG_COLOR_DELIMITER)
                    || !dg_syntax_buffer_append_n(output, p, 1)
                    || !dg_syntax_buffer_append(output, DG_COLOR_RESET)) {
                    return false;
                }
                in_variable = false;
                variable_parenthesis_depth = 0;
                p++;
                continue;
            }

            if (variable_parenthesis_depth && *p == '"') {
                if (!dg_syntax_buffer_append(output, DG_COLOR_STRING)
                    || !dg_syntax_buffer_append_n(output, p, 1)) {
                    return false;
                }
                in_variable_quote = true;
                p++;
                continue;
            }

            if (*p == '(' || *p == ')') {
                if (!dg_syntax_buffer_append(output, DG_COLOR_OPERATOR)
                    || !dg_syntax_buffer_append_n(output, p, 1)
                    || !dg_syntax_buffer_append(output, DG_COLOR_VARIABLE)) {
                    return false;
                }
                if (*p == '(') {
                    variable_parenthesis_depth++;
                } else if (variable_parenthesis_depth) {
                    variable_parenthesis_depth--;
                }
                p++;
                continue;
            }

            operator_length = variable_parenthesis_depth ? dg_operator_length(p) : 0;

            if (operator_length) {
                if (!dg_syntax_buffer_append(output, DG_COLOR_OPERATOR)
                    || !dg_syntax_buffer_append_n(output, p, operator_length)
                    || !dg_syntax_buffer_append(output, DG_COLOR_VARIABLE)) {
                    return false;
                }
                p += operator_length;
                continue;
            }

            if (variable_parenthesis_depth && isdigit((unsigned char)*p)) {
                const char *number_end = p + 1;

                while (isdigit((unsigned char)*number_end)) {
                    number_end++;
                }
                if (!dg_syntax_buffer_append(output, DG_COLOR_NUMBER)
                    || !dg_syntax_buffer_append_n(output, p, (size_t)(number_end - p))
                    || !dg_syntax_buffer_append(output, DG_COLOR_VARIABLE)) {
                    return false;
                }
                p = number_end;
                continue;
            }
            if (!dg_syntax_buffer_append_n(output, p, 1)) {
                return false;
            }
            p++;
            continue;
        }

        if (*p == '"') {
            if (!dg_syntax_buffer_append(output, DG_COLOR_STRING)
                || !dg_syntax_buffer_append_n(output, p, 1)) {
                return false;
            }
            in_quote = true;
            p++;
            continue;
        }

        if (*p == '%') {
            if (!dg_syntax_buffer_append(output, DG_COLOR_DELIMITER)
                || !dg_syntax_buffer_append_n(output, p, 1)
                || !dg_syntax_buffer_append(output, DG_COLOR_VARIABLE)) {
                return false;
            }
            in_variable = true;
            p++;
            continue;
        }

        operator_length = expression ? dg_operator_length(p) : 0;

        if (operator_length) {
            if (!dg_syntax_buffer_append(output, DG_COLOR_OPERATOR)
                || !dg_syntax_buffer_append_n(output, p, operator_length)
                || !dg_syntax_buffer_append(output, DG_COLOR_RESET)) {
                return false;
            }
            p += operator_length;
            continue;
        }

        if (isdigit((unsigned char)*p)) {
            const char *number_end = p + 1;

            while (isdigit((unsigned char)*number_end)) {
                number_end++;
            }

            if (!dg_syntax_buffer_append(output, DG_COLOR_NUMBER)
                || !dg_syntax_buffer_append_n(output, p, (size_t)(number_end - p))
                || !dg_syntax_buffer_append(output, DG_COLOR_RESET)) {
                return false;
            }
            p = number_end;
            continue;
        }

        if (!dg_syntax_buffer_append_n(output, p, 1)) {
            return false;
        }

        p++;
    }
    return true;
}

/**
 * @brief Returns the semantic colour tied to a line category.
 *
 * @param kind Line category.
 * @return Matching DG_COLOR_* constant.
 */
static const char *dg_line_color(enum dg_line_kind kind)
{
    switch (kind) {
        case DG_LINE_CONTROL:
            return DG_COLOR_CONTROL;
        case DG_LINE_GENERIC:
            return DG_COLOR_GENERIC;
        case DG_LINE_MOB:
            return DG_COLOR_MOB;
        case DG_LINE_OBJ:
            return DG_COLOR_OBJ;
        case DG_LINE_WLD:
            return DG_COLOR_WLD;
        case DG_LINE_PLAYER:
            return DG_COLOR_PLAYER;
        case DG_LINE_DYNAMIC:
            return DG_COLOR_DYNAMIC;
        case DG_LINE_UNKNOWN:
            return DG_COLOR_ERROR;
        case DG_LINE_EMPTY:
        case DG_LINE_COMMENT:
            break;
    }

    return DG_COLOR_RESET;
}

/**
 * @brief Applies lexical highlighting to a single line.
 *
 * @details Keeps the leading spaces uncoloured. Empty lines return only those
 * spaces (or an empty string). Comment lines, lines with an unknown command
 * and the wait command get a single colour to the end of the line; every
 * other line gets the command colour (dg_line_color()) followed by the
 * argument highlighted by append_highlighted_argument().
 *
 * @param source Original line (without \\r or \\n) to highlight.
 * @param trigger_type MOB_TRIGGER, OBJ_TRIGGER, WLD_TRIGGER or DG_TRIGGER_ANY.
 * @return Allocated buffer with the highlighted line, or NULL on allocation or parsing failure.
 */
static char *highlight_script_line(const char *source, int trigger_type)
{
    struct syntax_text_buffer output = { NULL, 0, 0 };
    struct dg_parsed_line parsed;
    const char *color;

    if (!dg_syntax_parse_line(source, trigger_type, &parsed)) {
        return NULL;
    }

    if (!dg_syntax_buffer_append_n(&output, source, (size_t)(parsed.first - source))) {
        return NULL;
    }

    if (parsed.kind == DG_LINE_EMPTY) {
        return output.data ? output.data : dg_syntax_str_dup("");
    }

    if (parsed.kind == DG_LINE_COMMENT) {
        color = DG_COLOR_COMMENT;
    } else if (parsed.kind == DG_LINE_UNKNOWN) {
        color = DG_COLOR_ERROR;
    } else if (parsed.kind == DG_LINE_GENERIC
        && dg_syntax_token_equals(parsed.first, parsed.command_length, "wait")) {
        color = DG_COLOR_GENERIC;
    } else {
        color = NULL;
    }

    if (color) {
        if (!dg_syntax_buffer_append(&output, color)
            || !dg_syntax_buffer_append(&output, parsed.first)
            || !dg_syntax_buffer_append(&output, DG_COLOR_RESET)) {
            free(output.data);
            return NULL;
        }
        return output.data;
    }

    color = dg_line_color(parsed.kind);

    if (!dg_syntax_buffer_append(&output, color)
        || !dg_syntax_buffer_append_n(&output, parsed.first, parsed.command_length)
        || !dg_syntax_buffer_append(&output, DG_COLOR_RESET)
        || !dg_syntax_buffer_append_n(&output, parsed.first + parsed.command_length,
            (size_t)(parsed.argument - parsed.first - parsed.command_length))
        || !append_highlighted_argument(&output, parsed.argument,
            parsed.spec && parsed.spec->expression_argument)) {
        free(output.data);
        return NULL;
    }

    return output.data;
}

/**
 * @brief Highlights a script using the command set of the given type.
 * @param string Script text.
 * @param trigger_type MOB_TRIGGER, OBJ_TRIGGER, WLD_TRIGGER, or an invalid value to accept them all.
 * @return Allocated buffer that must be freed by the caller, or NULL on error.
 */
char *command_syntax_highlighting(const char *string, int trigger_type)
{
    struct syntax_text_buffer output = { NULL, 0, 0 };
    const char *cursor, *line_end;

    if (!string) {
        return NULL;
    }

    if (trigger_type < MOB_TRIGGER || trigger_type > WLD_TRIGGER) {
        trigger_type = DG_TRIGGER_ANY;
    }

    cursor = string;

    while (*cursor) {
        char *source_line, *highlighted_line;
        size_t line_length;
        line_end = cursor;

        while (*line_end && *line_end != '\r' && *line_end != '\n') {
            line_end++;
        }

        line_length = (size_t)(line_end - cursor);
        source_line = dg_syntax_copy_text_range(cursor, line_length);
        highlighted_line = source_line ? highlight_script_line(source_line, trigger_type) : NULL;

        if (source_line) {
            free(source_line);
        }

        if (!highlighted_line || !dg_syntax_buffer_append(&output, highlighted_line)
            || !dg_syntax_buffer_append(&output, "\tn\r\n")) {
            if (highlighted_line) {
                free(highlighted_line);
            }

            if (output.data) {
                free(output.data);
            }

            return NULL;
        }

        free(highlighted_line);

        if (*line_end == '\r' && line_end[1] == '\n') {
            cursor = line_end + 2;
        } else if (*line_end) {
            cursor = line_end + 1;
        } else {
            cursor = line_end;
        }
    }

    return output.data ? output.data : dg_syntax_str_dup("");
}

/**
 * @brief Shows a highlighted copy of the script in the pager.
 *
 * @details Uses dg_syntax_descriptor_trigger_type(d) to restrict the
 * highlighting to the type of the trigger being edited. Does nothing if d or
 * string are null.
 *
 * @param d Descriptor that will receive the paged text.
 * @param string Script text to highlight.
 */
void dg_olc_script_syntax_highlighting(struct descriptor_data *d, char *string)
{
    if (!d || !string) {
        return;
    }

    char *highlighted = command_syntax_highlighting(string, dg_syntax_descriptor_trigger_type(d));

    if (!highlighted) {
        write_to_output(d, "Could not highlight the script: out of memory.\r\n");
        return;
    }

    page_string(d, highlighted, TRUE);
    free(highlighted);
}
