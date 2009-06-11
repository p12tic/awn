/*
 * Copyright (C) 2008 Rodney Cryderman <rcryderman@gmail.com>
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

G_DEFINE_TYPE (AwnThemedIcon, awn_themed_icon, AWN_TYPE_ICON)

#define AWN_THEMED_ICON_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj),\
  AWN_TYPE_THEMED_ICON, \
  AwnThemedIconPrivate))

#define LOAD_FLAGS GTK_ICON_LOOKUP_FORCE_SVG
#define AWN_ICON_THEME_NAME "awn-theme"
#define AWN_CHANGE_ICON_UI PKGDATADIR"/awn-themed-icon-ui.xml"

struct _AwnThemedIconPrivate
{
  GtkIconTheme *awn_theme;
  GtkIconTheme *override_theme;
  GtkIconTheme *gtk_theme;
  gchar        *icon_dir;
  
  gchar  *applet_name;
  gchar  *uid;
  gchar **states;
  gchar **icon_names;
  gchar **icon_names_orignal;
  gint    n_states;

  gchar  *current_state;
  gint    current_size;
  gint    cur_icon;
};

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
void on_icon_theme_changed              (GtkIconTheme     *theme, 
                                         AwnThemedIcon     *icon);
void awn_themed_icon_drag_data_received (GtkWidget        *widget, 
                                         GdkDragContext   *context,
                                         gint              x, 
                                         gint              y, 
                                         GtkSelectionData *selection,
                                         guint             info,
                                         guint             evt_time);

/* GObject stuff */
static void
awn_themed_icon_dispose (GObject *object)
{
  AwnThemedIconPrivate *priv;

  g_return_if_fail (AWN_IS_THEMED_ICON (object));
  priv = AWN_THEMED_ICON (object)->priv;

  g_strfreev (priv->states);             priv->states = NULL;
  g_strfreev (priv->icon_names);         priv->icon_names = NULL;
  g_strfreev (priv->icon_names_orignal); priv->icon_names_orignal = NULL;
  g_free (priv->applet_name);            priv->applet_name = NULL;
  g_free (priv->uid);                    priv->uid = NULL;
  g_free (priv->current_state);          priv->current_state = NULL;
  g_free (priv->icon_dir);               priv->icon_dir = NULL;

  if (G_IS_OBJECT (priv->awn_theme))
    g_object_unref (priv->awn_theme);
  if (G_IS_OBJECT (priv->override_theme))
    g_object_unref (priv->override_theme);

  g_signal_handlers_disconnect_by_func(priv->gtk_theme, on_icon_theme_changed, object );
  G_OBJECT_CLASS (awn_themed_icon_parent_class)->dispose (object);
}

static void
awn_themed_icon_class_init (AwnThemedIconClass *klass)
{
  GObjectClass   *obj_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *wid_class = GTK_WIDGET_CLASS (klass);
  
  obj_class->dispose = awn_themed_icon_dispose;
  
  wid_class->drag_data_received = awn_themed_icon_drag_data_received;

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
  priv->states = NULL;
  priv->icon_names = NULL;
  priv->icon_names_orignal = NULL;
  priv->current_state = NULL;
  priv->current_size = 48;

  /* Set-up the gtk-theme */
  priv->gtk_theme = gtk_icon_theme_get_default ();
  g_signal_connect (priv->gtk_theme, "changed", 
                    G_CALLBACK (on_icon_theme_changed), icon);

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
  g_free (index_src);
  g_free (index_dest);

  /* Now let's make our custom theme */
  priv->awn_theme = gtk_icon_theme_new ();
  gtk_icon_theme_set_custom_theme (priv->awn_theme, AWN_ICON_THEME_NAME);
  g_signal_connect (priv->awn_theme, "changed", 
                    G_CALLBACK (on_icon_theme_changed), icon);
  
  g_free (scalable_dir);
  g_free (theme_dir);

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

static GdkPixbuf *
get_pixbuf_at_size (AwnThemedIcon *icon, gint size, const gchar *state)
{
  AwnThemedIconPrivate *priv;
  GdkPixbuf            *pixbuf = NULL;
  gint                  idx;

  priv = icon->priv;

  /* Find the index of the current state in states */
  g_return_val_if_fail(priv->states,NULL);	
  for (idx = 0; priv->states[idx]; idx++)
  {
    if (strcmp (priv->states[idx], state ? state : priv->current_state) == 0)
    {
      const gchar *applet_name;
      const gchar *icon_name;
      const gchar *uid;
      gint         i;
      
      priv->cur_icon = idx;
      applet_name = priv->applet_name;
      icon_name = priv->icon_names[idx];
      uid = priv->uid;
      
      /* Go through all the possible outcomes until we get a pixbuf */
      for (i = 0; i < N_SCOPES; i++)
      {
        gchar *name = NULL;
        switch (i)
        {
          case SCOPE_UID:
            name = g_strdup_printf ("%s-%s-%s", icon_name, applet_name, uid);
            pixbuf = gtk_icon_theme_load_icon (priv->awn_theme, name,
                                               size, LOAD_FLAGS, NULL);
            break;

          case SCOPE_APPLET:
            name = g_strdup_printf ("%s-%s", icon_name, applet_name);
            pixbuf = gtk_icon_theme_load_icon (priv->awn_theme, name,
                                               size, LOAD_FLAGS, NULL);
            break;

          case SCOPE_AWN_THEME:
            pixbuf = gtk_icon_theme_load_icon (priv->awn_theme, icon_name, 
                                               size, LOAD_FLAGS, NULL);
            break;

          case SCOPE_OVERRIDE_THEME:
            pixbuf = NULL;
            if (priv->override_theme)
              pixbuf = gtk_icon_theme_load_icon (priv->override_theme,
                                                 icon_name, 
                                                 size, LOAD_FLAGS, NULL);
            break;

          case SCOPE_GTK_THEME:
            pixbuf = gtk_icon_theme_load_icon (priv->gtk_theme, icon_name,
                                               size, LOAD_FLAGS, NULL);
            break;

          case SCOPE_FILENAME:
            pixbuf = NULL;
            if (priv->icon_names_orignal)
            {
              gchar *real_name = priv->icon_names_orignal[idx];
              pixbuf = try_and_load_image_from_disk (real_name, size);
            }
            break;

          case SCOPE_FALLBACK_STOP:
            pixbuf = gtk_icon_theme_load_icon (priv->gtk_theme,
                                               GTK_STOCK_MISSING_IMAGE,
                                               size, LOAD_FLAGS, NULL);
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
    g_warning ("State does not exist: %s", priv->current_state);
  }

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

  if (!priv->n_states || !priv->states || !priv->icon_names 
      || !priv->current_state || !priv->current_size)
  {
    /* We're not ready yet */
    return;
  }

  /* Get the icon first */
  pixbuf = get_pixbuf_at_size (icon, priv->current_size, priv->current_state);

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

  g_return_if_fail (AWN_IS_THEMED_ICON (icon));
  priv = icon->priv;

  if (priv->current_state)
    g_free (priv->current_state);

  priv->current_state = g_strdup (state);
  ensure_icon (icon);
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

  return icon->priv->current_state;
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
  g_return_if_fail (AWN_IS_THEMED_ICON (icon));

  icon->priv->current_size = size;
  ensure_icon (icon);
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
static gchar **
normalise_names (gchar **names)
{
  gchar **ret = NULL;
  gint i;

  for (i = 0; names[i]; i++)
  {
    gint j;
    for (j = 0; names[i][j]; j++)
    {
      if (names[i][j] == '/')
      {
        if (!ret)
          ret = g_strdupv (names);
        names[i][j] = '-';
      }
    }
  }
  return ret;
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
  guint n_states;
  gchar ** iter;
  
  g_return_if_fail (AWN_IS_THEMED_ICON (icon));
  g_return_if_fail (applet_name);
  g_return_if_fail (uid);
  g_return_if_fail (states);
  g_return_if_fail (icon_names);
  priv = icon->priv;

  /* Check number of strings>0 and n_strings (states)==n_strings (icon_names)*/
  n_states = g_strv_length (states);
  if (n_states < 1 || n_states != g_strv_length (icon_names))
  {
    g_warning ("%s", n_states ? 
                       "Length of states must match length of icon_names" 
                       : "Length of states must be greater than 0");
    return;
  }
  
  /* Free the old states & icon_names */
  g_strfreev (priv->states);
  g_strfreev (priv->icon_names);
  g_strfreev (priv->icon_names_orignal);
  priv->states = NULL;
  priv->icon_names = NULL;
  priv->icon_names_orignal = NULL;

  /* Copy states & icon_names internally */
  priv->states = g_strdupv (states);
  priv->icon_names = g_strdupv (icon_names);
  priv->icon_names_orignal = normalise_names (priv->icon_names);
  priv->n_states = n_states;
  
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

  /*FIXME: Should we ensure_icon here? The current_state variable is probably
   * invalid at this moment...
   */
  /*
   * Initate drag_drop 
   */
  for (iter = priv->states; *iter; iter++)
  {
    if (g_strstr_len (*iter,-1, "::invisible::") !=  *iter)
    {
      gtk_drag_dest_set (GTK_WIDGET (icon),
                     GTK_DEST_DEFAULT_MOTION | GTK_DEST_DEFAULT_DROP,
                     drop_types, n_drop_types,
                     GDK_ACTION_COPY | GDK_ACTION_ASK);
      break;
    }
  }
}

/**
 * awn_themed_icon_set_info_info:
 * @icon: A pointer to an #AwnThemedIcon object.
 * @applet_name: The applet name.
 * @uid: The applet's UID.
 * @icon_n
 * instance.ame: A themed icon name.
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

  icon_names[0] = g_strdup (icon_name);
  
  awn_themed_icon_set_info (icon, applet_name, uid, states, icon_names);

  g_free (icon_names[0]);
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
  GStrv icon_names;
  GStrv states;
  AwnThemedIconPrivate *priv;  
  gchar * applet_name;
  gchar * uid;
  
  g_return_if_fail (AWN_IS_THEMED_ICON (icon));

  priv = icon->priv;

  applet_name = g_strdup (priv->applet_name?priv->applet_name:"__unknown__");
  uid = g_strdup (priv->uid?priv->uid:"__invisible__");
  
  icon_names = g_strdupv (priv->icon_names_orignal);
  states = g_strdupv (priv->states);
  
  icon_names = g_realloc (icon_names, sizeof (gchar *) * (priv->n_states+2));
  states = g_realloc (priv->states,sizeof (gchar *) * (priv->n_states+2) );
  
  icon_names[priv->n_states+1] = NULL;
  icon_names[priv->n_states] = g_strdup (icon_name);
  
  states [priv->n_states+1] = NULL;
  states [priv->n_states] = g_strdup (state?state:"::invisible::unknown");
 
  
  awn_themed_icon_set_info (icon,applet_name,uid,states,icon_names);
  g_strfreev (icon_names);
  g_strfreev (states);
  g_free (uid);
  g_free (applet_name);
  
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

  g_return_if_fail (AWN_IS_THEMED_ICON (icon));
  priv = icon->priv;

  /* Remove old theme, if it exists */
  if (priv->override_theme)
    g_object_unref (priv->override_theme);

  if (theme_name)
  {
    priv->override_theme = gtk_icon_theme_new ();
    gtk_icon_theme_set_custom_theme (priv->override_theme, theme_name);
  }
  else
  {
    priv->override_theme = NULL;
  }

  ensure_icon (icon);
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
  g_return_val_if_fail (priv->states,NULL);
  
  return get_pixbuf_at_size (icon, size, state);
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
                                    priv->icon_names[priv->cur_icon],
                                    types[i]);
        g_unlink (filename);
        g_free (filename);
      }
      
    case SCOPE_APPLET:
      for (i=0; types[i]; i++)
      {
        filename = g_strdup_printf ("%s/awn-theme/scalable/%s-%s.%s",
                                    priv->icon_dir,
                                    priv->icon_names[priv->cur_icon],
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
                                    priv->icon_names[priv->cur_icon],
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

  g_return_if_fail (AWN_IS_THEMED_ICON (icon));
  priv = icon->priv;

  /* Free the old states & icon_names */
  g_strfreev (priv->states);
  g_strfreev (priv->icon_names);
  g_strfreev (priv->icon_names_orignal);
  priv->states = NULL;
  priv->icon_names = NULL;
  priv->icon_names_orignal = NULL;
  gtk_drag_dest_unset (GTK_WIDGET(icon));
}

/*
 * Callbacks 
 */
void 
on_icon_theme_changed (GtkIconTheme *theme, AwnThemedIcon *icon)
{
  g_return_if_fail (AWN_IS_THEMED_ICON (icon));
  ensure_icon (icon);
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
      gtk_icon_theme_set_custom_theme (priv->awn_theme, NULL);
      gtk_icon_theme_set_custom_theme (priv->awn_theme, AWN_ICON_THEME_NAME);
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
                                 priv->icon_names[priv->cur_icon],
                                 priv->applet_name,
                                 priv->uid,
                                 suffix);
  }
  else if (scope == SCOPE_APPLET)
  {
    base_name = g_strdup_printf ("%s-%s.%s",
                                  priv->icon_names[priv->cur_icon],
                                  priv->applet_name,
                                  suffix);
  }
  else /* scope == SCOPE_AWN_THEME */
  {
    base_name = g_strdup_printf ("%s.%s", 
                                 priv->icon_names[priv->cur_icon],
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
  gtk_icon_theme_set_custom_theme (priv->awn_theme, NULL);
  gtk_icon_theme_set_custom_theme (priv->awn_theme, AWN_ICON_THEME_NAME);

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

