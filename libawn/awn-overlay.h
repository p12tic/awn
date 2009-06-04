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


/* awn-overlay.h */

#ifndef _AWN_OVERLAY
#define _AWN_OVERLAY

#include <glib-object.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <cairo/cairo-xlib.h>
#include <gdk/gdk.h>

#include "awn-defines.h"
#include "awn-themed-icon.h"

G_BEGIN_DECLS

#define AWN_TYPE_OVERLAY awn_overlay_get_type()

#define AWN_OVERLAY(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), AWN_TYPE_OVERLAY, AwnOverlay))

#define AWN_OVERLAY_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), AWN_TYPE_OVERLAY, AwnOverlayClass))

#define AWN_IS_OVERLAY(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), AWN_TYPE_OVERLAY))

#define AWN_IS_OVERLAY_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), AWN_TYPE_OVERLAY))

#define AWN_OVERLAY_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), AWN_TYPE_OVERLAY, AwnOverlayClass))


typedef struct {
  GObject parent;
  AwnOverlayType  overlay_type;  
} AwnOverlay;

typedef struct {
  GObjectClass parent_class;

  void          (*render_overlay)         (AwnOverlay* overlay,
                                          AwnThemedIcon * icon,
                                          cairo_t * cr,                                 
                                          gint width,
                                          gint height);
} AwnOverlayClass;

GType awn_overlay_get_type (void);

AwnOverlay* awn_overlay_new (void);

void awn_overlay_render_overlay    (AwnOverlay* overlay,
                                        AwnThemedIcon * icon,
                                        cairo_t * cr,                                 
                                        gint width,
                                        gint height);


GdkGravity  awn_overlay_get_gravity (AwnOverlay * overlay);

void awn_overlay_move_to (cairo_t * cr,
                           AwnOverlay* overlay,
                           gint   icon_width,
                           gint   icon_height,
                           gint   overlay_width,
                           gint   overlay_height
                           );
G_END_DECLS

#endif /* _AWN_OVERLAY */
