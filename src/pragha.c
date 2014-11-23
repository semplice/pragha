/*************************************************************************/
/* Copyright (C) 2007-2009 sujith <m.sujith@gmail.com>                   */
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

#if HAVE_CONFIG_H
#include <config.h>
#endif

#include "pragha.h"

#if defined(GETTEXT_PACKAGE)
#include <glib/gi18n-lib.h>
#else
#include <glib/gi18n.h>
#endif

#include <glib.h>
#include <locale.h> /* require LC_ALL */
#include <libintl.h>
#include <tag_c.h>

#ifdef HAVE_LIBPEAS
#include "pragha-plugins-engine.h"
#endif

#include "pragha-window.h"
#include "pragha-playback.h"
#include "pragha-musicobject-mgmt.h"
#include "pragha-menubar.h"
#include "pragha-file-utils.h"
#include "pragha-utils.h"
#include "pragha-music-enum.h"

#ifdef G_OS_WIN32
#include "../win32/win32dep.h"
#endif

gint debug_level;
#ifdef DEBUG
GThread *pragha_main_thread = NULL;
#endif

struct _PraghaApplication {
	GtkApplication base_instance;

	/* Main window and icon */

	GtkWidget         *mainwindow;
	GdkPixbuf         *pixbuf_app;

	/* Main stuff */

	PraghaBackend     *backend;
	PraghaPreferences *preferences;
	PraghaDatabase    *cdbase;
	PraghaArtCache    *art_cache;
	PraghaMusicEnum   *enum_map;

	PraghaScanner     *scanner;

	PreferencesDialog *setting_dialog;

	/* Main widgets */

	GtkUIManager      *menu_ui_manager;
	PraghaToolbar     *toolbar;
	GtkWidget         *infobox;
	GtkWidget         *pane1;
	GtkWidget         *pane2;
	PraghaSidebar     *sidebar1;
	PraghaSidebar     *sidebar2;
	PraghaLibraryPane *library;
	PraghaPlaylist    *playlist;
	PraghaStatusbar   *statusbar;

	PraghaStatusIcon  *status_icon;

	GBinding          *sidebar2_binding;

#ifdef HAVE_LIBPEAS
	PraghaPluginsEngine *plugins_engine;
#endif
};

G_DEFINE_TYPE (PraghaApplication, pragha_application, GTK_TYPE_APPLICATION);

/*
 * Some calbacks..
 */
static void
pragha_library_pane_append_tracks (PraghaLibraryPane *library, PraghaApplication *pragha)
{
	GList *list = NULL;
	list = pragha_library_pane_get_mobj_list (library);
	if (list) {
		pragha_playlist_append_mobj_list (pragha->playlist,
			                              list);
		g_list_free(list);
	}
}

static void
pragha_library_pane_replace_tracks (PraghaLibraryPane *library, PraghaApplication *pragha)
{
	GList *list = NULL;
	list = pragha_library_pane_get_mobj_list (library);
	if (list) {
		pragha_playlist_remove_all (pragha->playlist);

		pragha_playlist_append_mobj_list (pragha->playlist,
			                              list);
		g_list_free(list);
	}
}

static void
pragha_library_pane_replace_tracks_and_play (PraghaLibraryPane *library, PraghaApplication *pragha)
{
	GList *list = NULL;
	list = pragha_library_pane_get_mobj_list (library);
	if (list) {
		pragha_playlist_remove_all (pragha->playlist);

		pragha_playlist_append_mobj_list (pragha->playlist,
			                              list);

		if (pragha_backend_get_state (pragha->backend) != ST_STOPPED)
			pragha_playback_next_track(pragha);
		else
			pragha_playback_play_pause_resume(pragha);

		g_list_free(list);
	}
}

static void
pragha_playlist_update_change_tags (PraghaPlaylist *playlist, gint changed, PraghaMusicobject *mobj, PraghaApplication *pragha)
{
	PraghaBackend *backend;
	PraghaToolbar *toolbar;
	PraghaMusicobject *cmobj = NULL;

	backend = pragha_application_get_backend (pragha);

	if(pragha_backend_get_state (backend) != ST_STOPPED) {
		cmobj = pragha_backend_get_musicobject (backend);
		pragha_update_musicobject_change_tag (cmobj, changed, mobj);

		toolbar = pragha_application_get_toolbar (pragha);
		pragha_toolbar_set_title (toolbar, cmobj);
	}
}

static void
pragha_playlist_update_statusbar_playtime (PraghaPlaylist *playlist, PraghaApplication *pragha)
{
	PraghaStatusbar *statusbar;
	gint total_playtime = 0, no_tracks = 0;
	gchar *str, *tot_str;

	if(pragha_playlist_is_changing(playlist))
		return;

	total_playtime = pragha_playlist_get_total_playtime (playlist);
	no_tracks = pragha_playlist_get_no_tracks (playlist);

	tot_str = convert_length_str(total_playtime);
	str = g_strdup_printf("%i %s - %s",
	                      no_tracks,
	                      ngettext("Track", "Tracks", no_tracks),
	                      tot_str);

	CDEBUG(DBG_VERBOSE, "Updating status bar with new playtime: %s", tot_str);

	statusbar = pragha_application_get_statusbar (pragha);
	pragha_statusbar_set_main_text(statusbar, str);

	g_free(tot_str);
	g_free(str);
}

static void
pragha_art_cache_changed_handler (PraghaArtCache *cache, PraghaApplication *pragha)
{
	PraghaBackend *backend;
	PraghaToolbar *toolbar;
	PraghaMusicobject *mobj = NULL;
	gchar *album_art_path = NULL;
	const gchar *artist = NULL, *album = NULL;

	backend = pragha_application_get_backend (pragha);
	if (pragha_backend_get_state (backend) != ST_STOPPED) {
		mobj = pragha_backend_get_musicobject (backend);

		artist = pragha_musicobject_get_artist (mobj);
		album = pragha_musicobject_get_album (mobj);
	
		album_art_path = pragha_art_cache_get_uri (cache, artist, album);

		if (album_art_path) {
			toolbar = pragha_application_get_toolbar (pragha);
			pragha_toolbar_set_image_album_art (toolbar, album_art_path);
			g_free (album_art_path);
		}
	}
}

static void
pragha_libary_list_changed_cb (PraghaPreferences *preferences, PraghaApplication *pragha)
{
	GtkWidget *infobar = create_info_bar_update_music (pragha);
	pragha_window_add_widget_to_infobox (pragha, infobar);
}

static void
pragha_enum_map_removed_handler (PraghaMusicEnum *enum_map, gint enum_removed, PraghaApplication *pragha)
{
	pragha_playlist_crop_music_type (pragha->playlist, enum_removed);
}

/*
 * Some public actions.
 */

PraghaPreferences *
pragha_application_get_preferences (PraghaApplication *pragha)
{
	return pragha->preferences;
}

PraghaDatabase *
pragha_application_get_database (PraghaApplication *pragha)
{
	return pragha->cdbase;
}

PraghaArtCache *
pragha_application_get_art_cache (PraghaApplication *pragha)
{
	return pragha->art_cache;
}

PraghaBackend *
pragha_application_get_backend (PraghaApplication *pragha)
{
	return pragha->backend;
}

PraghaScanner *
pragha_application_get_scanner (PraghaApplication *pragha)
{
	return pragha->scanner;
}

GtkWidget *
pragha_application_get_window (PraghaApplication *pragha)
{
	return pragha->mainwindow;
}

GdkPixbuf *
pragha_application_get_pixbuf_app (PraghaApplication *pragha)
{
	return pragha->pixbuf_app;
}

PraghaPlaylist *
pragha_application_get_playlist (PraghaApplication *pragha)
{
	return pragha->playlist;
}

PraghaLibraryPane *
pragha_application_get_library (PraghaApplication *pragha)
{
	return pragha->library;
}

PreferencesDialog *
pragha_application_get_preferences_dialog (PraghaApplication *pragha)
{
	return pragha->setting_dialog;
}

PraghaToolbar *
pragha_application_get_toolbar (PraghaApplication *pragha)
{
	return pragha->toolbar;
}

PraghaSidebar *
pragha_application_get_first_sidebar (PraghaApplication *pragha)
{
	return pragha->sidebar1;
}

PraghaSidebar *
pragha_application_get_second_sidebar (PraghaApplication *pragha)
{
	return pragha->sidebar2;
}

PraghaStatusbar *
pragha_application_get_statusbar (PraghaApplication *pragha)
{
	return pragha->statusbar;
}

PraghaStatusIcon *
pragha_application_get_status_icon (PraghaApplication *pragha)
{
	return pragha->status_icon;
}

GtkUIManager *
pragha_application_get_menu_ui_manager (PraghaApplication *pragha)
{
	return pragha->menu_ui_manager;
}

GtkAction *
pragha_application_get_menu_action (PraghaApplication *pragha, const gchar *path)
{
	GtkUIManager *ui_manager = pragha_application_get_menu_ui_manager (pragha);

	return gtk_ui_manager_get_action (ui_manager, path);
}

GtkWidget *
pragha_application_get_menu_action_widget (PraghaApplication *pragha, const gchar *path)
{
	GtkUIManager *ui_manager = pragha_application_get_menu_ui_manager (pragha);

	return gtk_ui_manager_get_widget (ui_manager, path);
}

GtkWidget *
pragha_application_get_menubar (PraghaApplication *pragha)
{
	GtkUIManager *ui_manager = pragha_application_get_menu_ui_manager (pragha);

	return gtk_ui_manager_get_widget (ui_manager, "/Menubar");
}

GtkWidget *
pragha_application_get_infobox_container (PraghaApplication *pragha)
{
	return pragha->infobox;
}

GtkWidget *
pragha_application_get_first_pane (PraghaApplication *pragha)
{
	return pragha->pane1;
}

GtkWidget *
pragha_application_get_second_pane (PraghaApplication *pragha)
{
	return pragha->pane2;
}

gboolean
pragha_application_is_first_run (PraghaApplication *pragha)
{
	return string_is_empty (pragha_preferences_get_installed_version (pragha->preferences));
}

static void
pragha_application_construct_window (PraghaApplication *pragha)
{
	gchar *icon_uri = NULL;

	/* Main window */

	pragha->mainwindow = gtk_application_window_new (GTK_APPLICATION (pragha));

	icon_uri = g_build_filename (PIXMAPDIR, "pragha.png", NULL);
	pragha->pixbuf_app = gdk_pixbuf_new_from_file (icon_uri, NULL);
	g_free (icon_uri);

	if (!pragha->pixbuf_app)
		g_warning("Unable to load pragha png");
	else
		gtk_window_set_icon (GTK_WINDOW(pragha->mainwindow),
		                     pragha->pixbuf_app);
	
	gtk_window_set_title(GTK_WINDOW(pragha->mainwindow), _("Pragha Music Player"));

	/* Get all widgets instances */

	pragha->menu_ui_manager = pragha_menubar_new ();
	pragha->toolbar = pragha_toolbar_new ();
	pragha->infobox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
	pragha->pane1 = gtk_paned_new (GTK_ORIENTATION_HORIZONTAL);
	pragha->pane2 = gtk_paned_new (GTK_ORIENTATION_HORIZONTAL);
	pragha->sidebar1 = pragha_sidebar_new ();
	pragha->sidebar2 = pragha_sidebar_new ();
	pragha->library = pragha_library_pane_new ();
	pragha->playlist = pragha_playlist_new ();
	pragha->statusbar = pragha_statusbar_get ();
	pragha->scanner = pragha_scanner_new();

	pragha->status_icon = pragha_status_icon_new (pragha);

	pragha_menubar_connect_signals (pragha->menu_ui_manager, pragha);

	/* Contruct the window. */

	pragha_window_new (pragha);
}

static void
pragha_application_dispose (GObject *object)
{
	PraghaApplication *pragha = PRAGHA_APPLICATION (object);

	CDEBUG(DBG_INFO, "Cleaning up");

#ifdef HAVE_LIBPEAS
	if (pragha->plugins_engine) {
		g_object_unref (pragha->plugins_engine);
		pragha->plugins_engine = NULL;
	}
#endif
	if (pragha->setting_dialog) {
		pragha_preferences_dialog_free (pragha->setting_dialog);
		pragha->setting_dialog = NULL;
	}
	if (pragha->backend) {
		g_object_unref (pragha->backend);
		pragha->backend = NULL;
	}
	if (pragha->art_cache) {
		g_object_unref (pragha->art_cache);
		pragha->art_cache = NULL;
	}
	if (pragha->enum_map) {
		g_object_unref (pragha->enum_map);
		pragha->enum_map = NULL;
	}
	if (pragha->scanner) {
		pragha_scanner_free (pragha->scanner);
		pragha->scanner = NULL;
	}
	if (pragha->pixbuf_app) {
		g_object_unref (pragha->pixbuf_app);
		pragha->pixbuf_app = NULL;
	}
	if (pragha->menu_ui_manager) {
		g_object_unref (pragha->menu_ui_manager);
		pragha->menu_ui_manager = NULL;
	}

	/* Save Preferences and database. */

	if (pragha->preferences) {
		g_object_unref (pragha->preferences);
		pragha->preferences = NULL;
	}
	if (pragha->cdbase) {
		g_object_unref (pragha->cdbase);
		pragha->cdbase = NULL;
	}

	G_OBJECT_CLASS (pragha_application_parent_class)->dispose (object);
}

static void
pragha_application_startup (GApplication *application)
{
	PraghaToolbar *toolbar;
	PraghaPlaylist *playlist;
	const gchar *version = NULL;

	const GBindingFlags binding_flags =
		G_BINDING_SYNC_CREATE | G_BINDING_BIDIRECTIONAL;

	PraghaApplication *pragha = PRAGHA_APPLICATION (application);

	G_APPLICATION_CLASS (pragha_application_parent_class)->startup (application);

	/* Allocate memory for simple structures */

	pragha->preferences = pragha_preferences_get();

	pragha->cdbase = pragha_database_get();
	if (pragha_database_start_successfully(pragha->cdbase) == FALSE) {
		g_error("Unable to init music dbase");
	}

	version = pragha_preferences_get_installed_version (pragha->preferences);
	if (string_is_not_empty (version) && (g_ascii_strcasecmp (version, "1.3.1") < 0)) {
		pragha_database_compatibilize_version (pragha->cdbase);
	}

	pragha->enum_map = pragha_music_enum_get ();
	g_signal_connect (pragha->enum_map, "enum-removed",
	                  G_CALLBACK(pragha_enum_map_removed_handler), pragha);

#ifdef HAVE_LIBPEAS
	pragha->plugins_engine = pragha_plugins_engine_new (pragha);
#endif

	pragha->art_cache = pragha_art_cache_get ();
	g_signal_connect (pragha->art_cache, "cache-changed",
	                  G_CALLBACK(pragha_art_cache_changed_handler), pragha);

	pragha->backend = pragha_backend_new ();

	g_signal_connect (pragha->backend, "finished",
	                  G_CALLBACK(pragha_backend_finished_song), pragha);
	g_signal_connect (pragha->backend, "tags-changed",
	                  G_CALLBACK(pragha_backend_tags_changed), pragha);

	g_signal_connect (pragha->backend, "error",
	                 G_CALLBACK(gui_backend_error_show_dialog_cb), pragha);
	g_signal_connect (pragha->backend, "error",
	                  G_CALLBACK(gui_backend_error_update_current_playlist_cb), pragha);
	g_signal_connect (pragha->backend, "notify::state",
	                  G_CALLBACK (pragha_menubar_update_playback_state_cb), pragha);

	/*
	 * Collect widgets and construct the window.
	 */

	pragha_application_construct_window (pragha);

	/* Connect Signals and Bindings. */

	toolbar = pragha->toolbar;
	g_signal_connect_swapped (toolbar, "prev",
	                          G_CALLBACK(pragha_playback_prev_track), pragha);
	g_signal_connect_swapped (toolbar, "play",
	                          G_CALLBACK(pragha_playback_play_pause_resume), pragha);
	g_signal_connect_swapped (toolbar, "stop",
	                          G_CALLBACK(pragha_playback_stop), pragha);
	g_signal_connect_swapped (toolbar, "next",
	                          G_CALLBACK(pragha_playback_next_track), pragha);
	g_signal_connect (toolbar, "unfull-activated",
	                  G_CALLBACK(pragha_window_unfullscreen), pragha);
	g_signal_connect (toolbar, "album-art-activated",
	                  G_CALLBACK(pragha_playback_show_current_album_art), pragha);
	g_signal_connect (toolbar, "track-info-activated",
	                  G_CALLBACK(pragha_playback_edit_current_track), pragha);
	g_signal_connect (toolbar, "track-progress-activated",
	                  G_CALLBACK(pragha_playback_seek_fraction), pragha);

	playlist = pragha->playlist;
	g_signal_connect (playlist, "playlist-set-track",
	                  G_CALLBACK(pragha_playback_set_playlist_track), pragha);
	g_signal_connect (playlist, "playlist-change-tags",
	                  G_CALLBACK(pragha_playlist_update_change_tags), pragha);
	g_signal_connect (playlist, "playlist-changed",
	                  G_CALLBACK(pragha_playlist_update_statusbar_playtime), pragha);
	pragha_playlist_update_statusbar_playtime (playlist, pragha);
		
	g_signal_connect (pragha->library, "library-append-playlist",
	                  G_CALLBACK(pragha_library_pane_append_tracks), pragha);
	g_signal_connect (pragha->library, "library-replace-playlist",
	                  G_CALLBACK(pragha_library_pane_replace_tracks), pragha);
	g_signal_connect (pragha->library, "library-replace-playlist-and-play",
	                  G_CALLBACK(pragha_library_pane_replace_tracks_and_play), pragha);

	g_signal_connect (G_OBJECT(pragha->mainwindow), "window-state-event",
	                  G_CALLBACK(pragha_toolbar_window_state_event), toolbar);
	g_signal_connect (G_OBJECT(toolbar), "notify::timer-remaining-mode",
	                  G_CALLBACK(pragha_toolbar_show_ramaning_time_cb), pragha->backend);

	g_signal_connect (pragha->backend, "notify::state",
	                  G_CALLBACK(pragha_toolbar_playback_state_cb), toolbar);
	g_signal_connect (pragha->backend, "tick",
	                 G_CALLBACK(pragha_toolbar_update_playback_progress), toolbar);
	g_signal_connect (pragha->backend, "buffering",
	                  G_CALLBACK(pragha_toolbar_update_buffering_cb), toolbar);

	g_signal_connect (pragha->backend, "notify::state",
	                  G_CALLBACK (update_current_playlist_view_playback_state_cb), pragha->playlist);

	g_object_bind_property (pragha->backend, "volume",
	                        toolbar, "volume",
	                        binding_flags);

	g_object_bind_property (pragha->preferences, "timer-remaining-mode",
	                        toolbar, "timer-remaining-mode",
	                        binding_flags);

	g_signal_connect (pragha->preferences, "LibraryChanged",
	                  G_CALLBACK (pragha_libary_list_changed_cb), pragha);

	pragha->sidebar2_binding =
		g_object_bind_property (pragha->preferences, "secondary-lateral-panel",
		                        pragha->sidebar2, "visible",
		                        binding_flags);

	pragha->setting_dialog = pragha_preferences_dialog_new (pragha->mainwindow);

	#ifdef HAVE_LIBPEAS
	pragha_plugins_engine_startup (pragha->plugins_engine);
	#endif

	/* Finally fill the library and the playlist */

	pragha_init_gui_state (pragha);
}

static void
pragha_application_shutdown (GApplication *application)
{
	PraghaApplication *pragha = PRAGHA_APPLICATION (application);

	CDEBUG(DBG_INFO, "Pragha shutdown: Saving curret state.");

	if (pragha_preferences_get_restore_playlist (pragha->preferences))
		pragha_playlist_save_playlist_state (pragha->playlist);

	pragha_window_save_settings (pragha);

	pragha_playback_stop (pragha);

	/* Shutdown plugins can hide sidebar before save settings. */
	if (pragha->sidebar2_binding) {
		g_object_unref (pragha->sidebar2_binding);
		pragha->sidebar2_binding = NULL;
	}

#ifdef HAVE_LIBPEAS
	pragha_plugins_engine_shutdown (pragha->plugins_engine);
#endif

	gtk_widget_destroy (pragha->mainwindow);

	G_APPLICATION_CLASS (pragha_application_parent_class)->shutdown (application);
}

static void
pragha_application_activate (GApplication *application)
{
	PraghaApplication *pragha = PRAGHA_APPLICATION (application);

	CDEBUG(DBG_INFO, G_STRFUNC);

	gtk_window_present (GTK_WINDOW (pragha->mainwindow));
}

static void
pragha_application_open (GApplication *application, GFile **files, gint n_files, const gchar *hint)
{
	PraghaApplication *pragha = PRAGHA_APPLICATION (application);
	gint i;
	GList *mlist = NULL;

	for (i = 0; i < n_files; i++) {
		gchar *path = g_file_get_path (files[i]);
		mlist = append_mobj_list_from_unknown_filename (mlist, path);
		g_free (path);
	}

	if (mlist) {
		pragha_playlist_append_mobj_list (pragha->playlist, mlist);
		g_list_free (mlist);
	}

	gtk_window_present (GTK_WINDOW (pragha->mainwindow));
}

static int
pragha_application_command_line (GApplication *application, GApplicationCommandLine *command_line)
{
	PraghaApplication *pragha = PRAGHA_APPLICATION (application);
	int ret = 0;
	gint argc;

	gchar **argv = g_application_command_line_get_arguments (command_line, &argc);

	if (argc <= 1) {
		pragha_application_activate (application);
		goto exit;
	}

	ret = handle_command_line (pragha, command_line, argc, argv);

exit:
	g_strfreev (argv);

	return ret;
}

//it's used for --help and --version
static gboolean
pragha_application_local_command_line (GApplication *application, gchar ***arguments, int *exit_status)
{
	PraghaApplication *pragha = PRAGHA_APPLICATION (application);

	gchar **argv = *arguments;
	gint argc = g_strv_length (argv);

	*exit_status = handle_command_line (pragha, NULL, argc, argv);

	return FALSE;
}

void
pragha_application_quit (PraghaApplication *pragha)
{
	g_application_quit (G_APPLICATION (pragha));
}

static void
pragha_application_class_init (PraghaApplicationClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (class);
	GApplicationClass *application_class = G_APPLICATION_CLASS (class);

	object_class->dispose = pragha_application_dispose;

	application_class->startup = pragha_application_startup;
	application_class->shutdown = pragha_application_shutdown;
	application_class->activate = pragha_application_activate;
	application_class->open = pragha_application_open;
	application_class->command_line = pragha_application_command_line;
	application_class->local_command_line = pragha_application_local_command_line;
}

static void
pragha_application_init (PraghaApplication *pragha)
{
}

PraghaApplication *
pragha_application_new ()
{
	return g_object_new (PRAGHA_TYPE_APPLICATION,
	                     "application-id", "org.pragha",
	                     "flags", G_APPLICATION_HANDLES_COMMAND_LINE | G_APPLICATION_HANDLES_OPEN,
	                     NULL);
}

gint main(gint argc, gchar *argv[])
{
	PraghaApplication *pragha;
	int status;
#ifdef DEBUG
	g_print ("debug enabled\n");
	pragha_main_thread = g_thread_self ();
#endif
	debug_level = 0;

	/* setup translation domain */
	setlocale (LC_ALL, "");
	bindtextdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);

	/* Force unicode to taglib. */
	taglib_set_strings_unicode(TRUE);
	taglib_set_string_management_enabled(FALSE);

	/* Setup application name and pulseaudio role */
	g_set_application_name(_("Pragha Music Player"));
	g_setenv("PULSE_PROP_media.role", "audio", TRUE);

#if !GLIB_CHECK_VERSION(2,35,1)
	g_type_init ();
#endif

	pragha = pragha_application_new ();
	status = g_application_run (G_APPLICATION (pragha), argc, argv);
	g_object_run_dispose (G_OBJECT (pragha));
	g_object_unref (pragha);

	return status;
}
