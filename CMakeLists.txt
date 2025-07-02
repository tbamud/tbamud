cmake_minimum_required(VERSION 3.12)
project(TbaMUD C)

set(CMAKE_C_STANDARD 99)

# Include checker modules
include(CheckFunctionExists)
include(CheckIncludeFile)
include(CheckTypeSize)
include(CheckStructHasMember)
include(CheckSymbolExists)
include(CheckCSourceCompiles)

# Output paths
set(BIN_OUTPUT_DIR ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${BIN_OUTPUT_DIR})

# Include source and build paths
include_directories(src ${CMAKE_BINARY_DIR})

# ========== Compiler flags ==========
if (CMAKE_COMPILER_IS_GNUCC)
    include(CheckCCompilerFlag)

    check_c_compiler_flag(-Wall SUPPORTS_WALL)
    check_c_compiler_flag(-Wno-char-subscripts SUPPORTS_WNO_CHAR_SUBSCRIPTS)

    if (SUPPORTS_WALL)
        set(MYFLAGS "-Wall")
        if (SUPPORTS_WNO_CHAR_SUBSCRIPTS)
            set(MYFLAGS "${MYFLAGS} -Wno-char-subscripts")
        endif()
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${MYFLAGS}")
    endif()
endif()

# clang-tidy if available
find_program(CLANG_TIDY_EXE NAMES clang-tidy)

if(CLANG_TIDY_EXE AND STATIC_ANALYSIS)
    message(STATUS "clang-tidy enabled: ${CLANG_TIDY_EXE}")
    set(CMAKE_C_CLANG_TIDY "${CLANG_TIDY_EXE}")
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
else()
    message(WARNING "clang-tidy not found. Static analysis disabled.")
endif()

# ========== Header checks ==========
check_include_file("fcntl.h" HAVE_FCNTL_H)
check_include_file("errno.h" HAVE_ERRNO_H)
check_include_file("string.h" HAVE_STRING_H)
check_include_file("strings.h" HAVE_STRINGS_H)
check_include_file("limits.h" HAVE_LIMITS_H)
check_include_file("sys/select.h" HAVE_SYS_SELECT_H)
check_include_file("sys/wait.h" HAVE_SYS_WAIT_H)
check_include_file("sys/types.h" HAVE_SYS_TYPES_H)
check_include_file("unistd.h" HAVE_UNISTD_H)
check_include_file("memory.h" HAVE_MEMORY_H)
check_include_file("assert.h" HAVE_ASSERT_H)
check_include_file("arpa/telnet.h" HAVE_ARPA_TELNET_H)
check_include_file("arpa/inet.h" HAVE_ARPA_INET_H)
check_include_file("sys/stat.h" HAVE_SYS_STAT_H)
check_include_file("sys/socket.h" HAVE_SYS_SOCKET_H)
check_include_file("sys/resource.h" HAVE_SYS_RESOURCE_H)
check_include_file("netinet/in.h" HAVE_NETINET_IN_H)
check_include_file("netdb.h" HAVE_NETDB_H)
check_include_file("signal.h" HAVE_SIGNAL_H)
check_include_file("sys/uio.h" HAVE_SYS_UIO_H)
check_include_file("mcheck.h" HAVE_MCHECK_H)
check_include_file("stdlib.h" HAVE_STDLIB_H)
check_include_file("stdarg.h" HAVE_STDARG_H)
check_include_file("float.h" HAVE_FLOAT_H)

if (HAVE_STDLIB_H AND HAVE_STDARG_H AND HAVE_STRING_H AND HAVE_FLOAT_H)
    set(STDC_HEADERS 1)
endif()

# macros
macro(check_run_return_value CODE EXPECTED_RESULT VAR_NAME)
    set(_file "${CMAKE_BINARY_DIR}/check_run_${VAR_NAME}.c")
    file(WRITE "${_file}" "${CODE}")
    try_run(_run_result _compile_result
            ${CMAKE_BINARY_DIR} ${_file}
    )
    if (_compile_result EQUAL 0 AND _run_result EQUAL ${EXPECTED_RESULT})
        set(${VAR_NAME} TRUE)
    else()
        set(${VAR_NAME} FALSE)
    endif()
endmacro()

# ========== Function checks ==========
foreach(FUNC gettimeofday select snprintf strcasecmp strdup strerror
        stricmp strlcpy strncasecmp strnicmp strstr vsnprintf vprintf
        inet_addr inet_aton)
    string(TOUPPER "${FUNC}" _upper_name)
    check_function_exists(${FUNC} HAVE_${_upper_name})
endforeach()

if (NOT HAVE_VPRINTF)
    check_function_exists(_doprnt HAVE_DOPRNT)
endif()


# ========== Type checks ==========
check_type_size("pid_t" HAVE_PID_T)
check_type_size("size_t" HAVE_SIZE_T)
check_type_size("ssize_t" HAVE_SSIZE_T)
set(CMAKE_EXTRA_INCLUDE_FILES "sys/socket.h")
check_type_size("socklen_t" HAVE_SOCKLEN_T)
unset(CMAKE_EXTRA_INCLUDE_FILES)


if (NOT HAVE_PID_T)
    set(pid_t int)
endif()

if (NOT HAVE_SIZE_T)
    set(size_t "unsigned")
endif()

if (NOT HAVE_SSIZE_T)
    set(ssize_t int)
endif()

if (NOT HAVE_SOCKLEN_T)
    set(socklen_t int)
endif()

# ========== const ==========
check_c_source_compiles("
int main() {

/* Ultrix mips cc rejects this.  */
typedef int charset[2]; const charset x;
/* SunOS 4.1.1 cc rejects this.  */
char const *const *ccp;
char **p;
/* NEC SVR4.0.2 mips cc rejects this.  */
struct point {int x, y;};
static struct point const zero = {0,0};
/* AIX XL C 1.02.0.0 rejects this.
   It does not let you subtract one const X* pointer from another in an arm
   of an if-expression whose if-part is not a constant expression */
const char *g = \"string\";
ccp = &g + (g ? g-g : 0);
/* HPUX 7.0 cc rejects these. */
++ccp;
p = (char**) ccp;
ccp = (char const *const *) p;
{ /* SCO 3.2v4 cc rejects this.  */
  char *t;
  char const *s = 0 ? (char *) 0 : (char const *) 0;

  *t++ = 0;
}
{ /* Someone thinks the Sun supposedly-ANSI compiler will reject this.  */
  int x[] = {25, 17};
  const int *foo = &x[0];
  ++foo;
}
{ /* Sun SC1.0 ANSI compiler rejects this -- but not the above. */
  typedef const int *iptr;
  iptr p = 0;
  ++p;
}
{ /* AIX XL C 1.02.0.0 rejects this saying
     \"k.c\", line 2.27: 1506-025 (S) Operand must be a modifiable lvalue. */
  struct s { int j; const int *ap[3]; };
  struct s *b; b->j = 5;
}
{ /* ULTRIX-32 V3.1 (Rev 9) vcc rejects this */
  const int foo = 10;
}

; return 0; }
" HAVE_CONST)

if (HAVE_CONST)
    set(CONST_KEYWORD const)
else()
    set(CONST_KEYWORD "")
endif()

# ========== Struct checks ==========
if (HAVE_NETINET_IN_H)
    check_struct_has_member("struct in_addr" s_addr netinet/in.h HAVE_STRUCT_IN_ADDR)
endif()

# ========== crypt()/libcrypt ==========

find_library(CRYPT_LIBRARY crypt)
if (CRYPT_LIBRARY)
    message(STATUS "Found libcrypt: ${CRYPT_LIBRARY}")
    list(APPEND EXTRA_LIBS ${CRYPT_LIBRARY})
    set(_saved_lib_list ${CMAKE_REQUIRED_LIBRARIES})
    set(CMAKE_REQUIRED_LIBRARIES ${CRYPT_LIBRARY})
    check_include_file("crypt.h" HAVE_CRYPT_H)
    check_function_exists(crypt CIRCLE_CRYPT)

    check_run_return_value("
#include <string.h>
#include <unistd.h>
${HAVE_CRYPT_H} ? \"#include <crypt.h>\" : \"\"

int main(void)
{
  char pwd[11], pwd2[11];

  strncpy(pwd, (char *)crypt(\"FooBar\", \"BazQux\"), 10);
  pwd[10] = '\\\\0';
  strncpy(pwd2, (char *)crypt(\"xyzzy\", \"BazQux\"), 10);
  pwd2[10] = '\\\\0';
  if (strcmp(pwd, pwd2) == 0)
    exit(0);
  exit(1);
}
    " 0 HAVE_UNSAFE_CRYPT)

    set(CMAKE_REQUIRED_LIBRARIES ${_saved_lib_list})
endif()


# ========== network libs ==========
check_function_exists(gethostbyaddr HAVE_GETHOSTBYADDR)
if (NOT HAVE_GETHOSTBYADDR)
    message(STATUS "gethostbyaddr() not available, trying nsllib")
    find_library(NSL_LIBRARY nsl)
    if (NSL_LIBRARY)
        message(STATUS "...nsllib found.")
        list(APPEND EXTRA_LIBS ${NSL_LIBRARY})
    endif()
endif()

check_function_exists(socket HAVE_SOCKET)
if (NOT HAVE_SOCKET)
    message(STATUS "socket() not available, trying socketlib")
    find_library(SOCKET_LIBRARY socket)
    if (SOCKET_LIBRARY)
        message(STATUS "...socketlib found")
        list(APPEND EXTRA_LIBS ${SOCKET_LIBRARY})
    endif()
endif()

# ========== time.h needs special treatment ==========
check_include_file("sys/time.h" HAVE_SYS_TIME_H)
check_include_file("sys/time.h" HAVE_TIME_H)

if (HAVE_SYS_TIME_H AND HAVE_TIME_H)
    check_c_source_compiles("
#include <sys/types.h>
#include <sys/time.h>
#include <time.h>
int main() {
struct tm *tp;
; return 0; }
    " TIME_WITH_SYS_TIME)
endif()

# ========== Determine return value of signal() ==========
check_c_source_compiles("
    #include <signal.h>
    int handler(int sig) { return 0; }
    int main() {
        signal(SIGINT, handler);
        return 1;
    }
" SIGNAL_RETURNS_INT FAIL_REGEX ".*incompatible pointer type.*")

check_c_source_compiles("
    #include <signal.h>
    void handler(int sig) { }
    int main() {
        signal(SIGINT, handler);
        return 1;
    }
" SIGNAL_RETURNS_VOID FAIL_REGEX ".*incompatible pointer type.*")

if (SIGNAL_RETURNS_INT)
    message(STATUS "signal() returns int.")
    set(RETSIGTYPE int)
elseif (SIGNAL_RETURNS_VOID)
    message(STATUS "signal() returns void.")
    set(RETSIGTYPE void)
else()
    message(FATAL_ERROR "Could not determine return value from signal handler.")
endif()

# ========== Define general UNIX-system ==========
if (UNIX)
    set(CIRCLE_UNIX 1)
endif()

set(PROTO_FUNCTIONS
        accept
        bind
        gettimeofday
        atoi
        atol
        bzero
        chdir
        close
        fclose
        fcntl
        fflush
        fprintf
        fputc
        fread
        fscanf
        fseek
        fwrite
        getpeername
        getpid
        getrlimit
        getsockname
        htonl
        htons
        inet_addr
        inet_aton
        inet_ntoa
        listen
        ntohl
        perror
        printf
        qsort
        read
        remove
        rewind
        select
        setitimer
        setrlimit
        setsockopt
        snprintf
        sprintf
        sscanf
        strcasecmp
        strdup
        strerror
        stricmp
        strlcpy
        strncasecmp
        strnicmp
        system
        time
        unlink
        vsnprintf
        write
        socket
)

configure_file(
        ${CMAKE_SOURCE_DIR}/src/conf.h.cmake.in
        ${CMAKE_BINARY_DIR}/tmp_conf.h
)

macro(check_function_prototype FUNCTION)
    set(_code "
#define NO_LIBRARY_PROTOTYPES
#define __COMM_C__
#define __ACT_OTHER_C__
#include \"${CMAKE_BINARY_DIR}/tmp_conf.h\"
#include \"${CMAKE_SOURCE_DIR}/src/sysdep.h\"
#ifdef ${FUNCTION}
  error - already defined!
#endif
void ${FUNCTION}(int a, char b, int c, char d, int e, char f, int g, char h);

int main() {

; return 0; }
    ")
    string(TOUPPER "${FUNCTION}" _upper_name)
    check_c_source_compiles("${_code}" NEED_${_upper_name}_PROTO FAIL_REGEX ".*incompatible pointer type.*")
    if (NEED_${_upper_name}_PROTO)
        message(STATUS "${FUNCTION}() has no prototype, NEED_${_upper_name}_PROTO set!")
    else()
        message(STATUS "${FUNCTION}() has a prototype, not setting NEED_${_upper_name}_PROTO")
    endif()
endmacro()


foreach (FUNC ${PROTO_FUNCTIONS})
    check_function_prototype(${FUNC})
endforeach()


# ========== Generate conf.h ==========
configure_file(
        ${CMAKE_SOURCE_DIR}/src/conf.h.cmake.in
        ${CMAKE_BINARY_DIR}/conf.h
)



# ========== Source-filer ==========
file(GLOB SRC_FILES src/*.c)

# ========== Bygg kjørbar ==========
add_executable(circle ${SRC_FILES})
target_link_libraries(circle ${EXTRA_LIBS})

add_subdirectory(src/util)

if (MEMORY_DEBUG)
    message(STATUS "MEMORY_DEBUG is activated, setting up zmalloc")
    target_compile_definitions(circle PRIVATE MEMORY_DEBUG)
endif()
