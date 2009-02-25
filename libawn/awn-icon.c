/*
 * Copyright (C) 2008 Neil Jagdish Patel
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Library General Public License version 
 * 2 or later as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by Neil Jagdish Patel <njpatel@gmail.com>
 *
 */
#include <gdk/gdkx.h>
#include <cairo/cairo-xlib.h>

#include "awn-icon.h"
#include "awn-utils.h"

#define APPLY_SIZE_MULTIPLIER(x)	(x)*6/5

G_DEFINE_TYPE (AwnIcon, awn_icon, GTK_TYPE_DRAWING_AREA);

#define AWN_ICON_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj),\
  AWN_TYPE_ICON, \
  AwnIconPrivate))

struct _AwnIconPrivate
{
  AwnEffects   *effects;
  GtkWidget    *tooltip;
  
  AwnOrientation orient;
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

  PROP_ICON_WIDTH,
  PROP_ICON_HEIGHT
};

enum
{
  SIZE_CHANGED,

  LAST_SIGNAL
};
static guint32 _icon_signals[LAST_SIGNAL] = { 0 };

/* GObject stuff */
static gboolean
awn_icon_enter_notify_event (GtkWidget *widget, GdkEventCrossing *event)
{
  AwnIconPrivate *priv = AWN_ICON (widget)->priv;

  awn_effects_start (priv->effects, AWN_EFFECT_HOVER);

  return FALSE;
}

static gboolean
awn_icon_leave_notify_event (GtkWidget *widget, GdkEventCrossing *event)
{
  AwnIconPrivate *priv = AWN_ICON (widget)->priv;

  awn_effects_stop (priv->effects, AWN_EFFECT_HOVER);

  return FALSE;
}

static void
awn_icon_make_transparent (GtkWidget *widget, gpointer data)
{
  AwnIconPrivate *priv = AWN_ICON (widget)->priv;

  if (!GTK_WIDGET_REALIZED (widget)) return;

  /*
   * This is how we make sure that widget has transparent background
   * all the time.
   */
  if (gtk_widget_is_composited(widget)) /* FIXME: is is_composited correct here? */
  {
    awn_utils_make_transparent_bg (widget);
    /* optimize the render speed */
    g_object_set(priv->effects,
                 "no-clear", TRUE,
                 "indirect-paint", FALSE, NULL);
  }
  else
  {
    g_object_set(priv->effects,
                 "effects", 0,
                 "no-clear", TRUE,
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
  cr = awn_effects_cairo_create_clipped (priv->effects, event->region);

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

  switch (priv->orient)
  {
    case AWN_ORIENTATION_TOP:
    case AWN_ORIENTATION_BOTTOM:
      req->width = APPLY_SIZE_MULTIPLIER(priv->icon_width);
      req->height = priv->icon_height + priv->effects->icon_offset;
      break;
      
    default:
      req->width = priv->icon_width + priv->effects->icon_offset;
      req->height = APPLY_SIZE_MULTIPLIER(priv->icon_height);
      break;
  }
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
    case PROP_ICON_WIDTH:
      g_value_set_int (value, priv->icon_width);
      break;
    case PROP_ICON_HEIGHT:
      g_value_set_int (value, priv->icon_height);
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
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

static void
awn_icon_dispose (GObject *object)
{
  AwnIconPrivate *priv;

  g_return_if_fail (AWN_IS_ICON (object));
  priv = AWN_ICON (object)->priv;

  if (priv->effects)
    g_object_unref (priv->effects);
  priv->effects = NULL;

  if (priv->tooltip)
    gtk_widget_destroy (priv->tooltip);
  priv->tooltip = NULL;

  if (priv->icon_srfc)
    cairo_surface_destroy (priv->icon_srfc);
  priv->icon_srfc = NULL;

  G_OBJECT_CLASS (awn_icon_parent_class)->dispose (object);
}

static void
awn_icon_class_init (AwnIconClass *klass)
{
  GObjectClass   *obj_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *wid_class = GTK_WIDGET_CLASS (klass);

  obj_class->get_property = awn_icon_get_property;
  obj_class->set_property = awn_icon_set_property;
  obj_class->dispose      = awn_icon_dispose;

  wid_class->size_request       = awn_icon_size_request;
  wid_class->expose_event       = awn_icon_expose_event;
  wid_class->enter_notify_event = awn_icon_enter_notify_event;
  wid_class->leave_notify_event = awn_icon_leave_notify_event;

  /* Class properties */
  g_object_class_install_property (obj_class,
    PROP_ICON_WIDTH,
    g_param_spec_int ("icon-width",
                      "Icon width",
                      "Current icon width",
                      0, G_MAXINT, 0,
                      G_PARAM_READWRITE));

  g_object_class_install_property (obj_class,
    PROP_ICON_HEIGHT,
    g_param_spec_int ("icon-height",
                      "Icon height",
                      "Current icon height",
                      0, G_MAXINT, 0,
                      G_PARAM_READWRITE));

  /* Signals */
  _icon_signals[SIZE_CHANGED] =
    g_signal_new ("size-changed",
      G_OBJECT_CLASS_TYPE (obj_class),
      G_SIGNAL_RUN_FIRST,
      G_STRUCT_OFFSET (AwnIconClass, size_changed),
      NULL, NULL,
      g_cclosure_marshal_VOID__VOID, 
      G_TYPE_NONE, 0);

  g_type_class_add_private (obj_class, sizeof (AwnIconPrivate));
}

static void
awn_icon_init (AwnIcon *icon)
{
  AwnIconPrivate *priv;

  priv = icon->priv = AWN_ICON_GET_PRIVATE (icon);

  priv->icon_srfc = NULL;
  priv->orient = AWN_ORIENTATION_BOTTOM;
  priv->offset = 0;
  priv->size = 50;
  priv->icon_width = 0;
  priv->icon_height = 0;
  priv->tooltip = awn_tooltip_new_for_widget (GTK_WIDGET (icon));

  priv->effects = awn_effects_new_for_widget (GTK_WIDGET (icon));

  gtk_widget_add_events (GTK_WIDGET (icon), GDK_ALL_EVENTS_MASK);

  g_signal_connect (icon, "realize",
                    G_CALLBACK(awn_icon_make_transparent), NULL);
  g_signal_connect (icon, "style-set",
                    G_CALLBACK(awn_icon_make_transparent), NULL);
}

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
  gint tooltip_offset = 0;

  switch (priv->orient) {
    case AWN_ORIENTATION_TOP:
    case AWN_ORIENTATION_BOTTOM:
      tooltip_offset += priv->icon_height;
      break;
    default:
      tooltip_offset += priv->icon_width;
      break;
  }

  awn_tooltip_set_position_hint(AWN_TOOLTIP(priv->tooltip),
                                priv->orient, tooltip_offset);
}

void 
awn_icon_set_offset (AwnIcon        *icon,
                     gint            offset)
{
  AwnIconPrivate *priv;

  g_return_if_fail (AWN_IS_ICON (icon));
  priv = icon->priv;

  priv->offset = offset;

  g_object_set (priv->effects, "icon-offset", offset, NULL);

  gtk_widget_queue_resize (GTK_WIDGET(icon));

  awn_icon_update_tooltip_pos(icon);
}

void 
awn_icon_set_orientation (AwnIcon        *icon,
                          AwnOrientation  orient)
{
  AwnIconPrivate *priv;

  g_return_if_fail (AWN_IS_ICON (icon));
  priv = icon->priv;

  priv->orient = orient;

  g_object_set (priv->effects, "orientation", orient, NULL);

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

  switch (priv->orient)
  {
    case AWN_ORIENTATION_TOP:
    case AWN_ORIENTATION_BOTTOM:
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

void  
awn_icon_set_effect (AwnIcon *icon, AwnEffect effect)
{ 
  g_return_if_fail (AWN_IS_ICON (icon));

  awn_effects_start (icon->priv->effects, effect);
}

AwnEffects * 
awn_icon_get_effects (AwnIcon *icon)
{
  g_return_val_if_fail (AWN_IS_ICON (icon), NULL);

  return icon->priv->effects;
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

void 
awn_icon_set_from_context (AwnIcon *icon, cairo_t *ctx)
{
  g_return_if_fail (AWN_IS_ICON (icon));
  g_return_if_fail (ctx);

  awn_icon_set_from_surface(icon, cairo_get_target(ctx));
}

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
void   
awn_icon_set_tooltip_text (AwnIcon     *icon,
                           const gchar *text)
{
  g_return_if_fail (AWN_IS_ICON (icon));

  awn_tooltip_set_text (AWN_TOOLTIP (icon->priv->tooltip), text);
}

gchar * 
awn_icon_get_tooltip_text (AwnIcon *icon)
{
  g_return_val_if_fail (AWN_IS_ICON (icon), NULL);

  return awn_tooltip_get_text (AWN_TOOLTIP (icon->priv->tooltip));
}

/*
 * ICON EMBLEMS
 */

void 
awn_icon_set_message (AwnIcon *icon, const gchar  *message)
{
  g_return_if_fail (AWN_IS_ICON (icon));

  g_object_set (icon->priv->effects, "label", message, NULL);
}

gchar * 
awn_icon_get_message (AwnIcon *icon)
{
  gchar *result = NULL;
  
  g_return_val_if_fail (AWN_IS_ICON (icon), NULL);

  g_object_get (icon->priv->effects, "label", &result, NULL);
  /* caller gets a copy, so he's responsible for freeing it */
  return result;
}

void
awn_icon_set_progress (AwnIcon *icon, gfloat progress)
{
  g_return_if_fail (AWN_IS_ICON (icon));

  g_object_set(icon->priv->effects, "progress", progress, NULL);
}

gfloat        
awn_icon_get_progress (AwnIcon *icon)
{
  gfloat result;
  
  g_return_val_if_fail (AWN_IS_ICON (icon), 0.0);

  g_object_get (icon->priv->effects, "progress", &result, NULL);

  return result;
}

void
awn_icon_set_is_active (AwnIcon *icon, gboolean is_active)
{
  g_return_if_fail (AWN_IS_ICON (icon));

  g_object_set (icon->priv->effects, "active", is_active, NULL);
}

gboolean      
awn_icon_get_is_active (AwnIcon *icon)
{
  gboolean result;
  
  g_return_val_if_fail (AWN_IS_ICON (icon), FALSE);

  g_object_get (icon->priv->effects, "active", &result, NULL);

  return result;
}

void    
awn_icon_set_is_running (AwnIcon     *icon,
                         gboolean     is_running)
{
  g_return_if_fail (AWN_IS_ICON (icon));

  g_object_set (icon->priv->effects, "show-arrow", is_running, NULL);
}

gboolean
awn_icon_get_is_running (AwnIcon     *icon)
{
  gboolean result;
  
  g_return_val_if_fail (AWN_IS_ICON (icon), FALSE);

  g_object_get (icon->priv->effects, "show-arrow", &result, NULL);

  return result;
}
