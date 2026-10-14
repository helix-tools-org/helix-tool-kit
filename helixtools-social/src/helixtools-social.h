/*
 * helixtools-social.h - HelixTools Social Media dock (standalone)
 *
 * Sidebar-based browser dock for quick access to streaming/social
 * platform dashboards.
 *
 * Copyright (C) 2026 HelixTools
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <obs-module.h>

#ifndef HELIXTOOLS_SOCIAL_VERSION
#define HELIXTOOLS_SOCIAL_VERSION "0.1.0"
#endif

#ifdef __cplusplus
extern "C" {
#endif
void helixtools_social_init_dock(void);
void helixtools_social_shutdown_dock(void);
#ifdef __cplusplus
}
#endif
