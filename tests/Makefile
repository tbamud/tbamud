# Generated automatically from Makefile.in by configure.
# tests/Makefile.in
# Autoconf template — processed by configure to produce tests/Makefile.
#
# Build and run the tbaMUD unit-test suite.
# Usage (after running ./configure from the project root):
#
#   cd tests && make        # build all test binaries
#   cd tests && make test   # build and run all tests

CC        = gcc
MYFLAGS   = -Wall -Wno-char-subscripts -Wno-unused-but-set-variable
CFLAGS    = -g -O2 $(MYFLAGS)
LIBS      =  -lcrypt 

SRCDIR    = ../src
UNITYDIR  = vendor/unity

# Include paths:
#   ../src   — mud headers and the generated conf.h
#   vendor/unity — Unity framework headers
INCFLAGS  = -I$(SRCDIR) -I$(UNITYDIR)

# Suppress warnings that fire in generated stubs / vendored code
WARNFLAGS = -Wno-unused-parameter -Wno-unused-function -Wno-unused-variable

COMPILE   = $(CC) $(CFLAGS) $(WARNFLAGS) $(INCFLAGS)

# Common object files compiled into every test binary
UNITY_SRC  = $(UNITYDIR)/unity.c
STUBS_SRC  = test_stubs.c

# tbaMUD source files used by the tests
UTILS_SRC  = $(SRCDIR)/utils.c $(SRCDIR)/random.c

# All test binaries
TESTS = test_utils test_random test_interpreter test_class

.PHONY: all test clean

all: $(TESTS)

# ---------------------------------------------------------------------------
# test_utils — covers src/utils.c
# ---------------------------------------------------------------------------
test_utils: $(UNITY_SRC) $(STUBS_SRC) $(UTILS_SRC) test_utils.c
	$(COMPILE) -o $@ $^ $(LIBS)

# ---------------------------------------------------------------------------
# test_random — covers src/random.c and rand_number/dice in src/utils.c
# ---------------------------------------------------------------------------
test_random: $(UNITY_SRC) $(STUBS_SRC) $(UTILS_SRC) test_random.c
	$(COMPILE) -o $@ $^ $(LIBS)

# ---------------------------------------------------------------------------
# test_interpreter — covers string helpers in src/interpreter.c
# ---------------------------------------------------------------------------
test_interpreter: $(UNITY_SRC) $(STUBS_SRC) $(UTILS_SRC) \
                  $(SRCDIR)/interpreter.c test_interpreter.c
	$(COMPILE) -o $@ $^ $(LIBS)

# ---------------------------------------------------------------------------
# test_class — covers src/class.c
# ---------------------------------------------------------------------------
test_class: $(UNITY_SRC) $(STUBS_SRC) $(UTILS_SRC) \
            $(SRCDIR)/class.c test_class.c
	$(COMPILE) -o $@ $^ $(LIBS)

# ---------------------------------------------------------------------------
# Run all tests
# ---------------------------------------------------------------------------
test: $(TESTS)
	@echo "=========================================="
	@echo "Running tbaMUD unit tests"
	@echo "=========================================="
	@./test_utils   && echo "[PASS] test_utils"   || echo "[FAIL] test_utils"
	@./test_random  && echo "[PASS] test_random"  || echo "[FAIL] test_random"
	@./test_interpreter && echo "[PASS] test_interpreter" || echo "[FAIL] test_interpreter"
	@./test_class   && echo "[PASS] test_class"   || echo "[FAIL] test_class"

clean:
	rm -f $(TESTS)
