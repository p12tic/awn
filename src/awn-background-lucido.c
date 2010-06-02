/*
 *  Copyright (C) 2009 Michal Hruby <michal.mhr@gmail.com>
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
 *  Author : Alberto Aldegheri <albyrock87+dev@gmail.com>
 *  Thanks to: Matt <sharkbaitbobby@gmail.com>
 *             for the code section to analyze separators
 *
 */

#include "config.h"

#include <gdk/gdk.h>
#include <libawn/awn-cairo-utils.h>
#include <math.h>

#include "awn-applet-manager.h"
#include "awn-background-lucido.h"
#include "awn-separator.h"

G_DEFINE_TYPE (AwnBackgroundLucido, awn_background_lucido, AWN_TYPE_BACKGROUND)

#define AWN_BACKGROUND_LUCIDO_GET_PRIVATE(obj) ( \
    G_TYPE_INSTANCE_GET_PRIVATE (obj, AWN_TYPE_BACKGROUND_LUCIDO, \
                                 AwnBackgroundLucidoPrivate))

struct _AwnBackgroundLucidoPrivate
{
  /* stores the last position of 'special applets' */
  GArray *pos;
  gfloat expw;
  gint expn;
  gint expp;
  gfloat last_pad;
  guint tid;
  gboolean needs_animation;
};

#define ANIM_TIMEOUT 50

#define TRANSFORM_RADIUS(x) sqrt(x/50.)*50.

#define IS_SPECIAL(x) AWN_IS_SEPARATOR(x)

/* redraw timeout */
static gboolean
awn_background_lucido_redraw (AwnBackgroundLucido *bg)
{
  g_return_val_if_fail (AWN_IS_BACKGROUND_LUCIDO (bg), FALSE);

  if (AWN_BACKGROUND_LUCIDO_GET_PRIVATE (bg)->needs_animation)
  {
    awn_background_invalidate (AWN_BACKGROUND (bg));
    gtk_widget_queue_draw (GTK_WIDGET (AWN_BACKGROUND (bg)->panel));
    return TRUE;
  }
  else
  {
    AWN_BACKGROUND_LUCIDO_GET_PRIVATE (bg)->tid = 0;
    return FALSE;
  }
}

static void awn_background_lucido_draw (AwnBackground  *bg,
                                        cairo_t        *cr,
                                        GtkPositionType  position,
                                        GdkRectangle   *area);

static void awn_background_lucido_get_shape_mask (AwnBackground  *bg,
                                                  cairo_t        *cr,
                                                  GtkPositionType  position,
                                                  GdkRectangle   *area);

static void awn_background_lucido_padding_request (AwnBackground *bg,
                                                   GtkPositionType position,
                                                   guint *padding_top,
                                                   guint *padding_bottom,
                                                   guint *padding_left,
                                                   guint *padding_right);
                                                   
static gboolean
awn_background_lucido_get_needs_redraw (AwnBackground *bg,
                                        GtkPositionType position,
                                        GdkRectangle *area);

static void 
_set_special_widget_width_and_transparent (AwnBackground *bg,
                                           gint          width,
                                           gboolean      transp,
                                           gboolean      dispose);
                                        
static void 
awn_background_lucido_corner_radius_changed (AwnBackground *bg)
{
  gboolean expand = FALSE;
  g_object_get (bg->panel, "expand", &expand, NULL);
  
  _set_special_widget_width_and_transparent 
                  (bg, TRANSFORM_RADIUS (bg->corner_radius), TRUE, FALSE);
  
  if (!expand)
  {    
    awn_background_emit_padding_changed (bg);
  }
}

static void
awn_background_lucido_expand_changed (AwnBackground *bg)
{
  awn_background_emit_padding_changed (bg);
}

static void
awn_background_lucido_align_changed (AwnBackground *bg)
{
  awn_background_emit_padding_changed (bg);
}

static void
awn_background_lucido_constructed (GObject *object)
{
  G_OBJECT_CLASS (awn_background_lucido_parent_class)->constructed (object);
  
  AwnBackground *bg = AWN_BACKGROUND (object);
  gpointer monitor = NULL;
  
  g_signal_connect_swapped (bg, "notify::corner-radius",
                            G_CALLBACK (awn_background_lucido_corner_radius_changed),
                            object);

  g_return_if_fail (bg->panel);
  
  g_signal_connect_swapped (bg->panel, "notify::expand",
                            G_CALLBACK (awn_background_lucido_expand_changed),
                            object);

  g_object_get (bg->panel, "monitor", &monitor, NULL);

  g_return_if_fail (monitor);

  g_signal_connect_swapped (monitor, "notify::monitor-align",
                            G_CALLBACK (awn_background_lucido_align_changed),
                            object);
}

static gint
_get_applet_manager_size (AwnBackground* bg, GtkPositionType position)
{
  AwnAppletManager *manager = NULL;
  g_object_get (bg->panel, "applet-manager", &manager, NULL);
  
  switch (position)
  {
    case GTK_POS_BOTTOM:
    case GTK_POS_TOP:
      return GTK_WIDGET (manager)->allocation.width;
      break;
    default:
      return GTK_WIDGET (manager)->allocation.height;
      break;
  }
}

static GList*
_get_applet_widgets (AwnBackground* bg)
{
  AwnAppletManager *manager = NULL;
  g_object_get (bg->panel, "applet-manager", &manager, NULL);

  return gtk_container_get_children (GTK_CONTAINER (manager));
}

static void 
_init_positions (AwnBackground*  bg,
                 GtkPositionType position,
                 gfloat          w,
                 gboolean        expanded,
                 gfloat          align)
{
  AwnBackgroundLucidoPrivate *priv;
  priv = AWN_BACKGROUND_LUCIDO_GET_PRIVATE (AWN_BACKGROUND_LUCIDO (bg));
  
  if (priv->pos != NULL)
    g_array_free (priv->pos, TRUE);

  priv->pos = g_array_new (FALSE, TRUE, sizeof (gfloat));

  GList* widgets = _get_applet_widgets (bg);
  GList* i = widgets;
  GtkWidget* widget = NULL;
    
  gfloat target_pad_left = 0.;
   
  if (expanded)
    target_pad_left = (w - _get_applet_manager_size (bg, position)) * align;
  else if (align > 0.)
    target_pad_left = TRANSFORM_RADIUS (bg->corner_radius);
  
  //gint pos = 0;
  for (; i; i = i->next)
  {
    widget = GTK_WIDGET (i->data);
    if (IS_SPECIAL (widget))
      switch (position)
      {
        case GTK_POS_BOTTOM:
        case GTK_POS_TOP:
          w = target_pad_left + widget->allocation.x;
          break;
        default:
          w = target_pad_left + widget->allocation.y;
          break;
      }
      //pos = floor (w);
      g_array_append_val (priv->pos, w);
  }

  g_list_free (widgets);
}

static void 
_destroy_positions (AwnBackground *bg)
{
  AwnBackgroundLucidoPrivate *priv;
  priv = AWN_BACKGROUND_LUCIDO_GET_PRIVATE (AWN_BACKGROUND_LUCIDO (bg));
  
  if (priv->pos != NULL)
    g_array_free (priv->pos, TRUE);
  priv->pos = NULL;
}

static void
awn_background_lucido_dispose (GObject *object)
{
  _set_special_widget_width_and_transparent 
                          (AWN_BACKGROUND (object), 10, FALSE, TRUE);
  gpointer monitor = NULL;
  if (AWN_BACKGROUND (object)->panel)
  {
    g_object_get (AWN_BACKGROUND (object)->panel, "monitor", &monitor, NULL);

    if (monitor)
    {
      g_signal_handlers_disconnect_by_func (monitor, 
          G_CALLBACK (awn_background_lucido_align_changed), object);
    }
  }

  g_signal_handlers_disconnect_by_func (AWN_BACKGROUND (object)->panel, 
        G_CALLBACK (awn_background_lucido_expand_changed), object);

  g_signal_handlers_disconnect_by_func (AWN_BACKGROUND (object), 
        G_CALLBACK (awn_background_lucido_corner_radius_changed), object);

  

  G_OBJECT_CLASS (awn_background_lucido_parent_class)->dispose (object);
}

static void
awn_background_lucido_class_init (AwnBackgroundLucidoClass *klass)
{
  AwnBackgroundClass *bg_class = AWN_BACKGROUND_CLASS (klass);

  GObjectClass *obj_class = G_OBJECT_CLASS (klass);
  obj_class->constructed  = awn_background_lucido_constructed;
  obj_class->dispose = awn_background_lucido_dispose;

  bg_class->draw = awn_background_lucido_draw;
  bg_class->padding_request = awn_background_lucido_padding_request;
  bg_class->get_shape_mask = awn_background_lucido_get_shape_mask;
  bg_class->get_input_shape_mask = awn_background_lucido_get_shape_mask;
  bg_class->get_needs_redraw = awn_background_lucido_get_needs_redraw;
  
  g_type_class_add_private (obj_class, sizeof (AwnBackgroundLucidoPrivate));
}

static void
awn_background_lucido_init (AwnBackgroundLucido *bg)
{
  AwnBackgroundLucidoPrivate *priv;
  priv = AWN_BACKGROUND_LUCIDO_GET_PRIVATE (bg);
  priv->pos = NULL;
  priv->last_pad = 0.;
  priv->needs_animation = TRUE;
  priv->tid = g_timeout_add (ANIM_TIMEOUT, (GSourceFunc)awn_background_lucido_redraw, AWN_BACKGROUND (bg));
}

AwnBackground *
awn_background_lucido_new (DesktopAgnosticConfigClient *client,
                           AwnPanel *panel)
{
  AwnBackground *bg;

  bg = g_object_new (AWN_TYPE_BACKGROUND_LUCIDO,
                     "client", client,
                     "panel", panel,
                     NULL);
  return bg;
}

/*
 * Drawing functions
 */
static void 
_line_from_to ( cairo_t *cr,
                gfloat *xs,
                gfloat *ys,
                gfloat xf,
                gfloat yf)
{
  if ( *xs==xf || *ys==yf ) /* Vertical/Horizontal line */
  {
    cairo_line_to (cr, xf, yf);
  }
  else
  { /* Oblique */
    gfloat xm = ( *xs + xf ) / 2.0;
    cairo_curve_to (cr, xm, *ys, xm, yf, xf, yf);
  }
  *xs = xf;
  *ys = yf;
}

/**
 * _create_path_lucido:
 * @bg: The background pointer
 * @position: The position of the bar
 * @cairo_t: The cairo context
 * @y: The top left coordinate of the "bar rect" - default = 0
 * @w: The width of the bar
 * @h: The height of the bar
 * @d: The width of each curve in the path
 * @dc: The width of the external curves in non-expanded&auto mode
 * @internal: If Zero, creates the path for the stripe
 * @expanded: If Zero, the bar is not expanded
 * @align: the monitor align
 *
 * This function creates paths on which the bar will be drawn.
 * In atuo-stripe, it searchs for separators applet, each separator
 * equals to one curve.
 * If the first widget is an separator, start from bottom-left,
 * otherwise start from top-left
 */

static void
_create_path_lucido ( AwnBackground*  bg,
                      GtkPositionType position,
                      cairo_t*        cr,
                      gfloat          x,
                      gfloat          y,
                      gfloat          w,
                      gfloat          h,
                      gfloat          d,
                      gfloat          dc,
                      gboolean        internal,
                      gboolean        expanded,
                      gfloat          align,
                      gboolean        update_state)
{
  AwnBackgroundLucidoPrivate *priv;
  priv = AWN_BACKGROUND_LUCIDO_GET_PRIVATE (AWN_BACKGROUND_LUCIDO (bg));

  if (priv->pos == NULL)
    _init_positions (bg, position, w, expanded, align);
  
  cairo_new_path (cr);

  gfloat targetx = x;
  gfloat lx = x;
  gfloat ly = y;
  gfloat y3 = y + h;
  gfloat y2 = y3 - 5.;
  
  /* padding left */
  gfloat target_pad_left = 0.;
  
  /* padding difference than before */
  gfloat pad_diff = 0.;
  /* special applet index */
  gint j = 0;
  
  /* Get list of widgets */
  GList *widgets = _get_applet_widgets (bg);
  GList *i = widgets;
  GtkWidget *widget = NULL;
  gboolean first_widget_is_special = FALSE;
  if (i && IS_SPECIAL (i->data))
    first_widget_is_special = TRUE;
  
  /* We start from left */
  ly = y3;
  cairo_move_to (cr, lx, ly);
  
  gboolean na = FALSE;
  
  if (expanded)
  {
    target_pad_left = lx + (w - _get_applet_manager_size (bg, position)) * align;
    pad_diff = priv->last_pad - target_pad_left;
    if (update_state)
      priv->last_pad = target_pad_left;

    if (first_widget_is_special)
    {
      /* start from bottom */
      if (internal)
      {
        cairo_new_path (cr);
        ly = y;
        cairo_move_to (cr, lx, ly);
      }

      _line_from_to (cr, &lx, &ly, lx, y2);
      /* jump first special widget */
      i = i->next;
      ++j;
    }
    else
    {
      /* start from top */
      if (internal)
      {
        cairo_new_path (cr);
        ly = y;
        cairo_move_to (cr, lx, ly);
      }

      _line_from_to (cr, &lx, &ly, lx, y);
    }
  }
  else
  {
    if (align == 0.)
    {
      if (first_widget_is_special)
      {
        /* start from bottom */
        if (internal)
        {
          cairo_new_path (cr);
          ly = y;
          cairo_move_to (cr, lx, ly);
        }
        _line_from_to (cr, &lx, &ly, lx, y2);
        /* jump first special widget */
        i = i->next;
        ++j;
      }
      else
      {
        if (internal)
        {
          cairo_new_path (cr);
          ly = y;
          cairo_move_to (cr, lx, ly);
        }
        else
          /* start from top */
          _line_from_to (cr, &lx, &ly, lx, y);
      }
    }
    else
    {
      target_pad_left = lx + dc;
      pad_diff = priv->last_pad - target_pad_left;
      if (update_state)
        priv->last_pad = target_pad_left;

      if (first_widget_is_special)
      {
        /* start from bottom */
        if (internal)
        {
          cairo_new_path (cr);
          lx = lx + dc;
          ly = y;
          cairo_move_to (cr, lx, ly);
          _line_from_to (cr, &lx, &ly, targetx, y3);
          _line_from_to (cr, &lx, &ly, lx + dc, y2);
        }
        else
        {
          _line_from_to (cr, &lx, &ly, lx + dc, y2);
          /* jump first special widget */
        }
        i = i->next;
        ++j;
      }
      else
      {
        /* start from top */
        if (internal)
        {
          cairo_new_path (cr);
          ly = y;
          lx = lx + dc;
          cairo_move_to (cr, lx, ly);
        }
        else
          _line_from_to (cr, &lx, &ly, lx + dc, y);
      }
    }
  }
  /* "first curve" done.   */
  /* now we are on y or y2 */
  /* start loop on widgets */
  targetx = lx;
  gfloat lastx = 0.;
  
  for (; i; i = i->next)
  {
    widget = GTK_WIDGET (i->data);

    if (!IS_SPECIAL (widget)) 
    {
      /* if not special continue */
      continue;
    }
    /* special found */
    if (update_state)
    {
      /* update position array */
      switch (position)
      {
        case GTK_POS_BOTTOM:
        case GTK_POS_TOP:
          targetx = widget->allocation.x + target_pad_left;
          break;
        default:
          targetx = widget->allocation.y + target_pad_left;
          break;
      }
      lastx = g_array_index (priv->pos, gfloat, j);
      if (!expanded)
        /* expand mode has no padding */
        lastx += pad_diff;

      if (fabs (targetx - lastx) > 0.2)
      {
        targetx = lastx + (targetx - lastx) / 4.;
        g_array_index (priv->pos, gfloat, j) = targetx;
        /* needs animation = TRUE */
        na = TRUE;
      }
    }
    else
    {
      /* reuse last calculated position */
      targetx = g_array_index (priv->pos, gfloat, j);
    }

    ++j;

    _line_from_to (cr, &lx, &ly, targetx, ly);
    if (ly == y2)
      _line_from_to (cr, &lx, &ly, lx + d, y);
    else
      _line_from_to (cr, &lx, &ly, lx + d, y2);

  }
  g_list_free (widgets);
  
  if (update_state)
    priv->needs_animation = na;
  
  if (expanded)
  {
    /* make sure that cairo close path in the right way */
    _line_from_to (cr, &lx, &ly, w, ly);
    if (internal)
      _line_from_to (cr, &lx, &ly, lx, y);
    else
      _line_from_to (cr, &lx, &ly, lx, y3);
  }
  else
  {
    if (align == 1.)
    {
      if (internal)
      {
        if (ly == y2)
        {
          _line_from_to (cr, &lx, &ly, w, ly);
          _line_from_to (cr, &lx, &ly, lx, y);
        }
        /* else close path */
      }
      else
      {
        _line_from_to (cr, &lx, &ly, w, ly);
        _line_from_to (cr, &lx, &ly, lx, y3);
      }
    }
    else
    {
      if (internal)
      {
        if (ly == y2)
        {
          _line_from_to (cr, &lx, &ly, w - dc, y2);
          _line_from_to (cr, &lx, &ly, w, y3);
          _line_from_to (cr, &lx, &ly, w - dc, y);
        }
        /* else close path */
      }
      else
      {
        _line_from_to (cr, &lx, &ly, w - dc, ly);
        _line_from_to (cr, &lx, &ly, w, y3);
      }
    }
  }
  cairo_close_path (cr);
}

static void
draw_top_bottom_background (AwnBackground*   bg,
                            GtkPositionType  position,
                            cairo_t*         cr,
                            gfloat           width,
                            gfloat           height)
{
  cairo_pattern_t *pat = NULL;
  cairo_pattern_t *pat_hi = NULL;
  
  /* Basic set-up */
  cairo_set_line_width (cr, 1.0);
  cairo_set_operator (cr, CAIRO_OPERATOR_OVER);

  gboolean expand = FALSE;
  g_object_get (bg->panel, "expand", &expand, NULL);
  
  gfloat align = awn_background_get_panel_alignment (AWN_BACKGROUND (bg));
  
  /* Make sure the bar gets drawn on the 0.5 pixels (for sharp edges) */
  cairo_translate (cr, 0.5, 0.5);
  width -= 0.5;
  height -= 0.5;
  
  if (gtk_widget_is_composited (GTK_WIDGET (bg->panel)) == FALSE)
  {
    goto paint_lines;
  }

  /* create internal path */
  _create_path_lucido (bg, position, cr, -1.0, 0., width, height,
                       TRANSFORM_RADIUS (bg->corner_radius),
                       TRANSFORM_RADIUS (bg->corner_radius),
                       1, expand, align, TRUE);

  /* Draw internal pattern if needed */
  if (bg->enable_pattern && bg->pattern)
  {
    /* Prepare pattern */
    pat_hi = cairo_pattern_create_for_surface (bg->pattern);
    cairo_pattern_set_extend (pat_hi, CAIRO_EXTEND_REPEAT);
    /* Draw */
    cairo_save (cr);
    cairo_clip_preserve (cr);
    cairo_set_source (cr, pat_hi);
    cairo_paint (cr);
    cairo_restore (cr);
    cairo_pattern_destroy (pat_hi);
  }
  /* Prepare the hi-light */
  pat_hi = cairo_pattern_create_linear (0, 0, 0, height);
  awn_cairo_pattern_add_color_stop_color (pat_hi, 0.0, bg->g_histep_1);
  awn_cairo_pattern_add_color_stop_color (pat_hi, 0.3, bg->g_histep_2);
  double red, green, blue, alpha;
  desktop_agnostic_color_get_cairo_color (bg->g_histep_2, &red, &green, &blue, &alpha);
  cairo_pattern_add_color_stop_rgba (pat_hi, 0.4, red, green, blue, 0.);

  /* Prepare the internal background */
  pat = cairo_pattern_create_linear (0, 0, 0, height);
  awn_cairo_pattern_add_color_stop_color (pat, 0.0, bg->border_color);
  awn_cairo_pattern_add_color_stop_color (pat, 1.0, bg->hilight_color);

  /* Draw the internal background gradient */
  cairo_save (cr);
  cairo_clip_preserve (cr);
  cairo_set_source (cr, pat);
  cairo_paint (cr);
  cairo_restore (cr);
  /* Draw the internal hi-light gradient */
  cairo_save (cr);
  cairo_clip (cr);
  cairo_set_source (cr, pat_hi);
  cairo_paint (cr);
  cairo_restore (cr);

  /* Prepare external background gradient*/  
  cairo_pattern_destroy (pat);
  pat = cairo_pattern_create_linear (0, 0, 0, height);
  awn_cairo_pattern_add_color_stop_color (pat, 0.0, bg->g_step_1);
  awn_cairo_pattern_add_color_stop_color (pat, 1.0, bg->g_step_2);

  
  /* create external path */
  _create_path_lucido (bg, position, cr, -1.0, 0., width, height,
                       TRANSFORM_RADIUS (bg->corner_radius),
                       TRANSFORM_RADIUS (bg->corner_radius),
                       0, expand, align, FALSE);
                       
  /* Draw the external background  */
  cairo_save (cr);
  cairo_clip_preserve (cr);
  cairo_set_source (cr, pat);
  cairo_paint (cr);
  cairo_restore (cr);
  cairo_pattern_destroy (pat);
  
  /* Draw the internal hi-light gradient */
  cairo_save (cr);
  cairo_clip (cr);
  cairo_set_source (cr, pat_hi);
  cairo_paint (cr);
  cairo_restore (cr);
  
  cairo_pattern_destroy (pat_hi);

  return;
  /* if not composited */
paint_lines:

  if (expand)
  {
    /* Internal border */
    awn_cairo_set_source_color (cr, bg->hilight_color);
    cairo_rectangle (cr, 1, 1, width - 3, height + 3);
    cairo_stroke (cr);

    /* External border */    
    awn_cairo_set_source_color (cr, bg->border_color);
    cairo_rectangle (cr, 1, 1, width - 1, height + 3);
  }
  else
  {
    awn_cairo_set_source_color (cr, bg->border_color);
    _create_path_lucido (bg, position, cr, 0., 0., width, height,
                         TRANSFORM_RADIUS (bg->corner_radius),
                         TRANSFORM_RADIUS (bg->corner_radius),
                         0, expand, align, TRUE);
    cairo_stroke (cr);
    awn_cairo_set_source_color (cr, bg->hilight_color);
    _create_path_lucido (bg, position, cr, 1., 1., width-1., height-1.,
                         TRANSFORM_RADIUS (bg->corner_radius),
                         TRANSFORM_RADIUS (bg->corner_radius),
                         0, expand, align, FALSE);
  }
  cairo_stroke (cr);
}


static
void awn_background_lucido_padding_request (AwnBackground *bg,
    GtkPositionType position,
    guint *padding_top,
    guint *padding_bottom,
    guint *padding_left,
    guint *padding_right)
{
  #define TOP_PADDING 2
  gboolean expand = FALSE;
  g_object_get (bg->panel, "expand", &expand, NULL);
  gint side_padding = expand ? 0 : TRANSFORM_RADIUS (bg->corner_radius);
  gint zero_padding = 0;

  gfloat align = awn_background_get_panel_alignment (bg);
  if (awn_background_do_rtl_swap (bg))
  {
    if (align <= 0.0 || align >= 1.0)
    {
      zero_padding = side_padding;
      side_padding = 0;
    }
  }

  switch (position)
  {
    case GTK_POS_TOP:
      *padding_top  = 0;
      *padding_bottom = TOP_PADDING;
      *padding_left = align == 0.0 ? zero_padding : side_padding;
      *padding_right = align == 1.0 ? zero_padding : side_padding;
      break;
    case GTK_POS_BOTTOM:
      *padding_top  = TOP_PADDING;
      *padding_bottom = 0;
      *padding_left = align == 0.0 ? zero_padding : side_padding;
      *padding_right = align == 1.0 ? zero_padding : side_padding;
      break;
    case GTK_POS_LEFT:
      *padding_top  = align == 0.0 ? zero_padding : side_padding;
      *padding_bottom = align == 1.0 ? zero_padding : side_padding;
      *padding_left = 0;
      *padding_right = TOP_PADDING;
      break;
    case GTK_POS_RIGHT:
      *padding_top  = align == 0.0 ? zero_padding : side_padding;
      *padding_bottom = align == 1.0 ? zero_padding : side_padding;
      *padding_left = TOP_PADDING;
      *padding_right = 0;
      break;
    default:
      break;
  }
}



static void
awn_background_lucido_draw (AwnBackground  *bg,
                            cairo_t        *cr,
                            GtkPositionType  position,
                            GdkRectangle   *area)
{
  gint temp;
  gint x = area->x, y = area->y;
  gint width = area->width, height = area->height;
  cairo_save (cr);

  switch (position)
  {
    case GTK_POS_RIGHT:
      cairo_translate (cr, 0., y + height);
      cairo_scale (cr, 1., -1.);
      cairo_translate (cr, x, height);
      cairo_rotate (cr, M_PI * 1.5);
      temp = width;
      width = height;
      height = temp;
      break;
    case GTK_POS_LEFT:
      cairo_translate (cr, x + width, y);
      cairo_rotate (cr, M_PI * 0.5);
      temp = width;
      width = height;
      height = temp;
      break;
    case GTK_POS_TOP:
      cairo_translate (cr, x, y + height);
      cairo_scale (cr, 1., -1.);
      break;
    default:
      cairo_translate (cr, x, y);
      break;
  }
  
  draw_top_bottom_background (bg, position, cr, width, height);
  
  cairo_restore (cr);
}

static void 
_set_special_widget_width_and_transparent (AwnBackground *bg,
                                           gint          width,
                                           gboolean      transp,
                                           gboolean      dispose)
{
  GList *widgets = _get_applet_widgets (bg);
  GList *i = widgets;
  GtkWidget *widget = NULL;

  if (i && IS_SPECIAL (i->data) && !dispose)
  {
    widget = GTK_WIDGET (i->data);
    g_object_set (G_OBJECT (widget), "separator-size", 0, NULL);
    g_object_set (G_OBJECT (widget), "transparent", transp, NULL);
    i = i->next;
  }

  for (; i; i = i->next)
  {
    widget = GTK_WIDGET (i->data);
    if (!IS_SPECIAL (widget)) 
    {
      /* if not special continue */
      continue;
    }
    g_object_set (G_OBJECT (widget), "separator-size", width, NULL);
    g_object_set (G_OBJECT (widget), "transparent", transp, NULL);
  }

  g_list_free (widgets);
}

static void _restart_timeout (AwnBackground *bg,
                              AwnBackgroundLucidoPrivate *priv)
{
  priv->needs_animation = TRUE;
  if (!priv->tid)
  {
    priv->tid = g_timeout_add (ANIM_TIMEOUT, (GSourceFunc)awn_background_lucido_redraw, bg);
  }
}

static gboolean
awn_background_lucido_get_needs_redraw (AwnBackground *bg,
                                        GtkPositionType position,
                                        GdkRectangle *area)
{
  /* Check default needs redraw */
  gboolean nr = AWN_BACKGROUND_CLASS (awn_background_lucido_parent_class)->
                                      get_needs_redraw (bg, position, area);
  if (nr)
  {
    return TRUE;
  }

  /* Obtain Lucido Privates */
  AwnBackgroundLucido *lbg = NULL;
  lbg = AWN_BACKGROUND_LUCIDO (bg);
  AwnBackgroundLucidoPrivate *priv;
  priv = AWN_BACKGROUND_LUCIDO_GET_PRIVATE (lbg);

  /* obtain expand and align values */
  gboolean expand = FALSE;
  g_object_get (bg->panel, "expand", &expand, NULL);
  gfloat align = awn_background_get_panel_alignment (AWN_BACKGROUND (bg));
  
  /* obtain Panel width */
  gfloat w = 0.;
  switch (position)
  {
    case GTK_POS_BOTTOM:
    case GTK_POS_TOP:
      w = area->width;
      break;
    default:
      w = area->height;
      break;
  }
  
  GList* widgets = _get_applet_widgets (bg);
  GList* i = widgets;
  GtkWidget* widget = NULL;

  /* find the padding */
  gfloat target_pad_left = 0.;
   
  if (expand)
    target_pad_left = (w - _get_applet_manager_size (bg, position)) * align;
  else if (align > 0.)
    target_pad_left = TRANSFORM_RADIUS (bg->corner_radius);

  /* variables to check: number of special applets, order of that applets
   * and check for positions
   */
  gint n = 0;
  gint p = 0;
  w = 0.;

  for (; i; i = i->next)
  {
    widget = GTK_WIDGET (i->data);
    if (IS_SPECIAL (widget))
    {
      ++n;
      p += n;
      switch (position)
      {
        case GTK_POS_BOTTOM:
        case GTK_POS_TOP:
          w += (widget->allocation.x) * n;
          break;
        default:
          w += (widget->allocation.y) * n;
          break;
      }
    }
  }
  g_list_free (widgets);
  /* add padding & expand to width check */
  w += target_pad_left;
  w += -10000. * expand;

  /* number of specials changed */
  if (priv->expn != n)
  {
    priv->expn = n;
    priv->expp = p;
    priv->expw = w;
    _set_special_widget_width_and_transparent 
                  (bg, TRANSFORM_RADIUS (bg->corner_radius), TRUE, FALSE);
    _destroy_positions (bg);
    _restart_timeout (bg, priv);
    gtk_widget_queue_draw (GTK_WIDGET (AWN_BACKGROUND (bg)->panel));
    return TRUE;
  }
  if (priv->expp != p)
  {
    priv->expp = p;
    priv->expw = w;
    _restart_timeout (bg, priv);
    return TRUE;
  }
  if (priv->expw != w)
  {
    priv->expw = w;
    _restart_timeout (bg, priv);
    return TRUE;
  }
  return FALSE;
}

static void
awn_background_lucido_get_shape_mask (AwnBackground   *bg,
                                      cairo_t         *cr,
                                      GtkPositionType position,
                                      GdkRectangle    *area)
{
  gint temp;
  gint x = area->x, y = area->y;
  gint width = area->width, height = area->height;
  gfloat   align = 0.5;
  gboolean expand = FALSE;

  cairo_save (cr);

  align = awn_background_get_panel_alignment (bg);
  g_object_get (bg->panel, "expand", &expand, NULL);

  switch (position)
  {
    case GTK_POS_RIGHT:
      cairo_translate (cr, 0., y + height);
      cairo_scale (cr, 1., -1.);
      cairo_translate (cr, x, height);
      cairo_rotate (cr, M_PI * 1.5);
      temp = width;
      width = height;
      height = temp;
      break;
    case GTK_POS_LEFT:
      cairo_translate (cr, x + width, y);
      cairo_rotate (cr, M_PI * 0.5);
      temp = width;
      width = height;
      height = temp;
      break;
    case GTK_POS_TOP:
      cairo_translate (cr, x, y + height);
      cairo_scale (cr, 1., -1.);
      break;
    default:
      cairo_translate (cr, x, y);
      break;
  }
  if (expand)
  {
    cairo_rectangle (cr, 0, 0, width, height + 2);
  }
  else
  {
    _create_path_lucido (bg, position, cr, 0, 0., width, height,
                         TRANSFORM_RADIUS (bg->corner_radius),
                         TRANSFORM_RADIUS (bg->corner_radius),
                         0, expand, align, FALSE);
    cairo_fill (cr);
    _create_path_lucido (bg, position, cr, 0, 0., width, height,
                         TRANSFORM_RADIUS (bg->corner_radius),
                         TRANSFORM_RADIUS (bg->corner_radius),
                         1, expand, align, FALSE);

  }
  cairo_fill (cr);

  cairo_restore (cr);
}
/* vim: set et ts=2 sts=2 sw=2 : */
