/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2007 Neil J. Patel <njpatel@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * Authors: Neil J. Patel <njpatel@gmail.com>
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "awn-applet.h"

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <gconf/gconf.h>
#include <gconf/gconf-client.h>

G_DEFINE_TYPE (AwnApplet, awn_applet, GTK_TYPE_EVENT_BOX);

#define AWN_APPLET_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj),\
	AWN_TYPE_APPLET, \
	AwnAppletPrivate))

static GtkEventBoxClass *parent_class;

/* STRUCTS & ENUMS */
struct _AwnAppletPrivate
{
	gchar *uid;
	gchar *gconf_key;
	AwnOrientation orient;
	guint height;
	
};

enum
{
	PROP_0,
	PROP_UID,
	PROP_ORIENT,
	PROP_HEIGHT
};

enum
{
	ORIENT_CHANGED,
	HEIGHT_CHANGED,
	DELETED,
	LAST_SIGNAL
};

static guint _applet_signals[LAST_SIGNAL] = { 0 };

/* FORWARDS */

static void awn_applet_class_init(AwnAppletClass *klass);
static void awn_applet_init(AwnApplet *applet);
static void awn_applet_finalize(GObject *obj);


/* VIRTUAL GOBJECT METHODS */

static void awn_applet_virtual_on_plug_embedded( AwnApplet *applet ) {
//	printf( "[AWNAPPLET] default on_plug_embedded\n" );
}

static void awn_applet_virtual_on_size_changed( AwnApplet *applet, gint x ) {
//	printf( "[AWNAPPLET] default on_size_changed\n" );
}


/* MAIN FUNCTIONS */

static void
on_delete_selected (GtkMenuItem *item, AwnApplet *applet)
{
	;
}

GtkWidget*
awn_applet_create_default_menu (AwnApplet *applet)
{
	AwnAppletPrivate *priv;
	GtkWidget *menu = NULL;
	/*GtkWidget *item = NULL;*/
	
	g_return_val_if_fail (AWN_IS_APPLET (applet), menu);
	priv = AWN_APPLET_GET_PRIVATE(applet);
	
	menu = gtk_menu_new ();
	
	/* The first separator 
	item = gtk_separator_menu_item_new ();
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
	*/
	
	/* The configure applets separator 
	item = gtk_image_menu_item_new_from_stock (GTK_STOCK_REMOVE, NULL);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
	g_signal_connect (G_OBJECT (item), "activate", 
			  G_CALLBACK (on_delete_selected), applet);
	
	*/
	return menu;
}

AwnOrientation
awn_applet_get_orientation (AwnApplet *applet)
{
	AwnAppletPrivate *priv;

	g_return_val_if_fail (AWN_IS_APPLET (applet), 0);
	priv = AWN_APPLET_GET_PRIVATE(applet);
	
	return priv->orient;
}

guint
awn_applet_get_height (AwnApplet *applet)
{
	AwnAppletPrivate *priv;

	g_return_val_if_fail (AWN_IS_APPLET (applet), 48);
	priv = AWN_APPLET_GET_PRIVATE(applet);
	
	return priv->height;
}

gchar*
awn_applet_get_preferences_key (AwnApplet *applet)
{
	AwnAppletPrivate *priv;

	g_return_val_if_fail (AWN_IS_APPLET (applet), NULL);
	priv = AWN_APPLET_GET_PRIVATE(applet);
	
	return g_strdup (priv->gconf_key);
}

static void
awn_applet_associate_schemas_in_dir (GConfClient  *client,
				       const gchar  *prefs_key,
				       const gchar  *schema_dir,
				       GError      **error)
{
	GSList *list, *l;

	list = gconf_client_all_entries (client, schema_dir, error);

	g_return_if_fail (*error == NULL);

	for (l = list; l; l = l->next) {
		GConfEntry *entry = l->data;
		gchar      *key;
		gchar      *tmp;

		tmp = g_path_get_basename (gconf_entry_get_key (entry));

		if (strchr (tmp, '-'))
			g_warning ("Applet key '%s' contains a hyphen. Please "
				   "use underscores in gconf keys\n", tmp);

		key = g_strdup_printf ("%s/%s", prefs_key, tmp);

		g_free (tmp);

		gconf_engine_associate_schema (
			client->engine, key, gconf_entry_get_key (entry), error);

		g_free (key);

		gconf_entry_free (entry);

		if (*error) {
			g_slist_free (list);
			return;
		}
	}

	g_slist_free (list);

	list = gconf_client_all_dirs (client, schema_dir, error);

	for (l = list; l; l = l->next) {
		gchar *subdir = l->data;
		gchar *prefs_subdir;
		gchar *schema_subdir;
		gchar *tmp;

		tmp = g_path_get_basename (subdir);

		prefs_subdir  = g_strdup_printf ("%s/%s", prefs_key, tmp);
		schema_subdir = g_strdup_printf ("%s/%s", schema_dir, tmp);

		awn_applet_associate_schemas_in_dir (
			client, prefs_subdir, schema_subdir, error);

		g_free (prefs_subdir);
		g_free (schema_subdir);
		g_free (subdir);
		g_free (tmp);

		if (*error) {
			g_slist_free (list);
			return;
		}
	}

	g_slist_free (list);
}

void
awn_applet_add_preferences (AwnApplet  *applet,
			      const gchar  *schema_dir,
			      GError      **opt_error)
{
	AwnAppletPrivate *priv;
	GConfClient *client;
	GError **error = NULL;
	GError  *our_error = NULL;

	g_return_if_fail (AWN_IS_APPLET (applet));
	g_return_if_fail (schema_dir != NULL);
	
	priv = AWN_APPLET_GET_PRIVATE(applet);
	
	if (!priv->gconf_key)
		return;

	if (opt_error)
		error = opt_error;
	else
		error = &our_error;

	client = gconf_client_get_default ();
	awn_applet_associate_schemas_in_dir (client, priv->gconf_key,
					       schema_dir, error);

	if (!opt_error && our_error) {
		g_warning (G_STRLOC ": failed to add preferences from '%s' : '%s'",
			   schema_dir, our_error->message);
		g_error_free (our_error);
	}
}

static gboolean 
awn_applet_expose_event (GtkWidget *widget, GdkEventExpose *expose)
{
	cairo_t *cr = NULL;
  GtkWidget *child = NULL;

	if (!GDK_IS_DRAWABLE (widget->window))
		return FALSE;	
	
	cr = gdk_cairo_create (widget->window);
	if (!cr)
		return FALSE;
	
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

/*  GOBJECT STUFF */

static void
awn_applet_set_property (GObject      *object, 
			 guint         prop_id,
			 const GValue *value, 
			 GParamSpec   *pspec)
{
	AwnAppletPrivate *priv;

	g_return_if_fail (AWN_IS_APPLET (object));
	priv = AWN_APPLET_GET_PRIVATE(object);

	switch (prop_id) {
		case PROP_UID:
			if (priv->uid != NULL)
				g_free (priv->uid);
			priv->uid = g_strdup (g_value_get_string (value));
			priv->gconf_key = g_strdup_printf ("%s/%s", 
						          AWN_APPLET_GCONF_PATH,
						          priv->uid);
			break;
		case PROP_ORIENT:
			priv->orient = g_value_get_int (value);
			g_signal_emit (object, _applet_signals[ORIENT_CHANGED],
				       0, priv->orient);
			break;	
		case PROP_HEIGHT:
			priv->height = g_value_get_int (value);
			g_signal_emit (object, _applet_signals[HEIGHT_CHANGED],
				       0, priv->height);
			break;
		
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, 
							   pspec);
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

	g_return_if_fail (AWN_IS_APPLET (object));
	priv = AWN_APPLET_GET_PRIVATE(object);

	switch (prop_id) {
		case PROP_UID:
			g_value_set_string (value, priv->uid);
			break;
		
		case PROP_ORIENT:
			g_value_set_int (value, priv->orient);
			
		case PROP_HEIGHT:
			g_value_set_int (value, priv->height);
			break;
			
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id,
							   pspec);
		break;
	} 
}

static void
awn_applet_class_init(AwnAppletClass *klass)
{
	GObjectClass *gobject_class;
	GtkWidgetClass *widget_class;

	klass->size_changed = awn_applet_virtual_on_size_changed;
	klass->plug_embedded = awn_applet_virtual_on_plug_embedded;

	parent_class = g_type_class_peek_parent(klass);

	gobject_class = G_OBJECT_CLASS(klass);
	gobject_class->finalize = awn_applet_finalize;
	gobject_class->get_property = awn_applet_get_property;
	gobject_class->set_property = awn_applet_set_property;
		
	widget_class = GTK_WIDGET_CLASS (klass);
	widget_class->expose_event = awn_applet_expose_event;
	
	g_type_class_add_private (gobject_class, sizeof (AwnAppletPrivate));

	/* Class properties */
	g_object_class_install_property 
		(gobject_class,
		 PROP_UID,
		 g_param_spec_string ("uid",
		 "UID",
		 "Awn's Unique ID for this applet instance (used for gconf)",
		 NULL,
		 G_PARAM_CONSTRUCT|G_PARAM_READWRITE));	
		 
	g_object_class_install_property 
		(gobject_class,
		 PROP_ORIENT,
		 g_param_spec_int ("orient",
		 "Orientation",
		 "The current bar orientation",
		 0,10,
		 AWN_ORIENTATION_BOTTOM,
		 G_PARAM_CONSTRUCT|G_PARAM_READWRITE));	
		 	
	g_object_class_install_property 
		(gobject_class,
		 PROP_HEIGHT,
		 g_param_spec_int ("height",
		 "Height",
		 "The current visible height of the bar",
		 AWN_MIN_HEIGHT, AWN_MAX_HEIGHT, 48,
		 G_PARAM_CONSTRUCT|G_PARAM_READWRITE));

	/* Class signals */
	_applet_signals[ORIENT_CHANGED] =
		g_signal_new ("orientation-changed",
			     G_OBJECT_CLASS_TYPE (gobject_class),
			     G_SIGNAL_RUN_FIRST,
			     G_STRUCT_OFFSET (AwnAppletClass, orient_changed),
			     NULL, NULL,
			     g_cclosure_marshal_VOID__ENUM,
			     G_TYPE_NONE, 1, G_TYPE_INT);

	_applet_signals[HEIGHT_CHANGED] =
		g_signal_new ("height-changed",
			     G_OBJECT_CLASS_TYPE (gobject_class),
			     G_SIGNAL_RUN_FIRST,
			     G_STRUCT_OFFSET (AwnAppletClass, height_changed),
			     NULL, NULL,
			     g_cclosure_marshal_VOID__INT,
			     G_TYPE_NONE, 1, G_TYPE_INT);
			     
	_applet_signals[DELETED] =
		g_signal_new ("applet-deleted",
			     G_OBJECT_CLASS_TYPE (gobject_class),
			     G_SIGNAL_RUN_FIRST,
			     G_STRUCT_OFFSET (AwnAppletClass, deleted),
			     NULL, NULL,
			     g_cclosure_marshal_VOID__STRING,
			     G_TYPE_NONE, 1, G_TYPE_STRING);

}

static void
awn_applet_init(AwnApplet *applet)
{
	AwnAppletPrivate *priv;
		
	priv = AWN_APPLET_GET_PRIVATE (applet);
	
}



static void
awn_applet_finalize(GObject *obj)
{
	AwnApplet *applet;
	
	g_return_if_fail(obj != NULL);
	g_return_if_fail(AWN_IS_APPLET(obj));

	applet = AWN_APPLET(obj);
	
	if (G_OBJECT_CLASS(parent_class)->finalize)
		G_OBJECT_CLASS(parent_class)->finalize(obj);
}

GtkWidget *
awn_applet_new( const gchar* uid, gint orient, gint height )
{
	GtkWidget *applet = g_object_new(AWN_TYPE_APPLET, 
					 "above-child", FALSE,
					 "visible-window", TRUE,
					 NULL);
	g_object_set( G_OBJECT(applet),
			  "uid", uid, 
		      "orient", orient,
		      "height", height,
		      NULL);
	return applet;
}

