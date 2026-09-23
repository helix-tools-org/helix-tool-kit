# helixtools-dashboard: single dock for one selectable platform dashboard
# Copyright (C) 2026 HelixTools
# SPDX-License-Identifier: GPL-2.0-or-later

cmake_minimum_required(VERSION 3.28)
project(helixtools-dashboard VERSION 0.2.0 LANGUAGES C CXX)

find_package(libobs REQUIRED)
find_package(obs-frontend-api REQUIRED)
find_package(Qt6 REQUIRED COMPONENTS Core Widgets)

add_compile_definitions(HELIXTOOLS_DASHBOARD_VERSION="${PROJECT_VERSION}")

add_library(helixtools-dashboard MODULE
    src/helixtools-dashboard.cpp
    src/dashboard-dock.cpp
)

target_link_libraries(helixtools-dashboard PRIVATE
    OBS::libobs
    OBS::obs-frontend-api
    Qt6::Core
    Qt6::Widgets
)

target_include_directories(helixtools-dashboard PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

set_target_properties(helixtools-dashboard PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    AUTOMOC ON
)

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/rundir" CACHE PATH "" FORCE)
endif()

install(TARGETS helixtools-dashboard
    LIBRARY DESTINATION obs-plugins/64bit
    RUNTIME DESTINATION obs-plugins/64bit
)

install(DIRECTORY data/
    DESTINATION data/obs-plugins/helixtools-dashboard
    OPTIONAL
)
