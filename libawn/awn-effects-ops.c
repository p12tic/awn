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
#include <gtk/gtk.h>

//Following two functions are here temporarily.....

/*
GdkPixbuf * surface_2_pixbuf( GdkPixbuf * pixbuf, cairo_surface_t * surface)

original code from abiword (http://www.abisource.com/)  go-image.c
Function name:  static void pixbuf_to_cairo (GOImage *image);
Copyright (C) 2004, 2005 Jody Goldberg (jody@gnome.org)

modified by Rodney Cryderman (rcryderman@gmail.com).

Send it a allocated pixbuff and cairo image surface.  the heights and width 
must match.  Both must be ARGB.
will copy from the surface to the pixbuf.

*/

static GdkPixbuf * surface_2_pixbuf( GdkPixbuf * pixbuf, cairo_surface_t * surface)
{
	guint i,j;
	
	guint src_rowstride,dst_rowstride;
	guint src_height, src_width, dst_height, dst_width;
	
	unsigned char *src, *dst;
	guint t;

#define MULT(d,c,a,t) G_STMT_START { t = (a)? c * 255 / a: 0; d = t;} G_STMT_END

	dst = gdk_pixbuf_get_pixels (pixbuf);
	dst_rowstride = gdk_pixbuf_get_rowstride (pixbuf);
	dst_width=gdk_pixbuf_get_width (pixbuf);
	dst_height= gdk_pixbuf_get_height (pixbuf);
	src_width=cairo_image_surface_get_width (surface);
	src_height=cairo_image_surface_get_height (surface);
	src_rowstride=cairo_image_surface_get_stride (surface);
	src = cairo_image_surface_get_data (surface);

  g_return_val_if_fail( src_width == dst_width,NULL  );
  g_return_val_if_fail( src_height == dst_height,NULL  );   
  g_return_val_if_fail( cairo_image_surface_get_format(surface) == CAIRO_FORMAT_ARGB32,NULL);

	for (i = 0; i < dst_height; i++) 
	{
		for (j = 0; j < dst_width; j++) {
#if G_BYTE_ORDER == G_LITTLE_ENDIAN
			MULT(dst[0], src[2], src[3], t);
			MULT(dst[1], src[1], src[3], t);
			MULT(dst[2], src[0], src[3], t);
			dst[3] = src[3];
#else	  
			MULT(dst[3], src[2], src[3], t);
			MULT(dst[2], src[1], src[3], t);
			MULT(dst[1], src[0], src[3], t);
			dst[0] = src[3];
#endif
			src += 4;
			dst += 4;
		}
		dst += dst_rowstride - dst_width * 4;
		src += src_rowstride - src_width * 4;
			
	}
#undef MULT
    return pixbuf;
}


GdkPixbuf * get_pixbuf_from_surface(cairo_surface_t * surface)
{
    GdkPixbuf *pixbuf;
    pixbuf=gdk_pixbuf_new(GDK_COLORSPACE_RGB,TRUE,8,cairo_image_surface_get_width (surface),cairo_image_surface_get_height (surface));    
    g_return_val_if_fail( pixbuf !=NULL,NULL);
    return surface_2_pixbuf(pixbuf,surface);
}

gboolean awn_effect_op_scaling(   AwnEffects * fx,
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
      return FALSE;
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
    
    *picon_srfc = cairo_surface_create_similar(icon,CAIRO_CONTENT_COLOR_ALPHA,
                                         ds->current_width,
                                         ds->current_height);
    *picon_ctx = cairo_create(*picon_srfc);

    *preflect_srfc = cairo_surface_create_similar(icon,CAIRO_CONTENT_COLOR_ALPHA,
                                         ds->current_width,
                                         ds->current_height);
    *preflect_ctx = cairo_create(*preflect_srfc);
        
    cairo_set_operator(*picon_ctx, CAIRO_OPERATOR_SOURCE);    
    cairo_save(*picon_ctx);
    cairo_scale(*picon_ctx, 
                (double)ds->current_width / ((double)ds->current_width - fx->delta_width),
                (double)ds->current_height / ((double)ds->current_height - fx->delta_height));
    cairo_set_source_surface (*picon_ctx,icon,0,0);  
    cairo_paint(*picon_ctx);
    cairo_restore(*picon_ctx);
    return TRUE;
  }
  else
  {  
    
    gboolean up_flag;
    up_flag = !*picon_srfc?TRUE:
            (cairo_image_surface_get_width(*picon_srfc) != ds->current_width) ||
            (cairo_image_surface_get_height(*picon_srfc) != ds->current_height);
    if ( up_flag )
    {
      if (*picon_srfc)
      {
        cairo_surface_destroy(*picon_srfc);
        cairo_destroy(*picon_ctx);
      }
      *picon_srfc = cairo_surface_create_similar(icon,CAIRO_CONTENT_COLOR_ALPHA,
                                           ds->current_width,ds->current_height );
      *picon_ctx = cairo_create(*picon_srfc);
    }
    
    *preflect_srfc = cairo_surface_create_similar(icon,CAIRO_CONTENT_COLOR_ALPHA,
                                         ds->current_width,
                                         ds->current_height);
    *preflect_ctx = cairo_create(*preflect_srfc);    
    cairo_set_operator(*picon_ctx, CAIRO_OPERATOR_SOURCE);    
    cairo_set_source_surface (*picon_ctx,icon,0,0);  
    cairo_paint(*picon_ctx);
    return FALSE;
  }
}

//FIXME
static void
saturate(cairo_t * icon_ctx,gfloat saturation)
{
  GdkPixbuf * pbuf_icon = get_pixbuf_from_surface(cairo_get_target (icon_ctx) );  
  gdk_pixbuf_saturate_and_pixelate (pbuf_icon, pbuf_icon, saturation, FALSE);
  gdk_cairo_set_source_pixbuf (icon_ctx, pbuf_icon, 0, 0);      
  cairo_paint(icon_ctx);  
  g_object_unref(pbuf_icon);
}  


gboolean awn_effect_op_saturate(   AwnEffects * fx,
                                  DrawIconState * ds,
                                  cairo_surface_t * icon_srfc,
                                  cairo_t * icon_ctx
                               )   
{
  /* saturation  */
  if (fx->saturation < 1.0)
  {
//      printf("saturate disabled \n");   //FIXME WHAT USES saturate????
      saturate(icon_ctx,fx->saturation);
      return TRUE;
  }   
  return FALSE;
}

gboolean awn_effect_op_hflip(   AwnEffects * fx,
                                  DrawIconState * ds,
                                  cairo_surface_t * icon_srfc,
                                  cairo_t * icon_ctx
                               )   
{
  if (fx->flip)
  {
    cairo_matrix_t matrix;
    cairo_matrix_init (&matrix,
                      -1,
                       0,
                       0,
                       1,
                       (ds->current_width/2.0)*(1-(-1)),
                       0);

    cairo_transform(icon_ctx,&matrix);    
    cairo_set_source_surface (icon_ctx,icon_srfc,0,0);  
    cairo_paint(icon_ctx);
  }
}
