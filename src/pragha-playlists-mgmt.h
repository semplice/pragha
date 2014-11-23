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

#ifndef PRAGHA_PLAYLISTS_MGMT_H
#define PRAGHA_PLAYLISTS_MGMT_H

#include "pragha-database.h"
#include "pragha-playlist.h"
#include "pragha.h"

/* Playlist management */

typedef enum {
	SAVE_COMPLETE,
	SAVE_SELECTED
} PraghaPlaylistActionRange;

#define SAVE_PLAYLIST_STATE         "con_playlist"

gchar *get_playlist_name(PraghaPlaylistActionRange type, GtkWidget *parent);
void add_playlist_current_playlist(gchar *splaylist, PraghaApplication *pragha);
GList * add_playlist_to_mobj_list(PraghaDatabase *cdbase, const gchar *playlist, GList *list);
GList *add_radio_to_mobj_list(PraghaDatabase *cdbase, const gchar *playlist, GList *list);
gboolean delete_existing_item_dialog(const gchar *item, GtkWidget *parent);
gchar* rename_playlist_dialog(const gchar *oplaylist, GtkWidget *parent);
GIOChannel *create_m3u_playlist(gchar *file);
gint save_m3u_playlist(GIOChannel *chan, gchar *playlist, gchar *filename, PraghaDatabase *cdbase);
gchar *playlist_export_dialog_get_filename(const gchar *prefix, GtkWidget *parent);

void export_playlist (PraghaPlaylist* cplaylist, PraghaPlaylistActionRange choice);
void save_playlist(PraghaPlaylist* cplaylist, gint playlist_id, PraghaPlaylistActionRange type);
void new_playlist(PraghaPlaylist* cplaylist, const gchar *playlist, PraghaPlaylistActionRange type);
void append_playlist(PraghaPlaylist* cplaylist, const gchar *playlist, PraghaPlaylistActionRange type);

void pragha_playlist_save_selection (PraghaPlaylist *playlist, const gchar *name);
void pragha_playlist_save_playlist  (PraghaPlaylist *playlist, const gchar *name);

GList *
pragha_pl_parser_append_mobj_list_by_extension (GList *mlist, const gchar *file);
GSList *pragha_pl_parser_parse_from_file_by_extension (const gchar *filename);
GSList *pragha_totem_pl_parser_parse_from_uri(const gchar *uri);
void pragha_pl_parser_open_from_file_by_extension(const gchar *file, PraghaApplication *pragha);
gchar * pragha_pl_get_first_playlist_item (const gchar *uri);

void new_radio (PraghaPlaylist* cplaylist, const gchar *uri, const gchar *name);
void update_playlist_changes_on_menu (PraghaPlaylist *playlist);

#endif /* PRAGHA_PLAYLISTS_MGMT_H */