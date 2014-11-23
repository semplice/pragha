/*************************************************************************/
/* Copyright (C) 2009-2013 matias <mati86dl@gmail.com>                   */
/*                                                                       */
/* This program is free software: you can redistribute it and/or modify  */
/* it under the terms of the GNU General Public License as published by  */
/* the Free Software Foundation, either version 3 of the License, or     */
/* (at your option) any later version.                                   */
/*                                                                       */
/* This program is distributed in the hope that it will be useful,       */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of        */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         */
/* GNU General Public License for more details.                          */
/*                                                                       */
/* You should have received a copy of the GNU General Public License     */
/* along with this program.  If not, see <http://www.gnu.org/licenses/>. */
/*************************************************************************/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib.h>
#include <glib-object.h>
#include <gmodule.h>
#include <gtk/gtk.h>
#include <keybinder.h>

#include <libpeas/peas.h>
#include <libpeas-gtk/peas-gtk.h>

#include "pragha-keybinder-plugin.h"

#include "src/pragha.h"
#include "src/pragha-playback.h"
#include "src/pragha-window.h"

#include "plugins/pragha-plugin-macros.h"

PRAGHA_PLUGIN_REGISTER (PRAGHA_TYPE_KEYBINDER_PLUGIN,
                        PraghaKeybinderPlugin,
                        pragha_keybinder_plugin)

static void
keybind_prev_handler (const char *keystring, gpointer data)
{
	PraghaBackend *backend;
	PraghaApplication *pragha = data;

	backend = pragha_application_get_backend (pragha);

	if (pragha_backend_emitted_error (backend) == FALSE)
		pragha_playback_prev_track(pragha);
}

static void
keybind_play_handler (const char *keystring, gpointer data)
{
	PraghaBackend *backend;
	PraghaApplication *pragha = data;

	backend = pragha_application_get_backend (pragha);

	if (pragha_backend_emitted_error (backend) == FALSE)
		pragha_playback_play_pause_resume(pragha);
}

static void
keybind_stop_handler (const char *keystring, gpointer data)
{
	PraghaBackend *backend;
	PraghaApplication *pragha = data;

	backend = pragha_application_get_backend (pragha);

	if (pragha_backend_emitted_error (backend) == FALSE)
		pragha_playback_stop(pragha);
}

static void
keybind_next_handler (const char *keystring, gpointer data)
{
	PraghaBackend *backend;
	PraghaApplication *pragha = data;

	backend = pragha_application_get_backend (pragha);

	if (pragha_backend_emitted_error (backend) == FALSE)
		pragha_playback_next_track(pragha);
}

static void
keybind_media_handler (const char *keystring, gpointer data)
{
	PraghaApplication *pragha = data;

	pragha_window_toggle_state (pragha, FALSE);
}

static void
pragha_plugin_activate (PeasActivatable *activatable)
{
	PraghaKeybinderPlugin *plugin = PRAGHA_KEYBINDER_PLUGIN (activatable);

	PraghaKeybinderPluginPrivate *priv = plugin->priv;
	priv->pragha = g_object_get_data (G_OBJECT (plugin), "object");

	keybinder_init ();

	CDEBUG(DBG_PLUGIN, "Keybinder plugin %s", G_STRFUNC);

	keybinder_bind("XF86AudioPlay", (KeybinderHandler) keybind_play_handler, priv->pragha);
	keybinder_bind("XF86AudioStop", (KeybinderHandler) keybind_stop_handler, priv->pragha);
	keybinder_bind("XF86AudioPrev", (KeybinderHandler) keybind_prev_handler, priv->pragha);
	keybinder_bind("XF86AudioNext", (KeybinderHandler) keybind_next_handler, priv->pragha);
	keybinder_bind("XF86AudioMedia", (KeybinderHandler) keybind_media_handler, priv->pragha);
}

static void
pragha_plugin_deactivate (PeasActivatable *activatable)
{
	CDEBUG(DBG_PLUGIN, "Keybinder plugin %s", G_STRFUNC);

	keybinder_unbind("XF86AudioPlay", (KeybinderHandler) keybind_play_handler);
	keybinder_unbind("XF86AudioStop", (KeybinderHandler) keybind_stop_handler);
	keybinder_unbind("XF86AudioPrev", (KeybinderHandler) keybind_prev_handler);
	keybinder_unbind("XF86AudioNext", (KeybinderHandler) keybind_next_handler);
	keybinder_unbind("XF86AudioMedia", (KeybinderHandler) keybind_media_handler);
}