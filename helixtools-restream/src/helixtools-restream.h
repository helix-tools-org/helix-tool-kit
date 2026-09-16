/*
 * helixtools-restream.h - HelixTools Multi-RTMP Restream Plugin (standalone)
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
#include <obs-frontend-api.h>

#ifndef HELIXTOOLS_RESTREAM_VERSION
#define HELIXTOOLS_RESTREAM_VERSION "0.1.0"
#endif
#define HELIXTOOLS_RESTREAM_MAX_DESTINATIONS 8

/* A single restream destination */
struct restream_destination {
	char *name;
	char *rtmp_url;
	char *stream_key;
	bool enabled;
	bool active;

	obs_service_t *service;
	obs_output_t *output;
	obs_encoder_t *video_encoder; /* NULL = share main encoder */
	obs_encoder_t *audio_encoder; /* NULL = share main encoder */
};

/* Plugin-wide state */
struct restream_context {
	struct restream_destination destinations[HELIXTOOLS_RESTREAM_MAX_DESTINATIONS];
	int destination_count;
	bool streaming;

	/* Optional user-set label for the main stream shown in the dock.
	 * NULL means "auto-detect from the configured stream service". */
	char *main_stream_label;
};

/* restream-output.c */
void helixtools_restream_start_all(struct restream_context *ctx);
void helixtools_restream_stop_all(struct restream_context *ctx);
void helixtools_restream_load_config(struct restream_context *ctx,
				     obs_data_t *settings);
void helixtools_restream_save_config(struct restream_context *ctx,
				     obs_data_t *settings);
void helixtools_restream_free(struct restream_context *ctx);
