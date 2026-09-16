# helixtools-service: Rumble RTMP streaming service plugin for vanilla OBS Studio
# Copyright (C) 2025 HelixTools
# SPDX-License-Identifier: GPL-2.0-or-later

cmake_minimum_required(VERSION 3.28)
project(helixtools-service VERSION 0.1.0 LANGUAGES C CXX)

# --- Find OBS ---
# When building out-of-tree, set CMAKE_PREFIX_PATH to an OBS libobs SDK/rundir:
#   cmake -B build -DCMAKE_PREFIX_PATH="path/to/obs-studio/build_x64/rundir/RelWithDebInfo"
find_package(libobs REQUIRED)
find_package(obs-frontend-api REQUIRED)
find_package(Qt6 REQUIRED COMPONENTS Widgets)

# Standalone version string — does NOT depend on any fork-specific
# obsconfig.h macro, so this plugin builds against a vanilla OBS SDK.
add_compile_definitions(HELIXTOOLS_SERVICE_VERSION="${PROJECT_VERSION}")

# --- Plugin target ---
add_library(helixtools-service MODULE
    src/helixtools-service.c
    src/rumble-service.c
    src/service-menu.cpp
)

target_link_libraries(helixtools-service PRIVATE
    OBS::libobs
    OBS::obs-frontend-api
    Qt6::Widgets
)

target_include_directories(helixtools-service PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

set_target_properties(helixtools-service PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    AUTOMOC ON
)

# --- Install ---
# Install into OBS plugin directory structure
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/rundir" CACHE PATH "" FORCE)
endif()

install(TARGETS helixtools-service
    LIBRARY DESTINATION obs-plugins/64bit
    RUNTIME DESTINATION obs-plugins/64bit
)
