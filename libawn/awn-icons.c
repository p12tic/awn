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
 * GNU Library General Public License for more details.
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



G_DEFINE_TYPE (AwnIcons, awn_icons, G_TYPE_OBJECT)

#define AWN_ICONS_THEME_NAME "awn-theme"

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), LIBAWN_TYPE_AWN_ICONS, AwnIconsPrivate))

typedef struct _AwnIconsPrivate AwnIconsPrivate;

struct _AwnIconsPrivate {
  GtkWidget *   icon_widget;    //used if Non-NULL for drag and drop support

  GtkIconTheme *  awn_theme;
  
  AwnIconsChange    icon_change_cb;
  gpointer          icon_change_cb_data;
  GtkWidget * scope_radio1;       //this just seems the easiest way to to deal
  GtkWidget * scope_radio2;       //with the radio buttons in the dialog.
  GtkWidget * scope_radio3;       //also seems wrong.
 
  gchar **  states;
  gchar **  icon_names;
  gchar *   applet_name;
  gchar *   uid;
  gchar *   icon_dir;
  
  gint  height;
  gint  cur_icon;
  gint  count;
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
      
      GdkPixbufFormat* pixbuf_info = gdk_pixbuf_get_file_info (sdata,NULL,NULL);
      g_return_if_fail(pixbuf_info);
      gchar ** extensions = gdk_pixbuf_format_get_extensions(pixbuf_info);
      
      switch (scope)
      {
        case  0:
          new_basename=g_strdup_printf("%s.%s",priv->icon_names[priv->cur_icon],
                                       extensions[0] );
          break;
        case  1:
          new_basename=g_strdup_printf("%s-%s.%s",
                                       priv->icon_names[priv->cur_icon],
                                       priv->applet_name,
                                       extensions[0]);
          break;
        case  2:
        default:  //supress a warning.
          new_basename=g_strdup_printf("%s-%s-%s.%s",
                                       priv->icon_names[priv->cur_icon],
                                       priv->applet_name,
                                       priv->uid,
                                       extensions[0]);
          break;
      }
      g_strfreev(extensions);    
      gchar * dest = g_strdup_printf("%s/awn-theme/scalable/%s",priv->icon_dir,new_basename);
      if ( g_file_set_contents(dest,contents,length,&err))
      {
        gchar * filename;
        printf("Icon set %s\n",dest);
        //        gtk_icon_theme_rescan_if_needed(priv->awn_theme);        
        //  This ^ does not seem to force an update. For now will just recreate 
        // the damn thing.
        
//        g_object_unref(priv->awn_theme);
//        priv->awn_theme = gtk_icon_theme_new();
        
        gtk_icon_theme_set_custom_theme(priv->awn_theme,NULL);
        gtk_icon_theme_set_custom_theme(priv->awn_theme,AWN_ICONS_THEME_NAME);  
        
        switch(scope)
        {
          case 0:
          //FIXME put this stuff in a loop though an array { "png","xpm","svg"}
            filename=g_strdup_printf("%s/awn-theme/scalable/%s-%s.svg",
                               priv->icon_dir,
                               priv->icon_names[priv->cur_icon],
                               priv->applet_name);          
            g_unlink(filename); 
            g_free(filename);        
            filename=g_strdup_printf("%s/awn-theme/scalable/%s-%s.png",
                               priv->icon_dir,
                               priv->icon_names[priv->cur_icon],
                               priv->applet_name);          
            g_unlink(filename); 
            g_free(filename);           
            filename=g_strdup_printf("%s/awn-theme/scalable/%s-%s.xpm",
                               priv->icon_dir,
                               priv->icon_names[priv->cur_icon],
                               priv->applet_name);          
            g_unlink(filename); 
            g_free(filename);                 
          case 1:
            filename=g_strdup_printf("%s/awn-theme/scalable/%s-%s-%s.svg",
                               priv->icon_dir,                                     
                               priv->icon_names[priv->cur_icon],
                               priv->applet_name,
                               priv->uid);
            g_unlink(filename);
            g_free(filename);
            filename=g_strdup_printf("%s/awn-theme/scalable/%s-%s-%s.png",
                               priv->icon_dir,                                     
                               priv->icon_names[priv->cur_icon],
                               priv->applet_name,
                               priv->uid);
            g_unlink(filename);
            g_free(filename);            
            filename=g_strdup_printf("%s/awn-theme/scalable/%s-%s-%s.xpm",
                               priv->icon_dir,                                     
                               priv->icon_names[priv->cur_icon],
                               priv->applet_name,
                               priv->uid);
            g_unlink(filename);
            g_free(filename);            
            
        }
        
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
  g_free(dialog_data->sdata);
  g_free(dialog_data);
  gtk_widget_destroy(GTK_WIDGET(dialog));
  printf("DONE DIALOG\n");
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
  AwnIconsPrivate *priv=GET_PRIVATE(icons);   
  printf("awn_icons_drag_data_received\n");
  if((selection_data != NULL) && (selection_data-> length >= 0))
  {
    gchar * _sdata = (gchar*)selection_data-> data;
    printf("dnd %s \n",_sdata);
    if (_sdata)
    {
      for(;(*_sdata!='\0') && _sdata; _sdata++)
      {
        printf("char = %c \n",*_sdata);
        if (*_sdata == ':' )
        {
          _sdata=_sdata+3;
          break;
        }
      }
      
      printf("'%s' \n",_sdata);
      g_strchomp (_sdata);
      
      if ( gdk_pixbuf_get_file_info (_sdata,NULL,NULL) )
      {

        printf("DND %s \n",_sdata);
        printf("good pixbuf\n");
        GtkWidget *dialog=NULL;         
        AwnIconsDialogData * dialog_data = g_malloc(sizeof(AwnIconsDialogData));
        dialog_data->sdata = g_strdup(_sdata);
        dialog_data->awn_icons = icons;
       
        //TODO , add text, add scope options, display icon.
        dialog = gtk_dialog_new_with_buttons ("Change Icon?",
                                         NULL,
                                         GTK_DIALOG_NO_SEPARATOR,
                                         GTK_STOCK_CANCEL,
                                         GTK_RESPONSE_NONE,
                                         GTK_STOCK_OK,
                                         GTK_RESPONSE_ACCEPT,
                                         NULL);
        GtkWidget * content_area =  GTK_DIALOG(dialog)->vbox;
        
        GtkWidget * icon;
        GtkWidget * hbox;
        GtkWidget * vbox;
        GtkWidget * label;
        GdkPixbuf * pixbuf;
        
        pixbuf = gdk_pixbuf_new_from_file (_sdata,NULL);
        icon = gtk_image_new_from_pixbuf(pixbuf);

        hbox = gtk_hbox_new(FALSE,1);
        vbox = gtk_vbox_new(FALSE,1);   

        if ( priv->count > 0)
        {
          label = gtk_label_new ("Use this icon?");
          gtk_container_add (GTK_CONTAINER (vbox), label);        
        }
        else
        {
          label = gtk_label_new ("Use this icon?");
          gtk_container_add (GTK_CONTAINER (vbox), label);        
        }
        
        priv->scope_radio1 = gtk_radio_button_new (NULL);
        GtkWidget *scope_label = gtk_label_new("Apply to this applet instance.");
        gtk_container_add (GTK_CONTAINER (priv->scope_radio1),scope_label);
        
        priv->scope_radio2 = gtk_radio_button_new_with_label_from_widget
                    (GTK_RADIO_BUTTON(priv->scope_radio1),"Apply to this applet type.");
        priv->scope_radio3 = gtk_radio_button_new_with_label_from_widget
                    (GTK_RADIO_BUTTON(priv->scope_radio1),"Apply to icon name.");

        
        gtk_container_add (GTK_CONTAINER (vbox), priv->scope_radio1);
        gtk_container_add (GTK_CONTAINER (vbox), priv->scope_radio2);
        gtk_container_add (GTK_CONTAINER (vbox), priv->scope_radio3);
        
        gtk_container_add (GTK_CONTAINER (hbox), icon);
        gtk_container_add (GTK_CONTAINER (hbox), vbox);                
        gtk_container_add (GTK_CONTAINER (content_area), hbox);
        gtk_window_set_icon (GTK_WINDOW(dialog), pixbuf);
                             
        g_signal_connect(dialog, "response",G_CALLBACK(_awn_icons_dialog_response),dialog_data);

        gtk_widget_show_all(dialog);
        gtk_drag_finish (drag_context, TRUE, FALSE, time); //good enough... we'll say all was well.
        
        g_object_unref(pixbuf);
      }
      return;
    }
  }
      
   gtk_drag_finish (drag_context, FALSE, FALSE, time);
 }


//-----


//TODO implement individual set functions where it makes sense...
void awn_icons_set_icons_info(AwnIcons * icons,GtkWidget * applet,
                             gchar * applet_name,gchar * uid,gint height,
                             gchar **states,gchar **icon_names)
{
  g_return_if_fail(icons);
  g_return_if_fail(applet_name);
  g_return_if_fail(uid);
  g_return_if_fail(states);
  g_return_if_fail(icon_names);

  int count;
  AwnIconsPrivate *priv=GET_PRIVATE(icons); 
 
  if (applet)
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
    for(count=0;icon_names[count];count++)
    {
      g_free(icon_names[count]);
      g_free(states[count]);
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

  if (priv->applet_name)
  {
    g_free(priv->applet_name);
  }
  priv->applet_name = g_strdup(applet_name);
  
  if (priv->uid)
  {
    g_free(priv->uid);
  }
  priv->uid = g_strdup(uid);
  priv->height = height;
  
  gtk_icon_theme_rescan_if_needed(priv->awn_theme);
}

void awn_icons_set_icon_info(AwnIcons * icons,
                             GtkWidget * applet,
                             gchar * applet_name,
                             gchar * uid, 
                             gint height,
                             gchar *icon_name)
{
  g_return_if_fail(icons);  
  gchar *states[] = {"__SINGULAR__",NULL};
  gchar *icon_names[] = {NULL,NULL};
  icon_names[0] = icon_name;
  awn_icons_set_icons_info(icons,applet,applet_name,
                           uid,height,states,icon_names);
  
}

/*the callback could be more complicated if we wanted since we eventually deal 
 with sets of icons.  But it really it isn't needed.*/
void awn_icons_set_changed_cb(AwnIcons * icons,AwnIconsChange fn,gpointer data)
{
  AwnIconsPrivate *priv=GET_PRIVATE(icons); 
  
  priv->icon_change_cb = fn;
  priv->icon_change_cb_data = data;
  
}

GdkPixbuf * awn_icons_get_icon(AwnIcons * icons, gchar * state)
{
  g_return_val_if_fail(icons,NULL);
  g_return_val_if_fail(state,NULL);
  
  gint count;
  GdkPixbuf * pixbuf = NULL;
  GError *err=NULL;   
  AwnIconsPrivate *priv=GET_PRIVATE(icons);
  g_assert(priv->states[0]);
  for (count = 0; priv->states[count]; count++)
  {
    printf(" check '%s'  == '%s' \n",state,priv->states[count]);
    printf(" priv->height = %d\n",priv->height);
    if ( strcmp(state,priv->states[count]) == 0 )
    {
      int i;
      gchar * name=NULL;
      priv->cur_icon = count;
      
      for (i=0;i<6 ;i++)
      {
        switch (i)
        {
          case  0:                
            name = g_strdup_printf("%s-%s-%s",
                                  priv->icon_names[count],
                                  priv->applet_name,
                                  priv->uid);
            pixbuf = gtk_icon_theme_load_icon(priv->awn_theme, name , 
                                              priv->height, 0, &err);
            break;
          case  1:
            name = g_strdup_printf("%s-%s",
                                   priv->icon_names[count],
                                   priv->applet_name);
            pixbuf = gtk_icon_theme_load_icon(priv->awn_theme, name , 
                                        priv->height, 0, &err);
            break;
          case  2:
            pixbuf = gtk_icon_theme_load_icon(priv->awn_theme, 
                                        priv->icon_names[count],priv->height, 
                                        0, &err);
            break;
          case  3:
            pixbuf = gtk_icon_theme_load_icon(gtk_icon_theme_get_default(), 
                                    priv->icon_names[count],priv->height, 
                                    0, &err);
            break;
          case  4:
            pixbuf = gtk_icon_theme_load_icon(gtk_icon_theme_get_default(), 
                                  "stock_stop",priv->height, 0, &err);
            break;
          case  5:
            pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, 
                                    priv->height,priv->height);
            gdk_pixbuf_fill(pixbuf, 0xee221155);
            break;
        } 
        
        if (err)
        {
          g_warning("Failed loading icon: %s\n",err->message);
          g_error_free (err);
          err=NULL;
        }        
        printf("name = %s\n",name);
        g_free(name);
        name = NULL;
        if (pixbuf)
        {
          if (gdk_pixbuf_get_height(pixbuf) != priv->height)
          {
            GdkPixbuf * new_pixbuf = gdk_pixbuf_scale_simple(pixbuf,                                                             
                                     gdk_pixbuf_get_width(pixbuf)*
                                     priv->height/gdk_pixbuf_get_height(pixbuf),
                                     priv->height,                                                             
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


GdkPixbuf * awn_icons_get_icon_simple(AwnIcons * icons)
{
  g_return_val_if_fail(icons,NULL);

  return awn_icons_get_icon(icons,"__SINGULAR__");
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
  printf("Checking for index.theme\n");
  if (! g_file_test(index_theme_dest,G_FILE_TEST_EXISTS) )
  {
    printf("index.theme not found\n");
    if (g_file_get_contents (index_theme_src,&contents,&length,&err))
    {  
      if ( g_file_set_contents(index_theme_dest,contents,length,&err))
      {
        printf("index.theme copied %s\n",index_theme_dest);
        
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
} 

AwnIcons*
awn_icons_new (void)
{
  return g_object_new (LIBAWN_TYPE_AWN_ICONS, NULL);
}


