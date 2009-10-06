/*
 * Copyright (C) 2009 Michal Hruby <michal.mhr@gmail.com>
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
 */

/* awn-overlayable.h */

#ifndef _AWN_OVERLAYABLE
#define _AWN_OVERLAYABLE

#include <glib-object.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include "awn-defines.h"
#include "awn-effects.h"
#include "awn-overlay.h"

G_BEGIN_DECLS

#define AWN_TYPE_OVERLAYABLE awn_overlayable_get_type()

#define AWN_OVERLAYABLE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), AWN_TYPE_OVERLAYABLE, AwnOverlayable))

#define AWN_IS_OVERLAYABLE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), AWN_TYPE_OVERLAYABLE))

#define AWN_OVERLAYABLE_GET_INTERFACE(inst) \
  (G_TYPE_INSTANCE_GET_INTERFACE ((inst), AWN_TYPE_OVERLAYABLE, \
   AwnOverlayableIface))

typedef struct _AwnOverlayable AwnOverlayable;
typedef struct _AwnOverlayableIface AwnOverlayableIface;

struct _AwnOverlayableIface
{
  GTypeInterface parent;

  AwnEffects*   (*get_effects)          (AwnOverlayable* self);
};

GType awn_overlayable_get_type (void);

AwnEffects* awn_overlayable_get_effects   (AwnOverlayable* self);

void   awn_overlayable_add_overlay        (AwnOverlayable* self,
                                           AwnOverlay *overlay);

void   awn_overlayable_remove_overlay     (AwnOverlayable *self,
                                           AwnOverlay *overlay);

GList* awn_overlayable_get_overlays       (AwnOverlayable *self);


G_END_DECLS

#endif /* _AWN_OVERLAYABLE */
