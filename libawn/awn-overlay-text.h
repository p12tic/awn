/*
 * Copyright (C) 2009 Rodney Cryderman <rcryderman@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Library General Public License version 
 * 2 or later as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *
 */

/* awn-overlay-text.h */

#ifndef _AWN_OVERLAY_TEXT
#define _AWN_OVERLAY_TEXT

#include <glib-object.h>
#include "awn-overlay.h"

G_BEGIN_DECLS

#define AWN_TYPE_OVERLAY_TEXT awn_overlay_text_get_type()

#define AWN_OVERLAY_TEXT(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), AWN_TYPE_OVERLAY_TEXT, AwnOverlayText))

#define AWN_OVERLAY_TEXT_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), AWN_TYPE_OVERLAY_TEXT, AwnOverlayTextClass))

#define AWN_IS_OVERLAY_TEXT(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), AWN_TYPE_OVERLAY_TEXT))

#define AWN_IS_OVERLAY_TEXT_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), AWN_TYPE_OVERLAY_TEXT))

#define AWN_OVERLAY_TEXT_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), AWN_TYPE_OVERLAY_TEXT, AwnOverlayTextClass))

typedef struct {
  AwnOverlay parent;
} AwnOverlayText;

typedef struct {
  AwnOverlayClass parent_class;
} AwnOverlayTextClass;

GType awn_overlay_text_get_type (void);

AwnOverlayText* awn_overlay_text_new (void);

void awn_overlay_text_get_size (AwnOverlayText* overlay,
                                GtkWidget * widget,
                                gchar *text,
                                gint size,
                                gint *width, gint *height);

G_END_DECLS

#endif /* _AWN_OVERLAY_TEXT */

