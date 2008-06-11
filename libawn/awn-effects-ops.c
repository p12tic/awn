/*
 *  Copyright (C) 2007 Michal Hruby <michal.mhr@gmail.com>
 *  Copyright (C) 2008 Rodney Cryderman <rcryderman@gmail.com>
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

/* scaling */

#include "awn-effects-ops.h"

#include <math.h>
#include <string.h>
#include <gtk/gtk.h>

inline guchar
lighten_component(const guchar cur_value, const gfloat amount)
{
  int new_value = cur_value;

  if (cur_value < 2) //arbitrary cutoff  FIXME?
  {
    return cur_value;
  }

  new_value += (24 + (new_value >> 3)) * amount;

  if (new_value > 255)
  {
    new_value = 255;
  }

  return (guchar) new_value;
}

static void
lighten_surface(cairo_surface_t * src, const gfloat amount)
{
  int i, j;
  int width, height, row_stride, has_alpha;
  guchar *target_pixels;
  guchar *pixsrc;
  
  g_return_if_fail(src);
  has_alpha = TRUE;
  width = cairo_image_surface_get_width(src);
  height = cairo_image_surface_get_height(src);
  row_stride = cairo_image_surface_get_stride(src);
  target_pixels = cairo_image_surface_get_data(src);

  for (i = 0; i < height; i++)
  {
    pixsrc = target_pixels + i * row_stride;

    for (j = 0; j < width; j++)
    {
      *pixsrc = lighten_component(*pixsrc, amount);
      pixsrc++;
      *pixsrc = lighten_component(*pixsrc, amount);
      pixsrc++;
      *pixsrc = lighten_component(*pixsrc, amount);
      pixsrc++;

      if (has_alpha)
        pixsrc++;
    }
  }
}

/**
 * Modified from gdk_pixbuf_saturate_and_pixelate();
 * Original copyright on gdk_pixbuf_saturate_and_pixelate() below
 * Copyright (C) 1999 The Free Software Foundation
 *
 * Authors: Federico Mena-Quintero <federico@gimp.org>
 *          Cody Russell  <bratsche@gnome.org
 *
 * surface_saturate_and_pixelate:
 * @src: source surface
 * @dest: place to write modified version of @src
 * @saturation: saturation factor
 * @pixelate: whether to pixelate
 *
 * Modifies saturation and optionally pixelates @src, placing the result in
 * @dest. @src and @dest may be the same surface with no ill effects.  If
 * @saturation is 1.0 then saturation is not changed. If it's less than 1.0,
 * saturation is reduced (the image turns toward grayscale); if greater than
 * 1.0, saturation is increased (the image gets more vivid colors). If @pixelate
 * is %TRUE, then pixels are faded in a checkerboard pattern to create a
 * pixelated image. @src and @dest must have the same image format, size, and
 * rowstride.
 *
 **/
void
surface_saturate_and_pixelate(cairo_surface_t *src,
                                 cairo_surface_t *dest,
                                 gfloat saturation,
                                 gboolean pixelate)
{
  /* NOTE that src and dest MAY be the same surface! */

  g_return_if_fail(src);
  g_return_if_fail(dest);
  g_return_if_fail(cairo_image_surface_get_height(src) == cairo_image_surface_get_height(dest));
  g_return_if_fail(cairo_image_surface_get_width(src) == cairo_image_surface_get_height(dest));

  if (saturation == 1.0 && !pixelate)
  {
    if (dest != src)
      memcpy(cairo_image_surface_get_data(dest),
             cairo_image_surface_get_data(src),
             cairo_image_surface_get_height(src) * cairo_image_surface_get_stride(src));
  }
  else
  {
    int i, j, t;
    int width, height, has_alpha, src_rowstride, dest_rowstride, bytes_per_pixel;
    guchar *src_line;
    guchar *dest_line;
    guchar *src_pixel;
    guchar *dest_pixel;
    guchar intensity;

    has_alpha = TRUE;
    bytes_per_pixel = has_alpha ? 4 : 3;
    width = cairo_image_surface_get_width(src);
    height = cairo_image_surface_get_height(src);
    src_rowstride = cairo_image_surface_get_stride(src);
    dest_rowstride = cairo_image_surface_get_stride(dest);

    src_line = cairo_image_surface_get_data(src);
    dest_line = cairo_image_surface_get_data(dest);

#define DARK_FACTOR 0.7
#define INTENSITY(r, g, b) ((r) * 0.30 + (g) * 0.59 + (b) * 0.11)
#define CLAMP_UCHAR(v) (t = (v), CLAMP (t, 0, 255))
#define SATURATE(v) ((1.0 - saturation) * intensity + saturation * (v))

    for (i = 0 ; i < height ; i++)
    {
      src_pixel = src_line;
      src_line += src_rowstride;
      dest_pixel = dest_line;
      dest_line += dest_rowstride;

      for (j = 0 ; j < width ; j++)
      {
        intensity = INTENSITY(src_pixel[0], src_pixel[1], src_pixel[2]);

        if (pixelate && (i + j) % 2 == 0)
        {
          dest_pixel[0] = intensity / 2 + 127;
          dest_pixel[1] = intensity / 2 + 127;
          dest_pixel[2] = intensity / 2 + 127;
        }
        else if (pixelate)
        {
          dest_pixel[0] = CLAMP_UCHAR((SATURATE(src_pixel[0])) * DARK_FACTOR);
          dest_pixel[1] = CLAMP_UCHAR((SATURATE(src_pixel[1])) * DARK_FACTOR);
          dest_pixel[2] = CLAMP_UCHAR((SATURATE(src_pixel[2])) * DARK_FACTOR);
        }
        else
        {
          dest_pixel[0] = CLAMP_UCHAR(SATURATE(src_pixel[0]));
          dest_pixel[1] = CLAMP_UCHAR(SATURATE(src_pixel[1]));
          dest_pixel[2] = CLAMP_UCHAR(SATURATE(src_pixel[2]));
        }

        if (has_alpha)
          dest_pixel[3] = src_pixel[3];

        src_pixel += bytes_per_pixel;

        dest_pixel += bytes_per_pixel;
      }
    }
  }
}


//FIXME
static void
surface_saturate(cairo_surface_t * icon_srfc, gfloat saturation)
{
  surface_saturate_and_pixelate(icon_srfc,icon_srfc, saturation, FALSE);
}


/*There are a variety of issues here at the moment*/
static inline void
apply_3d_illusion(AwnEffects * fx, cairo_t * cr, cairo_surface_t * icon_srfc,
                  const gint x, const gint y, const gdouble alpha)
{
  gint i;

  for (i = 1; i < fx->icon_depth; i++)
  {
    if (fx->icon_depth_direction == 0)
      cairo_set_source_surface(cr, icon_srfc, x - fx->icon_depth + i, y);
    else
      cairo_set_source_surface(cr, icon_srfc, x + fx->icon_depth - i, y);

    cairo_paint_with_alpha(cr, alpha);
  }
}

//-------------------------------------------------------------------


gboolean awn_effect_op_scaling(AwnEffects * fx,
                               DrawIconState * ds,
                               cairo_surface_t * icon,
                               cairo_surface_t ** picon_srfc,
                               cairo_t ** picon_ctx,
                               cairo_surface_t ** preflect_srfc,
                               cairo_t ** preflect_ctx
                              )
{
  if (fx->delta_width || fx->delta_height)
  {
    // sanity check
    if (fx->delta_width <= -ds->current_width
        || fx->delta_height <= -ds->current_height)
    {
      // we would display blank icon
      printf("insane\n");
//      return FALSE;  //let the insanity happen!
    }
    
    // update current w&h
    ds->current_width = ds->current_width + fx->delta_width;

    ds->current_height = ds->current_height + fx->delta_height;

    // adjust offsets
    ds->x1 = (fx->window_width - ds->current_width) / 2;

    ds->y1 = ds->y1 - fx->delta_height;

    if (*picon_ctx)
    {
      cairo_surface_destroy(*preflect_srfc);
      cairo_destroy(*preflect_ctx);
    }

    *picon_srfc = cairo_surface_create_similar(icon, CAIRO_CONTENT_COLOR_ALPHA,

                  ds->current_width,
                  ds->current_height);
    *picon_ctx = cairo_create(*picon_srfc);

    *preflect_srfc = cairo_surface_create_similar(icon, CAIRO_CONTENT_COLOR_ALPHA,
                     ds->current_width,
                     ds->current_height);
    *preflect_ctx = cairo_create(*preflect_srfc);

    cairo_set_operator(*picon_ctx, CAIRO_OPERATOR_SOURCE);
    cairo_save(*picon_ctx);
    cairo_scale(*picon_ctx,
                (double)ds->current_width / ((double)ds->current_width - fx->delta_width),
                (double)ds->current_height / ((double)ds->current_height - fx->delta_height));
    cairo_set_source_surface(*picon_ctx, icon, 0, 0);
    cairo_paint(*picon_ctx);
    cairo_restore(*picon_ctx);
    return TRUE;
  }
  else
  {

    gboolean up_flag;
    
    up_flag = !*picon_srfc ? TRUE :
              (cairo_image_surface_get_width(*picon_srfc) != ds->current_width) ||
              (cairo_image_surface_get_height(*picon_srfc) != ds->current_height);
    if (up_flag)
    {
      if (*picon_srfc)
      {
        cairo_surface_destroy(*picon_srfc);
        cairo_destroy(*picon_ctx);
      }
      *picon_srfc = cairo_surface_create_similar(icon, CAIRO_CONTENT_COLOR_ALPHA,

                    ds->current_width, ds->current_height);
      *picon_ctx = cairo_create(*picon_srfc);
    }
      
    up_flag = !*preflect_srfc ? TRUE :
              (cairo_image_surface_get_width(*preflect_srfc) != ds->current_width) ||
              (cairo_image_surface_get_height(*preflect_srfc) != ds->current_height);      
    if (up_flag)
    {
      if (*picon_srfc)
      {
        cairo_surface_destroy(*preflect_srfc);
        cairo_destroy(*preflect_ctx);
      }
      *preflect_srfc = cairo_surface_create_similar(icon, CAIRO_CONTENT_COLOR_ALPHA,
                       ds->current_width,
                       ds->current_height);
      *preflect_ctx = cairo_create(*preflect_srfc);
    }
    else
    {
      cairo_set_operator (*preflect_ctx, CAIRO_OPERATOR_CLEAR); 
      cairo_paint(*preflect_ctx);
    }
    cairo_set_operator(*preflect_ctx, CAIRO_OPERATOR_SOURCE);
    cairo_set_operator(*picon_ctx, CAIRO_OPERATOR_SOURCE);
    cairo_set_source_surface(*picon_ctx, icon, 0, 0);
    cairo_paint(*picon_ctx);
    return FALSE;
  }
}

gboolean awn_effect_op_3dturn(AwnEffects * fx,
                             cairo_t * cr,
                             DrawIconState * ds,
                             cairo_surface_t * icon_srfc,
                             cairo_t * icon_ctx,
                             gpointer null
                            )
{
  /* icon depth */
  if (fx->icon_depth)
  {

    if (!fx->icon_depth_direction)
      ds->x1 += fx->icon_depth / 2;
    else
      ds->x1 -= fx->icon_depth / 2;
    apply_3d_illusion(fx, cr, icon_srfc, ds->x1, ds->y1, fx->alpha);
    return TRUE;
  }

  return FALSE;
}

//=============================================================

gboolean awn_effect_op_saturate(AwnEffects * fx,
                                DrawIconState * ds,
                                cairo_surface_t * icon_srfc,
                                cairo_t * icon_ctx,
                                gpointer null
                               )
{
  /* saturation  */
  if (fx->saturation < 1.0)
  {
//      printf("saturate disabled \n");   //FIXME WHAT USES saturate????
//    saturate(icon_ctx, fx->saturation);
    surface_saturate(icon_srfc, fx->saturation);    
    return TRUE;
  }

  return FALSE;
}

gboolean awn_effect_op_hflip(AwnEffects * fx,
                             DrawIconState * ds,
                             cairo_surface_t * icon_srfc,
                             cairo_t * icon_ctx,
                             gpointer null
                            )
{
  if (fx->flip)
  {
    cairo_matrix_t matrix;
    cairo_matrix_init(&matrix,
                      -1,
                      0,
                      0,
                      1,
                      (ds->current_width / 2.0)*(1 - (-1)),
                      0);

    cairo_transform(icon_ctx, &matrix);
    cairo_set_source_surface(icon_ctx, icon_srfc, 0, 0);
    cairo_paint(icon_ctx);
    return TRUE;
  }

  return FALSE;
}




gboolean awn_effect_op_glow(AwnEffects * fx,
                            DrawIconState * ds,
                            cairo_surface_t * icon_srfc,
                            cairo_t * icon_ctx,
                            gpointer null
                           )
{
  if (fx->glow_amount > 0)
  {
    lighten_surface(icon_srfc, fx->glow_amount);
    return TRUE;
  }

  return FALSE;
}

//========================================
gboolean
awn_effect_move_x(AwnEffects * fx,
                  DrawIconState * ds,
                  cairo_surface_t * icon_srfc,
                  cairo_t * icon_ctx,
                  gpointer null
                 )
{
  if (fx->x_offset)
  {
    ds->x1 += fx->x_offset;
  }

  return FALSE;
}
