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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "awn-applet.h"
#include "awn-applet-gconf.h"
#include "awn-applet-dialog.h"
#include "awn-cairo-utils.h"
#include "awn-defines.h"

G_DEFINE_TYPE(AwnAppletDialog, awn_applet_dialog, GTK_TYPE_WINDOW)

#define AWN_APPLET_DIALOG_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj),\
	AWN_TYPE_APPLET_DIALOG, \
	AwnAppletDialogPrivate))

struct _AwnAppletDialogPrivate
{
        AwnApplet *applet;
        GtkWidget *align;
};

/* PRIVATE CLASS METHODS */
static void awn_applet_dialog_class_init(AwnAppletDialogClass *klass);
static void awn_applet_dialog_init(AwnAppletDialog *dialog);
/*static void awn_applet_dialog_finalize(GObject *obj); */

static void 
_on_alpha_screen_changed (GtkWidget* pWidget, 
                          GdkScreen* pOldScreen, 
                          GtkWidget* pLabel)
{                       
	GdkScreen* pScreen = gtk_widget_get_screen (pWidget);
	GdkColormap* pColormap = gdk_screen_get_rgba_colormap (pScreen);
      
	if (!pColormap)
		pColormap = gdk_screen_get_rgb_colormap (pScreen);

	gtk_widget_set_colormap (pWidget, pColormap);
}


/*
 * Should "reposition" dialog-arrow if the dialog does not fit
 *(fall-off-screen) on the desired place.
 */
void 
awn_applet_dialog_position_reset (AwnAppletDialog *dialog) 
{
	if (dialog->priv->applet == NULL) 
                return;
	
	gint width, height;
	GtkWindow *window = GTK_WINDOW (dialog);

	gint x, y;
	gdk_window_get_origin(GTK_WIDGET(dialog->priv->applet)->window, 
                              &x, &y);
	
	/* offest by applet height */
	gtk_widget_get_size_request (GTK_WIDGET(dialog->priv->applet), 
                                     &width, &height);
	y += height;
	x += (width / 2) - 36; /* gap */

	/* offset dialog height */
	gtk_window_get_size (window, &width, &height);
	y -= height;
	
	gtk_window_move (GTK_WINDOW(window), x, y-1);
}

static void 
_on_realize(GtkWidget *widget, gpointer *data) 
{
	awn_applet_dialog_position_reset (AWN_APPLET_DIALOG (widget));
}

static gboolean 
_expose_event(GtkWidget *widget, GdkEventExpose *expose) 
{
	cairo_t *cr = NULL;
	GtkWidget *child = NULL;
	gint width, height;
        const gchar *text;
	
	cr = gdk_cairo_create (widget->window);
	if (!cr)
		return FALSE;
		
	gtk_window_get_size (GTK_WINDOW (widget), &width, &height);
	
	// Clear the background to transparent
	cairo_set_source_rgba (cr, 1.0f, 1.0f, 1.0f, 0.0f);
	cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
	cairo_paint (cr);

	// draw everything else over transparent background	
	cairo_set_operator (cr, CAIRO_OPERATOR_OVER);

	gint gap = 30;

	// background shading
	cairo_set_source_rgba (cr, 0, 0, 0, 0.85f);
	awn_cairo_rounded_rect (cr, 0, 0, width, height - gap, 15, ROUND_ALL);
        cairo_fill(cr);

	// draw arrow
	cairo_move_to (cr, 20, height - gap);
	cairo_line_to (cr, 36, height);
	cairo_line_to (cr, 52, height - gap);
	cairo_line_to (cr, 20, height - gap);
	cairo_fill (cr);

	// rasterize title text
	text = gtk_window_get_title (GTK_WINDOW (widget));
	if (text != NULL) 
        {
		cairo_text_extents_t extents;
		cairo_select_font_face (cr, 
                            "Sans", 
                            CAIRO_FONT_SLANT_NORMAL, 
                            CAIRO_FONT_WEIGHT_BOLD);
		cairo_set_font_size (cr, 14);
		cairo_text_extents (cr, text, &extents);
		// render title text
		cairo_set_source_rgba (cr, 1, 1, 1, 1);
		cairo_move_to (cr, 8, 8 + extents.height);
		cairo_show_text (cr, text);
	}
	
	// Clean up
	cairo_destroy (cr);

	/* Propagate the signal */
	child = gtk_bin_get_child (GTK_BIN (widget));
	if (child)
		gtk_container_propagate_expose (GTK_CONTAINER (widget),
						child, expose);
		
	return FALSE;
}


static gboolean 
_on_key_press_event(GtkWidget *widget, GdkEventKey *event, gpointer *data)
{
	if (event->keyval == GDK_Escape && AWN_IS_APPLET_DIALOG(widget))
        {
		AWN_APPLET_DIALOG(widget)->priv->applet = NULL;
		gtk_widget_destroy(widget);
	}
	return FALSE;
}

static void
awn_applet_dialog_add (GtkContainer *dialog, GtkWidget *widget)
{
        AwnAppletDialogPrivate *priv;

        g_return_if_fail (AWN_IS_APPLET_DIALOG (dialog));
        g_return_if_fail (GTK_IS_WIDGET (widget));
        priv = AWN_APPLET_DIALOG (dialog)->priv;

        gtk_container_add (GTK_CONTAINER (priv->align), widget);
}

/*
 * class init
 */
static void 
awn_applet_dialog_class_init (AwnAppletDialogClass *klass) 
{
        GtkWidgetClass *widget_class;
        GtkContainerClass *cont_class;

        widget_class = GTK_WIDGET_CLASS (klass);
	widget_class->expose_event = _expose_event;
	
        cont_class = GTK_CONTAINER_CLASS (klass);
        cont_class->add = awn_applet_dialog_add;

	g_type_class_add_private (G_OBJECT_CLASS (klass), 
                                  sizeof (AwnAppletDialogPrivate));
}


/*
 *  init
 */
static void 
awn_applet_dialog_init (AwnAppletDialog *dialog) 
{
        AwnAppletDialogPrivate *priv;

        priv = dialog->priv = AWN_APPLET_DIALOG_GET_PRIVATE (dialog);
        
        gtk_rc_parse_string ("style \"textcolor\"\n"
                          "{\n"
                          "fg[NORMAL] = \"#FFFFFF\"\n"
                          "text[NORMAL] = \"#FFFFFF\"\n"
                          "}\n"
                          "widget \"AwnDialog.*\" style \"textcolor\"");
        gtk_widget_set_name (GTK_WIDGET (dialog), "AwnDialog");
        
        /* setup struct init values */
	gtk_window_set_skip_taskbar_hint (GTK_WINDOW (dialog), TRUE);
	gtk_window_set_decorated(GTK_WINDOW(dialog), FALSE);
	gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);
	gtk_window_stick(GTK_WINDOW(dialog));
	
	/* TODO: makes non-movable, but breaks other things like key press 
         * event. 
         * gtk_window_set_type_hint(GTK_WINDOW(dialog), 
         *                          GDK_WINDOW_TYPE_HINT_DOCK);
         */
        _on_alpha_screen_changed(GTK_WIDGET(dialog), NULL, NULL);
        gtk_widget_set_app_paintable(GTK_WIDGET(dialog), TRUE);
	
	/* events */
	gtk_widget_add_events(GTK_WIDGET(dialog), GDK_ALL_EVENTS_MASK);
        g_signal_connect(G_OBJECT(dialog), "key-press-event", 
                         G_CALLBACK(_on_key_press_event), NULL);
	g_signal_connect(G_OBJECT(dialog), "realize", 
                         G_CALLBACK(_on_realize), NULL);

        priv->align = gtk_alignment_new (0.5, 0.5, 1, 1);
        gtk_alignment_set_padding (GTK_ALIGNMENT (priv->align),
                                   10, 40, 10, 10);
        
        GTK_CONTAINER_CLASS (awn_applet_dialog_parent_class)->add 
                                     (GTK_CONTAINER (dialog), priv->align);
}

/*
 * new - creates a new object
 */
GtkWidget* 
awn_applet_dialog_new (AwnApplet *applet) 
{
	AwnAppletDialog *dialog;
  
        g_return_val_if_fail (AWN_IS_APPLET (applet), NULL);

        dialog = g_object_new (AWN_TYPE_APPLET_DIALOG,
                               "type", GTK_WINDOW_TOPLEVEL, 
                               NULL);
	dialog->priv->applet = applet;
	
        return GTK_WIDGET (dialog);
}
