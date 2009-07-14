/*
 * Copyright (C) 2008,2009 Rodney Cryderman <rcryderman@gmail.com>
 * Copyright (C) 2008 Neil Jagdish Patel <njpatel@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Library General Public License version 
 * 2 or later as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by Neil Jagdish Patel <njpatel@gmail.com>
 *
 */

#include <glib/gstdio.h>
#include <string.h>
#include <gio/gio.h>

#include "awn-themed-icon.h"

#include "gseal-transition.h"
 
/**
 * SECTION:AwnThemedIcon
 * @short_description: A AwnIcon subclass that provides gtk themed icon support
 * @see_also: #AwnIcon, #AwnOverlaidIcon, #GtkIconTheme
 * @stability: Unstable
 * @include: libawn/libawn.h
 *
 * Provides convenient support for one or more themed icons using lists of 
 * of icon names / icon states.  Includes support of transparent (to applet)
 * modification of displayed icons through drag and drop.
 * 
 */

G_DEFINE_TYPE (AwnThemedIcon, awn_themed_icon, AWN_TYPE_ICON)

#define AWN_THEMED_ICON_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj),\
  AWN_TYPE_THEMED_ICON, \
  AwnThemedIconPrivate))

#define LOAD_FLAGS GTK_ICON_LOOKUP_FORCE_SIZE | GTK_ICON_LOOKUP_GENERIC_FALLBACK
#define AWN_ICON_THEME_NAME "awn-theme"
#define AWN_CHANGE_ICON_UI PKGDATADIR"/awn-themed-icon-ui.xml"


/*yes this is evil.  so sue me */
struct _GtkIconThemePrivate
{
  guint custom_theme        : 1;
  guint is_screen_singleton : 1;
  guint pixbuf_supports_svg : 1;
  guint themes_valid        : 1;
  guint check_reload        : 1;
  
  char *current_theme;
  char *fallback_theme;
  char **search_path;
  int search_path_len;

  /* A list of all the themes needed to look up icons.
   * In search order, without duplicates
   */
  GList *themes;
  GHashTable *unthemed_icons;
  
  /* Note: The keys of this hashtable are owned by the
   * themedir and unthemed hashtables.
   */
  GHashTable *all_icons;

  /* GdkScreen for the icon theme (may be NULL)
   */
  GdkScreen *screen;
  
  /* time when we last stat:ed for theme changes */
  long last_stat_time;
  GList *dir_mtimes;

  gulong reset_styles_idle;
};


typedef struct
{
  gchar * name;
  gchar * state;
  gchar * original_name;  /*do we need this ? */
  gboolean  sticky;
}AwnThemedIconItem;

struct _AwnThemedIconPrivate
{
  GtkIconTheme *awn_theme;
  GtkIconTheme *override_theme;
  GtkIconTheme *gtk_theme;
  gchar        *icon_dir;
  
  gchar  *applet_name;
  gchar  *uid;

  GList   *list;
  
  gint    current_size;
  AwnThemedIconItem    *current_item;
  GdkPixbufRotation    rotate;
  
  gulong  sig_id_for_gtk_theme;
  gulong  sig_id_for_awn_theme;  
  

};

typedef struct
{
  AwnThemedIcon * icon;
  gint            size;
  gchar         * state;
}AwnThemedIconPreloadItem;

enum
{
  SCOPE_UID=0,
  SCOPE_APPLET,
  SCOPE_AWN_THEME,
  SCOPE_OVERRIDE_THEME,
  SCOPE_GTK_THEME,
  SCOPE_FILENAME,
  SCOPE_FALLBACK_STOP,
  SCOPE_FALLBACK_FILL,

  N_SCOPES
};

static const GtkTargetEntry drop_types[] =
{
  { (gchar*)"text/uri-list", 0, 0}
};
static const gint n_drop_types = G_N_ELEMENTS(drop_types);

/* Forwards */
static GdkPixbuf* theme_load_icon (GtkIconTheme *icon_theme,
                                     const gchar *icon_name,
                                     gint size,
                                     GtkIconLookupFlags flags,
                                     GError **error);

void on_icon_theme_changed              (GtkIconTheme     *theme, 
                                         AwnThemedIcon     *icon);

static gboolean on_idle_preload (gpointer item);

void awn_themed_icon_drag_data_received (GtkWidget        *widget, 
                                         GdkDragContext   *context,
                                         gint              x, 
                                         gint              y, 
                                         GtkSelectionData *selection,
                                         guint             info,
                                         guint             evt_time);

static void ensure_icon                 (AwnThemedIcon *icon);

static void awn_themed_icon_preload_all (AwnThemedIcon * icon);

enum
{
  PROP_0,
  PROP_ROTATE,
  PROP_APPLET_NAME
};

static GtkIconTheme*
get_awn_theme(void)
{
  static GtkIconTheme *awn_theme = NULL;

  if (!awn_theme)
  {
    awn_theme = gtk_icon_theme_new ();
    gtk_icon_theme_set_custom_theme (awn_theme, AWN_ICON_THEME_NAME);    

    /* gtk_icon_theme_set_search_path() makes a copy.
     */
    gchar * path[1];    
    path[0] = g_strdup_printf ("%s/.icons", g_get_home_dir ());
    gtk_icon_theme_set_custom_theme (awn_theme, AWN_ICON_THEME_NAME);
    gtk_icon_theme_set_search_path (awn_theme, (const gchar **)path, 1);
    g_free (path[0]);
  }    
  return awn_theme;
}

/*------------------pixbuf caching------  
 Possible candidate for a public API.
 Just testing how well the basic approach works for the moment.  Will
 refactor to a small gobject, or something a bit nicer,
 if it works well enough.
 
 Notes:
 scope probably isn't necessary... if the theme_name is always provided.
 */
static  GHashTable *pixbufs;    /*our pixbuf cache*/

static void
add_pixbuf_to_cache (GdkPixbuf * pixbuf,const gchar * scope, 
            const gchar * theme_name, 
            const gchar * icon_name, 
            gint  size)
{
  gchar * key;
   
  g_return_if_fail (GDK_IS_PIXBUF(pixbuf));
  g_return_if_fail (icon_name);
  if (!pixbufs)
  {
    pixbufs = g_hash_table_new_full (g_str_hash,g_str_equal, g_free, g_object_unref);  
  }
  /*Conditional operator*/
  key = g_strdup_printf ("%s::%s::%s::%d",
                         scope?scope:"__NONE__",
                         theme_name?theme_name:"__NONE__",
                         icon_name,
                         size);
  g_hash_table_insert (pixbufs, key, pixbuf);  
  g_object_ref (pixbuf);  
}

static GdkPixbuf * 
lookup_pixbuf (const gchar * scope, const gchar * theme_name, 
               const gchar * icon_name, gint  size)
{
  GdkPixbuf *pixbuf;
  gchar * key;
  g_return_val_if_fail (icon_name,NULL);
  if (!pixbufs)
  {
    pixbufs = g_hash_table_new_full (g_str_hash,g_str_equal, g_free, g_object_unref);  
  }
  /*Conditional operator*/
  key = g_strdup_printf ("%s::%s::%s::%d",
                         scope?scope:"__NONE__",
                         theme_name?theme_name:"__NONE__",
                         icon_name,
                         size);
  pixbuf = g_hash_table_lookup (pixbufs,key);
//  g_debug ("Cache lookup: %s for %s",pixbuf?"Hit":"Miss",key);
  g_free (key);  
  return pixbuf;
}

#if 0
static void
invalidate_cache(void)
{
  g_debug ("CACHE INVALIDATE!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
//  g_hash_table_remove_all (pixbufs);
}

#endif
/*End of pixbuf caching functions */

/* GObject stuff */

static void
awn_themed_icon_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
  AwnThemedIconPrivate * priv;
  priv = AWN_THEMED_ICON_GET_PRIVATE (object);

  switch (property_id) 
  {
    case PROP_ROTATE:
      g_value_set_enum (value,priv->rotate);
      break;
    case PROP_APPLET_NAME:
      g_value_set_string (value,priv->applet_name);
      break;      
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
awn_themed_icon_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
  AwnThemedIconPrivate * priv;
  priv = AWN_THEMED_ICON_GET_PRIVATE (object);

  switch (property_id) 
  {
    case PROP_ROTATE:
      priv->rotate = g_value_get_enum (value);
      ensure_icon (AWN_THEMED_ICON(object));
      break;
    case PROP_APPLET_NAME:
      awn_themed_icon_set_applet_info (AWN_THEMED_ICON(object),
                                       g_value_get_string (value),priv->uid);
      break;      
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}


static void
awn_themed_icon_dispose (GObject *object)
{
  AwnThemedIconPrivate *priv;

  g_return_if_fail (AWN_IS_THEMED_ICON (object));
  priv = AWN_THEMED_ICON (object)->priv;
  
  if (priv->sig_id_for_awn_theme)
  {
    g_signal_handler_disconnect (priv->awn_theme,priv->sig_id_for_awn_theme);
    priv->sig_id_for_awn_theme = 0; 
  }
  if (G_IS_OBJECT (priv->override_theme))
  {
    g_object_unref (priv->override_theme);
    priv->override_theme = NULL;
  }
  if (priv->sig_id_for_gtk_theme)
  {
    g_signal_handler_disconnect (priv->gtk_theme,priv->sig_id_for_gtk_theme);
    priv->sig_id_for_gtk_theme = 0; 
  }
  
  G_OBJECT_CLASS (awn_themed_icon_parent_class)->dispose (object);
}

static void
awn_themed_icon_finalize (GObject *object)
{
  AwnThemedIconPrivate *priv;

  g_return_if_fail (AWN_IS_THEMED_ICON (object));
  priv = AWN_THEMED_ICON (object)->priv;

  g_list_foreach (priv->list, (GFunc) g_free,NULL);
  g_list_free (priv->list);
  
  g_free (priv->applet_name);            
  priv->applet_name = NULL;
  g_free (priv->uid);                    
  priv->uid = NULL;
  g_free (priv->icon_dir);               
  priv->icon_dir = NULL;
  
  G_OBJECT_CLASS (awn_themed_icon_parent_class)->finalize (object);  
}


static void
awn_themed_icon_class_init (AwnThemedIconClass *klass)
{
  GObjectClass   *obj_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *wid_class = GTK_WIDGET_CLASS (klass);
  GParamSpec   *pspec;        
  
  obj_class->get_property = awn_themed_icon_get_property;
  obj_class->set_property = awn_themed_icon_set_property;
  obj_class->dispose = awn_themed_icon_dispose;
  obj_class->finalize = awn_themed_icon_finalize;  
  
  wid_class->drag_data_received = awn_themed_icon_drag_data_received;

  pspec = g_param_spec_enum ("rotate",
                               "Rotate",
                               "Rotate",
                               GDK_TYPE_PIXBUF_ROTATION,
                               GDK_PIXBUF_ROTATE_NONE,
                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
  g_object_class_install_property (obj_class, PROP_ROTATE, pspec);   

  pspec = g_param_spec_string ("applet-name",
                             "Applet Name",
                             "Applet Name",
                             NULL,
                             G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
  g_object_class_install_property (obj_class, PROP_APPLET_NAME, pspec);   
  
  g_type_class_add_private (obj_class, sizeof (AwnThemedIconPrivate));
}

static void 
check_dest_or_copy (const gchar *src, const gchar *dest)
{
  GFile  *from;
  GFile  *to;
  GError *error = NULL;

  if (g_file_test (dest, G_FILE_TEST_EXISTS))
    return;

  from = g_file_new_for_path (src);
  to = g_file_new_for_path (dest);

  g_file_copy (from, to, 0, NULL, NULL, NULL, &error);

  if (error)
  {
    g_warning ("Unable to copy %s to %s: %s", src, dest, error->message);
    g_error_free (error);
  }

  g_object_unref (to);
  g_object_unref (from);
}

static void
check_and_make_dir (const gchar *dir)
{
  if (!g_file_test (dir, G_FILE_TEST_EXISTS))
  {
    g_mkdir (dir, 0755);
  }
}

static void
awn_themed_icon_init (AwnThemedIcon *icon)
{
  AwnThemedIconPrivate *priv;
  gchar                *icon_dir;
  gchar                *theme_dir;
  gchar                *scalable_dir;
  gchar                *index_src;
  gchar                *index_dest;

  priv = icon->priv = AWN_THEMED_ICON_GET_PRIVATE (icon);

  priv->applet_name = NULL;
  priv->uid = NULL;
  priv->list = NULL;
  priv->current_item = NULL;
  priv->current_size = 48;
 

  /* Set-up the gtk-theme */
  priv->gtk_theme = gtk_icon_theme_get_default ();
  priv->sig_id_for_gtk_theme = g_signal_connect (priv->gtk_theme, "changed", 
                    G_CALLBACK (on_icon_theme_changed), icon);
  
  /*Calling this with the default icon theme (which contains hicolor dirs)
   to supress an irritating gtk warning that occurs if the first time we try 
   to get a themed icon a GtkIconTheme is used that does not contain
   hicolor dirs. It shouldn't find a icon and even if it does... we don't
   care.*/
  theme_load_icon (priv->gtk_theme,"gtk_knows_best",16,0,NULL);
  
  /* 
   * Set-up our special theme. We need to check for all the dirs
   */

  /* First check all the directories */
  icon_dir = priv->icon_dir = g_strdup_printf ("%s/.icons", g_get_home_dir ());
  check_and_make_dir (icon_dir);

  theme_dir = g_strdup_printf ("%s/%s", icon_dir, AWN_ICON_THEME_NAME);
  check_and_make_dir (theme_dir);

  scalable_dir = g_strdup_printf ("%s/scalable", theme_dir);
  check_and_make_dir (scalable_dir);
  
  /* Copy over the index.theme if it's not already done */
  index_src = g_strdup (PKGDATADIR"/index.theme");
  index_dest = g_strdup_printf ("%s/index.theme", theme_dir);
  check_dest_or_copy (index_src, index_dest);
  g_free (index_dest);

  g_free (index_src);  
  /* Now let's make our custom theme */
  priv->awn_theme = get_awn_theme ();
  priv->sig_id_for_awn_theme = g_signal_connect (priv->awn_theme, "changed", 
                    G_CALLBACK (on_icon_theme_changed), icon);
  
  g_free (scalable_dir);
  g_free (theme_dir);
//  g_free (hicolor_dir);
}

/**
 * awn_themed_icon_new:
 *
 * Creates a new instance of #AwnThemedIcon.  
 * Returns: an instance of #AwnThemedIcon
 */

GtkWidget *
awn_themed_icon_new (void)
{
  GtkWidget *themed_icon = NULL;

  themed_icon = g_object_new (AWN_TYPE_THEMED_ICON, 
                              NULL);

  return themed_icon;
}

/*
 * Main function to get the correct icon for a size
 */

/*
 * This is a special case for .desktop files. It means we can use AwnThemedIcon
 * in the taskmananger
 */
static GdkPixbuf *
try_and_load_image_from_disk (const gchar *filename, gint size)
{
  GdkPixbuf *pixbuf = NULL;
  gchar *temp;

  /* Try straight file loading */
  pixbuf = gdk_pixbuf_new_from_file_at_scale (filename, size, size, TRUE, NULL);
  if (pixbuf)
    return pixbuf;

  /* Try loading from /usr/share/pixmaps */
  temp = g_build_filename ("/usr/share/pixmaps", filename, NULL);
  pixbuf = gdk_pixbuf_new_from_file_at_scale (filename, size, size, TRUE, NULL);
  if (pixbuf)
  {
    g_free (temp);
    return pixbuf;
  }
  g_free (temp);

  /* Try from /usr/local/share/pixmaps */
  temp = g_build_filename ("/usr/local/share/pixmaps", filename, NULL);
  pixbuf = gdk_pixbuf_new_from_file_at_scale (filename, size, size, TRUE, NULL);
  
  g_free (temp);
  return pixbuf;
} 


/* 
 This function exists because gtk_icon_theme_load_icon() seems to insist 
 on returning crappy builtin icons in certain situation  even when 
 GTK_ICON_LOOKUP_USE_BUILTIN is not one of the flags.  Obviously I must be 
 misunderstanding something as this bug _couldn't_ have been missed...  but this
 seems to make the "crappy" icons go bye bye.
 
 This issue may have been related to custom themes including system dirs 
 containing the hicolor theme in the search path...  test later.  TODO
 */
static GdkPixbuf* 
theme_load_icon (GtkIconTheme *icon_theme,
                                     const gchar *icon_name,
                                     gint size,
                                     GtkIconLookupFlags flags,
                                     GError **error)
{
  const gchar * names[2]={NULL,NULL};
  names[0] = icon_name;
  GtkIconInfo*  info = gtk_icon_theme_choose_icon (icon_theme,
                                                   names,
                                                   size,
                                                   flags);
  if (info)
  {
    GdkPixbuf *pbuf = gtk_icon_info_load_icon (info,error);
    gtk_icon_info_free( info);
    return pbuf;
  }
  return NULL;
}

/*FIXME  Big function */

static GdkPixbuf *
get_pixbuf_at_size (AwnThemedIcon *icon, gint size, const gchar *state)
{
  AwnThemedIconPrivate *priv;
  GdkPixbuf            *pixbuf = NULL;
  GList                 *iter;

  priv = icon->priv;

  /* Find the index of the current state in states */
  g_return_val_if_fail(priv-> list,NULL);	
  for (iter = priv->list; iter; iter=g_list_next (iter))
  {
    AwnThemedIconItem *item = iter->data;
    /*Conditional Operator */
    if (g_strcmp0 (item->state, state ? state : priv->current_item->state) == 0)
    {
      const gchar *applet_name;
      const gchar *icon_name;
      const gchar *uid;
      gint         i;
      
      applet_name = priv->applet_name;
      icon_name = item->name;
      uid = priv->uid;
      
      /* Go through all the possible outcomes until we get a pixbuf */
      for (i = 0; i < N_SCOPES; i++)
      {
        gchar *name = NULL;
        switch (i)
        {
          case SCOPE_UID:
            name = g_strdup_printf ("%s-%s-%s", icon_name, applet_name, uid);
            pixbuf = lookup_pixbuf ("scope_uid",
                                    "awn-theme",
                                    name,
                                    size);
            if (pixbuf)
            {
              g_object_ref (pixbuf);
              break;
            }
            pixbuf = theme_load_icon (priv->awn_theme, name,
                                             size, LOAD_FLAGS, NULL);
            if (pixbuf)
            {
              add_pixbuf_to_cache (pixbuf ,"scope_uid",
                                  "awn-theme",
                                  name,
                                  size);
            }
            break;

          case SCOPE_APPLET:
            name = g_strdup_printf ("%s-%s", icon_name, applet_name);
            pixbuf = lookup_pixbuf ("scope_applet",
                                    "awn-theme",
                                    name,
                                    size);
            if (pixbuf)
            {
              g_object_ref (pixbuf);
              break;
            }
            pixbuf = theme_load_icon (priv->awn_theme, name,
                                             size, LOAD_FLAGS, NULL);
            if (pixbuf)
            {
              add_pixbuf_to_cache (pixbuf ,"scope_applet",
                                  "awn-theme",
                                  name,
                                  size);
            }
            break;

          case SCOPE_AWN_THEME:
            pixbuf = lookup_pixbuf ("scope_awn_theme",
                                    "awn-theme",
                                    icon_name,
                                    size);            
            if (pixbuf)
            {
              g_object_ref (pixbuf);
              break;
            }
            pixbuf = theme_load_icon (priv->awn_theme, icon_name, 
                                             size, LOAD_FLAGS, NULL);
            if (pixbuf)
            {
              add_pixbuf_to_cache (pixbuf ,"scope_awn_theme",
                                  "awn-theme",
                                  icon_name,
                                  size);
            }
            break;

          case SCOPE_OVERRIDE_THEME:
            pixbuf = NULL;
            if (priv->override_theme)
            {
              pixbuf = lookup_pixbuf ("scope_override_theme",
                                    priv->override_theme->priv->current_theme,
                                    icon_name,
                                    size);              
              if (pixbuf)
              {
                g_object_ref (pixbuf);
                break;
              }
              pixbuf = theme_load_icon (priv->override_theme,
                                               icon_name, 
                                               size, LOAD_FLAGS, NULL);
              if (pixbuf)
              {
                add_pixbuf_to_cache (pixbuf ,"scope_override_theme",
                                  priv->override_theme->priv->current_theme,
                                  icon_name,
                                  size);
              }                
            }
            break;

          case SCOPE_GTK_THEME:
            pixbuf = lookup_pixbuf ("scope_gtk_theme",
                                    priv->gtk_theme->priv->current_theme,
                                    icon_name,
                                    size);            
            if (pixbuf)
            {
              g_object_ref (pixbuf);
              break;
            }
            pixbuf = theme_load_icon (priv->gtk_theme, icon_name,
                                             size, LOAD_FLAGS, NULL);
            if (pixbuf)
            {
              add_pixbuf_to_cache (pixbuf ,"scope_gtk_theme",
                                  priv->gtk_theme->priv->current_theme,
                                  icon_name,
                                  size);
            }              
            break;

          case SCOPE_FILENAME:
            pixbuf = NULL;
            if (priv->current_item->original_name)
            {
              pixbuf = lookup_pixbuf ("scope_filename",
                                    NULL,
                                    icon_name,
                                    size);              
              if (pixbuf)
              {
                g_object_ref (pixbuf);
                break;
              }
              pixbuf = try_and_load_image_from_disk (priv->current_item->original_name, 
                                                   size);
              if (pixbuf)
              {
                add_pixbuf_to_cache (pixbuf ,"scope_filename",
                                  NULL,
                                  icon_name,
                                  size);
              }                
            }
            break;

          case SCOPE_FALLBACK_STOP:
            pixbuf = lookup_pixbuf ("scope_fallback_stop",
                                    priv->gtk_theme->priv->current_theme,
                                    GTK_STOCK_MISSING_IMAGE,
                                    size);            
            if (pixbuf)
            {
              g_object_ref (pixbuf);
              break;
            }
            pixbuf = theme_load_icon (priv->gtk_theme,
                                             GTK_STOCK_MISSING_IMAGE,
                                             size, LOAD_FLAGS, NULL);
            if (pixbuf)
            {
              add_pixbuf_to_cache (pixbuf ,"scope_fallback_stop",
                                  priv->gtk_theme->priv->current_theme,
                                  GTK_STOCK_MISSING_IMAGE,
                                  size);
            }
            break;

          default:
            pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8, size, size);
            gdk_pixbuf_fill (pixbuf, 0xee221155);
            break;
        }

        /* Check if we got a valid pixbuf on this run */
        g_free (name);

        if (pixbuf)
        {
          /* FIXME: Should we make this orientation-aware? */
          if (gdk_pixbuf_get_height (pixbuf) > size)
          {
            GdkPixbuf *temp = pixbuf;
            gint       width, height;

            width = gdk_pixbuf_get_width (temp);
            height = gdk_pixbuf_get_height (temp);

            pixbuf = gdk_pixbuf_scale_simple (temp, width*size/height, size,
                                              GDK_INTERP_HYPER);
            g_object_unref (temp);
          }
          return pixbuf;
        }
      }
      
    }
  }
  /* Yes.. this is drastic... asserts should be disabled for releases.
   This just plain and simple should _not trigger*/
  g_warning ("%s: state '%s' not present in list",__func__,state);
  return pixbuf;
}


/*
 * Main function to ensure the icon
 */
static void
ensure_icon (AwnThemedIcon *icon)
{
  AwnThemedIconPrivate *priv;
  GdkPixbuf            *pixbuf;

  priv = icon->priv;

  if (!priv->list || !priv->current_item || !priv->current_size)
  {
    /* We're not ready yet */
    return;
  }
  /* Get the icon first */
  pixbuf = get_pixbuf_at_size (icon, priv->current_size, priv->current_item->state);

  if (priv->rotate)
  {
    GdkPixbuf * rotated;
    rotated = gdk_pixbuf_rotate_simple (pixbuf,priv->rotate);
    g_object_unref (pixbuf);
    pixbuf = rotated;
  }
  awn_icon_set_from_pixbuf (AWN_ICON (icon), pixbuf);

  g_object_unref (pixbuf);
}

/*
 * Public functions
 */

/**
 * awn_themed_icon_set_state:
 * @icon: A pointer to an #AwnThemedIcon object.
 * @state: The icon state that is to be enabled.
 *
 * Switches to the icon state specificed.  This will switch the displayed icon
 * to the corresponding themed icon.
 */

void  
awn_themed_icon_set_state (AwnThemedIcon *icon,
                           const gchar   *state)
{
  AwnThemedIconPrivate *priv;
  GList *  iter;

  g_return_if_fail (AWN_IS_THEMED_ICON (icon));
  g_return_if_fail (state);
  
  priv = icon->priv;
  
  for (iter = priv->list; iter; iter=g_list_next (iter))
  {
    AwnThemedIconItem *item = iter->data;
    /*Conditional Operator */

    if (g_strcmp0 (item->state, state ) == 0)
    {
      priv->current_item = item;
      ensure_icon (icon);
      return;
    }
  }
}

/**
 * awn_themed_icon_get_state:
 * @icon: A pointer to an #AwnThemedIcon object.
 *
 * Get the current icon state of the #AwnThemedIcon.
 * Returns: a pointer to the current icon state string.
 */

const gchar *
awn_themed_icon_get_state (AwnThemedIcon *icon)
{
  g_return_val_if_fail (AWN_IS_THEMED_ICON (icon), NULL);

  return icon->priv->current_item->state;
}


static void
awn_themed_icon_preload_all (AwnThemedIcon * icon)
{
  AwnThemedIconPrivate *priv;  
  GList *iter;
  
  priv = icon->priv;
  /*preload these icons */
  for (iter = priv->list; iter ; iter = g_list_next (iter) )
  {    
    AwnThemedIconItem * item = iter->data;
    awn_themed_icon_preload_icon (icon, item->state, -1);
  }
}

/**
 * awn_themed_icon_set_size:
 * @icon: A pointer to an #AwnThemedIcon object.
 * @size: An icon size
 *
 * Set the Icon size.
 */

void 
awn_themed_icon_set_size (AwnThemedIcon *icon,
                          gint           size)
{
  AwnThemedIconPrivate *priv;
  g_return_if_fail (AWN_IS_THEMED_ICON (icon));

  priv = icon->priv;
  if (priv->current_size != size)
  {
    priv->current_size = size;  
    ensure_icon (icon);
    awn_themed_icon_preload_all ( icon);    
  }    
  
}

/**
 * awn_themed_icon_get_size:
 * @icon: A pointer to an #AwnThemedIcon object.
 *
 * Get the current icon size.
 * Returns: the current icon size.
 */
gint
awn_themed_icon_get_size (AwnThemedIcon *icon)
{
  g_return_val_if_fail (AWN_IS_THEMED_ICON (icon), 0);

  return icon->priv->current_size;
}

/*
 * Check if any of the icon names have a slash in them, if they do, then
 * replace the slash with a '-' and copy the original names for the file
 * loader
 */
static gchar *
normalise_name (const gchar *names)
{
  gchar * normalized = g_strdup(names);
  gint j;

  for (j = 0; normalized[j]; j++)
  {
    if (normalized[j] == '/')
    {
      normalized[j] = '-';
    }
  }
  return normalized;
}

/**
 * awn_themed_icon_set_info:
 * @icon: A pointer to an #AwnThemedIcon object.
 * @applet_name: The applet name.
 * @uid: The applet's UID.
 * @states: A NULL terminated list of icon states.
 * @icon_names: A NULL terminated list of theme icon names that corresponds to
 * the states arra.
 *
 * Sets a list of Icon names and Icon states for the specific Applet name / UID 
 * instance.
 */

void
awn_themed_icon_set_info (AwnThemedIcon  *icon,
                          const gchar    *applet_name,
                          const gchar    *uid,
                          GStrv          states,
                          GStrv          icon_names)
{
  AwnThemedIconPrivate *priv;
  GList * iter;
  gint  n_states;
  gint  i;
  gchar * state = NULL;
  
  g_return_if_fail (AWN_IS_THEMED_ICON (icon));
  g_return_if_fail (applet_name);
  g_return_if_fail (uid);
  g_return_if_fail (states);
  g_return_if_fail (icon_names);
  priv = icon->priv;
  /*clear out the non-sticky*/
  for (iter = priv->list; iter; iter=g_list_next(iter) )
  {
    AwnThemedIconItem * item = iter->data;
    if (!item->sticky)
    {
      if (priv->current_item == item)
      {
        state = g_strdup (priv->current_item->state);
        priv->current_item = NULL;
      }
      g_free (item->name);
      g_free (item->original_name);
      g_free (item->state);
      g_free (item);
      priv->list = g_list_delete_link (priv->list, iter);
      iter = priv->list;  /* FIXME*/
    }
  }
  
  /* Check number of strings>0 and n_strings (states)==n_strings (icon_names)*/
  n_states = g_strv_length (states);
  if ( n_states && (g_strv_length (icon_names) != g_strv_length (states)) )
  {
    g_warning ("%s", n_states ? 
                       "Length of states must match length of icon_names" 
                       : "Length of states must be greater than 0");
    return;
  }
  
  /* Copy states & icon_names into list */
  for (i=0;i<n_states;i++)
  {
    AwnThemedIconItem * item = g_malloc (sizeof(AwnThemedIconItem) );
    item->original_name = g_strdup (icon_names[i]);
    item->state = g_strdup (states[i]);
    item->sticky = FALSE;
    item->name = normalise_name (icon_names[i]);
    priv->list = g_list_append (priv->list, item);
  }
  
  /* Now add the rest of the entries */
  g_free (priv->uid);
  priv->uid = g_strdup (uid);

  /* Finally set-up the applet name & theme information */
  if (priv->applet_name && strcmp (priv->applet_name, applet_name) == 0)
  {
    /* Already appended the search path to the GtkIconTheme, so we skip this */
  }
  else
  {
    /* FIXME... I'm thinking there's a slight problem here if the applet name 
     gets changed... which doesn't really make sense in the first place*/
    gchar *search_dir;

    g_free (priv->applet_name);
    priv->applet_name = g_strdup (applet_name);

    /* Add the applet's system-wide icon dir first */ 
    search_dir = g_strdup_printf (PKGDATADIR"/applets/%s/icons", applet_name);
    gtk_icon_theme_append_search_path (priv->gtk_theme, search_dir);
    g_free (search_dir);

    search_dir = g_strdup_printf (PKGDATADIR"/applets/%s/themes", applet_name);
    
    gtk_icon_theme_append_search_path (priv->gtk_theme, search_dir);
    g_free (search_dir); 
  }
  if (state)
  {
    awn_themed_icon_set_state (icon,state);
    g_free (state);
  }
  
  ensure_icon(icon);
  /*
   * Initate drag_drop  
   */
  gtk_drag_dest_unset (GTK_WIDGET(icon));
  
  for (iter = priv->list; iter; iter = g_list_next (iter) )
  {
    AwnThemedIconItem *item = iter->data;
    if (g_strstr_len (item->state,-1, "::invisible::") !=  item->state)
    {
      gtk_drag_dest_set (GTK_WIDGET (icon),
                     GTK_DEST_DEFAULT_MOTION | GTK_DEST_DEFAULT_DROP,
                     drop_types, n_drop_types,
                     GDK_ACTION_COPY | GDK_ACTION_ASK);
      break;
    }
  }
  /*preload these icons.  we could awn_themed_icon_preload_all() but this
   is a bit more efficient since we know what was added...*/
  for (i=0;i<n_states;i++)
  {    
    awn_themed_icon_preload_icon (icon, states[i], -1);
  }
}

/**
 * awn_themed_icon_set_info_simple:
 * @icon: A pointer to an #AwnThemedIcon object.
 * @applet_name: The applet name.
 * @uid: The applet's UID.
 * @icon_name: A themed icon name.
 *
 * Sets icon name for a specific Applet name / UID instance.  Used for Icons
 * that only have one icon. 
 */

void
awn_themed_icon_set_info_simple (AwnThemedIcon  *icon,
                                 const gchar    *applet_name,
                                 const gchar    *uid,
                                 const gchar    *icon_name)
{
  gchar *states[]   = { (gchar*)"__SINGULAR__", NULL };
  gchar *icon_names[] = { NULL, NULL };

  g_return_if_fail (AWN_IS_THEMED_ICON (icon));

  icon_names[0] = (gchar*)icon_name;
  
  awn_themed_icon_set_info (icon, applet_name, uid, states, icon_names);

  icon_names[0] = NULL;

  /* Set the state to __SINGULAR__, to keeps things easy for simple applets */
  awn_themed_icon_set_state (icon, states[0]);
}

/**
 * awn_themed_icon_set_info_append:
 * @icon: A pointer to an #AwnThemedIcon object.
 * @state: An Icon state.  
 * @icon_names: A icon name.
 *
 * Appends a icon state/ icon name pair to the existing list of themed icons.
 */

void
awn_themed_icon_set_info_append (AwnThemedIcon  *icon,
                                 const gchar    * state,                                 
                                 const gchar    *icon_name)
{
  /*  
   FIXME  This function needs to have some sanity imposed.
   */
  AwnThemedIconPrivate *priv;  
  AwnThemedIconItem *item = NULL;

  g_return_if_fail (icon_name);
  g_return_if_fail (AWN_IS_THEMED_ICON (icon));
  if (state)
  {
    g_return_if_fail (strlen(state) != 0);
  }
  priv = icon->priv;

  /*FIXME once we get a applet name into AwnApplet*/
  if (!priv->applet_name)
  {
    priv->applet_name = g_strdup( "__unknown__" );
  }

  if (!priv->uid)
  {
    priv->uid = g_strdup ("__invisible__");  
  }
  
  item = g_malloc (sizeof(AwnThemedIconItem));
  
  item->original_name = g_strdup (icon_name);
  item->name = normalise_name (icon_name);
  item->state = g_strdup (state?state:"::invisible::unknown");
  item->sticky = FALSE;
      
  priv->list = g_list_append (priv->list,item);
}

/**
 * awn_themed_icon_set_info_append:
 * @icon: A pointer to an #AwnThemedIcon object.
 * @applet_name: The applet name.  
 * @uid: The UID of the applet instance.
 *
 * Sets the applet name / uid pair for the icon.  If an existing applet 
 * name has previously been set then the value will not be modified.
 */
void
awn_themed_icon_set_applet_info (AwnThemedIcon  *icon,
                                 const gchar    *applet_name,
                                 const gchar    *uid)
{
  AwnThemedIconPrivate *priv;
  priv = icon->priv;
  
  g_free (priv->uid);
  priv->uid = g_strdup (uid);

  /* Finally set-up the applet name & theme information */
  if (priv->applet_name && strcmp (priv->applet_name, applet_name) == 0)
  {
    /* Already appended the search path to the GtkIconTheme, so we skip this */
  }
  else
  {
    gchar *search_dir;

    g_free (priv->applet_name);
    priv->applet_name = g_strdup (applet_name);

    /* Add the applet's system-wide icon dir first */
    search_dir = g_strdup_printf (PKGDATADIR"/applets/%s/icons", applet_name);
    gtk_icon_theme_append_search_path (priv->gtk_theme, search_dir);
    g_free (search_dir);

    search_dir = g_strdup_printf (PKGDATADIR"/applets/%s/themes", applet_name);
    gtk_icon_theme_append_search_path (priv->gtk_theme, search_dir);
    g_free (search_dir); 
  }  
}

/**
 * awn_themed_icon_override_gtk_theme:
 * @icon: A pointer to an #AwnThemedIcon object.
 * @theme_name: A icon theme name.
 *
 * Overrides the default icon theme with a different icon theme.
 */

void
awn_themed_icon_override_gtk_theme (AwnThemedIcon *icon,
                                    const gchar   *theme_name)
{
  AwnThemedIconPrivate *priv;
  gchar *search_dir;

  g_return_if_fail (AWN_IS_THEMED_ICON (icon));
  
  priv = icon->priv;
  /* Remove old theme, if it exists */
  if (priv->override_theme)
    g_object_unref (priv->override_theme);

  if ( theme_name && strlen (theme_name) )
  {
    priv->override_theme = gtk_icon_theme_new ();
    gtk_icon_theme_set_custom_theme (priv->override_theme, theme_name);
  }
  else
  {
    priv->override_theme = NULL;
  }


  /* Add the applet's system-wide icon dir first 
   
   TODO instead appending the override theme to the search path it's actually
   necessary to set to just those dirs as is being done with awn_theme FIXME
   */
  if (priv->override_theme)
  {
    gchar ** path;
    gint n_elements;
    
    if (priv->applet_name)
    {
      search_dir = g_strdup_printf (PKGDATADIR"/applets/%s/icons", priv->applet_name);
      gtk_icon_theme_append_search_path (priv->override_theme, search_dir);
      g_free (search_dir);

      search_dir = g_strdup_printf (PKGDATADIR"/applets/%s/themes", priv->applet_name);
      gtk_icon_theme_append_search_path (priv->override_theme, search_dir);
      g_free (search_dir); 
    }
    else
    {
      g_warning ("%s: applet_name not set. Unable to set applet specific icon theme dirs",
                 __func__);
    }
    /*strip out hicolor dirs from the search path*/
    gtk_icon_theme_get_search_path (priv->override_theme,&path,&n_elements);
    if (path)
    {
      gint i;
      gint removed = 0;
      for (i=0; i<n_elements;i++)
      {
        gchar * search;
        search = g_strstr_len (path[i],-1,"hicolor");
        if (search)
        {
          int j;
          for (j = i; j<n_elements;j++)
          {
            path[j] = path [j+1];
          }
          removed++;
        }
      }
      n_elements = n_elements - removed;
      gtk_icon_theme_set_search_path (priv->override_theme,(const gchar**)path,n_elements);
      g_strfreev (path);      
    }
  }

  ensure_icon (icon);
  awn_themed_icon_preload_all (icon);
}

/**
 * awn_themed_icon_get_icon_at_size:
 * @icon: A pointer to an #AwnThemedIcon object.
 * @size: A icon theme name.
 * @state: The desired icon state.
 *
 * Retrieve an icon as a #GdkPixbuf at a specific size and for a specific
 * icon state.  Note that this will not change the currently displayed icon.
 * The caller is responsible of unreffing the pixbuf.
 */

GdkPixbuf * 
awn_themed_icon_get_icon_at_size (AwnThemedIcon *icon,
                                  guint          size,
                                  const gchar   *state)
{
  AwnThemedIconPrivate *priv;
  g_return_val_if_fail (AWN_IS_THEMED_ICON (icon), NULL);
  priv = icon->priv;
  g_return_val_if_fail (priv->list,NULL);
  
  return get_pixbuf_at_size (icon, size, state);
}

const gchar *
awn_themed_icon_get_default_theme_name (AwnThemedIcon *icon)
{
  AwnThemedIconPrivate *priv;
  g_return_val_if_fail (AWN_IS_THEMED_ICON (icon), NULL);
  priv = icon->priv;
  g_return_val_if_fail (priv->gtk_theme, NULL);
  return priv->gtk_theme->priv->current_theme;
}

/**
 * awn_themed_icon_clear_icons:
 * @icon: A pointer to an #AwnThemedIcon object.
 * @size: Scope to clear. One of SCOPE_AWN_THEME, SCOPE_AWN_APPLET, SCOPE_AWN_UID.
 *
 * Delete icons from the custom awn-theme in $HOME/.icons/awn-theme
 */

void 
awn_themed_icon_clear_icons (AwnThemedIcon *icon,
                             gint           scope)
{
  AwnThemedIconPrivate *priv;
  gchar                *filename;
  const gchar          *types[] = { "png", "svg", NULL };
  gint                  i;

  g_return_if_fail (AWN_IS_THEMED_ICON (icon));
  priv = icon->priv;

  switch (scope)
  {
    case -1: /* Clean up everything */
    case SCOPE_AWN_THEME:
      for (i=0; types[i]; i++)
      {
        filename = g_strdup_printf ("%s/awn-theme/scalable/%s.%s",
                                    priv->icon_dir, 
                                    priv->current_item->name,
                                    types[i]);
        g_unlink (filename);
        g_free (filename);
      }
      
    case SCOPE_APPLET:
      for (i=0; types[i]; i++)
      {
        filename = g_strdup_printf ("%s/awn-theme/scalable/%s-%s.%s",
                                    priv->icon_dir,
                                    priv->current_item->name,
                                    priv->applet_name,
                                    types[i]);
        g_unlink (filename);
        g_free (filename);
      }

    case SCOPE_UID:
      for (i=0; types[i]; i++)
      {
        filename = g_strdup_printf ("%s/awn-theme/scalable/%s-%s-%s.%s",
                                    priv->icon_dir,
                                    priv->current_item->name,
                                    priv->applet_name,
                                    priv->uid,
                                    types[i]);
        g_unlink (filename);
        g_free (filename);
      }

    default:
      break;
  }
}

/**
 * awn_themed_icon_clear_info:
 * @icon: A pointer to an #AwnThemedIcon object.
 *
 * Clears any icon names and icon states that have been set for the Icon.
 */

void  
awn_themed_icon_clear_info (AwnThemedIcon *icon)
{
  AwnThemedIconPrivate *priv;
  GList * iter;

  g_return_if_fail (AWN_IS_THEMED_ICON (icon));
  priv = icon->priv;

  /* Free the old states & icon_names */
  priv->current_item = NULL;
  for (iter = priv->list;iter;iter=g_list_next(iter))
  {
    AwnThemedIconItem * item = iter->data;
  
    g_free (item->name);
    g_free (item->original_name);
    g_free (item->state);
    g_free (item);
    priv->list = g_list_delete_link (priv->list,iter);
  }
  gtk_drag_dest_unset (GTK_WIDGET(icon));
}

/**
 * awn_themed_icon_preload_icon:
 * @icon: A pointer to an #AwnThemedIcon object.
 * @state: The icon state.
 * @size: The size of the icon.  A value less than or equal to 0  indicates the 
 * current size should be used.
 *
 * Queues a preloads of an icon.  The icon load and cache of the icon is 
 * queued using g_idle_add().
 */

void
awn_themed_icon_preload_icon (AwnThemedIcon * icon, gchar * state, gint size)
{
  AwnThemedIconPreloadItem * item;
  AwnThemedIconPrivate *priv;

  g_return_if_fail (AWN_IS_THEMED_ICON (icon));
  g_return_if_fail (state);
  priv = icon->priv;

  item = g_malloc (sizeof (AwnThemedIconPreloadItem) );
  item->icon = icon;
  /*Conditional operator*/
  item->size = size > 0?size:priv->current_size;
  item->state = g_strdup(state);
  g_idle_add (on_idle_preload,item);
  
}

/*
 * Callbacks 
 */
void 
on_icon_theme_changed (GtkIconTheme *theme, AwnThemedIcon *icon)
{
  AwnThemedIconPrivate *priv;  
  g_return_if_fail (AWN_IS_THEMED_ICON (icon));
  priv = icon->priv;
  ensure_icon (icon);
}

static gboolean
on_idle_preload (gpointer data)
{
  AwnThemedIconPreloadItem * item = data;
  GdkPixbuf * pixbuf;
  AwnThemedIconPrivate *priv;  
  g_return_val_if_fail (item,FALSE);
  priv = item->icon->priv;

  /*CONDITIONAL operator*/
  pixbuf = get_pixbuf_at_size (item->icon, 
                               item->size>0?item->size:priv->current_size, 
                               item->state);
           
  g_object_unref (pixbuf);
  g_free (item->state);
  g_free (item);
  return FALSE;
}

/*
 FIXME among other things:

 Needs to be made aware of the concept of ::invisible:: mixed with normal states.
 
 I expect it does not deal with multiple icons currently beyond assuming the 
 drag and drop is for the currently selected stated.  Should provide a drop down
 of all icons that do not have ::invisible:: states default the choice to the 
 currently selected state.
 
 */

void 
awn_themed_icon_drag_data_received (GtkWidget        *widget, 
                                    GdkDragContext   *context,
                                    gint              x, 
                                    gint              y, 
                                    GtkSelectionData *selection_data,
                                    guint             info,
                                    guint             evt_time)
{
  AwnThemedIcon        *icon = AWN_THEMED_ICON (widget);
  AwnThemedIconPrivate *priv;
  GError               *error = NULL;
  gboolean              success = FALSE;
  gchar                *sdata = NULL;
  gchar                *decoded = NULL;
  GdkPixbuf            *pixbuf = NULL;
  gchar               **extensions = NULL;
  GtkBuilder           *builder = NULL;
  GtkWidget            *dialog = NULL;
  GtkWidget            *image;
  GtkWidget            *combo;
  gint                  res;
  gint                  scope;
  gchar                *base_name;
  gchar                *dest_filename;
  gboolean              svg = FALSE;
  const gchar          *suffix;

  if (!AWN_IS_THEMED_ICON (icon))
  {
    gtk_drag_finish (context, FALSE, FALSE, evt_time);
    return;
  }
  priv = icon->priv;

  /* First check we have valid data */
  if (selection_data == NULL ||
      gtk_selection_data_get_length (selection_data) == 0)
  {
    goto drag_out;
  }

  /* We have a valid selection, so let's process it */
  sdata = (gchar*)gtk_selection_data_get_data (selection_data);
  if (!sdata)
    goto drag_out;

  decoded = sdata = g_uri_unescape_string (sdata,NULL);
  if (!sdata)
    goto drag_out;  
  sdata = g_strchomp (sdata);  
  if (!sdata)
    goto drag_out;
  /* We only want the last dropped uri, and we want it in path form */
  sdata = g_strrstr (sdata, "file:///");
  if (!sdata)
    goto drag_out;                     
  sdata = sdata+7;

  /* Check if svg, yes there are better ways */
  if (strstr (sdata, ".svg"))
  {
    svg = TRUE;
    suffix = "svg";
  }
  else
  {
    suffix = "png";
  }
  /* Try and load the uri, to see if it's a pixbuf */
  pixbuf = gdk_pixbuf_new_from_file (sdata, NULL);

  if (!GDK_IS_PIXBUF (pixbuf))
    goto drag_out;

  /* Construct the dialog used for changing icons */
  builder = gtk_builder_new ();
  if (!gtk_builder_add_from_file (builder, AWN_CHANGE_ICON_UI, NULL))
    goto drag_out;
  
  dialog = (GtkWidget *)gtk_builder_get_object (builder, "dialog1");
  image = (GtkWidget *)gtk_builder_get_object (builder, "image1");
  combo = (GtkWidget *)gtk_builder_get_object (builder, "combobox1");
  
  gtk_image_set_from_pixbuf (GTK_IMAGE (image), pixbuf);
  gtk_widget_set_size_request (image, 64, 64);
  gtk_combo_box_set_active (GTK_COMBO_BOX (combo), 0);

  /* Run the dialog and get the user prefs */
  res = gtk_dialog_run (GTK_DIALOG (dialog));

  switch (res)
  {
    case 0: /* Apply */
      break;

    case 1: /* Cancel */
      gtk_widget_destroy (dialog);
      goto drag_out;
      break;

    case 2: /* Clear */
      awn_themed_icon_clear_icons (icon, -1);
/*      gtk_icon_theme_set_custom_theme (priv->awn_theme, NULL);
      gtk_icon_theme_set_custom_theme (priv->awn_theme, AWN_ICON_THEME_NAME);*/
      gtk_widget_destroy (dialog);
      goto drag_out;
      break;

    default:
      g_assert (0);
  }

  /* If we are here, the user wants to apply this icon in some way */
  scope = gtk_combo_box_get_active (GTK_COMBO_BOX (combo));

  if (scope == SCOPE_UID)
  {
    base_name = g_strdup_printf ("%s-%s-%s.%s",
                                 priv->current_item->name,
                                 priv->applet_name,
                                 priv->uid,
                                 suffix);
  }
  else if (scope == SCOPE_APPLET)
  {
    base_name = g_strdup_printf ("%s-%s.%s",
                                  priv->current_item->name,
                                  priv->applet_name,
                                  suffix);
  }
  else /* scope == SCOPE_AWN_THEME */
  {
    base_name = g_strdup_printf ("%s.%s", 
                                 priv->current_item->name,
                                 suffix);
  }

  dest_filename = g_build_filename (priv->icon_dir,
                                    "awn-theme", "scalable",
                                    base_name, NULL);

  /* Make sure we don't have any conflicting icons */
  awn_themed_icon_clear_icons (icon, scope);
  
  if (svg)
    check_dest_or_copy (sdata, dest_filename);
  else
    gdk_pixbuf_save (pixbuf, dest_filename, "png", 
                   &error, "compression", "0", NULL);
  if (error)
  {
    g_warning ("Unable to save %s: %s", dest_filename, error->message);
    g_error_free (error);
  }

  /* Refresh icon-theme */
/*  gtk_icon_theme_set_custom_theme (priv->awn_theme, NULL);
  gtk_icon_theme_set_custom_theme (priv->awn_theme, AWN_ICON_THEME_NAME);
*/
  success = TRUE;

  /* Clean up */
  gtk_widget_destroy (dialog);
  g_strfreev (extensions);

drag_out:

  g_free (decoded);

  if (builder)
    g_object_unref (builder);
  
  if (pixbuf)
    g_object_unref (pixbuf);
  
  gtk_drag_finish (context, success, FALSE, evt_time);

  if (success)
    ensure_icon (icon);
}


