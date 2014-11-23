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

#include "pragha-menubar.h"

#if defined(GETTEXT_PACKAGE)
#include <glib/gi18n-lib.h>
#else
#include <glib/gi18n.h>
#endif

#include <gdk/gdkkeysyms.h>

#include "pragha-playback.h"
#include "pragha-file-utils.h"
#include "pragha-utils.h"
#include "pragha-filter-dialog.h"
#include "pragha-playlists-mgmt.h"
#include "pragha-tagger.h"
#include "pragha-tags-dialog.h"
#include "pragha-tags-mgmt.h"
#include "pragha-preferences-dialog.h"
#include "pragha-musicobject-mgmt.h"
#include "pragha-equalizer-dialog.h"
#include "pragha.h"

/*
 * Menubar callbacks.
 */

/* Playback */

static void prev_action(GtkAction *action, PraghaApplication *pragha);
static void play_pause_action(GtkAction *action, PraghaApplication *pragha);
static void stop_action(GtkAction *action, PraghaApplication *pragha);
static void next_action (GtkAction *action, PraghaApplication *pragha);
// void edit_tags_playing_action(GtkAction *action, PraghaApplication *pragha);
static void quit_action(GtkAction *action, PraghaApplication *pragha);

/* Playlist */

// void open_file_action(GtkAction *action, PraghaApplication *pragha);
// void add_location_action(GtkAction *action, PraghaApplication *pragha);
static void add_libary_action(GtkAction *action, PraghaApplication *pragha);
static void pragha_menubar_remove_playlist_action      (GtkAction *action, PraghaApplication *pragha);
static void pragha_menubar_crop_playlist_action        (GtkAction *action, PraghaApplication *pragha);
static void pragha_menubar_clear_playlist_action       (GtkAction *action, PraghaApplication *pragha);
static void pragha_menubar_save_playlist_action        (GtkAction *action, PraghaApplication *pragha);
static void pragha_menubar_export_playlist_action      (GtkAction *action, PraghaApplication *pragha);
static void pragha_menubar_save_selection_action       (GtkAction *action, PraghaApplication *pragha);
static void pragha_menubar_export_selection_action     (GtkAction *action, PraghaApplication *pragha);
static void pragha_menu_action_save_playlist           (GtkAction *action, PraghaApplication *pragha);
static void pragha_menu_action_save_selection          (GtkAction *action, PraghaApplication *pragha);
static void search_playlist_action(GtkAction *action, PraghaApplication *pragha);

/* View */

static void fullscreen_action (GtkAction *action, PraghaApplication *pragha);
static void show_controls_below_action (GtkAction *action, PraghaApplication *pragha);
static void jump_to_playing_song_action (GtkAction *action, PraghaApplication *pragha);

/* Tools */

static void show_equalizer_action(GtkAction *action, PraghaApplication *pragha);
static void rescan_library_action(GtkAction *action, PraghaApplication *pragha);
static void update_library_action(GtkAction *action, PraghaApplication *pragha);
static void statistics_action(GtkAction *action, PraghaApplication *pragha);
static void pref_action(GtkAction *action, PraghaApplication *pragha);

/* Help */

static void home_action(GtkAction *action, PraghaApplication *pragha);
static void community_action(GtkAction *action, PraghaApplication *pragha);
static void wiki_action(GtkAction *action, PraghaApplication *pragha);
static void translate_action(GtkAction *action, PraghaApplication *pragha);
// void about_action(GtkAction *action, PraghaApplication *pragha);

/*
 * Menu bar ui definition.
 */

static const gchar *main_menu_xml = "<ui>					\
	<menubar name=\"Menubar\">						\
		<menu action=\"PlaybackMenu\">					\
			<separator/>						\
			<menuitem action=\"Prev\"/>				\
			<menuitem action=\"Play_pause\"/>			\
			<menuitem action=\"Stop\"/>				\
			<menuitem action=\"Next\"/>				\
			<separator/>						\
			<menuitem action=\"Shuffle\"/>				\
			<menuitem action=\"Repeat\"/>				\
			<separator/>						\
			<menuitem action=\"Edit tags\"/>			\
			<separator/>						\
			<menuitem action=\"Quit\"/>				\
		</menu>								\
		<menu action=\"PlaylistMenu\">					\
			<menuitem action=\"Add files\"/>			\
			<menuitem action=\"Add location\"/>			\
			<placeholder name=\"pragha-append-music-placeholder\"/>		\
			<separator/>				    			\
			<menuitem action=\"Add the library\"/>		\
			<separator/>				    		\
			<menuitem action=\"Remove from playlist\"/>		\
			<menuitem action=\"Crop playlist\"/>			\
			<menuitem action=\"Clear playlist\"/>			\
			<separator/>				    		\
			<menu action=\"SavePlaylist\">				\
				<menuitem action=\"New playlist1\"/>		\
				<menuitem action=\"Export1\"/>			\
				<separator/>				    	\
				<placeholder name=\"pragha-save-playlist-placeholder\"/> 	\
			</menu>							\
			<menu action=\"SaveSelection\">				\
				<menuitem action=\"New playlist2\"/>		\
				<menuitem action=\"Export2\"/>			\
				<separator/>				    	\
				<placeholder name=\"pragha-save-selection-placeholder\"/> 	\
			</menu>							\
			<separator/>						\
			<menuitem action=\"Search in playlist\"/>		\
		</menu>								\
		<menu action=\"ViewMenu\">					\
			<menuitem action=\"Fullscreen\"/>			\
			<separator/>						\
			<menuitem action=\"Lateral panel1\"/>		\
			<menuitem action=\"Lateral panel2\"/>		\
			<menuitem action=\"Playback controls below\"/>	\
			<menuitem action=\"Status bar\"/>			\
			<separator/>						\
			<menuitem action=\"Jump to playing song\"/>	\
		</menu>								\
		<menu action=\"ToolsMenu\">					\
			<separator/>						\
			<menuitem action=\"Equalizer\"/>			\
			<separator/>						\
			<placeholder name=\"pragha-plugins-placeholder\"/>		\
			<separator/>						\
			<menuitem action=\"Rescan library\"/>			\
			<menuitem action=\"Update library\"/>			\
			<separator/>						\
			<menuitem action=\"Statistics\"/>			\
			<separator/>						\
			<menuitem action=\"Preferences\"/>			\
		</menu>								\
		<menu action=\"HelpMenu\">					\
			<menuitem action=\"Home\"/>				\
			<menuitem action=\"Community\"/>			\
			<menuitem action=\"Wiki\"/>				\
			<separator/>						\
			<menuitem action=\"Translate Pragha\"/>			\
			<separator/>						\
			<menuitem action=\"About\"/>				\
		</menu>								\
	</menubar>								\
</ui>";

static GtkActionEntry main_aentries[] = {
	{"PlaybackMenu", NULL, N_("_Playback")},
	{"PlaylistMenu", NULL, N_("Play_list")},
	{"ViewMenu", NULL, N_("_View")},
	{"ToolsMenu", NULL, N_("_Tools")},
	{"HelpMenu", NULL, N_("_Help")},
	{"Prev", "media-skip-backward", N_("Previous track"),
	 "<Alt>Left", "Prev track", G_CALLBACK(prev_action)},
	{"Play_pause", "media-playback-start", N_("Play / Pause"),
	 "<Control>space", "Play / Pause", G_CALLBACK(play_pause_action)},
	{"Stop", "media-playback-stop", N_("Stop"),
	 "", "Stop", G_CALLBACK(stop_action)},
	{"Next", "media-skip-forward", N_("Next track"),
	 "<Alt>Right", "Next track", G_CALLBACK(next_action)},
	{"Edit tags", NULL, N_("Edit track information"),
	 "<Control>E", "Edit information of current track", G_CALLBACK(edit_tags_playing_action)},
	{"Quit", "application-exit", N_("_Quit"),
	 "<Control>Q", "Quit pragha", G_CALLBACK(quit_action)},
	{"Add files", "document-open", N_("_Add files"),
	 "<Control>O", N_("Open a media file"), G_CALLBACK(open_file_action)},
	{"Add location", "network-workgroup", N_("Add _location"),
	 "", "Add a no local stream", G_CALLBACK(add_location_action)},
	{"Add the library", "list-add", N_("_Add the library"),
	"", "Add all the library", G_CALLBACK(add_libary_action)},
	{"Remove from playlist", "list-remove", N_("Remove selection from playlist"),
	 "", "Remove selection from playlist", G_CALLBACK(pragha_menubar_remove_playlist_action)},
	{"Crop playlist", "list-remove", N_("Crop playlist"),
	 "<Control>C", "Crop playlist", G_CALLBACK(pragha_menubar_crop_playlist_action)},
	{"Clear playlist", "edit-clear", N_("Clear playlist"),
	 "<Control>L", "Clear the current playlist", G_CALLBACK(pragha_menubar_clear_playlist_action)},
	{"SavePlaylist", "document-save-as", N_("Save playlist")},
	{"New playlist1", "document-new", N_("New playlist"),
	 "<Control>S", "Save new playlist", G_CALLBACK(pragha_menubar_save_playlist_action)},
	{"Export1", "media-floppy", N_("Export"),
	 "", "Export playlist", G_CALLBACK(pragha_menubar_export_playlist_action)},
	{"SaveSelection", NULL, N_("Save selection")},
	{"New playlist2", "document-new", N_("New playlist"),
	 "<Control><Shift>S", "Save new playlist", G_CALLBACK(pragha_menubar_save_selection_action)},
	{"Export2", "media-floppy", N_("Export"),
	 "", "Export playlist", G_CALLBACK(pragha_menubar_export_selection_action)},
	{"Search in playlist", "edit-find", N_("_Search in playlist"),
	 "<Control>F", "Search in playlist", G_CALLBACK(search_playlist_action)},
	{"Preferences", "preferences-system", N_("_Preferences"),
	 "<Control>P", "Set preferences", G_CALLBACK(pref_action)},
	{"Jump to playing song", "go-jump", N_("Jump to playing song"),
	 "<Control>J", "Jump to playing song", G_CALLBACK(jump_to_playing_song_action)},
	{"Equalizer", NULL, N_("E_qualizer"),
	 "", "Equalizer", G_CALLBACK(show_equalizer_action)},
	{"Rescan library", "system-run", N_("_Rescan library"),
	 "", "Rescan library", G_CALLBACK(rescan_library_action)},
	{"Update library", "system-run", N_("_Update library"),
	 "", "Update library", G_CALLBACK(update_library_action)},
	{"Statistics", "dialog-information", N_("_Statistics"),
	 "", "Statistics", G_CALLBACK(statistics_action)},
	{"Home", "go-home", N_("Homepage"),
	 "", "Homepage", G_CALLBACK(home_action)},
	{"Community", "dialog-information", N_("Community"),
	 "", "Forum of pragha", G_CALLBACK(community_action)},
	{"Wiki", NULL, N_("Wiki"),
	 "", "Wiki of pragha", G_CALLBACK(wiki_action)},
	{"Translate Pragha", "preferences-desktop-locale", N_("Translate Pragha"),
	 "", "Translate Pragha", G_CALLBACK(translate_action)},
	{"About", "help-about", N_("About"),
	 "", "About pragha", G_CALLBACK(about_action)},
};

static GtkToggleActionEntry toggles_entries[] = {
	{"Shuffle", NULL, N_("_Shuffle"),
	 "<Control>U", "Shuffle Songs", NULL,
	 FALSE},
	{"Repeat", NULL, N_("_Repeat"),
	 "<Control>R", "Repeat Songs", NULL,
	 FALSE},
	{"Fullscreen", NULL, N_("_Fullscreen"),
	 "F11", "Switch between full screen and windowed mode", G_CALLBACK(fullscreen_action),
	FALSE},
	{"Lateral panel1", NULL, N_("Lateral _panel"),
	 "F9", "Lateral panel", NULL,
	TRUE},
	{"Lateral panel2", NULL, N_("Secondary lateral panel"),
	 "", "Secondary lateral panel", NULL,
	FALSE},
	{"Playback controls below", NULL, N_("Playback controls below"),
	 NULL, "Show playback controls below", G_CALLBACK(show_controls_below_action),
	FALSE},
	{"Status bar", NULL, N_("Status bar"),
	 "", "Status bar", NULL,
	TRUE}
};

/* Sentitive menubar actions depending on the playback status. */

void
pragha_menubar_update_playback_state_cb (PraghaBackend *backend, GParamSpec *pspec, gpointer user_data)
{
	GtkAction *action;
	gboolean playing = FALSE;

	PraghaApplication *pragha = user_data;

	playing = (pragha_backend_get_state (backend) != ST_STOPPED);

	action = pragha_application_get_menu_action (pragha, "/Menubar/PlaybackMenu/Prev");
	gtk_action_set_sensitive (GTK_ACTION (action), playing);

	action = pragha_application_get_menu_action (pragha, "/Menubar/PlaybackMenu/Stop");
	gtk_action_set_sensitive (GTK_ACTION (action), playing);

	action = pragha_application_get_menu_action (pragha, "/Menubar/PlaybackMenu/Next");
	gtk_action_set_sensitive (GTK_ACTION (action), playing);

	action = pragha_application_get_menu_action (pragha, "/Menubar/PlaybackMenu/Edit tags");
	gtk_action_set_sensitive (GTK_ACTION (action), playing);

	action = pragha_application_get_menu_action (pragha, "/Menubar/ViewMenu/Jump to playing song");
	gtk_action_set_sensitive (GTK_ACTION (action), playing);
}

static void
pragha_menubar_update_playlist_changes (PraghaDatabase *database, PraghaApplication *pragha)
{
	GtkUIManager *ui_manager;
	GtkAction *action;
	PraghaPreparedStatement *statement;
	const gchar *sql = NULL, *playlist = NULL;
	gchar *action_name = NULL;

	static gint playlist_ui_id = 0;
	static GtkActionGroup *playlist_action_group = NULL;

	ui_manager = pragha_application_get_menu_ui_manager (pragha);

	gtk_ui_manager_remove_ui (ui_manager, playlist_ui_id);
	gtk_ui_manager_ensure_update (ui_manager);

	if (playlist_action_group) {
		gtk_ui_manager_remove_action_group (ui_manager, playlist_action_group);
		g_object_unref (playlist_action_group);
	}

	playlist_action_group = gtk_action_group_new ("playlists-action-group");
	gtk_ui_manager_insert_action_group (ui_manager, playlist_action_group, -1);

	playlist_ui_id = gtk_ui_manager_new_merge_id (ui_manager);

	sql = "SELECT name FROM PLAYLIST WHERE name != ? ORDER BY name COLLATE NOCASE DESC";
	statement = pragha_database_create_statement (database, sql);
	pragha_prepared_statement_bind_string (statement, 1, SAVE_PLAYLIST_STATE);

	while (pragha_prepared_statement_step (statement)) {
		playlist = pragha_prepared_statement_get_string(statement, 0);

		/* Save playlist */
		action_name = g_strdup_printf ("playlist-to-%s", playlist);
		action = gtk_action_new (action_name, playlist, NULL, NULL);
		gtk_action_group_add_action (playlist_action_group, action);
		g_object_unref (action);

		g_signal_connect (G_OBJECT (action), "activate",
		                  G_CALLBACK (pragha_menu_action_save_playlist), pragha);

		gtk_ui_manager_add_ui (ui_manager, playlist_ui_id,
				       "/Menubar/PlaylistMenu/SavePlaylist/pragha-save-playlist-placeholder",
				       playlist, action_name,
				       GTK_UI_MANAGER_MENUITEM, FALSE);
		g_free (action_name);

		/* Save selection */
		action_name = g_strdup_printf ("selection-to-%s", playlist);
		action = gtk_action_new (action_name, playlist, NULL, NULL);
		gtk_action_group_add_action (playlist_action_group, action);
		g_object_unref (action);

		g_signal_connect (G_OBJECT (action), "activate",
		                  G_CALLBACK (pragha_menu_action_save_selection), pragha);

		gtk_ui_manager_add_ui (ui_manager, playlist_ui_id,
				       "/Menubar/PlaylistMenu/SaveSelection/pragha-save-selection-placeholder",
				       playlist, action_name,
				       GTK_UI_MANAGER_MENUITEM, FALSE);
		g_free (action_name);

		pragha_process_gtk_events ();
	}
	pragha_prepared_statement_free (statement);

}

/* Add Files a folders to play list based on Audacius code.*/
/* /src/ui_fileopen.c */
static void
close_button_cb(GtkWidget *widget, gpointer data)
{
    gtk_widget_destroy(GTK_WIDGET(data));
}

static void
add_button_cb(GtkWidget *widget, gpointer data)
{
	PraghaPlaylist *playlist;
	GSList *files = NULL, *l;
	gboolean add_recursively;
	GList *mlist = NULL;

	GtkWidget *window = g_object_get_data(data, "window");
	GtkWidget *chooser = g_object_get_data(data, "chooser");
	GtkWidget *toggle = g_object_get_data(data, "toggle-button");
	PraghaApplication *pragha = g_object_get_data(data, "pragha");

	PraghaPreferences *preferences = pragha_application_get_preferences (pragha);

	add_recursively = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(toggle));
	pragha_preferences_set_add_recursively (preferences, add_recursively);

	gchar *last_folder = gtk_file_chooser_get_current_folder ((GtkFileChooser *) chooser);
	pragha_preferences_set_last_folder (preferences, last_folder);
	g_free (last_folder);

	files = gtk_file_chooser_get_filenames((GtkFileChooser *) chooser);

	gtk_widget_destroy(window);

	if (files) {
		for (l = files; l != NULL; l = l->next) {
			mlist = append_mobj_list_from_unknown_filename(mlist, l->data);
		}
		g_slist_free_full(files, g_free);

		playlist = pragha_application_get_playlist (pragha);
		pragha_playlist_append_mobj_list (playlist, mlist);
		g_list_free (mlist);
	}
}

static gboolean
open_file_on_keypress(GtkWidget *dialog,
                        GdkEventKey *event,
                        gpointer data)
{
    if (event->keyval == GDK_KEY_Escape) {
        gtk_widget_destroy(dialog);
        return TRUE;
    }

    return FALSE;
}

/* Handler for the 'Open' item in the File menu */

void open_file_action(GtkAction *action, PraghaApplication *pragha)
{
	PraghaPreferences *preferences;
	GtkWidget *window, *hbox, *vbox, *chooser, *bbox, *toggle, *close_button, *add_button;
	gpointer storage;
	gint i=0;
	GtkFileFilter *media_filter, *playlist_filter, *all_filter;
	const gchar *last_folder = NULL;

	/* Create a file chooser dialog */

	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

	gtk_window_set_type_hint (GTK_WINDOW (window), GDK_WINDOW_TYPE_HINT_DIALOG);
	gtk_window_set_title(GTK_WINDOW(window), (_("Select a file to play")));
	gtk_window_set_default_size(GTK_WINDOW(window), 700, 450);
	gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
	gtk_container_set_border_width(GTK_CONTAINER(window), 10);

	vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);

	gtk_container_add(GTK_CONTAINER(window), vbox);

	chooser = gtk_file_chooser_widget_new(GTK_FILE_CHOOSER_ACTION_OPEN);

	/* Set various properties */

	gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(chooser), TRUE);

	preferences = pragha_application_get_preferences (pragha);
	last_folder = pragha_preferences_get_last_folder (preferences);
	if (string_is_not_empty(last_folder))
		gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(chooser), last_folder);

	hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);

	toggle = gtk_check_button_new_with_label(_("Add files recursively"));
	if(pragha_preferences_get_add_recursively (preferences))
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(toggle), TRUE);

	bbox = gtk_button_box_new (GTK_ORIENTATION_HORIZONTAL);
	gtk_button_box_set_layout(GTK_BUTTON_BOX(bbox), GTK_BUTTONBOX_END);
	gtk_box_set_spacing(GTK_BOX(bbox), 6);

	close_button = gtk_button_new_with_mnemonic (_("_Cancel"));
	add_button = gtk_button_new_with_mnemonic (_("_Add"));
	gtk_container_add(GTK_CONTAINER(bbox), close_button);
	gtk_container_add(GTK_CONTAINER(bbox), add_button);

	gtk_box_pack_start(GTK_BOX(hbox), toggle, TRUE, TRUE, 3);
	gtk_box_pack_end(GTK_BOX(hbox), bbox, FALSE, FALSE, 3);

	gtk_box_pack_end(GTK_BOX(vbox), hbox, FALSE, FALSE, 3);
	gtk_box_pack_end(GTK_BOX(vbox), chooser, TRUE, TRUE, 3);

	/* Create file filters  */

	media_filter = gtk_file_filter_new();
	gtk_file_filter_set_name(GTK_FILE_FILTER(media_filter), _("Supported media"));
	
	while (mime_wav[i])
		gtk_file_filter_add_mime_type(GTK_FILE_FILTER(media_filter),
					      mime_wav[i++]);
	i = 0;
	while (mime_mpeg[i])
		gtk_file_filter_add_mime_type(GTK_FILE_FILTER(media_filter),
					      mime_mpeg[i++]);
	i = 0;
	while (mime_flac[i])
		gtk_file_filter_add_mime_type(GTK_FILE_FILTER(media_filter),
					      mime_flac[i++]);
	i = 0;
	while (mime_ogg[i])
		gtk_file_filter_add_mime_type(GTK_FILE_FILTER(media_filter),
					      mime_ogg[i++]);

	i = 0;
	while (mime_asf[i])
		gtk_file_filter_add_mime_type(GTK_FILE_FILTER(media_filter),
					      mime_asf[i++]);
	i = 0;
	while (mime_mp4[i])
		gtk_file_filter_add_mime_type(GTK_FILE_FILTER(media_filter),
					      mime_mp4[i++]);
	i = 0;
	while (mime_ape[i])
		gtk_file_filter_add_mime_type(GTK_FILE_FILTER(media_filter),
					      mime_ape[i++]);
	i = 0;
	while (mime_tracker[i])
		gtk_file_filter_add_mime_type(GTK_FILE_FILTER(media_filter),
					      mime_tracker[i++]);

	#ifdef HAVE_PLPARSER
	i = 0;
	while (mime_playlist[i])
		gtk_file_filter_add_mime_type(GTK_FILE_FILTER(media_filter),
					      mime_playlist[i++]);
	i = 0;
	while (mime_dual[i])
		gtk_file_filter_add_mime_type(GTK_FILE_FILTER(media_filter),
					      mime_dual[i++]);
	#else
	gtk_file_filter_add_pattern(GTK_FILE_FILTER(media_filter), "*.m3u");
	gtk_file_filter_add_pattern(GTK_FILE_FILTER(media_filter), "*.M3U");

	gtk_file_filter_add_pattern(GTK_FILE_FILTER(media_filter), "*.pls");
	gtk_file_filter_add_pattern(GTK_FILE_FILTER(media_filter), "*.PLS");

	gtk_file_filter_add_pattern(GTK_FILE_FILTER(media_filter), "*.xspf");
	gtk_file_filter_add_pattern(GTK_FILE_FILTER(media_filter), "*.XSPF");

	gtk_file_filter_add_pattern(GTK_FILE_FILTER(media_filter), "*.wax");
	gtk_file_filter_add_pattern(GTK_FILE_FILTER(media_filter), "*.WAX");
	#endif

	playlist_filter = gtk_file_filter_new();

	#ifdef HAVE_PLPARSER
	i = 0;
	while (mime_playlist[i])
		gtk_file_filter_add_mime_type(GTK_FILE_FILTER(playlist_filter),
					      mime_playlist[i++]);
	i = 0;
	while (mime_dual[i])
		gtk_file_filter_add_mime_type(GTK_FILE_FILTER(playlist_filter),
					      mime_dual[i++]);
	#else
	gtk_file_filter_add_pattern(GTK_FILE_FILTER(playlist_filter), "*.m3u");
	gtk_file_filter_add_pattern(GTK_FILE_FILTER(playlist_filter), "*.M3U");

	gtk_file_filter_add_pattern(GTK_FILE_FILTER(playlist_filter), "*.pls");
	gtk_file_filter_add_pattern(GTK_FILE_FILTER(playlist_filter), "*.PLS");

	gtk_file_filter_add_pattern(GTK_FILE_FILTER(playlist_filter), "*.xspf");
	gtk_file_filter_add_pattern(GTK_FILE_FILTER(playlist_filter), "*.XSPF");

	gtk_file_filter_add_pattern(GTK_FILE_FILTER(playlist_filter), "*.wax");
	gtk_file_filter_add_pattern(GTK_FILE_FILTER(playlist_filter), "*.WAX");
	#endif

	gtk_file_filter_set_name(GTK_FILE_FILTER(playlist_filter), _("Playlists"));

	all_filter = gtk_file_filter_new();
	gtk_file_filter_set_name(GTK_FILE_FILTER(all_filter), _("All files"));
	gtk_file_filter_add_pattern(GTK_FILE_FILTER(all_filter), "*");

	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(chooser),
				    GTK_FILE_FILTER(media_filter));
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(chooser),
				    GTK_FILE_FILTER(playlist_filter));
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(chooser),
				    GTK_FILE_FILTER(all_filter));

	gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(chooser),
				    GTK_FILE_FILTER(media_filter));

	storage = g_object_new(G_TYPE_OBJECT, NULL);
	g_object_set_data(storage, "window", window);
	g_object_set_data(storage, "chooser", chooser);
	g_object_set_data(storage, "toggle-button", toggle);
	g_object_set_data(storage, "pragha", pragha);

	g_signal_connect(add_button, "clicked",
		G_CALLBACK(add_button_cb), storage);
	g_signal_connect(chooser, "file-activated",
		G_CALLBACK(add_button_cb), storage);
	g_signal_connect(close_button, "clicked",
			G_CALLBACK(close_button_cb), window);
	g_signal_connect(window, "destroy",
			G_CALLBACK(gtk_widget_destroy), window);
	g_signal_connect(window, "key-press-event",
			G_CALLBACK(open_file_on_keypress), NULL);

	gtk_window_set_transient_for(GTK_WINDOW (window), GTK_WINDOW(pragha_application_get_window(pragha)));
	gtk_window_set_destroy_with_parent (GTK_WINDOW (window), TRUE);

	gtk_widget_show_all(window);
}

/* Build a dialog to get a new playlist name */

static char *
totem_open_location_set_from_clipboard (GtkWidget *open_location)
{
	GtkClipboard *clipboard;
	gchar *clipboard_content;

	/* Initialize the clipboard and get its content */
	clipboard = gtk_clipboard_get_for_display (gtk_widget_get_display (GTK_WIDGET (open_location)), GDK_SELECTION_CLIPBOARD);
	clipboard_content = gtk_clipboard_wait_for_text (clipboard);

	/* Check clipboard for "://". If it exists, return it */
	if (clipboard_content != NULL && strcmp (clipboard_content, "") != 0)
	{
		if (g_strrstr (clipboard_content, "://") != NULL)
			return clipboard_content;
	}

	g_free (clipboard_content);
	return NULL;
}

void add_location_action(GtkAction *action, PraghaApplication *pragha)
{
	PraghaPlaylist *playlist;
	PraghaDatabase *cdbase;
	GtkWidget *dialog;
	GtkWidget *vbox, *hbox;
	GtkWidget *label_new, *uri_entry, *label_name, *name_entry;
	const gchar *uri = NULL, *name = NULL;
	gchar *clipboard_location = NULL, *parsed_uri = NULL;
	PraghaMusicobject *mobj;
	gint result;

	/* Create dialog window */
	vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);

	label_new = gtk_label_new_with_mnemonic(_("Enter the URL of an internet radio stream"));
	uri_entry = gtk_entry_new();
	gtk_entry_set_max_length(GTK_ENTRY(uri_entry), 255);

	hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
	label_name = gtk_label_new_with_mnemonic(_("Give it a name to save"));
	name_entry = gtk_entry_new();
	gtk_entry_set_max_length(GTK_ENTRY(name_entry), 255);

	gtk_box_pack_start(GTK_BOX(hbox), label_name, FALSE, FALSE, 2);
	gtk_box_pack_start(GTK_BOX(hbox), name_entry, TRUE, TRUE, 2);

	gtk_box_pack_start(GTK_BOX(vbox), label_new, FALSE, FALSE, 2);
	gtk_box_pack_start(GTK_BOX(vbox), uri_entry, FALSE, FALSE, 2);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 2);

	/* Get item from clipboard to fill GtkEntry */
	clipboard_location = totem_open_location_set_from_clipboard (uri_entry);
	if (clipboard_location != NULL && strcmp (clipboard_location, "") != 0) {
		gtk_entry_set_text (GTK_ENTRY(uri_entry), clipboard_location);
		g_free (clipboard_location);
	}

	dialog = gtk_dialog_new_with_buttons (_("Add a location"),
	                                      GTK_WINDOW(pragha_application_get_window(pragha)),
	                                      GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
	                                      _("_Cancel"), GTK_RESPONSE_CANCEL,
	                                      _("_Ok"), GTK_RESPONSE_ACCEPT,
	                                      NULL);

	gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_ACCEPT);

	gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), vbox);

	gtk_window_set_default_size(GTK_WINDOW (dialog), 450, -1);

	gtk_entry_set_activates_default (GTK_ENTRY(uri_entry), TRUE);
	gtk_entry_set_activates_default (GTK_ENTRY(name_entry), TRUE);

	gtk_widget_show_all(dialog);

	result = gtk_dialog_run(GTK_DIALOG(dialog));
	switch(result) {
	case GTK_RESPONSE_ACCEPT:
		if (gtk_entry_get_text_length (GTK_ENTRY(uri_entry)))
			uri = gtk_entry_get_text(GTK_ENTRY(uri_entry));

		if (string_is_not_empty(uri)) {
			if (gtk_entry_get_text_length (GTK_ENTRY(name_entry)))
				name = gtk_entry_get_text(GTK_ENTRY(name_entry));

			parsed_uri = pragha_pl_get_first_playlist_item (uri);
			mobj = new_musicobject_from_location (parsed_uri, name);

			playlist = pragha_application_get_playlist (pragha);
			pragha_playlist_append_single_song (playlist, mobj);

			if (string_is_not_empty(name)) {
				new_radio (playlist, parsed_uri, name);

				cdbase = pragha_application_get_database (pragha);
				pragha_database_change_playlists_done (cdbase);
			}
			g_free (parsed_uri);
		}
		break;
	case GTK_RESPONSE_CANCEL:
		break;
	default:
		break;
	}
	gtk_widget_destroy(dialog);

	return;
}

/* Handler for 'Add All' action in the Tools menu */

static void add_libary_action(GtkAction *action, PraghaApplication *pragha)
{
	PraghaPlaylist *playlist;
	PraghaDatabase *cdbase;
	GList *list = NULL;
	PraghaMusicobject *mobj;

	/* Query and insert entries */

	set_watch_cursor (pragha_application_get_window(pragha));

	cdbase = pragha_application_get_database (pragha);

	const gchar *sql = "SELECT id FROM LOCATION";
	PraghaPreparedStatement *statement = pragha_database_create_statement (cdbase, sql);

	while (pragha_prepared_statement_step (statement)) {
		gint location_id = pragha_prepared_statement_get_int (statement, 0);
		mobj = new_musicobject_from_db (cdbase, location_id);

		if (G_LIKELY(mobj))
			list = g_list_prepend (list, mobj);
		else
			g_warning ("Unable to retrieve details for"
			            " location_id : %d",
			            location_id);

		pragha_process_gtk_events ();
	}

	pragha_prepared_statement_free (statement);

	remove_watch_cursor (pragha_application_get_window(pragha));

	if (list) {
		list = g_list_reverse(list);
		playlist = pragha_application_get_playlist (pragha);
		pragha_playlist_append_mobj_list (playlist, list);
		g_list_free(list);
	}
}

/* Handler for the 'Prev' item in the pragha menu */

static void prev_action(GtkAction *action, PraghaApplication *pragha)
{
	pragha_playback_prev_track(pragha);
}

/* Handler for the 'Play / Pause' item in the pragha menu */

static void play_pause_action(GtkAction *action, PraghaApplication *pragha)
{
	pragha_playback_play_pause_resume(pragha);
}

/* Handler for the 'Stop' item in the pragha menu */

static void stop_action(GtkAction *action, PraghaApplication *pragha)
{
	pragha_playback_stop(pragha);
}

/* Handler for the 'Next' item in the pragha menu */

static void next_action (GtkAction *action, PraghaApplication *pragha)
{
	pragha_playback_next_track(pragha);
}

static void
pragha_edit_tags_dialog_response (GtkWidget      *dialog,
                                  gint            response_id,
                                  PraghaApplication *pragha)
{
	PraghaBackend *backend;
	PraghaToolbar *toolbar;
	PraghaPlaylist *playlist;
	PraghaMusicobject *nmobj, *bmobj;
	PraghaTagger *tagger;
	gint changed = 0;

	if (response_id == GTK_RESPONSE_HELP) {
		nmobj = pragha_tags_dialog_get_musicobject(PRAGHA_TAGS_DIALOG(dialog));
		pragha_track_properties_dialog(nmobj, pragha_application_get_window(pragha));
		return;
	}

	if (response_id == GTK_RESPONSE_OK) {
		changed = pragha_tags_dialog_get_changed(PRAGHA_TAGS_DIALOG(dialog));
		if(changed) {
			nmobj = pragha_tags_dialog_get_musicobject(PRAGHA_TAGS_DIALOG(dialog));

			backend = pragha_application_get_backend (pragha);

			if(pragha_backend_get_state (backend) != ST_STOPPED) {
				PraghaMusicobject *current_mobj = pragha_backend_get_musicobject (backend);
				if (pragha_musicobject_compare (nmobj, current_mobj) == 0) {
					toolbar = pragha_application_get_toolbar (pragha);
					playlist = pragha_application_get_playlist (pragha);


					/* Update public current song */
					pragha_update_musicobject_change_tag (current_mobj, changed, nmobj);

					/* Update current song on playlist */
					pragha_playlist_update_current_track(playlist, changed, nmobj);

					/* Update current song on backend */
					bmobj = g_object_ref(pragha_backend_get_musicobject(backend));
					pragha_update_musicobject_change_tag(bmobj, changed, nmobj);
					g_object_unref(bmobj);

					pragha_toolbar_set_title(toolbar, current_mobj);
				}
			}

			if(G_LIKELY(pragha_musicobject_is_local_file (nmobj))) {
				tagger = pragha_tagger_new();
				pragha_tagger_add_file (tagger, pragha_musicobject_get_file(nmobj));
				pragha_tagger_set_changes(tagger, nmobj, changed);
				pragha_tagger_apply_changes (tagger);
				g_object_unref(tagger);
			}
		}
	}
	gtk_widget_destroy (dialog);
}

void edit_tags_playing_action(GtkAction *action, PraghaApplication *pragha)
{
	PraghaBackend *backend;
	GtkWidget *dialog;

	backend = pragha_application_get_backend (pragha);

	if(pragha_backend_get_state (backend) == ST_STOPPED)
		return;

	dialog = pragha_tags_dialog_new();

	g_signal_connect (G_OBJECT (dialog), "response",
	                  G_CALLBACK (pragha_edit_tags_dialog_response), pragha);

	pragha_tags_dialog_set_musicobject (PRAGHA_TAGS_DIALOG(dialog),
	                                    pragha_backend_get_musicobject (backend));
	
	gtk_widget_show (dialog);
}

/* Handler for the 'Quit' item in the pragha menu */

static void quit_action(GtkAction *action, PraghaApplication *pragha)
{
	pragha_application_quit (pragha);
}

/* Handler for 'Search Playlist' option in the Edit menu */

static void search_playlist_action(GtkAction *action, PraghaApplication *pragha)
{
	pragha_filter_dialog (pragha_application_get_playlist (pragha));
}

/* Handler for the 'Preferences' item in the Edit menu */

static void pref_action(GtkAction *action, PraghaApplication *pragha)
{
	PreferencesDialog *dialog;
	dialog = pragha_application_get_preferences_dialog (pragha);
	pragha_preferences_dialog_show (dialog);
}

/* Handler for the 'Full screen' item in the Edit menu */

static void
fullscreen_action (GtkAction *action, PraghaApplication *pragha)
{
	GtkWidget *menu_bar;
	gboolean fullscreen;
	GdkWindowState state;

	menu_bar = pragha_application_get_menubar (pragha);

	fullscreen = gtk_toggle_action_get_active(GTK_TOGGLE_ACTION(action));

	if(fullscreen){
		gtk_window_fullscreen(GTK_WINDOW(pragha_application_get_window(pragha)));
		gtk_widget_hide(GTK_WIDGET(menu_bar));
	}
	else {
		state = gdk_window_get_state (gtk_widget_get_window (pragha_application_get_window(pragha)));
		if (state & GDK_WINDOW_STATE_FULLSCREEN)
			gtk_window_unfullscreen(GTK_WINDOW(pragha_application_get_window(pragha)));
		gtk_widget_show(GTK_WIDGET(menu_bar));
	}
}

/* Handler for the 'Show_controls_below_action' item in the view menu */

static void
show_controls_below_action (GtkAction *action, PraghaApplication *pragha)
{
	PraghaPreferences *preferences;
	PraghaToolbar *toolbar;
	GtkWidget *parent;

	preferences = pragha_application_get_preferences (pragha);

	pragha_preferences_set_controls_below (preferences,
		gtk_toggle_action_get_active(GTK_TOGGLE_ACTION(action)));

	toolbar = pragha_application_get_toolbar (pragha);
	parent  = gtk_widget_get_parent (GTK_WIDGET(toolbar));

	gint position = pragha_preferences_get_controls_below (preferences) ? 3 : 1;

	gtk_box_reorder_child(GTK_BOX(parent), GTK_WIDGET(toolbar), position);
}

static void
jump_to_playing_song_action (GtkAction *action, PraghaApplication *pragha)
{
	PraghaPlaylist *playlist;
	playlist = pragha_application_get_playlist (pragha);

	pragha_playlist_show_current_track (playlist);
}

/* Handler for the 'Equalizer' item in the Tools menu */

static void
show_equalizer_action(GtkAction *action, PraghaApplication *pragha)
{
	GtkWidget *parent = pragha_application_get_window (pragha);
	PraghaBackend *backend = pragha_application_get_backend(pragha);

	pragha_equalizer_dialog_show (backend, parent);
}

/* Handler for the 'Rescan Library' item in the Tools menu */

static void rescan_library_action(GtkAction *action, PraghaApplication *pragha)
{
	PraghaScanner *scanner;
	scanner = pragha_application_get_scanner (pragha);

	pragha_scanner_scan_library (scanner);
}

/* Handler for the 'Update Library' item in the Tools menu */

static void update_library_action(GtkAction *action, PraghaApplication *pragha)
{
	PraghaScanner *scanner;
	scanner = pragha_application_get_scanner (pragha);

	pragha_scanner_update_library (scanner);
}

/* Handler for remove, crop and clear action in the Tools menu */

static void
pragha_menubar_remove_playlist_action (GtkAction *action, PraghaApplication *pragha)
{
	PraghaPlaylist *playlist;

	playlist = pragha_application_get_playlist (pragha);
	pragha_playlist_remove_selection (playlist);
}

static void
pragha_menubar_crop_playlist_action (GtkAction *action, PraghaApplication *pragha)
{
	PraghaPlaylist *playlist;

	playlist = pragha_application_get_playlist (pragha);
	pragha_playlist_crop_selection (playlist);
}

static void
pragha_menubar_clear_playlist_action (GtkAction *action, PraghaApplication *pragha)
{
	PraghaPlaylist *playlist;

	playlist = pragha_application_get_playlist (pragha);
	pragha_playlist_remove_all (playlist);
}

static void
pragha_menubar_save_playlist_action (GtkAction *action, PraghaApplication *pragha)
{
	PraghaPlaylist *playlist = pragha_application_get_playlist (pragha);
	save_current_playlist (NULL, playlist);
}

static void
pragha_menubar_export_playlist_action (GtkAction *action, PraghaApplication *pragha)
{
	PraghaPlaylist *playlist = pragha_application_get_playlist (pragha);
	export_current_playlist (NULL, playlist);
}

static void
pragha_menubar_save_selection_action (GtkAction *action, PraghaApplication *pragha)
{
	PraghaPlaylist *playlist = pragha_application_get_playlist (pragha);
	save_selected_playlist (NULL, playlist);
}

static void
pragha_menu_action_save_playlist (GtkAction *action, PraghaApplication *pragha)
{
	PraghaPlaylist *playlist = pragha_application_get_playlist (pragha);;
	const gchar *name = gtk_action_get_label (action);

	pragha_playlist_save_playlist (playlist, name);
}

static void
pragha_menu_action_save_selection (GtkAction *action, PraghaApplication *pragha)
{
	PraghaPlaylist *playlist = pragha_application_get_playlist (pragha);;
	const gchar *name = gtk_action_get_label (action);

	pragha_playlist_save_selection (playlist, name);
}

static void
pragha_menubar_export_selection_action (GtkAction *action, PraghaApplication *pragha)
{
	PraghaPlaylist *playlist = pragha_application_get_playlist (pragha);
	export_selected_playlist (NULL, playlist);
}

/* Handler for 'Statistics' action in the Tools menu */

static void statistics_action(GtkAction *action, PraghaApplication *pragha)
{
	PraghaDatabase *cdbase;
	gint n_artists, n_albums, n_tracks;
	GtkWidget *dialog;

	cdbase = pragha_application_get_database (pragha);

	n_artists = pragha_database_get_artist_count (cdbase);
	n_albums = pragha_database_get_album_count (cdbase);
	n_tracks = pragha_database_get_track_count (cdbase);

	dialog = gtk_message_dialog_new(GTK_WINDOW(pragha_application_get_window(pragha)),
					GTK_DIALOG_DESTROY_WITH_PARENT,
					GTK_MESSAGE_INFO,
					GTK_BUTTONS_OK,
					"%s %d\n%s %d\n%s %d",
					_("Total Tracks:"),
					n_tracks,
					_("Total Artists:"),
					n_artists,
					_("Total Albums:"),
					n_albums);

	gtk_window_set_title(GTK_WINDOW(dialog), _("Statistics"));

	g_signal_connect (dialog, "response",
	                  G_CALLBACK (gtk_widget_destroy), NULL);

	gtk_widget_show_all(dialog);
}

/* Handler for the 'About' action in the Help menu */

void about_widget(PraghaApplication *pragha)
{
	GtkWidget *mainwindow;
	GdkPixbuf *pixbuf_app;

	mainwindow = pragha_application_get_window (pragha);
	pixbuf_app = pragha_application_get_pixbuf_app (pragha);

	const gchar *authors[] = {
		"sujith ( m.sujith@gmail.com )",
		"matias ( mati86dl@gmail.com )",
		NULL};

	gtk_show_about_dialog(GTK_WINDOW(mainwindow),
	                      "logo", pixbuf_app,
	                      "authors", authors,
	                      "translator-credits", _("translator-credits"),
	                      "comments", "A lightweight GTK+ music player",
	                      "copyright", "(C) 2007-2009 Sujith\n(C) 2009-2014 Matias",
	                      "license-type", GTK_LICENSE_GPL_3_0,
	                      "name", PACKAGE_NAME,
	                      "version", PACKAGE_VERSION,
	                      NULL);
}

static void home_action(GtkAction *action, PraghaApplication *pragha)
{
	const gchar *uri = "http://pragha.wikispaces.com/";
	open_url(uri, pragha_application_get_window(pragha));
}

static void community_action(GtkAction *action, PraghaApplication *pragha)
{
	const gchar *uri = "http://bbs.archlinux.org/viewtopic.php?id=46171";
	open_url(uri, pragha_application_get_window(pragha));
}

static void wiki_action(GtkAction *action, PraghaApplication *pragha)
{
	const gchar *uri = "http://pragha.wikispaces.com/";
	open_url(uri, pragha_application_get_window(pragha));
}

static void translate_action(GtkAction *action, PraghaApplication *pragha)
{
	const gchar *uri = "http://www.transifex.net/projects/p/Pragha/";
	open_url(uri, pragha_application_get_window(pragha));
}

void about_action(GtkAction *action, PraghaApplication *pragha)
{
	about_widget(pragha);
}

void
pragha_menubar_connect_signals (GtkUIManager *menu_ui_manager, PraghaApplication *pragha)
{
	PraghaPreferences *preferences;
	GtkActionGroup *main_actions;

	const GBindingFlags binding_flags =
		G_BINDING_SYNC_CREATE | G_BINDING_BIDIRECTIONAL;
 
	main_actions = gtk_action_group_new("Main Actions");

	gtk_action_group_set_translation_domain (main_actions, GETTEXT_PACKAGE);

	gtk_action_group_add_actions (main_actions,
	                              main_aentries,
	                              G_N_ELEMENTS(main_aentries),
	                              (gpointer)pragha);
	gtk_action_group_add_toggle_actions (main_actions,
	                                     toggles_entries,
	                                     G_N_ELEMENTS(toggles_entries),
	                                     pragha);

	gtk_window_add_accel_group (GTK_WINDOW(pragha_application_get_window(pragha)),
	                            gtk_ui_manager_get_accel_group(menu_ui_manager));

	gtk_ui_manager_insert_action_group (menu_ui_manager, main_actions, 0);

	/* Hide second sidebar */
	GtkAction *action_sidebar = gtk_ui_manager_get_action(menu_ui_manager, "/Menubar/ViewMenu/Lateral panel2");
	gtk_action_set_visible (action_sidebar, FALSE);

	/* Binding properties to Actions. */

	preferences = pragha_application_get_preferences (pragha);

	GtkAction *action_shuffle = gtk_ui_manager_get_action(menu_ui_manager, "/Menubar/PlaybackMenu/Shuffle");
	g_object_bind_property (preferences, "shuffle", action_shuffle, "active", binding_flags);

	GtkAction *action_repeat = gtk_ui_manager_get_action(menu_ui_manager,"/Menubar/PlaybackMenu/Repeat");
	g_object_bind_property (preferences, "repeat", action_repeat, "active", binding_flags);

	GtkAction *action_lateral1 = gtk_ui_manager_get_action(menu_ui_manager, "/Menubar/ViewMenu/Lateral panel1");
	g_object_bind_property (preferences, "lateral-panel", action_lateral1, "active", binding_flags);

	GtkAction *action_lateral2 = gtk_ui_manager_get_action(menu_ui_manager, "/Menubar/ViewMenu/Lateral panel2");
	g_object_bind_property (preferences, "secondary-lateral-panel", action_lateral2, "active", binding_flags);

	GtkAction *action_status_bar = gtk_ui_manager_get_action(menu_ui_manager, "/Menubar/ViewMenu/Status bar");
	g_object_bind_property (preferences, "show-status-bar", action_status_bar, "active", binding_flags);

	g_signal_connect (pragha_application_get_database(pragha), "PlaylistsChanged",
	                  G_CALLBACK(pragha_menubar_update_playlist_changes), pragha);

	pragha_menubar_update_playlist_changes (pragha_application_get_database(pragha), pragha);

	g_object_unref (main_actions);
}

GtkUIManager*
pragha_menubar_new (void)
{
	GtkUIManager *main_menu = NULL;
	gchar *pragha_accels_path = NULL;
	GError *error = NULL;

	main_menu = gtk_ui_manager_new();

	if (!gtk_ui_manager_add_ui_from_string(main_menu, main_menu_xml, -1, &error)) {
		g_critical("Unable to create main menu, err : %s", error->message);
	}

	/* Load menu accelerators edited */

	pragha_accels_path = g_build_path(G_DIR_SEPARATOR_S, g_get_user_config_dir(), "/pragha/accels.scm", NULL);
	gtk_accel_map_load (pragha_accels_path);
	g_free (pragha_accels_path);

	return main_menu;
}
