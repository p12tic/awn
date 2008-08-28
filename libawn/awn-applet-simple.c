/*
 * Copyright (c) 2007 Neil Jagdish Patel <njpatel@gmail.com>
 * Copyright (C) 2008 Rodney Cryderman <rcryderman@gmail.com>
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif


#include <gtk/gtk.h>
#include <cairo/cairo-xlib.h>

#include "awn-applet.h"
#include "awn-applet-simple.h"
#include "awn-config-client.h"
#include "awn-icons.h"

G_DEFINE_TYPE(AwnAppletSimple, awn_applet_simple, AWN_TYPE_APPLET)

#define AWN_APPLET_SIMPLE_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj),\
    AWN_TYPE_APPLET_SIMPLE, \
    AwnAppletSimplePrivate))

struct _AwnAppletSimplePrivate
{
  GdkPixbuf *org_icon;
  
  /*contemplate doing a union for icon and reflect but instead creating 
  a separate member for the surface vars*/
  GdkPixbuf *icon;
  GdkPixbuf *reflect;
  
  AwnTitle *title;  
  gchar * title_string;
  gboolean  title_visible;
  
  cairo_t * icon_context;
  cairo_t * reflect_context;
  gboolean icon_cxt_copied;

  AwnEffects * effects;

  gint icon_width;
  gint icon_height;

  gint bar_height_on_icon_recieved;

  gint offset;
  gint bar_height;
  gint bar_angle;
  
  AwnIcons  * awn_icons;
  gchar     * current_state;

};

static void
adjust_icon(AwnAppletSimple *simple)
{
  AwnAppletSimplePrivate *priv;
  GdkPixbuf *old0, *old1;
  int refcount = 0;

  priv = simple->priv;

  old0 = priv->icon;
  old1 = priv->reflect;
  if (priv->icon_context)
  {
    cairo_surface_destroy(cairo_get_target(priv->icon_context));
    cairo_destroy(priv->icon_context);
    priv->icon_context = NULL;
  }
  if (priv->reflect_context)
  {
    cairo_surface_destroy(cairo_get_target(priv->reflect_context));
    cairo_destroy(priv->reflect_context);
    priv->reflect_context = NULL;
  }
  
  if (priv->bar_height == priv->bar_height_on_icon_recieved)
  {
    priv->icon_height = gdk_pixbuf_get_height(priv->org_icon);
    priv->icon_width = gdk_pixbuf_get_width(priv->org_icon);
    priv->icon = gdk_pixbuf_copy(priv->org_icon);
  }
  else
  {
    priv->icon_height = gdk_pixbuf_get_height(priv->org_icon) + priv->bar_height - priv->bar_height_on_icon_recieved;
    priv->icon_width = (int)((double)priv->icon_height / (double)gdk_pixbuf_get_height(priv->org_icon) * (double)gdk_pixbuf_get_width(priv->org_icon));
    priv->icon = gdk_pixbuf_scale_simple(priv->org_icon,
                                         priv->icon_width,
                                         priv->icon_height,
                                         GDK_INTERP_BILINEAR);
  }

  g_object_ref(priv->icon);

  priv->reflect = gdk_pixbuf_flip(priv->icon, FALSE);
  g_object_ref(priv->reflect);

  if (old0)
  {
    for (refcount = (G_OBJECT(old0))->ref_count; refcount > 0; refcount--)
    {
      g_object_unref(old0);
    }
  }

  if (old1)
  {
    for (refcount = (G_OBJECT(old1))->ref_count; refcount > 0; refcount--)
    {
      g_object_unref(old1);
    }
  }

  /* for some reason priv->reflect is not always a valid pixbuf.
     my suspicion is that we are seeing a gdk bug here.  So...if
     priv->reflect isn't a good pixbuf, well... let's try making
     one from priv->org_icon */
  if (!GDK_IS_PIXBUF(priv->reflect))
  {
    priv->reflect = gdk_pixbuf_flip(priv->org_icon, FALSE);
  }

  // awn-effects require the window to be 25% bigger than icon
  gtk_widget_set_size_request(GTK_WIDGET(simple),
                              priv->icon_width *5 / 4,
                              (priv->bar_height + 2) * 2);

  gtk_widget_queue_draw(GTK_WIDGET(simple));
}

/**
 * awn_applet_simple_set_icon_context:
 * @simple: The applet whose icon is being set.
 * @cr: The context containing the icon surface.
 *
 * Sets the applet icon to the contexts surface provided as an argument.  The 
 * context is not copied.  Call this function with an updated context
 * before destroying the existing context.  
 *
 * NOTE: at the moment there is probably a memory leak there's a switch between
 * pixbuf method and cairo method of setting icons.  FIXME
 */
void
awn_applet_simple_set_icon_context(AwnAppletSimple *simple,
                                   cairo_t * cr)
{
  AwnAppletSimplePrivate *priv;

  g_return_if_fail(AWN_IS_APPLET_SIMPLE(simple));
  priv = simple->priv;
  if (priv->icon_cxt_copied)
  {
    cairo_surface_destroy(cairo_get_target(priv->icon_context));
    cairo_destroy(priv->icon_context);
    priv->icon_cxt_copied = FALSE;
  }
  else if (priv->icon_context)
  {
    cairo_destroy(priv->icon_context);
  }
           
  if (priv->icon)
  {
    g_object_unref(priv->icon);
    priv->icon = NULL;
  }
  if (priv->reflect)
  {
    g_object_unref(priv->reflect);
    priv->reflect = NULL;    
  }

  
  priv->icon_context=cr;
  cairo_reference(priv->icon_context);

  switch (cairo_surface_get_type(cairo_get_target(cr) ) )
  {
    case CAIRO_SURFACE_TYPE_IMAGE:
	    priv->icon_width = cairo_image_surface_get_width  (cairo_get_target(cr));
	    priv->icon_height = cairo_image_surface_get_height  (cairo_get_target(cr));
      break;       
    case CAIRO_SURFACE_TYPE_XLIB:
	    priv->icon_width = cairo_xlib_surface_get_width  (cairo_get_target(cr));
	    priv->icon_height = cairo_xlib_surface_get_height  (cairo_get_target(cr));
      break;
    default:
      g_assert(FALSE);
  }     
  priv->reflect_context=NULL;  
  
  gtk_widget_set_size_request(GTK_WIDGET(simple),
                              priv->icon_width *5 / 4,
                              (priv->bar_height + 2) * 2);
  gtk_widget_queue_draw(GTK_WIDGET(simple)); 
}

/**
 * awn_applet_simple_set_icon_context_scaled:
 * @simple: The applet whose icon is being set.
 * @cr: The context containing the icon surface.
 *
 * Sets the applet icon to the contexts surface provided as an argument.  The 
 * context is not copied.  Call this function with an updated context
 * before destroying the existing context.  
 *
 * NOTE: at the moment there is probably a memory leak there's a switch between
 * pixbuf method and cairo method of setting icons.  FIXME
 */
void
awn_applet_simple_set_icon_context_scaled(AwnAppletSimple *simple,
                                   cairo_t * cr)
{
  AwnAppletSimplePrivate *priv;

  g_return_if_fail(AWN_IS_APPLET_SIMPLE(simple));
  priv = simple->priv;
  if (priv->icon_cxt_copied)
  {    
    cairo_destroy(priv->icon_context);
    priv->icon_cxt_copied=FALSE;
  }
  else if (priv->icon_context)
  {   
    cairo_destroy(priv->icon_context);   
  }

  if (priv->icon)
  {
    g_object_unref(priv->icon);
    priv->icon = NULL;
  }
  if (priv->reflect)
  {
    g_object_unref(priv->reflect);
    priv->reflect = NULL;    
  }
  
  priv->icon_context=cr;
  cairo_reference(priv->icon_context);
  switch (cairo_surface_get_type(cairo_get_target(cr) ) )
  {
    case CAIRO_SURFACE_TYPE_IMAGE:
	    priv->icon_width = cairo_image_surface_get_width  (cairo_get_target(cr));
	    priv->icon_height = cairo_image_surface_get_height  (cairo_get_target(cr));
      break;       
    case CAIRO_SURFACE_TYPE_XLIB:
	    priv->icon_width = cairo_xlib_surface_get_width  (cairo_get_target(cr));
	    priv->icon_height = cairo_xlib_surface_get_height  (cairo_get_target(cr));
      break;
    default:
      g_assert(FALSE);
  }     
  
  if (priv->icon_height != priv->bar_height)
  {
    cairo_t * new_icon_ctx;
    cairo_surface_t * new_icon_srfc = cairo_surface_create_similar(cairo_get_target(cr),
                                      CAIRO_CONTENT_COLOR_ALPHA,
                                      priv->icon_width * priv->bar_height / priv->icon_height, 
                                      priv->bar_height);

    new_icon_ctx = cairo_create(new_icon_srfc);
    cairo_save(new_icon_ctx);
    cairo_scale(new_icon_ctx,
                priv->bar_height / (double) priv->icon_height ,
                priv->bar_height / (double) priv->icon_height );
    priv->icon_width = priv->icon_width * priv->bar_height / priv->icon_height;
    priv->icon_height = priv->bar_height;    
    cairo_set_source_surface(new_icon_ctx,cairo_get_target(cr),0,0);
    cairo_paint(new_icon_ctx);
    cairo_restore(new_icon_ctx);     
    cairo_destroy(priv->icon_context);    
    priv->icon_context=new_icon_ctx;
    priv->icon_cxt_copied = TRUE;    
  }
  
  priv->reflect_context=NULL;  
  
  gtk_widget_set_size_request(GTK_WIDGET(simple),
                              priv->icon_width *5 / 4,
                              (priv->bar_height + 2) * 2);
  gtk_widget_queue_draw(GTK_WIDGET(simple)); 
}

/**
 * awn_applet_simple_set_icon:
 * @simple: The applet whose icon is being set.
 * @pixbuf: The pixbuf image to use as the icon.
 *
 * Sets the applet icon to the pixbuf provided as an argument.  A private copy
 * of the pixbuf argument is made by awn_applet_simple_set_icon() and the
 * original argument is left unchanged.  The caller retains ownership of pixbuf
 * and is required to unref it when it is no longer required.
 */
void
awn_applet_simple_set_icon(AwnAppletSimple *simple, GdkPixbuf *pixbuf)
{
  g_return_if_fail(GDK_IS_PIXBUF(pixbuf));

  /* awn_applet_simple_set_icon is not heavily used.
     Previous inplementation was causing nasty leaks.
     This fix seems sensible, easy to maintain.
     And it works.  Note we are making a copy here so
     the unref in set_temp_icon leaves the user's original
     untouched.
   */
  awn_applet_simple_set_temp_icon(simple, gdk_pixbuf_copy(pixbuf));
}

/**
 * awn_applet_simple_set_temp_icon:
 * @simple: The applet whose icon is being set.
 * @pixbuf: The pixbuf image to use as the icon.
 *
 * A convenience function that sets the applet icon to the pixbuf provided as an
 * argument.  A private copy of the pixbuf argument is made by the function, and
 * the argument is unreferenced.  The caller should not reference pixbuf after
 * calling this function.  If the pixbuf needs to be retained, then
 * awn_applet_simple_set_icon() should be used.
 */
void
awn_applet_simple_set_temp_icon(AwnAppletSimple *simple, GdkPixbuf *pixbuf)
{
  AwnAppletSimplePrivate *priv;
  GdkPixbuf *old0;
  int refcount;

  g_return_if_fail(AWN_IS_APPLET_SIMPLE(simple));
  g_return_if_fail(GDK_IS_PIXBUF(pixbuf));

  priv = simple->priv;

  /* let's make sure that an applet can't screw around with OUR
     pixbuf.  We'll make our own copy, and free up theirs.
   */
  old0 = pixbuf;
  pixbuf = gdk_pixbuf_copy(pixbuf);
  g_object_unref(old0);

  old0 = priv->org_icon;
  priv->org_icon = pixbuf;
  priv->bar_height_on_icon_recieved = priv->bar_height;

  if (old0)
  {
    for (refcount = (G_OBJECT(old0))->ref_count; refcount > 0; refcount--)
    {
      g_object_unref(old0);
    }
  }

  adjust_icon(simple);
}

/**
 * awn_applet_simple_icon_changed:
 * @awn_icons: the awn-icons object whose icon has changed.
 * @simple: The applet whose icon is being changed.
 *
 * Callback function.  Generated by awn-icons when the icons in use
 * have, or may have, changed.
 */
static void _awn_applet_simple_icon_changed(AwnIcons * awn_icons, AwnAppletSimple *simple)
{
  AwnAppletSimplePrivate *priv; 
  GdkPixbuf * pixbuf;
  priv = simple->priv;  
  pixbuf = awn_icons_get_icon(priv->awn_icons,priv->current_state);
  awn_applet_simple_set_icon(simple,pixbuf);  
  g_object_unref(pixbuf);   
}

/**
 * awn_applet_simple_set_awn_icons:
 * @simple: The applet whose icon is being set.
 * @applet_name: The name of the applet.
 * @uid: The applet uid.
 * @states: A NULL terminated array of pointers to strings representing 
 *          possible icon states.
 * @icon_names: A NULL terminated array of pointers to strings representing 
 *          icon_names that correspond to the states.
 *
 * Sets up a working awn-icons configuration within Applet Simple. Unless 
 * multiple icon states are needed awn_applet_simple_set_awn_icon() should be 
 * used instead.
 *
 * Returns: the currently used pixbuf.  The caller does NOT own a reference
 * to the pixbuf and should reference it if needed.
 */    
GdkPixbuf * awn_applet_simple_set_awn_icons(AwnAppletSimple *simple,
                                    const gchar * applet_name,
                                    const GStrv states,
                                    const GStrv icon_names
                                    )
{
  AwnAppletSimplePrivate *priv;  
  static GdkPixbuf * pixbuf = NULL;
  g_return_val_if_fail(simple,NULL);  
  priv = simple->priv;
  if (pixbuf)
  {
    g_object_unref(pixbuf); 
    pixbuf = NULL;
  }
  if ( !priv->awn_icons)
  {
    priv->awn_icons = awn_icons_new(applet_name);
  }
  awn_icons_set_icons_info(priv->awn_icons,
                              GTK_WIDGET(simple),
                              awn_applet_get_uid(AWN_APPLET(simple)),
                              priv->bar_height,
                              states,
                              icon_names);
  if (priv->current_state)
  {
    g_free(priv->current_state);
  }  
  priv->current_state = g_strdup(states[0]);
  awn_icons_set_changed_cb(priv->awn_icons,(AwnIconsChange)_awn_applet_simple_icon_changed,simple);
  pixbuf = awn_icons_get_icon(priv->awn_icons,states[0]);
  awn_applet_simple_set_icon(simple, pixbuf );
  return pixbuf;
}

/**
 * awn_applet_simple_set_awn_icon:
 * @simple: The applet whose icon is being set.
 * @applet_name: The name of the applet.
 * @uid: The applet uid.
 * @icon_name: Name of the icon to use.
 *
 * Sets up a working awn-icons configuration within Applet Simple.  If mutiple
 * icons/states are need then awn_applet_simple_set_awn_icons() should be used.
 *
 * Returns: the currently used pixbuf.  The caller does NOT own a reference
 * to the pixbuf and should reference it if needed.
 */
GdkPixbuf * awn_applet_simple_set_awn_icon(AwnAppletSimple *simple,
                                    const gchar * applet_name,
                                    const gchar * icon_name)
{
  AwnAppletSimplePrivate *priv;  
  static GdkPixbuf * pixbuf = NULL;  
  g_return_val_if_fail(simple,NULL);
  priv = simple->priv;
  if (pixbuf)
  {
    g_object_unref(pixbuf); 
    pixbuf = NULL;
  }  
  if ( !priv->awn_icons)
  {
    priv->awn_icons = awn_icons_new(applet_name);
  }
  awn_icons_set_icon_info(priv->awn_icons,
                              GTK_WIDGET(simple),                              
                              awn_applet_get_uid(AWN_APPLET(simple)),
                              priv->bar_height,
                              icon_name);
  if (priv->current_state)
  {
    g_free(priv->current_state);
  }
  priv->current_state = g_strdup("__SINGULAR__");
  awn_icons_set_changed_cb(priv->awn_icons,(AwnIconsChange)_awn_applet_simple_icon_changed,simple);  
  pixbuf = awn_icons_get_icon_simple(priv->awn_icons);
  awn_applet_simple_set_icon(simple, pixbuf);  
  return pixbuf;
}

/**
 * awn_applet_simple_set_awn_icon_state:
 * @simple: The applet whose icon is being set.
 * @state: The new icon state.  Must exist in the states that were provided to
 *          awn_applet_simple_set_awn_icons().
 *
 * Change the icon state.  In other words will change the currently displayed
 * icon.
 *
 * Returns: the currently used pixbuf.  The caller does NOT own a reference
 * to the pixbuf and should reference it if needed.
 */
GdkPixbuf * awn_applet_simple_set_awn_icon_state(AwnAppletSimple *simple, 
                                                 const gchar * state)
{
  AwnAppletSimplePrivate *priv;  
  static GdkPixbuf * pixbuf = NULL;  
  g_return_val_if_fail(simple,NULL);  
  priv = simple->priv;
  if (pixbuf)
  {
    g_object_unref(pixbuf); 
    pixbuf = NULL;
  }    
  if (priv->current_state)
  {
    g_free(priv->current_state);
  }
  priv->current_state = g_strdup(state);                                                             
  pixbuf = awn_icons_get_icon(priv->awn_icons,state);
  awn_applet_simple_set_icon(simple,pixbuf);
  return pixbuf;   
}


/**
 * awn_applet_simple_get_awn_icons:
 * @simple: The applet whose icon is being set.
 *
 * Get the AwnIcons object owned by AwnAppletSimple
 *
 * Returns: The AwnIcons object owned by AwnAppletSimple.  The caller does NOT 
 * own a reference to this object.
 */
AwnIcons * awn_applet_simple_get_awn_icons(AwnAppletSimple *simple)
{
  AwnAppletSimplePrivate *priv;  
  g_return_val_if_fail(simple,NULL);  
  priv = simple->priv;
  return priv->awn_icons;
}

/*Adding the ability to start and stop the application of effects.
 The underlying implementation may change at some point... instead of t
 taking the current approach it might end up being best to shut off the 
 timer when effects are off.*/

/*
 * awn_applet_simple_effects_on:
 * @simple: The applet.
 *
 * Enables the applicaton of awn-effects to the applet's icon window.
 */
void awn_applet_simple_effects_on(AwnAppletSimple *simple)
{
  AwnAppletSimplePrivate *priv;
  priv = simple->priv;
  awn_effects_register(priv->effects, GTK_WIDGET(simple));
}

/*
 * awn_applet_simple_effects_off:
 * @simple: The applet.
 *
 * Disables the applicaton of awn-effects to the applet's icon window.
 */
void awn_applet_simple_effects_off(AwnAppletSimple *simple)
{
  AwnAppletSimplePrivate *priv;
  priv = simple->priv; 
  awn_effects_unregister(priv->effects);
}


static gboolean
_expose_event(GtkWidget *widget, GdkEventExpose *expose)
{
  static gboolean done_once=FALSE;
  AwnAppletSimplePrivate *priv;
  cairo_t *cr=NULL;

  gint width, height, bar_height;
  
  priv = AWN_APPLET_SIMPLE(widget)->priv;
  
  /* For some reason, priv->reflect is not always a valid pixbuf.
     my suspicion is that we are seeing a gdk bug here. So... if
     priv->reflect isn't a good pixbuf well.. let's try making one from
     priv->org_icon.  I'm not happy as I'm not exactly sure of the root
     cause of this...  but this does resolve the issue */
  
  width = widget->allocation.width;
  height = widget->allocation.height;
  awn_effects_draw_set_window_size(priv->effects, width, height);
  bar_height = priv->bar_height;
  cr = gdk_cairo_create(widget->window);
  /* task back */
  cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
  cairo_paint(cr);

  cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
  
  awn_effects_draw_background(priv->effects, cr);    
  
  if (!priv->icon_context)  
  {
    cairo_surface_t * srfc = cairo_surface_create_similar(cairo_get_target(cr),
                                            CAIRO_CONTENT_COLOR_ALPHA,
                                            gdk_pixbuf_get_width(priv->icon), 
                                            gdk_pixbuf_get_height(priv->icon));
    priv->icon_context = cairo_create(srfc);  
    //cairo_surface_destroy(srfc);
    gdk_cairo_set_source_pixbuf (priv->icon_context, priv->icon, 0, 0);
    cairo_paint(priv->icon_context);
    
    if ( priv->reflect && GDK_IS_PIXBUF(priv->reflect) )
    {
      srfc = cairo_surface_create_similar(cairo_get_target(cr),
                                        CAIRO_CONTENT_COLOR_ALPHA,
                                        gdk_pixbuf_get_width(priv->reflect), 
                                        gdk_pixbuf_get_height(priv->reflect));      
      priv->reflect_context = cairo_create(srfc);  
      gdk_cairo_set_source_pixbuf (priv->reflect_context, priv->reflect, 0, 0);
      cairo_paint(priv->reflect_context);
      
    }
    //First time through the effects engine may not be properly initialize.
    //so we'll skip the first frame.
    if (! done_once)
    {
      gtk_widget_queue_draw(widget);
      done_once = TRUE;
      return TRUE;
    }
  }
  
  if (priv->icon_context)
  {
    switch (cairo_surface_get_type(cairo_get_target(priv->icon_context) ) )
    {
      case CAIRO_SURFACE_TYPE_IMAGE:
        {

          cairo_t * new_icon_ctx;
          cairo_surface_t * new_icon_srfc = cairo_surface_create_similar(cairo_get_target(cr),
                                            CAIRO_CONTENT_COLOR_ALPHA,
                                            cairo_image_surface_get_width (cairo_get_target(priv->icon_context) ), 
                                            cairo_image_surface_get_height (cairo_get_target(priv->icon_context) ));
          new_icon_ctx = cairo_create(new_icon_srfc);
          cairo_set_source_surface(new_icon_ctx,cairo_get_target(priv->icon_context),0,0);
          cairo_paint(new_icon_ctx);
//          printf("priv->icon_context  refs = %d \n", cairo_get_reference_count(new_icon_ctx));
          cairo_destroy(priv->icon_context);
          priv->icon_context=new_icon_ctx;
          priv->icon_cxt_copied = TRUE;
        }
        break;
      case CAIRO_SURFACE_TYPE_XLIB:
        break;//we're good.
      default:
        g_warning("invalid surface type \n");
        return TRUE;
    }     

    awn_effects_draw_icons_cairo(priv->effects,cr,priv->icon_context,priv->reflect_context);

  }
  awn_effects_draw_foreground(priv->effects, cr);      
  cairo_destroy(cr);
  
  return TRUE;
}

static void
awn_applet_simple_class_init(AwnAppletSimpleClass *klass)
{
  GtkWidgetClass *widget_class;

  widget_class = GTK_WIDGET_CLASS(klass);
  widget_class->expose_event = _expose_event;

  g_type_class_add_private(G_OBJECT_CLASS(klass),
                           sizeof(AwnAppletSimplePrivate));
}

static void
bar_height_changed(AwnConfigClientNotifyEntry *entry, AwnAppletSimple *simple)
{
  AwnAppletSimplePrivate *priv;

  priv = simple->priv;
  priv->bar_height = entry->value.int_val;
  if (priv->icon)
  {
    if(priv->awn_icons)
    {
      awn_icons_set_height(priv->awn_icons,priv->bar_height);
      awn_applet_simple_set_icon(simple,awn_icons_get_icon_simple(priv->awn_icons));      
    }
    else
    {
      adjust_icon(simple);
    }
  }
}

static void
icon_offset_changed(AwnConfigClientNotifyEntry *entry, AwnAppletSimple *simple)
{
  AwnAppletSimplePrivate *priv;

  priv = simple->priv;
  priv->offset = entry->value.int_val;
  gtk_widget_queue_draw(GTK_WIDGET(simple));
}

static void
bar_angle_changed(AwnConfigClientNotifyEntry *entry, AwnAppletSimple *simple)
{
  AwnAppletSimplePrivate *priv;

  priv = simple->priv;
  priv->bar_angle = entry->value.int_val;

  gtk_widget_queue_draw(GTK_WIDGET(simple));
}

void
awn_applet_simple_set_title(AwnAppletSimple *simple,const char * title_string)
{
  g_return_if_fail(AWN_IS_APPLET_SIMPLE(simple));  
  AwnAppletSimplePrivate *priv;
  priv = simple->priv;  
  if (!priv->title)
  {
    priv->title = AWN_TITLE(awn_title_get_default());
  }
  if (priv->title_string)
  {
    g_free(priv->title_string) ;
  }  
  if (title_string)
  {
    priv->title_string = g_strdup(title_string);    
  }
  else
  {
    priv->title_string = NULL;
  }
  if (priv->title_string && priv->title_visible)
  {
    awn_title_show(priv->title, GTK_WIDGET(simple), priv->title_string);
  }  
  
}

static gboolean
awn_applet_simple_on_enter_event(GtkWidget * widget, GdkEventCrossing * event,
                   AwnAppletSimple *simple)
{
  
  AwnAppletSimplePrivate * priv = simple->priv;  
  
  if (priv->title && priv->title_string)
  {
    priv->title_visible = TRUE;
    awn_title_show(priv->title, GTK_WIDGET(simple), priv->title_string);
  }
  return FALSE;
}

static
gboolean awn_applet_simple_hide_title(gpointer data)
{
  AwnAppletSimple *simple = data;
  
  AwnAppletSimplePrivate * priv = simple->priv;    
  
  if (! priv->title_visible)
  {    
    awn_title_hide(priv->title, GTK_WIDGET(simple));  
  }
  return FALSE;  
}

void
awn_applet_simple_set_title_visibility(AwnAppletSimple *simple, gboolean state)
{
  g_return_if_fail(AWN_IS_APPLET_SIMPLE(simple));  
  AwnAppletSimplePrivate *priv;

  priv = simple->priv;  
  priv->title_visible = state;  
  if (priv->title)
  {
    if (priv->title_visible)
    {
      awn_title_show(priv->title, GTK_WIDGET(simple), priv->title_string);
    }
    else
    {
      g_timeout_add(500,awn_applet_simple_hide_title,simple);    
      awn_title_hide(priv->title, GTK_WIDGET(simple));
    }
  }  
}

static gboolean
awn_applet_simple_on_leave_event(GtkWidget * widget, GdkEventCrossing * event,
                   AwnAppletSimple *simple)
{
  AwnAppletSimplePrivate * priv = simple->priv;    

  if (priv->title && priv->title_string)
  {
    priv->title_visible = FALSE;
    g_timeout_add(100,awn_applet_simple_hide_title,simple);
  }
  return FALSE;
}


static void
awn_applet_simple_init(AwnAppletSimple *simple)
{
  AwnAppletSimplePrivate *priv;
  AwnConfigClient *client;

  priv = simple->priv = AWN_APPLET_SIMPLE_GET_PRIVATE(simple);

  priv->icon_cxt_copied = FALSE;  
  priv->icon = NULL;
  priv->org_icon = NULL;
  priv->reflect = NULL;
  priv->icon_height = 0;
  priv->icon_width = 0;
  priv->offset = 0;
  priv->title = NULL;
  priv->title_string = NULL;
  priv->title_visible = FALSE;
  priv->awn_icons = NULL;
  priv->current_state = NULL;

  /*
   * FIXME: AwnEffects is allocated, but never freed -> write custom dispose
   *  and finalize functions + register them in awn_applet_simple_class_init
   */
  priv->effects = awn_effects_new_for_widget(GTK_WIDGET(simple));
  // register hover effects
  awn_effects_register(priv->effects, GTK_WIDGET(simple));
  // start open effect
  awn_effects_start_ex(priv->effects, AWN_EFFECT_OPENING, 0, 0, 1);

  client = awn_config_client_new();
  awn_config_client_ensure_group(client, "bar");
  priv->offset = awn_config_client_get_int(client, "bar", "icon_offset", NULL);
  priv->bar_height = awn_config_client_get_int(client, "bar", "bar_height", NULL);
  priv->bar_angle = awn_config_client_get_int(client, "bar", "bar_angle", NULL);
  awn_config_client_notify_add(client, "bar", "bar_angle",
                               (AwnConfigClientNotifyFunc)bar_angle_changed, simple);
  awn_config_client_notify_add(client, "bar", "bar_height",
                               (AwnConfigClientNotifyFunc)bar_height_changed, simple);
  awn_config_client_notify_add(client, "bar", "icon_offset",
                               (AwnConfigClientNotifyFunc)icon_offset_changed, simple);  
  
  g_signal_connect(GTK_WIDGET(simple), "enter-notify-event",
                    G_CALLBACK(awn_applet_simple_on_enter_event), simple);
  g_signal_connect(GTK_WIDGET(simple), "leave-notify-event",
                    G_CALLBACK(awn_applet_simple_on_leave_event), simple);  
}

/**
 * awn_applet_simple_get_effects:
 * @simple: The applet whose properties are being queried.
 *
 * Retrieves the #AwnEffects object associated with the applet.
 * Returns: a pointer to an #AwnEffects object associated with the
 * applet.  The caller does not own this object.
 */
AwnEffects*
awn_applet_simple_get_effects(AwnAppletSimple *simple)
{
  AwnAppletSimplePrivate *priv = simple->priv;
  return priv->effects;
}

/**
 * awn_applet_simple_new:
 * @uid: The unique identifier of the instance of the applet on the dock.
 * @orient: The orientation of the applet - see #AwnOrientation.
 * @height: The height of the applet.
 *
 * Creates a new #AwnAppletSimple object.  This applet will have awn-effects
 * effects applied to its icon automatically if awn_applet_simple_set_icon() or
 * awn_applet_simple_set_temp_icon() are used to specify the applet icon.
 * Returns: a new instance of an applet.
 */
GtkWidget*
awn_applet_simple_new(const gchar *uid, gint orient, gint height)
{
  AwnAppletSimple *simple;

  simple = g_object_new(AWN_TYPE_APPLET_SIMPLE,
                        "uid", uid,
                        "orient", orient,
                        "height", height,
                        NULL);

  return GTK_WIDGET(simple);
}
