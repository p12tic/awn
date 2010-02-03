/*
 *  Copyright (C) 2009 Michal Hruby <michal.mhr@gmail.com>
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
 *  Author : Michal Hruby <michal.mhr@gmail.com>
*/

#ifndef __AWN_ALIGNMENT_H__
#define __AWN_ALIGNMENT_H__

#include <gtk/gtk.h>

#include "awn-defines.h"
#include "awn-applet.h"

G_BEGIN_DECLS

#define AWN_TYPE_ALIGNMENT		(awn_alignment_get_type ())

#define AWN_ALIGNMENT(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj),\
        AWN_TYPE_ALIGNMENT,\
        AwnAlignment))

#define AWN_ALIGNMENT_CLASS(obj)	(G_TYPE_CHECK_CLASS_CAST ((obj), \
        AWN_TYPE_ALIGNMENT, \
        AwnAlignmentClass))
                                
#define AWN_IS_ALIGNMENT(obj)	(G_TYPE_CHECK_INSTANCE_TYPE ((obj),\
        AWN_TYPE_ALIGNMENT))

#define AWN_IS_ALIGNMENT_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE ((obj), \
        AWN_TYPE_ALIGNMENT))

#define AWN_ALIGNMENT_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), \
        AWN_TYPE_ALIGNMENT, \
        AwnAlignmentClass))

typedef struct _AwnAlignment AwnAlignment;
typedef struct _AwnAlignmentClass AwnAlignmentClass;
typedef struct _AwnAlignmentPrivate AwnAlignmentPrivate;

struct _AwnAlignment
{
  GtkAlignment parent;

  AwnAlignmentPrivate *priv;
};

struct _AwnAlignmentClass
{
  GtkAlignmentClass parent_class;

  /* Future padding */
  void (*_alignment0) (void);
  void (*_alignment1) (void);
  void (*_alignment2) (void);
  void (*_alignment3) (void);
};

GType         awn_alignment_get_type             (void);

GtkWidget *   awn_alignment_new_for_applet       (AwnApplet *applet);

gint          awn_alignment_get_offset_modifier  (AwnAlignment *alignment);

void          awn_alignment_set_offset_modifier  (AwnAlignment *alignment, 
                                                  gint modifier);

G_END_DECLS

#endif
