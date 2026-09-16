# helixtools-restream: Multi-RTMP restreaming output plugin for vanilla OBS Studio
# Copyright (C) 2026 HelixTools
# SPDX-License-Identifier: GPL-2.0-or-later

cmake_minimum_required(VERSION 3.28)
project(helixtools-restream VERSION 0.5.0 LANGUAGES C CXX)

find_package(libobs REQUIRED)
find_package(obs-frontend-api REQUIRED)

# Qt 6 required for the destinations editor dialog
find_package(Qt6 REQUIRED COMPONENTS Core Widgets)

add_compile_definitions(HELIXTOOLS_RESTREAM_VERSION="${PROJECT_VERSION}")

add_library(helixtools-restream MODULE
    src/helixtools-restream.c
    src/restream-output.c
    src/restream-storage.c
    src/restream-dock.cpp
    src/restream-dock-init.cpp
)

target_link_libraries(helixtools-restream PRIVATE
    OBS::libobs
    OBS::obs-frontend-api
    Qt6::Core
    Qt6::Widgets
)

target_include_directories(helixtools-restream PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

set_target_properties(helixtools-restream PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    AUTOMOC ON
)

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/rundir" CACHE PATH "" FORCE)
endif()

install(TARGETS helixtools-restream
    LIBRARY DESTINATION obs-plugins/64bit
    RUNTIME DESTINATION obs-plugins/64bit
)
