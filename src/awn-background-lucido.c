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
  gfloat    lastx;
  gfloat    lastxend;
  GArray   *pos;
  gint      pos_size;
  guint     tid;
  gboolean  needs_animation;
};

#define TOP_PADDING 2
/* Timeout for animation -> 40 = 25fps*/
#define ANIM_TIMEOUT 40
/* ANIMATION SPEED needs to be greater than 0. - Lower values are faster */
#define ANIMATION_SPEED 16.

/* draw shape mask for debug */
#define DEBUG_SHAPE_MASK      FALSE

#define TRANSFORM_RADIUS(x) sqrt(x/60.)*60.

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
awn_background_lucido_applets_refreshed (AwnBackground *bg)
{
  _set_special_widget_width_and_transparent 
                  (bg, TRANSFORM_RADIUS (bg->corner_radius), TRUE, FALSE);
  awn_background_emit_changed (bg);
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

  AwnAppletManager *manager = NULL;
  g_object_get (bg->panel, "applet-manager", &manager, NULL);
  g_return_if_fail (manager);
  g_signal_connect_swapped (manager, "applets-refreshed",
                      G_CALLBACK (awn_background_lucido_applets_refreshed), bg);
  awn_background_lucido_applets_refreshed (AWN_BACKGROUND (bg));
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

  AwnAppletManager *manager = NULL;
  g_object_get (AWN_BACKGROUND (object)->panel,
                                "applet-manager", &manager, NULL);
  if (manager)
  {
    g_signal_handlers_disconnect_by_func (manager, 
        G_CALLBACK (awn_background_lucido_applets_refreshed), object);
  }
  /* remove animation timer */
  if (priv->tid)
  {
    g_source_remove (priv->tid);
    priv->tid = 0;
  }

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
  AwnBackgroundLucidoPrivate *priv = AWN_BACKGROUND_LUCIDO_GET_PRIVATE (bg);
  priv->lastx = -1;
  priv->lastxend = INT_MAX;
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
_add_n_positions (AwnBackgroundLucidoPrivate *priv, gint n, gfloat startpos)
{
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
_get_applet_widgets (AwnBackground* bg)
{
  AwnAppletManager *manager = NULL;
  g_object_get (bg->panel, "applet-manager", &manager, NULL);
  if (!manager)
  {
    return NULL;
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
 *
 * returns the calculated y coordinate to draw the gradient
 */
static gfloat
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
                      gboolean        composited,
                      gboolean        update_positions,
                      gboolean        shape_mask)
{
  AwnBackgroundLucido *lbg = AWN_BACKGROUND_LUCIDO (bg);
  AwnBackgroundLucidoPrivate *priv = AWN_BACKGROUND_LUCIDO_GET_PRIVATE (lbg);
  if (shape_mask)
  {
    update_positions = FALSE;
  }
  gfloat applet_manager_x = 0.;
  _get_applet_manager_size (bg, position, &applet_manager_x);
  gboolean needs_animation = FALSE;
  gfloat x_start_limit = lroundf (x);

  /****************************************************************************/
  /********************     UPDATE STARTING POINT     *************************/
  /****************************************************************************/
  /* x variable stores the starting x point for draw the panel */
  if (!composited)
  {
    /* animation disabled if not composited */
    priv->lastx = x;
  }
  else if (update_positions)
  {
    /* the start position is animated by the panel */
    x = priv->lastx = MAX (x_start_limit, 0.);
  }
  else
  {
    x = priv->lastx;
  }
  /****************************************************************************/
  /********************     INIT GENERAL COORDINATES  *************************/
  /****************************************************************************/
  gfloat sym = bg->curves_symmetry;
  gfloat curx = x;
  gfloat lx = x;
  gfloat ly = y;
  gfloat y3 = y + h;
  gfloat y2 = y3 - 8. * bg->thickness;
  gfloat rdc = dc;

  /****************************************************************************/
  /********************     CURVES SYMMETRY HANDLER   *************************/
  /****************************************************************************/
  gfloat exty = y + h * fabs (sym - 0.5) * 2.;
  if (exty > y2)
  {
    y2 = exty;
  }
  if (internal && sym > 0.5)
  {
    y = exty;
  }
  else if (!internal && sym < 0.5)
  {
    y = exty;
  }
  /****************************************************************************/
  /********************     OBTAIN LIST OF APPLETS    *************************/
  /****************************************************************************/
  gboolean docklet_mode = awn_panel_get_docklet_mode (bg->panel);
  GList *widgets = _get_applet_widgets (bg);
  if (widgets && awn_background_do_rtl_swap (bg))
  {
    widgets = g_list_reverse (widgets);
  }
  GList *i = widgets;
  GtkWidget *widget = NULL;
  /* j = index of last special widget found */
  gint j = -1;
  /* Check for docklet mode - In docklet mode first widget is the docklet */
  if (i && docklet_mode)
  {
    i = i->next;
  }
  /* Check if the first widget is special */
  gboolean first_widget_is_special = FALSE;
  if (i && IS_SPECIAL (i->data))
  {
    first_widget_is_special = TRUE;
  }
  /****************************************************************************/
  /********************     SETUP EXTERNAL CORNERS    *************************/
  /****************************************************************************/
  if (expanded)
  {
    dc = rdc = 0.;
  }
  else if (align == 0.)
  {
    dc = 0.;
  }
  else if (align == 1.)
  {
    rdc = 0.;
  }
  /****************************************************************************/
  /********************        START THE PATH         *************************/
  /****************************************************************************/
  if (internal)
  {
    cairo_new_path (cr);
    lx = lx + dc;
    ly = y;
    cairo_move_to (cr, lx, ly);
    _line_from_to (cr, &lx, &ly, curx, y3);
  }
  else
  {
    cairo_new_path (cr);
    lx = curx;
    ly = y3;
    cairo_move_to (cr, lx, ly);
  }
  /* check for special in first position */
  if (first_widget_is_special)
  {
    i = i->next;
    _line_from_to (cr, &lx, &ly, lx + dc, y2);
  }
  else if (!internal)
  {
    _line_from_to (cr, &lx, &ly, lx + dc, y);
  }
  else
  {
    _line_from_to (cr, &lx, &ly, lx + dc, exty);
  }
  /****************************************************************************/
  /********************     UPDATE WIDTH FOR LAST X   *************************/
  /****************************************************************************/
  /* w stores the "right corner", lastxend equals to last w */
  if (!composited || update_positions)
  {
    /* the width is animated by the panel */
    priv->lastxend = w;
  }
  else
  {
    w = priv->lastxend;
  }
  /****************************************************************************/
  /********************        LOOP ON APPLETS        *************************/
  /****************************************************************************/
  gfloat saved_y = y;
  if (bg->curves_symmetry < 0.5)
  {
    y = exty;
  }
  curx = lx;
  gint wx, wy;
  /* fake docklet mode if not composited */
  if (!docklet_mode && composited)
  {
    for (; i; i = i->next)
    {
      widget = GTK_WIDGET (i->data);
      if (!IS_SPECIAL (widget)) 
      {
        /* if not special continue */
        continue;
      }
      /* special found, obtain coordinates */
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
        /* New special applet found, resize the array */
        _add_n_positions (priv, 1, MAX (lx, curx));
      }
      /************************************************************************/
      /*****************    UPDATE SINGLE CURVE POSITION  *********************/
      /************************************************************************/
      if (update_positions)
      {
        curx = lroundf (curx);
        if (curx != g_array_index (priv->pos, gfloat, j))
        {
          needs_animation = TRUE;
        }
        curx = coord_get_near (g_array_index (priv->pos, gfloat, j), curx);
        if (curx > (w - rdc - d))
        {
          curx = w - rdc - d;
        }
        g_array_index (priv->pos, gfloat, j) = curx;
      }
      /* when drawing shape mask, use the final coord */
      else if (!shape_mask)
      {
        curx = g_array_index (priv->pos, gfloat, j);
      }
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
  y = saved_y;
  g_list_free (widgets);
  /****************************************************************************/
  /************************     CLOSE THE PATH   ******************************/
  /****************************************************************************/
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
        _line_from_to (cr, &lx, &ly, w, ly);
        _line_from_to (cr, &lx, &ly, lx, y);
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
        _line_from_to (cr, &lx, &ly, w - rdc, y2);
        _line_from_to (cr, &lx, &ly, w, y3);
        _line_from_to (cr, &lx, &ly, w - rdc, y);
      }
      else
      {
        _line_from_to (cr, &lx, &ly, w - rdc, ly);
        _line_from_to (cr, &lx, &ly, w, y3);
      }
    }
  }
  cairo_close_path (cr);
  /****************************************************************************/
  /********************     RESTART ANIMATION IF NEEDED  **********************/
  /****************************************************************************/
  if (update_positions)
  {
    if (needs_animation && composited)
    {
      _restart_timeout (bg, priv);
    }
    else
    {
      /* animation disabled if not composited */
      priv->needs_animation = FALSE;
    }
  }
  return y;
}

static void
draw_top_bottom_background (AwnBackground*   bg,
                            GtkPositionType  position,
                            cairo_t*         cr,
                            gfloat           width,
                            gfloat           height,
                            gint             x_start_limit)
{
  cairo_pattern_t *pat = NULL;

  /* Basic set-up */
  cairo_set_line_width (cr, 1.0);
  cairo_set_operator (cr, CAIRO_OPERATOR_OVER);

  gboolean expand = FALSE;
  g_object_get (bg->panel, "expand", &expand, NULL);

  gfloat align = awn_background_get_panel_alignment (AWN_BACKGROUND (bg));

  /* Make sure the bar gets drawn on the 0.5 pixels (for sharp edges) */
  cairo_translate (cr, -0.5, 0.5);
  width += 1.;

  gboolean composited = awn_panel_get_composited (bg->panel);
  if (composited == FALSE)
  {
    goto paint_lines;
  }

  gfloat x = x_start_limit,
         y = 0.;
  gfloat y_pat;

  /* create internal path */
  y_pat = _create_path_lucido (bg, position, cr, x, y, width, height,
                               TRANSFORM_RADIUS (bg->corner_radius),
                               TRANSFORM_RADIUS (bg->corner_radius),
                               TRUE, expand, align, composited, TRUE, FALSE);

  /* Draw internal pattern if needed */
  if (bg->enable_pattern && bg->pattern)
  {
    /* Prepare pattern */
    pat = cairo_pattern_create_for_surface (bg->pattern);
    cairo_pattern_set_extend (pat, CAIRO_EXTEND_REPEAT);
    /* Draw */
    cairo_save (cr);
    cairo_clip_preserve (cr);
    cairo_set_source (cr, pat);
    cairo_paint (cr);
    cairo_restore (cr);
    cairo_pattern_destroy (pat);
  }

  /* Prepare the internal background */
  pat = cairo_pattern_create_linear (x, y_pat, x, height);
  awn_cairo_pattern_add_color_stop_color (pat, 0., bg->g_histep_1);
  awn_cairo_pattern_add_color_stop_color (pat, 1.0, bg->g_histep_2);

  /* Draw the internal background gradient */
  cairo_save (cr);
  cairo_clip_preserve (cr);
  cairo_set_source (cr, pat);
  cairo_paint (cr);
  cairo_restore (cr);
  awn_cairo_set_source_color (cr, bg->border_color);
  cairo_stroke (cr);

  /* create external path */
  y_pat = _create_path_lucido (bg, position, cr, x, y, width, height,
                               TRANSFORM_RADIUS (bg->corner_radius),
                               TRANSFORM_RADIUS (bg->corner_radius),
                               FALSE, expand, align, composited, FALSE, FALSE);

  /* Prepare external background gradient*/
  cairo_pattern_destroy (pat);
  pat = cairo_pattern_create_linear (x, y_pat, x, height);
  awn_cairo_pattern_add_color_stop_color (pat, 0.0, bg->g_step_1);
  awn_cairo_pattern_add_color_stop_color (pat, 1.0, bg->g_step_2);

  /* Clean below external background */
  cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
  /* Draw the external background  */
  cairo_save (cr);
  cairo_clip_preserve (cr);
  cairo_set_source (cr, pat);
  cairo_paint (cr);
  cairo_restore (cr);
  cairo_pattern_destroy (pat);
  /* Restore operator */
  cairo_set_operator (cr, CAIRO_OPERATOR_OVER);

  /* Draw border */
  awn_cairo_set_source_color (cr, bg->hilight_color);
  cairo_stroke (cr);

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
                         FALSE, expand, align, composited, TRUE, FALSE);
    cairo_stroke (cr);
    awn_cairo_set_source_color (cr, bg->hilight_color);
    _create_path_lucido (bg, position, cr, 1., 1., width-1., height-1.,
                         TRANSFORM_RADIUS (bg->corner_radius),
                         TRANSFORM_RADIUS (bg->corner_radius),
                         FALSE, expand, align, composited, FALSE, FALSE);
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
  gint x_start_limit = x;
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
      x_start_limit = y;
      break;
    case GTK_POS_LEFT:
      height += y;
      cairo_translate (cr, x + width, 0.);
      cairo_rotate (cr, M_PI * 0.5);
      temp = width;
      width = height;
      height = temp;
      x_start_limit = y;
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

  draw_top_bottom_background (bg, position, cr, width, height, x_start_limit);

  cairo_restore (cr);
#if DEBUG_SHAPE_MASK
  awn_background_lucido_get_shape_mask (bg, cr, position, area);
#endif
}

static void 
_set_special_widget_width_and_transparent (AwnBackground *bg,
                                           gint          width,
                                           gboolean      transp,
                                           gboolean      dispose)
{
  GList *widgets = _get_applet_widgets (bg);
  if (widgets && awn_background_do_rtl_swap (bg))
  {
    widgets = g_list_reverse (widgets);
  }
  GList *i = widgets;
  GtkWidget *widget = NULL;

  if (i && IS_SPECIAL (i->data) && !dispose)
  {
    widget = GTK_WIDGET (i->data);
    awn_separator_set_separator_size (AWN_SEPARATOR (widget), 1);
    awn_separator_set_transparent (AWN_SEPARATOR (widget), transp);
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
    awn_separator_set_separator_size (AWN_SEPARATOR (widget), width);
    awn_separator_set_transparent (AWN_SEPARATOR (widget), transp);
  }

  g_list_free (widgets);
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
  gint x_start_limit = x;
  gboolean expand = FALSE;
  g_object_get (bg->panel, "expand", &expand, NULL);

  gfloat align = awn_background_get_panel_alignment (AWN_BACKGROUND (bg));

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
      x_start_limit = y;
      break;
    case GTK_POS_LEFT:
      height += y;
      cairo_translate (cr, x + width, 0.);
      cairo_rotate (cr, M_PI * 0.5);
      temp = width;
      width = height;
      height = temp;
      x_start_limit = y;
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
  cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
  cairo_set_source_rgba (cr, 1., 1., 1., 1.);

  gboolean composited = awn_panel_get_composited (bg->panel);
  if (!composited)
  {
    gfloat rad = TRANSFORM_RADIUS (bg->corner_radius);
    if (expand)
    {
      rad = 0.;
    }
    cairo_new_path (cr);
    gfloat lx = 0., ly = height;
    cairo_move_to (cr, lx, ly);
    _line_from_to (cr, &lx, &ly, lx + (align == 0. ? 0. : rad), 0);
    _line_from_to (cr, &lx, &ly, width - (align == 1. ? 0. : rad), 0);
    _line_from_to (cr, &lx, &ly, width, height);
    cairo_close_path (cr);
  }
  else
  {
    cairo_set_line_width (cr, 1.0);
    cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
    /* Make sure the bar gets drawn on the 0.5 pixels (for sharp edges) */
    cairo_translate (cr, -0.5, 0.5);
    width += 1.;
      /* create internal path */
    _create_path_lucido (bg, position, cr, 0., 0., width, height,
                         TRANSFORM_RADIUS (bg->corner_radius),
                         TRANSFORM_RADIUS (bg->corner_radius),
                         TRUE, expand, align, composited, FALSE, TRUE);
    /* Draw the internal background */
    cairo_fill (cr);
    /* create external path */
    _create_path_lucido (bg, position, cr, 0., 0., width, height,
                         TRANSFORM_RADIUS (bg->corner_radius),
                         TRANSFORM_RADIUS (bg->corner_radius),
                         FALSE, expand, align, composited, FALSE, TRUE);

    /* Draw the external background */
    cairo_fill (cr);
  }

  cairo_fill (cr);
  cairo_restore (cr);
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
  gboolean expand = FALSE;
  g_object_get (bg->panel, "expand", &expand, NULL);
  if (!expand)
  {
    return FALSE;
  }
  /* Check separators positions, 
   * because bar's width doesn't change in expanded mode
   */
  GList *widgets = _get_applet_widgets (bg);
  GList *i = widgets;
  GtkWidget *widget = NULL;
  gint  wcheck = 0, j = 0;

  for (; i; i = i->next)
  {
    ++j;
    widget = GTK_WIDGET (i->data);
    if (!IS_SPECIAL (widget)) 
    {
      /* if not special continue */
      continue;
    }
    switch (position)
    {
      case GTK_POS_LEFT:
      case GTK_POS_RIGHT:
        wcheck += widget->allocation.y * j;
        break;
      default:
        wcheck += widget->allocation.x * j;
        break;
    }
  }
  g_list_free (widgets);
  AwnBackgroundLucidoPrivate *priv = 
                AWN_BACKGROUND_LUCIDO_GET_PRIVATE (AWN_BACKGROUND_LUCIDO (bg));
  if (priv->expw != wcheck)
  {
    priv->expw = wcheck;
    return TRUE;
  }
  return FALSE;
}
/* vim: set et ts=2 sts=2 sw=2 : */
