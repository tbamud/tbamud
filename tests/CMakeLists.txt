# tests/CMakeLists.txt
# Unity-based unit tests for tbaMUD
#
# Each test executable is built from:
#   vendor/unity/unity.c   — test framework
#   test_stubs.c           — weak-symbol stubs for all unresolved mud globals
#   src/<module>.c         — the source file(s) under test
#   test_<name>.c          — the test file itself
#
# Only the source files being tested are compiled — in particular comm.c
# (which contains main()) is never included.

cmake_minimum_required(VERSION 3.12)

enable_testing()

# ---------------------------------------------------------------------------
# Common settings shared by all test targets
# ---------------------------------------------------------------------------

set(UNITY_SRC  ${CMAKE_CURRENT_SOURCE_DIR}/vendor/unity/unity.c)
set(STUBS_SRC  ${CMAKE_CURRENT_SOURCE_DIR}/test_stubs.c)

set(MUD_SRCDIR ${CMAKE_SOURCE_DIR}/src)

# Include paths: mud source dir for conf.h / structs.h etc.,
# CMake binary dir for the generated conf.h (cmake builds place it there),
# and the Unity header dir.
set(TEST_INCLUDES
    ${MUD_SRCDIR}
    ${CMAKE_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/vendor/unity
)

# Suppress warnings that fire in generated stubs / vendored code and in the
# mud sources when compiled outside their normal full-build context.
set(TEST_CFLAGS)
if(CMAKE_C_COMPILER_ID MATCHES "^(GNU|Clang|AppleClang)$")
    list(APPEND TEST_CFLAGS
        -Wno-unused-parameter
        -Wno-unused-function
        -Wno-unused-variable
    )
endif()

# Helper macro: add_mud_test(name SRC1 [SRC2 …])
# Creates an executable, registers it with CTest.
macro(add_mud_test TEST_NAME)
    add_executable(${TEST_NAME}
        ${UNITY_SRC}
        ${STUBS_SRC}
        ${ARGN}
    )
    target_include_directories(${TEST_NAME} PRIVATE ${TEST_INCLUDES})
    target_compile_options(${TEST_NAME} PRIVATE ${TEST_CFLAGS})
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
endmacro()

# ---------------------------------------------------------------------------
# test_utils  — covers src/utils.c
# ---------------------------------------------------------------------------
add_mud_test(test_utils
    ${MUD_SRCDIR}/utils.c
    ${MUD_SRCDIR}/random.c
    ${CMAKE_CURRENT_SOURCE_DIR}/test_utils.c
)

# ---------------------------------------------------------------------------
# test_random — covers src/random.c and rand_number/dice in src/utils.c
# ---------------------------------------------------------------------------
add_mud_test(test_random
    ${MUD_SRCDIR}/random.c
    ${MUD_SRCDIR}/utils.c
    ${CMAKE_CURRENT_SOURCE_DIR}/test_random.c
)

# ---------------------------------------------------------------------------
# test_interpreter — covers string helpers in src/interpreter.c
# ---------------------------------------------------------------------------
add_mud_test(test_interpreter
    ${MUD_SRCDIR}/interpreter.c
    ${MUD_SRCDIR}/utils.c
    ${MUD_SRCDIR}/random.c
    ${CMAKE_CURRENT_SOURCE_DIR}/test_interpreter.c
)

# crypt() is referenced from interpreter.c (nanny password hashing).
# Reuse the crypt library detected by the top-level build when one is needed;
# on platforms where crypt() is provided by libc, no extra link library is
# required.
if(CRYPT_LIBRARY)
    target_link_libraries(test_interpreter PRIVATE ${CRYPT_LIBRARY})
endif()

# ---------------------------------------------------------------------------
# test_class — covers src/class.c
# ---------------------------------------------------------------------------
add_mud_test(test_class
    ${MUD_SRCDIR}/class.c
    ${MUD_SRCDIR}/utils.c
    ${MUD_SRCDIR}/random.c
    ${CMAKE_CURRENT_SOURCE_DIR}/test_class.c
)

# ---------------------------------------------------------------------------
# test_ibt — covers the IBT_DATA structure in src/ibt.h
# ---------------------------------------------------------------------------
add_mud_test(test_ibt
    ${CMAKE_CURRENT_SOURCE_DIR}/test_ibt.c
)

# ---------------------------------------------------------------------------
# test_lists — covers src/lists.c
# ---------------------------------------------------------------------------
add_mud_test(test_lists
    ${MUD_SRCDIR}/lists.c
    ${MUD_SRCDIR}/utils.c
    ${MUD_SRCDIR}/random.c
    ${CMAKE_CURRENT_SOURCE_DIR}/test_lists.c
)

# ---------------------------------------------------------------------------
# test_handler — covers innate-affect handling in src/handler.c
# ---------------------------------------------------------------------------
add_mud_test(test_handler
    ${MUD_SRCDIR}/handler.c
    ${MUD_SRCDIR}/lists.c
    ${MUD_SRCDIR}/utils.c
    ${MUD_SRCDIR}/random.c
    ${CMAKE_CURRENT_SOURCE_DIR}/test_handler.c
)

# ---------------------------------------------------------------------------
# test_mud_event — covers char_has_mud_event() in src/mud_event.c
# ---------------------------------------------------------------------------
add_mud_test(test_mud_event
    ${MUD_SRCDIR}/mud_event.c
    ${MUD_SRCDIR}/dg_event.c
    ${MUD_SRCDIR}/lists.c
    ${MUD_SRCDIR}/utils.c
    ${MUD_SRCDIR}/random.c
    ${CMAKE_CURRENT_SOURCE_DIR}/test_mud_event.c
)

add_mud_test(test_obj_walk
    ${MUD_SRCDIR}/handler.c
    ${MUD_SRCDIR}/lists.c
    ${MUD_SRCDIR}/utils.c
    ${MUD_SRCDIR}/random.c
    ${CMAKE_CURRENT_SOURCE_DIR}/test_obj_walk.c
)

# ---------------------------------------------------------------------------
# test_handler_prefix — covers the keyword helpers in src/handler.c
# interpreter.c is here for one_argument, lists.c for the group list handler.c uses.
# ---------------------------------------------------------------------------
add_mud_test(test_handler_prefix
    ${MUD_SRCDIR}/handler.c
    ${MUD_SRCDIR}/interpreter.c
    ${MUD_SRCDIR}/lists.c
    ${MUD_SRCDIR}/utils.c
    ${MUD_SRCDIR}/random.c
    ${CMAKE_CURRENT_SOURCE_DIR}/test_handler_prefix.c
)

# interpreter.c brings crypt() along with it, as above.
if(CRYPT_LIBRARY)
    target_link_libraries(test_handler_prefix PRIVATE ${CRYPT_LIBRARY})
endif()
# test_improved_edit — covers format_text() and replace_str() in src/improved-edit.c
# ---------------------------------------------------------------------------
add_mud_test(test_improved_edit
    ${MUD_SRCDIR}/improved-edit.c
    ${MUD_SRCDIR}/utils.c
    ${MUD_SRCDIR}/random.c
    ${CMAKE_CURRENT_SOURCE_DIR}/test_improved_edit.c
)

# These fixtures use section garbage collection to link only the paths under
# test without unrelated game or OLC editor functions. MSVC resolves those
# discarded functions' dependencies before elimination, so it cannot link
# these targets.
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang" AND NOT MSVC)
    # test_flight.c includes act.movement.c and links act.item.c + interpreter.c
    # with section GC to discard unrelated command handlers.
    add_mud_test(test_flight
        ${MUD_SRCDIR}/act.item.c
        ${MUD_SRCDIR}/interpreter.c
        ${CMAKE_CURRENT_SOURCE_DIR}/test_flight.c
    )
    target_compile_options(test_flight PRIVATE -ffunction-sections -fdata-sections)
    if(APPLE)
        set_property(TARGET test_flight APPEND_STRING PROPERTY LINK_FLAGS " -Wl,-dead_strip")
    else()
        set_property(TARGET test_flight APPEND_STRING PROPERTY LINK_FLAGS " -Wl,--gc-sections")
    endif()
    if(CRYPT_LIBRARY)
        target_link_libraries(test_flight PRIVATE ${CRYPT_LIBRARY})
    endif()

    # test_shop.c includes shop.c to reach the private list and identify
    # commands, with section GC to discard the rest of the shop code.
    add_mud_test(test_shop
        ${MUD_SRCDIR}/utils.c
        ${MUD_SRCDIR}/random.c
        ${MUD_SRCDIR}/handler.c
        ${MUD_SRCDIR}/lists.c
        ${MUD_SRCDIR}/interpreter.c
        ${MUD_SRCDIR}/constants.c
        ${CMAKE_CURRENT_SOURCE_DIR}/test_shop.c
    )
    target_compile_options(test_shop PRIVATE -ffunction-sections -fdata-sections)
    if(APPLE)
        set_property(TARGET test_shop APPEND_STRING PROPERTY LINK_FLAGS " -Wl,-dead_strip")
    else()
        set_property(TARGET test_shop APPEND_STRING PROPERTY LINK_FLAGS " -Wl,--gc-sections")
    endif()
    if(CRYPT_LIBRARY)
        target_link_libraries(test_shop PRIVATE ${CRYPT_LIBRARY})
    endif()

    add_executable(test_editor_lifecycle
        ${UNITY_SRC}
        ${STUBS_SRC}
        ${MUD_SRCDIR}/utils.c
        ${MUD_SRCDIR}/random.c
        ${MUD_SRCDIR}/lists.c
        ${MUD_SRCDIR}/handler.c
        ${MUD_SRCDIR}/interpreter.c
        ${MUD_SRCDIR}/act.comm.c
        ${MUD_SRCDIR}/act.wizard.c
        ${MUD_SRCDIR}/limits.c
        ${MUD_SRCDIR}/modify.c
        ${MUD_SRCDIR}/improved-edit.c
        ${MUD_SRCDIR}/ibt.c
        ${MUD_SRCDIR}/genolc.c
        ${MUD_SRCDIR}/genobj.c
        ${MUD_SRCDIR}/oasis.c
        ${MUD_SRCDIR}/tedit.c
        ${MUD_SRCDIR}/prefedit.c
        ${CMAKE_CURRENT_SOURCE_DIR}/test_editor_lifecycle.c
    )
    target_include_directories(test_editor_lifecycle PRIVATE ${TEST_INCLUDES})
    target_compile_options(test_editor_lifecycle PRIVATE ${TEST_CFLAGS}
        -ffunction-sections -fdata-sections)
    if(APPLE)
        set_property(TARGET test_editor_lifecycle APPEND_STRING PROPERTY LINK_FLAGS " -Wl,-dead_strip")
    else()
        set_property(TARGET test_editor_lifecycle APPEND_STRING PROPERTY LINK_FLAGS " -Wl,--gc-sections")
    endif()
    if(CRYPT_LIBRARY)
        target_link_libraries(test_editor_lifecycle PRIVATE ${CRYPT_LIBRARY})
    endif()
    add_test(NAME test_editor_lifecycle COMMAND test_editor_lifecycle)

    # Rescue eligibility in quest.c, combat in fight.c, and the treatments that
    # finish a rescue a dying captive held up.  interpreter.c supplies the
    # real one_argument() for "bandage <name>".  Section GC drops the quest
    # commands and the rest of magic.c, act.offensive.c and interpreter.c.
    add_executable(test_quest
        ${UNITY_SRC}
        ${STUBS_SRC}
        ${MUD_SRCDIR}/utils.c
        ${MUD_SRCDIR}/random.c
        ${MUD_SRCDIR}/lists.c
        ${MUD_SRCDIR}/handler.c
        ${MUD_SRCDIR}/quest.c
        ${MUD_SRCDIR}/fight.c
        ${MUD_SRCDIR}/constants.c
        ${MUD_SRCDIR}/magic.c
        ${MUD_SRCDIR}/spell_parser.c
        ${MUD_SRCDIR}/dg_misc.c
        ${MUD_SRCDIR}/act.movement.c
        ${MUD_SRCDIR}/act.offensive.c
        ${MUD_SRCDIR}/interpreter.c
        ${MUD_SRCDIR}/dg_mobcmd.c
        ${CMAKE_CURRENT_SOURCE_DIR}/test_quest.c
    )
    target_include_directories(test_quest PRIVATE ${TEST_INCLUDES})
    target_compile_options(test_quest PRIVATE ${TEST_CFLAGS}
        -ffunction-sections -fdata-sections)
    if(APPLE)
        set_property(TARGET test_quest APPEND_STRING PROPERTY LINK_FLAGS " -Wl,-dead_strip")
    else()
        set_property(TARGET test_quest APPEND_STRING PROPERTY LINK_FLAGS " -Wl,--gc-sections")
    endif()
    if(CRYPT_LIBRARY)
        target_link_libraries(test_quest PRIVATE ${CRYPT_LIBRARY})
    endif()
    add_test(NAME test_quest COMMAND test_quest)

    add_executable(test_genolc_savers
        ${UNITY_SRC}
        ${CMAKE_CURRENT_SOURCE_DIR}/test_genolc_savers.c
        ${MUD_SRCDIR}/genmob.c
        ${MUD_SRCDIR}/genobj.c
        ${MUD_SRCDIR}/genwld.c
        ${MUD_SRCDIR}/genolc.c
        ${MUD_SRCDIR}/genzon.c
    )
    target_include_directories(test_genolc_savers PRIVATE ${TEST_INCLUDES})
    target_compile_options(test_genolc_savers PRIVATE
        -ffunction-sections -fdata-sections)
    if(APPLE)
        set_property(TARGET test_genolc_savers APPEND_STRING PROPERTY LINK_FLAGS " -Wl,-dead_strip")
    else()
        set_property(TARGET test_genolc_savers APPEND_STRING PROPERTY LINK_FLAGS " -Wl,--gc-sections")
    endif()
    add_test(NAME test_genolc_savers COMMAND test_genolc_savers)

    if(UNIX)
        add_mud_test(test_house_save
            ${MUD_SRCDIR}/utils.c
            ${MUD_SRCDIR}/random.c
            ${MUD_SRCDIR}/house.c
            ${MUD_SRCDIR}/genolc.c
            ${MUD_SRCDIR}/constants.c
            ${MUD_SRCDIR}/interpreter.c
            ${CMAKE_CURRENT_SOURCE_DIR}/test_house_save.c
        )
        target_compile_options(test_house_save PRIVATE -ffunction-sections -fdata-sections)
        if(APPLE)
            set_property(TARGET test_house_save APPEND_STRING PROPERTY LINK_FLAGS " -Wl,-dead_strip")
        else()
            set_property(TARGET test_house_save APPEND_STRING PROPERTY LINK_FLAGS " -Wl,--gc-sections")
        endif()
        if(CRYPT_LIBRARY)
            target_link_libraries(test_house_save PRIVATE ${CRYPT_LIBRARY})
        endif()

        # test_mail.c includes mail.c to reach postmaster_receive_mail() and wrap
        # genolc_install_file(), with section GC to discard unrelated genolc code.
        add_mud_test(test_mail
            ${MUD_SRCDIR}/utils.c
            ${MUD_SRCDIR}/random.c
            ${MUD_SRCDIR}/genolc.c
            ${CMAKE_CURRENT_SOURCE_DIR}/test_mail.c
        )
        target_compile_options(test_mail PRIVATE -ffunction-sections -fdata-sections)
        if(APPLE)
            set_property(TARGET test_mail APPEND_STRING PROPERTY LINK_FLAGS " -Wl,-dead_strip")
        else()
            set_property(TARGET test_mail APPEND_STRING PROPERTY LINK_FLAGS " -Wl,--gc-sections")
        endif()

        add_mud_test(test_players
            ${MUD_SRCDIR}/utils.c
            ${MUD_SRCDIR}/random.c
            ${MUD_SRCDIR}/players.c
            ${CMAKE_CURRENT_SOURCE_DIR}/test_players.c
        )
        target_compile_options(test_players PRIVATE -ffunction-sections -fdata-sections)
        if(APPLE)
            set_property(TARGET test_players APPEND_STRING PROPERTY LINK_FLAGS " -Wl,-dead_strip")
        else()
            set_property(TARGET test_players APPEND_STRING PROPERTY LINK_FLAGS " -Wl,--gc-sections")
        endif()
    endif()
endif()
