/*
 *  Copyright (C) 2007-2008 Michal Hruby <michal.mhr@gmail.com>
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
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "awn-effects-ops-new.h"
#include "awn-effects-ops-helpers.h"

#ifndef M_PI
 #define  M_PI 3.14159265358979323846
#endif

extern GdkPixbuf *SPOTLIGHT_PIXBUF;

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
  // this should be always the first op executed, will set correct
  // coordinates, so the icon can be painted at [0,0]
  double dx;
  double dy;

  // delta_width/height is swapped when using LEFT/RIGHT orientations
  switch (fx->orientation) {
    case AWN_EFFECT_ORIENT_TOP:
      dx = (fx->window_width - fx->icon_width - fx->delta_width)/2.0 + fx->side_offset;
      dy = fx->top_offset;
      dy += fx->icon_offset;
      break;
    case AWN_EFFECT_ORIENT_RIGHT:
      dx = (fx->window_width - fx->icon_width - fx->delta_height) - fx->top_offset;
      dx -= fx->icon_offset;
      dy = (fx->window_height - fx->icon_height - fx->delta_width)/2.0 - fx->side_offset;
      break;
    case AWN_EFFECT_ORIENT_BOTTOM:
      dx = (fx->window_width - fx->icon_width - fx->delta_width)/2.0 - fx->side_offset;
      dy = (fx->window_height - fx->icon_height - fx->delta_height) - fx->top_offset;
      dy -= fx->icon_offset;
      break;
    case AWN_EFFECT_ORIENT_LEFT:
      dx = fx->top_offset;
      dx += fx->icon_offset;
      dy = (fx->window_height - fx->icon_height - fx->delta_width)/2.0 + fx->side_offset;
      break;
    default:
      return FALSE;
  }

  cairo_translate(cr, (int)dx, (int)dy);
  return FALSE; // ?
}

gboolean awn_effects_pre_op_clip(AwnEffects * fx,
                               cairo_t * cr,
                               GtkAllocation * ds,
                               gpointer user_data
                              )
{
  if (fx->clip) {
    gint m_w, m_h, m_x, m_y;
    double dx, dy;
    // translate, so the icon will still be painted on bottom even if clipped
    switch (fx->orientation) {
      case AWN_EFFECT_ORIENT_TOP:
        dx = fx->clip_region.width - fx->clip_region.x;
        dy = fx->clip_region.height - fx->clip_region.y;

        cairo_translate(cr, -fx->icon_width + dx, -fx->icon_height + dy);
        cairo_rectangle(cr, fx->clip_region.x, fx->icon_height - fx->clip_region.y - fx->clip_region.height,
                        fx->clip_region.width, fx->clip_region.height);
        break;
      case AWN_EFFECT_ORIENT_BOTTOM:
        dx = fx->clip_region.width - fx->clip_region.x;
        dy = fx->clip_region.height - fx->clip_region.y;

        cairo_translate(cr, fx->icon_width - dx, fx->icon_height - dy);
        cairo_rectangle(cr, fx->clip_region.x, fx->clip_region.y,
                        fx->clip_region.width, fx->clip_region.height);
        break;
      // we have to map the clipping coordinates when using left/right orients
      case AWN_EFFECT_ORIENT_RIGHT:
        m_w = (float)fx->clip_region.height/fx->icon_height * fx->icon_width;
        m_h = (float)fx->clip_region.width/fx->icon_width * fx->icon_height;
        m_x = (float)fx->clip_region.y/fx->icon_height * fx->icon_width;
        m_y = (float)fx->clip_region.x/fx->icon_width * fx->icon_height;

        dx = m_w - m_x;
        dy = m_h - m_y;

        cairo_translate(cr, fx->icon_width - dx, fx->icon_height - dy);
        cairo_rectangle(cr, m_x, m_y,
                        m_w, m_h);
        break;
      case AWN_EFFECT_ORIENT_LEFT:
        m_w = (float)fx->clip_region.height/fx->icon_height * fx->icon_width;
        m_h = (float)fx->clip_region.width/fx->icon_width * fx->icon_height;
        m_x = (float)fx->clip_region.y/fx->icon_height * fx->icon_width;
        m_y = (float)fx->clip_region.x/fx->icon_width * fx->icon_height;

        dx = m_w - m_x;
        dy = m_h - m_y;

        cairo_translate(cr, -fx->icon_width + dx, -fx->icon_height + dy);
        cairo_rectangle(cr, fx->icon_width - m_x - m_w, m_y,
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
  if (fx->delta_width || fx->delta_height)
    switch (fx->orientation) {
      case AWN_EFFECT_ORIENT_TOP:
      case AWN_EFFECT_ORIENT_BOTTOM:
        cairo_scale(cr,
          (fx->icon_width + fx->delta_width) / (double)fx->icon_width,
          (fx->icon_height + fx->delta_height) / (double)fx->icon_height);
        break;
      case AWN_EFFECT_ORIENT_RIGHT:
      case AWN_EFFECT_ORIENT_LEFT:
        cairo_scale(cr,
          (fx->icon_width + fx->delta_height) / (double)fx->icon_width,
          (fx->icon_height + fx->delta_width) / (double)fx->icon_height);
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
  if (fx->rotate_degrees > 0)
    cairo_rotate(cr, fx->rotate_degrees / 180.0 * M_PI);
  return FALSE;
}

gboolean awn_effects_pre_op_flip(AwnEffects * fx,
                               cairo_t * cr,
                               GtkAllocation * ds,
                               gpointer user_data
                              )
{
  // currently we only have variable for hflip
  if (fx->flip) {
    cairo_matrix_t matrix;
    switch (fx->orientation) {
      case AWN_EFFECT_ORIENT_TOP:
      case AWN_EFFECT_ORIENT_BOTTOM:
        cairo_matrix_init(&matrix,
                          -1, 0,
                          0, 1,
                          ds->width, 0);
        break;
      case AWN_EFFECT_ORIENT_RIGHT:
      case AWN_EFFECT_ORIENT_LEFT:
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

gboolean awn_effects_post_op_clip(AwnEffects * fx,
                               cairo_t * cr,
                               GtkAllocation * ds,
                               gpointer user_data
                              )
{
  // any previous clipping should be reset by now
  /* 4px offset for 3D look for reflection*/
  if (fx->border_clip) {
    switch (fx->orientation) {
      case AWN_EFFECT_ORIENT_TOP:
        cairo_rectangle(cr, 0, fx->border_clip, fx->window_width, fx->window_height - fx->border_clip);
        break;
      case AWN_EFFECT_ORIENT_LEFT:
        cairo_rectangle(cr, fx->border_clip, 0, fx->window_width - fx->border_clip, fx->window_height);
        break;
      case AWN_EFFECT_ORIENT_BOTTOM:
        cairo_rectangle(cr, 0, 0, fx->window_width, fx->window_height - fx->border_clip);
        break;
      case AWN_EFFECT_ORIENT_RIGHT:
        cairo_rectangle(cr, 0, 0, fx->window_width - fx->border_clip, fx->window_height);
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
  if (fx->saturation < 1.0)
  {
    surface_saturate(cairo_get_target(cr), fx->saturation);
    return TRUE;
  }

  return FALSE;
}

gboolean awn_effects_post_op_glow(AwnEffects * fx,
                               cairo_t * cr,
                               GtkAllocation * ds,
                               gpointer user_data
                              )
{
  if (fx->glow_amount > 0)
  {
    lighten_surface(cairo_get_target(cr), fx->glow_amount);
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
  if (fx->icon_depth) {
    double x = fx->icon_depth;
    x /= fx->icon_depth_direction ? -2 : 2;
    cairo_surface_flush(cairo_get_target(cr));
    // FIXME: we really could use the GtkAllocation here for optimization
    // copy current surface look into temp one
    cairo_surface_t *srfc = cairo_surface_create_similar(cairo_get_target(cr),
                                                         CAIRO_CONTENT_COLOR_ALPHA,
                                                         fx->window_width,
                                                         fx->window_height
                                                        );
    cairo_t *ctx = cairo_create(srfc);
    cairo_set_operator(ctx, CAIRO_OPERATOR_SOURCE);
    cairo_set_source_surface(ctx, cairo_get_target(cr), 0, 0);
    cairo_paint(ctx);
    cairo_destroy(ctx);
    cairo_surface_flush(srfc);

    gint i, multiplier = fx->icon_depth_direction ? 1 : -1;
    cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
    switch (fx->orientation) {
      case AWN_EFFECT_ORIENT_TOP:
      case AWN_EFFECT_ORIENT_BOTTOM:
        for (i=1; i < fx->icon_depth; i++) {
          cairo_set_source_surface(cr,
                                   srfc,
                                   x + (multiplier * (fx->icon_depth-i)),
                                   0);
          cairo_paint(cr);
        }
        break;
      case AWN_EFFECT_ORIENT_LEFT:
      case AWN_EFFECT_ORIENT_RIGHT:
        for (i=1; i < fx->icon_depth; i++) {
          cairo_set_source_surface(cr,
                                   srfc,
                                   0,
                                   x + (multiplier * (fx->icon_depth-i)));
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
  if (fx->make_shadow) {
    cairo_surface_t * blur_srfc;
    cairo_t * blur_ctx;

    int w = fx->window_width, h = fx->window_height;
    blur_srfc = cairo_surface_create_similar(cairo_get_target(cr),
                                             CAIRO_CONTENT_COLOR_ALPHA,
                                             w,
                                             h);
    blur_ctx = cairo_create(blur_srfc);

    cairo_set_operator(blur_ctx, CAIRO_OPERATOR_SOURCE);
    cairo_set_source_surface(blur_ctx, cairo_get_target(cr), 0, 0);
    cairo_paint(blur_ctx);

    darken_surface(blur_srfc);
    blur_surface_shadow(blur_srfc, 4);

    cairo_save(cr);
    cairo_set_operator(cr, CAIRO_OPERATOR_DEST_OVER);
    const double SHADOW_SCALE = 1.0625;
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
  if (fx->spotlight && fx->spotlight_alpha > 0) {
    cairo_save(cr);
    double x, y, rotate=0;
    switch (fx->orientation) {
      case AWN_EFFECT_ORIENT_TOP:
        x = fx->window_width;
        y = fx->icon_height - fx->icon_height/12;
        y += fx->icon_offset;
        rotate = M_PI;
        break;
      case AWN_EFFECT_ORIENT_RIGHT:
        x = fx->window_width - fx->icon_height + fx->icon_height/12;
        x -= fx->icon_offset;
        y = fx->window_height;
        rotate = -M_PI/2;
        break;
      case AWN_EFFECT_ORIENT_BOTTOM:
        x = 0;
        y = fx->window_height - fx->icon_height + fx->icon_height/12;
        y -= fx->icon_offset;
        break;
      case AWN_EFFECT_ORIENT_LEFT:
        x = fx->icon_height - fx->icon_height/12;
        x += fx->icon_offset;
        y = 0;
        rotate = M_PI/2;
        break;
      default:
        return FALSE;
    }
    cairo_translate(cr, x, y);
    if (fx->orientation == AWN_EFFECT_ORIENT_TOP ||
        fx->orientation == AWN_EFFECT_ORIENT_BOTTOM) {
      cairo_scale(cr, fx->window_width / (double) gdk_pixbuf_get_width(SPOTLIGHT_PIXBUF), fx->icon_height*5/4 / (double) gdk_pixbuf_get_height(SPOTLIGHT_PIXBUF));
    } else {
      cairo_scale(cr, fx->icon_height*5/4 / (double) gdk_pixbuf_get_height(SPOTLIGHT_PIXBUF), fx->window_height / (double) gdk_pixbuf_get_width(SPOTLIGHT_PIXBUF));
    }
    cairo_rotate(cr, rotate);

    gdk_cairo_set_source_pixbuf(cr, SPOTLIGHT_PIXBUF, 0, 0);
    cairo_set_operator(cr, CAIRO_OPERATOR_DEST_OVER);
    cairo_paint_with_alpha(cr, fx->spotlight_alpha);
    cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
    cairo_paint_with_alpha(cr, fx->spotlight_alpha/2);

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
  if (fx->alpha < 1 || fx->icon_alpha < 1) {
    cairo_surface_t *srfc = cairo_surface_create_similar(cairo_get_target(cr),
                                                         CAIRO_CONTENT_COLOR_ALPHA,
                                                         fx->window_width,
                                                         fx->window_height
                                                        );
    cairo_t *ctx = cairo_create(srfc);
    cairo_set_operator(ctx, CAIRO_OPERATOR_SOURCE);
    cairo_set_source_surface(ctx, cairo_get_target(cr), 0, 0);
    cairo_paint_with_alpha(ctx, fx->alpha * fx->icon_alpha);
    cairo_destroy(ctx);

    cairo_save(cr);
    cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
    cairo_set_source_surface(cr, srfc, 0, 0);
    cairo_paint(cr);
    cairo_restore(cr);

    cairo_surface_destroy(srfc);
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
  if (fx->do_reflection) {
    int dx = fx->window_width - fx->icon_offset*2 - fx->refl_offset;
    int dy = fx->window_height - fx->icon_offset*2 - fx->refl_offset;

    cairo_surface_t *srfc = cairo_surface_create_similar(cairo_get_target(cr),
                                                         CAIRO_CONTENT_COLOR_ALPHA,
                                                         fx->window_width,
                                                         fx->window_height
                                                        );
    cairo_t *ctx = cairo_create(srfc);
    cairo_matrix_t matrix;
    switch (fx->orientation) {
      case AWN_EFFECT_ORIENT_TOP:
        dy *= -1; // careful no break here
      case AWN_EFFECT_ORIENT_BOTTOM:
        dx = 0;
        cairo_matrix_init(&matrix,
                          1, 0,
                          0, -1,
                          0, fx->window_height);
        break;
      case AWN_EFFECT_ORIENT_LEFT:
        dx *= -1; // careful no break here
      case AWN_EFFECT_ORIENT_RIGHT:
        dy = 0;
        cairo_matrix_init(&matrix,
                          -1, 0,
                          0, 1,
                          fx->window_width, 0);
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
    cairo_paint_with_alpha(cr, fx->alpha * fx->refl_alpha);
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
  if (fx->progress < 1.0) {
    double dx, dy; // center coordinates for the pie
    switch (fx->orientation) {
      case AWN_EFFECT_ORIENT_TOP:
        dx = fx->window_width / 2.0;
        dy = fx->icon_offset + fx->icon_height / 2.0;
        break;
      case AWN_EFFECT_ORIENT_BOTTOM:
        dx = fx->window_width / 2.0;
        dy = fx->window_height - fx->icon_offset - fx->icon_height / 2.0;
        break;
      case AWN_EFFECT_ORIENT_LEFT:
        dx = fx->icon_offset + fx->icon_width / 2.0;
        dy = fx->window_height / 2.0;
        break;
      case AWN_EFFECT_ORIENT_RIGHT:
        dx = fx->window_width - fx->icon_offset - fx->icon_width / 2.0;
        dy = fx->window_height / 2.0;
        break;
      default:
        return FALSE;
    }

    cairo_set_operator(cr, CAIRO_OPERATOR_OVER);

    double radius = fx->icon_width >= fx->icon_height ?
      fx->icon_height / 2.0 : fx->icon_width / 2.0;
    radius *= 0.7; // the circle should be smaller than the icon
    double alpha_mult = fx->alpha * fx->icon_alpha;

    cairo_new_path(cr);
    // first full circle background
    cairo_move_to(cr, dx, dy);
    // FIXME: how to define the colors ???
    //  add property to effects? read from theme?
    //  or read from the managed widget? <- sounds reasonable, but what exactly
    cairo_set_source_rgba(cr, 0.2, 0.2, 0.2, 0.7 * alpha_mult);

    cairo_arc(cr, dx, dy, radius, 0, M_PI*2);
    cairo_fill(cr);

    // now the pie itself
    cairo_move_to(cr, dx, dy);
    cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.8 * alpha_mult);
    // start at 270 degrees (north direction) <=> M_PI * 1.5
    cairo_arc(cr, dx, dy, radius*0.85, M_PI*1.5,
              (1.5 + fx->progress*2) * M_PI);
    cairo_fill(cr);

    return TRUE;
  }
  return FALSE;
}

