/*
 * Copyright (C) 2008 Rodney Cryderman <rcryderman@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor Boston, MA 02110-1301,  USA
 */
 
/* awn-icons.c */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "awn-icons.h"

#include <string.h>

#include <glib.h>
#include <glib/gstdio.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <glib/gi18n.h>


G_DEFINE_TYPE (AwnIcons, awn_icons, G_TYPE_OBJECT)

#define AWN_ICONS_THEME_NAME "awn-theme"
#define AWN_ICONS_RESPONSE_CLEAR 1

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), AWN_TYPE_ICONS, AwnIconsPrivate))

typedef struct _AwnIconsPrivate AwnIconsPrivate;

struct _AwnIconsPrivate {
  GtkWidget       *   icon_widget;    //used if Non-NULL for drag and drop support

  GtkIconTheme    *   awn_theme;
  GtkIconTheme    *   override_theme;
  GtkIconTheme    *   app_theme;
  
  AwnIconsChange      icon_change_cb;
  gpointer            icon_change_cb_data;
  GtkWidget       *   scope_radio1;       //this just seems the easiest way to to deal
  GtkWidget       *   scope_radio2;       //with the radio buttons in the dialog.
  GtkWidget       *   scope_radio3;       //also seems wrong.
  GtkWidget       *   combo;
 
  gchar           **  states;
  gchar           **  icon_names;
  gchar           *   applet_name;
  gchar           *   uid;
  gchar           *   icon_dir;
  
  gint                height;
  gint                cur_icon;
  gint                count;
};

typedef struct AwnIconsDialogData
{
  AwnIcons  *awn_icons;
  gchar     *sdata;
}AwnIconsDialogData;

static const GtkTargetEntry drop_types[] =
{
  { "STRING", 0, 0 },
  { "text/plain", 0, 0},
  { "text/uri-list", 0, 0}
};
static const gint n_drop_types = G_N_ELEMENTS(drop_types);


#if !GLIB_CHECK_VERSION(2,16,0) 

/*
 The following licensing and copyrights apply to 
 
 unescape_character(),
 g_uri_unescape_segment(),
 and
 g_uri_unescape_string()
 
GIO - GLib Input, Output and Streaming Library
 * 
 * Copyright (C) 2006-2007 Red Hat, Inc.
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
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Author: Alexander Larsson <alexl@redhat.com>
 */

static int
unescape_character (const char *scanner)
{
  int first_digit;
  int second_digit;

  first_digit = g_ascii_xdigit_value (*scanner++);
  if (first_digit < 0)
    return -1;

  second_digit = g_ascii_xdigit_value (*scanner++);
  if (second_digit < 0)
    return -1;

  return (first_digit << 4) | second_digit;
}


static char *
g_uri_unescape_segment (const char *escaped_string,
			const char *escaped_string_end,
                        const char *illegal_characters)
{
  const char *in;
  char *out, *result;
  gint character;

  if (escaped_string == NULL)
    return NULL;

  if (escaped_string_end == NULL)
    escaped_string_end = escaped_string + strlen (escaped_string);

  result = g_malloc (escaped_string_end - escaped_string + 1);

  out = result;
  for (in = escaped_string; in < escaped_string_end; in++)
    {
      character = *in;

      if (*in == '%')
	{
	  in++;

	  if (escaped_string_end - in < 2)
	    {
	      /* Invalid escaped char (to short) */
	      g_free (result);
	      return NULL;
	    }

	  character = unescape_character (in);

	  /* Check for an illegal character. We consider '\0' illegal here. */
	  if (character <= 0 ||
	      (illegal_characters != NULL &&
	       strchr (illegal_characters, (char)character) != NULL))
	    {
	      g_free (result);
	      return NULL;
	    }

	  in++; /* The other char will be eaten in the loop header */
	}
      *out++ = (char)character;
    }

  *out = '\0';

  return result;
}

/**
 * g_uri_unescape_string:
 * @escaped_string: an escaped string to be unescaped.
 * @illegal_characters: an optional string of illegal characters not to be allowed.
 * 
 * Unescapes a whole escaped string.
 * 
 * If any of the characters in @illegal_characters or the character zero appears
 * as an escaped character in @escaped_string then that is an error and %NULL
 * will be returned. This is useful it you want to avoid for instance having a
 * slash being expanded in an escaped path element, which might confuse pathname
 * handling.
 *
 * Returns: an unescaped version of @escaped_string. The returned string 
 * should be freed when no longer needed.
 *
 * Since: 2.16
 **/
static char *
g_uri_unescape_string (const char *escaped_string,
		       const char *illegal_characters)
{
  return g_uri_unescape_segment (escaped_string, NULL, illegal_characters);
}

#endif


static void 
awn_icons_file_cleanup(AwnIconsPrivate *priv, int scope)
{
  gchar *filename=NULL;  
  gint  cur_icon = priv->cur_icon;   
  switch(scope)
  {
    case 0:
      filename=g_strdup_printf("%s/awn-theme/scalable/%s.svg",priv->icon_dir,priv->icon_names[cur_icon]);
      g_unlink(filename); 
      g_free(filename);  
      filename=g_strdup_printf("%s/awn-theme/scalable/%s.png",priv->icon_dir,priv->icon_names[cur_icon]);
      g_unlink(filename); 
      g_free(filename);                
      filename=g_strdup_printf("%s/awn-theme/scalable/%s.xpm",priv->icon_dir,priv->icon_names[cur_icon]);
      g_unlink(filename); 
      g_free(filename);                                                     
    case 1:
    //FIXME put this stuff in a loop though an array { "png","xpm","svg"}
      filename=g_strdup_printf("%s/awn-theme/scalable/%s-%s.svg",
                         priv->icon_dir,
                         priv->icon_names[cur_icon],
                         priv->applet_name);          
      g_unlink(filename); 
      g_free(filename);        
      filename=g_strdup_printf("%s/awn-theme/scalable/%s-%s.png",
                         priv->icon_dir,
                         priv->icon_names[cur_icon],
                         priv->applet_name);          
      g_unlink(filename); 
      g_free(filename);           
      filename=g_strdup_printf("%s/awn-theme/scalable/%s-%s.xpm",
                         priv->icon_dir,
                         priv->icon_names[cur_icon],
                         priv->applet_name);          
      g_unlink(filename); 
      g_free(filename);                 
    case 2:
      filename=g_strdup_printf("%s/awn-theme/scalable/%s-%s-%s.svg",
                         priv->icon_dir,                                     
                         priv->icon_names[cur_icon],
                         priv->applet_name,
                         priv->uid);
      g_unlink(filename);
      g_free(filename);
      filename=g_strdup_printf("%s/awn-theme/scalable/%s-%s-%s.png",
                         priv->icon_dir,                                     
                         priv->icon_names[cur_icon],
                         priv->applet_name,
                         priv->uid);
      g_unlink(filename);
      g_free(filename);            
      filename=g_strdup_printf("%s/awn-theme/scalable/%s-%s-%s.xpm",
                         priv->icon_dir,                                     
                         priv->icon_names[cur_icon],
                         priv->applet_name,
                         priv->uid);
      g_unlink(filename);
      g_free(filename);            
      
  }      
}

void _awn_icons_dialog_response(GtkDialog *dialog,
                                  gint       response,
                                  AwnIconsDialogData * dialog_data) 
{
  gchar * contents;
  gsize length;
  gint  scope = 2;
  GError *err=NULL;  
  gchar * sdata = dialog_data->sdata;
  AwnIconsPrivate *priv=GET_PRIVATE(dialog_data->awn_icons);    
  
  if (response == GTK_RESPONSE_ACCEPT)
  {
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(priv->scope_radio1) ))
    {
      scope = 2;  
    }
    else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(priv->scope_radio2) ))
    {
      scope = 1;
    }
    else
    {
      scope = 0;
    }
    
    if (g_file_get_contents (sdata,&contents,&length,&err))
    {
      gchar * new_basename;
      gint  cur_icon = priv->cur_icon;      
      GdkPixbufFormat* pixbuf_info = gdk_pixbuf_get_file_info (sdata,NULL,NULL);
      g_return_if_fail(pixbuf_info);
      gchar ** extensions = gdk_pixbuf_format_get_extensions(pixbuf_info);
      
      if (priv->combo)
      {
        cur_icon=gtk_combo_box_get_active (GTK_COMBO_BOX(priv->combo));
      }
      
      switch (scope)
      {
        case  0:
          new_basename=g_strdup_printf("%s.%s",priv->icon_names[cur_icon],
                                       extensions[0] );
          break;
        case  1:
          new_basename=g_strdup_printf("%s-%s.%s",
                                       priv->icon_names[cur_icon],
                                       priv->applet_name,
                                       extensions[0]);
          break;
        case  2:
        default:  //supress a warning.
          new_basename=g_strdup_printf("%s-%s-%s.%s",
                                       priv->icon_names[cur_icon],
                                       priv->applet_name,
                                       priv->uid,
                                       extensions[0]);
          break;
      }
      g_strfreev(extensions);    
      gchar * dest = g_strdup_printf("%s/awn-theme/scalable/%s",priv->icon_dir,new_basename);

      awn_icons_file_cleanup(priv,scope);
      
      
      if ( g_file_set_contents(dest,contents,length,&err))
      {     
       
        gtk_icon_theme_set_custom_theme(priv->awn_theme,NULL);
        gtk_icon_theme_set_custom_theme(priv->awn_theme,AWN_ICONS_THEME_NAME);  
        
        if (priv->icon_change_cb)
        {
          priv->icon_change_cb(dialog_data->awn_icons,priv->icon_change_cb_data);
        }
        
      }
      else if (err)
      {
        g_warning("Failed to copy icon %s: %s\n",sdata,err->message);
        g_error_free (err);
      }
      g_free(contents);
      g_free(new_basename);
      g_free(dest);
    }
    else if (err)
    {
      g_warning("Failed to copy icon %s: %s\n",sdata,err->message);
      g_error_free (err);
    }
  }
  else if (response == AWN_ICONS_RESPONSE_CLEAR)
  {
    scope = 0;
    awn_icons_file_cleanup(priv,scope);
    gtk_icon_theme_set_custom_theme(priv->awn_theme,NULL);
    gtk_icon_theme_set_custom_theme(priv->awn_theme,AWN_ICONS_THEME_NAME);  
    
    if (priv->icon_change_cb)
    {
      priv->icon_change_cb(dialog_data->awn_icons,priv->icon_change_cb_data);
    }    
  }
  g_free(dialog_data->sdata);
  g_free(dialog_data);
  gtk_widget_destroy(GTK_WIDGET(dialog));
}


void  
awn_icons_drag_data_received (GtkWidget          *widget,
                    GdkDragContext     *drag_context,
                    gint                x,
                    gint                y,
                    GtkSelectionData   *selection_data,
                    guint               info,
                    guint               time,
                    AwnIcons            *icons)
{
  gchar * decode = NULL;
  AwnIconsPrivate *priv=GET_PRIVATE(icons);   
  if((selection_data != NULL) && (selection_data-> length >= 0))
  {
    gchar * _sdata = (gchar*)selection_data-> data;
    if (_sdata)
    {
      decode = g_uri_unescape_string (_sdata,NULL);
      _sdata = decode;
      for(;(*_sdata!='\0') && _sdata; _sdata++)
      {
        if (*_sdata == ':' )
        {
          _sdata=_sdata+3;
          break;
        }
      }
      g_strchomp (_sdata);
      
      if ( gdk_pixbuf_get_file_info (_sdata,NULL,NULL) )
      {
        GtkWidget *dialog=NULL;         
        AwnIconsDialogData * dialog_data;
        GtkWidget * icon;
        GtkWidget * hbox;
        GtkWidget * vbox;
        GtkWidget * label;
        GdkPixbuf * pixbuf;
        
        pixbuf = gdk_pixbuf_new_from_file (_sdata,NULL);
        if (!pixbuf)
        {
          goto bad_gdk_pixbuf_get_file_info_bad;
        }
        if (! GDK_IS_PIXBUF(pixbuf))
        {
          goto bad_gdk_pixbuf_get_file_info_bad;
        }
        dialog_data = g_malloc(sizeof(AwnIconsDialogData));
        dialog_data->sdata = g_strdup(_sdata);
        dialog_data->awn_icons = icons;
       
        //TODO , add text, add scope options, display icon.
        dialog = gtk_dialog_new_with_buttons (_("Change Icon?"),
                                         NULL,
                                         GTK_DIALOG_NO_SEPARATOR,
                                         GTK_STOCK_CLEAR,
                                         AWN_ICONS_RESPONSE_CLEAR,
                                         GTK_STOCK_CANCEL,
                                         GTK_RESPONSE_NONE,
                                         GTK_STOCK_OK,
                                         GTK_RESPONSE_ACCEPT,
                                         NULL);
        GtkWidget * content_area =  GTK_DIALOG(dialog)->vbox;

        icon = gtk_image_new_from_pixbuf(pixbuf);
       

        hbox = gtk_hbox_new(FALSE,1);
        vbox = gtk_vbox_new(FALSE,1);   

        if ( priv->count > 0)
        {
          label = gtk_label_new (_("Use this icon?"));
          gtk_container_add (GTK_CONTAINER (vbox), label);        
        }
        else
        {
          label = gtk_label_new (_("Use this icon?"));
          gtk_container_add (GTK_CONTAINER (vbox), label);        
        }
        
        priv->scope_radio1 = gtk_radio_button_new (NULL);
        GtkWidget *scope_label = gtk_label_new(_("Apply to this applet instance."));
        gtk_container_add (GTK_CONTAINER (priv->scope_radio1),scope_label);
        
        priv->scope_radio2 = gtk_radio_button_new_with_label_from_widget
                    (GTK_RADIO_BUTTON(priv->scope_radio1),_("Apply to this applet type."));
        priv->scope_radio3 = gtk_radio_button_new_with_label_from_widget
                    (GTK_RADIO_BUTTON(priv->scope_radio1),_("Apply to icon name."));

        gtk_container_add (GTK_CONTAINER (vbox), priv->scope_radio1);
        gtk_container_add (GTK_CONTAINER (vbox), priv->scope_radio2);
        gtk_container_add (GTK_CONTAINER (vbox), priv->scope_radio3);
        
        if (priv->count >1 )
        {
          int i;
          priv->combo = gtk_combo_box_new_text ();
          gtk_container_add (GTK_CONTAINER (vbox), priv->combo);
          for (i=0; priv->states[i];i++)
          {
            gtk_combo_box_append_text(GTK_COMBO_BOX(priv->combo), priv->states[i]);
          }
          gtk_combo_box_set_active (GTK_COMBO_BOX(priv->combo),priv->cur_icon);
        }
        else
        {
          priv->combo=NULL;
        }
        
        gtk_container_add (GTK_CONTAINER (hbox), icon);
        gtk_container_add (GTK_CONTAINER (hbox), vbox);                
        gtk_container_add (GTK_CONTAINER (content_area), hbox);
        gtk_window_set_icon (GTK_WINDOW(dialog), pixbuf);
                             
        g_signal_connect(dialog, "response",G_CALLBACK(_awn_icons_dialog_response),dialog_data);

        gtk_widget_show_all(dialog);
        gtk_drag_finish (drag_context, TRUE, FALSE, time); //good enough... we'll say all was well.
        
        g_object_unref(pixbuf);
      }
      g_free (decode);
      return;
    }
  }

bad_gdk_pixbuf_get_file_info_bad:
   g_free (decode);
   gtk_drag_finish (drag_context, FALSE, FALSE, time);
 }


//-----
void 
awn_icons_set_height(AwnIcons * icons, gint height)
{
  AwnIconsPrivate *priv=GET_PRIVATE(icons); 
  priv->height = height;
}

//TODO implement individual set functions where it makes sense...
void 
awn_icons_set_icons_info(AwnIcons * icons,GtkWidget * applet,
                              const gchar * uid,
                              gint height,
                              const GStrv states,
                              const GStrv icon_names)
{
  g_return_if_fail(icons);
  g_return_if_fail(uid);
  g_return_if_fail(states);
  g_return_if_fail(icon_names);

  static gboolean doneonce = FALSE;
  int count;
  AwnIconsPrivate *priv=GET_PRIVATE(icons); 
 
  if (applet && !doneonce)
  {
    priv->icon_widget = GTK_WIDGET(applet);
    gtk_drag_dest_set (priv->icon_widget, 
                       GTK_DEST_DEFAULT_ALL,
                       drop_types,
                       n_drop_types,
                       GDK_ACTION_COPY | GDK_ACTION_ASK);
    
    g_signal_connect(priv->icon_widget,"drag_data_received",
                     G_CALLBACK(awn_icons_drag_data_received),icons);
  }
  for (count=0;states[count];count++);
  priv->count = count;
  
  for (count=0;icon_names[count];count++);
  g_return_if_fail(count == priv->count);

  if (priv->states)
  {
    for(count=0;priv->icon_names[count];count++)
    {
      g_free(priv->icon_names[count]);
      g_free(priv->states[count]);
    }
    g_free(priv->states);
    g_free(priv->icon_names);
  }
  
  priv->states = g_malloc( sizeof(gchar *) * count);
  priv->icon_names = g_malloc( sizeof(gchar *) * count);
  
  for (count=0; count < priv->count; count ++)
  {
    priv->states[count] = g_strdup(states[count]);
    priv->icon_names[count] = g_strdup(icon_names[count]);
  }
  priv->states[count] = NULL;
  priv->icon_names[count] = NULL;

  if (priv->uid)
  {
    g_free(priv->uid);
  }
  priv->uid = g_strdup(uid);
  priv->height = height;
  
  gtk_icon_theme_rescan_if_needed(priv->awn_theme);
  doneonce = TRUE;
}

void awn_icons_set_icon_info(AwnIcons * icons,
                             GtkWidget * applet,
                             const gchar * uid, 
                             gint height,
                             const gchar *icon_name)
{
  g_return_if_fail(icons);  
  gchar *states[] = {"__SINGULAR__",NULL};
  gchar *icon_names[] = {NULL,NULL};
  icon_names[0] = (gchar *)icon_name;
  awn_icons_set_icons_info(icons,applet,
                           uid,height,
                           (const GStrv)states,
                           (const GStrv)icon_names);
  
}

/*the callback could be more complicated if we wanted since we eventually deal 
 with sets of icons.  But it really it isn't needed.*/
void awn_icons_set_changed_cb(AwnIcons * icons,AwnIconsChange fn,gpointer user_data)
{
  AwnIconsPrivate *priv=GET_PRIVATE(icons); 
  priv->icon_change_cb = fn;
  priv->icon_change_cb_data = user_data;
  
}

GdkPixbuf * 
awn_icons_get_icon_at_height(AwnIcons * icons, const gchar * state, gint height)
{
  g_return_val_if_fail(icons,NULL);  
  gint count;
  GdkPixbuf * pixbuf = NULL;
  GError *err=NULL;   
  AwnIconsPrivate *priv=GET_PRIVATE(icons);
  g_assert(priv->states[0]);
  for (count = 0; priv->states[count]; count++)
  {
    if ( strcmp(state,priv->states[count]) == 0 )
    {
      int i;
      gchar * name=NULL;
      priv->cur_icon = count;
      
      for (i=0;i<7 ;i++)
      {
        switch (i)
        {
          case  0:                
            name = g_strdup_printf("%s-%s-%s",
                                  priv->icon_names[count],
                                  priv->applet_name,
                                  priv->uid);
            pixbuf = gtk_icon_theme_load_icon(priv->awn_theme, name , 
                                              height, 0, &err);
            break;
          case  1:
            name = g_strdup_printf("%s-%s",
                                   priv->icon_names[count],
                                   priv->applet_name);
            pixbuf = gtk_icon_theme_load_icon(priv->awn_theme, name , 
                                        height, 0, &err);
            break;
          case  2:
            pixbuf = gtk_icon_theme_load_icon(priv->awn_theme, 
                                        priv->icon_names[count],height, 
                                        0, &err);
            break;
          case  3:
            pixbuf = NULL;
            if (priv->override_theme)
            {
              pixbuf = gtk_icon_theme_load_icon(priv->override_theme,
                                                priv->icon_names[count],height,
                                                0,&err);
            }
            break;
          case  4:
            pixbuf = gtk_icon_theme_load_icon(gtk_icon_theme_get_default(), 
                                    priv->icon_names[count],height, 
                                    0, &err);
            break;
          case  5:
            pixbuf = gtk_icon_theme_load_icon(gtk_icon_theme_get_default(), 
                                  "stock_stop",height, 0, &err);
            break;
          case  6:
            pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, 
                                    height,height);
            gdk_pixbuf_fill(pixbuf, 0xee221155);
            break;
        } 
        
        if (err)
        {
          g_error_free (err);
          err=NULL;
        }        
        g_free(name);
        name = NULL;
        if (pixbuf)
        {
          if (gdk_pixbuf_get_height(pixbuf) != height)
          {
            GdkPixbuf * new_pixbuf = gdk_pixbuf_scale_simple(pixbuf,                                                             
                                     gdk_pixbuf_get_width(pixbuf)*
                                     height/gdk_pixbuf_get_height(pixbuf),
                                     height,                                                             
                                     GDK_INTERP_HYPER
                                     );
            g_object_unref(pixbuf);
            pixbuf = new_pixbuf;
          }
          break;
        }
      }
    }
  }
  g_assert(pixbuf);  
  return pixbuf;
}  

GdkPixbuf * 
awn_icons_get_icon_simple_at_height(AwnIcons * icons, gint height)
{
  g_return_val_if_fail(icons,NULL);
  AwnIconsPrivate *priv=GET_PRIVATE(icons);  
  return awn_icons_get_icon_at_height(icons, priv->states[priv->cur_icon],height);
}


GdkPixbuf * 
awn_icons_get_icon(AwnIcons * icons, const gchar * state)
{
  g_return_val_if_fail(icons,NULL);
  g_return_val_if_fail(state,NULL);
  AwnIconsPrivate *priv=GET_PRIVATE(icons);
  return awn_icons_get_icon_at_height(icons,state,priv->height);
}

GdkPixbuf * awn_icons_get_icon_simple(AwnIcons * icons)
{
  g_return_val_if_fail(icons,NULL);
  AwnIconsPrivate *priv=GET_PRIVATE(icons);
  return awn_icons_get_icon(icons, priv->states[priv->cur_icon]);
}

void awn_icons_override_gtk_theme(AwnIcons * icons,gchar * theme_name)
{
  g_return_if_fail(icons);
  AwnIconsPrivate *priv=GET_PRIVATE(icons);

  /*if NULL then remove any override*/
  if (!theme_name)
  {
    g_object_unref(priv->override_theme);
    priv->override_theme = NULL;
    return;
  }
  
  if (!priv->override_theme)
  {
    priv->override_theme = gtk_icon_theme_new();
  }
  gtk_icon_theme_set_custom_theme(priv->override_theme,theme_name);  
  if (priv->icon_change_cb)
  {
    priv->icon_change_cb(icons,priv->icon_change_cb_data);
  }   
  
}

void _theme_changed(GtkIconTheme *icon_theme,AwnIcons * awn_icons) 
{
  AwnIconsPrivate *priv=GET_PRIVATE(awn_icons);  
  if (priv->icon_change_cb)
  {
    priv->icon_change_cb(awn_icons,priv->icon_change_cb_data);
  }  
}

static void
awn_icons_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
  switch (property_id) {
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
awn_icons_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
  switch (property_id) {
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
awn_icons_dispose (GObject *object)
{
  if (G_OBJECT_CLASS (awn_icons_parent_class)->dispose)
    G_OBJECT_CLASS (awn_icons_parent_class)->dispose (object);
}

static void
awn_icons_finalize (GObject *object)
{
  if (G_OBJECT_CLASS (awn_icons_parent_class)->finalize)
    G_OBJECT_CLASS (awn_icons_parent_class)->finalize (object);
}

static void
awn_icons_class_init (AwnIconsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (AwnIconsPrivate));

  object_class->get_property = awn_icons_get_property;
  object_class->set_property = awn_icons_set_property;
  object_class->dispose = awn_icons_dispose;
  object_class->finalize = awn_icons_finalize;
}

static void
awn_icons_init (AwnIcons *self)
{
  gchar * contents;
  gsize length;
  GError *err=NULL;    
  AwnIconsPrivate *priv=GET_PRIVATE(self);
  
  
  priv->icon_widget = NULL;
  priv->states = NULL;
  priv->icon_names = NULL;
  priv->uid = NULL;
  priv->cur_icon = 0;
  priv->count = 0;  
  priv->height = 0;
  priv->icon_change_cb = NULL;
  priv->icon_change_cb_data = NULL;
  priv->override_theme = NULL;

  //do we have our dirs....
  const gchar * home = g_getenv("HOME");
  //do not free icon_dir... saving it.
  gchar * icon_dir = g_strdup_printf("%s/.icons",home);
  priv->icon_dir=icon_dir;
  
  if ( !g_file_test(icon_dir,G_FILE_TEST_IS_DIR) )
  {
    g_mkdir(icon_dir,0775);
  }
  
  gchar * awn_theme_dir = g_strdup_printf("%s/%s",icon_dir,AWN_ICONS_THEME_NAME);

  if ( !g_file_test(awn_theme_dir,G_FILE_TEST_IS_DIR) )
  {
    g_mkdir(awn_theme_dir,0775);
  }
  
  gchar * awn_scalable_dir = g_strdup_printf("%s/scalable",awn_theme_dir);  
  g_free(awn_theme_dir);
  if ( !g_file_test(awn_scalable_dir,G_FILE_TEST_IS_DIR) )
  {
    g_mkdir(awn_scalable_dir,0775);
  }
  
  g_free(awn_scalable_dir);
  
  gchar * index_theme_src = g_strdup_printf("%s/avant-window-navigator/index.theme",DATADIR);
  gchar * index_theme_dest = g_strdup_printf("%s/%s/index.theme",icon_dir,
                                             AWN_ICONS_THEME_NAME);
  if (! g_file_test(index_theme_dest,G_FILE_TEST_EXISTS) )
  {
    if (g_file_get_contents (index_theme_src,&contents,&length,&err))
    {  
      if ( g_file_set_contents(index_theme_dest,contents,length,&err))
      {
        
      }
      else if (err)
      {
        g_warning("Failed to copy index.theme: %s\n",err->message);
        g_error_free (err);
      }
      g_free(contents);
    }
    else if (err)
    {
      g_warning("Failed to copy index.theme : %s\n",err->message);
      g_error_free (err);
    }
  }
  g_free(index_theme_src);
  g_free(index_theme_dest);
  
  priv->awn_theme = gtk_icon_theme_new();
  gtk_icon_theme_set_custom_theme(priv->awn_theme,AWN_ICONS_THEME_NAME);  
  
  g_signal_connect(gtk_icon_theme_get_default(),"changed",
                   G_CALLBACK(_theme_changed),
                   self);
  g_signal_connect(priv->awn_theme ,"changed",
                   G_CALLBACK(_theme_changed),
                   self);
} 

AwnIcons*
awn_icons_new (const gchar * applet_name)
{
  AwnIcons * obj = g_object_new (AWN_TYPE_ICONS, NULL);
  AwnIconsPrivate *priv=GET_PRIVATE(obj);    
  priv->applet_name = g_strdup(applet_name);
  gchar * applet_icon_dir = g_strdup_printf("%s/avant-window-navigator/applets/%s/icons",
                                         DATADIR,
                                         priv->applet_name);
  gtk_icon_theme_append_search_path (gtk_icon_theme_get_default(),applet_icon_dir);
  g_free(applet_icon_dir);  
  
  applet_icon_dir = g_strdup_printf("%s/avant-window-navigator/applets/%s/themes",
                                         DATADIR,
                                         priv->applet_name);
  gtk_icon_theme_prepend_search_path (gtk_icon_theme_get_default(),applet_icon_dir);
  g_free(applet_icon_dir);      
  return obj;
}


