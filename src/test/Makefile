# Using µnit is very simple; just include the header and add the C
# file to your sources.  That said, here is a simple Makefile to build
# the example.

CSTD:=99
OPENMP:=n
ASAN:=n
UBSAN:=n
EXTENSION:=
TEST_ENV:=
CFLAGS:=-Wall -Wno-char-subscripts -Wno-unused-but-set-variable -g
CFLAGS+=-Wl,--wrap=send_to_char,--wrap=vwrite_to_output
AGGRESSIVE_WARNINGS=n
LIBS:=-lcrypt 

#ifeq ($(CC),pgcc)
#        CFLAGS+=-c$(CSTD)
#else
#        CFLAGS+=-std=c$(CSTD)
#endif

ifeq ($(OPENMP),y)
        ifeq ($(CC),pgcc)
                CFLAGS+=-mp
        else
                CFLAGS+=-fopenmp
        endif
endif

ifneq ($(SANITIZER),)
        CFLAGS+=-fsanitize=$(SANITIZER)
endif

ifneq ($(CC),pgcc)
        ifeq ($(EXTRA_WARNINGS),y)
                CFLAGS+=-Wall -Wextra -Werror
        endif

        ifeq ($(ASAN),y)
                CFLAGS+=-fsanitize=address
        endif

        ifeq ($(UBSAN),y)
                CFLAGS+=-fsanitize=undefined
        endif
endif

#MUNIT_FILES := ../munit/munit.h ../munit/munit.c
TEST_FILES := $(ls *.c)

# exclude main.c to avoid having multiple entrypoints.
OBJ_FILES_FROM_MUD_CODE != ls ../*.c | grep -v main.c | sed 's/\.c$$/.o/g' | xargs
OBJ_FILES_FROM_MUNIT:= ../munit/munit.o
OBJ_FILES_FROM_TESTS!= ls *.c | sed 's/\.c/.o/g' | xargs

OBJ_FILES := $(OBJ_FILES_FROM_MUD_CODE) $(OBJ_FILES_FROM_MUNIT) $(OBJ_FILES_FROM_TESTS) $(LIBS)

testrunner: $(OBJ_FILES)
	$(CC) $(CFLAGS) -o testrunner $(OBJ_FILES)

test: testrunner
	$(TEST_ENV) ./testrunner

debug: testrunner
	$(TEST_ENV) gdb -q --args ./testrunner --no-fork


$%.o: %.c
	$(CC) $< $(CFLAGS) -c -o $@ 

clean:
	rm -f *.o testrunner depend
all: test

default: all

depend:
	$(CC) -MM *.c > depend

-include depend