/**
 * @file dg_olc_syntax_format.c
 * @brief Structural validation and formatting of DG scripts inside trigedit.
 *
 * @details Validates expressions and blocks, lists every problem found, and
 * only replaces the original text when the reindentation is safe.
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
#include "utils.h"

/* Only GCC-compatible compilers understand the printf format attribute; on
 * everything else the call sites simply lose the compile-time check. */
#if defined(__GNUC__)
#define DG_SYNTAX_PRINTF(fmt, args) __attribute__ ((format(printf, fmt, args)))
#else
#define DG_SYNTAX_PRINTF(fmt, args)
#endif

/**
 * @brief Type of control block tracked by the format_script() indentation stack.
 */
enum dg_format_block_type {
    DG_FORMAT_IF,      /**< Block opened by 'if', closed by 'end'. */
    DG_FORMAT_WHILE,   /**< Block opened by 'while', closed by 'done'. */
    DG_FORMAT_SWITCH   /**< Block opened by 'switch', closed by 'done'. */
};

/**
 * @brief One level of the stack of blocks left open while format_script() formats/validates.
 */
struct dg_format_block {
    enum dg_format_block_type type; /**< Block type. */
    size_t line;        /**< Line (1-based) where the block was opened, used in error messages. */
    bool case_active;   /**< True inside the body of a 'case' or 'default'. */
    bool else_seen;     /**< True if this 'if' already had an 'else'; a second one is an error. */
    bool default_seen;  /**< True if this 'switch' already had a 'default'. */
};

/**
 * @brief Computes the current indentation, including active case bodies.
 *
 * @param stack Stack of open blocks.
 * @param depth Number of valid levels in stack.
 * @return Indentation level: depth plus one for each switch with case_active on the stack.
 */
static size_t dg_format_indent(const struct dg_format_block *stack, size_t depth)
{
    size_t indent = depth;

    for (size_t i = 0; i < depth; i++) {
        if (stack[i].type == DG_FORMAT_SWITCH && stack[i].case_active) {
            indent++;
        }
    }

    return indent;
}

/**
 * @brief Returns true if there is an open while or switch that a break can target.
 *
 * @param stack Stack of open blocks.
 * @param depth Number of valid levels in stack.
 * @return true if any level of the stack, innermost to outermost, is DG_FORMAT_WHILE or DG_FORMAT_SWITCH.
 */
static bool dg_format_has_break_target(const struct dg_format_block *stack, size_t depth)
{
    while (depth > 0) {
        depth--;
        if (stack[depth].type == DG_FORMAT_WHILE || stack[depth].type == DG_FORMAT_SWITCH) {
            return true;
        }
    }

    return false;
}

/**
 * @brief Validates quotes, parentheses, variables and boolean operators of an expression.
 *
 * @details Walks the expression of an if/while/switch/return checking that
 * double quotes, variable `%` signs and parentheses are balanced, and that
 * `&`/`|` always appear doubled (`&&`/`||`). Stops at the first problem found.
 *
 * @note A `&`/`|` followed by an alphanumeric character is accepted as a
 * colour code (`&Y`, `&n`) rather than an incomplete operator. The editor's
 * `/t` command converts the whole script between tabbed codes and `&X`, so
 * without this exception any coloured text inside an `eval` would block the
 * formatting of the script.
 *
 * @param expression Expression text to validate.
 * @return First problem found, or NULL if the expression looks well formed.
 */
static const char *validate_expression(const char *expression)
{
    int parentheses = 0;
    bool in_quote = false, in_variable = false;

    for (const char *p = expression; *p; p++) {
        if (*p == '\\' && in_quote && p[1]) {
            p++;
            continue;
        }
        if (*p == '"' && !in_variable) {
            in_quote = !in_quote;
            continue;
        }
        if (*p == '%' && !in_quote) {
            in_variable = !in_variable;
            continue;
        }
        if (in_quote || in_variable) {
            continue;
        }
        if (*p == '(') {
            parentheses++;
        } else if (*p == ')' && --parentheses < 0) {
            return "unmatched closing parenthesis";
        } else if ((*p == '&' || *p == '|') && p[1] == *p) {
            p++;
        } else if ((*p == '&' || *p == '|') && !isalnum((unsigned char)p[1])) {
            return "incomplete boolean operator; use && or ||";
        }
    }

    if (in_quote) {
        return "unterminated double quote";
    }

    if (in_variable) {
        return "variable missing its closing '%'";
    }

    if (parentheses > 0) {
        return "unmatched opening parenthesis";
    }

    return NULL;
}

/**
 * @brief Appends one problem to the script validation report.
 * @param d Descriptor that receives the report.
 * @param error_count Total problem counter; incremented by this function.
 * @param line Script line the problem relates to.
 * @param format Formatted description of the problem.
 *
 * @warning The printf format attribute is mandatory: validate_expression()
 * returns messages that contain `%` and must be passed as an argument to
 * "%s", never as the format itself.
 */
static void DG_SYNTAX_PRINTF(4, 5) report_syntax_error(
    struct descriptor_data *d,
    size_t *error_count,
    size_t line,
    const char *format,
    ...
) {
    char message[512];
    va_list args;

    if (!d || !error_count || !format) {
        return;
    }

    if (*error_count == 0) {
        write_to_output(d, "\tOProblems found in the script:\r\n\r\n");
    }

    va_start(args, format);
    vsnprintf(message, sizeof(message), format, args);
    va_end(args);
    write_to_output(d, "  \trLine %-3lu\tn: %s.\r\n", (unsigned long)line, message);
    (*error_count)++;
}

/**
 * @brief Reindents and validates the structure and commands of the script being edited.
 *
 * @details Lists every problem found in a single pass and keeps the original
 * text whenever there is any error. Command validation honours the
 * MOB/OBJ/WLD type of the trigger.
 *
 * @param d Descriptor whose *d->str holds the script to reindent/validate.
 * @return TRUE if the script was reindented successfully and *d->str was
 * replaced; FALSE if there was any syntax/structure error (nothing is
 * changed) or an allocation failure.
 */
int format_script(struct descriptor_data *d)
{
    struct syntax_text_buffer formatted = { NULL, 0, 0 };
    struct dg_format_block *stack = NULL;
    const char *cursor, *line_end;
    size_t depth = 0, error_count = 0, line_number = 0, max_depth, output_limit;
    bool output_overflow = false;
    int trigger_type;

    if (!d || !d->str || !*d->str) {
        return FALSE;
    }

    /* Every open block consumes at least one line, so the number of line
     * separators plus one is a safe ceiling for the depth - and orders of
     * magnitude smaller than the size of the script in bytes. */
    max_depth = 1;

    for (cursor = *d->str; *cursor; cursor++) {
        if (*cursor == '\r' || *cursor == '\n') {
            max_depth++;
        }
    }

    stack = (struct dg_format_block *)calloc(max_depth, sizeof(*stack));

    if (!stack) {
        write_to_output(d, "Out of memory while formatting the script.\r\n");
        return FALSE;
    }

    /* string_add() reserves 3 bytes for "\r\n\0"; without that slack the
     * editor would refuse the next line typed after the formatting. */
    output_limit = d->max_str < (size_t)MAX_CMD_LENGTH ? d->max_str : (size_t)MAX_CMD_LENGTH;

    if (output_limit <= 3) {
        write_to_output(d, "The editor has no room for the formatted script.\r\n");
        free(stack);
        return FALSE;
    }

    trigger_type = dg_syntax_descriptor_trigger_type(d);
    cursor = *d->str;

    while (*cursor) {
        struct dg_parsed_line parsed;
        char *raw_line;
        const char *line;
        size_t line_length, indent;
        bool activate_case = false;

        line_number++;
        line_end = cursor;

        while (*line_end && *line_end != '\r' && *line_end != '\n') {
            line_end++;
        }
        line_length = (size_t)(line_end - cursor);
        raw_line = dg_syntax_copy_text_range(cursor, line_length);

        if (!raw_line) {
            goto allocation_failure;
        }

        line = raw_line;

        while (isspace((unsigned char)*line)) {
            line++;
        }

        if (!dg_syntax_parse_line(line, trigger_type, &parsed)) {
            free(raw_line);
            goto allocation_failure;
        }

        if (parsed.kind == DG_LINE_UNKNOWN) {
            char command[64];

            if (!dg_syntax_copy_command(&parsed, command, sizeof(command))) {
                memcpy(command + sizeof(command) - 4, "...", 4);
            }

            report_syntax_error(d, &error_count, line_number,
                "unknown command '%s', or invalid for this trigger type", command);
        }

        if (parsed.spec && parsed.spec->requires_argument && !*parsed.argument) {
            report_syntax_error(d, &error_count, line_number,
                "command '%s' is missing its argument", parsed.spec->name);
        } else if (parsed.spec && parsed.spec->expression_argument) {
            const char *expression_error = validate_expression(parsed.argument);

            if (expression_error) {
                report_syntax_error(d, &error_count, line_number, "%s", expression_error);
            }
        }

        indent = dg_format_indent(stack, depth);
        if (parsed.kind == DG_LINE_CONTROL && dg_syntax_token_equals(parsed.first, parsed.command_length, "elseif")) {
            if (!depth || stack[depth - 1].type != DG_FORMAT_IF) {
                report_syntax_error(d, &error_count, line_number, "'elseif' without a matching 'if'");
            } else {
                if (stack[depth - 1].else_seen) {
                    report_syntax_error(d, &error_count, line_number, "'elseif' after 'else'");
                }
                indent--;
            }
        } else if (parsed.kind == DG_LINE_CONTROL
            && dg_syntax_token_equals(parsed.first, parsed.command_length, "else")) {
            if (!depth || stack[depth - 1].type != DG_FORMAT_IF) {
                report_syntax_error(d, &error_count, line_number, "'else' without a matching 'if'");
            } else {
                if (stack[depth - 1].else_seen) {
                    report_syntax_error(d, &error_count, line_number, "more than one 'else' in the same 'if'");
                } else {
                    stack[depth - 1].else_seen = true;
                }
                indent--;
            }
        } else if (parsed.kind == DG_LINE_CONTROL
            && dg_syntax_token_equals(parsed.first, parsed.command_length, "end")) {
            if (!depth || stack[depth - 1].type != DG_FORMAT_IF) {
                report_syntax_error(d, &error_count, line_number, "'end' without a matching 'if'");
            } else {
                depth--;
                indent = dg_format_indent(stack, depth);
            }
        } else if (parsed.kind == DG_LINE_CONTROL
            && dg_syntax_token_equals(parsed.first, parsed.command_length, "done")) {
            if (!depth || (stack[depth - 1].type != DG_FORMAT_WHILE
                && stack[depth - 1].type != DG_FORMAT_SWITCH)) {
                report_syntax_error(d, &error_count, line_number,
                    "'done' without a matching 'while' or 'switch'");
            } else {
                depth--;
                indent = dg_format_indent(stack, depth);
            }
        } else if (parsed.kind == DG_LINE_CONTROL
            && (dg_syntax_token_equals(parsed.first, parsed.command_length, "case")
                || dg_syntax_token_equals(parsed.first, parsed.command_length, "default"))) {
            bool is_default = dg_syntax_token_equals(parsed.first, parsed.command_length, "default");

            if (!depth || stack[depth - 1].type != DG_FORMAT_SWITCH) {
                report_syntax_error(d, &error_count, line_number, "'case/default' outside a 'switch'");
            } else if (stack[depth - 1].default_seen) {
                report_syntax_error(d, &error_count, line_number,
                    is_default ? "more than one 'default' in the same 'switch'" : "'case' after 'default'");
            } else {
                stack[depth - 1].case_active = false;
                stack[depth - 1].default_seen = is_default;
                indent = dg_format_indent(stack, depth);
                activate_case = true;
            }
        } else if (parsed.kind == DG_LINE_CONTROL
            && dg_syntax_token_equals(parsed.first, parsed.command_length, "break")) {
            if (!dg_format_has_break_target(stack, depth)) {
                report_syntax_error(d, &error_count, line_number, "'break' outside a 'while' or 'switch'");
            } else if (depth && stack[depth - 1].type == DG_FORMAT_SWITCH
                && stack[depth - 1].case_active) {
                /* Line the 'break' up with the 'case', but keep the body
                 * active: whatever comes next stays at the case indentation. */
                indent--;
            }
        }

        if (!output_overflow) {
            if (*line) {
                for (size_t i = 0; i < indent; i++) {
                    if (!dg_syntax_buffer_append(&formatted, "  ")) {
                        free(raw_line);
                        goto allocation_failure;
                    }
                }
                if (!dg_syntax_buffer_append(&formatted, line)) {
                    free(raw_line);
                    goto allocation_failure;
                }
            }
            if (!dg_syntax_buffer_append(&formatted, "\r\n")) {
                free(raw_line);
                goto allocation_failure;
            }
            if (formatted.length + 3 > output_limit) {
                report_syntax_error(d, &error_count, line_number,
                    "the formatted text would exceed the allowed limit");
                output_overflow = true;
            }
        }

        if (activate_case) {
            stack[depth - 1].case_active = true;
        } else if (parsed.kind == DG_LINE_CONTROL
            && dg_syntax_token_equals(parsed.first, parsed.command_length, "if")) {
            stack[depth++] = (struct dg_format_block){ DG_FORMAT_IF, line_number, false, false, false };
        } else if (parsed.kind == DG_LINE_CONTROL
            && dg_syntax_token_equals(parsed.first, parsed.command_length, "while")) {
            stack[depth++] = (struct dg_format_block){ DG_FORMAT_WHILE, line_number, false, false, false };
        } else if (parsed.kind == DG_LINE_CONTROL
            && dg_syntax_token_equals(parsed.first, parsed.command_length, "switch")) {
            stack[depth++] = (struct dg_format_block){ DG_FORMAT_SWITCH, line_number, false, false, false };
        }
        free(raw_line);

        if (*line_end == '\r' && line_end[1] == '\n') {
            cursor = line_end + 2;
        } else if (*line_end) {
            cursor = line_end + 1;
        } else {
            cursor = line_end;
        }
    }

    for (size_t i = 0; i < depth; i++) {
        const char *block_name = stack[i].type == DG_FORMAT_IF ? "if"
            : stack[i].type == DG_FORMAT_WHILE ? "while" : "switch";

        report_syntax_error(d, &error_count, stack[i].line,
            "block '%s' was never closed", block_name);
    }

    if (error_count) {
        write_to_output(d, "\r\n\tOTotal: \tY%lu\tO problem%s; the script was left unchanged.\tn\r\n",
            (unsigned long)error_count, error_count == 1 ? "" : "s");
        goto fail;
    }

    free(stack);
    free(*d->str);
    *d->str = formatted.data ? formatted.data : dg_syntax_str_dup("");
    return *d->str ? TRUE : FALSE;

allocation_failure:
    write_to_output(d, "Out of memory while formatting the script.\r\n");
fail:
    free(stack);

    if (formatted.data) {
        free(formatted.data);
    }

    return FALSE;
}
