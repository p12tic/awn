/*
 *  Copyright (C) 2007 Michal Hruby <michal.mhr@gmail.com>
 *  Copyright (C) 2008 Rodney Cryderman <rcryderman@gmail.com>
 *  Copyright (C) 1999 The Free Software Foundation
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

#include "awn-effects-ops-helpers.h"

void
paint_arrow_triangle (cairo_t *cr, double size, gint count)
{
  switch (count)
  {
    case 3:
      /* one triangle on the left */
      cairo_move_to (cr, -size * 2.75, 0);
      cairo_line_to (cr, -size * 0.75, 0);
      cairo_line_to (cr, -size * 1.75, -size);
      cairo_close_path (cr);

      cairo_set_source_rgba (cr, 1.0, 1.0, 1.0, 0.6);
      cairo_fill_preserve (cr);

      cairo_set_line_width (cr, 0.5);
      cairo_set_source_rgba (cr, 0.0, 0.0, 0.0, 1.0);
      cairo_stroke (cr);

      /* one triangle on the right */
      cairo_move_to (cr, size * 0.75, 0);
      cairo_line_to (cr, size * 2.75, 0);
      cairo_line_to (cr, size * 1.75, -size);
      cairo_close_path (cr);

      cairo_set_source_rgba (cr, 1.0, 1.0, 1.0, 0.6);
      cairo_fill_preserve (cr);

      cairo_set_line_width (cr, 0.5);
      cairo_set_source_rgba (cr, 0.0, 0.0, 0.0, 1.0);
      cairo_stroke (cr);

      cairo_set_line_width (cr, 1.0);
      /* fall through */
    case 1:
      cairo_move_to (cr, -size, 0);
      cairo_line_to (cr, +size, 0);
      cairo_line_to (cr, 0, -size);
      cairo_close_path (cr);

      cairo_set_source_rgba (cr, 1.0, 1.0, 1.0, 0.6);
      cairo_fill_preserve (cr);

      cairo_set_line_width (cr, 0.5);
      cairo_set_source_rgba (cr, 0.0, 0.0, 0.0, 1.0);
      cairo_stroke (cr);
      break;
    case 2:
      cairo_move_to (cr, -size * 1.75, 0);
      cairo_line_to (cr, +size * 1.75, 0);
      cairo_line_to (cr, +size*3/4, -size);
      cairo_line_to (cr, 0.0, -size*1/4);
      cairo_line_to (cr, -size*3/4, -size);
      cairo_close_path (cr);

      cairo_set_source_rgba (cr, 1.0, 1.0, 1.0, 0.6);
      cairo_fill_preserve (cr);

      cairo_set_line_width (cr, 0.5);
      cairo_set_source_rgba (cr, 0.0, 0.0, 0.0, 1.0);
      cairo_stroke (cr);
      break;
  default:
    break;
  }
}

void
paint_arrow_dot (cairo_t *cr, double size, gint count,
                 double r, double g, double b)
{
  cairo_pattern_t *pat = NULL;

  switch (count)
  {
    case 3:
      /* left dot */
      cairo_set_source_rgb (cr, r, g, b);
      cairo_translate (cr, -size / 1.5, 0.0);
      cairo_arc (cr, 0.0, 0.0, size / 6, 0.0, 2*M_PI);
      cairo_fill (cr);

      pat = cairo_pattern_create_radial (0.0, 0.0, 0.01,
                                         0.0, 0.0, size);
      cairo_pattern_add_color_stop_rgba (pat, 0.0, r, g, b, 0.5);
      cairo_pattern_add_color_stop_rgba (pat, 1.0, r, g, b, 0.0);

      cairo_set_source (cr, pat);

      cairo_arc (cr, 0.0, 0.0, size, 0.0, 2*M_PI);
      cairo_fill (cr);
      cairo_pattern_destroy (pat);

      /* right dot */
      cairo_set_source_rgb (cr, r, g, b);
      cairo_translate (cr, +size / 1.5 * 2, 0.0);
      cairo_arc (cr, 0.0, 0.0, size / 6, 0.0, 2*M_PI);
      cairo_fill (cr);

      pat = cairo_pattern_create_radial (0.0, 0.0, 0.01,
                                         0.0, 0.0, size);
      cairo_pattern_add_color_stop_rgba (pat, 0.0, r, g, b, 0.5);
      cairo_pattern_add_color_stop_rgba (pat, 1.0, r, g, b, 0.0);

      cairo_set_source (cr, pat);

      cairo_arc (cr, 0.0, 0.0, size, 0.0, 2*M_PI);
      cairo_fill (cr);
      cairo_pattern_destroy (pat);

      cairo_translate (cr, -size / 1.5, 0.0);
    case 1:
      /* center dot */
      cairo_set_source_rgb (cr, r, g, b);
      cairo_arc (cr, 0.0, 0.0, size / 6, 0.0, 2*M_PI);
      cairo_fill (cr);

      pat = cairo_pattern_create_radial (0.0, 0.0, 0.01,
                                         0.0, 0.0, size);
      cairo_pattern_add_color_stop_rgba (pat, 0.0, r, g, b, 0.5);
      cairo_pattern_add_color_stop_rgba (pat, 1.0, r, g, b, 0.0);

      cairo_set_source (cr, pat);

      cairo_arc (cr, 0.0, 0.0, size, 0.0, 2*M_PI);
      cairo_fill (cr);
      cairo_pattern_destroy (pat);
      break;
    case 2:
      /* left dot */
      cairo_set_source_rgb (cr, r, g, b);
      cairo_translate (cr, -size / 3, 0.0);
      cairo_arc (cr, 0.0, 0.0, size / 6, 0.0, 2*M_PI);
      cairo_fill (cr);

      pat = cairo_pattern_create_radial (0.0, 0.0, 0.01,
                                         0.0, 0.0, size);
      cairo_pattern_add_color_stop_rgba (pat, 0.0, r, g, b, 0.5);
      cairo_pattern_add_color_stop_rgba (pat, 1.0, r, g, b, 0.0);

      cairo_set_source (cr, pat);

      cairo_arc (cr, 0.0, 0.0, size, 0.0, 2*M_PI);
      cairo_fill (cr);
      cairo_pattern_destroy (pat);

      /* right dot */
      cairo_set_source_rgb (cr, r, g, b);
      cairo_translate (cr, +size / 3 * 2, 0.0);
      cairo_arc (cr, 0.0, 0.0, size / 6, 0.0, 2*M_PI);
      cairo_fill (cr);

      pat = cairo_pattern_create_radial (0.0, 0.0, 0.01,
                                         0.0, 0.0, size);
      cairo_pattern_add_color_stop_rgba (pat, 0.0, r, g, b, 0.5);
      cairo_pattern_add_color_stop_rgba (pat, 1.0, r, g, b, 0.0);

      cairo_set_source (cr, pat);

      cairo_arc (cr, 0.0, 0.0, size, 0.0, 2*M_PI);
      cairo_fill (cr);
      cairo_pattern_destroy (pat);
      break;
    default:
      break;
  }
}

guchar
lighten_component(const guchar cur_value, const gfloat amount,
                  gboolean absolute)
{
  int new_value = cur_value;

  if (cur_value < 1) /* arbitrary cutoff  FIXME? */
  {
    return cur_value;
  }
  
  if (absolute)
  {
    new_value = CLAMP (new_value + amount, 0, 255);
  }
  else
  {
    new_value = MIN (new_value + (24 + (new_value >> 3)) * amount, 255);
  }

  return (guchar) new_value;
}

/*
 *    FIXME it would be nice to not have to use image surfaces for this effect
 */
void
lighten_surface (cairo_surface_t * src, 
                 gint surface_width, gint surface_height,
                 const gfloat amount)
{
  cairo_surface_t * temp_srfc;
  cairo_t         * temp_ctx;

  g_return_if_fail (src);

  temp_srfc = cairo_surface_create_similar (src,
                                            CAIRO_CONTENT_COLOR_ALPHA,
                                            surface_width, surface_height);
  temp_ctx = cairo_create (temp_srfc);
  cairo_set_operator (temp_ctx, CAIRO_OPERATOR_SOURCE);
  cairo_set_source_surface (temp_ctx, src, 0, 0);
  cairo_paint (temp_ctx);

/*
  cairo_set_source_rgba (temp_ctx, 1.0, 1.0, 1.0, 
                         CLAMP (amount * 0.1825, 0.0, 1.0));
  cairo_set_operator (temp_ctx, CAIRO_OPERATOR_DEST_IN);
  cairo_rectangle (temp_ctx, 0.0, 0.0, surface_width, surface_height);
  cairo_fill (temp_ctx);
*/
  cairo_destroy (temp_ctx);
  
  temp_ctx = cairo_create (src);

  cairo_set_operator (temp_ctx, CAIRO_OPERATOR_ADD);
  cairo_set_source_surface (temp_ctx, temp_srfc, 0.0, 0.0);

  cairo_paint_with_alpha (temp_ctx, CLAMP (amount * 0.1825, 0.0, 1.0));

  cairo_destroy (temp_ctx);
  cairo_surface_destroy (temp_srfc);
}

void
darken_surface (cairo_t *cr, gint surface_width, gint surface_height)
{
  cairo_set_source_rgba (cr, 0.0, 0.0, 0.0, 1.0);
  cairo_set_operator (cr, CAIRO_OPERATOR_DEST_IN);
  cairo_rectangle (cr, 0.0, 0.0, surface_width, surface_height);
  cairo_fill (cr);
}

void
blur_surface_shadow (cairo_surface_t *src,
                     gint surface_width, gint surface_height, const int radius)
{
  guchar * pixdest, * target_pixels_dest, * target_pixels, * pixsrc;
  cairo_surface_t * temp_srfc, * temp_srfc_dest;
  cairo_t         * temp_ctx, * temp_ctx_dest;

  g_return_if_fail(src);

  /* the original stuff */
  temp_srfc = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
                                          surface_width, surface_height);
  temp_ctx = cairo_create(temp_srfc);
  cairo_set_operator(temp_ctx,CAIRO_OPERATOR_SOURCE);
  cairo_set_source_surface(temp_ctx, src, 0, 0);
  cairo_paint(temp_ctx);

  /* the stuff we draw to */
  temp_srfc_dest = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 
                                              surface_width, surface_height);
  temp_ctx_dest = cairo_create(temp_srfc_dest);
  /* --- */

  cairo_surface_flush(temp_srfc);
  cairo_surface_flush(temp_srfc_dest);

  int row_stride = cairo_image_surface_get_stride(temp_srfc);
  target_pixels = cairo_image_surface_get_data(temp_srfc);
  target_pixels_dest = cairo_image_surface_get_data(temp_srfc_dest);

  /* -- blur the alpha channel (color should be only black anyway) --- */
  /* Implements standard box filter, which is separable so we'll use it to
       speed up the algorithm. */
  int total_a;
  int x, y, kx, ky;
  const int kernel_size = radius * 2 + 1;

  for (y = 0; y < surface_height; ++y)
  {
    for (x = 0; x < surface_width; ++x)
    {
      total_a = 0;

      for (kx = -radius; kx <= radius; ++kx)
      {
        if((x+kx)>0 && (x+kx)<surface_width)
        {
          pixsrc = (target_pixels + y * row_stride) + ((x+kx)*4) + 3;

          total_a += *pixsrc;
        }
      }

      total_a /= kernel_size;

      pixdest = (target_pixels_dest + y * row_stride);
      pixdest += x*4;
      pixdest += 3;
      *pixdest = (guchar) total_a;
    }
  }

  // swap src & dest
  target_pixels_dest = cairo_image_surface_get_data (temp_srfc);
  target_pixels = cairo_image_surface_get_data (temp_srfc_dest);

  for (y = 0; y < surface_height; ++y)
  {
    for (x = 0; x < surface_width; ++x)
    {
      total_a = 0;

      for (ky = -radius; ky <= radius; ++ky)
      {
        if ((y+ky)>0 && (y+ky)<surface_height)
        {
          pixsrc = (target_pixels + (y+ky) * row_stride) + (x*4) + 3;

          total_a += *pixsrc;
        }
      }

      total_a /= kernel_size;

      pixdest = (target_pixels_dest + y * row_stride);
      pixdest += x*4;
      pixdest += 3;
      *pixdest = (guchar) total_a;
    }
  }
  /* ---------- */
  
  cairo_surface_mark_dirty(temp_srfc);

  if (temp_ctx)
  {
    cairo_destroy(temp_ctx);
  }

  temp_ctx = cairo_create (src);
  cairo_set_operator (temp_ctx, CAIRO_OPERATOR_SOURCE);
  g_assert (cairo_get_operator (temp_ctx) == CAIRO_OPERATOR_SOURCE);
  cairo_set_source_surface (temp_ctx, temp_srfc, 0, 0);
  cairo_paint (temp_ctx);
  cairo_surface_destroy (temp_srfc);
  cairo_surface_destroy (temp_srfc_dest);
  cairo_destroy (temp_ctx);
  cairo_destroy (temp_ctx_dest);
}

/**
 *    FIXME it would be nice to not have to use image surfaces for this effect
 *
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
static void
surface_saturate_and_pixelate(cairo_surface_t *src,
                              cairo_surface_t *dest,
                              const gfloat saturation,
                              gboolean pixelate)
{
  /* NOTE that src and dest MAY be the same surface! */
  cairo_t *temp_src_ctx;
  cairo_surface_t * temp_src_srfc;
  cairo_t *temp_dest_ctx;
  cairo_surface_t * temp_dest_srfc;

  // FIXME: cairo_xlib_surface_get_width/height doesn't work correctly
  //   during resizes, pass as param!
  g_return_if_fail(src);
  g_return_if_fail(dest);
  g_return_if_fail(cairo_xlib_surface_get_height(src) ==
                   cairo_xlib_surface_get_height(dest));
  g_return_if_fail(cairo_xlib_surface_get_width(src) ==
                   cairo_xlib_surface_get_width(dest));

  temp_dest_srfc = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,
                   cairo_xlib_surface_get_width(dest),
                   cairo_xlib_surface_get_height(dest)
                                             );
  temp_dest_ctx = cairo_create(temp_dest_srfc);  
  cairo_set_source_surface(temp_dest_ctx, dest, 0, 0);
  cairo_set_operator(temp_dest_ctx,CAIRO_OPERATOR_SOURCE);  
  cairo_paint(temp_dest_ctx);
  cairo_destroy(temp_dest_ctx);

  if (src == dest)
  {
    temp_src_srfc = temp_dest_srfc;
  }
  else
  {
    temp_src_srfc = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,
                    cairo_xlib_surface_get_width(src),
                    cairo_xlib_surface_get_height(src)
                                              );
    temp_src_ctx = cairo_create(temp_src_srfc);
    cairo_set_source_surface(temp_src_ctx, src, 0, 0);
    cairo_set_operator(temp_src_ctx,CAIRO_OPERATOR_SOURCE);      
    cairo_paint(temp_src_ctx);
    cairo_destroy(temp_src_ctx);
  }

  if (saturation == 1.0 && !pixelate)
  {
    if (dest != src)
      memcpy(cairo_image_surface_get_data(temp_dest_srfc),
             cairo_image_surface_get_data(temp_src_srfc),
             cairo_image_surface_get_height(temp_src_srfc) *
             cairo_image_surface_get_stride(temp_src_srfc));
  }
  else
  {
    int i, j, t;
    int width, height;
    int has_alpha, src_rowstride, dest_rowstride, bytes_per_pixel;

    guchar *src_line;
    guchar *dest_line;
    guchar *src_pixel;
    guchar *dest_pixel;
    guchar intensity;

    has_alpha = TRUE;
    bytes_per_pixel = has_alpha ? 4 : 3;
    width = cairo_image_surface_get_width(temp_src_srfc);
    height = cairo_image_surface_get_height(temp_src_srfc);
    src_rowstride = cairo_image_surface_get_stride(temp_src_srfc);
    dest_rowstride = cairo_image_surface_get_stride(temp_dest_srfc);

    src_line = cairo_image_surface_get_data(temp_src_srfc);
    dest_line = cairo_image_surface_get_data(temp_dest_srfc);

#define DARK_FACTOR 0.7
#define INTENSITY(r, g, b) ((r) * 0.30 + (g) * 0.59 + (b) * 0.11)
#define CLAMP_UCHAR(v) (t = (v), CLAMP (t, 0, 255))
#define SATURATE(v) ((1.0 - saturation) * intensity + saturation * (v))

    for (i = 0 ; i < height ; i++)
    {
      src_pixel = src_line;
      src_line = src_line + src_rowstride;
      dest_pixel = dest_line;
      dest_line = dest_line + dest_rowstride;

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

        src_pixel = src_pixel + bytes_per_pixel;

        dest_pixel = dest_pixel + bytes_per_pixel;
      }
    }
  }
  /* ---------- */
  cairo_surface_mark_dirty(temp_dest_srfc);

  cairo_t * tmp;

  tmp = cairo_create(dest);
  cairo_set_operator(tmp,CAIRO_OPERATOR_SOURCE);        
  cairo_set_source_surface(tmp, temp_dest_srfc, 0, 0);
  cairo_paint(tmp);
  cairo_destroy(tmp);

  if (temp_dest_srfc == temp_src_srfc)
  {
    cairo_surface_destroy(temp_dest_srfc);
  }
  else
  {
    cairo_surface_destroy(temp_dest_srfc);
    cairo_surface_destroy(temp_src_srfc);
  }
}


void
surface_saturate(cairo_surface_t * icon_srfc, const gfloat saturation)
{
  surface_saturate_and_pixelate(icon_srfc, icon_srfc, saturation, FALSE);
}

