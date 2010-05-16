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
 *             for the code section to analyze expanders
 *
 */

#include "config.h"

#include <gdk/gdk.h>
#include <libawn/awn-cairo-utils.h>
#include <math.h>

#include "awn-applet-manager.h"
#include "awn-background-lucido.h"
#include "awn-separator.h"

G_DEFINE_TYPE (AwnBackgroundLucido, awn_background_lucido, AWN_TYPE_BACKGROUND_FLAT)

#define AWN_BACKGROUND_LUCIDO_GET_PRIVATE(obj) ( \
    G_TYPE_INSTANCE_GET_PRIVATE (obj, AWN_TYPE_BACKGROUND_LUCIDO, \
                                 AwnBackgroundLucidoPrivate))

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

static void
awn_background_lucido_constructed (GObject *object)
{
  G_OBJECT_CLASS (awn_background_lucido_parent_class)->constructed (object);
}

static void
awn_background_lucido_dispose (GObject *object)
{
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

}

static void
awn_background_lucido_init (AwnBackgroundLucido *bg)
{

}

AwnBackground *
awn_background_lucido_new (DesktopAgnosticConfigClient *client, AwnPanel *panel)
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
    cairo_line_to (cr, xf, yf);
  else
  { /* Oblique */
    gfloat xm = ( *xs + xf ) / 2.0;
    cairo_curve_to (cr, xm, *ys, xm, yf, xf, yf);
  }
  *xs = xf;
  *ys = yf;
}

/* if (expanded)
 *    move_to(0,0)
 * else{
 *    move_to(0,h)
 *    line_from_to(corner_radius,h-5)
 * }
 * foreach ( expander ){
 *    line_from_to(expander)
 *    curve_line_to_change_y_level()
 * }
 * if (no expander found)
 *    use stripe width
 * if (expanded)
 *    move_to(w,currenty)
 * else{
 *    move_to(w-corner_radius,current_y)
 *    line_from_to(w,h)
 * }
 *
 */

static void
_create_path_lucido ( AwnBackground*  bg,
                      GtkPositionType position,
                      cairo_t*        cr,
                      gfloat          y,
                      gfloat          w,
                      gfloat          h,
                      gfloat          stripe, /* %fill of stripe 0.0-1.0 */
                      gfloat          d, /* delta curviness */
                      gfloat          dc,/* delta corner rad*/
                      gfloat          symmetry,
                      gboolean        internal,
                      gboolean        expanded
                      )
{
  cairo_new_path (cr);

  gfloat lx = 0.0;
  gfloat ly = y;
  gfloat y3 = y + h;
  gfloat y2 = y3 - 5;
  stripe = ( w * stripe ); /* now stripe = non-stripe */
  
  if ( stripe>0 )
  {
    if (expanded)
    {
      if (internal)
      {
        lx = stripe * symmetry;
        cairo_move_to (cr, lx, ly);
        _line_from_to (cr, &lx, &ly, lx+d, y2);
        _line_from_to (cr, &lx, &ly, w-stripe*(1-symmetry)-d, y2);
        _line_from_to (cr, &lx, &ly, w-stripe*(1-symmetry), y);
        cairo_close_path(cr);
      }
      else
      {
        ly = y3;
        cairo_move_to (cr, lx, ly);
        _line_from_to (cr, &lx, &ly, lx, y);
        _line_from_to (cr, &lx, &ly, stripe*symmetry, y);
        _line_from_to (cr, &lx, &ly, stripe*symmetry+d, y2);
        _line_from_to (cr, &lx, &ly, w-stripe*(1-symmetry)-d, y2);
        _line_from_to (cr, &lx, &ly, w-stripe*(1-symmetry), y);
        _line_from_to (cr, &lx, &ly, w, y);
        _line_from_to (cr, &lx, &ly, w, y3);
        cairo_close_path(cr);
      }
    }    
    else /* not expanded */
    {
      if (internal)
      {
        lx = stripe * symmetry + dc;
        cairo_move_to (cr, lx, ly);
        _line_from_to (cr, &lx, &ly, lx+d, y2);
        _line_from_to (cr, &lx, &ly, w-stripe*(1-symmetry)-dc-d, y2);
        _line_from_to (cr, &lx, &ly, w-stripe*(1-symmetry)-dc, y);
        cairo_close_path(cr);
      }
      else
      {
        ly = y3;
        cairo_move_to (cr, lx, ly);
        _line_from_to (cr, &lx, &ly, lx+dc, y);
        _line_from_to (cr, &lx, &ly, stripe*symmetry+dc, y);
        _line_from_to (cr, &lx, &ly, stripe*symmetry+d+dc, y2);
        _line_from_to (cr, &lx, &ly, w-stripe*(1-symmetry)-dc-d, y2);
        _line_from_to (cr, &lx, &ly, w-stripe*(1-symmetry)-dc, y);
        _line_from_to (cr, &lx, &ly, w-dc, y);
        _line_from_to (cr, &lx, &ly, w, y3);
        cairo_close_path(cr);
      }
    }
  }
  else /* auto-stripe */
  {
    gint exps_found = 0;
    gfloat curx = 0.0;
    if (expanded)
    {
      if (internal)
      {        
        /* start from top so.. end on top, and then close path */
        AwnAppletManager *manager;
        g_object_get (bg->panel, "applet-manager", &manager, NULL);

        GList *widgets = gtk_container_get_children (GTK_CONTAINER (manager));
        GList *i;

        GtkWidget *widget = NULL;

        for (i = widgets; i; i = i->next)
        {
          widget = GTK_WIDGET (i->data);

          if (!GTK_IS_IMAGE (widget) || AWN_IS_SEPARATOR (widget)) 
          {
            /* if not expander continue */
            continue;
          }          
          /* expander */
          switch (position)
          {
            case GTK_POS_BOTTOM:
            case GTK_POS_TOP:
              curx = widget->allocation.x;
              if (exps_found%2!=0)
                curx += widget->allocation.width;
            break;
            default:
              curx = widget->allocation.y;
              if (exps_found%2!=0)
                curx += widget->allocation.height;
            break;
          }
          if (curx<0)
            continue;
          //curx /= w;
          //fprintf(stderr,"\n %d = %.2f ",exps_found,curx); 
          if (exps_found==0){  
            lx = curx;
            cairo_move_to (cr, lx, ly);
            _line_from_to (cr, &lx, &ly, curx+d, y2);
          }else{
            if (exps_found%2!=0){
              _line_from_to (cr, &lx, &ly, curx-d, y2);
              _line_from_to (cr, &lx, &ly, curx, y);
            }else{
              _line_from_to (cr, &lx, &ly, curx, y);
              _line_from_to (cr, &lx, &ly, curx+d, y2);
            }
          }
          ++exps_found;
        }
       
        _line_from_to (cr, &lx, &ly, w, ly);
       
        if(exps_found%2!=0)
          _line_from_to (cr, &lx, &ly, lx, y);
        cairo_close_path (cr);
      }
      else /* external */
      {        
        /* start from bottom so.. end on bottom, and then close path */
        AwnAppletManager *manager;
        g_object_get (bg->panel, "applet-manager", &manager, NULL);

        GList *widgets = gtk_container_get_children (GTK_CONTAINER (manager));
        GList *i;

        GtkWidget *widget = NULL;
        
        ly = y3;
        cairo_move_to (cr, lx, ly);

        _line_from_to (cr, &lx, &ly, lx, y);

        for (i = widgets; i; i = i->next)
        {
          widget = GTK_WIDGET (i->data);

          if (!GTK_IS_IMAGE (widget) || AWN_IS_SEPARATOR (widget)) 
          {
            /* if not expander continue */
            continue;
          }          
          /* expander */
          switch (position)
          {
            case GTK_POS_BOTTOM:
            case GTK_POS_TOP:
              curx = widget->allocation.x;
              if (exps_found%2!=0)
                curx += widget->allocation.width;
            break;
            default:
              curx = widget->allocation.y;
              if (exps_found%2!=0)
                curx += widget->allocation.height;
            break;
          }
          if (curx<0)
            continue;
          //curx /= w;
          //fprintf(stderr,"\n %d = %.2f ",exps_found,curx); 
          if (exps_found%2!=0){
            _line_from_to (cr, &lx, &ly, curx-d, y2);
            _line_from_to (cr, &lx, &ly, curx, y);
          }else{
            _line_from_to (cr, &lx, &ly, curx, y);
            _line_from_to (cr, &lx, &ly, curx+d, y2);
          }
          ++exps_found;
        }
        
        _line_from_to (cr, &lx, &ly, w, ly);
        _line_from_to (cr, &lx, &ly, lx, y3);

        cairo_close_path (cr);
      }
    }
    else /* not expanded */
    {
      if (internal)
      {
        /* none */
      }
      else
      {
        ly = y3;
        cairo_move_to (cr, lx, ly);
        _line_from_to (cr, &lx, &ly, lx+dc, y);
        _line_from_to (cr, &lx, &ly, w-dc, y);
        _line_from_to (cr, &lx, &ly, w, y3);
        cairo_close_path (cr);
      }
    }
  }
}

static void
draw_top_bottom_background (AwnBackground*  bg,
                           GtkPositionType  position,
                           cairo_t*         cr,
                           gint             width,
                           gint             height)
{
  cairo_pattern_t *pat;
  cairo_pattern_t *pat_hi;

  /* Make sure the bar gets drawn on the 0.5 pixels (for sharp edges) */
  cairo_translate (cr, 0.5, 0.5);
  
  /* Basic set-up */
  cairo_set_line_width (cr, 1.0);
  cairo_set_operator (cr, CAIRO_OPERATOR_OVER);

  if(gtk_widget_is_composited (GTK_WIDGET (bg->panel) ) == FALSE)
  {
    goto paint_lines;
  }

  gboolean expand = FALSE;
  g_object_get (bg->panel, "expand", &expand, NULL);
  
  /* create internal path */
  _create_path_lucido ( bg, position, cr, 0, width, height,
                        bg->stripe_width, bg->curviness,
                        MAX (6, bg->corner_radius * 3 / 4),
                        bg->curves_symmetry, 1, expand);

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
  pat = cairo_pattern_create_linear (0, 0, 0, height);
  awn_cairo_pattern_add_color_stop_color (pat, 0.0, bg->border_color);
  awn_cairo_pattern_add_color_stop_color (pat, 1.0, bg->hilight_color);

  /* Draw the internal background gradient */
  cairo_save (cr);
  cairo_clip_preserve (cr);
  cairo_set_source (cr, pat);
  cairo_paint (cr);
  cairo_restore (cr);
  cairo_pattern_destroy (pat);

  /* Prepare external background gradient*/
  pat = cairo_pattern_create_linear (0, 0, 0, height);
  awn_cairo_pattern_add_color_stop_color (pat, 0.0, bg->g_step_1);
  awn_cairo_pattern_add_color_stop_color (pat, 1.0, bg->g_step_2);
  
  /* create external path */
  _create_path_lucido ( bg, position, cr, 0, width, height,
                        bg->stripe_width, bg->curviness,
                        MAX (6, bg->corner_radius * 3 / 4),
                        bg->curves_symmetry, 0, expand);
                       
  /* Draw the external background  */
  cairo_save (cr);
  cairo_clip (cr);
  cairo_set_source (cr, pat);
  cairo_paint (cr);
  cairo_restore (cr);
  cairo_pattern_destroy (pat);
  
  /* Draw the hi-light */
  pat_hi = cairo_pattern_create_linear (0, 0, 0, (height / 3.0));
  awn_cairo_pattern_add_color_stop_color (pat_hi, 0.0, bg->g_histep_1);
  awn_cairo_pattern_add_color_stop_color (pat_hi, 1.0, bg->g_histep_2);
  cairo_rectangle (cr, 0, 0, width, height / 3.0);
  cairo_set_source (cr, pat_hi);
  cairo_fill (cr);
  cairo_pattern_destroy (pat_hi);
  /**/

  return;
  /* if not composited */
paint_lines:

  /* Internal border */
  awn_cairo_set_source_color (cr, bg->hilight_color);
  cairo_rectangle (cr, 1, 1, width - 3, height + 3);
  cairo_stroke (cr);

  /* External border */
  awn_cairo_set_source_color (cr, bg->border_color);
  cairo_rectangle (cr, 1, 1, width - 1, height + 3);
  cairo_stroke (cr); /**/
}


static
void awn_background_lucido_padding_request (AwnBackground *bg,
    GtkPositionType position,
    guint *padding_top,
    guint *padding_bottom,
    guint *padding_left,
    guint *padding_right)
{
  AWN_BACKGROUND_CLASS (awn_background_lucido_parent_class)->padding_request (
        bg, position, padding_top, padding_bottom, padding_left, padding_right);
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
      cairo_translate (cr, 0, y + height);
      cairo_scale(cr, 1, -1);
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
      cairo_scale(cr, 1, -1);
      break;
    default:
      cairo_translate (cr, x, y);
      break;
  }
  /*
   * Check if we had already do the draw for this settings
   * otherwise we do it now
   */
   
  /* Check expanders positions & sizes changed */
  AwnAppletManager *manager;
  g_object_get (bg->panel, "applet-manager", &manager, NULL);

  GList *widgets = gtk_container_get_children (GTK_CONTAINER (manager));
  GList *i;

  GtkWidget *widget = NULL;
  gint wcheck = 0;
  for (i = widgets; i; i = i->next)
  {
    widget = GTK_WIDGET (i->data);

    if (!GTK_IS_IMAGE (widget) || AWN_IS_SEPARATOR (widget)) 
    {
      /* if not expander continue */
      continue;
    }
    switch (position)
    {
      case GTK_POS_BOTTOM:
      case GTK_POS_TOP:
        wcheck += widget->allocation.x + widget->allocation.width;
      break;
      default:
        wcheck += widget->allocation.y + widget->allocation.height;
      break;
    }
  }
  AwnBackgroundLucido *lbg;
  lbg = AWN_BACKGROUND_LUCIDO (bg);
  if(bg->needs_redraw==1 || lbg->expw!=wcheck || lbg->oldw!=width || lbg->oldh!=height)
  {
    /* DEBUG: fprintf(stderr,"Update bg\n"); */
    bg->needs_redraw = 0;
    lbg->expw = wcheck;
    lbg->oldw = width;
    lbg->oldh = height;
    cairo_surface_destroy(bg->helper_surface);
    bg->helper_surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,
                                                    width, height);
    cairo_t* temp_cr = cairo_create (bg->helper_surface);
    draw_top_bottom_background (bg, position, temp_cr, width, height);
    cairo_destroy (temp_cr);  
  }
  cairo_set_source_surface (cr, bg->helper_surface, 0, 0);
  cairo_paint(cr);
  cairo_restore (cr);
}

static void
awn_background_lucido_get_shape_mask (AwnBackground  *bg,
                                     cairo_t        *cr,
                                     GtkPositionType  position,
                                     GdkRectangle   *area)
{
  AWN_BACKGROUND_CLASS (awn_background_lucido_parent_class)->get_shape_mask (
    bg, cr, position, area);
}

/* vim: set et ts=2 sts=2 sw=2 : */
