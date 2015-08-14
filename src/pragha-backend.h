/*************************************************************************/
/* Copyright (C) 2010-2015 matias <mati86dl@gmail.com>                   */
/* Copyright (C) 2012-2013 Pavel Vasin                                   */
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

#ifndef PRAGHA_BACKEND_H
#define PRAGHA_BACKEND_H

#include <gst/gst.h>
#include <glib-object.h>
#include "pragha-musicobject.h"

G_BEGIN_DECLS

typedef enum {
	ST_PLAYING = 1,
	ST_STOPPED,
	ST_PAUSED
} PraghaBackendState;

#define PRAGHA_TYPE_BACKEND                  (pragha_backend_get_type ())
#define PRAGHA_BACKEND(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), PRAGHA_TYPE_BACKEND, PraghaBackend))
#define PRAGHA_IS_BACKEND(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PRAGHA_TYPE_BACKEND))
#define PRAGHA_BACKEND_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), PRAGHA_TYPE_BACKEND, PraghaBackendClass))
#define PRAGHA_IS_BACKEND_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), PRAGHA_TYPE_BACKEND))
#define PRAGHA_BACKEND_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), PRAGHA_TYPE_BACKEND, PraghaBackendClass))

struct PraghaBackendPrivate;
typedef struct PraghaBackendPrivate PraghaBackendPrivate;

typedef struct {
	GObject parent;
	PraghaBackendPrivate *priv;
} PraghaBackend;

typedef struct {
	GObjectClass parent_class;
	void (*set_device) (PraghaBackend *backend, GObject *obj);
	void (*prepare_source) (PraghaBackend *backend);
	void (*clean_source) (PraghaBackend *backend);
	void (*tick) (PraghaBackend *backend);
	void (*seeked) (PraghaBackend *backend);
	void (*buffering) (PraghaBackend *backend, gint percent);
	void (*finished) (PraghaBackend *backend);
	void (*error) (PraghaBackend *backend, const GError *error);
	void (*tags_changed) (PraghaBackend *backend, gint changed);
} PraghaBackendClass;

gboolean           pragha_backend_can_seek             (PraghaBackend *backend);
void               pragha_backend_seek                 (PraghaBackend *backend, gint64 seek);

gint64             pragha_backend_get_current_length   (PraghaBackend *backend);
gint64             pragha_backend_get_current_position (PraghaBackend *backend);

void               pragha_backend_set_soft_volume      (PraghaBackend *backend, gboolean value);
gdouble            pragha_backend_get_volume           (PraghaBackend *backend);
void               pragha_backend_set_volume           (PraghaBackend *backend, gdouble volume);
void               pragha_backend_set_delta_volume     (PraghaBackend *backend, gdouble delta);

gboolean           pragha_backend_is_playing           (PraghaBackend *backend);
gboolean           pragha_backend_is_paused            (PraghaBackend *backend);

gboolean           pragha_backend_emitted_error        (PraghaBackend *backend);
GError            *pragha_backend_get_error            (PraghaBackend *backend);
PraghaBackendState pragha_backend_get_state            (PraghaBackend *backend);

void               pragha_backend_pause                (PraghaBackend *backend);
void               pragha_backend_resume               (PraghaBackend *backend);
void               pragha_backend_play                 (PraghaBackend *backend);
void               pragha_backend_stop                 (PraghaBackend *backend);

void               pragha_backend_set_playback_uri     (PraghaBackend *backend, const gchar *uri);
void               pragha_backend_set_musicobject      (PraghaBackend *backend, PraghaMusicobject *mobj);
PraghaMusicobject *pragha_backend_get_musicobject      (PraghaBackend *backend);

GstElement        *pragha_backend_get_equalizer        (PraghaBackend *backend);
void               pragha_backend_update_equalizer     (PraghaBackend *backend, const gdouble *bands);
GstElement        *pragha_backend_get_preamp           (PraghaBackend *backend);

PraghaBackend     *pragha_backend_new                  (void);

G_END_DECLS

#endif /* PRAGHA_BACKEND_H */
