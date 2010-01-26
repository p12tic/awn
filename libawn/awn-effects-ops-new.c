/*
 *  Copyright (C) 2007-2009 Michal Hruby <michal.mhr@gmail.com>
 *  Copyright (C) 2008 Rodney Cryderman <rcryderman@gmail.com>
 *  Copyright (C) 1999 The Free Software Foundation
 *
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

#include "awn-effects-ops-new.h"
#include "awn-effects-ops-helpers.h"
#include "awn-cairo-utils.h"

#include "anims/awn-effects-shared.h"

static
cairo_surface_t *awn_effects_quark_to_surface(AwnEffects *fx, GQuark quark)
{
  GData **icons = &(AWN_EFFECTS_GET_CLASS(fx)->custom_icons);
  cairo_surface_t *surface = 
    g_datalist_id_get_data(icons, quark);

  return surface;
}

/* returns top left coordinates of the icon (without clipping and offsets) */
void
awn_effects_get_base_coords(AwnEffects *fx, double *x, double *y)
{
  AwnEffectsPrivate *priv = fx->priv;

  switch (fx->position)
  {
    case GTK_POS_TOP:
      *x = (priv->window_width - priv->icon_width) / 2.0;
      *y = fx->icon_offset;
      break;
    case GTK_POS_LEFT:
      *x = fx->icon_offset;
      *y = (priv->window_height - priv->icon_height) / 2.0;
      break;
    case GTK_POS_RIGHT:
      *x = priv->window_width - priv->icon_width - fx->icon_offset;
      *y = (priv->window_height - priv->icon_height) / 2.0;
      break;
    default: /* GTK_POS_BOTTOM: */
      *x = (priv->window_width - priv->icon_width) / 2.0;
      *y = priv->window_height - priv->icon_height - fx->icon_offset;
      break;
  }
}

gboolean awn_effects_pre_op_clear(AwnEffects * fx,
                               cairo_t * cr,
                               GtkAllocation * ds,
                               gpointer user_data
                              )
{
  cairo_save(cr);
  cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
  cairo_set_source_rgba(cr, 0, 0, 0, 0);
  cairo_paint(cr);
  cairo_restore(cr);
  return FALSE;
}

gboolean awn_effects_pre_op_translate(AwnEffects * fx,
                               cairo_t * cr,
                               GtkAllocation * ds,
                               gpointer user_data
                              )
{
  /* this should be always the first op executed, will set correct */
  /* coordinates, so the icon can be painted at [0,0] */
  double dx;
  double dy;

  AwnEffectsPrivate *priv = fx->priv;

  /* width/height_mod is swapped when using LEFT/RIGHT positions */
  switch (fx->position) {
    case GTK_POS_TOP:
      dx = (priv->window_width - priv->icon_width * priv->width_mod)/2.0 + priv->side_offset;
      dy = priv->top_offset;
      dy += fx->icon_offset;
      break;
    case GTK_POS_RIGHT:
      dx = (priv->window_width - priv->icon_width * priv->height_mod) - priv->top_offset;
      dx -= fx->icon_offset;
      dy = (priv->window_height - priv->icon_height * priv->width_mod)/2.0 - priv->side_offset;
      break;
    case GTK_POS_BOTTOM:
      dx = (priv->window_width - priv->icon_width * priv->width_mod)/2.0 - priv->side_offset;
      dy = (priv->window_height - priv->icon_height * priv->height_mod) - priv->top_offset;
      dy -= fx->icon_offset;
      break;
    case GTK_POS_LEFT:
      dx = priv->top_offset;
      dx += fx->icon_offset;
      dy = (priv->window_height - priv->icon_height * priv->width_mod)/2.0 + priv->side_offset;
      break;
    default:
      return FALSE;
  }

  cairo_translate(cr, (int)dx, (int)dy);
  return FALSE; /* ? */
}

gboolean awn_effects_pre_op_clip(AwnEffects * fx,
                               cairo_t * cr,
                               GtkAllocation * ds,
                               gpointer user_data
                              )
{
  AwnEffectsPrivate *priv = fx->priv;

  if (priv->clip) {
    gint m_w, m_h, m_x, m_y;
    double dx, dy;
    /* translate, so the icon will still be painted on bottom even if clipped */
    switch (fx->position) {
      case GTK_POS_TOP:
        dx = priv->clip_region.width - priv->clip_region.x;
        dy = priv->clip_region.height - priv->clip_region.y;

        cairo_translate(cr, -priv->icon_width + dx, -priv->icon_height + dy);
        cairo_rectangle(cr, priv->clip_region.x, priv->icon_height - priv->clip_region.y - priv->clip_region.height,
                        priv->clip_region.width, priv->clip_region.height);
        break;
      case GTK_POS_BOTTOM:
        dx = priv->clip_region.width - priv->clip_region.x;
        dy = priv->clip_region.height - priv->clip_region.y;

        cairo_translate(cr, priv->icon_width - dx, priv->icon_height - dy);
        cairo_rectangle(cr, priv->clip_region.x, priv->clip_region.y,
                        priv->clip_region.width, priv->clip_region.height);
        break;
      /* we have to map the clipping coordinates when using left/right positions */
      case GTK_POS_RIGHT:
        m_w = (float)priv->clip_region.height/priv->icon_height * priv->icon_width;
        m_h = (float)priv->clip_region.width/priv->icon_width * priv->icon_height;
        m_x = (float)priv->clip_region.y/priv->icon_height * priv->icon_width;
        m_y = (float)priv->clip_region.x/priv->icon_width * priv->icon_height;

        dx = m_w - m_x;
        dy = m_h - m_y;

        cairo_translate(cr, priv->icon_width - dx, priv->icon_height - dy);
        cairo_rectangle(cr, m_x, m_y,
                        m_w, m_h);
        break;
      case GTK_POS_LEFT:
        m_w = (float)priv->clip_region.height/priv->icon_height * priv->icon_width;
        m_h = (float)priv->clip_region.width/priv->icon_width * priv->icon_height;
        m_x = (float)priv->clip_region.y/priv->icon_height * priv->icon_width;
        m_y = (float)priv->clip_region.x/priv->icon_width * priv->icon_height;

        dx = m_w - m_x;
        dy = m_h - m_y;

        cairo_translate(cr, -priv->icon_width + dx, -priv->icon_height + dy);
        cairo_rectangle(cr, priv->icon_width - m_x - m_w, m_y,
                        m_w, m_h);
        break;
      default:
        return FALSE;
    }
    cairo_clip(cr);
  }
  return FALSE;
}

gboolean awn_effects_pre_op_scale(AwnEffects * fx,
                               cairo_t * cr,
                               GtkAllocation * ds,
                               gpointer user_data
                              )
{
  AwnEffectsPrivate *priv = fx->priv;

  if (priv->width_mod != 1.0 || priv->height_mod != 1.0)
    switch (fx->position) {
      case GTK_POS_TOP:
      case GTK_POS_BOTTOM:
        cairo_scale(cr, priv->width_mod, priv->height_mod);
        break;
      case GTK_POS_RIGHT:
      case GTK_POS_LEFT:
        cairo_scale(cr, priv->height_mod, priv->width_mod);
        break;
    }
  return FALSE;
}

gboolean awn_effects_pre_op_rotate(AwnEffects * fx,
                               cairo_t * cr,
                               GtkAllocation * ds,
                               gpointer user_data
                              )
{
  AwnEffectsPrivate *priv = fx->priv;

  if (priv->rotate_degrees > 0)
    cairo_rotate(cr, priv->rotate_degrees / 180.0 * M_PI);
  return FALSE;
}

gboolean awn_effects_pre_op_flip(AwnEffects * fx,
                               cairo_t * cr,
                               GtkAllocation * ds,
                               gpointer user_data
                              )
{
  AwnEffectsPrivate *priv = fx->priv;

  /* currently we only have variable for hflip */
  if (priv->flip) {
    cairo_matrix_t matrix;
    switch (fx->position) {
      case GTK_POS_TOP:
      case GTK_POS_BOTTOM:
        cairo_matrix_init(&matrix,
                          -1, 0,
                          0, 1,
                          ds->width, 0);
        break;
      case GTK_POS_RIGHT:
      case GTK_POS_LEFT:
        cairo_matrix_init(&matrix,
                          1, 0,
                          0, -1,
                          0, ds->height);
        break;
    }
    cairo_transform(cr, &matrix);
  }
  return FALSE;
}

gboolean awn_effects_post_op_active(AwnEffects * fx,
                                   cairo_t * cr,
                                   GtkAllocation * ds,
                                   gpointer user_data
                                   )
{
  const int PADDING = AWN_EFFECTS_ACTIVE_RECT_PADDING;
  AwnEffectsPrivate *priv = fx->priv;

  if (fx->is_active || priv->simple_rect) {
    double x,y;
    awn_effects_get_base_coords(fx, &x, &y);
    switch (fx->position)
    {
      case GTK_POS_LEFT:
        x += priv->top_offset;
        break;
      case GTK_POS_RIGHT:
        x -= priv->top_offset;
        break;
      case GTK_POS_TOP:
        y += priv->top_offset;
        break;
      default: /* GTK_POS_BOTTOM: */
        y -= priv->top_offset;
        break;
    }
    cairo_save (cr);
    cairo_set_operator (cr, CAIRO_OPERATOR_DEST_OVER);
    if (!fx->custom_active_icon || priv->simple_rect)
    {
      GtkStyle *style = NULL;
      if (fx->widget) {
        style = gtk_widget_get_style (fx->widget);
      }

      if (priv->simple_rect)
      {
        /* Get the color from style */
        if (style)
          gdk_cairo_set_source_color (cr, &style->bg[GTK_STATE_SELECTED]);
        else
          cairo_set_source_rgba (cr, 1.0, 0.4, 0.0, 1.0);
      }
      else
      {
        if (priv->active_rect_color)
        {
          awn_cairo_set_source_color (cr, priv->active_rect_color);
        }
        else if (style)
        {
          DesktopAgnosticColor *color;
          
          color = desktop_agnostic_color_new (&style->dark[GTK_STATE_ACTIVE],
                                              0.4 * G_MAXUSHORT);
          awn_cairo_set_source_color (cr, color);
          g_object_unref (color);
        }
        else
        {
          cairo_set_source_rgba (cr, 0.0, 0.0, 0.0, 0.3);
        }
      }
      awn_cairo_rounded_rect (cr, x-PADDING, y-PADDING,
                              priv->icon_width+(2*PADDING),
                              priv->icon_height+(2*PADDING),
                              priv->icon_width / 8.0, ROUND_ALL);
      cairo_fill_preserve (cr);

      cairo_set_line_width (cr, 1.0);
      if (priv->active_rect_outline)
      {
        awn_cairo_set_source_color (cr, priv->active_rect_outline);
      }
      else
      {
        cairo_set_source_rgba (cr, 0, 0, 0, 0);
      }
      cairo_stroke (cr);
    }
    else
    {
      /* get the icon surface */
      cairo_surface_t *srfc =
        awn_effects_quark_to_surface(fx, fx->custom_active_icon);
      if (srfc)
      {
        float srfc_width = cairo_image_surface_get_width(srfc);
        float srfc_height = cairo_image_surface_get_height(srfc);

        double w_ratio = (priv->icon_width+(2*PADDING)) / srfc_width;
        double h_ratio = (priv->icon_height+(2*PADDING)) / srfc_height;

        cairo_translate(cr, x-PADDING, y-PADDING);
        cairo_scale(cr, w_ratio, h_ratio);
        cairo_set_source_surface(cr, srfc, 0, 0);
        cairo_paint(cr);
      }
    }
    cairo_restore (cr);

    return TRUE;
  }
  return FALSE;
}

gboolean awn_effects_post_op_arrow(AwnEffects * fx,
                                   cairo_t * cr,
                                   GtkAllocation * ds,
                                   gpointer user_data
                                  )
{
  const int MAX_ARROWS = 3;
  AwnEffectsPrivate *priv = fx->priv;
  int arrows_count = fx->arrows_count > MAX_ARROWS ?
                                      MAX_ARROWS : fx->arrows_count;

  if (arrows_count > 0)
  {
    cairo_surface_t *srfc = NULL;
    gint arrow_w = 0, arrow_h = 0;
    srfc = awn_effects_quark_to_surface(fx, fx->arrow_icon);
    if (srfc) {
      /* get custom surface dimensions */
      arrow_w = cairo_image_surface_get_width(srfc);
      arrow_h = cairo_image_surface_get_height(srfc);
    }

    gdouble x, y, rotation;
    awn_effects_get_base_coords(fx, &x, &y);
    /* get coordinates of bottom center (for BOTTOM) and equivalents */
    switch (fx->position)
    {
      case GTK_POS_TOP:
        x += priv->icon_width / 2.0;
        x += arrow_w / 2.0;
        y -= fx->icon_offset / 1.5;
        y += arrow_h;
        rotation = M_PI;
        break;

      case GTK_POS_LEFT:
        x -= fx->icon_offset / 1.5;
        x += arrow_h;
        y += priv->icon_height / 2.0;
        y -= arrow_w / 2.0;
        rotation = M_PI * 0.5;
        break;

      case GTK_POS_RIGHT:
        x += priv->icon_width;
        x += fx->icon_offset / 1.5;
        x -= arrow_h;
        y += priv->icon_height / 2.0;
        y += arrow_w / 2.0;
        rotation = M_PI * 1.5;
        break;

      default: /* GTK_POS_BOTTOM: */
        x += priv->icon_width / 2.0;
        x -= arrow_w / 2.0;
        y += priv->icon_height;
        y += fx->icon_offset / 1.5;
        y -= arrow_h;
        rotation = 0;
        break;
    }
    cairo_save(cr);

    cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
    cairo_translate(cr, x, y);
    cairo_rotate(cr, rotation);

    if (srfc == NULL)
    {
      cairo_set_line_width (cr, 1.0);

      switch (fx->priv->arrow_type)
      {
        case AWN_ARROW_TYPE_1:
        {
          /* Paint the triangular arrow */
          const double ARROW_SIZE = 6.0;
          paint_arrow_triangle (cr, ARROW_SIZE, arrows_count);
          break;
        }
        case AWN_ARROW_TYPE_2:
        {
          /* Paint the dot */
          const double DOT_RADIUS = 9.0;
          double r = 1.0, g = 1.0, b = 1.0;

          if (priv->dot_color)
          {
            desktop_agnostic_color_get_cairo_color (priv->dot_color,
                                                    &r, &g, &b, NULL);
          }
          else if (fx->widget)
          {
            GdkColor c = gtk_widget_get_style (fx->widget)
                           ->light[GTK_STATE_SELECTED];
            r = c.red / 65535.0; g = c.green / 65535.0; b = c.blue / 65535.0;
          }
          // lighten a bit if necessary
          if (priv->glow_amount > 0.0)
          {
            r = lighten_component (r * 255, priv->glow_amount, FALSE) / 255.0;
            g = lighten_component (g * 255, priv->glow_amount, FALSE) / 255.0;
            b = lighten_component (b * 255, priv->glow_amount, FALSE) / 255.0;
          }
          paint_arrow_dot (cr, DOT_RADIUS, arrows_count, r, g ,b);
          break;
        }
        default:
          // we're here if user isn't using valid png / doesn't want arrows
          break;
      }
    }
    else
    {
      cairo_set_source_surface(cr, srfc, 0, 0);
      cairo_paint(cr);
    }

    cairo_restore(cr);

    return TRUE;
  }

  return FALSE;
}

gboolean awn_effects_post_op_clip(AwnEffects * fx,
                               cairo_t * cr,
                               GtkAllocation * ds,
                               gpointer user_data
                              )
{
  AwnEffectsPrivate *priv = fx->priv;

  /* any previous clipping should be reset by now
   * 4px offset for 3D look for reflection
   */
  if (fx->border_clip) {
    switch (fx->position) {
      case GTK_POS_TOP:
        cairo_rectangle(cr, 0, fx->border_clip, priv->window_width, priv->window_height - fx->border_clip);
        break;
      case GTK_POS_LEFT:
        cairo_rectangle(cr, fx->border_clip, 0, priv->window_width - fx->border_clip, priv->window_height);
        break;
      case GTK_POS_BOTTOM:
        cairo_rectangle(cr, 0, 0, priv->window_width, priv->window_height - fx->border_clip);
        break;
      case GTK_POS_RIGHT:
        cairo_rectangle(cr, 0, 0, priv->window_width - fx->border_clip, priv->window_height);
        break;
      default:
        return FALSE;
    }
    cairo_clip(cr);
  }
  return FALSE;
}

gboolean awn_effects_post_op_saturate(AwnEffects * fx,
                               cairo_t * cr,
                               GtkAllocation * ds,
                               gpointer user_data
                              )
{
  AwnEffectsPrivate *priv = fx->priv;

  if (priv->saturation < 1.0)
  {
    surface_saturate (cairo_get_target(cr), priv->saturation);
    return TRUE;
  }

  return FALSE;
}

gboolean awn_effects_post_op_glow(AwnEffects * fx,
                                  cairo_t * cr,
                                  GtkAllocation * ds,
                                  gpointer user_data)
{
  AwnEffectsPrivate *priv = fx->priv;

  if (priv->glow_amount > 0 || fx->depressed)
  {
    gfloat amount = fx->depressed ? 1 : priv->glow_amount;
    lighten_surface (cairo_get_target (cr),
                     priv->window_width, priv->window_height,
                     amount);
    return TRUE;
  }
  return FALSE;
}

gboolean awn_effects_post_op_depth(AwnEffects * fx,
                               cairo_t * cr,
                               GtkAllocation * ds,
                               gpointer user_data
                              )
{
  AwnEffectsPrivate *priv = fx->priv;

  if (priv->icon_depth) {
    double x = priv->icon_depth;
    x /= priv->icon_depth_direction ? -2 : 2;
    cairo_surface_flush(cairo_get_target(cr));
    /* FIXME: we really could use the GtkAllocation here for optimization
     * copy current surface look into temp one
     */
    cairo_surface_t *srfc = cairo_surface_create_similar(cairo_get_target(cr),
                                                         CAIRO_CONTENT_COLOR_ALPHA,
                                                         priv->window_width,
                                                         priv->window_height
                                                        );
    cairo_t *ctx = cairo_create(srfc);
    cairo_set_operator(ctx, CAIRO_OPERATOR_SOURCE);
    cairo_set_source_surface(ctx, cairo_get_target(cr), 0, 0);
    cairo_paint(ctx);
    cairo_destroy(ctx);
    cairo_surface_flush(srfc);

    gint i, multiplier = priv->icon_depth_direction ? 1 : -1;
    cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
    switch (fx->position) {
      case GTK_POS_TOP:
      case GTK_POS_BOTTOM:
        for (i=1; i < priv->icon_depth; i++) {
          cairo_set_source_surface(cr,
                                   srfc,
                                   x + (multiplier * (priv->icon_depth-i)),
                                   0);
          cairo_paint(cr);
        }
        break;
      case GTK_POS_LEFT:
      case GTK_POS_RIGHT:
        for (i=1; i < priv->icon_depth; i++) {
          cairo_set_source_surface(cr,
                                   srfc,
                                   0,
                                   x + (multiplier * (priv->icon_depth-i)));
          cairo_paint(cr);
        }
        break;
      default:
        return FALSE;
    }

    cairo_surface_destroy(srfc);
    return TRUE;
  }
  return FALSE;
}

gboolean awn_effects_post_op_shadow(AwnEffects * fx,
                               cairo_t * cr,
                               GtkAllocation * ds,
                               gpointer user_data
                              )
{
  AwnEffectsPrivate *priv = fx->priv;

  if (fx->make_shadow) {
    cairo_surface_t * blur_srfc;
    cairo_t * blur_ctx;

    int w = priv->window_width, h = priv->window_height;
    blur_srfc = cairo_surface_create_similar(cairo_get_target(cr),
                                             CAIRO_CONTENT_COLOR_ALPHA,
                                             w,
                                             h);
    blur_ctx = cairo_create (blur_srfc);

    cairo_set_operator(blur_ctx, CAIRO_OPERATOR_SOURCE);
    cairo_set_source_surface(blur_ctx, cairo_get_target(cr), 0, 0);
    cairo_paint(blur_ctx);

    darken_surface (blur_ctx, priv->window_width, priv->window_height);
    blur_surface_shadow (blur_srfc, priv->window_width, priv->window_height, 4);

    cairo_save(cr);
    cairo_set_operator(cr, CAIRO_OPERATOR_DEST_OVER);
    const double SHADOW_SCALE = 1.0234375;
    cairo_scale(cr, SHADOW_SCALE, SHADOW_SCALE);
    cairo_set_source_surface(cr, blur_srfc, (w - w*SHADOW_SCALE)/2, (h - h*SHADOW_SCALE)/2);
    cairo_paint_with_alpha(cr, 0.5);
    cairo_restore(cr);

    cairo_surface_destroy(blur_srfc);
    cairo_destroy(blur_ctx);

    return TRUE;
  }
  return FALSE;
}

gboolean awn_effects_post_op_spotlight(AwnEffects * fx,
                               cairo_t * cr,
                               GtkAllocation * ds,
                               gpointer user_data
                              )
{
  AwnEffectsPrivate *priv = fx->priv;

  if (priv->spotlight && priv->spotlight_alpha > 0) {
    cairo_surface_t *srfc =
      awn_effects_quark_to_surface(fx, fx->spotlight_icon);
    if (!srfc) return FALSE;

    float srfc_width = cairo_image_surface_get_width(srfc);
    float srfc_height = cairo_image_surface_get_height(srfc);

    cairo_save(cr);
    double x, y, rotate=0;
    switch (fx->position) {
      case GTK_POS_TOP:
        x = priv->window_width;
        y = priv->icon_height - priv->icon_height/12;
        y += fx->icon_offset;
        rotate = M_PI;
        break;
      case GTK_POS_RIGHT:
        x = priv->window_width - priv->icon_height + priv->icon_height/12;
        x -= fx->icon_offset;
        y = priv->window_height;
        rotate = -M_PI/2;
        break;
      case GTK_POS_BOTTOM:
        x = 0;
        y = priv->window_height - priv->icon_height + priv->icon_height/12;
        y -= fx->icon_offset;
        break;
      case GTK_POS_LEFT:
        x = priv->icon_height - priv->icon_height/12;
        x += fx->icon_offset;
        y = 0;
        rotate = M_PI/2;
        break;
      default:
        return FALSE;
    }
    cairo_translate(cr, x, y);
    if (fx->position == GTK_POS_TOP ||
        fx->position == GTK_POS_BOTTOM) {
      cairo_scale(cr, priv->window_width / srfc_width,
                      priv->icon_height*5/4 / srfc_height);
    } else {
      cairo_scale(cr, priv->icon_height*5/4 / srfc_height,
                      priv->window_height / srfc_width);
    }
    cairo_rotate(cr, rotate);

    cairo_set_source_surface(cr, srfc, 0, 0);
    cairo_set_operator(cr, CAIRO_OPERATOR_DEST_OVER);
    cairo_paint_with_alpha(cr, priv->spotlight_alpha);
    cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
    cairo_paint_with_alpha(cr, priv->spotlight_alpha/2);

    cairo_restore(cr);
    return TRUE;
  }
  return FALSE;
}

gboolean awn_effects_post_op_alpha(AwnEffects * fx,
                               cairo_t * cr,
                               GtkAllocation * ds,
                               gpointer user_data
                              )
{
  AwnEffectsPrivate *priv = fx->priv;

  if (priv->alpha < 1 || fx->icon_alpha < 1) {
    cairo_save (cr);

    cairo_set_operator (cr, CAIRO_OPERATOR_DEST_OUT);
    cairo_set_source_rgba (cr, 0.0, 0.0, 0.0, 1.0 - priv->alpha*fx->icon_alpha);
    cairo_translate (cr, -0.5, -0.5);
    cairo_rectangle (cr, 0.0, 0.0, priv->window_width, priv->window_height);
    cairo_fill (cr);

    cairo_restore (cr);
    return TRUE;
  }
  return FALSE;
}

gboolean awn_effects_post_op_reflection(AwnEffects * fx,
                               cairo_t * cr,
                               GtkAllocation * ds,
                               gpointer user_data
                              )
{
  AwnEffectsPrivate *priv = fx->priv;

  if (fx->do_reflection) {
    int dx = priv->window_width - fx->icon_offset*2 - fx->refl_offset;
    int dy = priv->window_height - fx->icon_offset*2 - fx->refl_offset;

    cairo_surface_t *srfc = cairo_surface_create_similar(cairo_get_target(cr),
                                                         CAIRO_CONTENT_COLOR_ALPHA,
                                                         priv->window_width,
                                                         priv->window_height
                                                        );
    cairo_t *ctx = cairo_create(srfc);
    cairo_matrix_t matrix;
    switch (fx->position) {
      case GTK_POS_TOP:
        dy *= -1; /* careful no break here */
      case GTK_POS_BOTTOM:
        dx = 0;
        cairo_matrix_init(&matrix,
                          1, 0,
                          0, -1,
                          0, priv->window_height);
        break;
      case GTK_POS_LEFT:
        dx *= -1; /* careful no break here */
      case GTK_POS_RIGHT:
        dy = 0;
        cairo_matrix_init(&matrix,
                          -1, 0,
                          0, 1,
                          priv->window_width, 0);
        break;
    }
    cairo_transform(ctx, &matrix);
    cairo_set_operator(ctx, CAIRO_OPERATOR_SOURCE);
    cairo_set_source_surface(ctx, cairo_get_target(cr), 0, 0);
    cairo_paint(ctx);
    cairo_destroy(ctx);

    cairo_save(cr);

    cairo_set_operator(cr, CAIRO_OPERATOR_DEST_OVER);
    cairo_set_source_surface(cr, srfc, dx, dy);
    cairo_paint_with_alpha(cr, priv->alpha * fx->refl_alpha);
    cairo_restore(cr);

    cairo_surface_destroy(srfc);
    return TRUE;
  }
  return FALSE;
}

gboolean awn_effects_post_op_progress(AwnEffects * fx,
                               cairo_t * cr,
                               GtkAllocation * ds,
                               gpointer user_data
                              )
{
  AwnEffectsPrivate *priv = fx->priv;

  if (fx->progress < 1.0) {
    double dx, dy; /* center coordinates for the pie */
    switch (fx->position) {
      case GTK_POS_TOP:
        dx = priv->window_width / 2.0;
        dy = fx->icon_offset + priv->icon_height / 2.0;
        break;
      case GTK_POS_BOTTOM:
        dx = priv->window_width / 2.0;
        dy = priv->window_height - fx->icon_offset - priv->icon_height / 2.0;
        break;
      case GTK_POS_LEFT:
        dx = fx->icon_offset + priv->icon_width / 2.0;
        dy = priv->window_height / 2.0;
        break;
      case GTK_POS_RIGHT:
        dx = priv->window_width - fx->icon_offset - priv->icon_width / 2.0;
        dy = priv->window_height / 2.0;
        break;
      default:
        return FALSE;
    }

    cairo_set_operator(cr, CAIRO_OPERATOR_OVER);

    double radius = priv->icon_width >= priv->icon_height ?
      priv->icon_height / 2.0 : priv->icon_width / 2.0;
    radius *= 0.7; /* the circle should be smaller than the icon */
    double alpha_mult = priv->alpha * fx->icon_alpha;

    cairo_new_path(cr);
    /* first full circle background */
    cairo_move_to(cr, dx, dy);
    /* FIXME: how to define the colors ??? 
     * add property to effects? read from theme?
     * or read from the managed widget? <- sounds reasonable, but what exactly
     */
    cairo_set_source_rgba(cr, 0.2, 0.2, 0.2, 0.7 * alpha_mult);

    cairo_arc(cr, dx, dy, radius, 0, M_PI*2);
    cairo_fill(cr);

    /* now the pie itself */
    cairo_move_to(cr, dx, dy);
    cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.8 * alpha_mult);
    /* start at 270 degrees (north direction) <=> M_PI * 1.5 */
    cairo_arc(cr, dx, dy, radius*0.85, M_PI*1.5,
              (1.5 + fx->progress*2) * M_PI);
    cairo_fill(cr);

    return TRUE;
  }
  return FALSE;
}

