/*
 * Copyright (C) 2008 Neil J. Patel <njpatel@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Authored by Neil Jagdish Patel <njpatel@gmail.com>
 *
 */

/*
 * AwnApp is the 'controller' for the panel. It will launch the panel with the
 * correct settings.
 *
 * In the future, AwnApp will be responsible for launching each Awn panel 
 * separately, and making dealing with the configuration backend for each
 * panel.
 */

#include <glib.h>

#include <stdio.h>
#include <string.h>

#include <libawn/awn-config-client.h>

#include "awn-app.h"
#include "awn-defines.h"
#include "awn-panel.h"

G_DEFINE_TYPE (AwnApp, awn_app, G_TYPE_OBJECT)

#define AWN_APP_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE(obj, \
        AWN_TYPE_APP, AwnAppPrivate))

struct _AwnAppPrivate
{
  AwnConfigClient *client;
  
  GtkWidget *panel;
};

/* GObject functions */
static void
awn_app_finalize (GObject *app)
{
  AwnAppPrivate *priv;
  
  g_return_if_fail (AWN_IS_APP (app));
  priv = AWN_APP (app)->priv;

  G_OBJECT_CLASS (awn_app_parent_class)->finalize (app);
}

#include "awn-app-glue.h"

static void
awn_app_class_init (AwnAppClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (klass);

  obj_class->finalize = awn_app_finalize;

  g_type_class_add_private (obj_class, sizeof (AwnAppPrivate)); 
  
  dbus_g_object_type_install_info (G_TYPE_FROM_CLASS (klass), 
                                   &dbus_glib_awn_app_object_info);
}

static void
awn_app_init (AwnApp *app)
{
  AwnAppPrivate *priv;
    
  priv = app->priv = AWN_APP_GET_PRIVATE (app);

  priv->client = awn_config_client_new ();
  
  priv->panel = awn_panel_new_from_config (priv->client);

  gtk_widget_show_all (priv->panel);
}

AwnApp*
awn_app_get_default (void)
{
  static AwnApp *app = NULL;
  
  if (app == NULL)
    app = g_object_new (AWN_TYPE_APP, 
                         NULL);

  return app;
}
