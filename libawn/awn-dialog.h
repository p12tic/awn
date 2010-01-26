/*
 * Copyright (c) 2009 Michal Hruby <michal.mhr@gmail.com>
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
 */

#ifndef __AWN_DIALOG_H__
#define __AWN_DIALOG_H__

#include <gtk/gtk.h>
#include <stdlib.h>

#include "awn-applet.h"
#include "awn-defines.h"

G_BEGIN_DECLS

#define AWN_TYPE_DIALOG (awn_dialog_get_type ())

#define AWN_DIALOG(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
                         AWN_TYPE_DIALOG, AwnDialog))

#define AWN_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), \
                                 AWN_TYPE_DIALOG, AwnDialogClass))

#define AWN_IS_DIALOG(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
                            AWN_TYPE_DIALOG))

#define AWN_IS_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), \
                                    AWN_TYPE_DIALOG))

#define AWN_DIALOG_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), \
                                   AWN_TYPE_DIALOG, AwnDialogClass))


/* AwnDialog CLASS & TYPE */

typedef struct _AwnDialog AwnDialog;

typedef struct _AwnDialogClass AwnDialogClass;

typedef struct _AwnDialogPrivate AwnDialogPrivate;

struct _AwnDialog
{
  GtkWindow   window;

  AwnDialogPrivate  *priv;
};

struct _AwnDialogClass
{
  GtkWindowClass parent_class;
};

GType      awn_dialog_get_type (void);

GtkWidget* awn_dialog_new (void);

GtkWidget* awn_dialog_new_for_widget (GtkWidget *widget);

GtkWidget* awn_dialog_new_for_widget_with_applet (GtkWidget *widget,
                                                  AwnApplet *applet);

void       awn_dialog_set_padding (AwnDialog *dialog, gint padding);

GtkWidget* awn_dialog_get_content_area (AwnDialog *dialog);

G_END_DECLS

#endif

