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

/* scaling */

#include <gtk/gtk.h>
#include <cairo/cairo-xlib.h>
#include "awn-effects-ops.h"

#include <math.h>
#include <string.h>



guchar
lighten_component(const guchar cur_value, const gfloat amount)
{
  int new_value = cur_value;

  if (cur_value < 1) //arbitrary cutoff  FIXME?
  {
    return cur_value;
  }
  
  new_value = new_value + (24 + (new_value >> 3)) * amount;

  if (new_value > 255)
  {
    new_value = 255;
  }

  return (guchar) new_value;
}

/*
 *    FIXME it would be nice to not have to use image surfaces for this effect
 */
static void
lighten_surface(cairo_surface_t * src, const gfloat amount)
{
  int i, j;
  int width, height, row_stride, has_alpha;
  guchar *target_pixels;
  guchar *pixsrc;
  cairo_surface_t * temp_srfc;
  cairo_t         * temp_ctx;

  g_return_if_fail(src);
  has_alpha = TRUE;
  temp_srfc = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,
                                         cairo_xlib_surface_get_width(src),
                                         cairo_xlib_surface_get_height(src)
                                        );
  temp_ctx = cairo_create(temp_srfc);
  cairo_set_operator(temp_ctx,CAIRO_OPERATOR_SOURCE);  
  cairo_set_source_surface(temp_ctx, src, 0, 0);
  cairo_paint(temp_ctx);

  width = cairo_image_surface_get_width(temp_srfc);
  height = cairo_image_surface_get_height(temp_srfc);
  row_stride = cairo_image_surface_get_stride(temp_srfc);
  target_pixels = cairo_image_surface_get_data(temp_srfc);

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

  cairo_destroy(temp_ctx);

  temp_ctx = cairo_create(src);
  cairo_set_operator(temp_ctx,CAIRO_OPERATOR_SOURCE);
  g_assert( cairo_get_operator(temp_ctx) == CAIRO_OPERATOR_SOURCE);
  cairo_set_source_surface(temp_ctx, temp_srfc, 0, 0);
  cairo_paint(temp_ctx);
  cairo_surface_destroy(temp_srfc);
  cairo_destroy(temp_ctx);
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
void
surface_saturate_and_pixelate(cairo_surface_t *src,
                              cairo_surface_t *dest,
                              gfloat saturation,
                              gboolean pixelate)
{
  /* NOTE that src and dest MAY be the same surface! */
  cairo_t *temp_src_ctx;
  cairo_surface_t * temp_src_srfc;
  cairo_t *temp_dest_ctx;
  cairo_surface_t * temp_dest_srfc;

  g_return_if_fail(src);
  g_return_if_fail(dest);
  g_return_if_fail(cairo_xlib_surface_get_height(src) ==
                   cairo_xlib_surface_get_height(dest));
  g_return_if_fail(cairo_xlib_surface_get_width(src) ==
                   cairo_xlib_surface_get_height(dest));

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


static void
surface_saturate(cairo_surface_t * icon_srfc, gfloat saturation)
{
  surface_saturate_and_pixelate(icon_srfc, icon_srfc, saturation, FALSE);
}


/*There are a variety of issues here at the moment*/
static void
apply_3d_illusion(AwnEffects * fx, DrawIconState * ds, const gdouble alpha)
{
  gint i;
  cairo_surface_t * icon_srfc = cairo_get_target(fx->icon_ctx);

  cairo_surface_t * tmp_srfc = cairo_surface_create_similar(icon_srfc,
                             CAIRO_CONTENT_COLOR_ALPHA,
                             fx->icon_width,
                             fx->icon_height);
  cairo_t * tmp_ctx = cairo_create(tmp_srfc);
  cairo_set_source_surface(tmp_ctx,icon_srfc,0,0);
  cairo_set_operator(tmp_ctx,CAIRO_OPERATOR_SOURCE);  
  cairo_paint_with_alpha(tmp_ctx,alpha);  
  
  if (fx->icon_depth + ds->current_width > cairo_xlib_surface_get_width(icon_srfc) )
  {
    cairo_surface_t * replacement = cairo_surface_create_similar(
                           icon_srfc,
                           CAIRO_CONTENT_COLOR_ALPHA,
                           fx->icon_depth + ds->current_width,
                           cairo_xlib_surface_get_height(icon_srfc)
                           );
    cairo_surface_destroy (icon_srfc);
    icon_srfc = replacement;
    cairo_destroy (fx->icon_ctx);    
    fx->icon_ctx = cairo_create(icon_srfc);
    fx->icon_width = cairo_xlib_surface_get_height(icon_srfc);
  }
  
  cairo_save(fx->icon_ctx);
  
  cairo_antialias_t  state = cairo_get_antialias( fx->icon_ctx );
  cairo_set_antialias (fx->icon_ctx,CAIRO_ANTIALIAS_NONE);
  cairo_set_operator(fx->icon_ctx,CAIRO_OPERATOR_CLEAR);  
  cairo_paint(fx->icon_ctx);  
  cairo_set_operator(fx->icon_ctx,CAIRO_OPERATOR_OVER);  

  //FIXME possibly use mask.
    
  for (i = 0; i < fx->icon_depth; i++)  
  {
    if (fx->icon_depth_direction == 0)
    {
      cairo_save(fx->icon_ctx);
      cairo_translate(fx->icon_ctx,i,0);         
      cairo_set_source_surface(fx->icon_ctx, tmp_srfc,  0, 0);      
      cairo_paint(fx->icon_ctx);            
      cairo_restore(fx->icon_ctx);
    }
    else
    {
      cairo_save(fx->icon_ctx);
      cairo_translate(fx->icon_ctx,fx->icon_depth - 1 -i,0);         
      cairo_set_source_surface(fx->icon_ctx, tmp_srfc,  0, 0);      
      cairo_paint(fx->icon_ctx);            
      cairo_restore(fx->icon_ctx);
    }    
  }
  if (fx->icon_depth>1)
  {
    ds->x1 = ds->x1 - fx->icon_depth/2;
  }
  cairo_set_antialias (fx->icon_ctx,state);
  cairo_set_operator(fx->icon_ctx,CAIRO_OPERATOR_SOURCE);
  
  cairo_destroy(tmp_ctx);
  cairo_surface_destroy(tmp_srfc);
  cairo_restore(fx->icon_ctx);
}

//-------------------------------------------------------------------
/*
 This will reuse surfaces if possible.  Create new surfaces if
 ther is none. And destroy and create new surfaces if the size (scaling)
 is changing

 This function in it's original form was relatively clean.  It has gotten a 
 bit nasty in the quest to optimize.
 
 FIXME
 The function name probably should be changed... not sure to what :-)
 */
gboolean awn_effect_op_scale_and_clip(AwnEffects * fx,
                               DrawIconState * ds,
                               cairo_surface_t * icon,
                               cairo_t ** picon_ctx,
                               cairo_t ** preflect_ctx
                              )
{
  gint  x,y,w,h;
  cairo_surface_t * icon_srfc=NULL;
  cairo_surface_t * reflect_srfc=NULL;
  
  if (*picon_ctx)
  {
    icon_srfc=cairo_get_target(*picon_ctx);
  }
  if (*preflect_ctx)
  {
    reflect_srfc=cairo_get_target(*preflect_ctx);
  }

  
  /* clipping */
	x = fx->clip_region.x;
	y = fx->clip_region.y;
	w = fx->clip_region.width;
	h = fx->clip_region.height;    
	if (fx->clip) 
  {		
		if (	x >= 0 && x < fx->icon_width &&
			w-x > 0 && w-x <= fx->icon_width &&
			y >= 0 && x < fx->icon_height &&
			h-y > 0 && h-y <= fx->icon_height) 
    {
			// update current w&h
			ds->current_width = w - x;
			ds->current_height = h - y;
			// adjust offsets
			ds->x1 = (fx->window_width - ds->current_width) / 2;
			ds->y1 += fx->icon_height - ds->current_height;
		} 
  }
  
  
  if (fx->delta_width || fx->delta_height) //is surface size changing?
  {
    // update current w&h
    ds->current_width = ds->current_width + fx->delta_width;
    ds->current_height = ds->current_height + fx->delta_height;

    // adjust offsets
    ds->x1 = (fx->window_width - ds->current_width) / 2;
    ds->y1 = ds->y1 - fx->delta_height;

    //getting rid of the existing surfaces

    if (*picon_ctx)
    {
      cairo_surface_destroy(icon_srfc);
      cairo_destroy(*picon_ctx);
    }

    if (*preflect_ctx)
    {
      cairo_surface_destroy(reflect_srfc);
      cairo_destroy(*preflect_ctx);
    }

    //new surfaces
    icon_srfc = cairo_surface_create_similar(icon,
                  CAIRO_CONTENT_COLOR_ALPHA,
                  ds->current_width,
                  ds->current_height);

    *picon_ctx = cairo_create(icon_srfc);

    reflect_srfc = cairo_surface_create_similar(icon,
                     CAIRO_CONTENT_COLOR_ALPHA,
                     ds->current_width,
                     ds->current_height);

    *preflect_ctx = cairo_create(reflect_srfc);

    /*
     Need to make a high resolution copy of the icon available somehow.
     Probably a fn callback will be implemented as it's rarely needed and
     it's best not to force the applet to provide it all the time especially
     with dynamic icons
     */
    cairo_set_operator(*picon_ctx, CAIRO_OPERATOR_SOURCE);
       
    if (fx->clip)
    {
      cairo_set_source_surface(*picon_ctx, icon, x,y);      
      cairo_rectangle(*picon_ctx,x,y,w-x,h-7);
      cairo_fill(*picon_ctx);
      cairo_save(*picon_ctx);
      cairo_scale(*picon_ctx,
                  ds->current_width  / ((double)ds->current_width - fx->delta_width),
                  ds->current_height / ((double)ds->current_height - fx->delta_height));
      cairo_set_source_surface(*picon_ctx, icon_srfc, 0, 0);      
    }
    else
    {
      cairo_save(*picon_ctx);

      cairo_scale(*picon_ctx,
                  ds->current_width  / ((double)ds->current_width - fx->delta_width),
                  ds->current_height / ((double)ds->current_height - fx->delta_height));

      cairo_set_source_surface(*picon_ctx, icon, 0, 0);
    }    
    cairo_paint(*picon_ctx);

    cairo_restore(*picon_ctx);

    return TRUE;
  }
  else  //everything is "normal" size.
  {
    gboolean up_flag;

    //has the size changed.  probably not
    up_flag = !icon_srfc ? TRUE :
              (cairo_xlib_surface_get_width(icon_srfc) != ds->current_width) ||
              (cairo_xlib_surface_get_height(icon_srfc) != ds->current_height);

    if (up_flag) //if it has then recreate the surface..
    {
      if (icon_srfc)
      {
        cairo_surface_destroy(icon_srfc);
        cairo_destroy(*picon_ctx);
      }

      icon_srfc = cairo_surface_create_similar(icon,
                    CAIRO_CONTENT_COLOR_ALPHA,
                    ds->current_width,
                    ds->current_height);
      *picon_ctx = cairo_create(icon_srfc);
    }
    else
    {
      cairo_set_operator(*picon_ctx, CAIRO_OPERATOR_CLEAR);
      cairo_paint(*picon_ctx);
    }

    up_flag = !reflect_srfc ? TRUE :
              (cairo_xlib_surface_get_width(reflect_srfc) != ds->current_width) ||
              (cairo_xlib_surface_get_height(reflect_srfc) != ds->current_height);

    if (up_flag)
    {
      if (icon_srfc)
      {
        cairo_surface_destroy(reflect_srfc);
        cairo_destroy(*preflect_ctx);
      }

      reflect_srfc = cairo_surface_create_similar(icon,
                       CAIRO_CONTENT_COLOR_ALPHA,
                       ds->current_width,
                       ds->current_height);
      *preflect_ctx = cairo_create(reflect_srfc);
    }
    else
    {
      cairo_set_operator(*preflect_ctx, CAIRO_OPERATOR_CLEAR);
      cairo_paint(*preflect_ctx);
    }

    cairo_set_operator(*picon_ctx, CAIRO_OPERATOR_SOURCE);      
    cairo_set_operator(*preflect_ctx, CAIRO_OPERATOR_SOURCE);

    cairo_set_operator(*picon_ctx, CAIRO_OPERATOR_SOURCE);
    cairo_save(*picon_ctx);
    if (fx->clip)
    {
      cairo_set_source_surface(*picon_ctx, icon, x,y);  
      cairo_rectangle(*picon_ctx,x,y,ds->current_width,ds->current_height);
      cairo_fill(*picon_ctx);
    }
    else
    {
      cairo_set_source_surface(*picon_ctx, icon, 0, 0);
      cairo_paint(*picon_ctx);      
    }

    cairo_restore(*picon_ctx);      
    return FALSE;
  }
}

gboolean awn_effect_op_3dturn(AwnEffects * fx,
                              DrawIconState * ds,                             
                              gpointer null
                             )
{
  /* icon depth */
  if (fx->icon_depth)
  {
    if ( fx->settings->icon_depth_on)
    {
      apply_3d_illusion(fx, ds,  fx->alpha);
    }
    return TRUE;
  }

  return FALSE;
}

//=============================================================

gboolean awn_effect_op_saturate(AwnEffects * fx,
                                DrawIconState * ds,
                                gpointer null
                               )
{
  /* saturation  */
  if (fx->saturation < 1.0)
  {
    surface_saturate(cairo_get_target(fx->icon_ctx), fx->saturation);
    return TRUE;
  }

  return FALSE;
}

gboolean awn_effect_op_hflip(AwnEffects * fx,
                             DrawIconState * ds,
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
    cairo_save(fx->icon_ctx);
    cairo_transform(fx->icon_ctx, &matrix);
    cairo_set_source_surface(fx->icon_ctx, cairo_get_target(fx->icon_ctx), 0, 0);
    cairo_paint(fx->icon_ctx);
    cairo_restore(fx->icon_ctx);
    return TRUE;
  }

  return FALSE;
}




gboolean awn_effect_op_glow(AwnEffects * fx,
                            DrawIconState * ds,
                            gpointer null
                           )
{
  if (fx->glow_amount > 0)
  {
    lighten_surface(cairo_get_target(fx->icon_ctx), fx->glow_amount);
    return TRUE;
  }

  return FALSE;
}


gboolean awn_effect_op_todest(AwnEffects * fx,
                              DrawIconState * ds,
                              SourceToDest *data
                             )
{
  gfloat alpha = 1.0;
  cairo_surface_t * surface = cairo_get_target( fx->icon_ctx);
  cairo_operator_t operator = CAIRO_OPERATOR_SOURCE;

  if (data)
  {
    alpha = data->alpha;
    operator = data->operator;

    if (data->surface)
    {
      surface = data->surface;
    }
  }

  cairo_set_operator(fx->icon_ctx, operator);

  cairo_set_source_surface(fx->icon_ctx, surface, 0, 0);
  cairo_paint_with_alpha(fx->icon_ctx, alpha);
  cairo_set_operator(fx->icon_ctx, CAIRO_OPERATOR_SOURCE);
  return TRUE;
}


//------------------------------------------------------
//possibly create ops for setting cairo source operator...  FIXME ?



//========================================
gboolean
awn_effect_move_x(AwnEffects * fx,
                  DrawIconState * ds,
                  gpointer null
                 )
{
  if (fx->x_offset)
  {
    ds->x1 = ds->x1 + fx->x_offset;
  }

  return FALSE;
}
