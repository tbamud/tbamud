/**
 * @file test_interpreter.c
 * Unit tests for pure string-handling functions in src/interpreter.c:
 *   is_number, is_abbrev, delete_doubledollar, any_one_arg, one_word
 * and for the alias expansion path perform_complex_alias() (via perform_alias()).
 */

#include "unity.h"

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"

extern FILE *logfile;

void setUp(void)    { logfile = stderr; }
void tearDown(void) { logfile = NULL; }

/* =========================================================
 * write_to_q — real implementation for alias tests.
 * Overrides the __attribute__((weak)) stub in test_stubs.c so that
 * the queue is actually populated and we can inspect its contents.
 * ========================================================= */

void write_to_q(const char *txt, struct txt_q *queue, int aliased)
{
    struct txt_block *newt;
    CREATE(newt, struct txt_block, 1);
    newt->text    = strdup(txt);
    newt->aliased = aliased;
    if (!queue->head) {
        newt->next  = NULL;
        queue->head = queue->tail = newt;
    } else {
        queue->tail->next = newt;
        queue->tail       = newt;
        newt->next        = NULL;
    }
}

/* =========================================================
 * is_number
 * ========================================================= */

void test_is_number_digits_only(void)
{
    TEST_ASSERT_TRUE(is_number("42"));
}

void test_is_number_zero(void)
{
    TEST_ASSERT_TRUE(is_number("0"));
}

void test_is_number_negative(void)
{
    TEST_ASSERT_TRUE(is_number("-5"));
}

void test_is_number_empty_string(void)
{
    TEST_ASSERT_FALSE(is_number(""));
}

void test_is_number_contains_letter(void)
{
    TEST_ASSERT_FALSE(is_number("12x3"));
}

void test_is_number_minus_only(void)
{
    TEST_ASSERT_FALSE(is_number("-"));
}

void test_is_number_float(void)
{
    TEST_ASSERT_FALSE(is_number("3.14"));
}

/* =========================================================
 * is_abbrev
 * ========================================================= */

void test_is_abbrev_exact_match(void)
{
    TEST_ASSERT_TRUE(is_abbrev("north", "north"));
}

void test_is_abbrev_valid_prefix(void)
{
    TEST_ASSERT_TRUE(is_abbrev("nort", "north"));
    TEST_ASSERT_TRUE(is_abbrev("n", "north"));
}

void test_is_abbrev_non_prefix(void)
{
    TEST_ASSERT_FALSE(is_abbrev("south", "north"));
}

void test_is_abbrev_empty_arg1(void)
{
    TEST_ASSERT_FALSE(is_abbrev("", "north"));
}

void test_is_abbrev_arg1_longer_than_arg2(void)
{
    TEST_ASSERT_FALSE(is_abbrev("northward", "north"));
}

void test_is_abbrev_case_insensitive(void)
{
    TEST_ASSERT_TRUE(is_abbrev("Nor", "north"));
    TEST_ASSERT_TRUE(is_abbrev("NOR", "NORTH"));
}

/* =========================================================
 * delete_doubledollar
 * ========================================================= */

void test_delete_doubledollar_no_dollars(void)
{
    char s[] = "hello";
    delete_doubledollar(s);
    TEST_ASSERT_EQUAL_STRING("hello", s);
}

void test_delete_doubledollar_double_at_start(void)
{
    char s[] = "$$hello";
    delete_doubledollar(s);
    TEST_ASSERT_EQUAL_STRING("$hello", s);
}

void test_delete_doubledollar_double_in_middle(void)
{
    char s[] = "hello$$world";
    delete_doubledollar(s);
    TEST_ASSERT_EQUAL_STRING("hello$world", s);
}

void test_delete_doubledollar_four_dollars(void)
{
    char s[] = "$$$$";
    delete_doubledollar(s);
    TEST_ASSERT_EQUAL_STRING("$$", s);
}

void test_delete_doubledollar_single_dollar_unchanged(void)
{
    char s[] = "hello$world";
    delete_doubledollar(s);
    TEST_ASSERT_EQUAL_STRING("hello$world", s);
}

/* =========================================================
 * any_one_arg
 * ========================================================= */

void test_any_one_arg_basic(void)
{
    char first[64];
    char *rest = any_one_arg("hello world", first);
    TEST_ASSERT_EQUAL_STRING("hello", first);
    TEST_ASSERT_EQUAL_STRING(" world", rest);
}

void test_any_one_arg_leading_spaces(void)
{
    char first[64];
    any_one_arg("  hello world", first);
    TEST_ASSERT_EQUAL_STRING("hello", first);
}

void test_any_one_arg_single_word(void)
{
    char first[64];
    char *rest = any_one_arg("hello", first);
    TEST_ASSERT_EQUAL_STRING("hello", first);
    TEST_ASSERT_EQUAL_STRING("", rest);
}

void test_any_one_arg_empty_string(void)
{
    char first[64];
    any_one_arg("", first);
    TEST_ASSERT_EQUAL_STRING("", first);
}

void test_any_one_arg_lowercases(void)
{
    char first[64];
    any_one_arg("HELLO", first);
    TEST_ASSERT_EQUAL_STRING("hello", first);
}

/* =========================================================
 * one_word
 * ========================================================= */

void test_one_word_unquoted_like_any_one_arg(void)
{
    char first[64];
    char *rest = one_word("hello world", first);
    TEST_ASSERT_EQUAL_STRING("hello", first);
    TEST_ASSERT_EQUAL_STRING(" world", rest);
}

void test_one_word_quoted_string(void)
{
    char first[64];
    char *rest = one_word("\"hello world\" rest", first);
    TEST_ASSERT_EQUAL_STRING("hello world", first);
    /* rest points just past the closing quote */
    TEST_ASSERT_EQUAL_STRING(" rest", rest);
}

void test_one_word_empty_quoted(void)
{
    char first[64];
    one_word("\"\" rest", first);
    TEST_ASSERT_EQUAL_STRING("", first);
}

void test_one_word_leading_spaces_skipped(void)
{
    char first[64];
    one_word("  hello", first);
    TEST_ASSERT_EQUAL_STRING("hello", first);
}

/* =========================================================
 * Helpers for perform_complex_alias tests
 * ========================================================= */

/* Release every txt_block in a queue and reset head/tail to NULL. */
static void free_txt_q(struct txt_q *q)
{
    struct txt_block *b = q->head, *n;
    while (b) { n = b->next; free(b->text); free(b); b = n; }
    q->head = q->tail = NULL;
}

/* Build a heap-allocated alias_data with type ALIAS_COMPLEX. */
static struct alias_data *make_test_alias(const char *name, const char *repl)
{
    struct alias_data *a;
    CREATE(a, struct alias_data, 1);
    a->alias       = strdup(name);
    a->replacement = strdup(repl);
    a->type        = ALIAS_COMPLEX;
    a->next        = NULL;
    return a;
}

/* Free a single alias_data allocated by make_test_alias(). */
static void destroy_test_alias(struct alias_data *a)
{
    free(a->alias);
    free(a->replacement);
    free(a);
}

/* Zero-initialise all three objects and wire them together so that
 * IS_NPC() returns false and GET_ALIASES() returns a. */
static void alias_env_init(struct descriptor_data     *d,
                           struct char_data            *ch,
                           struct player_special_data  *psd,
                           struct alias_data           *a)
{
    memset(d,   0, sizeof(*d));
    memset(ch,  0, sizeof(*ch));
    memset(psd, 0, sizeof(*psd));
    ch->player_specials = psd;
    psd->aliases        = a;
    d->character        = ch;
}

/* =========================================================
 * perform_complex_alias — tested via the public perform_alias()
 * ========================================================= */

/* Literal replacement with no variable tokens. */
void test_pca_literal(void)
{
    struct alias_data         *a = make_test_alias("lit", "hello world");
    struct descriptor_data     d;
    struct char_data           ch;
    struct player_special_data psd;
    char orig[] = "lit";

    alias_env_init(&d, &ch, &psd, a);
    perform_alias(&d, orig, sizeof(orig));

    TEST_ASSERT_NOT_NULL(d.input.head);
    TEST_ASSERT_EQUAL_STRING("hello world", d.input.head->text);
    TEST_ASSERT_NULL(d.input.head->next);

    free_txt_q(&d.input);
    destroy_test_alias(a);
}

/* $* glob expands to the full argument string after the alias trigger. */
void test_pca_glob_expansion(void)
{
    struct alias_data         *a = make_test_alias("say", "say $*");
    struct descriptor_data     d;
    struct char_data           ch;
    struct player_special_data psd;
    char orig[] = "say hello there";

    alias_env_init(&d, &ch, &psd, a);
    perform_alias(&d, orig, sizeof(orig));

    TEST_ASSERT_NOT_NULL(d.input.head);
    TEST_ASSERT_EQUAL_STRING("say hello there", d.input.head->text);

    free_txt_q(&d.input);
    destroy_test_alias(a);
}

/* $1 expands to the first whitespace-delimited token of the arguments. */
void test_pca_token1_expansion(void)
{
    struct alias_data         *a = make_test_alias("s1", "say $1");
    struct descriptor_data     d;
    struct char_data           ch;
    struct player_special_data psd;
    char orig[] = "s1 hello world";

    alias_env_init(&d, &ch, &psd, a);
    perform_alias(&d, orig, sizeof(orig));

    TEST_ASSERT_NOT_NULL(d.input.head);
    TEST_ASSERT_EQUAL_STRING("say hello", d.input.head->text);

    free_txt_q(&d.input);
    destroy_test_alias(a);
}

/* Semicolon separator (;) produces two independent queue entries. */
void test_pca_separator(void)
{
    struct alias_data         *a = make_test_alias("seq", "go north;go east");
    struct descriptor_data     d;
    struct char_data           ch;
    struct player_special_data psd;
    char orig[] = "seq";

    alias_env_init(&d, &ch, &psd, a);
    perform_alias(&d, orig, sizeof(orig));

    TEST_ASSERT_NOT_NULL(d.input.head);
    TEST_ASSERT_EQUAL_STRING("go north", d.input.head->text);
    TEST_ASSERT_NOT_NULL(d.input.head->next);
    TEST_ASSERT_EQUAL_STRING("go east", d.input.head->next->text);

    free_txt_q(&d.input);
    destroy_test_alias(a);
}

/* $$ in the replacement is preserved as $$ in the output (act-safety doubling). */
void test_pca_dollar_dollar(void)
{
    struct alias_data         *a = make_test_alias("dol", "cost $$5");
    struct descriptor_data     d;
    struct char_data           ch;
    struct player_special_data psd;
    char orig[] = "dol";

    alias_env_init(&d, &ch, &psd, a);
    perform_alias(&d, orig, sizeof(orig));

    TEST_ASSERT_NOT_NULL(d.input.head);
    TEST_ASSERT_EQUAL_STRING("cost $$5", d.input.head->text);

    free_txt_q(&d.input);
    destroy_test_alias(a);
}

/* Overflow via $*: 255 "$*" tokens × 50-char argument exceeds
 * MAX_RAW_INPUT_LENGTH.  The queue must be empty after the call. */
void test_pca_overflow_glob(void)
{
    /* 255 × "$*" = 510 bytes + NUL */
    char repl[511];
    char orig[64];
    struct alias_data         *a;
    struct descriptor_data     d;
    struct char_data           ch;
    struct player_special_data psd;
    int i;

    for (i = 0; i < 255; i++) { repl[i * 2] = '$'; repl[i * 2 + 1] = '*'; }
    repl[510] = '\0';

    /* "al " + 50 'x' chars */
    memcpy(orig, "al ", 3);
    memset(orig + 3, 'x', 50);
    orig[53] = '\0';

    a = make_test_alias("al", repl);
    alias_env_init(&d, &ch, &psd, a);
    perform_alias(&d, orig, sizeof(orig));

    /* No partial results must leak into the queue on overflow. */
    TEST_ASSERT_NULL(d.input.head);

    free_txt_q(&d.input);
    destroy_test_alias(a);
}

/* Overflow via $1: 255 "$1" tokens × 50-char first token exceeds
 * MAX_RAW_INPUT_LENGTH.  The queue must be empty after the call. */
void test_pca_overflow_token(void)
{
    /* 255 × "$1" = 510 bytes + NUL */
    char repl[511];
    char orig[64];
    struct alias_data         *a;
    struct descriptor_data     d;
    struct char_data           ch;
    struct player_special_data psd;
    int i;

    for (i = 0; i < 255; i++) { repl[i * 2] = '$'; repl[i * 2 + 1] = '1'; }
    repl[510] = '\0';

    /* "al " + 50 'y' chars */
    memcpy(orig, "al ", 3);
    memset(orig + 3, 'y', 50);
    orig[53] = '\0';

    a = make_test_alias("al", repl);
    alias_env_init(&d, &ch, &psd, a);
    perform_alias(&d, orig, sizeof(orig));

    /* No partial results must leak into the queue on overflow. */
    TEST_ASSERT_NULL(d.input.head);

    free_txt_q(&d.input);
    destroy_test_alias(a);
}

/* =========================================================
 * main
 * ========================================================= */

int main(void)
{
    UNITY_BEGIN();

    /* is_number */
    RUN_TEST(test_is_number_digits_only);
    RUN_TEST(test_is_number_zero);
    RUN_TEST(test_is_number_negative);
    RUN_TEST(test_is_number_empty_string);
    RUN_TEST(test_is_number_contains_letter);
    RUN_TEST(test_is_number_minus_only);
    RUN_TEST(test_is_number_float);

    /* is_abbrev */
    RUN_TEST(test_is_abbrev_exact_match);
    RUN_TEST(test_is_abbrev_valid_prefix);
    RUN_TEST(test_is_abbrev_non_prefix);
    RUN_TEST(test_is_abbrev_empty_arg1);
    RUN_TEST(test_is_abbrev_arg1_longer_than_arg2);
    RUN_TEST(test_is_abbrev_case_insensitive);

    /* delete_doubledollar */
    RUN_TEST(test_delete_doubledollar_no_dollars);
    RUN_TEST(test_delete_doubledollar_double_at_start);
    RUN_TEST(test_delete_doubledollar_double_in_middle);
    RUN_TEST(test_delete_doubledollar_four_dollars);
    RUN_TEST(test_delete_doubledollar_single_dollar_unchanged);

    /* any_one_arg */
    RUN_TEST(test_any_one_arg_basic);
    RUN_TEST(test_any_one_arg_leading_spaces);
    RUN_TEST(test_any_one_arg_single_word);
    RUN_TEST(test_any_one_arg_empty_string);
    RUN_TEST(test_any_one_arg_lowercases);

    /* one_word */
    RUN_TEST(test_one_word_unquoted_like_any_one_arg);
    RUN_TEST(test_one_word_quoted_string);
    RUN_TEST(test_one_word_empty_quoted);
    RUN_TEST(test_one_word_leading_spaces_skipped);

    /* perform_complex_alias */
    RUN_TEST(test_pca_literal);
    RUN_TEST(test_pca_glob_expansion);
    RUN_TEST(test_pca_token1_expansion);
    RUN_TEST(test_pca_separator);
    RUN_TEST(test_pca_dollar_dollar);
    RUN_TEST(test_pca_overflow_glob);
    RUN_TEST(test_pca_overflow_token);

    return UNITY_END();
}
