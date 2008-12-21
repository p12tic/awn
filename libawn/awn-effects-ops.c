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

#include <gtk/gtk.h>
#include <cairo/cairo-xlib.h>
#include "awn-effects-ops.h"
#include "awn-effects-ops-helpers.h"

#include <math.h>
#include <string.h>

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
                               GtkAllocation * ds,
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
			ds->width = w - x;
			ds->height = h - y;
			// adjust offsets
			ds->x = (fx->window_width - ds->width) / 2;
			ds->y += fx->icon_height - ds->height;
		} 
  }
  
  
  if (fx->delta_width || fx->delta_height) //is surface size changing?
  {
    // update current w&h
    ds->width = ds->width + fx->delta_width;
    ds->height = ds->height + fx->delta_height;

    // adjust offsets
    ds->x = (fx->window_width - ds->width) / 2;
    ds->y = ds->y - fx->delta_height;

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
                  ds->width,
                  ds->height);

    *picon_ctx = cairo_create(icon_srfc);

    reflect_srfc = cairo_surface_create_similar(icon,
                     CAIRO_CONTENT_COLOR_ALPHA,
                     ds->width,
                     ds->height);

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
                  ds->width  / ((double)ds->width - fx->delta_width),
                  ds->height / ((double)ds->height - fx->delta_height));
      cairo_set_source_surface(*picon_ctx, icon_srfc, 0, 0);      
    }
    else
    {
      cairo_save(*picon_ctx);

      cairo_scale(*picon_ctx,
                  ds->width  / ((double)ds->width - fx->delta_width),
                  ds->height / ((double)ds->height - fx->delta_height));

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
              (cairo_xlib_surface_get_width(icon_srfc) != ds->width) ||
              (cairo_xlib_surface_get_height(icon_srfc) != ds->height);

    if (up_flag) //if it has then recreate the surface..
    {
      if (icon_srfc)
      {
        cairo_surface_destroy(icon_srfc);
        cairo_destroy(*picon_ctx);
      }

      icon_srfc = cairo_surface_create_similar(icon,
                    CAIRO_CONTENT_COLOR_ALPHA,
                    ds->width,
                    ds->height);
      *picon_ctx = cairo_create(icon_srfc);
    }
    else
    {
      cairo_set_operator(*picon_ctx, CAIRO_OPERATOR_CLEAR);
      cairo_paint(*picon_ctx);
    }

    up_flag = !reflect_srfc ? TRUE :
              (cairo_xlib_surface_get_width(reflect_srfc) != ds->width) ||
              (cairo_xlib_surface_get_height(reflect_srfc) != ds->height);

    if (up_flag)
    {
      if (icon_srfc)
      {
        cairo_surface_destroy(reflect_srfc);
        cairo_destroy(*preflect_ctx);
      }

      reflect_srfc = cairo_surface_create_similar(icon,
                       CAIRO_CONTENT_COLOR_ALPHA,
                       ds->width,
                       ds->height);
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
      cairo_rectangle(*picon_ctx,x,y,ds->width,ds->height);
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
                              GtkAllocation * ds,                             
                              gpointer null
                             )
{
  /* icon depth */
  if (fx->icon_depth)
  {
    apply_3d_illusion(fx, ds,  fx->alpha);
    return TRUE;
  }

  return FALSE;
}

//=============================================================

gboolean awn_effect_op_saturate(AwnEffects * fx,
                                GtkAllocation * ds,
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
                             GtkAllocation * ds,
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
                      (ds->width / 2.0)*(1 - (-1)),
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
                            GtkAllocation * ds,
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
                              GtkAllocation * ds,
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
                  GtkAllocation * ds,
                  gpointer null
                 )
{
  if (fx->side_offset)
  {
    ds->x = ds->x + fx->side_offset;
  }

  return FALSE;
}
