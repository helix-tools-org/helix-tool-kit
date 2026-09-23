/*
 * music-credentials.h - Windows Credential Manager target name shared
 * between music-dock.cpp (writer) and music-manager.cpp (reader)
 *
 * Only the bare user ID is cached — see music-auth-script.h for why
 * no access/refresh token is captured or stored here at all.
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

static const wchar_t *MUSIC_CRED_USERID = L"HelixTools/MusicUserId";
