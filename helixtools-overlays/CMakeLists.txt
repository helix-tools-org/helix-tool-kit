# helixtools-overlays: HelixTools Hub overlay source plugin for vanilla OBS Studio
# Copyright (C) 2026 HelixTools
# SPDX-License-Identifier: GPL-2.0-or-later

cmake_minimum_required(VERSION 3.28)
project(helixtools-overlays VERSION 0.1.0 LANGUAGES C)

find_package(libobs REQUIRED)

add_compile_definitions(HELIXTOOLS_OVERLAYS_VERSION="${PROJECT_VERSION}")

add_library(helixtools-overlays MODULE
    src/helixtools-overlays.c
    src/overlay-source.c
)

target_link_libraries(helixtools-overlays PRIVATE
    OBS::libobs
    advapi32
)

target_include_directories(helixtools-overlays PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/rundir" CACHE PATH "" FORCE)
endif()

install(TARGETS helixtools-overlays
    LIBRARY DESTINATION obs-plugins/64bit
    RUNTIME DESTINATION obs-plugins/64bit
)
