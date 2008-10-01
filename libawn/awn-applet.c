/* -*- Mode: C; tab-width: 2; indent-tabs-mode: t; c-basic-offset: 2 -*- */
/*
 * Copyright (C) 2007 Neil J. Patel <njpatel@gmail.com>
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
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <glib/gstdio.h>
#include <glib/gi18n.h>
#include "awn-applet.h"

G_DEFINE_TYPE (AwnApplet, awn_applet, GTK_TYPE_EVENT_BOX);

#define AWN_APPLET_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj),\
                                     AWN_TYPE_APPLET, AwnAppletPrivate))

struct _AwnAppletPrivate
{
  gchar *uid;
  gchar *gconf_key;
  AwnOrientation orient;
  guint size;

  AwnAppletFlags flags;
};

enum
{
  PROP_0,
  PROP_UID,
  PROP_ORIENT,
  PROP_SIZE
};

enum
{
  ORIENT_CHANGED,
  SIZE_CHANGED,
  PLUG_EMBEDDED,
  DELETED,
  MENU_CREATION,
  FLAGS_CHANGED,

  LAST_SIGNAL
};
static guint _applet_signals[LAST_SIGNAL] = { 0 };

/*  GOBJECT STUFF */
static gboolean
awn_applet_expose_event (GtkWidget *widget, GdkEventExpose *expose)
{
  cairo_t   *cr = NULL;
  GtkWidget *child = NULL;

  if (!GDK_IS_DRAWABLE (widget->window))
    return FALSE;

  cr = gdk_cairo_create (widget->window);

  if (!cr)
    return FALSE;

  gdk_cairo_region (cr, expose->region);
  cairo_clip (cr);

  /* Clear the background to transparent */
  cairo_set_source_rgba (cr, 1.0f, 1.0f, 1.0f, 0.0f);
  cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
  cairo_paint (cr);

  /* Clean up */
  cairo_destroy (cr);

  /* Propagate the signal */
  child = gtk_bin_get_child (GTK_BIN (widget));

  if (child)
    gtk_container_propagate_expose (GTK_CONTAINER (widget), child,  expose);

  return FALSE;
}

static void
awn_applet_set_property (GObject      *object,
                         guint         prop_id,
                         const GValue *value,
                         GParamSpec   *pspec)
{
  AwnApplet *applet = AWN_APPLET (object);
  
  switch (prop_id)
  {
    case PROP_UID:
      awn_applet_set_uid (applet, g_value_get_string (value));
      break;
    case PROP_ORIENT:
      awn_applet_set_orientation (applet, g_value_get_int (value));
      break;
    case PROP_SIZE:
      awn_applet_set_size (applet, g_value_get_int (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
awn_applet_get_property (GObject    *object,
                         guint       prop_id,
                         GValue     *value,
                         GParamSpec *pspec)
{
  AwnAppletPrivate *priv;

  g_return_if_fail(AWN_IS_APPLET(object));
  priv = AWN_APPLET_GET_PRIVATE(object);

  switch (prop_id)
  {
    case PROP_UID:
      g_value_set_string (value, priv->uid);
      break;

    case PROP_ORIENT:
      g_value_set_int (value, priv->orient);

    case PROP_SIZE:
      g_value_set_int (value, priv->size);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
awn_applet_dispose (GObject *obj)
{
  G_OBJECT_CLASS (awn_applet_parent_class)->dispose(obj);
}

static void
awn_applet_class_init (AwnAppletClass *klass)
{
  GObjectClass *gobject_class;
  GtkWidgetClass *widget_class;

  gobject_class = G_OBJECT_CLASS(klass);
  gobject_class->dispose = awn_applet_dispose;
  gobject_class->get_property = awn_applet_get_property;
  gobject_class->set_property = awn_applet_set_property;

  widget_class = GTK_WIDGET_CLASS(klass);
  widget_class->expose_event = awn_applet_expose_event;

   /* Class properties */
  g_object_class_install_property (gobject_class,
    PROP_UID,
    g_param_spec_string ("uid",
                         "wid",
                         "Awn's Unique ID for this applet instance",
                         NULL,
                         G_PARAM_CONSTRUCT | G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class,
   PROP_ORIENT,
   g_param_spec_int ("orient",
                     "Orientation",
                     "The current bar orientation",
                     0, 3, AWN_ORIENTATION_BOTTOM,
                     G_PARAM_CONSTRUCT | G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class,
   PROP_SIZE,
   g_param_spec_int ("size",
                     "Height",
                     "The current visible size of the bar",
                     0, G_MAXINT, 48,
                     G_PARAM_CONSTRUCT | G_PARAM_READWRITE));

  /* Class signals */
  _applet_signals[ORIENT_CHANGED] =
    g_signal_new("orientation-changed",
                 G_OBJECT_CLASS_TYPE(gobject_class),
                 G_SIGNAL_RUN_FIRST,
                 G_STRUCT_OFFSET(AwnAppletClass, orient_changed),
                 NULL, NULL,
                 g_cclosure_marshal_VOID__ENUM,
                 G_TYPE_NONE, 1, G_TYPE_INT);

  _applet_signals[SIZE_CHANGED] =
    g_signal_new("size-changed",
                 G_OBJECT_CLASS_TYPE(gobject_class),
                 G_SIGNAL_RUN_FIRST,
                 G_STRUCT_OFFSET(AwnAppletClass, size_changed),
                 NULL, NULL,
                 g_cclosure_marshal_VOID__INT,
                 G_TYPE_NONE, 1, G_TYPE_INT);

  _applet_signals[PLUG_EMBEDDED] =
    g_signal_new("plug-embedded",
                 G_OBJECT_CLASS_TYPE(gobject_class),
                 G_SIGNAL_RUN_FIRST,
                 G_STRUCT_OFFSET(AwnAppletClass, plug_embedded),
                 NULL, NULL,
                 g_cclosure_marshal_VOID__VOID,
                 G_TYPE_NONE, 0);

  _applet_signals[DELETED] =
    g_signal_new("applet-deleted",
                 G_OBJECT_CLASS_TYPE(gobject_class),
                 G_SIGNAL_RUN_FIRST,
                 G_STRUCT_OFFSET(AwnAppletClass, deleted),
								 NULL, NULL,
                 g_cclosure_marshal_VOID__STRING,
                 G_TYPE_NONE, 1, G_TYPE_STRING);

  _applet_signals[MENU_CREATION] =
    g_signal_new("menu-creation",
                 G_OBJECT_CLASS_TYPE (gobject_class),
                 G_SIGNAL_RUN_FIRST,
                 G_STRUCT_OFFSET (AwnAppletClass, menu_creation),
								 NULL, NULL,
                 g_cclosure_marshal_VOID__OBJECT,
                 G_TYPE_NONE, 1, GTK_TYPE_MENU);

  _applet_signals[FLAGS_CHANGED] =
    g_signal_new("flags-changed",
                 G_OBJECT_CLASS_TYPE (gobject_class),
                 G_SIGNAL_RUN_FIRST,
                 G_STRUCT_OFFSET (AwnAppletClass, flags_changed),
								 NULL, NULL,
                 g_cclosure_marshal_VOID__INT,
                 G_TYPE_NONE, 1, G_TYPE_INT);



  g_type_class_add_private(gobject_class, sizeof(AwnAppletPrivate));
}

static void
awn_applet_init (AwnApplet *applet)
{
  AwnAppletPrivate *priv;

  priv = applet->priv = AWN_APPLET_GET_PRIVATE(applet);

  priv->flags = AWN_APPLET_FLAGS_NONE;
}

AwnApplet *
awn_applet_new (const gchar* uid, gint orient, gint size)
{
  AwnApplet *applet = g_object_new (AWN_TYPE_APPLET,
                                    "above-child", FALSE,
                                    "visible-window", TRUE,
                                    "uid", uid,
                                    "orient", orient,
                                    "size", size,
                                    NULL);
  return applet;
}

/*
 * Public funcs
 */
void  
awn_applet_plug_embedded (AwnApplet *applet)
{
  g_return_if_fail (AWN_IS_APPLET (applet));

  g_signal_emit (applet, _applet_signals[PLUG_EMBEDDED], 0);
}

/*
 * Callback to start awn-manager.  See awn_applet_create_default_menu()
 */
static gboolean
_start_awn_manager (GtkMenuItem *menuitem, gpointer null)
{
  GError *err = NULL;
  
  g_spawn_command_line_async("awn-manager", &err);

  if (err)
  {
    g_warning("Failed to start awn-manager: %s\n", err->message);
    g_error_free(err);
  }

  /* FIXME: Call a method on AwnPanel to popup it's prefs dialog */

  return TRUE;
}

/*create a Dock Preferences menu item */
GtkWidget *
awn_applet_create_pref_item (void)
{
	GtkWidget *item;
  
  item = gtk_image_menu_item_new_with_label (_("Dock Preferences"));
  gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (item),
                                 gtk_image_new_from_stock(GTK_STOCK_PREFERENCES,
                                                          GTK_ICON_SIZE_MENU));
  gtk_widget_show_all (item);
  g_signal_connect (item, "activate", 
                    G_CALLBACK (_start_awn_manager), NULL);
	return item;
}

GtkWidget*
awn_applet_create_default_menu (AwnApplet *applet)
{
  AwnAppletPrivate *priv;
  GtkWidget *menu = NULL;
  GtkWidget *item = NULL;
  
  g_return_val_if_fail (AWN_IS_APPLET (applet), menu);
  priv = AWN_APPLET_GET_PRIVATE (applet);

  menu = gtk_menu_new ();

  /* The preferences (awn-manager) menu item  */
  item = awn_applet_create_pref_item ();
  gtk_menu_shell_append (GTK_MENU_SHELL(menu), item);

  /* And Neil said: "Let there be customisation of thy menu" */
  g_signal_emit (applet, _applet_signals[MENU_CREATION], 0, menu);

  /* The second separator  */
  item = gtk_separator_menu_item_new ();
  gtk_widget_show_all (item);
  gtk_menu_shell_append (GTK_MENU_SHELL(menu), item);

  return menu;
}

AwnOrientation
awn_applet_get_orientation (AwnApplet *applet)
{
  AwnAppletPrivate *priv;

  g_return_val_if_fail(AWN_IS_APPLET (applet), AWN_ORIENTATION_BOTTOM);
  priv = AWN_APPLET_GET_PRIVATE (applet);

  return priv->orient;
}

void
awn_applet_set_orientation (AwnApplet *applet, AwnOrientation orient)
{
  AwnAppletPrivate *priv;

  g_return_if_fail (AWN_IS_APPLET (applet));
  priv = applet->priv;

  priv->orient = orient;

  g_signal_emit (applet, _applet_signals[ORIENT_CHANGED], 0, orient);
}

guint
awn_applet_get_size (AwnApplet *applet)
{
  AwnAppletPrivate *priv;

  g_return_val_if_fail(AWN_IS_APPLET (applet), 48);
  priv = AWN_APPLET_GET_PRIVATE (applet);

  return priv->size;
}

void
awn_applet_set_size (AwnApplet *applet, gint size)
{
  AwnAppletPrivate *priv;

  g_return_if_fail (AWN_IS_APPLET (applet));
  priv = applet->priv;

  priv->size = size;

  g_signal_emit (applet, _applet_signals[SIZE_CHANGED], 0, size);
}

const gchar *
awn_applet_get_uid (AwnApplet *applet)
{
  AwnAppletPrivate *priv;

  g_return_val_if_fail (AWN_IS_APPLET (applet), NULL);
  priv = AWN_APPLET_GET_PRIVATE (applet);

  return priv->uid;
}

void
awn_applet_set_uid (AwnApplet *applet, const gchar *uid)
{
  AwnAppletPrivate *priv;

  g_return_if_fail (AWN_IS_APPLET (applet));
  g_return_if_fail (uid);
  priv = applet->priv;

  priv->uid = g_strdup (uid);
}

void 
awn_applet_set_flags (AwnApplet *applet, AwnAppletFlags flags)
{
  AwnAppletPrivate *priv;

  g_return_if_fail (AWN_IS_APPLET (applet));
  priv = applet->priv;

  priv->flags = flags;

  g_signal_emit (applet, _applet_signals[FLAGS_CHANGED], 0, flags);
}

AwnAppletFlags 
awn_applet_get_flags (AwnApplet *applet)
{
  g_return_val_if_fail (AWN_IS_APPLET (applet), AWN_APPLET_FLAGS_NONE);

  return applet->priv->flags;
}

