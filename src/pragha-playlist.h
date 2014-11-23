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

#ifndef PRAGHA_PLAYLIST_H
#define PRAGHA_PLAYLIST_H

#include <gtk/gtk.h>
#include "pragha-backend.h"
#include "pragha-database.h"

#define PRAGHA_TYPE_PLAYLIST                  (pragha_playlist_get_type ())
#define PRAGHA_PLAYLIST(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), PRAGHA_TYPE_PLAYLIST, PraghaPlaylist))
#define PRAGHA_IS_PLAYLIST(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PRAGHA_TYPE_PLAYLIST))
#define PRAGHA_PLAYLIST_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), PRAGHA_TYPE_PLAYLIST, PraghaPlaylistClass))
#define PRAGHA_IS_PLAYLIST_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), PRAGHA_TYPE_PLAYLIST))
#define PRAGHA_PLAYLIST_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), PRAGHA_TYPE_PLAYLIST, PraghaPlaylistClass))

typedef struct _PraghaPlaylist PraghaPlaylist;

typedef struct {
	GtkScrolledWindowClass __parent__;
	void (*playlist_set_track) (PraghaPlaylist *playlist, PraghaMusicobject *mobj);
	void (*playlist_change_tags) (PraghaPlaylist *playlist, gint changes, PraghaMusicobject *mobj);
	void (*playlist_changed) (PraghaPlaylist *playlist);
} PraghaPlaylistClass;

/* Columns in current playlist view */

enum curplaylist_columns {
	P_MOBJ_PTR,
	P_QUEUE,
	P_BUBBLE,
	P_STATUS_PIXBUF,
	P_TRACK_NO,
	P_TITLE,
	P_ARTIST,
	P_ALBUM,
	P_GENRE,
	P_BITRATE,
	P_YEAR,
	P_COMMENT,
	P_LENGTH,
	P_FILENAME,
	P_MIMETYPE,
	P_PLAYED,
	N_P_COLUMNS
};

/* Current playlist movement */

typedef enum {
	PLAYLIST_NONE,
	PLAYLIST_CURR,
	PLAYLIST_NEXT,
	PLAYLIST_PREV
} PraghaUpdateAction;

void save_selected_playlist   (GtkAction *action, PraghaPlaylist *cplaylist);
void save_current_playlist    (GtkAction *action, PraghaPlaylist *cplaylist);
void export_current_playlist  (GtkAction *action, PraghaPlaylist *cplaylist);
void export_selected_playlist (GtkAction *action, PraghaPlaylist *cplaylist);

void pragha_playlist_remove_selection (PraghaPlaylist *playlist);
void pragha_playlist_crop_selection   (PraghaPlaylist *playlist);
void pragha_playlist_crop_music_type  (PraghaPlaylist *playlist, PraghaMusicSource music_type);
void pragha_playlist_remove_all       (PraghaPlaylist *playlist);

void pragha_playlist_go_prev_track    (PraghaPlaylist *playlist);
void pragha_playlist_go_any_track     (PraghaPlaylist *playlist);
void pragha_playlist_go_next_track    (PraghaPlaylist *playlist);
void pragha_playlist_stopped_playback (PraghaPlaylist *playlist);

void               pragha_playlist_show_current_track (PraghaPlaylist *playlist);
void               pragha_playlist_set_track_error    (PraghaPlaylist *playlist, GError *error);

void select_numered_path_of_current_playlist(PraghaPlaylist *cplaylist, gint path_number, gboolean center);

void update_current_playlist_view_playback_state_cb (PraghaBackend *backend, GParamSpec *pspec, PraghaPlaylist *cplaylist);

PraghaMusicobject * current_playlist_mobj_at_path(GtkTreePath *path,
						  PraghaPlaylist *cplaylist);

void toggle_queue_selected_current_playlist (PraghaPlaylist *cplaylist);

void pragha_playlist_update_current_track(PraghaPlaylist *cplaylist, gint changed, PraghaMusicobject *nmobj);
void
pragha_playlist_append_single_song(PraghaPlaylist *cplaylist, PraghaMusicobject *mobj);
void
pragha_playlist_append_mobj_and_play(PraghaPlaylist *cplaylist, PraghaMusicobject *mobj);
void
pragha_playlist_append_mobj_list(PraghaPlaylist *cplaylist, GList *list);
gboolean
pragha_mobj_list_already_has_title_of_artist(GList *list,
					     const gchar *title,
					     const gchar *artist);
gboolean
pragha_playlist_already_has_title_of_artist(PraghaPlaylist *cplaylist,
					    const gchar *title,
					    const gchar *artist);

GList *pragha_playlist_get_mobj_list(PraghaPlaylist* cplaylist);
GList *pragha_playlist_get_selection_mobj_list(PraghaPlaylist* cplaylist);
GList *pragha_playlist_get_selection_ref_list(PraghaPlaylist *cplaylist);

void pragha_playlist_save_playlist_state (PraghaPlaylist* cplaylist);
void pragha_playlist_init_playlist_state (PraghaPlaylist* cplaylist);
PraghaMusicobject *pragha_playlist_get_selected_musicobject(PraghaPlaylist* cplaylist);
gboolean pragha_playlist_propagate_event(PraghaPlaylist* cplaylist, GdkEventKey *event);

void pragha_playlist_activate_path        (PraghaPlaylist* cplaylist, GtkTreePath *path);
void pragha_playlist_activate_unique_mobj (PraghaPlaylist* cplaylist, PraghaMusicobject *mobj);

gint pragha_playlist_get_no_tracks      (PraghaPlaylist *playlist);
gint pragha_playlist_get_total_playtime (PraghaPlaylist *playlist);

gboolean pragha_playlist_has_queue(PraghaPlaylist* cplaylist);

gboolean pragha_playlist_is_changing (PraghaPlaylist* cplaylist);
void     pragha_playlist_set_changing (PraghaPlaylist* cplaylist, gboolean changing);

GtkWidget    *pragha_playlist_get_view  (PraghaPlaylist* cplaylist);
GtkTreeModel *pragha_playlist_get_model (PraghaPlaylist* cplaylist);

GtkUIManager   *pragha_playlist_get_context_menu(PraghaPlaylist* cplaylist);

gint            pragha_playlist_append_plugin_action (PraghaPlaylist *cplaylist, GtkActionGroup *action_group, const gchar *menu_xml);
void            pragha_playlist_remove_plugin_action (PraghaPlaylist *cplaylist, GtkActionGroup *action_group, gint merge_id);

PraghaDatabase *pragha_playlist_get_database(PraghaPlaylist* cplaylist);

PraghaPlaylist *pragha_playlist_new  (void);


#endif /* PRAGHA_PLAYLIST_H */
