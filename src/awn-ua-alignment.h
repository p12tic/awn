/*
 *  Copyright (C) 2009 Rodney Cryderman <rcryderman@gmail.com>
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
 *
 */

/* awn-ua-alignment.h */

#ifndef _AWN_UA_ALIGNMENT
#define _AWN_UA_ALIGNMENT

#include <gtk/gtk.h>
#include "awn-applet-manager.h"

G_BEGIN_DECLS

#define AWN_TYPE_UA_ALIGNMENT awn_ua_alignment_get_type()

#define AWN_UA_ALIGNMENT(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), AWN_TYPE_UA_ALIGNMENT, AwnUAAlignment))

#define AWN_UA_ALIGNMENT_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), AWN_TYPE_UA_ALIGNMENT, AwnUAAlignmentClass))

#define AWN_IS_UA_ALIGNMENT(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), AWN_TYPE_UA_ALIGNMENT))

#define AWN_IS_UA_ALIGNMENT_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), AWN_TYPE_UA_ALIGNMENT))

#define AWN_UA_ALIGNMENT_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), AWN_TYPE_UA_ALIGNMENT, AwnUAAlignmentClass))

typedef struct {
  GtkAlignment parent;
} AwnUAAlignment;

typedef struct {
  GtkAlignmentClass parent_class;
} AwnUAAlignmentClass;

GType awn_ua_alignment_get_type (void);

GtkWidget* awn_ua_alignment_new (AwnAppletManager *manager,gchar * ua_list_entry,double ua_ratio);

GtkWidget* awn_ua_alignment_get_socket (AwnUAAlignment *self);

GdkWindow * awn_ua_alignment_add_id (AwnUAAlignment *self, GdkNativeWindow native_window);

gint awn_ua_alignment_list_cmp (gconstpointer a, gconstpointer b);

G_END_DECLS

#endif /* _AWN_UA_ALIGNMENT */
