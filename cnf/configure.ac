dnl Process this file with autoconf to produce a configure script.  Run both
dnl of these from the top of the tree, not from cnf/:
dnl
dnl   autoconf  -I cnf -o configure cnf/configure.ac
dnl   autoheader -I cnf            cnf/configure.ac
dnl
dnl autoheader writes src/conf.h.in; configure turns that into src/conf.h.
dnl Never edit configure or src/conf.h.in by hand -- the next regeneration
dnl throws the edit away without saying so.

AC_PREREQ([2.69])

dnl The name and version below only feed the PACKAGE_* defines.  No tbaMUD
dnl source reads them; the version players see is tbamud_version, in
dnl src/constants.c.
AC_INIT([tbaMUD],[2025],[https://github.com/tbamud/tbamud/issues])
AC_CONFIG_SRCDIR([src/act.comm.c])
AC_CONFIG_HEADERS([src/conf.h])

dnl src/conf.h.in used to carry this guard by hand.  autoheader owns the
dnl file now, so the guard has to be declared here to survive regeneration.
AH_TOP([#ifndef _CONF_H_
#define _CONF_H_])
AH_BOTTOM([#endif /* _CONF_H_ */])

AC_SUBST(MYFLAGS)
AC_SUBST(NETLIB)
AC_SUBST(CRYPTLIB)

AC_DEFINE([CIRCLE_UNIX], [1],
  [Define if we're compiling CircleMUD under any type of UNIX system.])

dnl Find the 'more' program
AC_CHECK_PROGS(MORE, less most more cat)

dnl Checks for programs.
AC_PROG_CC

dnl If we're using gcc, use gcc options.
dnl If not, test for various common switches to make a 'cc' compiler
dnl compile ANSI C code.
if test "$GCC" = yes; then

  dnl Determine if gcc -Wall causes warnings on isascii(), etc.
  AC_CACHE_CHECK(whether ${CC-cc} -Wall also needs -Wno-char-subscripts,
	ac_cv_char_warn,
  [
    OLDCFLAGS=$CFLAGS
    CFLAGS="$CFLAGS -Wall -Werror"
    AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <ctype.h>]],
       [[ int i; char c = '0';
         i = isascii(c);
         i = isdigit(c);
         i = isprint(c);
         (void)i;
       ]])], ac_cv_char_warn=no, ac_cv_char_warn=yes)
    CFLAGS=$OLDCFLAGS
  ])

  dnl If Determine if gcc can accept -Wno-char-subscripts
  AC_CACHE_CHECK(whether ${CC-cc} accepts -Wno-char-subscripts, ac_cv_gcc_ncs,
  [
    OLDCFLAGS=$CFLAGS
    CFLAGS="$CFLAGS -Wno-char-subscripts"
    AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[]], [[]])], ac_cv_gcc_ncs=yes, ac_cv_gcc_ncs=no)
    CFLAGS=$OLDCFLAGS
  ])

  dnl If Determine if gcc can accept -fno-builtin
  AC_CACHE_CHECK(whether ${CC-cc} accepts -fno-builtin, ac_cv_gcc_fnb,
  [
    OLDCFLAGS=$CFLAGS
    CFLAGS="$CFLAGS -fno-builtin"
    AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[]], [[]])], ac_cv_gcc_fnb=yes, ac_cv_gcc_fnb=no)
    CFLAGS=$OLDCFLAGS
  ])

  dnl If gcc -Wall gives no warnings with isascii(), use "-Wall";
  dnl Otherwise, if gcc -Wall gives isascii warnings:
  dnl    If we can use -Wno-char-subscripts, use "-Wall -Wno-char-subscripts"
  dnl    If can't use -Wno-char-subscripts, use no flags at all.

  if test ${ac_cv_char_warn:-ERROR} = no; then
    MYFLAGS="-Wall"
  else
    if test ${ac_cv_gcc_ncs:-ERROR} = yes; then
      MYFLAGS="-Wall -Wno-char-subscripts"
    else
      MYFLAGS=""
    fi
  fi

else
  dnl We aren't using gcc so we can't assume any special flags.
  MYFLAGS=""

fi

dnl Checks for libraries.  We check for the library only if the function is
dnl not available without the library.
AC_CHECK_FUNC(gethostbyaddr, ,
    [AC_CHECK_LIB(nsl, gethostbyaddr, NETLIB="-lnsl $NETLIB")])

AC_CHECK_FUNC(socket, ,
    [AC_CHECK_LIB(socket, socket, NETLIB="-lsocket $NETLIB")])

AC_CHECK_FUNC(malloc, ,
    [AC_CHECK_LIB(malloc, malloc)])

AC_CHECK_FUNC(crypt,
    [AC_DEFINE([CIRCLE_CRYPT], [1],
       [Define if the system is capable of using crypt() to encrypt.])],
    [AC_CHECK_LIB(crypt, crypt,
       [AC_DEFINE([CIRCLE_CRYPT], [1],
          [Define if the system is capable of using crypt() to encrypt.])
        CRYPTLIB="-lcrypt"])]
    )

dnl Checks for header files.
dnl AC_HEADER_STDC became a no-op in autoconf 2.70: every hosted C
dnl implementation has had the C89 headers for decades, so the probe could
dnl only ever answer yes.  sysdep.h still reads STDC_HEADERS, so define it
dnl here rather than leave it silently undefined.
AC_DEFINE([STDC_HEADERS], [1], [Define if you have the ANSI C header files.])
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS(fcntl.h sys/fcntl.h errno.h net/errno.h string.h strings.h)
AC_CHECK_HEADERS(limits.h sys/time.h sys/select.h sys/types.h unistd.h)
AC_CHECK_HEADERS(memory.h crypt.h assert.h arpa/telnet.h arpa/inet.h)
AC_CHECK_HEADERS(sys/stat.h sys/socket.h sys/resource.h netinet/in.h netdb.h)
AC_CHECK_HEADERS(signal.h sys/uio.h mcheck.h)

AC_UNSAFE_CRYPT

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T

dnl AC_HEADER_TIME was withdrawn in autoconf 2.70.  This is the body it used
dnl to expand to; sysdep.h still switches on TIME_WITH_SYS_TIME.
if test ${ac_cv_header_sys_time_h:-no} = yes; then
  AC_DEFINE([TIME_WITH_SYS_TIME], [1],
    [Define if you can safely include both <sys/time.h> and <time.h>.])
fi

dnl Check for the 'struct in_addr' definition. Ugly, yes.
if test $ac_cv_header_netinet_in_h = no; then
  ac_cv_struct_in_addr=no
else
  if test $ac_cv_header_sys_types_h = yes; then
    headers=`cat << EOF
#include <sys/types.h>
#include <netinet/in.h>
EOF
`
  else
    headers="#include <netinet/in.h>"
  fi

  AC_CACHE_CHECK([for struct in_addr], ac_cv_struct_in_addr,
    [ AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[$headers]], [[struct in_addr tp; (void)tp.s_addr;]])], ac_cv_struct_in_addr=yes, ac_cv_struct_in_addr=no)])

  if test $ac_cv_struct_in_addr = yes; then
    AC_DEFINE([HAVE_STRUCT_IN_ADDR], [1],
      [Define if the system has struct in_addr.])
  fi
fi


dnl Check for the 'typedef socklen_t' definition. Even uglier, yes.
if test $ac_cv_header_sys_socket_h = no; then
  ac_cv_socklen_t=no
else
  AC_CACHE_CHECK([for typedef socklen_t], ac_cv_socklen_t,
    [ AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <sys/socket.h>]], [[socklen_t sl; sl=0; (void)sl;]])], ac_cv_socklen_t=yes, ac_cv_socklen_t=no)])
fi

if test $ac_cv_socklen_t = no; then
  AC_DEFINE([socklen_t], [int],
    [Define to 'int' if <sys/socket.h> doesn't define.])
fi


dnl Checks for library functions.
dnl AC_TYPE_SIGNAL was withdrawn in autoconf 2.70: C89 fixed the return type
dnl at void, and the probe it used to run is the one that answered 'int' on a
dnl modern compiler and broke the build without a word.
AC_DEFINE([RETSIGTYPE], [void],
  [Define as the return type of signal handlers (int or void).])
AC_FUNC_VPRINTF
AC_CHECK_FUNCS(gettimeofday select snprintf strcasecmp strdup strerror stricmp strlcpy strncasecmp strnicmp strstr vsnprintf)

dnl Check for functions that parse IP addresses
ORIGLIBS=$LIBS
LIBS="$LIBS $NETLIB"
AC_CHECK_FUNCS(inet_addr inet_aton)
LIBS=$ORIGLIBS

dnl Check for prototypes
AC_CHECK_PROTO(accept)
AC_CHECK_PROTO(atoi)
AC_CHECK_PROTO(atol)
AC_CHECK_PROTO(bind)
AC_CHECK_PROTO(bzero)
AC_CHECK_PROTO(chdir)
AC_CHECK_PROTO(close)
AC_CHECK_PROTO(crypt)
AC_CHECK_PROTO(fclose)
AC_CHECK_PROTO(fcntl)
AC_CHECK_PROTO(fflush)
AC_CHECK_PROTO(fprintf)
AC_CHECK_PROTO(fputc)
AC_CHECK_PROTO(fputs)
AC_CHECK_PROTO(fread)
AC_CHECK_PROTO(fscanf)
AC_CHECK_PROTO(fseek)
AC_CHECK_PROTO(fwrite)
AC_CHECK_PROTO(getpeername)
AC_CHECK_PROTO(getpid)
AC_CHECK_PROTO(getrlimit)
AC_CHECK_PROTO(getsockname)
AC_CHECK_PROTO(gettimeofday)
AC_CHECK_PROTO(htonl)
AC_CHECK_PROTO(htons)
AC_CHECK_PROTO(inet_addr)
AC_CHECK_PROTO(inet_aton)
AC_CHECK_PROTO(inet_ntoa)
AC_CHECK_PROTO(listen)
AC_CHECK_PROTO(ntohl)
AC_CHECK_PROTO(perror)
AC_CHECK_PROTO(printf)
AC_CHECK_PROTO(qsort)
AC_CHECK_PROTO(read)
AC_CHECK_PROTO(remove)
AC_CHECK_PROTO(rewind)
AC_CHECK_PROTO(select)
AC_CHECK_PROTO(setitimer)
AC_CHECK_PROTO(setrlimit)
AC_CHECK_PROTO(setsockopt)
AC_CHECK_PROTO(snprintf)
AC_CHECK_PROTO(socket)
AC_CHECK_PROTO(sprintf)
AC_CHECK_PROTO(sscanf)
AC_CHECK_PROTO(strcasecmp)
AC_CHECK_PROTO(strdup)
AC_CHECK_PROTO(strerror)
AC_CHECK_PROTO(stricmp)
AC_CHECK_PROTO(strlcpy)
AC_CHECK_PROTO(strncasecmp)
AC_CHECK_PROTO(strnicmp)
AC_CHECK_PROTO(system)
AC_CHECK_PROTO(time)
AC_CHECK_PROTO(unlink)
AC_CHECK_PROTO(vsnprintf)
AC_CHECK_PROTO(write)

AC_CONFIG_FILES([src/Makefile src/util/Makefile tests/Makefile])
AC_OUTPUT
#
echo "Configuration completed.  To compile, type:  cd src; make"
