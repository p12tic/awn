/*
 *  Copyright (C) 2007 Neil Jagdish Patel <njpatel@gmail.com>
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
 *  Author : Neil Jagdish Patel <njpatel@gmail.com>
*/

#ifndef __AWN_TOOLTIP_H__
#define __AWN_TOOLTIP_H__

#include <libdesktop-agnostic/desktop-agnostic.h>
#include <gtk/gtk.h>

#include "awn-defines.h"

G_BEGIN_DECLS

#define AWN_TYPE_TOOLTIP		(awn_tooltip_get_type ())

#define AWN_TOOLTIP(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj),\
        AWN_TYPE_TOOLTIP,\
        AwnTooltip))

#define AWN_TOOLTIP_CLASS(obj)	(G_TYPE_CHECK_CLASS_CAST ((obj), \
        AWN_TYPE_TOOLTIP, \
        AwnTooltipClass))
                                
#define AWN_IS_TOOLTIP(obj)	(G_TYPE_CHECK_INSTANCE_TYPE ((obj),\
        AWN_TYPE_TOOLTIP))

#define AWN_IS_TOOLTIP_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE ((obj), \
        AWN_TYPE_TOOLTIP))

#define AWN_TOOLTIP_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), \
        AWN_TYPE_TOOLTIP, \
        AwnTooltipClass))

typedef struct _AwnTooltip AwnTooltip;
typedef struct _AwnTooltipClass	AwnTooltipClass;
typedef struct _AwnTooltipPrivate AwnTooltipPrivate;

struct _AwnTooltip
{
  GtkWindow parent;

  AwnTooltipPrivate *priv;
};

struct _AwnTooltipClass
{
  GtkWindowClass parent_class;

  /* Future padding */
  void (*_tooltip0) (void);
  void (*_tooltip1) (void);
  void (*_tooltip2) (void);
  void (*_tooltip3) (void);
};

GType         awn_tooltip_get_type             (void);

GtkWidget *   awn_tooltip_new_for_widget       (GtkWidget   *widget);

void          awn_tooltip_set_text             (AwnTooltip  *tooltip,
                                                const gchar *text);

gchar *       awn_tooltip_get_text             (AwnTooltip  *tooltip);

void          awn_tooltip_set_focus_widget     (AwnTooltip *tooltip,
                                                GtkWidget  *widget);

void          awn_tooltip_set_font_name        (AwnTooltip *tooltip,
                                                const gchar *font_name);

void          awn_tooltip_set_font_color       (AwnTooltip           *tooltip,
                                                DesktopAgnosticColor *font_color);

void          awn_tooltip_set_outline_color    (AwnTooltip           *tooltip,
                                                DesktopAgnosticColor *outline);

void          awn_tooltip_set_background_color (AwnTooltip           *tooltip,
                                                DesktopAgnosticColor *bg_color);

void          awn_tooltip_set_delay            (AwnTooltip  *tooltip,
                                                gint         msecs);

gint          awn_tooltip_get_delay            (AwnTooltip  *tooltip);

void          awn_tooltip_update_position      (AwnTooltip *tooltip);

void          awn_tooltip_set_position_hint    (AwnTooltip *tooltip,
                                                GtkPositionType position,
                                                gint size);

G_END_DECLS

#endif
