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

  void (*position_changed) (AwnApplet *applet, GtkPositionType position);
  void (*offset_changed)   (AwnApplet *applet, gint offset);
  void (*size_changed)     (AwnApplet *applet, gint size);
  void (*deleted)          (AwnApplet *applet);
  void (*menu_creation)    (AwnApplet *applet, GtkMenu *menu);
  void (*flags_changed)    (AwnApplet *applet, AwnAppletFlags flags);
  void (*panel_configure)  (AwnApplet *applet, GdkEventConfigure *event);
  void (*origin_changed)   (AwnApplet *applet, GdkRectangle *rect);

  void (*_applet0) (void);
  void (*_applet1) (void);
  void (*_applet2) (void);
  void (*_applet3) (void);
};


/**
 * AwnAppletInitFunc:
 * @applet: AwnApplet instance created for you.
 *
 * Function prototype used as entry point for applets.
 * Hook to have an #AwnApplet built for you.
 *
 * Returns: return TRUE if applet construction was successful, FALSE otherwise.
 */
typedef gboolean   (*AwnAppletInitFunc)           (AwnApplet   *applet);

/**
 * AwnAppletInitPFunc:
 * @canonical_name: applet's canonical name.
 * @uid: applet's unique ID.
 * @panel_id: ID of AwnPanel associated with this applet.
 *
 * Function prototype used as entry point for applets.
 * Hook to build your own #AwnApplet.
 *
 * Returns: return the constructed #AwnApplet (or its subclass) or NULL.
 */
typedef AwnApplet* (*AwnAppletInitPFunc)          (const gchar *canonical_name,
                                                   const gchar *uid,
                                                   gint         panel_id);

GType              awn_applet_get_type            (void);

AwnApplet *        awn_applet_new                 (const gchar *canonical_name,
                                                   const gchar *uid,
                                                   gint         panel_id);

const gchar*       awn_applet_get_canonical_name  (AwnApplet      *applet);

GtkPositionType    awn_applet_get_pos_type        (AwnApplet      *applet);
void               awn_applet_set_pos_type        (AwnApplet      *applet,
                                                   GtkPositionType position);

AwnPathType        awn_applet_get_path_type       (AwnApplet *applet);
void               awn_applet_set_path_type       (AwnApplet *applet,
                                                   AwnPathType path);

gint               awn_applet_get_offset          (AwnApplet      *applet);
gint               awn_applet_get_offset_at       (AwnApplet      *applet,
                                                   gint           x,
                                                   gint           y);
void               awn_applet_set_offset          (AwnApplet      *applet,
                                                   gint           offset);

gint               awn_applet_get_size            (AwnApplet      *applet);
void               awn_applet_set_size            (AwnApplet      *applet,
                                                   gint           size);

const gchar *      awn_applet_get_uid             (AwnApplet      *applet);
void               awn_applet_set_uid             (AwnApplet      *applet,
                                                   const gchar    *uid);

AwnAppletFlags     awn_applet_get_behavior        (AwnApplet      *applet);
void               awn_applet_set_behavior        (AwnApplet      *applet,
                                                   AwnAppletFlags  flags);

GtkWidget*         awn_applet_create_default_menu (AwnApplet      *applet);

guint              awn_applet_inhibit_autohide    (AwnApplet *applet,
                                                   const gchar *reason);
void               awn_applet_uninhibit_autohide  (AwnApplet *applet,
                                                   guint cookie);

GdkNativeWindow    awn_applet_docklet_request     (AwnApplet *applet,
                                                   gint min_size,
                                                   gboolean shrink,
                                                   gboolean expand);
/*
 * Returns a gtk menu item for the "Dock Preferences".
 * Note: awn_applet_create_default_menu() adds this item to its menu.  
 * This function should only be used in cases where 
 * awn_applet_create_default_menu() cannot be used 
 */
GtkWidget *        awn_applet_create_pref_item    (void);
GtkWidget *        awn_applet_create_about_item   (AwnApplet *applet,
                                                   const gchar       *copyright,
                                                   AwnAppletLicense   license,
                                                   const gchar       *version,
                                                   const gchar       *comments,
                                                   const gchar       *website,
                                                   const gchar       *website_label,
                                                   const gchar       *icon_name,
                                                   const gchar       *translator_credits,
                                                   const gchar      **authors,
                                                   const gchar      **artists,
                                                   const gchar      **documenters);

GtkWidget *        awn_applet_create_about_item_simple (AwnApplet        *applet,
                                                        const gchar      *copyright,
                                                        AwnAppletLicense  license,
                                                        const gchar      *version);

G_END_DECLS

#endif
