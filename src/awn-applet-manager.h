/*
 *  Copyright (C) 2008 Neil Jagdish Patel <njpatel@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301  USA.
 *
 *  Author : Neil Jagdish Patel <njpatel@gmail.com>
*/

#ifndef	_AWN_APPLET_MANAGER_H
#define	_AWN_APPLET_MANAGER_H

#include <glib.h>
#include <gtk/gtk.h>

#include <libdesktop-agnostic/desktop-agnostic.h>
#include <libawn/libawn.h>

G_BEGIN_DECLS

#define AWN_TYPE_APPLET_MANAGER (awn_applet_manager_get_type())

#define AWN_APPLET_MANAGER(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
        AWN_TYPE_APPLET_MANAGER, AwnAppletManager))

#define AWN_APPLET_MANAGER_CLASS(obj) (G_TYPE_CHECK_CLASS_CAST ((obj), \
        AWN_APPLET_MANAGER, AwnAppletManagerClass))

#define AWN_IS_APPLET_MANAGER(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
        AWN_TYPE_APPLET_MANAGER))

#define AWN_IS_APPLET_MANAGER_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE ((obj), \
        AWN_TYPE_APPLET_MANAGER))

#define AWN_APPLET_MANAGER_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), \
        AWN_TYPE_APPLET_MANAGER, AwnAppletManagerClass))

typedef struct _AwnAppletManager AwnAppletManager;
typedef struct _AwnAppletManagerClass AwnAppletManagerClass;
typedef struct _AwnAppletManagerPrivate AwnAppletManagerPrivate;

struct _AwnAppletManager 
{
  AwnBox parent;

  /*< private >*/
  AwnAppletManagerPrivate *priv;
};

struct _AwnAppletManagerClass 
{
  AwnBoxClass parent_class;

  /*< signals >*/
  void (*applet_embedded)    (AwnAppletManager *manager, GtkWidget *applet);
  void (*applet_removed)     (AwnAppletManager *manager, GtkWidget *applet);
  void (*shape_mask_changed) (AwnAppletManager *manager);
  void (*applets_refreshed)  (AwnAppletManager *manager);
};

GType       awn_applet_manager_get_type          (void) G_GNUC_CONST;

GtkWidget * awn_applet_manager_new_from_config   (DesktopAgnosticConfigClient *client);

void        awn_applet_manager_refresh_applets   (AwnAppletManager *manager);

gchar*      awn_applet_manager_generate_uid      (AwnAppletManager *manager);

void        awn_applet_manager_add_widget        (AwnAppletManager *manager,
                                                  GtkWidget *widget, gint pos);

void        awn_applet_manager_remove_widget     (AwnAppletManager *manager,
                                                  GtkWidget *widget);

gboolean    awn_applet_manager_get_expands       (AwnAppletManager *manager);

void        awn_applet_manager_set_applet_flags  (AwnAppletManager *manager,
                                                  const gchar *uid,
                                                  AwnAppletFlags flags);

void        awn_applet_manager_show_applets      (AwnAppletManager *manager);
void        awn_applet_manager_hide_applets      (AwnAppletManager *manager);

void        awn_applet_manager_add_docklet       (AwnAppletManager *manager,
                                                  GtkWidget *docklet);

void        awn_applet_manager_redraw_throbbers  (AwnAppletManager *manager);

GdkRegion*  awn_applet_manager_get_mask          (AwnAppletManager *manager,
                                                  AwnPathType path_type,
                                                  gfloat offset_modifier);

/* UA stuff */

gboolean    awn_ua_get_all_server_flags          (AwnAppletManager *manager,
                                                  GHashTable **hash,
                                                  gchar     *name,
                                                  GError   **error);

gboolean    awn_ua_add_applet                    (AwnAppletManager *manager,
                                                  gchar *name, glong xid,
                                                  gint width, gint height,
                                                  gchar *size_type,
                                                  GError **error);

G_END_DECLS

#endif /* _AWN_APPLET_MANAGER_H */

