/*
 * helixtools-service.c - Plugin entry point for the Rumble service
 *
 * Copyright (C) 2025 HelixTools
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

#include "helixtools-service.h"

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("helixtools-service", "en-US")

MODULE_EXPORT const char *obs_module_description(void)
{
	return "Native Rumble RTMP streaming service (HelixTools)";
}

bool obs_module_load(void)
{
	obs_register_service(&helixtools_rumble_service_info);
	helixtools_service_init_menu();
	blog(LOG_INFO, "[helixtools-service] Plugin loaded (v%s)",
	     HELIXTOOLS_SERVICE_VERSION);
	return true;
}

void obs_module_unload(void)
{
	blog(LOG_INFO, "[helixtools-service] Plugin unloaded");
}
