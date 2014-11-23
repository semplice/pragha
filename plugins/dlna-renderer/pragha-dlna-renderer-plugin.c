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

#include <gio/gio.h>

#include <libpeas/peas.h>
#include <libpeas-gtk/peas-gtk.h>

#include <grilo.h>

#include "pragha-dlna-renderer-plugin.h"

#include "src/pragha.h"
#include "src/pragha-utils.h"
#include "src/pragha-musicobject-mgmt.h"
#include "src/pragha-playlist.h"
#include "src/pragha-musicobject.h"
#include "src/pragha-musicobject-mgmt.h"
#include "src/pragha-window.h"

#include "plugins/pragha-plugin-macros.h"

typedef struct _PraghaDlnaRendererPluginPrivate PraghaDlnaRendererPluginPrivate;

struct _PraghaDlnaRendererPluginPrivate {
	PraghaApplication    *pragha;

	GtkActionGroup       *action_group_main_menu;
	guint                 merge_id_main_menu;
};

PRAGHA_PLUGIN_REGISTER (PRAGHA_TYPE_DLNA_RENDERER_PLUGIN,
                        PraghaDlnaRendererPlugin,
                        pragha_dlna_renderer_plugin)
/*
 *
 */
static void
pragha_dlna_renderer_plugin_search_music (PraghaDlnaRendererPlugin *plugin);

/*
 * Popups
 */
static void
pragha_dlna_renderer_plugin_search_music_action (GtkAction *action, PraghaDlnaRendererPlugin *plugin)
{
	pragha_dlna_renderer_plugin_search_music (plugin);
}

static const GtkActionEntry main_menu_actions [] = {
	{"Search dlna music", NULL, N_("Search music on DLNA server"),
	 "", "Search dlna music", G_CALLBACK(pragha_dlna_renderer_plugin_search_music_action)}
};

static const gchar *main_menu_xml = "<ui>						\
	<menubar name=\"Menubar\">									\
		<menu action=\"ToolsMenu\">								\
			<placeholder name=\"pragha-plugins-placeholder\">	\
				<menuitem action=\"Search dlna music\"/>		\
				<separator/>									\
			</placeholder>										\
		</menu>													\
	</menubar>													\
</ui>";

static GList *
pragha_dlna_renderer_append_media (GList *list, GrlMedia *media)
{
	PraghaMusicobject *mobj;
	const gchar *title = NULL, *url = NULL;
	guint seconds = 0;

	url = grl_media_get_url (media);
	title = grl_media_get_title (media);
	seconds = grl_media_get_duration (media);

	mobj = g_object_new (PRAGHA_TYPE_MUSICOBJECT,
	                     "file", url,
	                     "source", FILE_HTTP,
	                     "title", title,
	                     "length", seconds,
	                     NULL);

	if (G_LIKELY(mobj))
		list = g_list_prepend (list, mobj);

	return list;
}

static GList *
pragha_dlna_renderer_append_source (GList     *list,
                                    GrlSource *source,
                                    GrlMedia  *container)
{
	GrlOperationOptions *options;
	GrlCaps *caps;
	GrlMedia *media;
	GList *keys = NULL;
	GList *medias = NULL, *media_iter;

	keys = grl_metadata_key_list_new (GRL_METADATA_KEY_TITLE,
	                                  GRL_METADATA_KEY_DURATION,
	                                  GRL_METADATA_KEY_URL,
	                                  GRL_METADATA_KEY_CHILDCOUNT,
	                                  GRL_METADATA_KEY_INVALID);

	caps = grl_source_get_caps (source, GRL_OP_BROWSE);
	options = grl_operation_options_new (caps);

	grl_operation_options_set_flags (options, GRL_RESOLVE_IDLE_RELAY);

	medias = grl_source_browse_sync (source, container, keys, options, NULL);
	for (media_iter = medias; media_iter; media_iter = g_list_next (media_iter)) {
		if (media_iter->data == NULL)
			continue;

		media = GRL_MEDIA (media_iter->data);

		if (GRL_IS_MEDIA_BOX (media)) {
			list = pragha_dlna_renderer_append_source (list, source, media);
		}
		else if (GRL_IS_MEDIA_AUDIO (media)) {
			list = pragha_dlna_renderer_append_media (list, media);
		}
		pragha_process_gtk_events ();

		g_object_unref (media);
	}

	g_object_unref (options);

	g_list_free (keys);
	g_list_free (medias);

	return list;
}

static void
pragha_dlna_renderer_plugin_search_music (PraghaDlnaRendererPlugin *plugin)
{
	PraghaPlaylist *playlist;
	PraghaStatusbar *statusbar;
	GList *sources = NULL, *sources_iter;
	GrlRegistry *registry;
	GList *list = NULL;

	CDEBUG(DBG_PLUGIN, "DLNA Renderer plugin %s", G_STRFUNC);

	registry = grl_registry_get_default ();

	sources = grl_registry_get_sources_by_operations (registry, GRL_OP_BROWSE, FALSE);
	for (sources_iter = sources; sources_iter; sources_iter = g_list_next (sources_iter)) {
		list = pragha_dlna_renderer_append_source (list, GRL_SOURCE(sources_iter->data), NULL);
		if (list)
			break;
	}

	statusbar = pragha_statusbar_get ();

	if (list) {
		playlist = pragha_application_get_playlist (plugin->priv->pragha);

		pragha_playlist_append_mobj_list (playlist, list);
		g_list_free (list);

		const gchar *server = grl_source_get_name (GRL_SOURCE(sources_iter->data));
		gchar *msge = g_strdup_printf (_("Music of the %s server was added."), server);
		pragha_statusbar_set_misc_text (statusbar, msge);
		g_free (msge);
	}
	else {
		pragha_statusbar_set_misc_text (statusbar, _("Could not find any DLNA server."));
	}

	g_object_unref (statusbar);

	g_list_free (sources);
}

/*
 * Plugin.
 */
static void
pragha_plugin_activate (PeasActivatable *activatable)
{
	GrlRegistry *registry;
	GError *error = NULL;

	PraghaDlnaRendererPlugin *plugin = PRAGHA_DLNA_RENDERER_PLUGIN (activatable);

	PraghaDlnaRendererPluginPrivate *priv = plugin->priv;
	priv->pragha = g_object_get_data (G_OBJECT (plugin), "object");

	CDEBUG(DBG_PLUGIN, "DLNA Renderer plugin %s", G_STRFUNC);

	grl_init (NULL, NULL);

	registry = grl_registry_get_default ();
	if (!grl_registry_load_plugin_by_id (registry, "grl-upnp", &error)) {
		g_print ("Failed to load plugins: %s\n\n", error->message);
 	}

	/* Attach main menu */

	priv->action_group_main_menu = gtk_action_group_new ("PraghaDlnaPlugin");
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
	PraghaDlnaRendererPlugin *plugin = PRAGHA_DLNA_RENDERER_PLUGIN (activatable);

	PraghaDlnaRendererPluginPrivate *priv = plugin->priv;

	CDEBUG(DBG_PLUGIN, "DLNA Renderer plugin %s", G_STRFUNC);

	pragha_menubar_remove_plugin_action (priv->pragha,
	                                     priv->action_group_main_menu,
	                                     priv->merge_id_main_menu);
	priv->merge_id_main_menu = 0;

	grl_deinit ();
}