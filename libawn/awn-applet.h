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
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 *  Author : Neil Jagdish Patel <njpatel@gmail.com>
*/

#ifndef __AWN_APPLET_H__
#define __AWN_APPLET_H__

#include <gtk/gtk.h>

#include "awn-defines.h"

G_BEGIN_DECLS

#define AWN_TYPE_APPLET (awn_applet_get_type ())

#define AWN_APPLET(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), AWN_TYPE_APPLET,\
        AwnApplet))

#define AWN_APPLET_CLASS(obj)	(G_TYPE_CHECK_CLASS_CAST ((obj), \
        AWN_TYPE_APPLET, AwnAppletClass))
                                
#define AWN_IS_APPLET(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), AWN_TYPE_APPLET))

#define AWN_IS_APPLET_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE ((obj), \
        AWN_TYPE_APPLET))

#define AWN_APPLET_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), \
        AWN_TYPE_APPLET, AwnAppletClass))

typedef struct _AwnApplet AwnApplet;
typedef struct _AwnAppletClass AwnAppletClass;
typedef struct _AwnAppletPrivate AwnAppletPrivate;

struct _AwnApplet
{
  GtkPlug parent;

  /*< private >*/
  AwnAppletPrivate *priv;
};

struct _AwnAppletClass
{
  GtkPlugClass parent_class;

  /*<signals >*/
  void (*orient_changed)  (AwnApplet *applet, AwnOrientation orient);
  void (*offset_changed)  (AwnApplet *applet, gint offset);
  void (*size_changed)    (AwnApplet *applet, gint size);
  void (*deleted)         (AwnApplet *applet, const gchar *uid);
  void (*menu_creation)   (AwnApplet *applet, GtkMenu *menu);
  void (*flags_changed)   (AwnApplet *applet, AwnAppletFlags flags);
  void (*panel_configure) (AwnApplet *applet, GdkEventConfigure *event);
  void (*origin_changed)  (AwnApplet *applet, GdkRectangle *rect);

  /*< Future padding >*/
  void (*_applet0) (void);
  void (*_applet1) (void);
  void (*_applet2) (void);
  void (*_applet3) (void);
};


/* Hook to have an AWN Applet built for you */
typedef gboolean   (*AwnAppletInitFunc)           (AwnApplet   *applet);
/* Hook to build your own AWN Applet */
typedef AwnApplet* (*AwnAppletInitPFunc)          (const gchar *uid,
                                                   gint         panel_id);

GType              awn_applet_get_type            (void);

AwnApplet *        awn_applet_new                 (const gchar *uid, 
                                                   gint         panel_id);

AwnOrientation     awn_applet_get_orientation     (AwnApplet      *applet);
void               awn_applet_set_orientation     (AwnApplet      *applet,
                                                   AwnOrientation  orient);

AwnPathType        awn_applet_get_path_type       (AwnApplet *applet);
void               awn_applet_set_path_type       (AwnApplet *applet,
                                                   AwnPathType path);

gint               awn_applet_get_offset          (AwnApplet      *applet);
gint               awn_applet_get_offset_at       (AwnApplet      *applet,
                                                   gint           x,
                                                   gint           y);
void               awn_applet_set_offset          (AwnApplet      *applet,
                                                   gint           offset);

guint              awn_applet_get_size            (AwnApplet      *applet);
void               awn_applet_set_size            (AwnApplet      *applet,
                                                   gint           size);

const gchar *      awn_applet_get_uid             (AwnApplet      *applet);
void               awn_applet_set_uid             (AwnApplet      *applet,
                                                   const gchar    *uid);

AwnAppletFlags     awn_applet_get_flags           (AwnApplet      *applet);
void               awn_applet_set_flags           (AwnApplet      *applet, 
                                                   AwnAppletFlags  flags);

GtkWidget*         awn_applet_create_default_menu (AwnApplet      *applet);

void               awn_applet_set_panel_window_id (AwnApplet      *applet,
                                                   GdkNativeWindow anid);

guint              awn_applet_inhibit_autohide    (AwnApplet *applet,
                                                   const gchar *reason);
void               awn_applet_uninhibit_autohide  (AwnApplet *applet,
                                                   guint cookie);

/*
 * Returns a gtk menu item for the "Dock Preferences".
 * Note: awn_applet_create_default_menu() adds this item to its menu.  
 * This function should only be used in cases where 
 * awn_applet_create_default_menu() cannot be used 
 */
GtkWidget *        awn_applet_create_pref_item    (void);

G_END_DECLS

#endif
