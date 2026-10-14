# helixtools-macros: Native macro system for HelixTools
# Copyright (C) 2026 HelixTools
# SPDX-License-Identifier: GPL-2.0-or-later

cmake_minimum_required(VERSION 3.28)
project(helixtools-macros VERSION 0.1.0 LANGUAGES CXX)

find_package(libobs REQUIRED)
find_package(obs-frontend-api REQUIRED)

# Qt 6 required for macro editor dialog
find_package(Qt6 REQUIRED COMPONENTS Core Widgets)

add_compile_definitions(HELIXTOOLS_MACROS_VERSION="${PROJECT_VERSION}")

add_library(helixtools-macros MODULE
    src/helixtools-macros.cpp
    src/macro-engine.cpp
    src/macro-storage.cpp
    src/macro-editor-dialog.cpp
    src/macro-action-widget.cpp
    src/macro-vendor.cpp
    src/macro-hub-triggers.cpp
)

target_link_libraries(helixtools-macros PRIVATE
    OBS::libobs
    OBS::obs-frontend-api
    Qt6::Core
    Qt6::Widgets
)

target_include_directories(helixtools-macros PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    # obs-websocket vendor API (obs-websocket-api.h), vendored locally so
    # this plugin only needs OBS's public SDK, not obs-websocket's source tree
    ${CMAKE_CURRENT_SOURCE_DIR}/src/vendor
)

set_target_properties(helixtools-macros PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    AUTOMOC ON
)

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/rundir" CACHE PATH "" FORCE)
endif()

install(TARGETS helixtools-macros
    LIBRARY DESTINATION obs-plugins/64bit
    RUNTIME DESTINATION obs-plugins/64bit
)

install(DIRECTORY data/
    DESTINATION data/obs-plugins/helixtools-macros
    OPTIONAL
)
