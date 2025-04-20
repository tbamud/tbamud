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

# ========== Header checks ==========
check_include_file("fcntl.h" HAVE_FCNTL_H)
check_include_file("errno.h" HAVE_ERRNO_H)
check_include_file("string.h" HAVE_STRING_H)
check_include_file("strings.h" HAVE_STRINGS_H)
check_include_file("limits.h" HAVE_LIMITS_H)
check_include_file("sys/time.h" HAVE_SYS_TIME_H)
check_include_file("sys/select.h" HAVE_SYS_SELECT_H)
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

# macro for checking for functions in libraries
macro(check_function_with_libx FUNCTION LIB VARIABLE)
    check_function_exists(${FUNCTION} ${VARIABLE})
    if (NOT ${VARIABLE})
        find_library(_libpath ${LIB})
        if (_libpath)
            message(STATUS "${FUNCTION} not available, trying library: ${_libpath}")
            set(_saved_libs ${CMAKE_REQUIRED_LIBRARIES})
            set(CMAKE_REQUIRED_LIBRARIES ${_libpath})
            check_function_exists(${FUNCTION} ${VARIABLE})
            if (VARIABLE)
                message(STATUS "...including library ${_libpath} to get access to ${FUNCTION}.")
                list(APPEND EXTRA_LIBS ${_libpath})
            else()
                message(STATUS "...but didn't find it!")
            endif()
            set(CMAKE_REQUIRED_LIBRARIES ${_saved_libs})
        else()
            message(STATUS "Library ${LIB} not available - ${FUNCTION}() undefined.")
        endif()
    endif()
endmacro()

macro(check_function_with_lib FUNCTION LIBNAME VARIABLE)
    check_function_exists(${FUNCTION} ${VARIABLE})
    if (NOT ${VARIABLE})

        find_library(_FOUND_LIB-${LIBNAME} ${LIBNAME})
        if (_FOUND_LIB-${LIBNAME})
            message(STATUS "${FUNCTION} not found without lib, trying with: ${_FOUND_LIB-${LIBNAME}}")
            set(_saved_libs ${CMAKE_REQUIRED_LIBRARIES})
            set(CMAKE_REQUIRED_LIBRARIES ${_FOUND_LIB-${LIBNAME}})

            set(_code "
            #include <crypt.h>
            int main() {
                (void)${FUNCTION};
                return 0;
            }
        ")

            # Testkildekoden
            check_c_source_compiles("${_code}" ${VARIABLE})
            if (${VARIABLE})
                message(STATUS "Found ${FUNCTION} using ${_FOUND_LIB-${LIBNAME}}")
                list(APPEND EXTRA_LIBS ${_FOUND_LIB-${LIBNAME}})
            else()
                message(STATUS "${FUNCTION} still not found with ${_FOUND_LIB-${LIBNAME}}")
            endif()
            set(CMAKE_REQUIRED_LIBRARIES ${_saved_libs})
        else()
            message(STATUS "Could not find library ${LIBNAME} to try ${FUNCTION}")
        endif()
    endif()
endmacro()


# ========== Function checks ==========
foreach(FUNC gettimeofday select snprintf strcasecmp strdup strerror
        stricmp strlcpy strncasecmp strnicmp strstr vsnprintf
        inet_addr inet_aton)
    check_function_exists(${FUNC} HAVE_${FUNC})
endforeach()

# ========== Type checks ==========
check_type_size("pid_t" HAVE_PID_T)
check_type_size("size_t" HAVE_SIZE_T)
check_type_size("ssize_t" HAVE_SSIZE_T)
check_type_size("socklen_t" HAVE_SOCKLEN_T)

# ========== Struct checks ==========
if (HAVE_NETINET_IN_H)
    check_struct_has_member("struct in_addr" s_addr netinet/in.h HAVE_STRUCT_IN_ADDR)
endif()

# ========== crypt()/libcrypt ==========
find_library(CRYPT_LIBRARY crypt)
if (CRYPT_LIBRARY)
    message(STATUS "Found libcrypt: ${CRYPT_LIBRARY}")
    list(APPEND EXTRA_LIBS ${CRYPT_LIBRARY})
    set(CMAKE_REQUIRED_LIBRARIES ${CRYPT_LIBRARY})
endif()

# ========== Ekstra nettverkslibs ==========
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

# ========== Define generelt UNIX-system ==========
if (UNIX)
    set(CIRCLE_UNIX 1)
endif()

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
