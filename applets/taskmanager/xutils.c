/* Xlib utils */
/* vim: set sw=2 et: */

/*
 * Copyright (C) 2001 Havoc Pennington
 * Copyright (C) 2005-2007 Vincent Untz
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <config.h>

#include <string.h>
#include <stdio.h>

#include <X11/Xatom.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>

#include <unistd.h>

#include "xutils.h"

typedef struct _WnckIconCache WnckIconCache;

static void
_wnck_error_trap_push (void)
{
  gdk_error_trap_push ();
}

static int
_wnck_error_trap_pop (void)
{
  XSync (gdk_display, False);
  return gdk_error_trap_pop ();
}

static char*
latin1_to_utf8 (const char *latin1)
{
  GString *str;
  const char *p;
  
  str = g_string_new (NULL);

  p = latin1;
  while (*p)
    {
      g_string_append_unichar (str, (gunichar) *p);
      ++p;
    }

  return g_string_free (str, FALSE);
}

void
_wnck_get_wmclass (Window xwindow,
                   char **res_class,
                   char **res_name)
{
  XClassHint ch;
  char *retval;
  
  _wnck_error_trap_push ();

  ch.res_name = NULL;
  ch.res_class = NULL;

  XGetClassHint (gdk_display, xwindow,
                 &ch);

  _wnck_error_trap_pop ();
  
  retval = NULL;

  if (res_class)
    *res_class = NULL;

  if (res_name)
    *res_name = NULL;
  
  if (ch.res_name)
    {
      if (res_name)
        *res_name = latin1_to_utf8 (ch.res_name);
      
      XFree (ch.res_name);
    }

  if (ch.res_class)
    {
      if (res_class)
        *res_class = latin1_to_utf8 (ch.res_class);
      
      XFree (ch.res_class);
    }
}


void
_wnck_get_client_name (Window xwindow, char **client_name)
{
  XTextProperty	text_prop;
  Status status;
  
  _wnck_error_trap_push ();

	status = XGetWMClientMachine(gdk_display, xwindow, &text_prop);

  _wnck_error_trap_pop ();
  
  if (!status)
  {
    *client_name = NULL;
    if (text_prop.value)
    {
      XFree (text_prop.value);
    }
    return;
  }
  if (text_prop.value)
  {
    *client_name = latin1_to_utf8 (text_prop.value);       
    XFree (text_prop.value);
  }
}

/*
 * WNCK_GET_ICON_AT_SIZE
 */
static GHashTable *atom_hash = NULL;
static GHashTable *reverse_atom_hash = NULL;

static Atom
_wnck_atom_get (const char *atom_name)
{
  Atom retval;

  g_return_val_if_fail (atom_name != NULL, None);

  if (!atom_hash)
  {
    atom_hash = g_hash_table_new (g_str_hash, g_str_equal);
    reverse_atom_hash = g_hash_table_new (NULL, NULL);
  }

  retval = GPOINTER_TO_UINT (g_hash_table_lookup (atom_hash, atom_name));
  if (!retval)
  {
    retval = XInternAtom (gdk_display, atom_name, FALSE);

    if (retval != None)
    {
      char *name_copy;

      name_copy = g_strdup (atom_name);

      g_hash_table_insert (atom_hash, name_copy, GUINT_TO_POINTER (retval));
      g_hash_table_insert (reverse_atom_hash,
			   GUINT_TO_POINTER (retval), name_copy);
    }
  }

  return retval;
}

/* The icon-reading code is copied
 * from metacity, please sync bugfixes
 */
static gboolean
find_largest_sizes (gulong * data, gulong nitems, int *width, int *height)
{
  *width = 0;
  *height = 0;

  while (nitems > 0)
  {
    int w, h;
    gboolean replace;

    replace = FALSE;

    if (nitems < 3)
      return FALSE;		/* no space for w, h */

    w = data[0];
    h = data[1];

    if (nitems < ((w * h) + 2))
      return FALSE;		/* not enough data */

    *width = MAX (w, *width);
    *height = MAX (h, *height);

    data += (w * h) + 2;
    nitems -= (w * h) + 2;
  }

  return TRUE;
}

static gboolean
find_best_size (gulong * data,
		gulong nitems,
		int ideal_width,
		int ideal_height, int *width, int *height, gulong ** start)
{
  int best_w;
  int best_h;
  gulong *best_start;
  int max_width, max_height;

  *width = 0;
  *height = 0;
  *start = NULL;

  if (!find_largest_sizes (data, nitems, &max_width, &max_height))
    return FALSE;

  if (ideal_width < 0)
    ideal_width = max_width;
  if (ideal_height < 0)
    ideal_height = max_height;

  best_w = 0;
  best_h = 0;
  best_start = NULL;

  while (nitems > 0)
  {
    int w, h;
    gboolean replace;

    replace = FALSE;

    if (nitems < 3)
      return FALSE;		/* no space for w, h */

    w = data[0];
    h = data[1];

    if (nitems < ((w * h) + 2))
      break;			/* not enough data */

    if (best_start == NULL)
    {
      replace = TRUE;
    }
    else
    {
      /* work with averages */
      const int ideal_size = (ideal_width + ideal_height) / 2;
      int best_size = (best_w + best_h) / 2;
      int this_size = (w + h) / 2;

      /* larger than desired is always better than smaller */
      if (best_size < ideal_size && this_size >= ideal_size)
	      replace = TRUE;
      /* if we have too small, pick anything bigger */
      else if (best_size < ideal_size && this_size > best_size)
	      replace = TRUE;
      /* if we have too large, pick anything smaller
       * but still >= the ideal
       */
      else if (best_size > ideal_size &&
	       this_size >= ideal_size && this_size < best_size)
	      replace = TRUE;
    }

    if (replace)
    {
      best_start = data + 2;
      best_w = w;
      best_h = h;
    }

    data += (w * h) + 2;
    nitems -= (w * h) + 2;
  }

  if (best_start)
  {
    *start = best_start;
    *width = best_w;
    *height = best_h;
    return TRUE;
  }
  else
    return FALSE;
}

static void
argbdata_to_pixdata (gulong * argb_data, int len, guchar ** pixdata)
{
  guchar *p;
  int i;

  *pixdata = g_new (guchar, len * 4);
  p = *pixdata;

  /* One could speed this up a lot. */
  i = 0;
  while (i < len)
  {
    guint argb;
    guint rgba;

    argb = argb_data[i];
    rgba = (argb << 8) | (argb >> 24);

    *p = rgba >> 24;
    ++p;
    *p = (rgba >> 16) & 0xff;
    ++p;
    *p = (rgba >> 8) & 0xff;
    ++p;
    *p = rgba & 0xff;
    ++p;

    ++i;
  }
}

static gboolean
read_rgb_icon (Window xwindow,
	       int ideal_width,
	       int ideal_height,
	       int ideal_mini_width,
	       int ideal_mini_height,
	       int *width,
	       int *height,
	       guchar ** pixdata,
	       int *mini_width, int *mini_height, guchar ** mini_pixdata)
{
  Atom type;
  int format;
  gulong nitems;
  gulong bytes_after;
  int result, err;
  gulong *data;
  gulong *best;
  int w, h;
  gulong *best_mini;
  int mini_w, mini_h;

  _wnck_error_trap_push ();
  type = None;
  data = NULL;
  result = XGetWindowProperty (gdk_display,
			       xwindow,
			       _wnck_atom_get ("_NET_WM_ICON"),
			       0, G_MAXLONG,
			       False, XA_CARDINAL, &type, &format, &nitems,
			       &bytes_after, (void *) &data);

  err = _wnck_error_trap_pop ();

  if (err != Success || result != Success)
    return FALSE;

  if (type != XA_CARDINAL)
  {
    XFree (data);
    return FALSE;
  }

  if (!find_best_size (data, nitems,
		       ideal_width, ideal_height, &w, &h, &best))
  {
    XFree (data);
    return FALSE;
  }

  if (!find_best_size (data, nitems,
		       ideal_mini_width, ideal_mini_height,
		       &mini_w, &mini_h, &best_mini))
  {
    XFree (data);
    return FALSE;
  }

  *width = w;
  *height = h;

  *mini_width = mini_w;
  *mini_height = mini_h;

  argbdata_to_pixdata (best, w * h, pixdata);
  argbdata_to_pixdata (best_mini, mini_w * mini_h, mini_pixdata);

  XFree (data);

  return TRUE;
}

static void
free_pixels (guchar * pixels, gpointer data)
{
  g_free (pixels);
}

static void
get_pixmap_geometry (Pixmap pixmap, int *w, int *h, int *d)
{
  Window root_ignored;
  int x_ignored, y_ignored;
  guint width, height;
  guint border_width_ignored;
  guint depth;

  if (w)
    *w = 1;
  if (h)
    *h = 1;
  if (d)
    *d = 1;

  XGetGeometry (gdk_display,
		pixmap, &root_ignored, &x_ignored, &y_ignored,
		&width, &height, &border_width_ignored, &depth);

  if (w)
    *w = width;
  if (h)
    *h = height;
  if (d)
    *d = depth;
}

static GdkPixbuf *
apply_mask (GdkPixbuf * pixbuf, GdkPixbuf * mask)
{
  int w, h;
  int i, j;
  GdkPixbuf *with_alpha;
  guchar *src;
  guchar *dest;
  int src_stride;
  int dest_stride;

  w = MIN (gdk_pixbuf_get_width (mask), gdk_pixbuf_get_width (pixbuf));
  h = MIN (gdk_pixbuf_get_height (mask), gdk_pixbuf_get_height (pixbuf));

  with_alpha = gdk_pixbuf_add_alpha (pixbuf, FALSE, 0, 0, 0);

  dest = gdk_pixbuf_get_pixels (with_alpha);
  src = gdk_pixbuf_get_pixels (mask);

  dest_stride = gdk_pixbuf_get_rowstride (with_alpha);
  src_stride = gdk_pixbuf_get_rowstride (mask);

  i = 0;
  while (i < h)
  {
    j = 0;
    while (j < w)
    {
      guchar *s = src + i * src_stride + j * 3;
      guchar *d = dest + i * dest_stride + j * 4;

      /* s[0] == s[1] == s[2], they are 255 if the bit was set, 0
       * otherwise
       */
      if (s[0] == 0)
	      d[3] = 0;		/* transparent */
      else
	      d[3] = 255;		/* opaque */
      ++j;
    }

    ++i;
  }

  return with_alpha;
}

static GdkColormap *
get_cmap (GdkPixmap * pixmap)
{
  GdkColormap *cmap;

  cmap = gdk_drawable_get_colormap (pixmap);
  if (cmap)
    g_object_ref (G_OBJECT (cmap));

  if (cmap == NULL)
  {
    if (gdk_drawable_get_depth (pixmap) == 1)
    {
      /* try null cmap */
      cmap = NULL;
    }
    else
    {
      /* Try system cmap */
      GdkScreen *screen = gdk_drawable_get_screen (GDK_DRAWABLE (pixmap));
      cmap = gdk_screen_get_system_colormap (screen);
      g_object_ref (G_OBJECT (cmap));
    }
  }

  /* Be sure we aren't going to blow up due to visual mismatch */
  if (cmap &&
      (gdk_colormap_get_visual (cmap)->depth !=
       gdk_drawable_get_depth (pixmap)))
    cmap = NULL;

  return cmap;
}

static GdkPixbuf *
_wnck_gdk_pixbuf_get_from_pixmap (GdkPixbuf * dest,
				  Pixmap xpixmap,
				  int src_x,
				  int src_y,
				  int dest_x,
				  int dest_y, int width, int height)
{
  GdkDrawable *drawable;
  GdkPixbuf *retval;
  GdkColormap *cmap;

  retval = NULL;
  cmap = NULL;

  drawable = gdk_xid_table_lookup (xpixmap);

  if (drawable)
    g_object_ref (G_OBJECT (drawable));
  else
    drawable = gdk_pixmap_foreign_new (xpixmap);

  if (drawable)
  {
    cmap = get_cmap (drawable);

    /* GDK is supposed to do this but doesn't in GTK 2.0.2,
     * fixed in 2.0.3
     */
    if (width < 0)
      gdk_drawable_get_size (drawable, &width, NULL);
    if (height < 0)
      gdk_drawable_get_size (drawable, NULL, &height);

    retval = gdk_pixbuf_get_from_drawable (dest,
					   drawable,
					   cmap,
					   src_x, src_y,
					   dest_x, dest_y, width, height);
  }

  if (cmap)
    g_object_unref (G_OBJECT (cmap));
  if (drawable)
    g_object_unref (G_OBJECT (drawable));

  return retval;
}

static gboolean
try_pixmap_and_mask (Pixmap src_pixmap,
		     Pixmap src_mask,
		     GdkPixbuf ** iconp,
		     int ideal_width,
		     int ideal_height,
		     GdkPixbuf ** mini_iconp,
		     int ideal_mini_width, int ideal_mini_height)
{
  GdkPixbuf *unscaled = NULL;
  GdkPixbuf *mask = NULL;
  int w, h;

  if (src_pixmap == None)
    return FALSE;

  _wnck_error_trap_push ();

  get_pixmap_geometry (src_pixmap, &w, &h, NULL);

  unscaled = _wnck_gdk_pixbuf_get_from_pixmap (NULL,
					       src_pixmap, 0, 0, 0, 0, w, h);

  if (unscaled && src_mask != None)
  {
    get_pixmap_geometry (src_mask, &w, &h, NULL);
    mask = _wnck_gdk_pixbuf_get_from_pixmap (NULL,
					     src_mask, 0, 0, 0, 0, w, h);
  }

  _wnck_error_trap_pop ();

  if (mask)
  {
    GdkPixbuf *masked;

    masked = apply_mask (unscaled, mask);
    g_object_unref (G_OBJECT (unscaled));
    unscaled = masked;

    g_object_unref (G_OBJECT (mask));
    mask = NULL;
  }

  if (unscaled)
  {
    *iconp =
      gdk_pixbuf_scale_simple (unscaled,
			       ideal_width > 0 ? ideal_width :
			       gdk_pixbuf_get_width (unscaled),
			       ideal_height > 0 ? ideal_height :
			       gdk_pixbuf_get_height (unscaled),
			       GDK_INTERP_BILINEAR);
    *mini_iconp =
      gdk_pixbuf_scale_simple (unscaled,
			       ideal_mini_width > 0 ? ideal_mini_width :
			       gdk_pixbuf_get_width (unscaled),
			       ideal_mini_height > 0 ? ideal_mini_height :
			       gdk_pixbuf_get_height (unscaled),
			       GDK_INTERP_BILINEAR);

    g_object_unref (G_OBJECT (unscaled));
    return TRUE;
  }
  else
    return FALSE;
}

static void
get_kwm_win_icon (Window xwindow, Pixmap * pixmap, Pixmap * mask)
{
  Atom type;
  int format;
  gulong nitems;
  gulong bytes_after;
  Pixmap *icons;
  int err, result;

  *pixmap = None;
  *mask = None;

  _wnck_error_trap_push ();
  icons = NULL;
  result = XGetWindowProperty (gdk_display, xwindow,
			       _wnck_atom_get ("KWM_WIN_ICON"),
			       0, G_MAXLONG,
			       False,
			       _wnck_atom_get ("KWM_WIN_ICON"),
			       &type, &format, &nitems,
			       &bytes_after, (void *) &icons);

  err = _wnck_error_trap_pop ();
  if (err != Success || result != Success)
    return;

  if (type != _wnck_atom_get ("KWM_WIN_ICON"))
  {
    XFree (icons);
    return;
  }

  *pixmap = icons[0];
  *mask = icons[1];

  XFree (icons);

  return;
}

typedef enum
{
  /* These MUST be in ascending order of preference;
   * i.e. if we get _NET_WM_ICON and already have
   * WM_HINTS, we prefer _NET_WM_ICON
   */
  USING_NO_ICON,
  USING_FALLBACK_ICON,
  USING_KWM_WIN_ICON,
  USING_WM_HINTS,
  USING_NET_WM_ICON
} IconOrigin;

struct _WnckIconCache
{
  IconOrigin origin;
  Pixmap prev_pixmap;
  Pixmap prev_mask;
  GdkPixbuf *icon;
  GdkPixbuf *mini_icon;
  int ideal_width;
  int ideal_height;
  int ideal_mini_width;
  int ideal_mini_height;
  guint want_fallback:1;
  /* TRUE if these props have changed */
  guint wm_hints_dirty:1;
  guint kwm_win_icon_dirty:1;
  guint net_wm_icon_dirty:1;
};


static GdkPixbuf *
scaled_from_pixdata (guchar * pixdata, int w, int h, int new_w, int new_h)
{
  GdkPixbuf *src;
  GdkPixbuf *dest;

  src = gdk_pixbuf_new_from_data (pixdata,
				  GDK_COLORSPACE_RGB,
				  TRUE, 8, w, h, w * 4, free_pixels, NULL);

  if (src == NULL)
    return NULL;

  if (w != h)
  {
    GdkPixbuf *tmp;
    int size;

    size = MAX (w, h);

    tmp = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8, size, size);

    if (tmp != NULL)
    {
      gdk_pixbuf_fill (tmp, 0);
      gdk_pixbuf_copy_area (src, 0, 0, w, h,
			    tmp, (size - w) / 2, (size - h) / 2);

      if (src)
      	g_object_unref (src);
      src = tmp;
    }
  }

  if (w != new_w || h != new_h)
  {
    dest = gdk_pixbuf_scale_simple (src, new_w, new_h, GDK_INTERP_BILINEAR);

    g_object_unref (G_OBJECT (src));
  }
  else
  {
    dest = src;
  }

  return dest;
}

static gboolean
_wnck_read_icons_ (Window xwindow,
		   GtkWidget * icon_cache,
		   GdkPixbuf ** iconp,
		   int ideal_width,
		   int ideal_height,
		   GdkPixbuf ** mini_iconp,
		   int ideal_mini_width, int ideal_mini_height)
{
  guchar *pixdata;
  int w, h;
  guchar *mini_pixdata;
  int mini_w, mini_h;
  Pixmap pixmap;
  Pixmap mask;
  XWMHints *hints;

  *iconp = NULL;
  *mini_iconp = NULL;
  pixdata = NULL;
  if (read_rgb_icon (xwindow,
		     ideal_width, ideal_height,
		     ideal_mini_width, ideal_mini_height,
		     &w, &h, &pixdata, &mini_w, &mini_h, &mini_pixdata))
  {
    *iconp = scaled_from_pixdata (pixdata, w, h, ideal_width, ideal_height);

    *mini_iconp = scaled_from_pixdata (mini_pixdata, mini_w, mini_h,
				       ideal_mini_width, ideal_mini_height);

    return TRUE;
  }


  _wnck_error_trap_push ();
  hints = XGetWMHints (gdk_display, xwindow);
  _wnck_error_trap_pop ();
  pixmap = None;
  mask = None;
  if (hints)
  {
    if (hints->flags & IconPixmapHint)
      pixmap = hints->icon_pixmap;
    if (hints->flags & IconMaskHint)
      mask = hints->icon_mask;

    XFree (hints);
    hints = NULL;
  }


  if (try_pixmap_and_mask (pixmap, mask,
			   iconp, ideal_width, ideal_height,
			   mini_iconp, ideal_mini_width, ideal_mini_height))
  {
    return TRUE;
  }

  get_kwm_win_icon (xwindow, &pixmap, &mask);

  if (try_pixmap_and_mask (pixmap, mask,
			   iconp, ideal_width, ideal_height,
			   mini_iconp, ideal_mini_width, ideal_mini_height))
  {
    return TRUE;
  }
  return FALSE;
}


GdkPixbuf *
_wnck_get_icon_at_size (WnckWindow *window,
                        gint        width,
                        gint        height)
{
  GdkPixbuf *icon, *icon_scaled, *mini_icon;
  gboolean res;

  icon = NULL;
  icon_scaled = NULL;
  mini_icon = NULL;

  res = _wnck_read_icons_ (wnck_window_get_xid (window),
                           NULL,
                           &icon, width, width,
                           &mini_icon, 24, 24);

  if (res)
  {
    if (mini_icon)
      g_object_unref (mini_icon);

    if (icon)
      return icon;
  }

  icon = wnck_window_get_icon (window);
  icon_scaled = gdk_pixbuf_scale_simple (icon, width, height, 
                                         GDK_INTERP_BILINEAR);

  return icon_scaled;
}

/*
 * XUTILS_GET_NAMED_ICON
 *
 * Probably shouldn't be in here, but I'm not feeling like creating another
 * source header and file for one function. Sue me. Well don't actually.
 */

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
    pixbuf = gtk_icon_theme_load_icon (theme, "stock_folder",
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
xutils_get_named_icon (const gchar *icon_name, 
                       gint         width,
                       gint         height)
{
  return get_icon (icon_name, width);
}
