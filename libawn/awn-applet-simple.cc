/*
 * Copyright (c) 2007 Neil Jagdish Patel <njpatel@gmail.com>
 * Copyright (C) 2008 Rodney Cryderman <rcryderman@gmail.com>
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
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gtk/gtk.h>

#include "awn-applet-simple.h"

#include "awn-overlayable.h"
#include "awn-icon.h"
#include "awn-themed-icon.h"
#include "awn-utils.h"
#include "gseal-transition.h"

static void awn_applet_simple_overlayable_init (AwnOverlayableIface *iface);

G_DEFINE_TYPE_WITH_CODE (AwnAppletSimple, awn_applet_simple, AWN_TYPE_APPLET,
                         G_IMPLEMENT_INTERFACE (AWN_TYPE_OVERLAYABLE,
                                          awn_applet_simple_overlayable_init))

#define AWN_APPLET_SIMPLE_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj),\
    AWN_TYPE_APPLET_SIMPLE, \
    AwnAppletSimplePrivate))

struct _AwnAppletSimplePrivate
{
  GtkWidget *icon;
  gint       last_set_icon;
};

enum
{
  ICON_NONE=0,
  ICON_PIXBUF,
  ICON_CAIRO,
  ICON_THEMED_SIMPLE,
  ICON_THEMED_MANY
};

enum
{
  CLICKED,
  MIDDLE_CLICKED,
  MENU_POPUP,

  LAST_SIGNAL
};
static guint32 _simple_signals[LAST_SIGNAL] = { 0 };

/* GObject stuff */

/*
 * When the position changes, we need to update the size of the applet
 */
static void
awn_applet_simple_position_changed (AwnApplet *applet, GtkPositionType position)
{
  AwnAppletSimplePrivate *priv = AWN_APPLET_SIMPLE (applet)->priv;

  if (AWN_IS_ICON (priv->icon))
  {
    awn_icon_set_pos_type (AWN_ICON (priv->icon), position);
  }
}

static void
awn_applet_simple_offset_changed (AwnApplet *applet, gint offset)
{
  AwnAppletSimplePrivate *priv = AWN_APPLET_SIMPLE (applet)->priv;

  if (AWN_IS_ICON (priv->icon))
  {
    GtkAllocation alloc;
    gtk_widget_get_allocation (GTK_WIDGET (priv->icon), &alloc);
    gint x = alloc.x + alloc.width / 2;
    gint y = alloc.y + alloc.height / 2;
    offset = awn_applet_get_offset_at (applet, x, y);
    awn_icon_set_offset (AWN_ICON (priv->icon), offset);
  }
}

static void
awn_applet_simple_size_changed (AwnApplet *applet, gint size)
{
  AwnAppletSimplePrivate *priv = AWN_APPLET_SIMPLE (applet)->priv;
  
  if (!AWN_IS_ICON (priv->icon))
    return;

  if (priv->last_set_icon == ICON_THEMED_SIMPLE
      || priv->last_set_icon == ICON_THEMED_MANY)
    awn_themed_icon_set_size (AWN_THEMED_ICON (priv->icon), size);

  awn_applet_simple_position_changed (applet, 
                                      awn_applet_get_pos_type (applet));
}

static void
on_icon_clicked (AwnAppletSimple *simple, AwnIcon *icon)
{
  g_signal_emit (simple, _simple_signals[CLICKED], 0);
}

static void
on_icon_middle_clicked (AwnAppletSimple *simple, AwnIcon *icon)
{
  g_signal_emit (simple, _simple_signals[MIDDLE_CLICKED], 0);
}

static void
on_icon_menu_popup (AwnAppletSimple *simple, GdkEventButton *event, 
                    AwnIcon *icon)
{
  g_signal_emit (simple, _simple_signals[MENU_POPUP], 0, event);
}

static void
awn_applet_simple_size_allocate (GtkWidget *widget, GtkAllocation *alloc)
{
  AwnAppletSimplePrivate *priv = AWN_APPLET_SIMPLE_GET_PRIVATE (widget);

  if (AWN_IS_ICON (priv->icon))
  {
    gint x = alloc->x + alloc->width / 2;
    gint y = alloc->y + alloc->height / 2;
    gint offset = awn_applet_get_offset_at (AWN_APPLET (widget), x, y);
    awn_icon_set_offset (AWN_ICON (priv->icon), offset);
  }
}

static AwnEffects*
awn_applet_simple_get_effects (AwnOverlayable *simple)
{
  AwnAppletSimplePrivate *priv = AWN_APPLET_SIMPLE_GET_PRIVATE (simple);

  if (AWN_IS_ICON (priv->icon))
    return awn_overlayable_get_effects (AWN_OVERLAYABLE (priv->icon));

  return NULL;
}

static void
awn_applet_simple_menu_creation (AwnApplet *applet, GtkMenu *menu)
{
  /* FIXME: If last_icon_type == ICON_THEMED_*, add "Clear custom icons" */
}

static void
awn_applet_simple_constructed (GObject *object)
{
  G_OBJECT_CLASS (awn_applet_simple_parent_class)->constructed (object);

  AwnAppletSimple        *applet = AWN_APPLET_SIMPLE (object);
  AwnAppletSimplePrivate *priv = applet->priv;
  gchar                  *applet_name;
  
  g_object_get (object,
                "canonical-name",&applet_name,
                NULL);

  priv->icon = awn_themed_icon_new ();
  
  g_object_set (priv->icon,
                "applet-name",applet_name,
                NULL);
  awn_icon_set_pos_type (AWN_ICON (priv->icon), 
                         awn_applet_get_pos_type (AWN_APPLET (object)));
  awn_icon_set_offset (AWN_ICON (priv->icon),
                       awn_applet_get_offset (AWN_APPLET (object)));
  g_signal_connect_swapped (priv->icon, "clicked", 
                            G_CALLBACK (on_icon_clicked), object);
  g_signal_connect_swapped (priv->icon, "middle-clicked", 
                            G_CALLBACK (on_icon_middle_clicked), object);
  g_signal_connect_swapped (priv->icon, "context-menu-popup", 
                            G_CALLBACK (on_icon_menu_popup), object);
  gtk_container_add (GTK_CONTAINER (applet), priv->icon);
  gtk_widget_show (priv->icon);
  g_free (applet_name);
}

static void
awn_applet_simple_dispose (GObject *object)
{
  G_OBJECT_CLASS (awn_applet_simple_parent_class)->dispose (object);
}

static void
awn_applet_simple_overlayable_init (AwnOverlayableIface *iface)
{
  iface->get_effects = awn_applet_simple_get_effects;
}

static void
awn_applet_simple_class_init (AwnAppletSimpleClass *klass)
{
  GObjectClass   *obj_class = G_OBJECT_CLASS (klass);
  AwnAppletClass *app_class = AWN_APPLET_CLASS (klass);
      
  obj_class->dispose     = awn_applet_simple_dispose;
  obj_class->constructed = awn_applet_simple_constructed;

  app_class->position_changed = awn_applet_simple_position_changed;
  app_class->offset_changed = awn_applet_simple_offset_changed;
  app_class->size_changed   = awn_applet_simple_size_changed;
  app_class->menu_creation  = awn_applet_simple_menu_creation;

  _simple_signals[CLICKED] =
    g_signal_new ("clicked",
      G_OBJECT_CLASS_TYPE (obj_class),
      G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
      G_STRUCT_OFFSET (AwnAppletSimpleClass, clicked),
      NULL, NULL,
      g_cclosure_marshal_VOID__VOID,
      G_TYPE_NONE, 0);

  _simple_signals[MIDDLE_CLICKED] =
    g_signal_new ("middle-clicked",
      G_OBJECT_CLASS_TYPE (obj_class),
      G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
      G_STRUCT_OFFSET (AwnAppletSimpleClass, middle_clicked),
      NULL, NULL,
      g_cclosure_marshal_VOID__VOID,
      G_TYPE_NONE, 0);

  /**
   * AwnAppletSimple::context-menu-popup:
   * @applet: the object which received the signal.
   * @event: the #GdkEventButton which triggered this signal.
   *
   * The ::context-menu-popup signal will be emitted when right mouse button
   * is pressed on the icon.
   */
  _simple_signals[MENU_POPUP] =
    g_signal_new ("context-menu-popup",
      G_OBJECT_CLASS_TYPE (obj_class),
      G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
      G_STRUCT_OFFSET (AwnAppletSimpleClass, context_menu_popup),
      NULL, NULL,
      g_cclosure_marshal_VOID__BOXED,
      G_TYPE_NONE, 1,
      GDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);

  g_type_class_add_private (obj_class, sizeof (AwnAppletSimplePrivate));
}

static void
awn_applet_simple_init (AwnAppletSimple *simple)
{
  AwnAppletSimplePrivate *priv;
  
  priv = simple->priv = AWN_APPLET_SIMPLE_GET_PRIVATE(simple);

  priv->last_set_icon = ICON_NONE;

  g_signal_connect (simple, "size-allocate",
                    G_CALLBACK (awn_applet_simple_size_allocate), NULL);
}

/**
 * awn_applet_simple_new:
 * @canonical_name: The canonical name of the applet (see
 * #AwnApplet:canonical-name).
 * @uid: The unique identifier of the instance of the applet on the dock.
 * @panel_id: The ID of the panel to connect to.
 *
 * Creates a new #AwnAppletSimple object.  This applet will have awn-effects
 * effects applied to its icon automatically if awn_applet_simple_set_icon() or
 * awn_applet_simple_set_temp_icon() are used to specify the applet icon.
 *
 * Returns: a new instance of an applet.
 */
GtkWidget*
awn_applet_simple_new (const gchar* canonical_name,
                       const gchar *uid,
                       gint panel_id)
{
  AwnAppletSimple *simple;

  simple = g_object_new (AWN_TYPE_APPLET_SIMPLE,
                         "canonical-name", canonical_name,
                         "uid", uid,
                         "panel-id", panel_id,
                         NULL);

  return GTK_WIDGET (simple);
}

/*
 * PUBLIC FUNCTIONS
 */
void    
awn_applet_simple_set_icon_pixbuf (AwnAppletSimple  *applet,
                                   GdkPixbuf        *pixbuf)
{
  AwnAppletSimplePrivate *priv;

  g_return_if_fail (AWN_IS_APPLET_SIMPLE (applet));
  g_return_if_fail (GDK_IS_PIXBUF (pixbuf));
  priv = applet->priv;

  /* Make sure AwnThemedIcon doesn't update the icon of it's own accord */
  if (priv->last_set_icon == ICON_THEMED_SIMPLE 
      || priv->last_set_icon == ICON_THEMED_MANY)
    awn_themed_icon_clear_info (AWN_THEMED_ICON (priv->icon));

  priv->last_set_icon = ICON_PIXBUF;
  awn_icon_set_from_pixbuf (AWN_ICON (priv->icon), pixbuf);
}

void 
awn_applet_simple_set_icon_context (AwnAppletSimple  *applet, 
                                    cairo_t          *cr)
{
  AwnAppletSimplePrivate *priv;

  g_return_if_fail (AWN_IS_APPLET_SIMPLE (applet));
  g_return_if_fail (cr);
  priv = applet->priv;

  /* Make sure AwnThemedIcon doesn't update the icon of it's own accord */
  if (priv->last_set_icon == ICON_THEMED_SIMPLE 
      || priv->last_set_icon == ICON_THEMED_MANY)
    awn_themed_icon_clear_info (AWN_THEMED_ICON (priv->icon));

  priv->last_set_icon = ICON_CAIRO;
  awn_icon_set_from_context (AWN_ICON (priv->icon), cr);
}

void 
awn_applet_simple_set_icon_surface (AwnAppletSimple  *applet,
                                    cairo_surface_t  *surface)
{
  AwnAppletSimplePrivate *priv;

  g_return_if_fail (AWN_IS_APPLET_SIMPLE (applet));
  g_return_if_fail (surface);
  priv = applet->priv;

  /* Make sure AwnThemedIcon doesn't update the icon of it's own accord */
  if (priv->last_set_icon == ICON_THEMED_SIMPLE
      || priv->last_set_icon == ICON_THEMED_MANY)
    awn_themed_icon_clear_info (AWN_THEMED_ICON (priv->icon));

  priv->last_set_icon = ICON_CAIRO;
  awn_icon_set_from_surface (AWN_ICON (priv->icon), surface);
}

void
awn_applet_simple_set_icon_name (AwnAppletSimple  *applet,
                                 const gchar      *icon_name)
{
  gchar *applet_name;  
  
  g_return_if_fail (AWN_IS_APPLET_SIMPLE (applet));
  g_return_if_fail (icon_name);

  g_object_get (applet,
                "canonical-name",&applet_name,
                NULL);

  g_return_if_fail (applet_name);

  applet->priv->last_set_icon = ICON_THEMED_SIMPLE;
  awn_themed_icon_set_size (AWN_THEMED_ICON (applet->priv->icon),
                            awn_applet_get_size (AWN_APPLET (applet)));  
  awn_themed_icon_set_info_simple (AWN_THEMED_ICON (applet->priv->icon),
                                   applet_name,
                                   awn_applet_get_uid (AWN_APPLET (applet)),
                                   icon_name);
  g_free (applet_name);
}
                                    
void   
awn_applet_simple_set_icon_info (AwnAppletSimple  *applet,
                                 GStrv            states,
                                 GStrv            icon_names)
{
  gchar *applet_name=NULL;
  
  g_return_if_fail (AWN_IS_APPLET_SIMPLE (applet));
  g_return_if_fail (states);
  g_return_if_fail (icon_names);

  g_object_get (applet,
                "canonical-name",&applet_name,
                NULL);  
  g_return_if_fail (applet_name);
  applet->priv->last_set_icon = ICON_THEMED_MANY;
  awn_themed_icon_set_size (AWN_THEMED_ICON (applet->priv->icon),
                            awn_applet_get_size (AWN_APPLET (applet)));
  awn_themed_icon_set_info (AWN_THEMED_ICON (applet->priv->icon),
                            applet_name,
                            awn_applet_get_uid (AWN_APPLET (applet)),
                            states,
                            icon_names);
  g_free (applet_name);
}
                                    
void 
awn_applet_simple_set_icon_state   (AwnAppletSimple  *applet,  
                                    const gchar      *state)
{
  g_return_if_fail (AWN_IS_APPLET_SIMPLE (applet));
  g_return_if_fail (state);

  if (applet->priv->last_set_icon == ICON_THEMED_MANY)
    awn_themed_icon_set_state (AWN_THEMED_ICON (applet->priv->icon), state);
}

void 
awn_applet_simple_set_tooltip_text (AwnAppletSimple  *applet,
                                    const gchar      *title)
{
  g_return_if_fail (AWN_IS_APPLET_SIMPLE (applet));
  
  awn_icon_set_tooltip_text (AWN_ICON (applet->priv->icon), title);
}

gchar *
awn_applet_simple_get_tooltip_text (AwnAppletSimple  *applet)
{
  g_return_val_if_fail (AWN_IS_APPLET_SIMPLE (applet), NULL);

  return awn_icon_get_tooltip_text (AWN_ICON (applet->priv->icon));
}

void 
awn_applet_simple_set_message (AwnAppletSimple  *applet,
                               const gchar      *message)
{
  g_return_if_fail (AWN_IS_APPLET_SIMPLE (applet));

  // FIXME: OverlayText
}

gchar *
awn_applet_simple_get_message (AwnAppletSimple  *applet)
{
  g_return_val_if_fail (AWN_IS_APPLET_SIMPLE (applet), NULL);

  // FIXME: OverlayText
  return NULL;
}

void 
awn_applet_simple_set_progress (AwnAppletSimple  *applet,
                                gfloat            progress)
{
  g_return_if_fail (AWN_IS_APPLET_SIMPLE (applet));

  // FIXME: OverlayProgress
}

gfloat  
awn_applet_simple_get_progress (AwnAppletSimple  *applet)
{
  g_return_val_if_fail (AWN_IS_APPLET_SIMPLE (applet), 0.0);

  // FIXME: OverlayProgress
  return 0.0f;
}

AwnIcon *  
awn_applet_simple_get_icon (AwnAppletSimple  *applet)
{
  g_return_val_if_fail (AWN_IS_APPLET_SIMPLE (applet), NULL);

  return AWN_ICON (applet->priv->icon);
}

void  
awn_applet_simple_set_effect (AwnAppletSimple  *applet,
                              AwnEffect         effect)
{
  g_return_if_fail (AWN_IS_APPLET_SIMPLE (applet));

  awn_icon_set_effect (AWN_ICON (applet->priv->icon), effect);
}

