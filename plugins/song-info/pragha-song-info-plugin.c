/*************************************************************************/
/* Copyright (C) 2011-2014 matias <mati86dl@gmail.com>                   */
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

#if defined(GETTEXT_PACKAGE)
#include <glib/gi18n-lib.h>
#else
#include <glib/gi18n.h>
#endif

#include <glib.h>
#include <glib-object.h>
#include <gmodule.h>
#include <gtk/gtk.h>

#include <glyr/glyr.h>
#include <glyr/cache.h>

#include <glib/gstdio.h>

#include <libpeas/peas.h>
#include <libpeas-gtk/peas-gtk.h>

#include "plugins/pragha-plugin-macros.h"

#include "pragha-song-info-plugin.h"
#include "pragha-song-info-dialog.h"
#include "pragha-song-info-pane.h"
#include "pragha-song-info-thread-albumart.h"
#include "pragha-song-info-thread-dialog.h"
#include "pragha-song-info-thread-pane.h"

#include "src/pragha.h"
#include "src/pragha-hig.h"
#include "src/pragha-playback.h"
#include "src/pragha-sidebar.h"
#include "src/pragha-simple-async.h"
#include "src/pragha-simple-widgets.h"
#include "src/pragha-preferences-dialog.h"
#include "src/pragha-utils.h"

struct _PraghaSongInfoPluginPrivate {
	PraghaApplication  *pragha;
	GtkWidget          *setting_widget;

	PraghaSonginfoPane *pane;

	GlyrDatabase       *cache_db;

	gboolean            download_album_art;
	GtkWidget          *download_album_art_w;

	GtkActionGroup     *action_group_playlist;
	guint               merge_id_playlist;

	GCancellable       *pane_search;
};

PRAGHA_PLUGIN_REGISTER_PRIVATE_CODE (PRAGHA_TYPE_SONG_INFO_PLUGIN,
                                     PraghaSongInfoPlugin,
                                     pragha_song_info_plugin)

/*
 * Popups
 */

static void get_lyric_current_playlist_action       (GtkAction *action, PraghaSongInfoPlugin *plugin);
static void get_artist_info_current_playlist_action (GtkAction *action, PraghaSongInfoPlugin *plugin);

static const GtkActionEntry playlist_actions [] = {
	{"Search lyric", NULL, N_("Search _lyric"),
	 "", "Search lyric", G_CALLBACK(get_lyric_current_playlist_action)},
	{"Search artist info", "dialog-information", N_("Search _artist info"),
	 "", "Search artist info", G_CALLBACK(get_artist_info_current_playlist_action)},
};

static const gchar *playlist_xml = "<ui>						\
	<popup name=\"SelectionPopup\">		   				\
	<menu action=\"ToolsMenu\">							\
		<placeholder name=\"pragha-glyr-placeholder\">			\
			<menuitem action=\"Search lyric\"/>				\
			<menuitem action=\"Search artist info\"/>			\
			<separator/>							\
		</placeholder>								\
	</menu>										\
	</popup>				    						\
</ui>";

/*
 * Action on playlist that show a dialog
 */

static void
get_artist_info_current_playlist_action (GtkAction *action, PraghaSongInfoPlugin *plugin)
{
	PraghaPlaylist *playlist;
	PraghaMusicobject *mobj;
	const gchar *artist = NULL;

	PraghaApplication *pragha = NULL;

	pragha = plugin->priv->pragha;
	playlist = pragha_application_get_playlist (pragha);

	mobj = pragha_playlist_get_selected_musicobject (playlist);

	artist = pragha_musicobject_get_artist (mobj);

	CDEBUG(DBG_INFO, "Get Artist info Action of current playlist selection");

	if (string_is_empty(artist))
		return;

	pragha_songinfo_plugin_get_info_to_dialog (plugin, GLYR_GET_ARTISTBIO, artist, NULL);
}

static void
get_lyric_current_playlist_action (GtkAction *action, PraghaSongInfoPlugin *plugin)
{
	PraghaPlaylist *playlist;
	PraghaMusicobject *mobj;
	const gchar *artist = NULL;
	const gchar *title = NULL;

	PraghaApplication *pragha = NULL;
	pragha = plugin->priv->pragha;

	playlist = pragha_application_get_playlist (pragha);
	mobj = pragha_playlist_get_selected_musicobject (playlist);

	artist = pragha_musicobject_get_artist (mobj);
	title = pragha_musicobject_get_title (mobj);

	CDEBUG(DBG_INFO, "Get lyrics Action of current playlist selection.");

	if (string_is_empty(artist) || string_is_empty(title))
		return;

	pragha_songinfo_plugin_get_info_to_dialog (plugin, GLYR_GET_LYRICS, artist, title);
}

/*
 * Handlers depending on backend status
 */

static void
related_get_album_art_handler (PraghaSongInfoPlugin *plugin)
{
	PraghaBackend *backend;
	PraghaArtCache *art_cache;
	PraghaMusicobject *mobj;
	const gchar *artist = NULL;
	const gchar *album = NULL;
	gchar *album_art_path;

	CDEBUG(DBG_INFO, "Get album art handler");

	backend = pragha_application_get_backend (plugin->priv->pragha);
	if (pragha_backend_get_state (backend) == ST_STOPPED)
		return;

	mobj = pragha_backend_get_musicobject (backend);
	artist = pragha_musicobject_get_artist (mobj);
	album = pragha_musicobject_get_album (mobj);

	if (string_is_empty(artist) || string_is_empty(album))
		return;

	art_cache = pragha_application_get_art_cache (plugin->priv->pragha);
	album_art_path = pragha_art_cache_get_uri (art_cache, artist, album);

	if (album_art_path)
		goto exists;

	pragha_songinfo_plugin_get_album_art (plugin, artist, album);

exists:
	g_free(album_art_path);
}

static void
cancel_pane_search (PraghaSongInfoPlugin *plugin)
{
	PraghaSongInfoPluginPrivate *priv = plugin->priv;

	if (priv->pane_search) {
		g_cancellable_cancel (priv->pane_search);
		g_object_unref (priv->pane_search);
		priv->pane_search = NULL;
	}
}

static void
related_get_song_info_pane_handler (PraghaSongInfoPlugin *plugin)
{
	PraghaSongInfoPluginPrivate *priv = plugin->priv;
	PraghaBackend *backend;
	PraghaMusicobject *mobj;
	const gchar *artist = NULL;
	const gchar *title = NULL;
	const gchar *filename = NULL;

	CDEBUG (DBG_INFO, "Get song info handler");

	backend = pragha_application_get_backend (plugin->priv->pragha);
	if (pragha_backend_get_state (backend) == ST_STOPPED) {
		pragha_songinfo_pane_clear_text (plugin->priv->pane);
		return;
	}

	mobj = pragha_backend_get_musicobject (backend);
	artist = pragha_musicobject_get_artist (mobj);
	title = pragha_musicobject_get_title (mobj);
	filename = pragha_musicobject_get_file (mobj);

	if (string_is_empty(artist) || string_is_empty(title))
		return;

	cancel_pane_search (plugin);
	priv->pane_search = pragha_songinfo_plugin_get_info_to_pane (plugin, pragha_songinfo_pane_get_default_view(plugin->priv->pane), artist, title, filename);
}

static void
pragha_song_info_get_info (gpointer data)
{
	PraghaSongInfoPlugin *plugin = data;
	PraghaSongInfoPluginPrivate *priv = plugin->priv;

	if (priv->download_album_art)
		related_get_album_art_handler (plugin);

	if (gtk_widget_is_visible(GTK_WIDGET(priv->pane)))
		related_get_song_info_pane_handler (plugin);
}

static void
backend_changed_state_cb (PraghaBackend *backend, GParamSpec *pspec, gpointer user_data)
{
	PraghaMusicSource file_source = FILE_NONE;
	PraghaBackendState state = 0;

	PraghaSongInfoPlugin *plugin = user_data;

	cancel_pane_search (plugin);

	state = pragha_backend_get_state (backend);

	CDEBUG(DBG_INFO, "Configuring thread to get the cover art");

	if (state == ST_STOPPED)
		pragha_songinfo_pane_clear_text (plugin->priv->pane);

	if (state != ST_PLAYING)
		return;

	file_source = pragha_musicobject_get_source (pragha_backend_get_musicobject (backend));

	if (file_source == FILE_NONE) {
		pragha_songinfo_pane_clear_text (plugin->priv->pane);
		return;
	}
	
	pragha_song_info_get_info (plugin);
}

/*
 * Update handlers
 */

static void
pragha_songinfo_pane_type_changed (PraghaSonginfoPane *pane, PraghaSongInfoPlugin *plugin)
{
	related_get_song_info_pane_handler (plugin);
}

static void
pragha_songinfo_pane_visibility_changed (PraghaPreferences *preferences, GParamSpec *pspec, PraghaSongInfoPlugin *plugin)
{
	if (pragha_preferences_get_secondary_lateral_panel (preferences))
		related_get_song_info_pane_handler (plugin);
}

/*
 * Public api
 */

GlyrDatabase *
pragha_songinfo_plugin_get_cache (PraghaSongInfoPlugin *plugin)
{
	PraghaSongInfoPluginPrivate *priv = plugin->priv;

	return priv->cache_db;
}

PraghaSonginfoPane *
pragha_songinfo_plugin_get_pane (PraghaSongInfoPlugin *plugin)
{
	PraghaSongInfoPluginPrivate *priv = plugin->priv;

	return priv->pane;
}

PraghaApplication *
pragha_songinfo_plugin_get_application (PraghaSongInfoPlugin *plugin)
{
	PraghaSongInfoPluginPrivate *priv = plugin->priv;

	return priv->pragha;
}

/*
 * Preferences plugin
 */

static void
pragha_songinfo_preferences_dialog_response (GtkDialog            *dialog,
                                             gint                  response_id,
                                             PraghaSongInfoPlugin *plugin)
{
	PraghaPreferences *preferences;
	gchar *plugin_group = NULL;

	PraghaSongInfoPluginPrivate *priv = plugin->priv;

	switch(response_id) {
		case GTK_RESPONSE_CANCEL:
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(priv->download_album_art_w),
			                              priv->download_album_art);
			break;
		case GTK_RESPONSE_OK:
			priv->download_album_art =
				gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(priv->download_album_art_w));

			preferences = pragha_preferences_get ();
			plugin_group = pragha_preferences_get_plugin_group_name(preferences, "song-info");
			pragha_preferences_set_boolean (preferences,
			                                plugin_group, "DownloadAlbumArt",
			                                priv->download_album_art);
			g_object_unref (preferences);
			g_free (plugin_group);
			break;
		default:
			break;
	}
}

static void
pragha_songinfo_plugin_append_setting (PraghaSongInfoPlugin *plugin)
{
	PreferencesDialog *dialog;
	PraghaPreferences *preferences = NULL;
	gchar *plugin_group = NULL;
	GtkWidget *table, *download_album_art_w;
	guint row = 0;

	PraghaSongInfoPluginPrivate *priv = plugin->priv;

	table = pragha_hig_workarea_table_new ();

	pragha_hig_workarea_table_add_section_title(table, &row, _("Song Information"));

	download_album_art_w = gtk_check_button_new_with_label (_("Download the album art while playing their songs."));
	pragha_hig_workarea_table_add_wide_control (table, &row, download_album_art_w);

	preferences = pragha_preferences_get ();
	plugin_group = pragha_preferences_get_plugin_group_name(preferences, "song-info");

	priv->download_album_art =
		pragha_preferences_get_boolean (preferences, plugin_group, "DownloadAlbumArt");
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(download_album_art_w),
	                              priv->download_album_art);

	priv->setting_widget = table;
	priv->download_album_art_w = download_album_art_w;

	dialog = pragha_application_get_preferences_dialog (priv->pragha);
	pragha_preferences_append_services_setting (dialog, table, FALSE);

	pragha_preferences_dialog_connect_handler (dialog,
	                                           G_CALLBACK(pragha_songinfo_preferences_dialog_response),
	                                           plugin);

	g_object_unref (G_OBJECT (preferences));
	g_free (plugin_group);
}

static void
pragha_songinfo_plugin_remove_setting (PraghaSongInfoPlugin *plugin)
{
	PreferencesDialog *dialog;
	PraghaSongInfoPluginPrivate *priv = plugin->priv;

	dialog = pragha_application_get_preferences_dialog (priv->pragha);

	pragha_preferences_dialog_disconnect_handler (dialog,
	                                              G_CALLBACK(pragha_songinfo_preferences_dialog_response),
	                                              plugin);

	pragha_preferences_remove_services_setting (dialog, priv->setting_widget);
}

/*
 * Plugin
 */
static void
pragha_plugin_activate (PeasActivatable *activatable)
{
	PraghaPreferences *preferences;
	PraghaPlaylist *playlist;
	PraghaSidebar *sidebar;
	gchar *cache_folder = NULL;

	PraghaSongInfoPlugin *plugin = PRAGHA_SONG_INFO_PLUGIN (activatable);
	PraghaSongInfoPluginPrivate *priv = plugin->priv;

	CDEBUG(DBG_PLUGIN, "Song-info plugin %s", G_STRFUNC);

	priv->pragha = g_object_get_data (G_OBJECT (plugin), "object");

	glyr_init ();

	cache_folder = g_build_path (G_DIR_SEPARATOR_S, g_get_user_cache_dir (), "pragha", NULL);

	g_mkdir_with_parents (cache_folder, S_IRWXU);
	priv->cache_db = glyr_db_init (cache_folder);
	g_free (cache_folder);

	/* Attach Playlist popup menu*/
	priv->action_group_playlist = gtk_action_group_new ("PraghaGlyrPlaylistActions");
	gtk_action_group_set_translation_domain (priv->action_group_playlist, GETTEXT_PACKAGE);
	gtk_action_group_add_actions (priv->action_group_playlist,
	                              playlist_actions,
	                              G_N_ELEMENTS (playlist_actions),
	                              plugin);

	playlist = pragha_application_get_playlist (priv->pragha);
	priv->merge_id_playlist = pragha_playlist_append_plugin_action (playlist,
	                                                                priv->action_group_playlist,
	                                                                playlist_xml);

	/* Create the pane and attach it */
	priv->pane = pragha_songinfo_pane_new ();
	sidebar = pragha_application_get_second_sidebar (priv->pragha);
	pragha_sidebar_attach_plugin (sidebar,
		                          GTK_WIDGET (priv->pane),
		                          pragha_songinfo_pane_get_pane_title (priv->pane),
		                          pragha_songinfo_pane_get_popup_menu (priv->pane));

	/* Connect signals */

	g_signal_connect (pragha_application_get_backend (priv->pragha), "notify::state",
	                  G_CALLBACK (backend_changed_state_cb), plugin);
	backend_changed_state_cb (pragha_application_get_backend (priv->pragha), NULL, plugin);

	preferences = pragha_application_get_preferences (priv->pragha);

	g_signal_connect (G_OBJECT(preferences), "notify::secondary-lateral-panel",
	                  G_CALLBACK(pragha_songinfo_pane_visibility_changed), plugin);

	g_signal_connect (G_OBJECT(priv->pane), "type-changed",
	                  G_CALLBACK(pragha_songinfo_pane_type_changed), plugin);

	/* Default values */

	pragha_songinfo_plugin_append_setting (plugin);
}

static void
pragha_plugin_deactivate (PeasActivatable *activatable)
{
	PraghaApplication *pragha = NULL;
	PraghaPreferences *preferences;
	PraghaPlaylist *playlist;
	PraghaSidebar *sidebar;
	gchar *plugin_group = NULL;

	PraghaSongInfoPlugin *plugin = PRAGHA_SONG_INFO_PLUGIN (activatable);
	PraghaSongInfoPluginPrivate *priv = plugin->priv;

	pragha = plugin->priv->pragha;

	CDEBUG(DBG_PLUGIN, "SongInfo plugin %s", G_STRFUNC);

	g_signal_handlers_disconnect_by_func (pragha_application_get_backend (pragha),
	                                      backend_changed_state_cb, plugin);

	playlist = pragha_application_get_playlist (pragha);
	pragha_playlist_remove_plugin_action (playlist,
	                                      priv->action_group_playlist,
	                                      priv->merge_id_playlist);

	priv->merge_id_playlist = 0;

	preferences = pragha_application_get_preferences (pragha);

	g_signal_handlers_disconnect_by_func (G_OBJECT(preferences),
	                                      pragha_songinfo_pane_visibility_changed,
	                                      plugin);

	g_signal_handlers_disconnect_by_func (G_OBJECT(preferences),
	                                      pragha_songinfo_pane_type_changed,
	                                      plugin);

	plugin_group = pragha_preferences_get_plugin_group_name (preferences, "song-info");
	if (!pragha_plugins_is_shutdown(pragha_application_get_plugins_engine(priv->pragha))) {
		pragha_preferences_remove_group (preferences, plugin_group);
	}
	g_free (plugin_group);

	sidebar = pragha_application_get_second_sidebar (priv->pragha);
	pragha_sidebar_remove_plugin (sidebar, GTK_WIDGET(priv->pane));

	pragha_songinfo_plugin_remove_setting (plugin);

	glyr_db_destroy (priv->cache_db);

	glyr_cleanup ();

	priv->pragha = NULL;
}
