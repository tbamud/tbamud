cmake_minimum_required(VERSION 3.27)

# Set the project name
project(circle)

# Global definitions
if(MSVC)
	# using Visual Studio C++
	add_definitions(-D_CRT_SECURE_NO_WARNINGS -D_CRT_SECURE_NO_DEPRECATE -D_CRT_NONSTDC_NO_DEPRECATE)
endif()

# circle itself
file(GLOB CIRCLE_SOURCES
	"*.h"
	"*.c"
)

list(FILTER CIRCLE_SOURCES EXCLUDE REGEX ".*/dg_.*\\.c$")

add_executable(circle ${CIRCLE_SOURCES})
target_sources(circle PRIVATE ${CMAKE_SOURCE_DIR}/third_party/tomlc99/toml.c)
target_include_directories(circle PRIVATE ${CMAKE_SOURCE_DIR}/third_party/tomlc99)

if(MSVC)
	target_link_libraries(circle wsock32.lib)
	
	set_target_properties(circle PROPERTIES
		VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/.."
	)
endif()
