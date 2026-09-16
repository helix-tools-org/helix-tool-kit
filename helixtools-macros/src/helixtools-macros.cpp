/*
 * helixtools-macros.cpp - Plugin entry point for HelixTools macros
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

#include "helixtools-macros.h"
#include "macro-editor-dialog.h"

#include <QAction>
#include <QMainWindow>

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("helixtools-macros", "en-US")

MODULE_EXPORT const char *obs_module_description(void)
{
	return "Native macro system for HelixTools";
}

static MacroEditorDialog *editor_dialog = nullptr;

bool obs_module_load(void)
{
	/* Load saved macros from disk */
	helixtools_macros_load();

	/* Register hotkeys for all macros */
	helixtools_macros_register_hotkeys();

	/* Create the editor dialog (pattern from obs-websocket.cpp) */
	obs_frontend_push_ui_translation(obs_module_get_string);
	QMainWindow *mainWindow =
		static_cast<QMainWindow *>(obs_frontend_get_main_window());
	editor_dialog = new MacroEditorDialog(mainWindow);
	obs_frontend_pop_ui_translation();

	/* Add "HelixTools Macros" to the Tools menu */
	const char *menuText =
		obs_module_text("HelixToolsMacros.MenuTitle");
	QAction *menuAction =
		(QAction *)obs_frontend_add_tools_menu_qaction(menuText);
	QObject::connect(menuAction, &QAction::triggered,
			 [=] { editor_dialog->ToggleShowHide(); });

	blog(LOG_INFO, "[helixtools-macros] Plugin loaded (v%s)",
	     HELIXTOOLS_MACROS_VERSION);
	return true;
}

void obs_module_post_load(void)
{
	/* Vendor registration MUST happen here per obs-websocket-api.h */
	helixtools_macros_vendor_init();

	/* Initialize Hub event trigger stubs */
	helixtools_macros_hub_triggers_init();
}

void obs_module_unload(void)
{
	/* Shut down engine: cancel macros and wait for threads */
	helixtools_macros_engine_shutdown();

	/* Clean up hotkeys */
	helixtools_macros_unregister_hotkeys();

	/* Clean up vendor requests */
	helixtools_macros_vendor_shutdown();

	/* Clean up Hub event triggers */
	helixtools_macros_hub_triggers_shutdown();

	blog(LOG_INFO, "[helixtools-macros] Plugin unloaded");
}
