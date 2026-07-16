cmake_minimum_required(VERSION 3.16)
project(SpaceCadetPinballPSP)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/bin)

# --- Source files ---
file(GLOB_RECURSE GAME_SOURCE_FILES
    ${CMAKE_CURRENT_LIST_DIR}/src/game/*.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/game/*.h
)

# Add PSP-specific source files
list(APPEND GAME_SOURCE_FILES
    ${CMAKE_CURRENT_LIST_DIR}/src/psp_input.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/psp_input.h
)

add_executable(${PROJECT_NAME} ${GAME_SOURCE_FILES})

# Precompiled headers
if(${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.16.0")
    target_precompile_headers(${PROJECT_NAME} PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/src/game/pch.h
    )
endif()

# SDL2 via pkg-config (pspsdk provides SDL2)
include(FindPkgConfig)
pkg_search_module(SDL2 REQUIRED sdl2)
pkg_search_module(SDL2_MIXER REQUIRED SDL2_mixer)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/src/game
    ${CMAKE_CURRENT_LIST_DIR}/src
    ${SDL2_INCLUDE_DIRS}
    ${SDL2_MIXER_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME}
    ${SDL2_LIBRARIES}
    ${SDL2_MIXER_LIBRARIES}
    pspdebug
    pspdisplay
    pspge
    pspctrl
    psputility
    pspaudio
    m
)

target_compile_options(${PROJECT_NAME} PRIVATE
    -O2
    -Wall
    -Wno-sign-compare
    -Wno-unused-variable
)

# Create EBOOT.PBP
create_pbp_file(
    TARGET ${PROJECT_NAME}
    ICON_PATH NULL
    BACKGROUND_PATH NULL
    PREVIEW_PATH NULL
    TITLE "Space Cadet Pinball"
    VERSION 01.00
)
