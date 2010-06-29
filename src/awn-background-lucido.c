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
  gint      expw;
  gint      expn;
  gint      expp;
  gfloat    lastx;
  gfloat    lastxend;
  GArray   *pos;
  gint      pos_size;
  guint     tid;
  gboolean  needs_animation;
};

/* Timeout for animation -> 40 = 25fps*/
#define ANIM_TIMEOUT 40
/* ANIMATION SPEED needs to be greater than 0. - Lower values are faster */
#define ANIMATION_SPEED 16.

/* draw shape mask for debug */
#define DEBUG_SHAPE_MASK      FALSE

/* enables a little speedup, that avoid to draw curves when internal = TRUE*/
#define LITTLE_SPEED_UP       TRUE

#define TRANSFORM_RADIUS(x) sqrt(x/50.)*50.

#define IS_SPECIAL(x) AWN_IS_SEPARATOR(x)

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

  _set_special_widget_width_and_transparent (
                                        bg,
                                        TRANSFORM_RADIUS (bg->corner_radius),
                                        TRUE,
                                        FALSE);

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
                            G_CALLBACK (
                            awn_background_lucido_corner_radius_changed),
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

static void
awn_background_lucido_dispose (GObject *object)
{
  _set_special_widget_width_and_transparent 
                          (AWN_BACKGROUND (object), 10, FALSE, TRUE);

  AwnBackgroundLucido *lbg = AWN_BACKGROUND_LUCIDO (object);
  AwnBackgroundLucidoPrivate *priv = AWN_BACKGROUND_LUCIDO_GET_PRIVATE (lbg);
  g_array_free (priv->pos, TRUE);

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

#if DEBUG_SHAPE_MASK
  bg_class->draw = awn_background_lucido_get_shape_mask;
#else
  bg_class->draw = awn_background_lucido_draw;
#endif
  bg_class->padding_request = awn_background_lucido_padding_request;
  bg_class->get_shape_mask = awn_background_lucido_get_shape_mask;
  bg_class->get_input_shape_mask = awn_background_lucido_get_shape_mask;
  bg_class->get_needs_redraw = awn_background_lucido_get_needs_redraw;
  
  g_type_class_add_private (obj_class, sizeof (AwnBackgroundLucidoPrivate));
}

static void
awn_background_lucido_init (AwnBackgroundLucido *bg)
{
  AwnBackgroundLucidoPrivate *priv = AWN_BACKGROUND_LUCIDO_GET_PRIVATE (bg);
  priv->lastx = 0;
  priv->lastxend = 1;
  priv->needs_animation = FALSE;
  priv->tid = 0;
  priv->pos = g_array_new (FALSE, TRUE, sizeof (gfloat));
  priv->pos_size = 0;
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

/* ANIMATION FUNCTIONS */

/*
 * _add_n_positions:
 * adds n positions to position array that contains
 * last positions of special applets
 * Usually called when a special applet is added
 */
static void
_add_n_positions (AwnBackgroundLucidoPrivate *priv, gint n)
{
  gfloat startpos = 0.;
  while (n-- > 0)
  {
    g_array_append_val (priv->pos, startpos);
    ++priv->pos_size;
  }  
}

/*
 * awn_background_lucido_redraw:
 * @lbg: the lucido background ojbect
 *
 * Queue redraw of the panel and repeat itself if needed
 */
static gboolean
awn_background_lucido_redraw (AwnBackgroundLucido *lbg)
{
  g_return_val_if_fail (AWN_IS_BACKGROUND_LUCIDO (lbg), FALSE);

  AwnBackgroundLucidoPrivate *priv;
  AwnBackground *bg = AWN_BACKGROUND (lbg);
  priv = AWN_BACKGROUND_LUCIDO_GET_PRIVATE (lbg);

  if (priv->needs_animation)
  {
    awn_background_invalidate (bg);
    gtk_widget_queue_draw (GTK_WIDGET (bg->panel));
    return TRUE;
  }
  else
  {
    priv->tid = 0;
    return FALSE;
  }
}

/*
 * _restart_timeout:
 * restarts animation's timer if needed
 */
static void _restart_timeout (AwnBackground *bg,
                              AwnBackgroundLucidoPrivate *priv)
{
  priv->needs_animation = TRUE;
  if (!priv->tid)
  {
    priv->tid = g_timeout_add (ANIM_TIMEOUT, (GSourceFunc)awn_background_lucido_redraw, bg);
  }
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

/*
 * Gets applet manager's childs.
 * Sets docklet mode = TRUE if panel is in docklet mode
 */
static GList*
_get_applet_widgets (AwnBackground* bg, gboolean *docklet_mode)
{
  AwnAppletManager *manager = NULL;
  g_object_get (bg->panel, "applet-manager", &manager, NULL);

  if (docklet_mode)
  {
    *docklet_mode = awn_applet_manager_get_docklet_mode (manager);
  }

  return gtk_container_get_children (GTK_CONTAINER (manager));
}

/*
 * Get applet manager size
 * Get offset-left stored in "x" variable passed to method
 *
 */
static gint
_get_applet_manager_size (AwnBackground* bg, GtkPositionType position, float *x)
{
  AwnAppletManager *manager = NULL;
  g_object_get (bg->panel, "applet-manager", &manager, NULL);
  GtkWidget *widget = GTK_WIDGET (manager);
  gint wx, wy;
  gtk_widget_translate_coordinates(widget, gtk_widget_get_toplevel(widget),
                                       0, 0, &wx, &wy);
  switch (position)
  {
    case GTK_POS_BOTTOM:
    case GTK_POS_TOP:
      if (x)
        *x = wx;
      return widget->allocation.width;
      break;
    default:
      if (x)
        *x = wy;
      return widget->allocation.height;
      break;
  }
}

/*
 * coord_get_near:
 * @from: the current position
 * @to: the target position
 * @return: a position between "from" and "to", if distance = 1, returns "to" 
 */
static float
coord_get_near (const gfloat from, const gfloat to)
{
  float inc = abs (to - from);
  float step = inc / ANIMATION_SPEED + 1.; // makes the resize shiny
  inc = MIN (inc, step);
  if (to > from)
  {
    return lroundf (from + inc);
  }
  else
  {
    return lroundf (from - inc);
  }
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
 * @update_positions: if FALSE it uses positions stored in priv->pos,
 *                    otherwise updates the position stored in priv->pos
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
                      gboolean        update_positions)
{
  AwnBackgroundLucido *lbg = AWN_BACKGROUND_LUCIDO (bg);
  AwnBackgroundLucidoPrivate *priv = AWN_BACKGROUND_LUCIDO_GET_PRIVATE (lbg);
  cairo_new_path (cr);
  gfloat appmx = 0.;
  _get_applet_manager_size (bg, position, &appmx);
  gboolean needs_animation = FALSE;

  /* x variable stores the starting x point for draw the panel */
  if (update_positions)
  {
    if (!expanded)
    {
      x += appmx - dc;
    }
    x = lroundf (x);
    if (x != priv->lastx)
    {
      needs_animation = TRUE;
    }
    x = coord_get_near (priv->lastx, x);
    priv->lastx = x;
  }
  else
  {
    x = priv->lastx;
  }

  gfloat curx = x;
  gfloat lx = x;
  gfloat ly = y;
  gfloat y3 = y + h;
  gfloat y2 = y3 - 5;
  /* j = index of last special widget found */
  gint j = -1;

  /* curves symmetry acts on starting y of the stripe */
  if (internal)
  {
    y = y + (h - 5) * (1. - bg->curves_symmetry);
  }

  /* Get list of widgets */
  gboolean docklet_mode = FALSE;
  GList *widgets = _get_applet_widgets (bg, &docklet_mode);
  GList *i = widgets;
  GtkWidget *widget = NULL;
  gboolean first_widget_is_special = FALSE;
  if (i && docklet_mode)
  {
    i = i->next;
  }
  if (i && IS_SPECIAL (i->data))
  {
    first_widget_is_special = TRUE;
  }

  /* We start from left */
  ly = y3;
  cairo_move_to (cr, lx, ly);

  if (expanded)
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
    }
    else
    {
      /* start from top */
      if (internal)
      {
        cairo_new_path (cr);
        ly = y;
        cairo_move_to (cr, lx, ly);
#if LITTLE_SPEED_UP
        _line_from_to (cr, &lx, &ly, lx, y2);
#else
        _line_from_to (cr, &lx, &ly, lx, y);
#endif
      }
      else
      {
        _line_from_to (cr, &lx, &ly, lx, y);
      }
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
        {
          /* start from top */
          _line_from_to (cr, &lx, &ly, lx, y);
        }
      }
    }
    else
    {
      if (first_widget_is_special)
      {
        /* start from bottom */
        if (internal)
        {
          cairo_new_path (cr);
          lx = lx + dc;
          ly = y;
          cairo_move_to (cr, lx, ly);
          _line_from_to (cr, &lx, &ly, curx, y3);
          _line_from_to (cr, &lx, &ly, lx + dc, ly);
          _line_from_to (cr, &lx, &ly, lx, y2);
        }
        else
        {
          _line_from_to (cr, &lx, &ly, lx + dc, y2);
          /* jump first special widget */
        }
        i = i->next;
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
#if LITTLE_SPEED_UP
          _line_from_to (cr, &lx, &ly, lx, y2);
#endif
        }
        else
        {
          _line_from_to (cr, &lx, &ly, lx + dc, y);
        }
      }
    }
  }
  /* "first curve" done. now we are on y or y2.
   * start loop on widgets
   */
  curx = lx;
  gint wx, wy;
  if (!docklet_mode)
  {
    for (; i; i = i->next)
    {
      widget = GTK_WIDGET (i->data);
      if (!IS_SPECIAL (widget)) 
      {
        /* if not special continue */
        continue;
      }
      /* special found */
      ++j;
      gtk_widget_translate_coordinates(widget, gtk_widget_get_toplevel(widget),
                                       0, 0, &wx, &wy);
      switch (position)
      {
        case GTK_POS_BOTTOM:
        case GTK_POS_TOP:
          curx = wx;
          break;
        default:
          curx = wy;
          break;
      }
      if (priv->pos_size <= j)
      {
        _add_n_positions (priv, 1);
      }
      if (update_positions)
      {
        curx = lroundf (curx);
        if (curx != g_array_index (priv->pos, gfloat, j))
        {
          needs_animation = TRUE;
        }
        curx = coord_get_near (g_array_index (priv->pos, gfloat, j), curx);
        g_array_index (priv->pos, gfloat, j) = curx;
      }
      else
      {
        curx = g_array_index (priv->pos, gfloat, j);
      }
      /* there's no reason to draw the curve, because the "external" part
       * will do the job for us clearing the context behind external
       */
#if LITTLE_SPEED_UP
      if (internal)
      {
        continue;
      }
#endif
      if (curx < 0)
      {
        continue;
      }
      _line_from_to (cr, &lx, &ly, curx, ly);
      if (ly == y2)
      {
        _line_from_to (cr, &lx, &ly, lx + d, y);
      }
      else
      {
        _line_from_to (cr, &lx, &ly, lx + d, y2);
      }
    }
  }

  g_list_free (widgets);

  /* w stores the "right corner", lastxend equals to last w */
  if (update_positions)
  {
    w = lroundf (w);
    if (w != priv->lastxend)
    {
      needs_animation = TRUE;
    }
    gfloat neww = coord_get_near (priv->lastxend, w);
    w = MIN (w, neww);
    priv->lastxend = w;
  }
  else
  {
    w = priv->lastxend;
  }

  if (expanded)
  {
    /* make sure that cairo close path in the right way */
    _line_from_to (cr, &lx, &ly, w, ly);
    if (internal)
    {
      _line_from_to (cr, &lx, &ly, lx, y);
    }
    else
    {
      _line_from_to (cr, &lx, &ly, lx, y3);
    }
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
#if LITTLE_SPEED_UP
          _line_from_to (cr, &lx, &ly, lx, y3);
          _line_from_to (cr, &lx, &ly, w, ly);
#else
          _line_from_to (cr, &lx, &ly, w, y3);
#endif
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
  if (update_positions)
  {
    if (needs_animation)
    {
      _restart_timeout (bg, priv);
    }
    else
    {
      priv->needs_animation = FALSE;
    }
  }
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
  if (!expand)
  {
    /* use only in non-expanded mode */
    cairo_translate (cr, 0.5, 0.);
    width -= 0.5;
  }

  if (gtk_widget_is_composited (GTK_WIDGET (bg->panel)) == FALSE)
  {
    goto paint_lines;
  }

  gfloat x = 0.,
         y = 0.;

  /* create internal path */
  _create_path_lucido (bg, position, cr, x, y, width, height,
                       TRANSFORM_RADIUS (bg->corner_radius),
                       TRANSFORM_RADIUS (bg->corner_radius),
                       TRUE, expand, align, TRUE);

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
  pat_hi = cairo_pattern_create_linear (x, y, 0., height);
  awn_cairo_pattern_add_color_stop_color (pat_hi, 0.0, bg->g_histep_1);
  awn_cairo_pattern_add_color_stop_color (pat_hi, 0.3, bg->g_histep_2);
  double red, green, blue, alpha;
  desktop_agnostic_color_get_cairo_color (bg->g_histep_2, &red, &green, &blue, &alpha);
  cairo_pattern_add_color_stop_rgba (pat_hi, 0.4, red, green, blue, 0.);

  /* Prepare the internal background */
  pat = cairo_pattern_create_linear (x, y, 0., height);
  awn_cairo_pattern_add_color_stop_color (pat, (1. - bg->curves_symmetry), bg->border_color);
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
  pat = cairo_pattern_create_linear (x, y, 0., height);
  awn_cairo_pattern_add_color_stop_color (pat, 0.0, bg->g_step_1);
  awn_cairo_pattern_add_color_stop_color (pat, 1.0, bg->g_step_2);

  /* create external path */
  _create_path_lucido (bg, position, cr, x, y, width, height,
                       TRANSFORM_RADIUS (bg->corner_radius),
                       TRANSFORM_RADIUS (bg->corner_radius),
                       FALSE, expand, align, FALSE);

  /* clean below external background */
  cairo_save (cr);
  cairo_clip_preserve (cr);
  cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
  cairo_paint (cr);
  cairo_restore (cr);
  cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
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
    cairo_rectangle (cr, 1., 1., width - 3., height + 3.);
    cairo_stroke (cr);

    /* External border */    
    awn_cairo_set_source_color (cr, bg->border_color);
    cairo_rectangle (cr, 1., 1., width - 1., height + 3.);
  }
  else
  {
    awn_cairo_set_source_color (cr, bg->border_color);
    _create_path_lucido (bg, position, cr, 0., 0., width, height,
                         TRANSFORM_RADIUS (bg->corner_radius),
                         TRANSFORM_RADIUS (bg->corner_radius),
                         FALSE, expand, align, TRUE);
    cairo_stroke (cr);
    awn_cairo_set_source_color (cr, bg->hilight_color);
    _create_path_lucido (bg, position, cr, 1., 1., width-1., height-1.,
                         TRANSFORM_RADIUS (bg->corner_radius),
                         TRANSFORM_RADIUS (bg->corner_radius),
                         FALSE, expand, align, FALSE);
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
      height += y;
      cairo_translate (cr, 0., height);
      cairo_scale (cr, 1., -1.);
      cairo_translate (cr, x, height);
      cairo_rotate (cr, M_PI * 1.5);
      temp = width;
      width = height;
      height = temp;
      break;
    case GTK_POS_LEFT:
      height += y;
      cairo_translate (cr, x + width, 0.);
      cairo_rotate (cr, M_PI * 0.5);
      temp = width;
      width = height;
      height = temp;
      break;
    case GTK_POS_TOP:
      width += x;
      cairo_translate (cr, 0., y + height);
      cairo_scale (cr, 1., -1.);
      break;
    default:
      width += x;
      cairo_translate (cr, 0., y);
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
  GList *widgets = _get_applet_widgets (bg, NULL);
  GList *i = widgets;
  GtkWidget *widget = NULL;

  if (i && IS_SPECIAL (i->data) && !dispose)
  {
    widget = GTK_WIDGET (i->data);
    g_object_set (G_OBJECT (widget), "separator-size", 1, NULL);
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

  /* Check separators positions & sizes changed */
  GList *widgets = _get_applet_widgets (bg, NULL);
  GList *i = widgets;
  GtkWidget *widget = NULL;
  gint  wcheck = 0,
        ncheck = 0,
        pcheck = 0,
        j = 0;

  if (i && IS_SPECIAL (i->data))
  {
    ++ncheck;
  }
  for (; i; i = i->next)
  {
    ++j;
    widget = GTK_WIDGET (i->data);
    if (!IS_SPECIAL (widget)) 
    {
      /* if not special continue */
      continue;
    }
    ++ncheck;
    pcheck += j;
    switch (position)
    {
      case GTK_POS_LEFT:
      case GTK_POS_RIGHT:
        wcheck += (widget->allocation.y * ncheck);
        break;
      default:
        wcheck += (widget->allocation.x * ncheck);
        break;
    }
  }
  g_list_free (widgets);

  pcheck += _get_applet_manager_size (bg, position, NULL);

  AwnBackgroundLucido *lbg = AWN_BACKGROUND_LUCIDO (bg);
  AwnBackgroundLucidoPrivate *priv = AWN_BACKGROUND_LUCIDO_GET_PRIVATE (lbg);

  if (priv->pos_size < ncheck) _add_n_positions (priv, ncheck - priv->pos_size);
  if (priv->expn != ncheck)
  {
    /* added/removed a "special" widget */
    _set_special_widget_width_and_transparent 
                  (bg, TRANSFORM_RADIUS (bg->corner_radius), TRUE, FALSE);

    priv->expn = ncheck;
    priv->expp = pcheck;
    priv->expw = wcheck;
    /* used to refresh bar */
    awn_background_emit_changed (bg);
    return TRUE;
  }

  if (priv->expp != pcheck)
  {
    priv->expp = pcheck;
    priv->expw = wcheck;
    /* used to refresh bar */
    awn_background_emit_changed (bg);
    return TRUE;
  }
  if (priv->expw != wcheck)
  {
    priv->expw = wcheck;
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
      height += y;
      cairo_translate (cr, 0., height);
      cairo_scale (cr, 1., -1.);
      cairo_translate (cr, x, height);
      cairo_rotate (cr, M_PI * 1.5);
      temp = width;
      width = height;
      height = temp;
      break;
    case GTK_POS_LEFT:
      height += y;
      cairo_translate (cr, x + width, 0.);
      cairo_rotate (cr, M_PI * 0.5);
      temp = width;
      width = height;
      height = temp;
      break;
    case GTK_POS_TOP:
      width += x;
      cairo_translate (cr, 0., y + height);
      cairo_scale (cr, 1., -1.);
      break;
    default:
      width += x;
      cairo_translate (cr, 0., y);
      break;
  }
  if (expand)
  {
    cairo_rectangle (cr, 0., 0., width, height + 2.);
  }
  else
  {
    _create_path_lucido (bg, position, cr, 0., 0., width, height,
                         TRANSFORM_RADIUS (bg->corner_radius),
                         TRANSFORM_RADIUS (bg->corner_radius),
                         FALSE, expand, align, FALSE);
    cairo_fill (cr);
    _create_path_lucido (bg, position, cr, 0., 0., width, height,
                         TRANSFORM_RADIUS (bg->corner_radius),
                         TRANSFORM_RADIUS (bg->corner_radius),
                         TRUE, expand, align, FALSE);

  }
  cairo_fill (cr);

  cairo_restore (cr);
}
/* vim: set et ts=2 sts=2 sw=2 : */
