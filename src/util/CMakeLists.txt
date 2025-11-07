
set(TOOLS
  asciipasswd
  autowiz
  plrtoascii
  rebuildIndex
  rebuildMailIndex
  shopconv
  sign
  split
  wld2html
)

# common includes and flags
include_directories(${CMAKE_SOURCE_DIR}/src)
add_definitions(-DCIRCLE_UTIL)

find_library(CRYPT_LIBRARY crypt)
find_library(NETLIB_LIBRARY nsl socket)  # for sign.c, hvis nødvendig

foreach(tool ${TOOLS})
    if(${tool} STREQUAL "rebuildIndex")
        add_executable(rebuildIndex rebuildAsciiIndex.c)
    else()
        add_executable(${tool} ${tool}.c)
    endif()

    # Set output location
    set_target_properties(${tool} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin
    )

    # Link to libcrypt for asciipasswd
    if(${tool} STREQUAL "asciipasswd" AND CRYPT_LIBRARY)
        target_link_libraries(${tool} ${CRYPT_LIBRARY})
    endif()

    # Link to netlib for sign
    if(${tool} STREQUAL "sign" AND NETLIB_LIBRARY)
        target_link_libraries(${tool} ${NETLIB_LIBRARY})
    endif()
endforeach()

add_custom_target(utils DEPENDS ${TOOLS})

