/*
 * Copyright (c) 2007 Neil Jagdish Patel <njpatel@gmail.com>
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

#ifndef __AWN_APPLET_SIMPLE_H__
#define __AWN_APPLET_SIMPLE_H__

#include <gtk/gtk.h>

#include "awn-applet.h"
#include "awn-effects.h"

G_BEGIN_DECLS

#define AWN_TYPE_APPLET_SIMPLE (awn_applet_simple_get_type ())

#define AWN_APPLET_SIMPLE(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
        AWN_TYPE_APPLET_SIMPLE, AwnAppletSimple))

#define AWN_APPLET_SIMPLE_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), \
        AWN_TYPE_APPLET_SIMPLE, AwnAppletSimpleClass))

#define AWN_IS_APPLET_SIMPLE(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
        AWN_TYPE_APPLET_SIMPLE))

#define AWN_IS_APPLET_SIMPLE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), \
        AWN_TYPE_APPLET_SIMPLE))

#define AWN_APPLET_SIMPLE_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), \
        AWN_TYPE_APPLET_SIMPLE, AwnAppletSimpleClass))

typedef struct _AwnAppletSimple AwnAppletSimple;
typedef struct _AwnAppletSimpleClass AwnAppletSimpleClass;
typedef struct _AwnAppletSimplePrivate AwnAppletSimplePrivate;

struct _AwnAppletSimple
{
  AwnApplet       parent;

  AwnAppletSimplePrivate  *priv;
};

struct _AwnAppletSimpleClass
{
  AwnAppletClass parent_class;

  /* padding */
  void (*_simple_applet0) (void);
  void (*_simple_applet1) (void);
  void (*_simple_applet2) (void);
  void (*_simple_applet3) (void);
};

GType         awn_applet_simple_get_type         (void);

GtkWidget *   awn_applet_simple_new              (const gchar *canonical_name,
                                                  const gchar      *uid, 
                                                  gint              panel_id);

void          awn_applet_simple_set_icon_pixbuf  (AwnAppletSimple  *applet,
                                                  GdkPixbuf        *pixbuf);

void          awn_applet_simple_set_icon_context (AwnAppletSimple  *applet, 
                                                  cairo_t          *cr);

void          awn_applet_simple_set_icon_surface (AwnAppletSimple  *applet,
                                                  cairo_surface_t  *surface);

void          awn_applet_simple_set_icon_name    (AwnAppletSimple  *applet,
                                                  const gchar      *applet_name,
                                                  const gchar      *icon_name);
                                    
void          awn_applet_simple_set_icon_info    (AwnAppletSimple  *applet,
                                                  const gchar      *applet_name,
                                                  GStrv            states,
                                                  GStrv            icon_names);
                                    
void          awn_applet_simple_set_icon_state   (AwnAppletSimple  *applet,  
                                                  const gchar      *state);

void          awn_applet_simple_set_tooltip_text (AwnAppletSimple  *applet,
                                                  const gchar      *text);

gchar *       awn_applet_simple_get_tooltip_text (AwnAppletSimple  *applet);

void          awn_applet_simple_set_message      (AwnAppletSimple  *applet,
                                                  const gchar      *message);

gchar *       awn_applet_simple_get_message      (AwnAppletSimple  *applet);

void          awn_applet_simple_set_progress     (AwnAppletSimple  *applet,
                                                  gfloat            progress);

gfloat        awn_applet_simple_get_progress     (AwnAppletSimple  *applet);

GtkWidget *   awn_applet_simple_get_icon         (AwnAppletSimple  *applet);

void          awn_applet_simple_set_effect       (AwnAppletSimple  *applet,
                                                  AwnEffect         effect);

GtkWidget *   awn_applet_simple_create_about_item (const gchar      *copyright,
                                                   AwnAppletLicense  license,
                                                   const gchar      *applet_name,
                                                   const gchar      *version);

G_END_DECLS

#endif

