/* -*- Mode: C; tab-width: 2; indent-tabs-mode: t; c-basic-offset: 2 -*- */
/*
 * Copyright (C) 2009 Rodney Cryderman <rcryderman@gmail.com>
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
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by Rodney Cryderman <rcryderman@gmail.com>
 *
 */

/* awn-pixbuf-cache.c */

/*
	Using a hash table.  Which optimizes to lookups.
	The pruning is an expensive operation.  And will prune to PRUNE_PERCENT.
	Set the max_cache_size prop with this in mind.

	Cache pruning will not occur more frequently than every 10s (consider making 
	                                                             this a prop)
 */

#define PRUNE_PERCENT 0.6
#define MAX_PRUNE_FREQ 60

#include "glib.h"

#include "awn-pixbuf-cache.h"

G_DEFINE_TYPE (AwnPixbufCache, awn_pixbuf_cache, G_TYPE_OBJECT)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), AWN_TYPE_PIXBUF_CACHE, AwnPixbufCachePrivate))

typedef struct _AwnPixbufCachePrivate AwnPixbufCachePrivate;

enum
{
  PROP_0,

  PROP_MAX_CACHE_SIZE
};

struct _AwnPixbufCachePrivate 
{
  GHashTable  * pixbufs;
	GList				* accessed;
	/*maintain this ourselves... yes we could get this GHashTable or GList*/
	guint				num_pixbufs;
	guint				max_cache_size;
	GTimeVal		last_prune;
};

static void
awn_pixbuf_cache_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
	AwnPixbufCachePrivate * priv = GET_PRIVATE(object);
  switch (property_id) 
	{
		case PROP_MAX_CACHE_SIZE:
			g_value_set_uint (value,priv->max_cache_size);
			break;
		default:
  		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
awn_pixbuf_cache_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
	AwnPixbufCachePrivate * priv = GET_PRIVATE(object);
  switch (property_id) 
	{
		case PROP_MAX_CACHE_SIZE:
			priv->max_cache_size = g_value_get_uint (value);
			break;		
		default:
  		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
awn_pixbuf_cache_dispose (GObject *object)
{
	AwnPixbufCachePrivate * priv = GET_PRIVATE(object);
	if (priv->pixbufs)
	{
		g_hash_table_destroy (priv->pixbufs);
		priv->pixbufs = NULL;
	}
	if (priv->accessed)
	{
		/* The list does not own any references to the data*/
		g_list_free (priv->accessed);
		priv->accessed = NULL;
	}
  G_OBJECT_CLASS (awn_pixbuf_cache_parent_class)->dispose (object);
}

static void
awn_pixbuf_cache_finalize (GObject *object)
{
  G_OBJECT_CLASS (awn_pixbuf_cache_parent_class)->finalize (object);
}

static void
awn_pixbuf_cache_constructed (GObject *object)
{
	if ( G_OBJECT_CLASS(awn_pixbuf_cache_parent_class)->constructed)
	{
		G_OBJECT_CLASS (awn_pixbuf_cache_parent_class)->constructed (object);
	}
}

static void
awn_pixbuf_cache_class_init (AwnPixbufCacheClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GParamSpec     *pspec;
	
  object_class->get_property = awn_pixbuf_cache_get_property;
  object_class->set_property = awn_pixbuf_cache_set_property;
  object_class->dispose = awn_pixbuf_cache_dispose;
  object_class->finalize = awn_pixbuf_cache_finalize;
	object_class->constructed = awn_pixbuf_cache_constructed;

  pspec = g_param_spec_uint ("max_cache_size",
                            "max_cache_size",
                            "Maximum number of pixbufs in the cache",
                            0,
                            10000,
                            25,
                            G_PARAM_CONSTRUCT | G_PARAM_READWRITE);
  g_object_class_install_property (object_class, PROP_MAX_CACHE_SIZE, pspec);
	
  g_type_class_add_private (klass, sizeof (AwnPixbufCachePrivate));
}

static void
awn_pixbuf_cache_item_unref (GObject * object)
{
	if (object)
	{
		g_object_unref (object);
	}
}

static void
awn_pixbuf_cache_init (AwnPixbufCache *self)
{
	AwnPixbufCachePrivate * priv = GET_PRIVATE(self);
	priv->pixbufs = g_hash_table_new_full (g_str_hash,g_str_equal, 
	                                       g_free, (GDestroyNotify)awn_pixbuf_cache_item_unref);
	priv->accessed = NULL;
	priv->num_pixbufs = 0;
	g_get_current_time (&priv->last_prune);
}

/**
 * awn_pixbuf_cache_new:
 *
 * Returns: Returns a #AwnPixbufCache object. You probably should be using awn_pixbuf_cache_get_default().
 */

AwnPixbufCache*
awn_pixbuf_cache_new (void)
{
  return g_object_new (AWN_TYPE_PIXBUF_CACHE, NULL);
}

/**
 * awn_pixbuf_cache_get_default:
 *
 * Returns: Returns the default #AwnPixbufCache object.
 */

AwnPixbufCache*
awn_pixbuf_cache_get_default (void)
{
  static AwnPixbufCache * def_cache = NULL;
  if (!def_cache)
  {
    def_cache = awn_pixbuf_cache_new ();
  }
  return def_cache;    
}

/*
 Relatively lazy about pruning the cache.  It will get done though.
 */
static void
awn_pixbuf_cache_prune (AwnPixbufCache * pixbuf_cache)
{
	/*
	Maybe it's best to use something other than a hash table.
	 But lookup speed is important*/
	AwnPixbufCachePrivate * priv = GET_PRIVATE(pixbuf_cache);	
	GTimeVal current_time;

	g_get_current_time (&current_time);
	
	if (priv->num_pixbufs > priv->max_cache_size)
	{
		if ( current_time.tv_sec - priv->last_prune.tv_sec > MAX_PRUNE_FREQ)
		{
			gint target_size = priv->max_cache_size * PRUNE_PERCENT;
			GList * keys = g_hash_table_get_keys (priv->pixbufs);
			GList * iter_keys;
			for (iter_keys = keys; iter_keys; iter_keys = g_list_next (iter_keys))
			{
				GdkPixbuf * value = g_hash_table_lookup (priv->pixbufs,iter_keys->data);
				if ( g_list_position (priv->accessed,g_list_find (priv->accessed,value))>= target_size )
				{
					g_message ("%s: removing pixbuf %p",__func__,value);
					g_hash_table_remove (priv->pixbufs, iter_keys->data);
				}
			}
			/*nasty*/
			while ( (gint)g_list_length (priv->accessed)>=target_size )
			{
				priv->accessed = g_list_delete_link (priv->accessed,g_list_last (priv->accessed));
				priv->num_pixbufs--;
			}
			priv->last_prune = current_time;
			g_list_free (keys);
		}
		else
		{
			g_warning ("%s: Frequent pruning is occurring last prune was %lds ago.  This should, generally, not occur (panel resizes being an acception). Consider increasing the max-cache-size property of the applet's AwnPixbufCache object",__func__,current_time.tv_sec - priv->last_prune.tv_sec);
		}
	}
}

static void
awn_pixbuf_cache_check(AwnPixbufCache * pixbuf_cache,GdkPixbuf * pbuf)
{
	AwnPixbufCachePrivate * priv = GET_PRIVATE(pixbuf_cache);	
	GList *needle;

	needle = g_list_find (priv->accessed,pbuf);
	if (needle)
	{
		priv->accessed = g_list_delete_link (priv->accessed,needle);
	}
	else
	{
		priv->num_pixbufs++;
	}
	priv->accessed = g_list_prepend (priv->accessed,pbuf);
	if (priv->num_pixbufs > priv->max_cache_size)
	{
		awn_pixbuf_cache_prune (pixbuf_cache);
	}
	g_assert (priv->num_pixbufs == g_list_length (priv->accessed));
}

/**
 * awn_pixbuf_cache_insert_pixbuf:
 * @pixbuf_cache: A pointer to an #AwnPixbufCache object.
 * @pbuf: A #GdkPixbuf to be added to the cache.
 * @scope: An arbitrary scope if desired.  NULL indicates the default scope.
 * @theme_name: An #GtkIconTheme name.  NULL indicates this is not a pixbuf was not loaded from a Gtk Icon theme.
 * @icon_name: The name assigned to the pixbuf.  In the case of a theme icon this should be the icon name.
 *
 * Inserts the pixbuf into the icon cache. Will not replace existing pixbufs.
 */

void
awn_pixbuf_cache_insert_pixbuf (AwnPixbufCache * pixbuf_cache,
                              GdkPixbuf * pbuf,
                              const gchar *scope, 
                              const gchar * theme_name, 
                              const gchar * icon_name)
{
  gchar * key;
	AwnPixbufCachePrivate * priv = GET_PRIVATE(pixbuf_cache);

	key = g_strdup_printf ("%s::%s::%s::%dx%d",
                         scope?scope:"__NONE__",
                         theme_name?theme_name:"__NONE__",
                         icon_name,
	                       -1,
                         gdk_pixbuf_get_height (pbuf));
  g_hash_table_insert ( priv->pixbufs, key, pbuf);
  g_object_ref (pbuf);

	key = g_strdup_printf ("%s::%s::%s::%dx%d",
                         scope?scope:"__NONE__",
                         theme_name?theme_name:"__NONE__",
                         icon_name,
                         gdk_pixbuf_get_width (pbuf),
	                       -1
	                       );
  g_hash_table_insert ( priv->pixbufs, key, pbuf);
  g_object_ref (pbuf);

	key = g_strdup_printf ("%s::%s::%s::%dx%d",
                         scope?scope:"__NONE__",
                         theme_name?theme_name:"__NONE__",
                         icon_name,
                         gdk_pixbuf_get_width (pbuf),
	                       gdk_pixbuf_get_height (pbuf));
  g_hash_table_insert ( priv->pixbufs, key, pbuf);
  g_object_ref (pbuf);
	awn_pixbuf_cache_check (pixbuf_cache,pbuf);
}

/**
 * awn_pixbuf_cache_insert_pixbuf_simple_key:
 * @pixbuf_cache: A pointer to an #AwnPixbufCache object.
 * @pbuf: A #GdkPixbuf to be added to the cache.
 * @simple_key: A single string as a key
 *
 * Inserts the pixbuf into the icon cache using simple_key as the key.  Use
 * in conjunction with awn_pixbuf_cache_lookup_simple_key.  It's normally best 
 * to use awn_pixbuf_cache_insert_pixbuf() instead.
 */

void
awn_pixbuf_cache_insert_pixbuf_simple_key (AwnPixbufCache * pixbuf_cache,
                              GdkPixbuf * pbuf,
                              const gchar * simple_key)
{
	AwnPixbufCachePrivate * priv = GET_PRIVATE(pixbuf_cache);

  g_hash_table_insert ( priv->pixbufs, g_strdup(simple_key), pbuf);
  g_object_ref (pbuf);
}

/**
 * awn_pixbuf_cache_insert_null_result:
 * @pixbuf_cache: A pointer to an #AwnPixbufCache object.
 * @scope: An arbitrary scope if desired.  NULL indicates the default scope.
 * @theme_name: An #GtkIconTheme name.  NULL indicates this is not a pixbuf was not loaded from a Gtk Icon theme.
 * @icon_name: The name assigned to the pixbuf.  In the case of a theme icon this should be the icon name.
 * @width: Width of the null result.
 * @height: Height of the null result.
 *
 * Inserts a null result into the pixbuf cache.  This can be used to avoid
 * further attempts to load an icon when the attempt has already failed.
 */

void
awn_pixbuf_cache_insert_null_result (AwnPixbufCache * pixbuf_cache,
                              const gchar *scope, 
                              const gchar * theme_name, 
                              const gchar * icon_name,
                              gint width,
                              gint height)
{
  gchar * key;
	AwnPixbufCachePrivate * priv = GET_PRIVATE(pixbuf_cache);
	key = g_strdup_printf ("%s::%s::%s::%dx%d",
                         scope?scope:"__NONE__",
                         theme_name?theme_name:"__NONE__",
                         icon_name,
                         width,
	                       height);

  g_hash_table_insert ( priv->pixbufs, key, NULL);

}

/**
 * awn_pixbuf_cache_lookup_simple_key:
 * @pixbuf_cache: A pointer to an #AwnPixbufCache object.
 * @simple_key: A single string to use as the lookup key.
 * @width: Width of the desired pixbuf.  A value of -1 should be used if the width value is to be ignored.
 * @height: Height of the desired pixbuf.  A value of -1 should be used if the height value is to be ignored.
 *
 * Attempts to lookup an matching pixbuf in the pixbuf cache using simple_key.  
 * It is best to use awn_pixbuf_cache_lookup() in most cases.
 * Returns: a pointer to a matching pixbuf in the cache or NULL.
 */

GdkPixbuf *
awn_pixbuf_cache_lookup_simple_key (AwnPixbufCache * pixbuf_cache,
                              const gchar * simple_key,
                       				gint width,
                       				gint height)
{
	gpointer pixbuf = NULL;
	AwnPixbufCachePrivate * priv = GET_PRIVATE(pixbuf_cache);

	pixbuf = g_hash_table_lookup (priv->pixbufs,simple_key);
	if (pixbuf)
	{
		g_object_ref (pixbuf);
	}
	return pixbuf;
}


/**
 * awn_pixbuf_cache_lookup:
 * @pixbuf_cache: A pointer to an #AwnPixbufCache object.
 * @scope: An arbitrary scope if desired.  NULL indicates the default scope.
 * @theme_name: An #GtkIconTheme name.  NULL indicates this is not a pixbuf was not loaded from a Gtk Icon theme.
 * @icon_name: The name assigned to the pixbuf.  In the case of a theme icon this should be the icon name.
 * @width: Width of the desired pixbuf.  A value of -1 should be used if the width value is to be ignored.
 * @height: Height of the desired pixbuf.  A value of -1 should be used if the height value is to be ignored.
 * @null_result: Pointer to a gboolean or NULL. If non-NULL will be set to indicate if there is a null result value in the cache for the key combination
 *
 * Attempts to lookup an matching pixbuf in the pixbuf cache.  
 * Returns: a pointer to a matching pixbuf in the cache or NULL.  Check null_result to determine
 * if a this key combination was previously determined to fail to load.
 */

GdkPixbuf *
awn_pixbuf_cache_lookup (AwnPixbufCache * pixbuf_cache,
                              const gchar *scope, 
                              const gchar * theme_name, 
                              const gchar * icon_name,
                       				gint width,
                       				gint height,
                       				gboolean * null_result)
{
	gpointer pixbuf = NULL;
  gchar *key = g_strdup_printf ("%s::%s::%s::%dx%d",
                         scope?scope:"__NONE__",
                         theme_name?theme_name:"__NONE__",
                         icon_name,
                         width,
                         height);
	AwnPixbufCachePrivate * priv = GET_PRIVATE(pixbuf_cache);
	gboolean success;

	success = g_hash_table_lookup_extended (priv->pixbufs,key,NULL,&pixbuf);
	if (pixbuf)
	{
		g_object_ref (pixbuf);
	}
	if (null_result && !pixbuf)
	{
		*null_result = success;
	}
	else if (null_result)
	{
		*null_result = FALSE;
	}
		
//  g_debug ("Cache lookup: %s for %s",pixbuf?"Hit":"Miss",key);
  g_free (key);  
	return pixbuf;
}

/**
 * awn_pixbuf_cache_invalidate:
 * @pixbuf_cache: A pointer to an #AwnPixbufCache object.
 *
 * Invalidates the cache.  Should be used, for example, if there was a change 
 * signal for any of the underlying themes being cached.
 */

void
awn_pixbuf_cache_invalidate (AwnPixbufCache* pixbuf_cache)
{
	AwnPixbufCachePrivate * priv = GET_PRIVATE(pixbuf_cache);

	g_hash_table_remove_all (priv->pixbufs);
	g_list_free (priv->accessed);
	priv->accessed = NULL;
	priv->num_pixbufs = 0;
	g_get_current_time (&priv->last_prune);
}

