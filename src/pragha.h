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

#ifndef PRAGHA_H
#define PRAGHA_H

#ifndef HAVE_PARANOIA_NEW_INCLUDES
#include "pragha-cdda.h" // Should be before config.h, libcdio 0.83 issue
#endif

#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib.h>
#include <glib/gstdio.h>
#include <dbus/dbus-glib-lowlevel.h>
#include <gtk/gtk.h>
#include <tag_c.h>

#include "pragha-album-art.h"
#include "pragha-art-cache.h"
#include "pragha-backend.h"
#include "pragha-database.h"
#include "pragha-glyr.h"
#include "pragha-musicobject.h"
#include "pragha-notify.h"
#include "pragha-preferences.h"
#include "pragha-preferences-dialog.h"
#include "pragha-playlist.h"
#include "pragha-library-pane.h"
#include "pragha-lastfm.h"
#include "pragha-scanner.h"
#include "pragha-sidebar.h"
#include "pragha-simple-async.h"
#include "pragha-statusbar.h"
#include "pragha-statusicon.h"
#include "pragha-window.h"
#include "gnome-media-keys.h"
#include "pragha-mpris.h"

/* With libcio 0.83 should be before config.h. libcdio issue */
#ifdef HAVE_PARANOIA_NEW_INCLUDES
#include "pragha-cdda.h"
#endif

/* Some default preferences. */

#define MIN_WINDOW_WIDTH           (gdk_screen_width() * 3 / 4)
#define MIN_WINDOW_HEIGHT          (gdk_screen_height() * 3 / 4)
#define COL_WIDTH_THRESH           30
#define DEFAULT_PLAYLIST_COL_WIDTH ((MIN_WINDOW_WIDTH - DEFAULT_SIDEBAR_SIZE) / 4)

#define WAIT_UPDATE 5

enum dnd_target {
	TARGET_REF_LIBRARY,
	TARGET_URI_LIST,
	TARGET_PLAIN_TEXT
};

typedef struct _PraghaApplication PraghaApplication;

struct _PraghaApplication {
	/* Main window and icon */

	GtkWidget         *mainwindow;
	GdkPixbuf         *pixbuf_app;

	/* Main stuff */

	PraghaBackend     *backend;
	PraghaPreferences *preferences;
	PraghaDatabase    *cdbase;
	PraghaArtCache    *art_cache;
	DBusConnection    *con_dbus;

	PraghaScanner     *scanner;

	/* Main widgets */

	GtkUIManager      *menu_ui_manager;
	PraghaToolbar     *toolbar;
	GtkWidget         *infobox;
	GtkWidget         *pane;
	PraghaSidebar     *sidebar;
	PraghaLibraryPane *library;
	PraghaPlaylist    *playlist;
	PraghaStatusbar   *statusbar;

	PraghaStatusIcon  *status_icon;

	/* Plugins?. */

	PraghaNotify      *notify;
#ifdef HAVE_LIBGLYR
	PraghaGlyr        *glyr;
#endif
#ifdef HAVE_LIBCLASTFM
	PraghaLastfm      *clastfm;
#endif
	PraghaMpris2      *mpris2;
	con_gnome_media_keys *cgnome_media_keys;

#ifdef HAVE_LIBKEYBINDER
	gboolean           keybinder;
#endif

	/* Flags */

	gboolean           unique_instance;
};

/* Functions to access private members */

PraghaPreferences *pragha_application_get_preferences     (PraghaApplication *pragha);
PraghaDatabase    *pragha_application_get_database        (PraghaApplication *pragha);
PraghaArtCache    *pragha_application_get_art_cache       (PraghaApplication *pragha);

PraghaBackend     *pragha_application_get_backend         (PraghaApplication *pragha);

PraghaScanner     *pragha_application_get_scanner         (PraghaApplication *pragha);

GtkWidget         *pragha_application_get_window          (PraghaApplication *pragha);
GdkPixbuf         *pragha_application_get_pixbuf_app      (PraghaApplication *pragha);
PraghaPlaylist    *pragha_application_get_playlist        (PraghaApplication *pragha);
PraghaLibraryPane *pragha_application_get_library         (PraghaApplication *pragha);
PraghaToolbar     *pragha_application_get_toolbar         (PraghaApplication *pragha);
PraghaSidebar     *pragha_application_get_sidebar         (PraghaApplication *pragha);
PraghaStatusbar   *pragha_application_get_statusbar       (PraghaApplication *pragha);
PraghaStatusIcon  *pragha_application_get_status_icon     (PraghaApplication *pragha);

GtkUIManager      *pragha_application_get_menu_ui_manager    (PraghaApplication *pragha);
GtkAction         *pragha_application_get_menu_action        (PraghaApplication *pragha, const gchar *path);
GtkWidget         *pragha_application_get_menu_action_widget (PraghaApplication *pragha, const gchar *path);
GtkWidget         *pragha_application_get_menubar            (PraghaApplication *pragha);
GtkWidget         *pragha_application_get_infobox_container  (PraghaApplication *pragha);
GtkWidget         *pragha_application_get_pane               (PraghaApplication *pragha);

PraghaMpris2      *pragha_application_get_mpris2             (PraghaApplication *pragha);
#ifdef HAVE_LIBCLASTFM
PraghaLastfm      *pragha_application_get_lastfm             (PraghaApplication *pragha);
#endif
#ifdef HAVE_LIBGLYR
PraghaGlyr        *pragha_application_get_glyr               (PraghaApplication *pragha);
#endif

PraghaNotify      *pragha_application_get_notify             (PraghaApplication *pragha);
void               pragha_application_set_notify             (PraghaApplication *pragha, PraghaNotify *notify);

gboolean           pragha_application_is_first_run           (PraghaApplication *pragha);

/* Info bar import music */

gboolean info_bar_import_music_will_be_useful(PraghaApplication *pragha);
GtkWidget* create_info_bar_import_music(PraghaApplication *pragha);
GtkWidget* create_info_bar_update_music(PraghaApplication *pragha);

/* Init */

gint init_options(PraghaApplication *pragha, int argc, char **argv);

/* Close */

void pragha_application_quit (void);

#endif /* PRAGHA_H */
