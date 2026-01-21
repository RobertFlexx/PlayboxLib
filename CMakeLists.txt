cmake_minimum_required(VERSION 3.16)
project(PlayboxLib VERSION 1.0.0 LANGUAGES C CXX)

include(GNUInstallDirs)

option(PLAYBOXLIB_BUILD_EXAMPLES "Build example games" ON)
option(PLAYBOXLIB_BUILD_CPP "Build C++ wrapper" ON)
option(PLAYBOXLIB_INSTALL "Install headers/libs" ON)
option(PLAYBOXLIB_WERROR "Treat warnings as errors" OFF)
option(PLAYBOXLIB_ENABLE_REPLAY "Enable record/replay support" ON)

set(PLAYBOXLIB_RENDERER "ansi" CACHE STRING "Renderer backend (ansi)")
set_property(CACHE PLAYBOXLIB_RENDERER PROPERTY STRINGS ansi)

set(PLAYBOXLIB_INPUT_IMPL "posix" CACHE STRING "Input backend (posix or default)")
set_property(CACHE PLAYBOXLIB_INPUT_IMPL PROPERTY STRINGS posix default)

set(PLAYBOXLIB_TERM_IMPL "posix" CACHE STRING "Terminal backend (posix)")
set_property(CACHE PLAYBOXLIB_TERM_IMPL PROPERTY STRINGS posix)

message(STATUS "PlayboxLib ${PROJECT_VERSION}")
message(STATUS "  BUILD_SHARED_LIBS         = ${BUILD_SHARED_LIBS}")
message(STATUS "  PLAYBOXLIB_BUILD_CPP      = ${PLAYBOXLIB_BUILD_CPP}")
message(STATUS "  PLAYBOXLIB_BUILD_EXAMPLES = ${PLAYBOXLIB_BUILD_EXAMPLES}")
message(STATUS "  PLAYBOXLIB_WERROR         = ${PLAYBOXLIB_WERROR}")
message(STATUS "  PLAYBOXLIB_INSTALL        = ${PLAYBOXLIB_INSTALL}")
message(STATUS "  PLAYBOXLIB_ENABLE_REPLAY  = ${PLAYBOXLIB_ENABLE_REPLAY}")
message(STATUS "  PLAYBOXLIB_RENDERER       = ${PLAYBOXLIB_RENDERER}")
message(STATUS "  PLAYBOXLIB_INPUT_IMPL     = ${PLAYBOXLIB_INPUT_IMPL}")
message(STATUS "  PLAYBOXLIB_TERM_IMPL      = ${PLAYBOXLIB_TERM_IMPL}")

set(PLAYBOXLIB_SOURCES
    src/pb_utf8.c
    src/pb_fb.c
    src/pb_time.c
    src/pb_app.c
)

if(PLAYBOXLIB_RENDERER STREQUAL "ansi")
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/pb_renderer_ansi.c")
        list(APPEND PLAYBOXLIB_SOURCES src/pb_renderer_ansi.c)
    elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/pb_renderer.c")
        list(APPEND PLAYBOXLIB_SOURCES src/pb_renderer.c)
    else()
        message(FATAL_ERROR "Renderer set to ansi but no renderer source found (src/pb_renderer_ansi.c or src/pb_renderer.c)")
    endif()
else()
    message(FATAL_ERROR "Unknown PLAYBOXLIB_RENDERER: ${PLAYBOXLIB_RENDERER}")
endif()

if(PLAYBOXLIB_TERM_IMPL STREQUAL "posix")
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/pb_term_posix.c")
        list(APPEND PLAYBOXLIB_SOURCES src/pb_term_posix.c)
    else()
        message(FATAL_ERROR "Term impl set to posix but src/pb_term_posix.c is missing")
    endif()
else()
    message(FATAL_ERROR "Unknown PLAYBOXLIB_TERM_IMPL: ${PLAYBOXLIB_TERM_IMPL}")
endif()

if(PLAYBOXLIB_INPUT_IMPL STREQUAL "posix")
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/pb_input_posix.c")
        list(APPEND PLAYBOXLIB_SOURCES src/pb_input_posix.c)
    else()
        message(FATAL_ERROR "Input impl set to posix but src/pb_input_posix.c is missing")
    endif()
elseif(PLAYBOXLIB_INPUT_IMPL STREQUAL "default")
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/pb_input.c")
        list(APPEND PLAYBOXLIB_SOURCES src/pb_input.c)
    elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/pb_input_posix.c")
        list(APPEND PLAYBOXLIB_SOURCES src/pb_input_posix.c)
    else()
        message(FATAL_ERROR "Input impl set to default but no input source found (src/pb_input.c or src/pb_input_posix.c)")
    endif()
else()
    message(FATAL_ERROR "Unknown PLAYBOXLIB_INPUT_IMPL: ${PLAYBOXLIB_INPUT_IMPL}")
endif()

if(PLAYBOXLIB_ENABLE_REPLAY)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/pb_replay.c")
        list(APPEND PLAYBOXLIB_SOURCES src/pb_replay.c)
        add_compile_definitions(PB_ENABLE_REPLAY=1)
    else()
        message(FATAL_ERROR "PLAYBOXLIB_ENABLE_REPLAY=ON but src/pb_replay.c is missing")
    endif()
else()
    add_compile_definitions(PB_ENABLE_REPLAY=0)
endif()

if(PLAYBOXLIB_BUILD_CPP)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cpp/playbox.cpp")
        list(APPEND PLAYBOXLIB_SOURCES cpp/playbox.cpp)
    else()
        message(FATAL_ERROR "PLAYBOXLIB_BUILD_CPP=ON but cpp/playbox.cpp is missing")
    endif()
endif()

add_library(playboxlib ${PLAYBOXLIB_SOURCES})

target_include_directories(playboxlib
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/cpp
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

set_target_properties(playboxlib PROPERTIES
    C_STANDARD 11
    C_STANDARD_REQUIRED YES
    C_EXTENSIONS NO
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
)

if(MSVC)
    target_compile_options(playboxlib PRIVATE /W4)
    if(PLAYBOXLIB_WERROR)
        target_compile_options(playboxlib PRIVATE /WX)
    endif()
else()
    target_compile_options(playboxlib PRIVATE -Wall -Wextra -Wpedantic)
    if(PLAYBOXLIB_WERROR)
        target_compile_options(playboxlib PRIVATE -Werror)
    endif()
    target_compile_options(playboxlib PRIVATE -fvisibility=hidden)
endif()

if(PLAYBOXLIB_BUILD_EXAMPLES)
    add_executable(pb_demo examples/demo.c)
    target_link_libraries(pb_demo PRIVATE playboxlib)

    add_executable(pb_tetris examples/tetris.c)
    target_link_libraries(pb_tetris PRIVATE playboxlib)

    add_executable(pb_sandbox examples/sandbox.c)
    target_link_libraries(pb_sandbox PRIVATE playboxlib)

    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/examples/pb_snake.c")
        add_executable(pb_snake examples/pb_snake.c)
        target_link_libraries(pb_snake PRIVATE playboxlib)
    endif()
endif()

if(PLAYBOXLIB_INSTALL)
    install(TARGETS playboxlib
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
    install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
    if(PLAYBOXLIB_BUILD_CPP AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cpp/playbox.hpp")
        install(FILES cpp/playbox.hpp DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/playbox)
    endif()
endif()
