/*
 * Copyright (C) 2009 Rodney Cryderman <rcryderman@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *
 */
 

/* awn-overlay-pixbuf.h */

#ifndef _AWN_OVERLAY_PIXBUF
#define _AWN_OVERLAY_PIXBUF

#include <glib-object.h>
#include <gdk/gdk.h>

#include "awn-overlay.h"

G_BEGIN_DECLS

#define AWN_TYPE_OVERLAY_PIXBUF awn_overlay_pixbuf_get_type()

#define AWN_OVERLAY_PIXBUF(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), AWN_TYPE_OVERLAY_PIXBUF, AwnOverlayPixbuf))

#define AWN_OVERLAY_PIXBUF_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), AWN_TYPE_OVERLAY_PIXBUF, AwnOverlayPixbufClass))

#define AWN_IS_OVERLAY_PIXBUF(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), AWN_TYPE_OVERLAY_PIXBUF))

#define AWN_IS_OVERLAY_PIXBUF_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), AWN_TYPE_OVERLAY_PIXBUF))

#define AWN_OVERLAY_PIXBUF_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), AWN_TYPE_OVERLAY_PIXBUF, AwnOverlayPixbufClass))

typedef struct {
  AwnOverlay parent;
} AwnOverlayPixbuf;

typedef struct {
  AwnOverlayClass parent_class;
} AwnOverlayPixbufClass;

GType awn_overlay_pixbuf_get_type (void);

AwnOverlayPixbuf* awn_overlay_pixbuf_new (void);

AwnOverlayPixbuf* awn_overlay_pixbuf_new_with_pixbuf (GdkPixbuf * pixbuf);

G_END_DECLS

#endif /* _AWN_OVERLAY_PIXBUF */
