/*
 * Copyright (C) 2008 Neil Jagdish Patel
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
 * Authored by Neil Jagdish Patel <njpatel@gmail.com>
 *
 */
#include <gdk/gdkx.h>
#include <cairo/cairo-xlib.h>

#include "awn-config.h"
#include "awn-icon.h"
#include "awn-utils.h"
#include "awn-overlayable.h"

#include "gseal-transition.h"

#define APPLY_SIZE_MULTIPLIER(x)	(x)*6/5

static void awn_icon_overlayable_init (AwnOverlayableIface *iface);

static AwnEffects* awn_icon_get_effects (AwnOverlayable *icon);

G_DEFINE_TYPE_WITH_CODE (AwnIcon, awn_icon, GTK_TYPE_DRAWING_AREA,
                         G_IMPLEMENT_INTERFACE (AWN_TYPE_OVERLAYABLE,
                                                awn_icon_overlayable_init))

#define AWN_ICON_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj),\
  AWN_TYPE_ICON, \
  AwnIconPrivate))

struct _AwnIconPrivate
{
  AwnEffects   *effects;
  GtkWidget    *tooltip;

  gboolean bind_effects;
  gboolean hover_effects_enable;
  gboolean left_was_pressed;
  gboolean middle_was_pressed;

  gint long_press_timeout;
  gdouble press_start_x;
  gdouble press_start_y;
  guint long_press_timer;
  gboolean long_press_emitted;

  GtkPositionType position;
  gint offset;
  gint icon_width;
  gint icon_height;
  gint size;

  /* Info relating to the current icon */
  cairo_surface_t *icon_srfc;
};

enum
{
  PROP_0,

  PROP_BIND_EFFECTS,

  PROP_ICON_WIDTH,
  PROP_ICON_HEIGHT,
  PROP_LONG_PRESS_TIMEOUT
};

enum
{
  SIZE_CHANGED,

  CLICKED,
  MIDDLE_CLICKED,
  LONG_PRESS,
  MENU_POPUP,

  LAST_SIGNAL
};
static guint32 _icon_signals[LAST_SIGNAL] = { 0 };

/* GObject stuff */
static gboolean
awn_icon_enter_notify_event (GtkWidget *widget, GdkEventCrossing *event)
{
  AwnIconPrivate *priv = AWN_ICON (widget)->priv;

  if (priv->hover_effects_enable)
  {
    awn_effects_start (priv->effects, AWN_EFFECT_HOVER);
  }

  return FALSE;
}

static gboolean
awn_icon_leave_notify_event (GtkWidget *widget, GdkEventCrossing *event)
{
  AwnIconPrivate *priv = AWN_ICON (widget)->priv;

  if (priv->hover_effects_enable)
  {
    awn_effects_stop (priv->effects, AWN_EFFECT_HOVER);
  }

  return FALSE;
}

static void
awn_icon_update_effects (GtkWidget *widget, gpointer data)
{
  AwnIconPrivate *priv = AWN_ICON (widget)->priv;
  DesktopAgnosticConfigClient *client = 
      awn_config_get_default (AWN_PANEL_ID_DEFAULT, NULL);
  GObject *fx = G_OBJECT (priv->effects);

  if (gtk_widget_is_composited (widget))
  {
    /* optimize the render speed for GTK+ <2.17.3.*/
    /* TODO:
     * We need to fix various image operations in awn-effects-ops to be able
     * to use indirect painting for Gtk's CSW.
     * See https://bugzilla.gnome.org/show_bug.cgi?id=597301.
     */
#if GTK_CHECK_VERSION(2,17,3)
    g_object_set (priv->effects, "indirect-paint", TRUE, NULL);
#else
    g_object_set (priv->effects, "indirect-paint", FALSE, NULL);
#endif

    if (priv->bind_effects)
    {
      desktop_agnostic_config_client_bind (
          client, "effects", "icon_effect",
          fx, "effects", TRUE,
          DESKTOP_AGNOSTIC_CONFIG_BIND_METHOD_FALLBACK, NULL
      );
    }
  }
  else
  {
    if (priv->bind_effects)
    {
      desktop_agnostic_config_client_unbind (
          client, "effects", "icon_effect",
          fx, "effects", TRUE,
          DESKTOP_AGNOSTIC_CONFIG_BIND_METHOD_FALLBACK, NULL
      );
    }

    g_object_set (priv->effects,
                  "effects", 0,
                  "indirect-paint", TRUE, NULL);
    /* we are also forcing icon-effects to "Simple", to prevent clipping
     * the icon in our small window
     */
  }
}

static gboolean
awn_icon_expose_event (GtkWidget *widget, GdkEventExpose *event)
{
  AwnIconPrivate *priv = AWN_ICON (widget)->priv;
  cairo_t        *cr;

  g_return_val_if_fail(priv->icon_srfc, FALSE);

  /* clip the drawing region, nvidia likes it */
  cr = awn_effects_cairo_create_clipped (priv->effects, event);

  /* if we're RGBA we have transparent background (awn_icon_make_transparent),
   * otherwise default widget background color
   */

  cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
  cairo_set_source_surface (cr, priv->icon_srfc, 0, 0);
  cairo_paint (cr);

  /* let effects know we're finished */
  awn_effects_cairo_destroy (priv->effects);

  return FALSE;
}

static void
awn_icon_size_request (GtkWidget *widget, GtkRequisition *req)
{
  AwnIconPrivate *priv = AWN_ICON_GET_PRIVATE (widget);

  switch (priv->position)
  {
    case GTK_POS_TOP:
    case GTK_POS_BOTTOM:
      req->width = APPLY_SIZE_MULTIPLIER(priv->icon_width);
      req->height = priv->icon_height + priv->effects->icon_offset;
      break;
      
    default:
      req->width = priv->icon_width + priv->effects->icon_offset;
      req->height = APPLY_SIZE_MULTIPLIER(priv->icon_height);
      break;
  }
}

static gboolean
awn_icon_long_press_timeout (gpointer data)
{
  AwnIcon *icon = AWN_ICON (data);
  AwnIconPrivate *priv = icon->priv;

  // get mouse position, so we know we aren't in d&d
  gint current_x, current_y;
  gdk_display_get_pointer (gtk_widget_get_display (GTK_WIDGET (icon)),
                           NULL, &current_x, &current_y, NULL);

  if (gtk_drag_check_threshold (GTK_WIDGET (icon), 
                                priv->press_start_x, priv->press_start_y, 
                                current_x, current_y) == FALSE)
  {
    if (g_signal_has_handler_pending (icon, 
                                      _icon_signals[LONG_PRESS], 0, TRUE))
    {
      g_signal_emit (icon, _icon_signals[LONG_PRESS], 0);
      priv->long_press_emitted = TRUE;
    }
  }
  else
  {
    // if we're in drag we won't emit clicked either
    priv->long_press_emitted = TRUE;
  }

  priv->long_press_timer = 0;

  return FALSE;
}

static gboolean
awn_icon_pressed (AwnIcon *icon, GdkEventButton *event, gpointer data)
{
  AwnIconPrivate *priv = icon->priv;

  if (event->type != GDK_BUTTON_PRESS) return FALSE;

  switch (event->button)
  {
    case 1:
      priv->left_was_pressed = TRUE;
      g_object_set (priv->effects, "depressed", TRUE, NULL);
      priv->long_press_emitted = FALSE;
      if (priv->long_press_timer == 0)
      {
        priv->press_start_x = event->x_root;
        priv->press_start_y = event->y_root;
        // make sure the timer has lower priority than X-events
        priv->long_press_timer = g_timeout_add_full (
            G_PRIORITY_DEFAULT + 10,
            priv->long_press_timeout,
            awn_icon_long_press_timeout,
            icon,
            NULL);
      }
      break;
    case 2:
      priv->middle_was_pressed = TRUE;
      break;
    case 3:
      g_signal_emit (icon, _icon_signals[MENU_POPUP], 0, event);
      break;
    default:
      break;
  }

  return FALSE;
}

static gboolean
awn_icon_released (AwnIcon *icon, GdkEventButton *event, gpointer data)
{
  AwnIconPrivate *priv = icon->priv;

  switch (event->button)
  {
    case 1:
      if (priv->left_was_pressed)
      {
        priv->left_was_pressed = FALSE;
        g_object_set (priv->effects, "depressed", FALSE, NULL);
        if (priv->long_press_timer)
        {
          g_source_remove (priv->long_press_timer);
          priv->long_press_timer = 0;
        }
        // emit clicked only if long-press wasn't emitted
        if (priv->long_press_emitted == FALSE)
          awn_icon_clicked (icon);
      }
      break;
    case 2:
      if (priv->middle_was_pressed)
      {
        priv->middle_was_pressed = FALSE;
        awn_icon_middle_clicked (icon);
      }
      break;
    default:
      break;
  }

  return FALSE;
}

static void
awn_icon_constructed (GObject *object)
{
  if (G_OBJECT_CLASS (awn_icon_parent_class)->constructed)
    G_OBJECT_CLASS (awn_icon_parent_class)->constructed (object);

  AwnIcon *icon = AWN_ICON (object);
  AwnIconPrivate *priv = icon->priv;
  GError *error = NULL;

  DesktopAgnosticConfigClient *client = awn_config_get_default (AWN_PANEL_ID_DEFAULT, &error);

  if (error)
  {
    g_critical ("An error occurred while trying to retrieve the configuration client: %s",
                error->message);
    g_error_free (error);
    return;
  }

  desktop_agnostic_config_client_bind (client, "shared", "long_press_timeout",
                                       object, "long_press_timeout", TRUE,
                                       DESKTOP_AGNOSTIC_CONFIG_BIND_METHOD_FALLBACK,
                                       NULL);

  if (!priv->bind_effects) return;

  GObject *fx = G_OBJECT (priv->effects);

  if (gtk_widget_is_composited (GTK_WIDGET (object)))
  {
    desktop_agnostic_config_client_bind (client, "effects", "icon_effect",
                                         fx, "effects", TRUE,
                                         DESKTOP_AGNOSTIC_CONFIG_BIND_METHOD_FALLBACK,
                                         NULL);
  }
  desktop_agnostic_config_client_bind (client, "effects", "icon_alpha",
                                       fx, "icon-alpha", TRUE,
                                       DESKTOP_AGNOSTIC_CONFIG_BIND_METHOD_FALLBACK,
                                       NULL);
  desktop_agnostic_config_client_bind (client, "effects", "reflection_alpha_multiplier",
                                       fx, "reflection-alpha", TRUE,
                                       DESKTOP_AGNOSTIC_CONFIG_BIND_METHOD_FALLBACK,
                                       NULL);
  desktop_agnostic_config_client_bind (client, "effects", "reflection_offset",
                                       fx, "reflection-offset", TRUE,
                                       DESKTOP_AGNOSTIC_CONFIG_BIND_METHOD_FALLBACK,
                                       NULL);
  desktop_agnostic_config_client_bind (client, "effects", "active_rect_color",
                                       fx, "active-rect-color", TRUE,
                                       DESKTOP_AGNOSTIC_CONFIG_BIND_METHOD_FALLBACK,
                                       NULL);
  desktop_agnostic_config_client_bind (client, "effects", "active_rect_outline",
                                       fx, "active-rect-outline", TRUE,
                                       DESKTOP_AGNOSTIC_CONFIG_BIND_METHOD_FALLBACK,
                                       NULL);
  desktop_agnostic_config_client_bind (client, "effects", "dot_color",
                                       fx, "dot-color", TRUE,
                                       DESKTOP_AGNOSTIC_CONFIG_BIND_METHOD_FALLBACK,
                                       NULL);
  desktop_agnostic_config_client_bind (client, "effects", "show_shadows",
                                       fx, "make-shadow", TRUE,
                                       DESKTOP_AGNOSTIC_CONFIG_BIND_METHOD_FALLBACK,
                                       NULL);
  desktop_agnostic_config_client_bind (client, "effects", "arrow_icon",
                                       fx, "arrow-png", TRUE,
                                       DESKTOP_AGNOSTIC_CONFIG_BIND_METHOD_FALLBACK,
                                       NULL);
  desktop_agnostic_config_client_bind (client, 
                                       "effects", "active_background_icon",
                                       fx, "custom-active-png", TRUE,
                                       DESKTOP_AGNOSTIC_CONFIG_BIND_METHOD_FALLBACK,
                                       NULL);
}

static void
awn_icon_get_property (GObject    *object,
                       guint       prop_id,
                       GValue     *value,
                       GParamSpec *pspec)
{
  AwnIcon *icon = AWN_ICON (object);
  AwnIconPrivate *priv;

  g_return_if_fail (AWN_IS_ICON (object));
  priv = icon->priv;

  switch (prop_id)
  {
    case PROP_BIND_EFFECTS:
      g_value_set_boolean (value, priv->bind_effects);
      break;
    case PROP_ICON_WIDTH:
      g_value_set_int (value, priv->icon_width);
      break;
    case PROP_ICON_HEIGHT:
      g_value_set_int (value, priv->icon_height);
      break;
    case PROP_LONG_PRESS_TIMEOUT:
      g_value_set_int (value, priv->long_press_timeout);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

static void
awn_icon_set_property (GObject      *object,
                       guint         prop_id,
                       const GValue *value,
                       GParamSpec   *pspec)
{
  AwnIcon *icon = AWN_ICON (object);
  AwnIconPrivate *priv;

  g_return_if_fail (AWN_IS_ICON (object));
  priv = icon->priv;

  switch (prop_id)
  {
    case PROP_BIND_EFFECTS:
      icon->priv->bind_effects = g_value_get_boolean (value);
      break;
    case PROP_ICON_WIDTH:
      awn_icon_set_custom_paint(icon,
                                g_value_get_int (value),
                                priv->icon_height);
      break;
    case PROP_ICON_HEIGHT:
      awn_icon_set_custom_paint(icon,
                                priv->icon_width,
                                g_value_get_int (value));
      break;
    case PROP_LONG_PRESS_TIMEOUT:
      icon->priv->long_press_timeout = g_value_get_int (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

static void
awn_icon_dispose (GObject *object)
{
  AwnIconPrivate *priv;
  GError *error = NULL;
  DesktopAgnosticConfigClient *client;

  g_return_if_fail (AWN_IS_ICON (object));
  priv = AWN_ICON (object)->priv;

  client = awn_config_get_default (AWN_PANEL_ID_DEFAULT, &error);

  if (error)
  {
    g_warning ("An error occurred while trying to retrieve the configuration client: %s",
               error->message);
    g_error_free (error);
  }
  else
  {
    desktop_agnostic_config_client_unbind_all_for_object (client,
                                                          object, NULL);
  }

  if (priv->tooltip)
    gtk_widget_destroy (priv->tooltip);
  priv->tooltip = NULL;

  if (priv->icon_srfc)
    cairo_surface_destroy (priv->icon_srfc);
  priv->icon_srfc = NULL;

  if (priv->long_press_timer)
    g_source_remove (priv->long_press_timer);
  priv->long_press_timer = 0;

  G_OBJECT_CLASS (awn_icon_parent_class)->dispose (object);
}

static void
awn_icon_finalize (GObject *object)
{
  AwnIconPrivate *priv;

  g_return_if_fail (AWN_IS_ICON (object));
  priv = AWN_ICON (object)->priv;

  if (priv->effects)
  {
    g_object_unref (priv->effects);
  }
  G_OBJECT_CLASS (awn_icon_parent_class)->finalize (object);
}


static void
awn_icon_overlayable_init (AwnOverlayableIface *iface)
{
  iface->get_effects = awn_icon_get_effects;
}

static void
awn_icon_class_init (AwnIconClass *klass)
{
  GObjectClass   *obj_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *wid_class = GTK_WIDGET_CLASS (klass);

  obj_class->constructed  = awn_icon_constructed;
  obj_class->get_property = awn_icon_get_property;
  obj_class->set_property = awn_icon_set_property;
  obj_class->dispose      = awn_icon_dispose;
  obj_class->finalize     = awn_icon_finalize;

  wid_class->size_request       = awn_icon_size_request;
  wid_class->expose_event       = awn_icon_expose_event;
  wid_class->enter_notify_event = awn_icon_enter_notify_event;
  wid_class->leave_notify_event = awn_icon_leave_notify_event;

  /* Class properties */
  g_object_class_install_property (obj_class,
    PROP_BIND_EFFECTS,
    g_param_spec_boolean ("bind-effects",
                          "Bind effects",
                          "If set to true, will load and bind effect property"
                          " values from config client",
                          TRUE,
                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY |
                          G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (obj_class,
    PROP_ICON_WIDTH,
    g_param_spec_int ("icon-width",
                      "Icon width",
                      "Current icon width",
                      0, G_MAXINT, 0,
                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (obj_class,
    PROP_ICON_HEIGHT,
    g_param_spec_int ("icon-height",
                      "Icon height",
                      "Current icon height",
                      0, G_MAXINT, 0,
                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (obj_class,
    PROP_LONG_PRESS_TIMEOUT,
    g_param_spec_int ("long-press-timeout",
                      "Long press timeout",
                      "Timeout after which long-press signal is emit",
                      250, 10000, 750,
                      G_PARAM_READWRITE | G_PARAM_CONSTRUCT |
                      G_PARAM_STATIC_STRINGS));

  /* Signals */
  _icon_signals[SIZE_CHANGED] =
    g_signal_new ("size-changed",
      G_OBJECT_CLASS_TYPE (obj_class),
      G_SIGNAL_RUN_FIRST,
      G_STRUCT_OFFSET (AwnIconClass, size_changed),
      NULL, NULL,
      g_cclosure_marshal_VOID__VOID, 
      G_TYPE_NONE, 0);

  _icon_signals[CLICKED] =
    g_signal_new ("clicked",
      G_OBJECT_CLASS_TYPE (obj_class),
      G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
      G_STRUCT_OFFSET (AwnIconClass, clicked),
      NULL, NULL,
      g_cclosure_marshal_VOID__VOID,
      G_TYPE_NONE, 0);

  _icon_signals[MIDDLE_CLICKED] =
    g_signal_new ("middle-clicked",
      G_OBJECT_CLASS_TYPE (obj_class),
      G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
      G_STRUCT_OFFSET (AwnIconClass, middle_clicked),
      NULL, NULL,
      g_cclosure_marshal_VOID__VOID,
      G_TYPE_NONE, 0);

  _icon_signals[LONG_PRESS] =
    g_signal_new ("long-press",
      G_OBJECT_CLASS_TYPE (obj_class),
      G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
      G_STRUCT_OFFSET (AwnIconClass, long_press),
      NULL, NULL,
      g_cclosure_marshal_VOID__VOID,
      G_TYPE_NONE, 0);

  _icon_signals[MENU_POPUP] =
    g_signal_new ("context-menu-popup",
      G_OBJECT_CLASS_TYPE (obj_class),
      G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
      G_STRUCT_OFFSET (AwnIconClass, context_menu_popup),
      NULL, NULL,
      g_cclosure_marshal_VOID__BOXED,
      G_TYPE_NONE, 1,
      GDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);

  g_type_class_add_private (obj_class, sizeof (AwnIconPrivate));
}

static void
awn_icon_init (AwnIcon *icon)
{
  AwnIconPrivate *priv;

  priv = icon->priv = AWN_ICON_GET_PRIVATE (icon);

  priv->hover_effects_enable = TRUE;
  priv->icon_srfc = NULL;
  priv->position = GTK_POS_BOTTOM;
  priv->offset = 0;
  priv->size = 50;
  priv->icon_width = 0;
  priv->icon_height = 0;
  priv->tooltip = awn_tooltip_new_for_widget (GTK_WIDGET (icon));

  priv->effects = awn_effects_new_for_widget (GTK_WIDGET (icon));
  gtk_widget_add_events (GTK_WIDGET (icon), GDK_ALL_EVENTS_MASK);

  g_signal_connect (icon, "button-press-event",
                    G_CALLBACK (awn_icon_pressed), NULL);
  g_signal_connect (icon, "button-release-event",
                    G_CALLBACK (awn_icon_released), NULL);

  awn_utils_ensure_transparent_bg (GTK_WIDGET (icon));
  g_signal_connect (icon, "realize",
                    G_CALLBACK (awn_icon_update_effects), NULL);
  g_signal_connect (icon, "composited-changed",
                    G_CALLBACK (awn_icon_update_effects), NULL);
}

/**
 * awn_icon_new:
 * 
 * Creates new #AwnIcon.
 *
 * Returns: newly created #AwnIcon.
 */
GtkWidget *
awn_icon_new (void)

{
  GtkWidget *icon = NULL;

  icon = g_object_new (AWN_TYPE_ICON, 
                       NULL);

  return icon;
}

static void
awn_icon_update_tooltip_pos(AwnIcon *icon)
{
  AwnIconPrivate *priv;

  g_return_if_fail (AWN_IS_ICON (icon));
  priv = icon->priv;

  /* we could set tooltip_offset = priv->offset, and use 
   * different offset in AwnTooltip
   * (do we want different icon bar offset and tooltip offset?)
   */
  gint tooltip_offset = priv->offset;

  switch (priv->position)
  {
    case GTK_POS_TOP:
    case GTK_POS_BOTTOM:
      tooltip_offset += priv->icon_height;
      break;
    default:
      tooltip_offset += priv->icon_width;
      break;
  }

  awn_tooltip_set_position_hint(AWN_TOOLTIP(priv->tooltip),
                                priv->position, tooltip_offset);
}

/**
 * awn_icon_get_offset:
 * @icon: an #AwnIcon.
 *
 * Returns current offset set for the icon.
 */
gint
awn_icon_get_offset (AwnIcon *icon)
{
  g_return_val_if_fail (AWN_IS_ICON (icon), 0);

  return icon->priv->offset;
}

/**
 * awn_icon_set_offset:
 * @icon: an #AwnIcon.
 * @offset: new offset for the icon.
 *
 * Sets offset of the icon.
 */
void 
awn_icon_set_offset (AwnIcon        *icon,
                     gint            offset)
{
  AwnIconPrivate *priv;

  g_return_if_fail (AWN_IS_ICON (icon));
  priv = icon->priv;

  if (priv->offset != offset)
  {
    priv->offset = offset;

    g_object_set (priv->effects, "icon-offset", offset, NULL);

    gtk_widget_queue_resize (GTK_WIDGET(icon));

    awn_icon_update_tooltip_pos(icon);
  }
}

/**
 * awn_icon_get_pos_type:
 * @icon: an #AwnIcon.
 *
 * Returns current position type set for the icon.
 */
GtkPositionType
awn_icon_get_pos_type (AwnIcon *icon)
{
  g_return_val_if_fail (AWN_IS_ICON (icon), GTK_POS_BOTTOM);

  return icon->priv->position;
}

/**
 * awn_icon_set_pos_type:
 * @icon: an #AwnIcon.
 * @position: position of the icon.
 *
 * Sets position of the icon.
 */
void 
awn_icon_set_pos_type (AwnIcon        *icon,
                       GtkPositionType  position)
{
  AwnIconPrivate *priv;

  g_return_if_fail (AWN_IS_ICON (icon));
  priv = icon->priv;

  priv->position = position;

  g_object_set (priv->effects, "position", position, NULL);

  gtk_widget_queue_resize (GTK_WIDGET(icon));

  awn_icon_update_tooltip_pos(icon);
}

static void
update_widget_to_size (AwnIcon *icon, gint width, gint height)
{
  AwnIconPrivate  *priv = icon->priv;
  gint old_size, old_width, old_height;

  old_size = priv->size;
  old_width = priv->icon_width;
  old_height = priv->icon_height;

  priv->icon_width = width;
  priv->icon_height = height;

  switch (priv->position)
  {
    case GTK_POS_TOP:
    case GTK_POS_BOTTOM:
      priv->size = priv->icon_width;
      break;

    default:
      priv->size = priv->icon_height;
  }

  if (old_size != priv->size)
  {
    g_signal_emit (icon, _icon_signals[SIZE_CHANGED], 0);
  }

  if (old_width != priv->icon_width || old_height != priv->icon_height)
  {
    gtk_widget_queue_resize_no_redraw (GTK_WIDGET(icon));

    awn_effects_set_icon_size (priv->effects,
                               priv->icon_width,
                               priv->icon_height,
                               FALSE);

    awn_icon_update_tooltip_pos(icon);
  }
}

static void
update_widget_size (AwnIcon *icon)
{
  AwnIconPrivate  *priv = icon->priv;
  gint width = 0, height = 0;

  if (priv->icon_srfc)
  {
    switch (cairo_surface_get_type(priv->icon_srfc))
    {
      case CAIRO_SURFACE_TYPE_XLIB:
        width = cairo_xlib_surface_get_width (priv->icon_srfc);
        height = cairo_xlib_surface_get_height (priv->icon_srfc);
        break;
      case CAIRO_SURFACE_TYPE_IMAGE:
        width = cairo_image_surface_get_width (priv->icon_srfc);
        height = cairo_image_surface_get_height (priv->icon_srfc);
        break;
      default:
        g_warning("Invalid surface type: Surfaces must be either xlib or image");
        return;
    }
  }

  update_widget_to_size (icon, width, height);
}

/**
 * awn_icon_set_effect:
 * @icon: an #AwnIcon.
 * @effect: #AwnEffect to start looping.
 *
 * Sets effect on the icon. Note that the effect will loop until
 * awn_effects_stop() is called.
 */
void
awn_icon_set_effect (AwnIcon *icon, AwnEffect effect)
{ 
  g_return_if_fail (AWN_IS_ICON (icon));

  awn_effects_start (icon->priv->effects, effect);
}

static AwnEffects*
awn_icon_get_effects (AwnOverlayable *icon)
{
  g_return_val_if_fail (AWN_IS_ICON (icon), NULL);

  return AWN_ICON_GET_PRIVATE (icon)->effects;
}

/**
 * awn_icon_get_tooltip:
 * @icon: an #AwnIcon.
 *
 * Gets the #AwnTooltip associated with this icon.
 *
 * Returns: tooltip widget.
 */
AwnTooltip *
awn_icon_get_tooltip (AwnIcon *icon)
{
  g_return_val_if_fail (AWN_IS_ICON (icon), NULL);

  return AWN_TOOLTIP(icon->priv->tooltip);
}

/*
 * ICON SETTING FUNCTIONS 
 */
static void
free_existing_icon (AwnIcon *icon)
{
  AwnIconPrivate *priv = icon->priv;

  if (priv->icon_srfc == NULL) return;

  cairo_surface_destroy (priv->icon_srfc);
  priv->icon_srfc = NULL;
}

/**
 * awn_icon_set_from_pixbuf:
 * @icon: an #AwnIcon.
 * @pixbuf: a #GdkPixbuf.
 *
 * Sets the icon from the given pixbuf. Note that a copy of the pixbuf is made.
 */
void
awn_icon_set_from_pixbuf (AwnIcon *icon, GdkPixbuf *pixbuf)
{
  AwnIconPrivate  *priv;
  cairo_t         *temp_cr;
  gint             width, height;

  g_return_if_fail (AWN_IS_ICON (icon));
  g_return_if_fail (GDK_IS_PIXBUF (pixbuf));
  priv = icon->priv;

  free_existing_icon (icon);

  /* Render the pixbuf into a image surface */
  width = gdk_pixbuf_get_width (pixbuf);
  height = gdk_pixbuf_get_height (pixbuf);

  priv->icon_srfc = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,
                                               width, height);
  temp_cr = cairo_create (priv->icon_srfc);

  gdk_cairo_set_source_pixbuf (temp_cr, pixbuf, 0, 0);
  cairo_paint (temp_cr);

  cairo_destroy (temp_cr);

  /* Queue a redraw */
  update_widget_size (icon);
  gtk_widget_queue_draw (GTK_WIDGET(icon));
}

/**
 * awn_icon_set_from_surface:
 * @icon: an #AwnIcon.
 * @surface: a #cairo_surface_t.
 *
 * Sets the icon from the given cairo surface. Note that the surface is only
 * referenced, so any later changes made to it will change the icon as well
 * (after a call to gtk_widget_queue_draw()).
 */
void 
awn_icon_set_from_surface (AwnIcon *icon, cairo_surface_t *surface)
{
  AwnIconPrivate       *priv;
  
  g_return_if_fail (AWN_IS_ICON (icon));
  g_return_if_fail (surface);
  priv = icon->priv;

  switch (cairo_surface_get_type(surface))
  {
    case CAIRO_SURFACE_TYPE_XLIB:
    case CAIRO_SURFACE_TYPE_IMAGE:
      free_existing_icon (icon);
      priv->icon_srfc = cairo_surface_reference(surface);
      break;
    default:
      g_warning("Invalid surface type: Surfaces must be either xlib or image");
      return;
  }

  update_widget_size (icon);
  gtk_widget_queue_draw (GTK_WIDGET (icon));
}

/**
 * awn_icon_set_from_context:
 * @icon: an #AwnIcon.
 * @ctx: a #cairo_t.
 *
 * Extracts the icon from the cairo surface associated with given cairo
 * context. Note that the surface is only referenced, so any later changes
 * made to it will change the icon as well
 * (after a call to gtk_widget_queue_draw()).
 */
void 
awn_icon_set_from_context (AwnIcon *icon, cairo_t *ctx)
{
  g_return_if_fail (AWN_IS_ICON (icon));
  g_return_if_fail (ctx);

  awn_icon_set_from_surface(icon, cairo_get_target(ctx));
}

/**
 * awn_icon_set_custom_paint:
 * @icon: an #AwnIcon.
 * @width: new width of the icon.
 * @height: new height of the icon.
 *
 * Prepares the icon for custom painting (by overriding
 * #GtkWidget::expose-event). Sets proper size requisition, tooltip position,
 * parameters for #AwnEffects and may emit size changed signal.
 *
 * <note>
 *  If there's already an icon set, it is not freed, so if you later disconnect
 *  from the #GtkWidget::expose-event, a second call to
 *  awn_icon_set_custom_paint() with the original dimensions of the icon will
 *  restore the icon.
 * </note>
 */
void
awn_icon_set_custom_paint (AwnIcon *icon, gint width, gint height)
{
  g_return_if_fail (AWN_IS_ICON (icon));

  /*
   * Don't free the icon, user may want to reuse it later
   *  (if doing temporary connect to expose-event)
   * free_existing_icon (icon);
   */

  /* this will set proper size requisition, tooltip position,
   * params for effects and may emit size changed signal
   * the only thing user needs is overriding expose-event
   */
  update_widget_to_size (icon, width, height);
  gtk_widget_queue_draw (GTK_WIDGET (icon));
}

/*
 * The tooltip 
 */
/**
 * awn_icon_set_tooltip_text:
 * @icon: an #AwnIcon.
 * @text: tooltip message.
 *
 * Sets tooltip message.
 */
void   
awn_icon_set_tooltip_text (AwnIcon     *icon,
                           const gchar *text)
{
  g_return_if_fail (AWN_IS_ICON (icon));

  awn_tooltip_set_text (AWN_TOOLTIP (icon->priv->tooltip), text);
}

/**
 * awn_icon_get_tooltip_text:
 * @icon: an #AwnIcon.
 *
 * Gets the message currently set for the associated #AwnTooltip. The caller
 * is responsible for freeing the string.
 *
 * Returns: currently used message by the associated #AwnTooltip.
 */
gchar * 
awn_icon_get_tooltip_text (AwnIcon *icon)
{
  g_return_val_if_fail (AWN_IS_ICON (icon), NULL);

  return awn_tooltip_get_text (AWN_TOOLTIP (icon->priv->tooltip));
}

/**
 * awn_icon_get_input_mask:
 * @icon: an #AwnIcon.
 *
 * Gets the standard input shape mask which should be used for the widget. The
 * called is responsible to destroying the newly created #GdkRegion.
 *
 * Returns: #GdkRegion with input shape mask of the widget (already offset by
 * the widget allocation).
 */
GdkRegion*
awn_icon_get_input_mask (AwnIcon *icon)
{
  AwnIconPrivate *priv;
  GtkAllocation alloc;

  g_return_val_if_fail (AWN_IS_ICON (icon), NULL);

  priv = icon->priv;

  gtk_widget_get_allocation (GTK_WIDGET (icon), &alloc);
  switch (priv->position)
  {
    case GTK_POS_BOTTOM:
      alloc.y = alloc.height - priv->icon_height - priv->offset;
    case GTK_POS_TOP:
      alloc.height = priv->icon_height + priv->offset;
      break;
    case GTK_POS_RIGHT:
      alloc.x = alloc.width - priv->icon_width - priv->offset;
    case GTK_POS_LEFT:
      alloc.width = priv->icon_width + priv->offset;
      break;
    default:
      g_assert_not_reached ();
      break;
  }

  return gdk_region_rectangle (&alloc);
}

/**
 * awn_icon_popup_gtk_menu:
 * @icon: an #AwnIcon.
 * @menu: a #GtkMenu to popup.
 * @button: the mouse button which was pressed to initiate the event.
 * @activate_time: the time at which the activation event occurred.
 *
 * Displays a menu relative to the icon's position.
 */
void awn_icon_popup_gtk_menu (AwnIcon   *icon,
                              GtkWidget *menu,
                              guint      button,
                              guint32    activate_time)
{
  g_return_if_fail (menu);
  g_return_if_fail (GTK_IS_MENU (menu));

  gtk_menu_popup (
            GTK_MENU (menu), NULL, NULL, 
            ((GtkMenuPositionFunc) awn_utils_menu_set_position_widget_relative),
            icon, button, activate_time);
}

/*
 * ICON EMBLEMS
 */

/**
 * awn_icon_set_is_active:
 * @icon: an #AwnIcon.
 * @is_active: value.
 *
 * Sets whether the icon is active (if it is paints a rectangle around the icon
 * by default).
 */
void
awn_icon_set_is_active (AwnIcon *icon, gboolean is_active)
{
  g_return_if_fail (AWN_IS_ICON (icon));

  g_object_set (icon->priv->effects, "active", is_active, NULL);
}

/**
 * awn_icon_get_is_active:
 * @icon: an #AwnIcon.
 *
 * Gets whether the icon is active.
 *
 * Returns: TRUE if the icon is active, FALSE otherwise.
 */
gboolean      
awn_icon_get_is_active (AwnIcon *icon)
{
  gboolean result;
  
  g_return_val_if_fail (AWN_IS_ICON (icon), FALSE);

  g_object_get (icon->priv->effects, "active", &result, NULL);

  return result;
}

/**
 * awn_icon_set_indicator_count:
 * @icon: an #AwnIcon.
 * @count: indicator count.
 *
 * Paints an indicator (or multiple) on the border of icon.
 */
void    
awn_icon_set_indicator_count (AwnIcon *icon, gint count)
{
  g_return_if_fail (AWN_IS_ICON (icon));

  g_object_set (icon->priv->effects, "arrows-count", count, NULL);
}

/**
 * awn_icon_get_indicator_count:
 * @icon: an #AwnIcon.
 *
 * Gets number of indicators set for this icon.
 *
 * Returns: number of indicators.
 */
gint
awn_icon_get_indicator_count (AwnIcon *icon)
{
  gint result;
  
  g_return_val_if_fail (AWN_IS_ICON (icon), 0);

  g_object_get (icon->priv->effects, "arrows-count", &result, NULL);

  return result;
}

gboolean
awn_icon_get_hover_effects (AwnIcon *icon)
{
  g_return_val_if_fail (AWN_IS_ICON (icon), FALSE);

  return icon->priv->hover_effects_enable;
}

void
awn_icon_set_hover_effects (AwnIcon *icon, gboolean enable)
{
  AwnIconPrivate *priv;
  g_return_if_fail (AWN_IS_ICON (icon));

  priv = icon->priv;
  priv->hover_effects_enable = enable;

  if (!enable)
  {
    awn_effects_stop (priv->effects, AWN_EFFECT_HOVER);
  }
}

void
awn_icon_clicked (AwnIcon *icon)
{
  g_return_if_fail (AWN_IS_ICON (icon));

  g_signal_emit (icon, _icon_signals[CLICKED], 0);
}

void
awn_icon_middle_clicked (AwnIcon *icon)
{
  g_return_if_fail (AWN_IS_ICON (icon));

  g_signal_emit (icon, _icon_signals[MIDDLE_CLICKED], 0);
}

