/*
 * restream-storage.h - Disk persistence for restream destinations
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

#include "helixtools-restream.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Load destinations.json from the plugin's config directory into ctx.
 * Safe to call with an empty/missing file (leaves ctx with 0 destinations). */
void helixtools_restream_load(struct restream_context *ctx);

/* Save ctx's current destinations to destinations.json in the plugin's
 * config directory, creating the directory if needed. */
void helixtools_restream_save(struct restream_context *ctx);

#ifdef __cplusplus
}
#endif
