/*
 * Copyright (c) 2007 Anthony Arobone <aarobone@gmail.com>
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
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/*
 *
 */

#ifndef __AWN_APPLET_DIALOG_H__
#define __AWN_APPLET_DIALOG_H__

#include <gtk/gtk.h>
#include <stdlib.h>

#include "awn-applet.h"
#include "awn-defines.h"

G_BEGIN_DECLS

#define AWN_TYPE_APPLET_DIALOG (awn_applet_dialog_get_type ())

#define AWN_APPLET_DIALOG(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
                                AWN_TYPE_APPLET_DIALOG, AwnAppletDialog))

#define AWN_APPLET_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), \
                                        AWN_TYPE_APPLET_DIALOG, AwnAppletDialogClass))

#define AWN_IS_APPLET_DIALOG(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
                                   AWN_TYPE_APPLET_DIALOG))

#define AWN_IS_APPLET_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), \
    AWN_TYPE_APPLET_DIALOG))

#define AWN_APPLET_DIALOG_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), \
    AWN_TYPE_APPLET_DIALOG, AwnAppletDialogClass))


/* AwnAppletDialog CLASS & TYPE */

typedef struct _AwnAppletDialog AwnAppletDialog;

typedef struct _AwnAppletDialogClass AwnAppletDialogClass;

typedef struct _AwnAppletDialogPrivate AwnAppletDialogPrivate;

struct _AwnAppletDialog
{
  GtkWindow   window;
  AwnAppletDialogPrivate  *priv;
};

struct _AwnAppletDialogClass
{
  GtkWindowClass parent_class;
};

GType awn_applet_dialog_get_type(void);

GtkWidget* awn_applet_dialog_new(AwnApplet *applet);

void awn_applet_dialog_position_reset(AwnAppletDialog *dialog);

G_END_DECLS

#endif

