AC_DEFUN(AC_CHECK_PROTO,
[
ac_safe=translit($1, './+-', '__p_');

AC_MSG_CHECKING([if $1 is prototyped])
AC_CACHE_VAL(ac_cv_prototype_$ac_safe, [#
  if test $ac_cv_gcc_fnb = yes; then
    OLDCFLAGS=$CFLAGS
    CFLAGS="$CFLAGS -fno-builtin"
  fi
AC_TRY_COMPILE([
#define NO_LIBRARY_PROTOTYPES
#define __COMM_C__
#define __ACT_OTHER_C__
#include "src/sysdep.h"
#ifdef $1
  error - already defined!
#endif
void $1(int a, char b, int c, char d, int e, char f, int g, char h);
],dnl
,
eval "ac_cv_prototype_$ac_safe=no",eval "ac_cv_prototype_$ac_safe=yes")
  if test $ac_cv_gcc_fnb = yes; then
    CFLAGS=$OLDCFLAGS
  fi
])

if eval "test \"`echo '$ac_cv_prototype_'$ac_safe`\" = yes"; then
  AC_MSG_RESULT(yes)
else
  AC_DEFINE(builtin(format, NEED_%s_PROTO, translit($1, 'a-z', 'A-Z')), , Check for a prototype to $1.)
  AC_MSG_RESULT(no)
fi
])

dnl @@@t1="MAKE_PROTO_SAFE($1)"; t2="MAKE_PROTO_NAME($t1)"; literals="$literals $t2"@@@])


AC_DEFUN(AC_UNSAFE_CRYPT, [
  AC_CACHE_CHECK([whether crypt needs over 10 characters], ac_cv_unsafe_crypt, [
    if test ${ac_cv_header_crypt_h-no} = yes; then
      use_crypt_header="#include <crypt.h>"
    fi
    if test ${ac_cv_lib_crypt_crypt-no} = yes; then
      ORIGLIBS=$LIBS
      LIBS="-lcrypt $LIBS"
    fi
    AC_TRY_RUN(
changequote(<<, >>)dnl
<<
#define _XOPEN_SOURCE
#include <string.h>
#include <unistd.h>
$use_crypt_header

int main(void)
{
  char pwd[11], pwd2[11];

  strncpy(pwd, (char *)crypt("FooBar", "BazQux"), 10);
  pwd[10] = '\0';
  strncpy(pwd2, (char *)crypt("xyzzy", "BazQux"), 10);
  pwd2[10] = '\0';
  if (strcmp(pwd, pwd2) == 0)
    exit(0);
  exit(1);
}
>>
changequote([, ])dnl
, ac_cv_unsafe_crypt=yes, ac_cv_unsafe_crypt=no, ac_cv_unsafe_crypt=no)])
if test $ac_cv_unsafe_crypt = yes; then
  AC_DEFINE(HAVE_UNSAFE_CRYPT)
fi
if test ${ac_cv_lib_crypt_crypt-no} = yes; then
  LIBS=$ORIGLIBS
fi
])
