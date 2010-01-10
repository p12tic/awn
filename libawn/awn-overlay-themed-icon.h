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

/* awn-overlay-themed-icon.h */

#ifndef _AWN_OVERLAY_THEMED_ICON
#define _AWN_OVERLAY_THEMED_ICON

#include <glib-object.h>

#include "awn-overlay.h"
#include "awn-themed-icon.h"

G_BEGIN_DECLS

#define AWN_TYPE_OVERLAY_THEMED_ICON awn_overlay_themed_icon_get_type()

#define AWN_OVERLAY_THEMED_ICON(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), AWN_TYPE_OVERLAY_THEMED_ICON, AwnOverlayThemedIcon))

#define AWN_OVERLAY_THEMED_ICON_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), AWN_TYPE_OVERLAY_THEMED_ICON, AwnOverlayThemedIconClass))

#define AWN_IS_OVERLAY_THEMED_ICON(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), AWN_TYPE_OVERLAY_THEMED_ICON))

#define AWN_IS_OVERLAY_THEMED_ICON_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), AWN_TYPE_OVERLAY_THEMED_ICON))

#define AWN_OVERLAY_THEMED_ICON_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), AWN_TYPE_OVERLAY_THEMED_ICON, AwnOverlayThemedIconClass))

typedef struct {
  AwnOverlay parent;
} AwnOverlayThemedIcon;

typedef struct {
  AwnOverlayClass parent_class;
} AwnOverlayThemedIconClass;

GType awn_overlay_themed_icon_get_type (void);

AwnOverlayThemedIcon* awn_overlay_themed_icon_new (const gchar * icon_name);

G_END_DECLS

#endif /* _AWN_OVERLAY_THEMED_ICON */
