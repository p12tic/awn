/*
 *  Copyright (C) 2007, 2008 Mark Lee <avant-wn@lazymalevolence.com>
 *                2008 Neil Jagdish Patel <njpatel@gmail.com>
 *
 *  For libgnome-desktop portions:
 *  Copyright (C) 1999, 2000 Red Hat Inc.
 *  Copyright (C) 2001 Sid Vicious
 *  All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the
 *  Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 *  Boston, MA 02111-1307, USA.
 *
 *  Author : Mark Lee <avant-wn@lazymalevolence.com>
 *           Neil Jagdish Patel <njpatel@gmail.com>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>
#include "egg/eggdesktopfile.h"

#include "awn-desktop-item.h"

struct _AwnDesktopItem 
{
	EggDesktopFile *desktop_file;
	GKeyFile       *key_file;
};

/* GType function */
GType awn_desktop_item_get_type (void)
{
  return egg_desktop_file_get_type ();
}

/* Wrapper functions */

static void 
awn_desktop_item_initialize (AwnDesktopItem *item, const gchar *filename)
{
  GError *err = NULL;
  gchar  *default_desktop_entry;
  
  default_desktop_entry = g_strdup_printf ("[%s]\nType=Unknown", 
                                           EGG_DESKTOP_FILE_GROUP);
	
  item->desktop_file = g_new0 (EggDesktopFile, 1);
  item->desktop_file->type = EGG_DESKTOP_FILE_TYPE_UNRECOGNIZED;
  item->desktop_file->source = g_strdup (filename);
  item->desktop_file->name = g_strdup ("");
  item->desktop_file->icon = g_strdup ("");
  item->key_file = g_key_file_new ();
	
  if (!g_key_file_load_from_data (item->key_file, 
                                  default_desktop_entry, 
                                  (gsize)strlen (default_desktop_entry), 
                                  G_KEY_FILE_NONE, &err)) 
  {
    g_error ("Could not load the default desktop entry: %s", err->message);
    g_error_free (err);
  }
	
  item->desktop_file->key_file = item->key_file;
  g_free (default_desktop_entry);
}

AwnDesktopItem *
awn_desktop_item_new (const gchar *filename)
{
  AwnDesktopItem *item;
  GError         *err = NULL;

  item = g_new0 (AwnDesktopItem, 1);

  if (g_file_test (filename, G_FILE_TEST_EXISTS)) 
  {
    item->desktop_file = egg_desktop_file_new (filename, &err);
    if (err) 
    {
      g_warning ("Could not load the desktop item at '%s': %s",
                 filename, err->message);
      g_error_free (err);
    }
    if (item->desktop_file) 
    {
      item->key_file = egg_desktop_file_get_key_file (item->desktop_file);
    } 
    else 
    {
      awn_desktop_item_initialize (item, filename);
    }
  } 
  else 
  {
    awn_desktop_item_initialize (item, filename);
  }
  return item;
}

AwnDesktopItem *
awn_desktop_item_copy (const AwnDesktopItem *item)
{
  AwnDesktopItem *new_item;

  g_return_val_if_fail (item, NULL);
  
  new_item = g_new0 (AwnDesktopItem, 1);  
  new_item->desktop_file = egg_desktop_file_copy (item->desktop_file);
	
  return new_item;
}

const gchar *
awn_desktop_item_get_filename (AwnDesktopItem *item)
{
  g_return_val_if_fail (item, NULL);
  return egg_desktop_file_get_source (item->desktop_file);
}

gchar *
awn_desktop_item_get_item_type (AwnDesktopItem *item)
{
  g_return_val_if_fail (item, NULL);
  return awn_desktop_item_get_string (item, EGG_DESKTOP_FILE_KEY_TYPE);
}

void 
awn_desktop_item_set_item_type (AwnDesktopItem *item, const gchar *item_type)
{
  g_return_if_fail (item);
  g_return_if_fail (item_type);
    
  awn_desktop_item_set_string (item, EGG_DESKTOP_FILE_KEY_TYPE, item_type);
	
  if (strcmp (item_type, "Application") == 0) 
  {
  item->desktop_file->type = EGG_DESKTOP_FILE_TYPE_APPLICATION; 
  } 
  else if (strcmp (item_type, "Link") == 0) 
  {
    item->desktop_file->type = EGG_DESKTOP_FILE_TYPE_LINK;
  } 
  else if (strcmp (item_type, "Directory") == 0) 
  {
    item->desktop_file->type = EGG_DESKTOP_FILE_TYPE_DIRECTORY;
  } 
  else 
  {
    item->desktop_file->type = EGG_DESKTOP_FILE_TYPE_UNRECOGNIZED;
  }
}

gchar *
awn_desktop_item_get_icon_name (AwnDesktopItem *item)
{
  g_return_val_if_fail (item, NULL);
  
  return g_strdup (egg_desktop_file_get_icon (item->desktop_file));
}

void 
awn_desktop_item_set_icon_name (AwnDesktopItem *item, const gchar *icon)
{
  g_return_if_fail (item);
  g_return_if_fail (icon);

  awn_desktop_item_set_string (item, EGG_DESKTOP_FILE_KEY_ICON, icon);
  
  if (item->desktop_file->icon)
    g_free (item->desktop_file->icon);
  
  item->desktop_file->icon = g_strdup (icon);
}
static char *
strip_extension (const char *file)
{
  char *stripped, *p;

  stripped = g_strdup (file);

  p = strrchr (stripped, '.');
  if (p &&
      (!strcmp (p, ".png") ||
       !strcmp (p, ".svg") ||
       !strcmp (p, ".xpm")))
	  *p = 0;

  return stripped;
}

/* Gets the pixbuf from a desktop file's icon name. Based on the same function
 * from matchbox-desktop
 */
static GdkPixbuf *
get_icon (const gchar *name, guint size)
{
  static GtkIconTheme *theme = NULL;
  GdkPixbuf *pixbuf = NULL;
  GError *error = NULL;
  gchar *stripped = NULL;

  gint width, height;

  if (theme == NULL)
    theme = gtk_icon_theme_get_default ();

  if (name == NULL)
  {
    pixbuf = gtk_icon_theme_load_icon (theme, "application-x-executable",
                                       size, 0, NULL);
    return pixbuf;
  }

  if (g_path_is_absolute (name))
  {
    if (g_file_test (name, G_FILE_TEST_EXISTS))
    {
      pixbuf = gdk_pixbuf_new_from_file_at_scale (name, size, size, 
                                                  TRUE, &error);
      if (error)
      {
        /*g_warning ("Error loading icon: %s\n", error->message);*/
        g_error_free (error);
        error = NULL;
     }
      return pixbuf;
    } 
  }

  stripped = strip_extension (name);
  
  pixbuf = gtk_icon_theme_load_icon (theme,
                                     stripped,
                                     size,
                                     GTK_ICON_LOOKUP_FORCE_SVG, &error);
  if (error)
  {   
    /*g_warning ("Error loading icon: %s\n", error->message);*/
    g_error_free (error);
    error = NULL;
  }
  
  /* Always try and send back something */
  if (pixbuf == NULL)
    pixbuf = gtk_icon_theme_load_icon (theme, "application-x-executable",
                                       size, 0, NULL);
  
  width = gdk_pixbuf_get_width (pixbuf);
  height = gdk_pixbuf_get_height (pixbuf);

  if (width != size || height != size)
  {
    GdkPixbuf *temp = pixbuf;
    pixbuf = gdk_pixbuf_scale_simple (temp, 
                                      size,
                                      size,
                                      GDK_INTERP_HYPER);
    g_object_unref (temp);
  }

  g_free (stripped);

 return pixbuf;
}

GdkPixbuf * 
awn_desktop_item_get_icon (AwnDesktopItem *item, guint size)
{
  const gchar *icon_name;

  g_return_val_if_fail (item, NULL);

  if (size < 16)
    size = 16;

  icon_name = egg_desktop_file_get_icon (item->desktop_file);

  if (!icon_name)
    icon_name = "application-x-executable";

  return get_icon (icon_name, size);
}

gchar *
awn_desktop_item_get_name (AwnDesktopItem *item)
{
  g_return_val_if_fail (item, NULL);
  
  return g_strdup (egg_desktop_file_get_name (item->desktop_file));
}

void 
awn_desktop_item_set_name (AwnDesktopItem *item, const gchar *name)
{
  g_return_if_fail (item);
  g_return_if_fail (name);

	awn_desktop_item_set_string (item, EGG_DESKTOP_FILE_KEY_NAME, name);

  if (item->desktop_file->name)
    g_free (item->desktop_file->name);

  item->desktop_file->name = g_strdup (name);
}

gchar *
awn_desktop_item_get_exec (AwnDesktopItem *item)
{
  g_return_val_if_fail (item, NULL);

  return awn_desktop_item_get_string (item, EGG_DESKTOP_FILE_KEY_EXEC);
}

void 
awn_desktop_item_set_exec (AwnDesktopItem *item, const gchar *exec)
{
  g_return_if_fail (item);
  g_return_if_fail (exec);

  awn_desktop_item_set_string (item, EGG_DESKTOP_FILE_KEY_EXEC, exec);
}

gchar *
awn_desktop_item_get_string (AwnDesktopItem *item, const gchar *key)
{
  GKeyFile *file;
  gchar *value;
  GError *err = NULL;

  g_return_val_if_fail (item, NULL);
  g_return_val_if_fail (key, NULL);

  file = egg_desktop_file_get_key_file (item->desktop_file);
	value = g_key_file_get_string (file, EGG_DESKTOP_FILE_GROUP, key, &err);
	
  if (err) 
  {
		g_warning("Could not get the value of '%s' from '%s': %s",
		          key, awn_desktop_item_get_filename (item), err->message);
	}
  return value;
}

void 
awn_desktop_item_set_string (AwnDesktopItem *item, 
                             const gchar    *key, 
                             const gchar    *value)
{
  g_return_if_fail (item);
  g_return_if_fail (key);
  g_return_if_fail (value);

  g_key_file_set_string (egg_desktop_file_get_key_file (item->desktop_file),
	                       EGG_DESKTOP_FILE_GROUP, key, value);
}

gchar *
awn_desktop_item_get_localestring (AwnDesktopItem *item, const gchar *key)
{
  gchar    *value = NULL;
  GError   *err = NULL;
  GKeyFile *file;
  const gchar* const *langs = g_get_language_names ();
  guint    i = 0;
  guint    langs_len = g_strv_length ((gchar**)langs);

  g_return_val_if_fail (item, NULL);
  g_return_val_if_fail (key, NULL);
	
  file = egg_desktop_file_get_key_file (item->desktop_file);

  do 
  {
    value = g_key_file_get_locale_string (file, EGG_DESKTOP_FILE_GROUP, 
                                          key, langs[i], &err);
    if (value)
      break;
    i++;
  } 
  while (err && i < langs_len);

  if (err) 
  {
    g_warning("Could not get the value of '%s' from '%s': %s",
		          key, awn_desktop_item_get_filename (item), err->message);
    g_error_free (err);
  }
  return value;
}

void 
awn_desktop_item_set_localestring (AwnDesktopItem *item, 
                                   const gchar    *key, 
                                   const gchar    *locale, 
                                   const gchar    *value)
{
  GKeyFile *file;

  g_return_if_fail (item);
  g_return_if_fail (key);
  g_return_if_fail (value);

  if (locale == NULL) 
  {
    /* use the first locale from g_get_language_names () */
    locale = (gchar*)(g_get_language_names ()[0]);
  }

  file = egg_desktop_file_get_key_file (item->desktop_file);
	g_key_file_set_locale_string (file, EGG_DESKTOP_FILE_GROUP, 
                                key, locale, value);
}

gboolean 
awn_desktop_item_exists (AwnDesktopItem *item)
{
  g_return_val_if_fail (item, FALSE);
  return egg_desktop_file_can_launch (item->desktop_file, NULL);
}

gint 
awn_desktop_item_launch (AwnDesktopItem *item, GSList *documents, GError **err)
{
  GPid pid;

  g_return_val_if_fail (item, 0);

  egg_desktop_file_launch (item->desktop_file, documents, err,
                           EGG_DESKTOP_FILE_LAUNCH_RETURN_PID, &pid,
	                         NULL);
  return pid;
}

void 
awn_desktop_item_save (AwnDesktopItem *item,  
                       const gchar    *new_filename, 
                       GError        **err)
{
	gchar *data = NULL;
	const gchar *filename;
	gsize data_len;

  g_return_if_fail (item);
  g_return_if_fail (new_filename);

	data = g_key_file_to_data (egg_desktop_file_get_key_file (item->desktop_file),
                             &data_len, err);
	if (err) 
  {
		g_free (data);
		return;
	} 
  else 
  {
		if (new_filename) 
    {
			filename = new_filename;
			item->desktop_file->source = g_strdup (filename);
		} 
    else 
    {
			filename = awn_desktop_item_get_filename (item);
		}
		g_file_set_contents (filename, data, data_len, err);
		g_free (data);
	}
}

void 
awn_desktop_item_free (AwnDesktopItem *item)
{
  g_return_if_fail (item);

	egg_desktop_file_free (item->desktop_file);
	g_free (item);
}
/*  vim: set noet ts=8 sts=8 sw=8 : */
