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

/* awn-overlay-progress-circle.h */

#ifndef _AWN_OVERLAY_PROGRESS_CIRCLE
#define _AWN_OVERLAY_PROGRESS_CIRCLE

#include <glib-object.h>
#include "awn-overlay-progress.h"

G_BEGIN_DECLS

#define AWN_TYPE_OVERLAY_PROGRESS_CIRCLE awn_overlay_progress_circle_get_type()

#define AWN_OVERLAY_PROGRESS_CIRCLE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), AWN_TYPE_OVERLAY_PROGRESS_CIRCLE, AwnOverlayProgressCircle))

#define AWN_OVERLAY_PROGRESS_CIRCLE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), AWN_TYPE_OVERLAY_PROGRESS_CIRCLE, AwnOverlayProgressCircleClass))

#define AWN_IS_OVERLAY_PROGRESS_CIRCLE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), AWN_TYPE_OVERLAY_PROGRESS_CIRCLE))

#define AWN_IS_OVERLAY_PROGRESS_CIRCLE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), AWN_TYPE_OVERLAY_PROGRESS_CIRCLE))

#define AWN_OVERLAY_PROGRESS_CIRCLE_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), AWN_TYPE_OVERLAY_PROGRESS_CIRCLE, AwnOverlayProgressCircleClass))

typedef struct {
  AwnOverlayProgress parent;
} AwnOverlayProgressCircle;

typedef struct {
  AwnOverlayProgressClass parent_class;
} AwnOverlayProgressCircleClass;

GType awn_overlay_progress_circle_get_type (void);

AwnOverlayProgressCircle* awn_overlay_progress_circle_new (void);

G_END_DECLS

#endif /* _AWN_OVERLAY_PROGRESS_CIRCLE */
