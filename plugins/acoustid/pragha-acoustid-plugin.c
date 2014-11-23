/*************************************************************************/
/* Copyright (C) 2014 matias <mati86dl@gmail.com>                        */
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

#include <gst/gst.h>

#include <libsoup/soup.h>

#include <libpeas/peas.h>

#include "src/pragha.h"
#include "src/pragha-playlist.h"
#include "src/pragha-playlists-mgmt.h"
#include "src/pragha-musicobject-mgmt.h"
#include "src/pragha-hig.h"
#include "src/pragha-utils.h"
#include "src/xml_helper.h"
#include "src/pragha-window.h"
#include "src/pragha-tagger.h"
#include "src/pragha-tags-dialog.h"

#include "plugins/pragha-plugin-macros.h"

#define PRAGHA_TYPE_ACOUSTID_PLUGIN         (pragha_acoustid_plugin_get_type ())
#define PRAGHA_ACOUSTID_PLUGIN(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), PRAGHA_TYPE_ACOUSTID_PLUGIN, PraghaAcoustidPlugin))
#define PRAGHA_ACOUSTID_PLUGIN_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), PRAGHA_TYPE_ACOUSTID_PLUGIN, PraghaAcoustidPlugin))
#define PRAGHA_IS_ACOUSTID_PLUGIN(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), PRAGHA_TYPE_ACOUSTID_PLUGIN))
#define PRAGHA_IS_ACOUSTID_PLUGIN_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), PRAGHA_TYPE_ACOUSTID_PLUGIN))
#define PRAGHA_ACOUSTID_PLUGIN_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), PRAGHA_TYPE_ACOUSTID_PLUGIN, PraghaAcoustidPluginClass))

struct _PraghaAcoustidPluginPrivate {
	PraghaApplication *pragha;

	PraghaMusicobject *mobj;

	GtkActionGroup    *action_group_main_menu;
	guint              merge_id_main_menu;
};
typedef struct _PraghaAcoustidPluginPrivate PraghaAcoustidPluginPrivate;

PRAGHA_PLUGIN_REGISTER (PRAGHA_TYPE_ACOUSTID_PLUGIN,
                        PraghaAcoustidPlugin,
                        pragha_acoustid_plugin)

/*
 * Prototypes
 */
static void pragha_acoustid_get_metadata_dialog (PraghaAcoustidPlugin *plugin);

/*
 * Popups
 */
static void
pragha_acoustid_plugin_get_metadata_action (GtkAction *action, PraghaAcoustidPlugin *plugin)
{
	PraghaBackend *backend;

	PraghaAcoustidPluginPrivate *priv = plugin->priv;

	CDEBUG(DBG_PLUGIN, "Get Metadata action");

	backend = pragha_application_get_backend (priv->pragha);
	if (pragha_backend_get_state (backend) == ST_STOPPED)
		return;

	pragha_acoustid_get_metadata_dialog (plugin);
}

static const GtkActionEntry main_menu_actions [] = {
	{"Search metadata", NULL, N_("Search Metadata"),
	 "", "Search metadata", G_CALLBACK(pragha_acoustid_plugin_get_metadata_action)}
};

static const gchar *main_menu_xml = "<ui>						\
	<menubar name=\"Menubar\">									\
		<menu action=\"ToolsMenu\">								\
			<placeholder name=\"pragha-plugins-placeholder\">	\
				<menuitem action=\"Search metadata\"/>			\
				<separator/>									\
			</placeholder>										\
		</menu>													\
	</menubar>													\
</ui>";

/*
 * AcoustID Handlers
 */


static void
pragha_acoustid_dialog_response (GtkWidget            *dialog,
                                 gint                  response_id,
                                 PraghaAcoustidPlugin *plugin)
{
	PraghaBackend *backend;
	PraghaPlaylist *playlist;
	PraghaToolbar *toolbar;
	PraghaMusicobject *nmobj, *current_mobj;
	PraghaTagger *tagger;
	gint changed = 0;

	PraghaAcoustidPluginPrivate *priv = plugin->priv;

	if (response_id == GTK_RESPONSE_HELP) {
		nmobj = pragha_tags_dialog_get_musicobject(PRAGHA_TAGS_DIALOG(dialog));
		pragha_track_properties_dialog(nmobj, pragha_application_get_window(priv->pragha));
		return;
	}

	if (response_id == GTK_RESPONSE_OK) {
		changed = pragha_tags_dialog_get_changed(PRAGHA_TAGS_DIALOG(dialog));
		if (changed) {
			backend = pragha_application_get_backend (priv->pragha);

			nmobj = pragha_tags_dialog_get_musicobject(PRAGHA_TAGS_DIALOG(dialog));

			if (pragha_backend_get_state (backend) != ST_STOPPED) {
				current_mobj = pragha_backend_get_musicobject (backend);
				if (pragha_musicobject_compare (nmobj, current_mobj) == 0) {
					toolbar = pragha_application_get_toolbar (priv->pragha);

					/* Update public current song */
					pragha_update_musicobject_change_tag (current_mobj, changed, nmobj);

					/* Update current song on playlist */
					playlist = pragha_application_get_playlist (priv->pragha);
					pragha_playlist_update_current_track (playlist, changed, nmobj);

					pragha_toolbar_set_title(toolbar, current_mobj);
				}
			}

			if (G_LIKELY(pragha_musicobject_is_local_file (nmobj))) {
				tagger = pragha_tagger_new();
				pragha_tagger_add_file (tagger, pragha_musicobject_get_file(nmobj));
				pragha_tagger_set_changes (tagger, nmobj, changed);
				pragha_tagger_apply_changes (tagger);
				g_object_unref(tagger);
			}
		}
	}

	gtk_widget_destroy (dialog);
}

static void
pragha_acoustid_plugin_get_metadata_done (SoupSession *session,
                                          SoupMessage *msg,
                                          gpointer     user_data)
{
	GtkWidget *dialog;
	GtkWidget *window;
	PraghaStatusbar *statusbar;
	XMLNode *xml = NULL, *xi;
	gchar *otitle = NULL, *oartist = NULL, *oalbum = NULL;
	gchar *ntitle = NULL, *nartist = NULL, *nalbum = NULL;
	gint prechanged = 0;

	PraghaAcoustidPlugin *plugin = user_data;
	PraghaAcoustidPluginPrivate *priv = plugin->priv;

	window = pragha_application_get_window (priv->pragha);
	remove_watch_cursor (window);

	if (!SOUP_STATUS_IS_SUCCESSFUL (msg->status_code))
		return;

	g_object_get (priv->mobj,
	              "title", &otitle,
	              "artist", &oartist,
	              "album", &oalbum,
	              NULL);

	xml = tinycxml_parse ((gchar *)msg->response_body->data);

	xi = xmlnode_get (xml, CCA{"response", "results", "result", "recordings", "recording", "title", NULL }, NULL, NULL);
	if (xi && string_is_not_empty(xi->content)) {
		ntitle = unescape_HTML (xi->content);
		if (g_strcmp0(otitle, ntitle)) {
			pragha_musicobject_set_title (priv->mobj, ntitle);
			prechanged |= TAG_TITLE_CHANGED;
		}
		g_free (ntitle);
	}

	xi = xmlnode_get (xml, CCA{"response", "results", "result", "recordings", "recording", "artists", "artist", "name", NULL }, NULL, NULL);
	if (xi && string_is_not_empty(xi->content)) {
		nartist = unescape_HTML (xi->content);
		if (g_strcmp0(oartist, nartist)) {
			pragha_musicobject_set_artist (priv->mobj, nartist);
			prechanged |= TAG_ARTIST_CHANGED;
		}
		g_free (nartist);
	}

	xi = xmlnode_get (xml, CCA{"response", "results", "result", "recordings", "recording", "releasegroups", "releasegroup", "title", NULL }, NULL, NULL);
	if (xi && string_is_not_empty(xi->content)) {
		nalbum = unescape_HTML (xi->content);
		if (g_strcmp0(oalbum, nalbum)) {
			pragha_musicobject_set_album (priv->mobj, nalbum);
			prechanged |= TAG_ALBUM_CHANGED;
		}
		g_free (nalbum);
	}

	if (prechanged)	{
		dialog = pragha_tags_dialog_new ();

		g_signal_connect (G_OBJECT (dialog), "response",
			              G_CALLBACK (pragha_acoustid_dialog_response), plugin);

		pragha_tags_dialog_set_musicobject (PRAGHA_TAGS_DIALOG(dialog), priv->mobj);
		pragha_tags_dialog_set_changed (PRAGHA_TAGS_DIALOG(dialog), prechanged);

		gtk_widget_show (dialog);
	}
	else {
		statusbar = pragha_statusbar_get ();
		pragha_statusbar_set_misc_text (statusbar, _("AcoustID not found any similar song"));
		g_object_unref (statusbar);
	}

	g_free (otitle);
	g_free (oartist);
	g_free (oalbum);

	g_object_unref (priv->mobj);
	xmlnode_free (xml);
}

static void
pragha_acoustid_plugin_get_metadata (PraghaAcoustidPlugin *plugin, gint duration, const gchar *fingerprint)
{
	SoupSession *session;
	SoupMessage *msg;
	gchar *query = NULL;

	query = g_strdup_printf ("http://api.acoustid.org/v2/lookup?client=%s&meta=%s&format=%s&duration=%d&fingerprint=%s",
	                         "yPvUXBmO", "recordings+releasegroups+compress", "xml", duration, fingerprint);

	session = soup_session_sync_new ();

	msg = soup_message_new ("GET", query);
	soup_session_queue_message (session, msg,
	                            pragha_acoustid_plugin_get_metadata_done, plugin);

	g_free (query);
}

static void
error_cb (GstBus *bus, GstMessage *msg, void *data)
{
	GError *err;
	gchar *debug_info;
   
	/* Print error details on the screen */
	gst_message_parse_error (msg, &err, &debug_info);
	g_printerr ("Error received from element %s: %s\n", GST_OBJECT_NAME (msg->src), err->message);
	g_printerr ("Debugging information: %s\n", debug_info ? debug_info : "none");
	g_clear_error (&err);
	g_free (debug_info);
}

static gboolean
pragha_acoustid_get_fingerprint (const gchar *filename, gchar **fingerprint)
{
	GstElement *pipeline, *chromaprint;
	GstBus *bus;
	GstMessage *msg;
	gchar *uri, *pipestring = NULL;

	uri = g_filename_to_uri(filename, NULL, NULL);
	pipestring = g_strdup_printf("uridecodebin uri=%s ! audioconvert ! chromaprint name=chromaprint0 ! fakesink", uri);
	g_free (uri);

	pipeline = gst_parse_launch (pipestring, NULL);

	bus = gst_element_get_bus (pipeline);
	g_signal_connect (G_OBJECT (bus), "message::error", (GCallback)error_cb, NULL);

	gst_element_set_state (pipeline, GST_STATE_PLAYING);

	msg = gst_bus_timed_pop_filtered (bus, GST_CLOCK_TIME_NONE, GST_MESSAGE_ERROR | GST_MESSAGE_EOS);
	if (msg != NULL)
		gst_message_unref (msg);
	gst_object_unref (bus);

	gst_element_set_state (pipeline, GST_STATE_NULL);

	chromaprint = gst_bin_get_by_name (GST_BIN(pipeline), "chromaprint0");
	g_object_get (chromaprint, "fingerprint", fingerprint, NULL);

	gst_object_unref (pipeline);
	g_free (pipestring);

	return TRUE;
}

/*
 * AcoustID dialog
 */
static void
pragha_acoustid_get_metadata_dialog (PraghaAcoustidPlugin *plugin)
{
	GtkWidget *window;
	PraghaBackend *backend = NULL;
	PraghaMusicobject *mobj = NULL;
	const gchar *file = NULL;
	gchar *fingerprint = NULL;
	gint duration = 0;

	PraghaAcoustidPluginPrivate *priv = plugin->priv;

	backend = pragha_application_get_backend (priv->pragha);
	mobj = pragha_backend_get_musicobject (backend);

	priv->mobj = pragha_musicobject_dup (mobj);

	file = pragha_musicobject_get_file (mobj);
	duration = pragha_musicobject_get_length (mobj);

	window = pragha_application_get_window (priv->pragha);
	set_watch_cursor (window);

	if (pragha_acoustid_get_fingerprint (file, &fingerprint))
		pragha_acoustid_plugin_get_metadata (plugin, duration, fingerprint);
	else
		remove_watch_cursor (window);

	g_free (fingerprint);
}

/*
 * AcoustID plugin
 */
static void
pragha_plugin_activate (PeasActivatable *activatable)
{
	PraghaAcoustidPlugin *plugin = PRAGHA_ACOUSTID_PLUGIN (activatable);

	PraghaAcoustidPluginPrivate *priv = plugin->priv;
	priv->pragha = g_object_get_data (G_OBJECT (plugin), "object");

	CDEBUG(DBG_PLUGIN, "AcustId plugin %s", G_STRFUNC);

	/* Attach main menu */

	priv->action_group_main_menu = gtk_action_group_new ("PraghaAcoustidPlugin");
	gtk_action_group_set_translation_domain (priv->action_group_main_menu, GETTEXT_PACKAGE);
	gtk_action_group_add_actions (priv->action_group_main_menu,
	                              main_menu_actions,
	                              G_N_ELEMENTS (main_menu_actions),
	                              plugin);

	priv->merge_id_main_menu = pragha_menubar_append_plugin_action (priv->pragha,
	                                                                priv->action_group_main_menu,
	                                                                main_menu_xml);
}

static void
pragha_plugin_deactivate (PeasActivatable *activatable)
{
	PraghaAcoustidPlugin *plugin = PRAGHA_ACOUSTID_PLUGIN (activatable);
	PraghaAcoustidPluginPrivate *priv = plugin->priv;

	CDEBUG(DBG_PLUGIN, "AcustID plugin %s", G_STRFUNC);

	pragha_menubar_remove_plugin_action (priv->pragha,
	                                     priv->action_group_main_menu,
	                                     priv->merge_id_main_menu);
	priv->merge_id_main_menu = 0;
}