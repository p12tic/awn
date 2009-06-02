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

#ifndef _AWN_OVERLAID_ICON_H_
#define _AWN_OVERLAID_ICON_H_

#include <glib-object.h>
#include <gtk/gtk.h>

#include "awn-themed-icon.h"

G_BEGIN_DECLS

#define AWN_TYPE_OVERLAID_ICON (awn_overlaid_icon_get_type ())

#define AWN_OVERLAID_ICON(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj),\
	AWN_TYPE_OVERLAID_ICON, AwnOverlaidIcon))

#define AWN_OVERLAID_ICON_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass),\
	AWN_TYPE_OVERLAID_ICON, AwnOverlaidIconClass))

#define AWN_IS_OVERLAID_ICON(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj),\
	AWN_TYPE_OVERLAID_ICON))

#define AWN_IS_OVERLAID_ICON_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),\
	AWN_TYPE_OVERLAID_ICON))

#define AWN_OVERLAID_ICON_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj),\
	AWN_TYPE_OVERLAID_ICON, AwnOverlaidIconClass))

typedef struct _AwnOverlaidIcon        AwnOverlaidIcon;
typedef struct _AwnOverlaidIconClass   AwnOverlaidIconClass;
typedef struct _AwnOverlaidIconPrivate AwnOverlaidIconPrivate;
 
struct _AwnOverlaidIcon
{
  AwnThemedIcon  parent;	

  AwnOverlaidIconPrivate *priv;
};

struct _AwnOverlaidIconClass
{
  AwnThemedIconClass parent_class;
};

typedef struct
{
  AwnOverlayType  overlay_type;  
  
  union
  {
    gchar           * text;
    gchar           * icon_name;
    cairo_surface_t * srfc;
  }data;    
  
  union
  {
    int dummy;
  }cached_info;

  AwnGravity  gravity;
  AwnOverlayAlign align;

  double      x_adj;
  double      y_adj;
  double      x_per;      /*size in % of x axis*/
  double      y_per;      /*size in % of y axis*/
  
}AwnOverlay;


GType         awn_overlaid_icon_get_type           (void) G_GNUC_CONST;

GtkWidget *   awn_overlaid_icon_new                (void);

AwnOverlay *  awn_overlaid_icon_append_overlay     (AwnOverlaidIcon * icon,
                                                    AwnOverlayType  type,
                                                    AwnGravity      grav,
                                                    gpointer        data);
AwnOverlay *  awn_overlaid_icon_change_overlay_data (AwnOverlaidIcon * icon,
                                                    AwnOverlay * overlay,
                                                    gpointer * new_data);


G_END_DECLS

#endif /* _AWN_OVERLAID_ICON_H_ */
