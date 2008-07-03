/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor Boston, MA 02110-1301,  USA
 */
 
/* awn-icons.c */

#include "awn-icons.h"

#include <string.h>

G_DEFINE_TYPE (AwnIcons, awn_icons, G_TYPE_OBJECT)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), LIBAWN_TYPE_AWN_ICONS, AwnIconsPrivate))

typedef struct _AwnIconsPrivate AwnIconsPrivate;

struct _AwnIconsPrivate {
  GtkWindow *   icon_window;    //used if Non-NULL for drag and drop support
  
  gchar **  states;
  gchar **  icon_names;
  gchar *   applet_name;
  gchar *   uid;
  gchar *   theme_name;
  
  gint  height;
  gint  cur_icon;
  gint  count;
};

void awn_icons_set_icons_info(AwnIcons * icons,gchar * theme_name, 
                             gchar * applet_name,gchar * uid,gint height,
                             gchar **states,gchar **icon_names)
{
  g_return_if_fail(icons);
  g_return_if_fail(theme_name);  
  g_return_if_fail(applet_name);
  g_return_if_fail(uid);
  g_return_if_fail(states);
  g_return_if_fail(icon_names);

  int count;
  AwnIconsPrivate *priv=GET_PRIVATE(icons); 
  
  for (count=0;states[count];count++);
  priv->count = count;
  
  for (count=0;icon_names[count];count++);
  g_return_if_fail(count == priv->count);

  if (priv->states)
  {
    for(count=0;icon_names[count];count++)
    {
      g_free(icon_names[count]);
      g_free(states[count]);
    }
    g_free(priv->states);
    g_free(priv->icon_names);
  }
  
  priv->states = g_malloc( sizeof(gchar *) * count);
  priv->icon_names = g_malloc( sizeof(gchar *) * count);
  
  for (count=0; count < priv->count; count ++)
  {
    priv->states[count] = g_strdup(states[count]);
    priv->icon_names[count] = g_strdup(icon_names[count]);
  }
  priv->states[count] = NULL;
  priv->icon_names[count] = NULL;
  
  if (priv->theme_name)
  {
    g_free(priv->theme_name);
  }
  priv->theme_name = g_strdup(theme_name);

  if (priv->applet_name)
  {
    g_free(priv->applet_name);
  }
  priv->applet_name = g_strdup(applet_name);
  
  if (priv->uid)
  {
    g_free(priv->uid);
  }
  priv->uid = g_strdup(uid);
  priv->height = height;
}

void awn_icons_set_icon_info(AwnIcons * icons,gchar * theme_name, 
                             gchar * applet_name,gchar * uid, gint height,
                             gchar *state,gchar *icon_name)
{
  g_return_if_fail(icons);  
  gchar *states[] = {NULL,NULL};
  gchar *icon_names[] = {NULL,NULL};
  states[0] = state;
  icon_names[0] = icon_name;
  awn_icons_set_icons_info(icons,theme_name,applet_name,
                           uid,height,states,icon_names);
  
}

GdkPixbuf * awn_icons_get_icon(AwnIcons * icons, gchar * state)
{
  g_return_val_if_fail(icons,NULL);
  g_return_val_if_fail(state,NULL);
  
  gint count;
  GdkPixbuf * pixbuf = NULL;  
  AwnIconsPrivate *priv=GET_PRIVATE(icons);
  
  for (count = 0; priv->states[count]; count++)
  {
    if ( strcmp(state,priv->states[count]) == 0 )
    {
      gchar * name=NULL;
      priv->count = count;
      name = g_strdup_printf("%s-%s-%s",priv->icon_names[count],priv->applet_name,
                            priv->uid);
      pixbuf = gtk_icon_theme_load_icon(gtk_icon_theme_get_default(), name , 
                                        priv->height, 0, NULL);
      g_free(name);
      if (pixbuf)
      {
        break;
      }
      name = g_strdup_printf("%s-%s",priv->icon_names[count],priv->applet_name);
      pixbuf = gtk_icon_theme_load_icon(gtk_icon_theme_get_default(), name , 
                                        priv->height, 0, NULL);
      g_free(name);
      if (pixbuf)
      {
        break;
      }
      pixbuf = gtk_icon_theme_load_icon(gtk_icon_theme_get_default(), 
                                        priv->icon_names[count],priv->height, 
                                        0, NULL);
      if (pixbuf)
      {
        break;
      }
      pixbuf = gtk_icon_theme_load_icon(gtk_icon_theme_get_default(), 
                                        "stock_stop",priv->height, 0, NULL);
      if (pixbuf)
      {
        break;
      }
      pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, 
                              priv->height,priv->height);
      gdk_pixbuf_fill(pixbuf, 0xee221155);
      break;
      
    }
  }
  return pixbuf;
}


GdkPixbuf * awn_icons_get_icon_simple(AwnIcons * icons)
{
  g_return_val_if_fail(icons,NULL);

  AwnIconsPrivate *priv=GET_PRIVATE(icons);  
  return awn_icons_get_icon(icons,priv->states[priv->count]);
}

static void
awn_icons_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
  switch (property_id) {
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
awn_icons_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
  switch (property_id) {
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
awn_icons_dispose (GObject *object)
{
  if (G_OBJECT_CLASS (awn_icons_parent_class)->dispose)
    G_OBJECT_CLASS (awn_icons_parent_class)->dispose (object);
}

static void
awn_icons_finalize (GObject *object)
{
  if (G_OBJECT_CLASS (awn_icons_parent_class)->finalize)
    G_OBJECT_CLASS (awn_icons_parent_class)->finalize (object);
}

static void
awn_icons_class_init (AwnIconsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (AwnIconsPrivate));

  object_class->get_property = awn_icons_get_property;
  object_class->set_property = awn_icons_set_property;
  object_class->dispose = awn_icons_dispose;
  object_class->finalize = awn_icons_finalize;
}

static void
awn_icons_init (AwnIcons *self)
{
  AwnIconsPrivate *priv=GET_PRIVATE(self);
  priv->icon_window = NULL;
  priv->states = NULL;
  priv->icon_names = NULL;
  priv->uid = NULL;
  priv->theme_name = NULL;
  priv->cur_icon = 0;
  priv->count = 0;  
  priv->height = 0;
}

AwnIcons*
awn_icons_new (void)
{
  return g_object_new (LIBAWN_TYPE_AWN_ICONS, NULL);
}


