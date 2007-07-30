/*
 *  Copyright (C) 2007 Neil Jagdish Patel <njpatel@gmail.com>
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
 *  Author : Neil Jagdish Patel <njpatel@gmail.com>
*/

#include <gtk/gtk.h>
#include <string.h>
#include <math.h>
#include <libgnome/gnome-desktop-item.h>
#include <libgnomevfs/gnome-vfs.h>
#include <libgnome/libgnome.h>

#include "awn-task.h"
#include "awn-x.h"

#include "awn-utils.h"
#include "awn-marshallers.h"

#define AWN_TASK_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), AWN_TYPE_TASK, AwnTaskPrivate))

G_DEFINE_TYPE (AwnTask, awn_task, GTK_TYPE_DRAWING_AREA);

#define  AWN_FRAME_RATE	15

#define  AWN_TASK_EFFECT_DIR_DOWN		0
#define  AWN_TASK_EFFECT_DIR_UP			1
#define  AWN_TASK_EFFECT_DIR_LEFT		2
#define  AWN_TASK_EFFECT_DIR_RIGHT		3
#define  M_PI 					3.14159265358979323846
#define  AWN_CLICK_IDLE_TIME			450

#define M_PI		3.14159265358979323846

/* FORWARD DECLERATIONS */

static gboolean awn_task_expose (GtkWidget *task, GdkEventExpose *event);
static gboolean awn_task_button_press (GtkWidget *task, GdkEventButton *event);
static gboolean awn_task_proximity_in (GtkWidget *task, GdkEventCrossing *event);
static gboolean awn_task_proximity_out (GtkWidget *task, GdkEventCrossing *event);
static gboolean awn_task_win_enter_in (GtkWidget *window, GdkEventMotion *event, AwnTask *task);
static gboolean awn_task_win_enter_out (GtkWidget *window, GdkEventCrossing *event, AwnTask *task);

static gboolean awn_task_drag_motion (GtkWidget *task,
		GdkDragContext *context, gint x, gint y, guint t);
static void
awn_task_drag_leave (GtkWidget *widget, GdkDragContext *drag_context,
                                        guint time);
static void awn_task_create_menu(AwnTask *task, GtkMenu *menu);


/* STRUCTS & ENUMS */

typedef enum {
	AWN_TASK_EFFECT_NONE,
	AWN_TASK_EFFECT_OPENING,
	AWN_TASK_EFFECT_HOVER,
	AWN_TASK_EFFECT_ATTENTION,
	AWN_TASK_EFFECT_CLOSING,
	AWN_TASK_EFFECT_CHANGE_NAME

} AwnTaskEffect;

typedef enum {
	AWN_TASK_MENU_TYPE_NORMAL,
	AWN_TASK_MENU_TYPE_CHECK
} AwnTaskMenuType;

typedef struct {
	AwnTaskMenuType		type;
	GtkWidget		*item;
	gint 			id;
	gboolean		active;


} AwnTaskMenuItem;

typedef struct _AwnTaskPrivate AwnTaskPrivate;

struct _AwnTaskPrivate
{
	AwnTaskManager 	 *task_manager;
	AwnSettings 	 *settings;

	GnomeDesktopItem *item;
	gint pid;

	gchar *application;

	gboolean is_launcher;
	AwnSmartLauncher launcher;
	gboolean is_starter;

	WnckWindow *window;
	AwnTitle *title;

	gboolean is_active;
	gboolean needs_attention;
	gboolean is_closing;
	gboolean hover;

	GdkPixbuf *icon;
  GdkPixbuf *reflect;
  gint icon_width;
	gint icon_height;

	gint progress;

	gboolean info;
	gchar *info_text;

	/* EFFECT VARIABLES */
	gboolean effect_lock;
	AwnTaskEffect current_effect;
	gint effect_direction;
	gint count;
	GdkPixbuf *pixbufs[15];

	gdouble x_offset;
	gdouble y_offset;
	gint width;
	gint height;
	gdouble rotate_degrees;
	gfloat alpha;

	/* MenuItems */
	AwnTaskMenuItem *menu_items[5];

	/* random */
	guint timestamp;
	/* signal handler ID's, for clean close */
	gulong icon_changed;
	gulong state_changed;
	gulong name_changed; // 3v1n0: added notification on name changing
	gulong win_enter;
	gulong win_leave;
};

enum
{
	MENU_ITEM_CLICKED,
	CHECK_ITEM_CLICKED,
	LAST_SIGNAL
};

static guint awn_task_signals[LAST_SIGNAL] = { 0 };

/* GLOBALS */

static gint menu_item_id = 100;

static const GtkTargetEntry drop_types[] = {
	{ "STRING", 0, 0 }
};
static const gint n_drop_types = G_N_ELEMENTS (drop_types);

static void
awn_task_class_init (AwnTaskClass *class)
{
	GObjectClass *obj_class;
	GtkWidgetClass *widget_class;

	obj_class = G_OBJECT_CLASS (class);
	widget_class = GTK_WIDGET_CLASS (class);

	/* GtkWidget signals */
	widget_class->expose_event = awn_task_expose;
	widget_class->button_press_event = awn_task_button_press;
	widget_class->enter_notify_event = awn_task_proximity_in;
	widget_class->leave_notify_event = awn_task_proximity_out;
	widget_class->drag_motion = awn_task_drag_motion;
	widget_class->drag_leave = awn_task_drag_leave;

	g_type_class_add_private (obj_class, sizeof (AwnTaskPrivate));

	awn_task_signals[MENU_ITEM_CLICKED] =
		g_signal_new ("menu_item_clicked",
			      G_OBJECT_CLASS_TYPE (obj_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (AwnTaskClass, menu_item_clicked),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__UINT,
			      G_TYPE_NONE,
			      1,
			      G_TYPE_UINT);

	awn_task_signals[CHECK_ITEM_CLICKED] =
		g_signal_new ("check_item_clicked",
			      G_OBJECT_CLASS_TYPE (obj_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (AwnTaskClass, check_item_clicked),
			      NULL, NULL,
			      _awn_marshal_VOID__UINT_BOOLEAN,
			      G_TYPE_NONE,
			      2,
			      G_TYPE_UINT, G_TYPE_BOOLEAN);
}

static void
awn_task_init (AwnTask *task)
{
	gtk_widget_add_events (GTK_WIDGET (task),GDK_ALL_EVENTS_MASK);

	gtk_widget_set_size_request(GTK_WIDGET(task), 60, 20);

	gtk_drag_dest_set (GTK_WIDGET (task),
                           GTK_DEST_DEFAULT_MOTION | GTK_DEST_DEFAULT_DROP,
                           drop_types, n_drop_types,
                           GDK_ACTION_MOVE | GDK_ACTION_COPY);


#if !GTK_CHECK_VERSION(2,9,0)
			/* TODO: equivalent of following function: */
#else
			//gtk_drag_dest_set_track_motion  (GTK_WIDGET (task),TRUE);
#endif
	gtk_drag_dest_add_uri_targets (GTK_WIDGET (task));
	/* set all priv variables */
	AwnTaskPrivate *priv;
	priv = AWN_TASK_GET_PRIVATE (task);

	priv->item = FALSE;
	priv->pid = -1;
	priv->application = NULL;
	priv->is_launcher = FALSE;
	priv->is_starter = FALSE;
	priv->is_closing = FALSE;
	priv->hover = FALSE;
	priv->window = NULL;
	priv->title = NULL;
	priv->icon = NULL;
        priv->reflect = NULL;
	priv->progress = 100;
	priv->info = FALSE;
	priv->info_text = NULL;
	priv->is_active = FALSE;
	priv->needs_attention = FALSE;
	priv->effect_lock = FALSE;
	priv->effect_direction = AWN_TASK_EFFECT_DIR_UP;
	priv->current_effect = AWN_TASK_EFFECT_NONE;
	priv->pixbufs[0] = NULL;
	priv->x_offset = 0.0;
	priv->y_offset = 0.0;
	priv->width = 0;
	priv->height = 0;
	priv->rotate_degrees = 0.0;
	priv->alpha = 1.0;
	priv->name_changed = FALSE;
	priv->menu_items[0] = NULL;
	priv->menu_items[1] = NULL;
	priv->menu_items[2] = NULL;
	priv->menu_items[3] = NULL;
	priv->menu_items[4] = NULL;
}


/************* EFFECTS *****************/

static gboolean
_task_opening_effect (AwnTask *task)
{
	AwnTaskPrivate *priv;
	priv = AWN_TASK_GET_PRIVATE (task);
	static gint max = 10;

	if (priv->effect_lock) {
		if ( priv->current_effect != AWN_TASK_EFFECT_OPENING)
			return TRUE;
	} else {
		priv->effect_lock = TRUE;
		priv->current_effect = AWN_TASK_EFFECT_OPENING;
		priv->effect_direction = AWN_TASK_EFFECT_DIR_UP;
		priv->y_offset = priv->settings->bar_height * -1;
	}

	if (priv->effect_direction) {
		priv->y_offset +=1;

		if (priv->y_offset >= max)
			priv->effect_direction = AWN_TASK_EFFECT_DIR_DOWN;

	} else {
		priv->y_offset-=1;

		if (priv->y_offset < 1) {
			/* finished bouncing, back to normal */
			priv->effect_lock = FALSE;
			priv->current_effect = AWN_TASK_EFFECT_NONE;
			priv->effect_direction = AWN_TASK_EFFECT_DIR_UP;
			priv->y_offset = 0;

		}
	}
	gtk_widget_set_size_request(GTK_WIDGET(task), 
				    (priv->settings->bar_height * 5/4), 
				    (priv->settings->bar_height + 2) * 2);

	gtk_widget_queue_draw(GTK_WIDGET(task));


	if (priv->effect_lock == FALSE)
		return FALSE;

	return TRUE;
}


static void
launch_opening_effect (AwnTask *task )
{
	g_timeout_add(AWN_FRAME_RATE, (GSourceFunc)_task_opening_effect, (gpointer)task);
}

static gboolean
_task_launched_effect (AwnTask *task)
{
	AwnTaskPrivate *priv;
	priv = AWN_TASK_GET_PRIVATE (task);
	static gint max = 14;

	if (priv->effect_lock) {
		if ( priv->current_effect != AWN_TASK_EFFECT_OPENING)
			return TRUE;
	} else {
		priv->effect_lock = TRUE;
		priv->current_effect = AWN_TASK_EFFECT_OPENING;
		priv->effect_direction = AWN_TASK_EFFECT_DIR_UP;
		priv->y_offset = 0;
		priv->count = 0;
	}

	if (priv->effect_direction) {
		priv->y_offset +=1;

		if (priv->y_offset >= max)
			priv->effect_direction = AWN_TASK_EFFECT_DIR_DOWN;

	} else {
		priv->y_offset-=1;

		if (priv->y_offset < 1) {
			priv->effect_direction = AWN_TASK_EFFECT_DIR_UP;
			priv->count++;
		}
		if (priv->y_offset < 1 && (priv->window != NULL)) {
			/* finished bouncing, back to normal */
			priv->effect_lock = FALSE;
			priv->current_effect = AWN_TASK_EFFECT_NONE;
			priv->effect_direction = AWN_TASK_EFFECT_DIR_UP;
			priv->y_offset = 0;
			priv->count = 0;
		}
		if ( priv->count > 10 ) {
			priv->effect_lock = FALSE;
			priv->current_effect = AWN_TASK_EFFECT_NONE;
			priv->effect_direction = AWN_TASK_EFFECT_DIR_UP;
			priv->y_offset = 0;
			priv->count = 0;
		}
	}

	gtk_widget_queue_draw(GTK_WIDGET(task));


	if (priv->effect_lock == FALSE)
		return FALSE;

	return TRUE;
}


static void
launch_launched_effect (AwnTask *task )
{
	/*
	static guint tag = NULL; // 3v1n0: fix animation on multiple clicks
	if (tag)
		g_source_remove(tag);
	*/
	g_timeout_add(30, (GSourceFunc)_task_launched_effect, (gpointer)task);
}

static gboolean
_task_hover_effect (AwnTask *task)
{
	AwnTaskPrivate *priv;

	if (!AWN_IS_TASK (task)) return FALSE;

	priv = AWN_TASK_GET_PRIVATE (task);
	static gint max = 15;


	if (priv->effect_lock) {
		if ( priv->current_effect != AWN_TASK_EFFECT_HOVER)
			return TRUE;
	} else {
		priv->effect_lock = TRUE;
		priv->current_effect = AWN_TASK_EFFECT_HOVER;
		priv->effect_direction = AWN_TASK_EFFECT_DIR_UP;
		priv->y_offset = 0;
	}

	if (priv->effect_direction) {
		priv->y_offset = log10 (++priv->count) * 15.0;

		if (priv->y_offset >= max)
			priv->effect_direction = AWN_TASK_EFFECT_DIR_DOWN;
	} else {
		priv->y_offset = log10 (--priv->count) * 15.0;

		if (priv->y_offset < 1) {
			/* finished bouncing, back to normal */
			if (priv->hover)  {
				priv->effect_direction = AWN_TASK_EFFECT_DIR_UP;
				priv->count = 0;
			} else {
				priv->effect_lock = FALSE;
				priv->current_effect = AWN_TASK_EFFECT_NONE;
				priv->effect_direction = AWN_TASK_EFFECT_DIR_UP;
				priv->y_offset = 0;
			}
		}
	}

	gtk_widget_queue_draw(GTK_WIDGET(task));


	if (priv->effect_lock == FALSE)
		return FALSE;

	return TRUE;
}

static gboolean
_task_hover_effect2 (AwnTask *task)
{
	AwnTaskPrivate *priv;
	priv = AWN_TASK_GET_PRIVATE (task);
	if (priv->effect_lock) {
		if ( priv->current_effect != AWN_TASK_EFFECT_HOVER)
			return TRUE;
	} else {
		priv->effect_lock = TRUE;
		priv->current_effect = AWN_TASK_EFFECT_HOVER;
		priv->effect_direction = AWN_TASK_EFFECT_DIR_UP;
		priv->alpha = 1.0;
	}

	if (priv->effect_direction) {
		priv->alpha -=0.05;

		if (priv->alpha <= 0.1)
			priv->effect_direction = AWN_TASK_EFFECT_DIR_DOWN;

	} else {
		priv->alpha+=0.05;

		if (priv->alpha >= 1.0) {
			/* finished bouncing, back to normal */
			if (priv->hover)
				priv->effect_direction = AWN_TASK_EFFECT_DIR_UP;
			else {
				priv->effect_lock = FALSE;
				priv->current_effect = AWN_TASK_EFFECT_NONE;
				priv->effect_direction = AWN_TASK_EFFECT_DIR_UP;
				priv->alpha = 1.0;
			}
		}
	}

	gtk_widget_queue_draw(GTK_WIDGET(task));


	if (priv->effect_lock == FALSE)
		return FALSE;

	return TRUE;
}
GdkPixbuf *
icon_loader_get_icon_spec( const char *name, int width, int height )
{
	printf("%d,%d\n",width,height);
        GdkPixbuf *icon = NULL;
        GdkScreen *screen = gdk_screen_get_default();
        GtkIconTheme *theme = NULL;
        theme = gtk_icon_theme_get_for_screen (screen);

        if (!name)
                return NULL;

        GError *error = NULL;

        GtkIconInfo *icon_info = gtk_icon_theme_lookup_icon (gtk_icon_theme_get_default (),
                                name, width, 0);
	if (icon_info != NULL) {
		icon = gdk_pixbuf_new_from_file_at_size (
				      gtk_icon_info_get_filename (icon_info),
                                      width, -1, &error);
		gtk_icon_info_free(icon_info);
	}

        /* first we try gtkicontheme */
        if (icon == NULL)
        	icon = gtk_icon_theme_load_icon( theme, name, width, GTK_ICON_LOOKUP_FORCE_SVG, &error);
        else {
        	g_print("Icon theme could not be loaded");
        	error = (GError *) 1;
        }
        if (icon == NULL) {
                /* lets try and load directly from file */
                error = NULL;
                GString *str;

                if ( strstr(name, "/") != NULL )
                        str = g_string_new(name);
                else {
                        str = g_string_new("/usr/share/pixmaps/");
                        g_string_append(str, name);
                }

                icon = gdk_pixbuf_new_from_file_at_scale(str->str,
                                                         width,
                                                         height,
                                                         TRUE, &error);
                g_string_free(str, TRUE);
        }

        if (icon == NULL) {
                /* lets try and load directly from file */
                error = NULL;
                GString *str;

                if ( strstr(name, "/") != NULL )
                        str = g_string_new(name);
                else {
                        str = g_string_new("/usr/local/share/pixmaps/");
                        g_string_append(str, name);
                }

                icon = gdk_pixbuf_new_from_file_at_scale(str->str,
                                                         width,
                                                         -1,
                                                         TRUE, &error);
                g_string_free(str, TRUE);
        }

        if (icon == NULL) {
                error = NULL;
                GString *str;

                str = g_string_new("/usr/share/");
                g_string_append(str, name);
                g_string_erase(str, (str->len - 4), -1 );
                g_string_append(str, "/");
		g_string_append(str, "pixmaps/");
		g_string_append(str, name);

		icon = gdk_pixbuf_new_from_file_at_scale
       		                             (str->str,
                                             width,
                                             -1,
                                             TRUE,
                                             &error);
                g_string_free(str, TRUE);
        }

        return icon;
}

static void
launch_hover_effect (AwnTask *task )
{
	AwnTaskPrivate *priv;
	priv = AWN_TASK_GET_PRIVATE (task);

	if (priv->settings->fade_effect) {
		priv->alpha = 1.0;
		//g_timeout_add(25, (GSourceFunc)_task_hover_effect2, (gpointer)task);
	} else if (priv->settings->hover_bounce_effect) {
		g_timeout_add(40, (GSourceFunc)_task_hover_effect, (gpointer)task);
	}
}

static gboolean
_task_attention_effect (AwnTask *task)
{
	AwnTaskPrivate *priv;

	if (!AWN_IS_TASK (task))
		return FALSE;

	priv = AWN_TASK_GET_PRIVATE (task);
	static gint max = 20;

	if (priv->effect_lock) {
		if ( priv->current_effect != AWN_TASK_EFFECT_ATTENTION)
			return TRUE;
	} else {
		priv->effect_lock = TRUE;
		priv->current_effect = AWN_TASK_EFFECT_ATTENTION;
		priv->effect_direction = AWN_TASK_EFFECT_DIR_UP;
		priv->y_offset = 0;
		priv->rotate_degrees = 0.0;
	}

	if (priv->effect_direction) {
		priv->y_offset +=1;

		if (priv->y_offset >= max)
			priv->effect_direction = AWN_TASK_EFFECT_DIR_DOWN;

	} else {
		priv->y_offset-=1;
		if (priv->y_offset < 1) {
			/* finished bouncing, back to normal */
			if (priv->needs_attention)
				priv->effect_direction = AWN_TASK_EFFECT_DIR_UP;
			else {
				priv->effect_lock = FALSE;
				priv->current_effect = AWN_TASK_EFFECT_NONE;
				priv->effect_direction = AWN_TASK_EFFECT_DIR_UP;
				priv->y_offset = 0;
				priv->rotate_degrees = 0.0;
			}
		}
	}

	gtk_widget_queue_draw(GTK_WIDGET(task));


	if (priv->effect_lock == FALSE)
		return FALSE;

	return TRUE;
}

static void
launch_attention_effect (AwnTask *task )
{
	g_timeout_add(25, (GSourceFunc)_task_attention_effect, (gpointer)task);
}

static gboolean
_task_fade_out_effect (AwnTask *task)
{
	AwnTaskPrivate *priv;
	priv = AWN_TASK_GET_PRIVATE (task);

	if (priv->hover) {
		priv->alpha = 1.0;
		return FALSE;
	}

	priv->alpha-=0.05;

	if (priv->alpha <= 0.2) {
		priv->alpha = 0.2;
		gtk_widget_queue_draw(GTK_WIDGET(task));
		return FALSE;
	}

	gtk_widget_queue_draw(GTK_WIDGET(task));

	return TRUE;

}

static void
launch_fade_out_effect (AwnTask *task )
{
	g_timeout_add(15, (GSourceFunc)_task_fade_out_effect, (gpointer)task);
}


static gboolean
_task_fade_in_effect (AwnTask *task)
{
	AwnTaskPrivate *priv;
	priv = AWN_TASK_GET_PRIVATE (task);
	/*
	if (priv->hover) {
		priv->alpha = 1.0;
		return FALSE;
	}
	
	priv->alpha+=0.05;

	if (priv->alpha >= 1.0 ) {
		gtk_widget_queue_draw(GTK_WIDGET(task));
		return FALSE;
	}

	gtk_widget_queue_draw(GTK_WIDGET(task));

	return TRUE;
	*/
	priv->alpha = 1.0;
	gtk_widget_queue_draw(GTK_WIDGET(task));
	return FALSE;

}

static void
launch_fade_in_effect (AwnTask *task )
{
	g_timeout_add(40, (GSourceFunc)_task_fade_in_effect, (gpointer)task);
}

static gboolean
_task_destroy (AwnTask *task)
{
	AwnTaskPrivate *priv;
	priv = AWN_TASK_GET_PRIVATE (task);
	const gint max = priv->settings->bar_height + 2;
	gfloat i;

	if (priv->effect_lock) {
		if ( priv->current_effect != AWN_TASK_EFFECT_CLOSING)
			return TRUE;
	} else {
		priv->effect_lock = TRUE;
		priv->current_effect = AWN_TASK_EFFECT_CLOSING;
		priv->effect_direction = AWN_TASK_EFFECT_DIR_UP;
		priv->y_offset = 0;
	}


	//g_print("%d, %f\n",priv->y_offset, priv->alpha);
	if (priv->y_offset >= max) {
		if (priv->is_launcher)
			awn_task_manager_remove_launcher(priv->task_manager, task);
		else
			awn_task_manager_remove_task(priv->task_manager, task);

		priv->title = NULL;
		g_free(priv->application);
		gtk_widget_hide (GTK_WIDGET(task));
		
		if (priv->icon) {
			gdk_pixbuf_unref(priv->icon);
		}
		if (priv->reflect) {
			gdk_pixbuf_unref(priv->reflect);
		}
		gtk_object_destroy (GTK_OBJECT(task));
		task = NULL;
		return FALSE;

	} else {

		priv->y_offset +=1;
		i = (float) priv->y_offset / max;
		priv->alpha = 1.0 - i;
		gtk_widget_queue_draw(GTK_WIDGET(task));
	}

	return TRUE;

}

static gboolean				// 3v1n0: Change Name (title) effect
_task_change_name_effect (AwnTask *task)	// Based on notification effect
{
	AwnTaskPrivate *priv;
	priv = AWN_TASK_GET_PRIVATE (task);
	static gint max = 14;

	if (priv->effect_lock) {
		if ( priv->current_effect != AWN_TASK_EFFECT_CHANGE_NAME)
			return TRUE;
	} else {
		priv->effect_lock = TRUE;
		priv->current_effect = AWN_TASK_EFFECT_CHANGE_NAME;
		priv->effect_direction = AWN_TASK_EFFECT_DIR_UP;
		priv->y_offset = 0;
		priv->rotate_degrees = 0.0;
	}

	if (priv->effect_direction) {
		priv->y_offset +=1;

		if (priv->y_offset >= max)
			priv->effect_direction = AWN_TASK_EFFECT_DIR_DOWN;

	} else {
		priv->y_offset-=1;
		if (priv->y_offset < 1) {
			/* finished bouncing, back to normal */
			if (priv->name_changed)
				priv->effect_direction = AWN_TASK_EFFECT_DIR_UP;
			else {
				priv->effect_lock = FALSE;
				priv->current_effect = AWN_TASK_EFFECT_NONE;
				priv->effect_direction = AWN_TASK_EFFECT_DIR_UP;
				priv->y_offset = 0;
				priv->rotate_degrees = 0.0;
			}
		}
	}

	gtk_widget_queue_draw(GTK_WIDGET(task));


	if (priv->effect_lock == FALSE)
		return FALSE;

	return TRUE;
}

static gboolean
_launch_name_change_effect (AwnTask *task)
{
	//static guint tag = NULL; // 3v1n0: fix animation on multiple clicks
	//if (tag)
	//	g_source_remove(tag);
	//tag =
	g_timeout_add(30, (GSourceFunc)_task_change_name_effect, (gpointer)task);
	return FALSE;
}

/**********************  CALLBACKS  **********************/


static void
_rounded_rectangle (cairo_t *cr, double w, double h, double x, double y)
{
	const float RADIUS_CORNERS = 1.5;

	cairo_move_to (cr, x+RADIUS_CORNERS, y);
	cairo_line_to (cr, x+w-RADIUS_CORNERS, y);
	cairo_move_to (cr, w+x, y+RADIUS_CORNERS);
	cairo_line_to (cr, w+x, h+y-RADIUS_CORNERS);
	cairo_move_to (cr, w+x-RADIUS_CORNERS, h+y);
	cairo_line_to (cr, x+RADIUS_CORNERS, h+y);
	cairo_move_to (cr, x, h+y-RADIUS_CORNERS);
	cairo_line_to (cr, x, y+RADIUS_CORNERS);


}

static void
_rounded_corners (cairo_t *cr, double w, double h, double x, double y)
{
	double radius = 1.5;
	cairo_move_to (cr, x+radius, y);
	cairo_arc (cr, x+w-radius, y+radius, radius, M_PI * 1.5, M_PI * 2);
	cairo_arc (cr, x+w-radius, y+h-radius, radius, 0, M_PI * 0.5);
	cairo_arc (cr, x+radius,   y+h-radius, radius, M_PI * 0.5, M_PI);
	cairo_arc (cr, x+radius,   y+radius,   radius, M_PI, M_PI * 1.5);

}

static void
_rounded_rect (cairo_t *cr, gint x, gint y, gint w, gint h)
{
	double radius, RADIUS_CORNERS;
        radius = RADIUS_CORNERS = 6;

        cairo_move_to (cr, x+radius, y);
	cairo_arc (cr, x+w-radius, y+radius, radius, M_PI * 1.5, M_PI * 2);
	cairo_arc (cr, x+w-radius, y+h-radius, radius, 0, M_PI * 0.5);
	cairo_arc (cr, x+radius,   y+h-radius, radius, M_PI * 0.5, M_PI);
	cairo_arc (cr, x+radius,   y+radius,   radius, M_PI, M_PI * 1.5);
	cairo_move_to (cr, x+RADIUS_CORNERS, y);
	cairo_line_to (cr, x+w-RADIUS_CORNERS, y);
	cairo_move_to (cr, w+x, y+RADIUS_CORNERS);
	cairo_line_to (cr, w+x, h+y-RADIUS_CORNERS);
	cairo_move_to (cr, w+x-RADIUS_CORNERS, h+y);
	cairo_line_to (cr, x+RADIUS_CORNERS, h+y);
	cairo_move_to (cr, x, h+y-RADIUS_CORNERS);
	cairo_line_to (cr, x, y+RADIUS_CORNERS);

}

static void
draw (GtkWidget *task, cairo_t *cr)
{
	AwnTaskPrivate *priv;
	AwnSettings *settings;
	double width, height;

	priv = AWN_TASK_GET_PRIVATE (task);
	settings = priv->settings;

	width = task->allocation.width;
	height = task->allocation.height;

	/* task back */
	cairo_set_source_rgba (cr, 1, 0, 0, 0.0);
	cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
	cairo_rectangle(cr, -1, -1, width+1, height+1);
	cairo_fill (cr);

	cairo_set_operator (cr, CAIRO_OPERATOR_OVER);

	/* active */
	if (priv->window && wnck_window_is_active(priv->window)) {

		cairo_set_source_rgba(cr, settings->border_color.red, 
                                          settings->border_color.green,
                                          settings->border_color.blue, 
                                          0.2);
		_rounded_rect(cr, 0 , settings->bar_height, 
				width, settings->bar_height);
          
                cairo_fill(cr);
		
		cairo_set_source_rgba(cr, settings->border_color.red, 
                                          settings->border_color.green,
                                          settings->border_color.blue, 
                                          0.2/3);	
                _rounded_rect(cr, 0 , settings->bar_height*2, 
				width, settings->icon_offset);
		cairo_fill(cr);
	}
	/* content */
	if (priv->icon) {
		double x1, y1;

		x1 = (width-priv->icon_width)/2;
		y1 = settings->bar_height - priv->y_offset;

		gdk_cairo_set_source_pixbuf (cr, priv->icon, x1, y1);
		cairo_paint_with_alpha(cr, priv->alpha);

		if (priv->y_offset >= 0 && priv->reflect)
		{
			y1 = settings->bar_height 
                                + priv->icon_height + priv->y_offset;
			gdk_cairo_set_source_pixbuf (cr, 
                                                     priv->reflect,
                                                     x1, y1);
			cairo_paint_with_alpha(cr, priv->alpha/3);
		}
	}

        /* Don't paint in bottom 3px if there is an bar_angle */
        if ( settings->bar_angle != 0) {
                cairo_save (cr);
                cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
                cairo_set_source_rgba (cr, 1, 1, 1, 0);
                cairo_rectangle (cr, 
                                 0, 
                                 ((settings->bar_height *2)-4)+ settings->icon_offset,
                                 width,
                                 4);
                cairo_fill (cr);
                cairo_restore (cr);
        }

	/* arrows */
	double x1;
	x1 = width/2.0;
	cairo_set_source_rgba (cr, settings->arrow_color.red,
				   settings->arrow_color.green,
				   settings->arrow_color.blue,
				   settings->arrow_color.alpha);
	cairo_move_to(cr, x1-5, (settings->bar_height * 2));
	cairo_line_to(cr, x1, (settings->bar_height *2) - 5);
	cairo_line_to(cr, x1+5, (settings->bar_height * 2));
	cairo_close_path (cr);

	if (settings->tasks_have_arrows) {
		if (priv->window != NULL)
			cairo_fill(cr);

	} else if (priv->is_launcher && (priv->window == NULL))
		cairo_fill(cr);
	else {
		;
	}
	
	/* progress meter */
	cairo_new_path (cr);
	if (priv->progress != 100) {
		cairo_move_to (cr, width/2.0, settings->bar_height *1.5);
		cairo_set_source_rgba (cr, settings->background.red,
					   settings->background.green,
					   settings->background.blue,
					   settings->background.alpha);

		cairo_arc (cr, width/2.0, settings->bar_height *1.5, 15, 0, 360.0 * (M_PI / 180.0) );
		cairo_fill (cr);

		cairo_move_to (cr, width/2.0, settings->bar_height *1.5);
		cairo_set_source_rgba (cr, settings->text_color.red,
				   	   settings->text_color.green,
				   	   settings->text_color.blue,
				   	   0.8);
		cairo_arc (cr, width/2.0,
			   settings->bar_height * 1.5, 
			   12, 270.0 * (M_PI / 180.0),  
			   (270.0 + ((double)(priv->progress/100.0) * 360.0))  * (M_PI / 180.0) );
		cairo_fill (cr);
	}

	if ( (priv->progress == 100) && priv->info) {

		cairo_text_extents_t extents;
		cairo_select_font_face (cr, "Sans",CAIRO_FONT_SLANT_NORMAL,
					 	   CAIRO_FONT_WEIGHT_BOLD);
		gint len = strlen (priv->info_text);
		if ( len > 8)
			cairo_set_font_size (cr, 8);
		else if (len > 4)
			cairo_set_font_size (cr, 10);
		else
			cairo_set_font_size (cr, 12);
		cairo_text_extents (cr, priv->info_text, &extents);

		cairo_move_to (cr, width/2.0, settings->bar_height *1.5);
		cairo_set_source_rgba (cr, settings->background.red,
					   settings->background.green,
					   settings->background.blue,
					   settings->background.alpha);

		_rounded_rectangle (cr, (double) extents.width+8.0, 
					(double) extents.height+84.0, 
					(width/2.0) - 4.0 - ( (extents.width+extents.x_bearing)/2.0), 
					(settings->bar_height *1.5)- ( extents.height /2.0)-4.0);
		
		_rounded_corners (cr, (double) extents.width+8.0, 
				      (double) extents.height+8.0, 
				      (width/2.0) - 4.0 - ( (extents.width+extents.x_bearing)/2.0), 
				      (settings->bar_height *1.5)- ( extents.height /2.0)-4.0);
		
		cairo_fill (cr);


		cairo_set_source_rgba (cr, settings->text_color.red,
				   settings->text_color.green,
				   settings->text_color.blue,
				   0.8);

		cairo_move_to (cr, (width/2.0) - ((extents.width+ extents.x_bearing)/2.0)-1, (settings->bar_height *1.5)+ ( extents.height /2.0));
		cairo_show_text (cr, priv->info_text);
	}
}

static gboolean
awn_task_expose (GtkWidget *task, GdkEventExpose *event)
{
	AwnTaskPrivate *priv;
	cairo_t *cr;
	priv = AWN_TASK_GET_PRIVATE (task);

	/* get a cairo_t */
	cr = gdk_cairo_create (task->window);

	draw (task, cr);

	cairo_destroy (cr);

	return FALSE;
}

static void
awn_task_launch_unique (AwnTask *task, const char *arg_str)
{
	AwnTaskPrivate *priv;
	priv = AWN_TASK_GET_PRIVATE (task);;

	GError *err = NULL;
	int pid = gnome_desktop_item_launch_on_screen
                                            (priv->item,
                                             NULL,
                                             0,
                                             gdk_screen_get_default(),
                                             -1,
                                             &err);

        if (err)
        	g_print("Error: %s", err->message);
        else
        	g_print("Launched application : %d\n", pid);

}

static void
awn_task_launch (AwnTask *task, const char *arg_str)
{
	AwnTaskPrivate *priv;
	priv = AWN_TASK_GET_PRIVATE (task);;

	GError *err = NULL;
	priv->pid = gnome_desktop_item_launch_on_screen
                                            (priv->item,
                                             NULL,
                                             0,
                                             gdk_screen_get_default(),
                                             -1,
                                             &err);

        if (err)
        	g_print("Error: %s", err->message);
        else
        	g_print("Launched application : %d\n", priv->pid);

}

static gboolean
awn_task_button_press (GtkWidget *task, GdkEventButton *event)
{
	static guint32 past_time; // 3v1n0 double-click (or more) prevention
	AwnTaskPrivate *priv;
	GtkWidget *menu = NULL;

	priv = AWN_TASK_GET_PRIVATE (task);

	if (event->time - past_time > AWN_CLICK_IDLE_TIME) {
		past_time = event->time;
		if (priv->window) {

			switch (event->button) {
				case 1:
					if ( wnck_window_is_active( priv->window ) ) {
						wnck_window_minimize( priv->window );
						return TRUE;
					}
					wnck_window_activate_transient( priv->window,
							event->time );
					break;
				case 2:
					if (priv->is_launcher)
						awn_task_launch_unique(AWN_TASK (task), NULL);
					break;
				case 3:
					menu = wnck_create_window_action_menu(priv->window);
					awn_task_create_menu(AWN_TASK(task), GTK_MENU (menu));
					gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL,
							NULL, 3, event->time);
					break;
				default:
					return FALSE;
			}
		} else if (priv->is_launcher) {
			switch (event->button) {
				case 1:
					awn_task_launch(AWN_TASK (task), NULL);
					launch_launched_effect(AWN_TASK (task));
					break;
// 				case 2:
// 					// Manage middle click on launchers
// 					g_print("Middle click pressed for launcher\n");
// 					break;
				case 3:
					menu = gtk_menu_new();
					awn_task_create_menu(AWN_TASK(task), GTK_MENU (menu));
					gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL,
							NULL, 3, event->time);
					break;
				default:
					return FALSE;
			}
		} else {
			;
		}
	}
	else {
		return FALSE;
	}

	return TRUE;
}

static void
_task_drag_data_recieved (GtkWidget *widget, GdkDragContext *context,
				  gint x, gint y, GtkSelectionData *selection_data,
				  guint target_type, guint time,
                                              AwnTask *task)
{

        GList      *li;
	GList      *list;

        gboolean delete_selection_data = FALSE;

        AwnTaskPrivate *priv;
	priv = AWN_TASK_GET_PRIVATE (task);
	gtk_drag_finish (context, TRUE, FALSE, time);

	if (!priv->is_launcher) {
		gtk_drag_finish (context, FALSE, delete_selection_data, time);
		return;
	}

	char *res = NULL;
	res = strstr ((char *)selection_data->data, ".desktop");
	if (res)
		return;

	list = NULL;
        list = gnome_vfs_uri_list_parse ((const char*) selection_data->data);
	for (li = list; li != NULL; li = li->next) {
		GnomeVFSURI *uri = li->data;
		li->data = gnome_vfs_uri_to_string (uri, 0 /* hide_options */);
		gnome_vfs_uri_unref (uri);
	}

	GError *err = NULL;
	priv->pid = gnome_desktop_item_launch_on_screen
                                            (priv->item,
                                             list,
                                             0,
                                             gdk_screen_get_default(),
                                             -1,
                                             &err);

        if (err)
        	g_print("Error: %s", err->message);
        else
        	g_print("Launched application : %d\n", priv->pid);

	g_list_foreach (list, (GFunc)g_free, NULL);
	g_list_free (list);

	return;


}



static gboolean
awn_task_proximity_in (GtkWidget *task, GdkEventCrossing *event)
{
	AwnTaskPrivate *priv;
	AwnSettings *settings;

	priv = AWN_TASK_GET_PRIVATE (task);
	settings = priv->settings;

	if (priv->title) {
		gint x, y;
		gdk_window_get_origin (task->window, &x, &y);

		awn_title_show (AWN_TITLE (priv->title), awn_task_get_name(AWN_TASK(task)), x+30, 0);

	}

	if (priv->hover) {
		if (settings->fade_effect) {
			priv->alpha = 1.0;
		}
		return TRUE;
	} else {
		if (priv->needs_attention)
			return FALSE;
		priv->hover = TRUE;

		if (!settings->fade_effect) {
			if (priv->current_effect != AWN_TASK_EFFECT_HOVER)
				launch_hover_effect (AWN_TASK (task));
		} else {
			priv->alpha = 1.0;
			gtk_widget_queue_draw(GTK_WIDGET(task));
			//launch_fade_in_effect(AWN_TASK (task));
		}
	}


	return TRUE;
}

static gboolean
awn_task_proximity_out (GtkWidget *task, GdkEventCrossing *event)
{
	AwnTaskPrivate *priv;
	priv = AWN_TASK_GET_PRIVATE (task);

	if (priv->title)
		awn_title_show(AWN_TITLE (priv->title), " ", 20, 0);
	priv->hover = FALSE;
	
	if (priv->settings->fade_effect) {
		priv->alpha = 1.0;
		gtk_widget_queue_draw(GTK_WIDGET(task));
	}
	
	//priv->alpha = 1.0;
	//gtk_widget_queue_draw(GTK_WIDGET(task));
	//launch_fade_in_effect(task);
	return FALSE;
}

static gboolean
awn_task_win_enter_in (GtkWidget *window, GdkEventMotion *event, AwnTask *task)
{
	AwnTaskPrivate *priv;
	AwnSettings *settings;
	//g_return_if_fail(AWN_IS_TASK(task));
	if (!AWN_IS_TASK (task)) return FALSE;

	priv = AWN_TASK_GET_PRIVATE (task);
	settings = priv->settings;
	if (settings->fade_effect)
		launch_fade_out_effect(task);
	return FALSE;
}

static gboolean
awn_task_win_enter_out (GtkWidget *window, GdkEventCrossing *event, AwnTask *task)
{
	AwnTaskPrivate *priv;

	//g_return_if_fail(AWN_IS_TASK(task));
	if (!AWN_IS_TASK (task)) return FALSE;

	priv = AWN_TASK_GET_PRIVATE (task);
	//priv->alpha = 1.0;
	
	if (priv->settings->fade_effect) {
		priv->alpha = 0.2;
		launch_fade_in_effect(task);
	}
	
	gtk_widget_queue_draw(GTK_WIDGET(task));
	return FALSE;
}

static gboolean
activate_window(AwnTask *task)
{
	AwnTaskPrivate *priv;
	priv = AWN_TASK_GET_PRIVATE (task);

	if (priv->hover)
		wnck_window_activate_transient( priv->window, priv->timestamp );

	return FALSE;
}

static gboolean
awn_task_drag_motion (GtkWidget *task,
		GdkDragContext *context, gint x, gint y, guint t)
{
        AwnTaskPrivate *priv;
	priv = AWN_TASK_GET_PRIVATE (task);

	if (priv->is_closing)
		return FALSE;

	if (priv->window) {

		if ( wnck_window_is_active( priv->window ) )
			return FALSE;
		else {
			priv->hover = TRUE;
			priv->timestamp = gtk_get_current_event_time();
			g_timeout_add (1000, (GSourceFunc)activate_window, (gpointer)task);
		}
	}

	return FALSE;
}

static void
awn_task_drag_leave (GtkWidget *task, GdkDragContext *drag_context,
                                        guint time)
{
 	AwnTaskPrivate *priv;
	priv = AWN_TASK_GET_PRIVATE (task);
	priv->hover = FALSE;
}

static void
_task_wnck_icon_changed (WnckWindow *window, AwnTask *task)
{
        AwnTaskPrivate *priv;
	priv = AWN_TASK_GET_PRIVATE (task);

	if (priv->is_launcher)
		return;

        GdkPixbuf *old = NULL;
        GdkPixbuf *old_reflect = NULL;
        old = priv->icon;
        old_reflect = priv->reflect;
	int height = priv->settings->bar_height;
	if ((priv->settings->task_width - 12) < height) {
		height = priv->settings->task_width - 12;
	}
	priv->icon = awn_x_get_icon_for_window (priv->window, 
                                                height, height);
	priv->reflect = gdk_pixbuf_flip (priv->icon, FALSE); 

        priv->icon_width = gdk_pixbuf_get_width(priv->icon);
	priv->icon_height = gdk_pixbuf_get_height(priv->icon);
	
        gdk_pixbuf_unref(old);
        gdk_pixbuf_unref(old_reflect);
        gtk_widget_queue_draw(GTK_WIDGET(task));
}

static gboolean
_task_wnck_name_hide (AwnTask *task)
{
	AwnTaskPrivate *priv;
	priv = AWN_TASK_GET_PRIVATE (task);
	awn_title_show(AWN_TITLE (priv->title), " ", 20, 0);
	priv->name_changed = FALSE;
	return FALSE;
}

static void
_task_wnck_name_changed (WnckWindow *window, AwnTask *task)
{
    AwnTaskPrivate *priv;

	g_return_if_fail (AWN_IS_TASK (task));

	priv = AWN_TASK_GET_PRIVATE (task);

	if (!priv->window)
		return;
	if (!priv->settings->name_change_notify) {
		return;
	}

	if (priv->current_effect == AWN_TASK_EFFECT_CHANGE_NAME)
		return;

	//g_print("Name changed on window '%s'\n",
        //   wnck_window_get_name (priv->window));

	if (!wnck_window_is_active(priv->window)) {
		gint x, y;
		gdk_window_get_origin (GTK_WIDGET(task)->window, &x, &y);

		if (!priv->needs_attention) {
			priv->name_changed = TRUE;
			_launch_name_change_effect(task);
		}

		awn_title_show (AWN_TITLE (priv->title), awn_task_get_name(AWN_TASK(task)), x+30, 0);

		g_timeout_add(2500, (GSourceFunc)_task_wnck_name_hide, (gpointer)task);

	}
}

static void
_task_wnck_state_changed (WnckWindow *window, WnckWindowState  old,
                                WnckWindowState  new, AwnTask *task)
{
	AwnTaskPrivate *priv;
	priv = AWN_TASK_GET_PRIVATE (task);

	if (priv->window == NULL )
		return;

	if (wnck_window_is_skip_tasklist(priv->window))
		gtk_widget_hide (GTK_WIDGET(task));
	else
		gtk_widget_show (GTK_WIDGET(task));

	if (wnck_window_needs_attention(priv->window)) {
		if (!priv->needs_attention) {
			launch_attention_effect(task);
			priv->needs_attention = TRUE;
		}
	} else
		priv->needs_attention = FALSE;
}

/**********************Gets & Sets **********************/

gboolean
awn_task_get_is_launcher (AwnTask *task)
{
	AwnTaskPrivate *priv;
	priv = AWN_TASK_GET_PRIVATE (task);
	return priv->is_launcher;
}

gboolean
awn_task_set_window (AwnTask *task, WnckWindow *window)
{
	g_return_val_if_fail(WNCK_IS_WINDOW(window), 0);

	AwnTaskPrivate *priv;
	priv = AWN_TASK_GET_PRIVATE (task);

	if (priv->window != NULL)
		return FALSE;

	priv->window = window;
	if (!priv->is_launcher) {
		priv->icon = awn_x_get_icon_for_window (priv->window, 
                                               priv->settings->bar_height,
                                               priv->settings->bar_height);
                priv->reflect = gdk_pixbuf_flip (priv->icon, FALSE);
                		
                priv->icon_width = gdk_pixbuf_get_width(priv->icon);
		priv->icon_height = gdk_pixbuf_get_height(priv->icon);

	}
	priv->icon_changed = g_signal_connect (G_OBJECT (priv->window), "icon_changed",
        		  G_CALLBACK (_task_wnck_icon_changed), (gpointer)task);

	priv->state_changed = g_signal_connect (G_OBJECT (priv->window), "state_changed",
        		  G_CALLBACK (_task_wnck_state_changed), (gpointer)task);

	priv->name_changed = g_signal_connect (G_OBJECT (priv->window), "name_changed",
        		  G_CALLBACK (_task_wnck_name_changed), (gpointer)task);

        /* if launcher, set a launch_sequence
        else if starter, stop the launch_sequence, disable starter flag*/

        //awn_task_refresh_icon_geometry(task);
        if (wnck_window_is_skip_tasklist(window))
        	return TRUE;

        if (priv->is_launcher)
        	return TRUE;

        launch_opening_effect(task);
	return TRUE;
}

gboolean
awn_task_set_launcher (AwnTask *task, GnomeDesktopItem *item)
{
	AwnTaskPrivate *priv;
	gchar *icon_name;

	priv = AWN_TASK_GET_PRIVATE (task);

	priv->is_launcher = TRUE;
	icon_name = gnome_desktop_item_get_icon (item, priv->settings->icon_theme );
	if (!icon_name)
		return FALSE;
	g_free (icon_name);
	priv->item = item;
	priv->icon = awn_x_get_icon_for_launcher (item, 
                                               priv->settings->bar_height, 
                                               priv->settings->bar_height);
        priv->reflect = gdk_pixbuf_flip (priv->icon, FALSE);
	priv->icon_width = gdk_pixbuf_get_width(priv->icon);
	priv->icon_height = gdk_pixbuf_get_height(priv->icon);
	launch_opening_effect(task);

	return TRUE;
}

gboolean
awn_task_is_launcher (AwnTask *task)
{
	AwnTaskPrivate *priv;
	priv = AWN_TASK_GET_PRIVATE (task);
	return priv->is_launcher;
}

WnckWindow *
awn_task_get_window (AwnTask *task)
{
	AwnTaskPrivate *priv;
	priv = AWN_TASK_GET_PRIVATE (task);
	return priv->window;
}

gulong
awn_task_get_xid (AwnTask *task)
{
	AwnTaskPrivate *priv;
	priv = AWN_TASK_GET_PRIVATE (task);

	if (priv->window == NULL)
		return 0;

	g_return_val_if_fail(WNCK_IS_WINDOW(priv->window), 0);
	return wnck_window_get_xid(priv->window);
}

gint
awn_task_get_pid (AwnTask *task)
{
	AwnTaskPrivate *priv;
	priv = AWN_TASK_GET_PRIVATE (task);

	if (priv->window != NULL)
		return wnck_window_get_pid (priv->window);
	return priv->pid;
}
void
awn_task_set_is_active (AwnTask *task, gboolean is_active)
{
	AwnTaskPrivate *priv;
	priv = AWN_TASK_GET_PRIVATE (task);

	if (priv->window == NULL)
		return;
	priv->is_active = wnck_window_is_active(priv->window);
	gtk_widget_queue_draw(GTK_WIDGET(task));
}

void
awn_task_set_needs_attention (AwnTask *task, gboolean needs_attention)
{
	AwnTaskPrivate *priv;
	priv = AWN_TASK_GET_PRIVATE (task);

	if (needs_attention == priv->needs_attention)
		return;

	priv->needs_attention = needs_attention;
	if (needs_attention)
		launch_opening_effect(task);
}

const char*
awn_task_get_name (AwnTask *task)
{
	AwnTaskPrivate *priv;
	priv = AWN_TASK_GET_PRIVATE (task);

	const char *name = NULL;


	if (priv->window)
		name =  wnck_window_get_name(priv->window);

	else if (priv->is_launcher)
		name = gnome_desktop_item_get_localestring (priv->item,
						       GNOME_DESKTOP_ITEM_NAME);
	else
		name =  "No name";
	return name;
}

const char*
awn_task_get_application(AwnTask *task)
{
	AwnTaskPrivate *priv;
	WnckApplication *wnck_app;
	char *app;
	GString *str;

	priv = AWN_TASK_GET_PRIVATE (task);
	app = NULL;

	if (priv->application)
		return priv->application;

	if (priv->is_launcher) {

		str = g_string_new(gnome_desktop_item_get_string (priv->item,
					GNOME_DESKTOP_ITEM_EXEC));
		int i = 0;
		for (i=0; i < str->len; i++) {
			if ( str->str[i] == ' ')
				break;
		}

		if (i)
			str = g_string_truncate (str,i);
		priv->application = g_strdup(str->str);
		app = g_string_free (str, TRUE);

	} else if (priv->window) {
		wnck_app = wnck_window_get_application(priv->window);
		str = g_string_new (wnck_application_get_name(wnck_app));
		str = g_string_ascii_down (str);
		priv->application = g_strdup(str->str);
		app = g_string_free (str, TRUE);

	} else
		priv->application = NULL;

	return priv->application;
}

void
awn_task_set_title (AwnTask *task, AwnTitle *title)
{
	AwnTaskPrivate *priv;
	priv = AWN_TASK_GET_PRIVATE (task);

	priv->title = title;
}

AwnSettings*
awn_task_get_settings (AwnTask *task)
{
	AwnTaskPrivate *priv;
	priv = AWN_TASK_GET_PRIVATE (task);
	return priv->settings;
}

void
awn_task_refresh_icon_geometry (AwnTask *task)
{
	AwnTaskPrivate *priv;
	static gint old_x = 0;
	static gint old_y = 0;
	gint x, y;
	gint width, height;

	priv = AWN_TASK_GET_PRIVATE (task);
	if (priv->window == NULL)
		return;
	gdk_window_get_origin (GTK_WIDGET(task)->window, &x, &y);
	gdk_drawable_get_size (GDK_DRAWABLE (GTK_WIDGET(task)->window), 
                               &width, &height);
	
	width = priv->icon_width;
	height = priv->icon_height;

	//printf("width: %d, height: %d\n", width, height);

	if ( (x != old_x) || (y != old_y) ) {
		gint res = 0;

		if (x > old_x)
			res = x - old_x;
		else
			res = old_x - x;
		if (res > 10) {
			wnck_window_set_icon_geometry (priv->window, x, y, width, height);
		}
		old_x = x;
		old_y = y;
	}

}

void
awn_task_update_icon (AwnTask *task)
{
	AwnTaskPrivate *priv;
	GdkPixbuf *old = NULL;
	GdkPixbuf *old_reflect = NULL;
	priv = AWN_TASK_GET_PRIVATE (task);

	old = priv->icon;
	old_reflect = priv->reflect;

	int height = priv->settings->bar_height;
	if ((priv->settings->task_width - 12) < height) {
		height = priv->settings->task_width - 12;
	}
	priv->icon = awn_x_get_icon_for_launcher (priv->item,
                                                  height, height);
        priv->reflect = gdk_pixbuf_flip (priv->icon, FALSE);
       
        priv->icon_width = gdk_pixbuf_get_width(priv->icon);
	priv->icon_height = gdk_pixbuf_get_height(priv->icon);

	gdk_pixbuf_unref (old);
	gdk_pixbuf_unref (old_reflect);
}

void 
awn_task_set_width (AwnTask *task, gint width)
{
	AwnTaskPrivate *priv;
	GdkPixbuf *old = NULL;
	GdkPixbuf *old_reflect = NULL;
	
	if (!AWN_IS_TASK (task))
		return;
		
	g_return_if_fail (AWN_IS_TASK (task));
	priv = AWN_TASK_GET_PRIVATE (task);
	
	old = priv->icon;
	old_reflect = priv->reflect;

	if (priv->is_launcher) {
		char * icon_name = gnome_desktop_item_get_icon (priv->item, priv->settings->icon_theme );
		if (!icon_name)
			priv->icon = awn_x_get_icon_for_window (priv->window, 
                            width-6, width-6);
                        priv->reflect = gdk_pixbuf_flip (priv->icon,FALSE);

		priv->icon = icon_loader_get_icon_spec(icon_name, width-6, width-6);
		g_free (icon_name);
        } else {
        	if (WNCK_IS_WINDOW (priv->window))
        		priv->icon = awn_x_get_icon_for_window (priv->window, width-6, width-6);
                        priv->reflect = gdk_pixbuf_flip (priv->icon,FALSE);

        }
        	
	if (G_IS_OBJECT (priv->icon)) {
	        priv->icon_width = gdk_pixbuf_get_width(priv->icon);
		priv->icon_height = gdk_pixbuf_get_height(priv->icon);
	}
	if (G_IS_OBJECT (old) && priv->is_launcher)
		gdk_pixbuf_unref (old);
		
	if (G_IS_OBJECT (old_reflect))
		gdk_pixbuf_unref (old_reflect);

	gtk_widget_set_size_request (GTK_WIDGET (task), 
				     width, 
				     (priv->settings->bar_height + 2) * 2);		
}


GnomeDesktopItem* 
awn_task_get_item (AwnTask *task)
{
	AwnTaskPrivate *priv;
	g_return_val_if_fail (AWN_IS_TASK (task), NULL);
	priv = AWN_TASK_GET_PRIVATE (task);

        return priv->item;
}

/********************* DBUS FUNCTIONS *******************/

void
awn_task_set_custom_icon (AwnTask *task, GdkPixbuf *icon)
{
	AwnTaskPrivate *priv;
	GdkPixbuf *old_icon;
	GdkPixbuf *old_reflect;

	g_return_if_fail (AWN_IS_TASK (task));

	priv = AWN_TASK_GET_PRIVATE (task);
	old_icon = priv->icon;
	old_reflect = priv->reflect;

	priv->icon = icon;
        priv->reflect = gdk_pixbuf_flip (priv->icon,FALSE);
	priv->icon_width = gdk_pixbuf_get_width(priv->icon);
	priv->icon_height = gdk_pixbuf_get_height(priv->icon);

	g_object_unref (G_OBJECT (old_icon));
	g_object_unref (G_OBJECT (old_reflect));

	gtk_widget_queue_draw(GTK_WIDGET(task));
}

void
awn_task_unset_custom_icon (AwnTask *task)
{
	AwnTaskPrivate *priv;
	GdkPixbuf *old_icon;
	GdkPixbuf *old_reflect;
	char *icon_name = NULL;

	g_return_if_fail (AWN_IS_TASK (task));

	priv = AWN_TASK_GET_PRIVATE (task);
	old_icon = priv->icon;
	old_reflect = priv->reflect;

	if (priv->is_launcher) {
		icon_name = gnome_desktop_item_get_icon (priv->item, priv->settings->icon_theme );
		if (!icon_name)
			priv->icon = awn_x_get_icon_for_window (priv->window, priv->settings->bar_height, priv->settings->bar_height);

		priv->icon = icon_loader_get_icon_spec(icon_name,priv->settings->bar_height,priv->settings->bar_height);

                g_free (icon_name);
        } else {
        	priv->icon = awn_x_get_icon_for_window (priv->window,priv->settings->bar_height,priv->settings->bar_height);
        }
        priv->reflect = gdk_pixbuf_flip (priv->icon,FALSE);
        priv->icon_width = gdk_pixbuf_get_width(priv->icon);
	priv->icon_height = gdk_pixbuf_get_height(priv->icon);

	g_object_unref (G_OBJECT (old_icon));
	g_object_unref (G_OBJECT (old_reflect));

	gtk_widget_queue_draw(GTK_WIDGET(task));
}

void
awn_task_set_progress (AwnTask *task, gint progress)
{
	AwnTaskPrivate *priv;

	g_return_if_fail (AWN_IS_TASK (task));

	priv = AWN_TASK_GET_PRIVATE (task);

	if ( (progress < 101) && (progress >= 0))
		priv->progress = progress;
	else
		priv->progress = 100;
	gtk_widget_queue_draw(GTK_WIDGET(task));
}

void
awn_task_set_info (AwnTask *task, const gchar *info)
{
	AwnTaskPrivate *priv;

	g_return_if_fail (AWN_IS_TASK (task));

	priv = AWN_TASK_GET_PRIVATE (task);

	if (priv->info_text) {
		g_free (priv->info_text);
		priv->info_text = NULL;
	}
	priv->info = TRUE;
	priv->info_text = g_strdup (info);

	gtk_widget_queue_draw(GTK_WIDGET(task));
}

void
awn_task_unset_info (AwnTask *task)
{
	AwnTaskPrivate *priv;

	g_return_if_fail (AWN_IS_TASK (task));

	priv = AWN_TASK_GET_PRIVATE (task);

	priv->info = FALSE;
	if (priv->info_text) {
		g_free (priv->info_text);
		priv->info_text = NULL;
	}
	gtk_widget_queue_draw(GTK_WIDGET(task));
}

gint
awn_task_add_menu_item (AwnTask *task, GtkMenuItem *item)
{
	AwnTaskPrivate *priv;
	AwnTaskMenuItem *menu_item;

	g_return_val_if_fail (AWN_IS_TASK (task), 0);

	priv = AWN_TASK_GET_PRIVATE (task);

	int i;
	for (i = 0; i < 5; i++) {
		if (priv->menu_items[i] == NULL) {
			g_object_ref (G_OBJECT (item));
			menu_item = g_new0 (AwnTaskMenuItem, 1);
			menu_item->type = AWN_TASK_MENU_TYPE_NORMAL;
			menu_item->item = GTK_WIDGET (item);
			menu_item->active = FALSE;
			priv->menu_items[i] = menu_item;
			menu_item->id =  menu_item_id++;
			return menu_item->id;
		}
	}

	return 0;
}

gint
awn_task_add_check_item (AwnTask *task, GtkMenuItem *item)
{
	AwnTaskPrivate *priv;
	AwnTaskMenuItem *menu_item;

	g_return_val_if_fail (AWN_IS_TASK (task), 0);

	priv = AWN_TASK_GET_PRIVATE (task);

	int i;
	for (i = 0; i < 5; i++) {
		if (priv->menu_items[i] == NULL) {
			g_object_ref (G_OBJECT (item));
			menu_item = g_new0 (AwnTaskMenuItem, 1);
			menu_item->type = AWN_TASK_MENU_TYPE_CHECK;
			menu_item->item = GTK_WIDGET (item);
			menu_item->active = gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (item));
			priv->menu_items[i] = menu_item;
			menu_item->id =  menu_item_id++;
			return menu_item->id;
		}
	}

	return 0;
}

void
awn_task_set_check_item (AwnTask *task, gint id, gboolean active)
{
	AwnTaskPrivate *priv;
	AwnTaskMenuItem *menu_item;

	g_return_if_fail (AWN_IS_TASK (task));

	priv = AWN_TASK_GET_PRIVATE (task);
	int i;
	for (i = 0; i < 5; i++) {
		menu_item = priv->menu_items[i];
		if (menu_item != NULL) {
			if (menu_item->id == id) {
				menu_item->active = active;
			}
		}
	}
}

/********************* MISC FUNCTIONS *******************/


static void
_task_choose_custom_icon (AwnTask *task)
{
#define PIXBUF_SAVE_PATH ".awn/custom-icons"

	AwnTaskPrivate *priv;
	GtkWidget *dialog;
	gint res = -1;
	GdkPixbuf *pixbuf = NULL;
	GdkPixbuf *old_icon = NULL;
	GdkPixbuf *old_reflect = NULL;
	GError *err = NULL;
	gchar *filename = NULL;
	gchar *save = NULL;
	gchar *name;
	
	g_return_if_fail (AWN_IS_TASK (task));
	priv = AWN_TASK_GET_PRIVATE (task);
	
	/* Create the dialog */
	dialog = gtk_file_chooser_dialog_new ("Choose New Image...",
				GTK_WINDOW (priv->settings->window),
				GTK_FILE_CHOOSER_ACTION_OPEN,
				GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
				GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
				NULL);
	
	/* Run it and get the user response */
	res = gtk_dialog_run (GTK_DIALOG (dialog));
	
	/* If not accept, clean up and return */
	if (res != GTK_RESPONSE_ACCEPT) {
		gtk_widget_hide (dialog);
		gtk_widget_destroy (dialog);
		return;
	}
	
	/* Okay, the user has chosen a new icon, so lets load it up */
	filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
	pixbuf = gdk_pixbuf_new_from_file_at_size (filename, priv->settings->bar_height, priv->settings->bar_height, NULL);
	
	/* Check if is actually a pixbuf */
	if (pixbuf == NULL) {
		g_free (filename);
		gtk_widget_hide (dialog);
		gtk_widget_destroy (dialog);
		return;
	}
	
	/* So we have a nice new pixbuf, we now want to save it's location
	   for the future */
	if (priv->is_launcher) {
		name = g_strdup (gnome_desktop_item_get_string (priv->item,
						   GNOME_DESKTOP_ITEM_EXEC));
	} else {
		WnckApplication *app = NULL;
		app = wnck_window_get_application (priv->window);
		if (app == NULL)
			name = NULL;
		else
			name = g_strdup (wnck_application_get_name (app));
	}
	if (name == NULL) {
		/* Somethings gone very wrong */
		g_free (filename);
		gtk_widget_hide (dialog);
		gtk_widget_destroy (dialog);
		return;
	}
	
	/* Replace spaces with dashs */
	int i = 0;
	for (i = 0; i < strlen (name); i++) {
		if (name[i] == ' ')
			name[i] = '-';
	}
	
	/* Now lets build the save-filename and save it */
	save = g_build_filename (g_get_home_dir (),
				 PIXBUF_SAVE_PATH,
				 name,
				 NULL);

	gdk_pixbuf_save (pixbuf, save, "png", &err, NULL);
	
	if (err) {
		g_print ("%s\n", err->message);
		g_free (filename);
		g_free (save);
		gtk_widget_destroy (dialog);
		return;
	}
	
	/* Now we have saved the new pixbuf, lets actually set it as the main
	   pixbuf and refresh the view */
	old_icon = priv->icon;
	old_reflect = priv->reflect;

	priv->icon = pixbuf;
        priv->reflect = gdk_pixbuf_flip (priv->icon, FALSE);	
        priv->icon_width = gdk_pixbuf_get_width(priv->icon);
	priv->icon_height = gdk_pixbuf_get_height(priv->icon);

	g_object_unref (G_OBJECT (old_icon));
	g_object_unref (G_OBJECT (old_reflect));

	gtk_widget_queue_draw(GTK_WIDGET(task));	

	g_free (filename);
	g_free (save);
	g_free (name);
	gtk_widget_hide (dialog);
	gtk_widget_destroy (dialog);
}

static void
_task_menu_item_clicked (GtkMenuItem *item, AwnTask *task)
{
	AwnTaskPrivate *priv;
	AwnTaskMenuItem *menu_item;
	int i;

	priv = AWN_TASK_GET_PRIVATE (task);

	for (i = 0; i < 5; i++) {
		menu_item = priv->menu_items[i];
		if (menu_item != NULL) {
			if ( GTK_MENU_ITEM (menu_item->item) == item) {
				g_signal_emit (G_OBJECT (task), awn_task_signals[MENU_ITEM_CLICKED], 0,
					       (guint) menu_item->id);
			}
		}
	}

}


static void
_task_check_item_clicked (GtkMenuItem *item, AwnTask *task)
{
	AwnTaskPrivate *priv;
	AwnTaskMenuItem *menu_item;
	gboolean active;
	int i;

	priv = AWN_TASK_GET_PRIVATE (task);

	active = gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (item));

	for (i = 0; i < 5; i++) {
		menu_item = priv->menu_items[i];
		if (menu_item != NULL) {
			if ( GTK_MENU_ITEM (menu_item->item) == item) {
				g_signal_emit (G_OBJECT (task), awn_task_signals[CHECK_ITEM_CLICKED], 0,
					       (guint) menu_item->id, active );
				menu_item->active  = active;
			}
		}
	}

}

static void
on_change_icon_clicked (GtkButton *button, AwnTask *task)
{
	_task_choose_custom_icon (task);
}

static void
_task_show_prefs (GtkMenuItem *item, AwnTask *task)
{
	_task_choose_custom_icon (task);
  return;

  AwnTaskPrivate *priv;
	priv = AWN_TASK_GET_PRIVATE (task);

	GtkWidget *dialog;
	GtkWidget *hbox;
	GtkWidget *image;
	GtkWidget *button, *label;

	dialog = gtk_dialog_new_with_buttons ("Preferences",
                                                  GTK_WINDOW (priv->settings->window),
                                                  GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                                  "Change Icon...",
                                                  3,
                                                  GTK_STOCK_CANCEL,
                                                  GTK_RESPONSE_REJECT,
                                                  GTK_STOCK_OK,
                                                  GTK_RESPONSE_ACCEPT,
                                                  NULL);
	hbox = gtk_hbox_new (FALSE, 10);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG(dialog)->vbox), hbox, TRUE, TRUE, 0);

	image = gtk_image_new_from_pixbuf (priv->icon);

	button = gtk_button_new_with_label ("Change Image...");
	gtk_button_set_image (GTK_BUTTON (button), image);
	gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, TRUE, 0);
	
	g_signal_connect (G_OBJECT (button), "clicked",
			  G_CALLBACK (on_change_icon_clicked), task);

	label = gtk_label_new (" ");
	char *markup = g_strdup_printf ("<span size='larger' weight='bold'>%s</span>", awn_task_get_application (task));
	gtk_label_set_markup (GTK_LABEL (label), markup);
	g_free (markup);
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);

	gtk_widget_show_all (hbox);

	gint res = gtk_dialog_run (GTK_DIALOG (dialog));

	switch (res) {
		case 3:
			_task_choose_custom_icon (task);
			break;
		default:
			break;
	}
	gtk_widget_destroy (dialog);
}



typedef struct {
	const char *uri;
	AwnSettings *settings;

} AwnListTerm;

static void
_slist_foreach (char *uri, AwnListTerm *term)
{
	AwnSettings *settings = term->settings;

	if ( strcmp (uri, term->uri) == 0 ) {
		//g_print ("URI : %s\n", uri);
		settings->launchers = g_slist_remove(settings->launchers, uri);
		//g_slist_foreach(settings->launchers, (GFunc)_slist_print, (gpointer)term);
	}
}

static void
_task_remove_launcher (GtkMenuItem *item, AwnTask *task)
{
	AwnTaskPrivate *priv;
	AwnSettings *settings;
	AwnListTerm term;
	GString *uri;

	priv = AWN_TASK_GET_PRIVATE (task);
	settings = priv->settings;

	priv->is_closing = TRUE;
	priv->hover = FALSE;
	uri = g_string_new (gnome_desktop_item_get_location (priv->item));
	uri = g_string_erase(uri, 0, 7);

	g_print ("Remove : %s\n", uri->str);
	term.uri = uri->str;
	term.settings = settings;
	g_slist_foreach(settings->launchers, (GFunc)_slist_foreach, (gpointer)&term);

	GConfClient *client = gconf_client_get_default();
		gconf_client_set_list(client,
					"/apps/avant-window-navigator/window_manager/launchers",
					GCONF_VALUE_STRING,settings->launchers,NULL);

	priv->window = NULL;
	priv->needs_attention = FALSE;
	/* start closing effect */
	priv->is_closing = TRUE;
	g_timeout_add(AWN_FRAME_RATE, (GSourceFunc)_task_destroy, (gpointer)task);
}

static void
_shell_done (GtkMenuShell *shell, AwnTask *task)
{
	AwnTaskPrivate *priv;
	priv = AWN_TASK_GET_PRIVATE (task);
	if (priv->settings->auto_hide == FALSE) {
		if (priv->settings->hidden == TRUE) {
			awn_show (priv->settings);
		}
		return;
	}
	awn_hide (priv->settings);
}

static void
awn_task_create_menu(AwnTask *task, GtkMenu *menu)
{
	AwnTaskPrivate *priv;
	GtkWidget *item;

	priv = AWN_TASK_GET_PRIVATE (task);
	item = NULL;

	g_signal_connect (GTK_MENU_SHELL (menu), "selection-done",
			  G_CALLBACK(_shell_done), (gpointer)task);


	if (priv->is_launcher && priv->window == NULL) {


		item = gtk_image_menu_item_new_from_stock ("gtk-remove",
							   NULL);
		gtk_menu_shell_prepend (GTK_MENU_SHELL (menu), item);
		gtk_widget_show(item);
		g_signal_connect (G_OBJECT(item), "activate",
				  G_CALLBACK(_task_remove_launcher), (gpointer)task);
	}

	item = gtk_separator_menu_item_new ();
	gtk_menu_shell_prepend (GTK_MENU_SHELL (menu), item);
	gtk_widget_show(item);

	item = gtk_menu_item_new_with_label (_("Change Icon"));
	gtk_menu_shell_prepend (GTK_MENU_SHELL (menu), item);
	gtk_widget_show(item);
	g_signal_connect (G_OBJECT(item), "activate",
			  G_CALLBACK(_task_show_prefs), (gpointer)task);

	AwnTaskMenuItem *menu_item;
	int i = 0;
	for (i = 0; i < 5; i++) {
		menu_item = priv->menu_items[i];
		if ( menu_item != NULL) {

			if (i == 0) {
				item = gtk_separator_menu_item_new ();
				gtk_menu_shell_prepend (GTK_MENU_SHELL (menu), item);
				gtk_widget_show(item);
			}


			g_signal_handlers_disconnect_by_func(G_OBJECT (menu_item->item),
								       _task_menu_item_clicked, (gpointer)task);
			g_signal_handlers_disconnect_by_func(G_OBJECT (menu_item->item),
								       _task_check_item_clicked, (gpointer)task);
			gtk_widget_unparent (menu_item->item);
			gtk_menu_shell_prepend (GTK_MENU_SHELL (menu), menu_item->item);

			if (menu_item->type == AWN_TASK_MENU_TYPE_CHECK) {
				gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (menu_item->item),
								menu_item->active);
				g_signal_connect (G_OBJECT(menu_item->item), "activate",
						G_CALLBACK(_task_check_item_clicked), (gpointer)task);
			} else {
				g_signal_connect (G_OBJECT(menu_item->item), "activate",
				G_CALLBACK(_task_menu_item_clicked), (gpointer) task);
			}
			gtk_widget_show(menu_item->item);

		}
	}

}

/********************* awn_task_new * *******************/

GtkWidget *
awn_task_new (AwnTaskManager *task_manager, AwnSettings *settings)
{
	GtkWidget *task;
	AwnTaskPrivate *priv;
	task = g_object_new (AWN_TYPE_TASK, NULL);
	priv = AWN_TASK_GET_PRIVATE (task);

	priv->task_manager = task_manager;
	priv->settings = settings;

	/* This is code which I will add later for better hover effects over
	the bar */
	priv->win_enter = g_signal_connect(G_OBJECT(settings->window), "motion-notify-event",
			 G_CALLBACK(awn_task_win_enter_in), AWN_TASK(task));

	priv->win_leave = g_signal_connect(G_OBJECT(settings->window), "leave-notify-event",
			 G_CALLBACK(awn_task_win_enter_out), AWN_TASK(task));

	g_signal_connect (G_OBJECT(task), "drag-data-received",
		  G_CALLBACK(_task_drag_data_recieved), (gpointer)task);
	g_signal_connect (G_OBJECT(task), "drag-end",
		  G_CALLBACK(_task_drag_data_recieved), (gpointer)task);
	return task;
}

void
awn_task_close (AwnTask *task)
{
	AwnTaskPrivate *priv;
	priv = AWN_TASK_GET_PRIVATE (task);

	g_signal_handler_disconnect ((gpointer)priv->window, priv->icon_changed);
	g_signal_handler_disconnect ((gpointer)priv->window, priv->state_changed);
	g_signal_handler_disconnect ((gpointer)priv->settings->window, priv->win_enter);
	g_signal_handler_disconnect ((gpointer)priv->settings->window, priv->win_leave);

	priv->window = NULL;
	priv->needs_attention = FALSE;

	AwnTaskMenuItem *item;
	int i;
	for (i = 0; i < 5; i++) {
		item = priv->menu_items[i];
		if (item != NULL) {
			gtk_widget_destroy (item->item);
			g_free (item);
			item = priv->menu_items[i] = NULL;
		}

	}


	if (priv->is_launcher) {
		gtk_widget_queue_draw(GTK_WIDGET(task));
		return;
	}
	/* start closing effect */
	priv->is_closing = TRUE;
	g_timeout_add(AWN_FRAME_RATE, (GSourceFunc)_task_destroy, (gpointer)task);
}






