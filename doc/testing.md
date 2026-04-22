# tbaMUD Unit Testing

_Updated 2026-04_

## Overview

tbaMUD has a C unit-test suite built on the
[Unity](https://github.com/ThrowTheSwitch/Unity) framework.  Tests live in the
`tests/` directory alongside the vendored Unity source.

```
tests/
  Makefile.in          – Autoconf template; processed by configure
  test_stubs.c         – Weak-symbol stubs that satisfy mud headers
  unity_to_junit.py    – Converts Unity output to JUnit XML
  test_class.c         – Tests for src/class.c
  test_interpreter.c   – Tests for src/interpreter.c
  test_random.c        – Tests for src/random.c
  test_utils.c         – Tests for src/utils.c
  vendor/unity/        – Vendored Unity test framework
```

## Prerequisites

| Requirement | Notes |
|---|---|
| C compiler (gcc or clang) | Same compiler used to build the mud |
| GNU make | Any POSIX-compatible make works |
| Python 3 | Required only for JUnit XML conversion (`unity_to_junit.py`) |
| autoconf / configure | Already needed to build the mud |

## Running the tests

Run `./configure` from the repository root first (only needed once):

```sh
./configure
```

Then build and run all tests from the `tests/` directory:

```sh
cd tests
make test
```

`make test` performs the following steps for each test binary:

1. Compiles the test binary (if not already up to date).
2. Runs the binary and captures stdout/stderr to `test-results/<name>.out`.
3. Measures wall-clock elapsed time.
4. Converts the Unity output to JUnit XML via `unity_to_junit.py`, writing
   `test-results/<name>.xml`.
5. Prints `[PASS] <name>` or `[FAIL] <name>` and exits non-zero if any
   binary failed.

To build the test binaries without running them:

```sh
cd tests
make
```

To remove all test binaries and result files:

```sh
cd tests
make clean
```

## Test suites

| Binary | Source under test | Test file |
|---|---|---|
| `test_utils` | `src/utils.c`, `src/random.c` | `test_utils.c` |
| `test_random` | `src/random.c`, `rand_number`/`dice` in `src/utils.c` | `test_random.c` |
| `test_interpreter` | `src/interpreter.c` | `test_interpreter.c` |
| `test_class` | `src/class.c` | `test_class.c` |

## Writing a new test

### Adding a test case to an existing suite

1. Open the relevant `test_<name>.c` file.
2. Write a function with the signature `void test_my_feature(void)`.
3. Use Unity assertion macros such as `TEST_ASSERT_EQUAL_INT`,
   `TEST_ASSERT_NULL`, `TEST_ASSERT_TRUE`, etc.
4. Register the function in the `main()` block:
   ```c
   RUN_TEST(test_my_feature);
   ```

Example:

```c
void test_str_cmp_equal_strings(void)
{
    TEST_ASSERT_EQUAL_INT(0, str_cmp("hello", "hello"));
}
```

### Creating a new test suite

1. Create `tests/test_<module>.c`.  Copy the boilerplate from an existing
   suite: include `unity.h`, define `setUp`/`tearDown` (may be empty), write
   test functions, and provide a `main()` that calls `UNITY_BEGIN()`,
   `RUN_TEST(...)` for each function, and `return UNITY_END();`.

2. Add the binary to `tests/Makefile.in`:
   - Add the name to the `TESTS` variable.
   - Add a build rule:
     ```make
     test_<module>: $(UNITY_SRC) $(STUBS_SRC) $(UTILS_SRC) \
                    $(SRCDIR)/<module>.c test_<module>.c
         $(COMPILE) -o $@ $^ $(LIBS)
     ```

3. Re-run `./configure` from the repository root to regenerate
   `tests/Makefile` from the updated `tests/Makefile.in`.

### Stubs

Many mud source files reference global variables and functions that are only
meaningful at runtime (e.g. `descriptor_list`, `log()`).  `test_stubs.c`
provides zero-initialised definitions and `__attribute__((weak))` stub
implementations for these symbols so that test binaries link without pulling
in the full mud.

If a new test requires a function not yet stubbed, add a weak stub to
`test_stubs.c`:

```c
__attribute__((weak)) void my_function(void) { /* no-op */ }
```

## JUnit XML output and CI

`unity_to_junit.py` reads Unity's line-oriented output on stdin and writes a
JUnit-compatible XML file.  It accepts an optional third argument with the
elapsed wall-clock time in seconds (provided by the `make test` target):

```
usage: unity_to_junit.py <suite_name> <output.xml> [elapsed_seconds]
```

The GitHub Actions workflow (`.github/workflows/build.yml`) runs `make test`
on every push and pull request against `master`.  After the tests finish the
`dorny/test-reporter` action reads `tests/test-results/*.xml` and publishes a
formatted report as a GitHub Check with pass/fail counts and per-suite
execution times.
