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

add_executable(circle ${CIRCLE_SOURCES})

if(MSVC)
	target_link_libraries(circle wsock32.lib)
	
	set_target_properties(circle PROPERTIES
		VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/.."
	)
endif()