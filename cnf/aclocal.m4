dnl Local macros for cnf/configure.ac.  autoconf and autoheader pick this up
dnl from the -I cnf on their command lines; see the header of configure.ac.

dnl AC_CHECK_PROTO(FUNCTION)
dnl
dnl Declare FUNCTION with a deliberately wrong signature after sysdep.h has
dnl been read.  If the system already prototyped it the two declarations
dnl collide and the compile fails, which is the answer we want: no
dnl NEED_<FUNCTION>_PROTO.  If the compile succeeds nothing had declared it,
dnl so sysdep.h has to.
dnl
dnl The probe names sysdep.h by a path relative to the top of the tree, so the
dnl compiler has to be told where that is.  Without -I$srcdir a build from a
dnl separate directory cannot find the header at all, every probe fails to
dnl compile, and a failed compile here is read as "already prototyped" -- so
dnl all 56 would silently answer the opposite of the truth.
AC_DEFUN([AC_CHECK_PROTO],
[m4_pushdef([ac_Proto], [ac_cv_prototype_]m4_translit([$1], [./+-], [__p_]))dnl
AC_CACHE_CHECK([if $1 is prototyped], [ac_Proto],
[
  OLDCPPFLAGS=$CPPFLAGS
  CPPFLAGS="$CPPFLAGS -I$srcdir"
  dnl -fno-builtin keeps gcc's own idea of the signature out of the way.
  if test "${ac_cv_gcc_fnb-no}" = yes; then
    OLDCFLAGS=$CFLAGS
    CFLAGS="$CFLAGS -fno-builtin"
  fi
  AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#define NO_LIBRARY_PROTOTYPES
#define __COMM_C__
#define __ACT_OTHER_C__
#include "src/sysdep.h"
#ifdef $1
  error - already defined!
#endif
void $1(int a, char b, int c, char d, int e, char f, int g, char h);
]], [[]])], [ac_Proto=no], [ac_Proto=yes])
  if test "${ac_cv_gcc_fnb-no}" = yes; then
    CFLAGS=$OLDCFLAGS
  fi
  CPPFLAGS=$OLDCPPFLAGS
])
AS_IF([test "$ac_Proto" = no],
  [AC_DEFINE([NEED_]m4_toupper([$1])[_PROTO], [1],
     [Check for a prototype to $1.])])
m4_popdef([ac_Proto])dnl
])


dnl AC_UNSAFE_CRYPT
dnl
dnl Some crypt() implementations only look at the first eight characters of
dnl the password, so two different passwords can hash the same.  Detect that
dnl and let the game work around it.
AC_DEFUN([AC_UNSAFE_CRYPT],
[
  AC_CACHE_CHECK([whether crypt needs over 10 characters], ac_cv_unsafe_crypt, [
    if test ${ac_cv_header_crypt_h-no} = yes; then
      use_crypt_header="#include <crypt.h>"
    fi
    if test ${ac_cv_lib_crypt_crypt-no} = yes; then
      ORIGLIBS=$LIBS
      LIBS="-lcrypt $LIBS"
    fi
    AC_RUN_IFELSE([AC_LANG_SOURCE([[
#define _XOPEN_SOURCE
#include <stdlib.h>
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
]])], [ac_cv_unsafe_crypt=yes], [ac_cv_unsafe_crypt=no], [ac_cv_unsafe_crypt=no])])
if test $ac_cv_unsafe_crypt = yes; then
  AC_DEFINE([HAVE_UNSAFE_CRYPT], [1],
    [Define if we don't have proper support for the system's crypt().])
fi
if test ${ac_cv_lib_crypt_crypt-no} = yes; then
  LIBS=$ORIGLIBS
fi
])
