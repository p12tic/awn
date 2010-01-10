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


/* awn-overlay.h */

#ifndef _AWN_OVERLAY
#define _AWN_OVERLAY

#include <glib-object.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include "awn-defines.h"

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
  GInitiallyUnowned parent;
} AwnOverlay;


/**
 * AwnOverlayClass:
 * @render: Virtual function of the form
 *  void render (AwnOverlay* overlay,
 *               GtkWidget *widget,
 *               cairo_t * cr,
 *               gint width,
 *               gint height)
 *
 * Of interest to implementors of #AwnOverlay subclasses.
 */
typedef struct
{
  GInitiallyUnownedClass parent_class;

  void          (*render)                (AwnOverlay* overlay,
                                          GtkWidget *widget,
                                          cairo_t * cr,
                                          gint width,
                                          gint height);
} AwnOverlayClass;

/**
 * AwnOverlayCoord:
 * @x: Horizontal position as a #gdouble.
 * @x: Vertical position as a #gdboule.
 *
 * Structure contains x,y coordinates.
 */
typedef struct
{
  gdouble x;
  gdouble y;
} AwnOverlayCoord;

/**
 * AwnOverlayAlign:
 * @AWN_OVERLAY_ALIGN_CENTRE: Centre alignment.
 * @AWN_OVERLAY_ALIGN_LEFT: Left alignment.
 * @AWN_OVERLAY_ALIGN_RIGHT: Right alignment.
 *
 * An enum for horizontal alignment relative to the #GdkGravity specified for
 * an #AwnOverlay.  Possible values are %AWN_OVERLAY_ALIGN_CENTRE,
 * %AWN_OVERLAY_ALIGN_LEFT and %AWN_OVERLAY_ALIGN_RIGHT
 */
typedef enum
{
  AWN_OVERLAY_ALIGN_CENTRE,
  AWN_OVERLAY_ALIGN_LEFT,
  AWN_OVERLAY_ALIGN_RIGHT
}AwnOverlayAlign;

GType awn_overlay_get_type (void);

AwnOverlay* awn_overlay_new (void);

void awn_overlay_render  (AwnOverlay* overlay,
                          GtkWidget *widget,
                          cairo_t * cr,
                          gint width,
                          gint height);

void awn_overlay_move_to (AwnOverlay* overlay,
                          cairo_t * cr,
                          gint   icon_width,
                          gint   icon_height,
                          gint   overlay_width,
                          gint   overlay_height,
                          AwnOverlayCoord * coord_req);

gboolean awn_overlay_get_apply_effects (AwnOverlay *overlay);

void awn_overlay_set_apply_effects (AwnOverlay *overlay, gboolean value);

gboolean awn_overlay_get_use_source_op (AwnOverlay *overlay);

void awn_overlay_set_use_source_op (AwnOverlay *overlay, gboolean value);

G_END_DECLS

#endif /* _AWN_OVERLAY */
