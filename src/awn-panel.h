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

#ifndef	_AWN_PANEL_H
#define	_AWN_PANEL_H

#include <glib.h>
#include <gtk/gtk.h>

#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-bindings.h>

#include <libdesktop-agnostic/config.h>
#include <libawn/libawn.h>

G_BEGIN_DECLS

#define AWN_TYPE_PANEL (awn_panel_get_type())

#define AWN_PANEL(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), AWN_TYPE_PANEL, \
        AwnPanel))

#define AWN_PANEL_CLASS(obj)	(G_TYPE_CHECK_CLASS_CAST ((obj), AWN_PANEL, \
        AwnPanelClass))

#define AWN_IS_PANEL(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), AWN_TYPE_PANEL))

#define AWN_IS_PANEL_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE ((obj), \
        AWN_TYPE_PANEL))

#define AWN_PANEL_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), \
        AWN_TYPE_PANEL, AwnPanelClass))

typedef struct _AwnPanel AwnPanel;
typedef struct _AwnPanelClass AwnPanelClass;
typedef struct _AwnPanelPrivate AwnPanelPrivate;

struct _AwnPanel 
{
  GtkWindow parent;

  /*< private >*/
  AwnPanelPrivate *priv;
};

struct _AwnPanelClass 
{
  GtkWindowClass parent_class;

  /*< signals >*/
  void (*size_changed)     (AwnPanel *panel, gint size);
  void (*position_changed) (AwnPanel *panel, GtkPositionType position);
  void (*offset_changed)   (AwnPanel *panel, gint offset);
  void (*property_changed) (AwnPanel *panel,
                            const gchar *prop_name, GValue *value);
  void (*destroy_notify)   (AwnPanel *panel);
  void (*destroy_applet)   (AwnPanel *panel, const gchar *uid);

  gboolean (*autohide_start) (AwnPanel *panel);
  void     (*autohide_end)   (AwnPanel *panel);
};

GType       awn_panel_get_type            (void) G_GNUC_CONST;

GtkWidget * awn_panel_new_with_panel_id   (gint panel_id);

gboolean    awn_panel_add_applet          (AwnPanel        *panel,
                                           gchar           *desktop_file,
                                           GError         **error);

gboolean    awn_panel_delete_applet       (AwnPanel        *panel,
                                           gchar           *uid,
                                           GError         **error);

gboolean    awn_panel_set_applet_flags    (AwnPanel         *panel,
                                           const gchar      *uid,
                                           gint              flags,
                                           GError          **error);

void        awn_panel_set_glow            (AwnPanel         *panel,
                                           const gchar      *sender,
                                           gboolean          activate);

gint        awn_panel_get_glow_size       (AwnPanel *panel);

guint       awn_panel_inhibit_autohide    (AwnPanel *panel,
                                           const gchar *sender,
                                           const gchar *app_name,
                                           const gchar *reason);

gboolean    awn_panel_uninhibit_autohide  (AwnPanel         *panel,
                                           guint             cookie);

GStrv       awn_panel_get_inhibitors      (AwnPanel         *panel);

gint64      awn_panel_docklet_request     (AwnPanel         *panel,
                                           gint              min_size,
                                           gboolean          shrink,
                                           gboolean          expand,
                                           GError          **error);

gboolean    awn_panel_get_docklet_mode    (AwnPanel         *panel);

gboolean    awn_panel_get_composited      (AwnPanel         *panel);

gboolean    awn_panel_get_snapshot        (AwnPanel *panel,
                                           gint     *width,
                                           gint     *height,
                                           gint     *rowstride,
                                           gboolean *has_alpha,
                                           gint     *bits_per_sample,
                                           gint     *num_channels,
                                           gchar   **pixels,
                                           gint     *pixels_length,
                                           GError **error);

gboolean    awn_panel_get_all_server_flags(AwnPanel *panel,
                                           GHashTable **hash,
                                           gchar     *name,
                                           GError   **error);

gboolean    awn_panel_ua_add_applet       (AwnPanel *panel,
                                           gchar *name, glong xid,
                                           gint width, gint height,
                                           gchar *size_type,
                                           GError **error);

G_END_DECLS


#endif /* _AWN_PANEL_H */

