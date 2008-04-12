/* CircleMUD for ACORN conf.h file - manually created (G. Duncan 13 June 98)

#ifndef _CONF_H_
#define _CONF_H_

#define CIRCLE_ACORN

/* Define to empty if the keyword does not work.  */
/*#define const*/

/* Define to `int' if <sys/types.h> doesn't define.  */
/*#define pid_t int*/

/* Define as the return type of signal handlers (int or void).  */
#define RETSIGTYPE void

/* Define to `unsigned' if <sys/types.h> doesn't define.  */
/*#define size_t*/

/* Define if you have 'struct in_addr' */
#define HAVE_STRUCT_IN_ADDR 1

/* Define if your crypt isn't safe with only 10 characters. */
#undef HAVE_UNSAFE_CRYPT

/* Define to `int' if <sys/socket.h> doesn't define.  */
#define socklen_t int

/* Define if you have the <string.h> header file.  */
#define HAVE_STRING_H

/* Define if you have the <strings.h> header file.  */
#define HAVE_STRINGS_H

#define HAVE_MEMORY_H

/* Define if you have the ANSI C header files.  */
#define STDC_HEADERS

/* Define if you have the <sys/types.h> header file.  */
#define HAVE_SYS_TYPES_H
#define HAVE_UNISTD_H

/* Define if you have the <limits.h> header file.  */
#define HAVE_LIMITS_H

/* Define if you have the <errno.h> header file.  */
#define HAVE_ERRNO_H
#define HAVE_SYS_ERRNO_H

/* Define if you can safely include both <sys/time.h> and <time.h>.  */
#define TIME_WITH_SYS_TIME
#define HAVE_SYS_TIME_H

/* Define if you have the <assert.h> header file.  */
#define HAVE_ASSERT_H

/* Define if you have the <fcntl.h> header file.  */
#define HAVE_FCNTL_H
/*#define HAVE_SYS_FCNTL_H*/

/* Define if you have the <sys/socket.h> header file.  */
#define HAVE_SYS_SOCKET_H
#define HAVE_SYS_RESOURCE_H

/* Define if you have <sys/wait.h> that is POSIX. compatible.  */
#define HAVE_SYS_WAIT_H
#define HAVE_NETINET_IN_H
#define HAVE_NETDB_H
#define HAVE_SIGNAL_H
#define HAVE_SYS_UIO_H
#define HAVE_SYS_STAT_H

#define NEED_GETTIMEOFDAY_PROTO

/* Define if your compiler does not prototype remove().  */
/* #undef NEED_REMOVE_PROTO */

/* Define if your compiler does not prototype strerror().  */
/* #undef NEED_STRERROR_PROTO */

#endif /* _CONF_H_ */
