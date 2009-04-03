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
 *
 *  Notes : This manages the icons on the bar. Opens, closes, hides, and shows
 *          the icons according to the preferences.
*/

#define WNCK_I_KNOW_THIS_IS_UNSTABLE 1
#include <libwnck/libwnck.h>

#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-bindings.h>
#include <unistd.h>

#include "config.h"

#include "awn-task-manager.h"

#include "awn-task.h"
#include "awn-bar.h"
#include "awn-window.h"

#include "awn-marshallers.h"

#include <libawn/awn-config-client.h>
#include <libawn/awn-desktop-item.h>
#include <libawn/awn-title.h>

#define AWN_TASK_MANAGER_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), AWN_TYPE_TASK_MANAGER, AwnTaskManagerPrivate))

G_DEFINE_TYPE (AwnTaskManager, awn_task_manager, GTK_TYPE_HBOX);

/* FORWARD DECLERATIONS */
static void _task_manager_window_opened (WnckScreen *screen, WnckWindow *window,
						AwnTaskManager *task_manager);
static void _task_manager_window_closed (WnckScreen *screen, WnckWindow *window,
						 AwnTaskManager *task_manager);
#ifdef HAVE_LIBWNCK_220
static void _task_manager_window_activate (WnckScreen *screen,
			WnckWindow *window, AwnTaskManager *task_manager);
#else
static void _task_manager_window_activate (WnckScreen *screen,
						AwnTaskManager *task_manager);
#endif
static void _task_manager_viewports_changed (WnckScreen *screen,
						AwnTaskManager *task_manager);
static void _task_manager_drag_data_recieved (GtkWidget        *widget,
                                              GdkDragContext   *drag_context,
                                              gint              x,
                                              gint              y,
                                              GtkSelectionData *data,
                                              guint             info,
                                              guint             time,
                                              AwnTaskManager    *task_manager);


static void _refresh_box(AwnTaskManager *task_manager);
static void _task_manager_load_launchers(AwnTaskManager *task_manager);
void awn_task_manager_update_separator_position (AwnTaskManager *task_manager);
static void _task_manager_menu_item_clicked (AwnTask *task, guint id, AwnTaskManager *task_manager);
static void _task_manager_check_item_clicked (AwnTask *task, guint id, gboolean active, AwnTaskManager *task_manager);
static void _task_manager_check_width (AwnTaskManager *task_manager);

static void
on_height_changed (DBusGProxy *proxy, gint height, AwnTaskManager *manager);


/* STRUCTS & ENUMS */

typedef struct _AwnTaskManagerPrivate AwnTaskManagerPrivate;

struct _AwnTaskManagerPrivate
{
	AwnSettings *settings;

	WnckScreen *screen;

	GtkWidget *title_window;

	GtkWidget *launcher_box;
	GtkWidget *tasks_box;

	GList *launchers;
	GList *tasks;

	GtkWidget *eb;

	DBusGConnection *applet_man_connection;
	DBusGProxy *applet_man_proxy;

	gulong signal_handlers[6];

	gboolean got_viewport;
	gint activate_behavior;
};

enum
{
	MENU_ITEM_CLICKED,
	CHECK_ITEM_CLICKED,
	LAST_SIGNAL
};

static guint awn_task_manager_signals[LAST_SIGNAL] = { 0 };

/* GLOBALS */

static int TASKMAN_OWN_PID = -1;

static void
_load_launchers_func (const char *uri, AwnTaskManager *task_manager)
{
	AwnTaskManagerPrivate *priv;
	GtkWidget *task = NULL;
	AwnDesktopItem *item = NULL;

	priv = AWN_TASK_MANAGER_GET_PRIVATE (task_manager);

	item = awn_desktop_item_new ((gchar *)uri);
	if (item == NULL)
		return;

	task = awn_task_new(task_manager, priv->settings);
	awn_task_set_title (AWN_TASK (task), AWN_TITLE(priv->title_window));
	if (awn_task_set_launcher (AWN_TASK (task), item)) {

		g_signal_connect (G_OBJECT(task), "drag-data-received",
				  G_CALLBACK(_task_manager_drag_data_recieved), (gpointer)task_manager);

		g_signal_connect (G_OBJECT(task), "menu_item_clicked",
			  G_CALLBACK(_task_manager_menu_item_clicked), (gpointer)
			  task_manager);
		g_signal_connect (G_OBJECT(task), "check_item_clicked",
			  G_CALLBACK(_task_manager_check_item_clicked), (gpointer)
			  task_manager);

		priv->launchers = g_list_append(priv->launchers, (gpointer)task);
		gtk_box_pack_start(GTK_BOX(priv->launcher_box), task, FALSE, FALSE, 0);

		_refresh_box (task_manager);
		awn_task_refresh_icon_geometry(AWN_TASK(task));
		g_print("LOADED : %s\n", uri);
	} else {
		gtk_widget_destroy(task);
		awn_desktop_item_free (item);
		g_print("FAILED : %s\n", uri);
	}
}

static void
_task_manager_load_launchers(AwnTaskManager *task_manager)
{
	AwnTaskManagerPrivate *priv;
	priv = AWN_TASK_MANAGER_GET_PRIVATE (task_manager);

	g_slist_foreach (priv->settings->launchers, (GFunc)_load_launchers_func,
								 task_manager); 
}

typedef struct {
	AwnTask *task;
	int pid;
	WnckWindow *window;

} AwnLauncherTerm;

static void
_normalize (char *str)
{
	size_t len;
	int i;

	len = strlen(str);

	for (i = 0; i < len; i++) {
		switch (str[i]) {

			case '-':
			case ' ':
			case '_':
			case '.':
				str[i] = '.';
				break;
			default:
				break;
		}
	}

}

static void
_find_launcher (AwnTask *task, AwnLauncherTerm *term)
{
	g_return_if_fail (AWN_IS_TASK(task));

	int pid = awn_task_get_pid(task);

	/* try pid */
	if (term->pid == pid) {
		term->task = task;
	}

	/* try app name, works for 80% of sane applications */
	if (term->task == NULL) {
		WnckApplication *wnck_app;
		char *app_name;
		char *exec;
		GString *str;

		wnck_app = wnck_window_get_application(term->window);
		if (WNCK_IS_APPLICATION (wnck_app)) {
			app_name = (char*)wnck_application_get_name(wnck_app);
		} else {
			app_name = NULL;
		}
		str = g_string_new (app_name);
		str = g_string_ascii_down (str);
		_normalize (str->str);
		exec = g_strdup (awn_task_get_application(task));
		_normalize (exec);

		if (exec && str && strcmp (exec, str->str) == 0) {
			term->task = task;
		}

		g_string_free (str, TRUE);
		g_free(exec);


	}
	/* try window name */
	if (term->task == NULL) {
		GString *str1 = g_string_new (awn_task_get_name (task));
		str1 = g_string_ascii_down (str1);


		GString *str2 = g_string_new (wnck_window_get_name (term->window));
		str2 = g_string_ascii_down (str2);
		if ( str2->str[str2->len-1] == ' ')
			 str2 = g_string_truncate (str2, str2->len -1);

		if ( strcmp (str1->str, str2->str) == 0 ) {
			term->task = task;

		}


		g_string_free (str1, TRUE);
		g_string_free (str2, TRUE);
	}
	/* try and see if the program name contains the launcher name */
	if (term->task == NULL) {
		WnckApplication *wnck_app;
		wnck_app = wnck_window_get_application(term->window);
		
		GString *str1 = g_string_new (awn_task_get_name (task));
		str1 = g_string_ascii_down (str1);

		GString *str2;
		if (WNCK_IS_APPLICATION (wnck_app)) {
			str2 = g_string_new (wnck_application_get_name (wnck_app));
		} else {
			str2 = g_string_new (NULL);
		}
		str2 = g_string_ascii_down (str2);
		
		gchar *res = NULL;
		res = g_strstr_len (str2->str, str2->len, str1->str);

		if (res) {
			term->task = task;

		}


		g_string_free (str1, TRUE);
		g_string_free (str2, TRUE);
	}
	
	/* window title in launcher name. Listen, I'm looking at you  */
	if (term->task == NULL) {
		GString *str1 = g_string_new (awn_task_get_name (task));
		str1 = g_string_ascii_down (str1);

		GString *str2 = g_string_new (wnck_window_get_name (term->window));
		str2 = g_string_ascii_down (str2);
		
		gchar *res = NULL;
		res = g_strstr_len (str1->str, str1->len, str2->str);

		if (res) {
			term->task = task;

		}


		g_string_free (str1, TRUE);
		g_string_free (str2, TRUE);
	}		

}

GtkWidget *
_task_manager_window_has_launcher (AwnTaskManager *task_manager,
							   WnckWindow *window)
{
	AwnTaskManagerPrivate *priv;
	GtkWidget *task = NULL;
	AwnLauncherTerm term;

	//g_return_if_fail(AWN_IS_TASK(task));
	if (!WNCK_IS_WINDOW (window)) return NULL;

	priv = AWN_TASK_MANAGER_GET_PRIVATE (task_manager);

	term.task = NULL;
	term.pid = wnck_window_get_pid(window);
	term.window = window;

	if (term.pid == 0) {
		WnckApplication *app = wnck_window_get_application(window);
		if (app) {
			term.pid = wnck_application_get_pid(app);
		}
	}
	//g_print("New window Pid = %d\n", term.pid);

	g_list_foreach(priv->launchers, (GFunc)_find_launcher, (gpointer)&term);

	task = GTK_WIDGET(term.task);

	if (task != NULL)
		return GTK_WIDGET(term.task);
	return NULL;
}

GtkWidget *
_task_manager_window_has_starter (WnckWindow *window)
{
	/*
	Go through each task
		if task is starter for window
		return starter;
	*/
	return NULL;
}

static void
_task_manager_window_opened (WnckScreen *screen, WnckWindow *window,
						AwnTaskManager *task_manager)
{
	AwnTaskManagerPrivate *priv;
	GtkWidget *task = NULL;

	priv = AWN_TASK_MANAGER_GET_PRIVATE (task_manager);


	/* if is skipping task bar, its not worthy of a launcher ;) */
	if (!wnck_window_is_skip_tasklist(window)) {
		/* first check if it has a launcher */
		task = _task_manager_window_has_launcher(task_manager, window);
		if (task != NULL) {
			//g_print("\n\n\nFound launcher for %s\n\n\n", wnck_window_get_name(window));
			if (awn_task_set_window (AWN_TASK (task), window))
				awn_task_refresh_icon_geometry(AWN_TASK(task));
			else
				task = NULL;
		}

		/* check startup notification */
		if (task == NULL) {
			task = _task_manager_window_has_starter(window);
			if (task)
				awn_task_set_window (AWN_TASK (task), window);
		}
	}
	/* if not launcher & no starter, create new task */
	if (task == NULL) {
		// LP bug #258960 - always skip awn itself
		if (wnck_window_get_pid(window) == TASKMAN_OWN_PID) return;

		task = awn_task_new(task_manager, priv->settings);
		if (awn_task_set_window (AWN_TASK (task), window))
			;//g_print("Created for %s\n", wnck_window_get_name(window));
		awn_task_set_title (AWN_TASK(task), AWN_TITLE(priv->title_window));
		priv->tasks = g_list_append(priv->tasks, (gpointer)task);
		gtk_box_pack_start(GTK_BOX(priv->tasks_box), task, FALSE, FALSE, 0);

		g_signal_connect (G_OBJECT(task), "drag-data-received",
				  G_CALLBACK(_task_manager_drag_data_recieved), (gpointer)task_manager);
		g_signal_connect (G_OBJECT(task), "menu_item_clicked",
			  G_CALLBACK(_task_manager_menu_item_clicked), (gpointer)
			  task_manager);
		g_signal_connect (G_OBJECT(task), "check_item_clicked",
			  G_CALLBACK(_task_manager_check_item_clicked), (gpointer)
			  task_manager);
		_refresh_box (task_manager);
		awn_task_manager_update_separator_position (task_manager);
	}

	
}

static void
_win_reparent (AwnTask *task, AwnTaskManager *task_manager)
{
	AwnTaskManagerPrivate *priv;
	AwnTask *new_task = NULL;
	WnckWindow *window = NULL;

	priv = AWN_TASK_MANAGER_GET_PRIVATE (task_manager);

	g_return_if_fail (AWN_IS_TASK(task));
	window = awn_task_get_window(task);
	new_task = AWN_TASK (_task_manager_window_has_launcher(task_manager, window));

	if (new_task) {
		if (awn_task_set_window (AWN_TASK (new_task), window)) {
			awn_task_close(task);
			awn_task_manager_remove_task (task_manager, task);
			awn_task_refresh_icon_geometry(new_task);

		} else
			new_task = NULL;

	}


}

static void
_reparent_windows (AwnTaskManager *task_manager)
{
	/* go through all windows look if they have a launcher, and reparent
	the window */
	AwnTaskManagerPrivate *priv;

	priv = AWN_TASK_MANAGER_GET_PRIVATE (task_manager);

	g_list_foreach(priv->tasks, (GFunc)_win_reparent, (gpointer)task_manager);


}

typedef struct {
	gulong xid;
	GList *list;
	int was_launcher;
} AwnDestroyTerm;

static void
_task_destroy (AwnTask *task, AwnDestroyTerm *term)
{
	guint task_id;

	g_return_if_fail(AWN_IS_TASK(task));

	//window_id = GPOINTER_TO_UINT(xid);
	task_id = awn_task_get_xid(task);

	if (term->xid == task_id) {
		if (awn_task_is_launcher (task))
			term->was_launcher = 1;
		awn_task_close(task);
	}
}

static void
_task_manager_window_closed (WnckScreen *screen, WnckWindow *window,
						 AwnTaskManager *task_manager)
{
	g_return_if_fail (WNCK_IS_WINDOW (window));

	AwnTaskManagerPrivate *priv;
	AwnDestroyTerm term;
	guint xid;

	priv = AWN_TASK_MANAGER_GET_PRIVATE (task_manager);

	xid = wnck_window_get_xid (window);
	term.xid = xid;
	term.was_launcher = 0;
	term.list = priv->launchers;
	g_list_foreach (priv->launchers, (GFunc)_task_destroy, (gpointer)&term);
	term.list = priv->tasks;
	g_list_foreach (priv->tasks, (GFunc)_task_destroy, (gpointer)&term);

	_reparent_windows(task_manager);
	_refresh_box(task_manager);
}

#ifdef HAVE_LIBWNCK_220
static void
_task_manager_window_activate (WnckScreen *screen, WnckWindow *prevWindow,
						AwnTaskManager *task_manager)
{
	_refresh_box(task_manager);
}
#else
static void
_task_manager_window_activate (WnckScreen *screen,
						AwnTaskManager *task_manager)
{
	_refresh_box(task_manager);
}
#endif
static void
_task_manager_viewports_changed (WnckScreen *screen,
						AwnTaskManager *task_manager)
{
	awn_window_force_repos();
	_refresh_box(task_manager);
}

static void
_task_manager_menu_item_clicked (AwnTask *task, guint id, AwnTaskManager *task_manager)
{
	g_signal_emit (G_OBJECT (task_manager), awn_task_manager_signals[MENU_ITEM_CLICKED], 0,
	               (guint) id);
}

static void
_task_manager_check_item_clicked (AwnTask *task, guint id, gboolean active, AwnTaskManager *task_manager)
{
	g_signal_emit (G_OBJECT (task_manager), awn_task_manager_signals[CHECK_ITEM_CLICKED], 0,
	               (guint) id, (gboolean) active);
}


/********************************* D&D code *****************************/

static void
_task_manager_drag_data_recieved (GtkWidget *widget, GdkDragContext *context,
				  gint x, gint y, GtkSelectionData *selection_data,
				  guint target_type, guint time,
                                              AwnTaskManager *task_manager)
{
        gchar   *_sdata = NULL;

        gboolean dnd_success = FALSE;
        gboolean delete_selection_data = FALSE;

        const gchar *name = gtk_widget_get_name (widget);
        g_print ("%s: drag_data_received_handl\n", name);


        /* Deal with what we are given from source */
        if((selection_data != NULL) && (selection_data-> length >= 0))
        {
                /* Check that we got the format we can use */
               	g_print (" Receiving ");
                _sdata = (gchar*)selection_data-> data;
                g_print ("string: %s", _sdata);
                dnd_success = TRUE;

        }

	AwnTaskManagerPrivate *priv;
	GString *uri;
	AwnSettings *settings;

	priv = AWN_TASK_MANAGER_GET_PRIVATE (task_manager);

	uri = g_string_new(_sdata);
	uri = g_string_erase(uri, 0, 7);

	int i = 0;
	int res = 0;
	for (i =0; i < uri->len; i++) {
		if (uri->str[i] == 'p')
		res = i;
	}
	if (res)
		uri = g_string_truncate(uri, res+1);


	g_print("Desktop file: %s\n", uri->str);

	/******* Add to config *********/
	settings = priv->settings;
	// careful shallow copy
	GSList *launchers = g_slist_copy(settings->launchers);
	launchers = g_slist_append(launchers, uri->str);

	AwnConfigClient *client = awn_config_client_new ();
	awn_config_client_set_list(client, "window_manager", "launchers",
                                   AWN_CONFIG_CLIENT_LIST_TYPE_STRING,
                                   launchers, NULL);

	g_slist_free (launchers);

	g_string_free(uri, TRUE);
       	gtk_drag_finish (context, dnd_success, delete_selection_data, time);
}

/*****************  END OF D&D *************************************/

static int num_tasks = 0;
static int num_launchers = 0;

static void
_task_refresh (AwnTask *task, AwnTaskManager *task_manager)
{
	WnckWindow *window;
	AwnSettings *settings;
	AwnTaskManagerPrivate *priv;
	WnckWorkspace *space;

	priv = AWN_TASK_MANAGER_GET_PRIVATE (task_manager);

	space = wnck_screen_get_active_workspace(priv->screen);

	g_return_if_fail(AWN_IS_TASK (task));

	if (task == NULL)
		return;

	settings = awn_task_get_settings(task);
	window = awn_task_get_window(task);


	if (!space)
		return;

	if (awn_task_is_launcher (task)) {
		gtk_widget_show (GTK_WIDGET (task));
		awn_task_refresh_icon_geometry(task);
		gtk_widget_queue_draw(GTK_WIDGET(task));
		num_launchers++;
		awn_task_manager_update_separator_position (task_manager);
		return;
	}
	
	if (!window)
		return;
	if ( wnck_window_is_skip_tasklist (window)) {
		gtk_widget_hide (GTK_WIDGET (task));
		awn_task_manager_update_separator_position (task_manager);
		return;
	}

	if (settings->show_all_windows) {
		gtk_widget_show (GTK_WIDGET (task));
		awn_task_refresh_icon_geometry(task);
		gtk_widget_queue_draw(GTK_WIDGET(task));
		num_tasks++;
		awn_task_manager_update_separator_position (task_manager);
		return;
	}

	if (wnck_window_is_in_viewport (window, space)) {
		gtk_widget_show_all (GTK_WIDGET (task));
		awn_task_refresh_icon_geometry(task);
		gtk_widget_queue_draw(GTK_WIDGET(task));
		num_tasks++;
	} else
		gtk_widget_hide (GTK_WIDGET (task));

	awn_task_manager_update_separator_position (task_manager);
}

static void
_refresh_box(AwnTaskManager *task_manager)
{
	if(!AWN_IS_TASK_MANAGER(task_manager))
		return;	

	AwnTaskManagerPrivate *priv;

	WnckWorkspace *space;

	if (!AWN_IS_TASK_MANAGER (task_manager))
		return;
	priv = AWN_TASK_MANAGER_GET_PRIVATE (task_manager);

	space = wnck_screen_get_active_workspace(priv->screen);
	
	num_launchers = 0;
	g_list_foreach(priv->launchers, (GFunc)_task_refresh, (gpointer)task_manager);

	num_tasks = 0;
	g_list_foreach(priv->tasks, (GFunc)_task_refresh, (gpointer)task_manager);
	if (num_tasks == 0) {
		gtk_widget_hide (priv->eb);
	} else {
		gtk_widget_show (priv->eb);
	}
	//awn_task_manager_update_separator_position (task_manager);
	gtk_widget_queue_draw (GTK_WIDGET (priv->settings->window));
	
	_task_manager_check_width (task_manager);
}

gboolean
awn_task_manager_refresh_box (AwnTaskManager *task_manager)
{
	_refresh_box(task_manager);
	return FALSE;
}

void
awn_task_manager_remove_launcher (AwnTaskManager *task_manager, gpointer  task)
{
	AwnTaskManagerPrivate *priv;
	priv = AWN_TASK_MANAGER_GET_PRIVATE (task_manager);

	priv->launchers = g_list_remove(priv->launchers, task);
	awn_task_manager_update_separator_position (task_manager);
	gtk_widget_queue_draw (GTK_WIDGET (priv->settings->window));
}

void
awn_task_manager_remove_task (AwnTaskManager *task_manager, gpointer task)
{
	AwnTaskManagerPrivate *priv;
	priv = AWN_TASK_MANAGER_GET_PRIVATE (task_manager);

	priv->tasks = g_list_remove(priv->tasks, (gpointer)task);
	awn_task_manager_update_separator_position (task_manager);
	gtk_widget_queue_draw (GTK_WIDGET (priv->settings->window));
}

void
awn_task_manager_update_separator_position (AwnTaskManager *task_manager)
{
	AwnTaskManagerPrivate *priv;
	AwnSettings *settings;
	static int x = 0;
	int new_x = 0;

	priv = AWN_TASK_MANAGER_GET_PRIVATE (task_manager);
	settings = priv->settings;

	if (num_tasks == 0) {
		awn_bar_set_draw_separator (settings->bar, 0);
		return;
	}
	new_x = GTK_WIDGET(priv->eb)->allocation.x;

	awn_bar_set_draw_separator (settings->bar, 1);
	awn_bar_set_separator_position (settings->bar, x+2);
	x = new_x;

}


/*** Resizing code ***/

static void
_task_resize (AwnTask *task, gpointer width)
{
	awn_task_set_width (task, GPOINTER_TO_INT (width));
}

static void
_task_manager_check_width (AwnTaskManager *task_manager)
{
	AwnTaskManagerPrivate *priv;
	AwnSettings *settings;
	gint w, h;
	gint width;
	gint x;
	gint num;
		
	priv = AWN_TASK_MANAGER_GET_PRIVATE (task_manager);
	settings = priv->settings;

	gtk_window_get_size (GTK_WINDOW (settings->window), &w, &h);
	
	width = settings->task_width;
	num = num_tasks + num_launchers;
	
	if (num == 0) {
		awn_task_manager_update_separator_position (task_manager);
		return;
	}

	if (w + 20 > settings->monitor_width) {
		x = w - num * width;
		
		do {
			--width;
		} while (x + num * width + 40 > settings->monitor_width);
	} else if (width != settings->bar_height + 12 && w + 60 < settings->monitor_width) {
		x = w - num * width;

		do {
			++width;
		} while (x + num * width + 50 < settings->monitor_width);
	}

	if (width < settings->task_width && width >= AWN_MIN_BAR_HEIGHT) {
		settings->task_width = width;
		g_list_foreach(priv->launchers, (GFunc)_task_resize, GINT_TO_POINTER (settings->task_width));
		g_list_foreach(priv->tasks, (GFunc)_task_resize, GINT_TO_POINTER (settings->task_width));
	} else if (width > settings->task_width) {
		if (width > settings->bar_height + 12) {
			settings->task_width = settings->bar_height + 12;
		} else {
			settings->task_width = width;
		}
		g_list_foreach(priv->launchers, (GFunc)_task_resize, GINT_TO_POINTER (settings->task_width));
		g_list_foreach(priv->tasks, (GFunc)_task_resize, GINT_TO_POINTER (settings->task_width));
	}

	awn_task_manager_update_separator_position (task_manager);
}

static void
_task_update_icon (AwnTask *task, AwnTaskManager *task_manager)
{
	awn_task_update_icon (task);
}

static void
_task_manager_icon_theme_changed (GtkIconTheme *icon_theme, AwnTaskManager *task_manager)
{
	AwnTaskManagerPrivate *priv;
	priv = AWN_TASK_MANAGER_GET_PRIVATE (task_manager);

	g_list_foreach(priv->launchers, (GFunc)_task_update_icon, (gpointer)task_manager);
}

/********************* DBUS *********************************/

typedef struct {
	AwnTaskManager *task_manager;
	gchar *name;
	glong xid;
	gint pid;
	AwnTask *task;
} AwnDBusTerm;

static void
_dbus_find_task (AwnTask *task, AwnDBusTerm *term)
{
	gchar *temp = NULL;

	if (term->name) {
		temp = (gchar *)awn_task_get_application (task);
		if (temp && strcmp (term->name, temp) == 0) {
			term->task = task;
			return;
		}
		temp = (gchar *)awn_task_get_name (task);
		if (temp && strcmp (term->name, temp) == 0) {
			term->task = task;
			return;
		}
	} else if (term->xid) {
		glong id;
		id = awn_task_get_xid (task);
		if (term->xid == id) {
			term->task = task;
			return;
		}
	} else if (term->pid) {
		gint id;
		id = awn_task_get_pid (task);
		if (term->pid == id) {
			term->task = task;
			return;
		}
	} else {
		return;

	}
}

static void
__find_by_name (AwnTaskManagerPrivate *priv, AwnDBusTerm *term, const gchar *name)
{
	term->name = (char *)name;
	term->xid = 0;
	term->pid = 0;
	term->task = NULL;

	g_list_foreach(priv->launchers, (GFunc)_dbus_find_task, (gpointer)term);
	if (term->task == NULL) {
		g_list_foreach(priv->tasks, (GFunc)_dbus_find_task, (gpointer)term);
	}
}

static void
__find_by_xid (AwnTaskManagerPrivate *priv, AwnDBusTerm *term, glong xid)
{
	term->name = NULL;
	term->xid = xid;
	term->pid = 0;
	term->task = NULL;

	g_list_foreach(priv->launchers, (GFunc)_dbus_find_task, (gpointer)term);
	if (term->task == NULL) {
		g_list_foreach(priv->tasks, (GFunc)_dbus_find_task, (gpointer)term);
	}
}

static void
__find_by_pid (AwnTaskManagerPrivate *priv, AwnDBusTerm *term, gint pid)
{
	term->name = NULL;
	term->xid = 0;
	term->pid = pid;
	term->task = NULL;

	g_list_foreach(priv->launchers, (GFunc)_dbus_find_task, (gpointer)term);
	if (term->task == NULL) {
		g_list_foreach(priv->tasks, (GFunc)_dbus_find_task, (gpointer)term);
	}
}

static gboolean
awn_task_manager_get_task_by_pid (AwnTaskManager	*task_manager,
				  gint	 		pid,
				  gchar			**name,
				   GError		**error)
{
	AwnTaskManagerPrivate *priv;
	AwnDBusTerm term;

	priv = AWN_TASK_MANAGER_GET_PRIVATE (task_manager);

	__find_by_pid (priv, &term, pid);
	g_print ("%d\n", pid);
	if (term.task == NULL) {
		//*task_id = 10;
		g_print ("No match\n");
		return TRUE;
	} else {
		*name = g_strdup (awn_task_get_application (term.task));
		return TRUE;
	}
}

static gboolean
awn_task_manager_set_task_icon (AwnTaskManager *task_manager,
			    	gchar		*name,
			    	gchar 	  	*icon_path,
			        GError   	**error)
{
	AwnTaskManagerPrivate *priv;
	AwnDBusTerm term;
	GdkPixbuf *icon;

	priv = AWN_TASK_MANAGER_GET_PRIVATE (task_manager);

	__find_by_name (priv, &term, name);

	if (term.task == NULL) {
		g_print (" task not found\n");
		return TRUE;
	}

	g_print ("icon_path = %s\n", icon_path);
	/* Try and load icon from path */
	if (icon_path == NULL) {
		awn_task_unset_custom_icon (term.task);
		return TRUE;
	}
	icon = gdk_pixbuf_new_from_file_at_scale (icon_path,
                                                  priv->settings->task_width-12,
                                                  priv->settings->task_width-12,
                                                  TRUE,
                                                  NULL);
	if (icon)
		awn_task_set_custom_icon (term.task, icon);
	else
		g_print("%s Not Found\n", name);
	//g_free (icon_path);
	return TRUE;
}

static gboolean
awn_task_manager_set_task_progress (AwnTaskManager *task_manager,
			    	    gchar		*name,
			    	    gint		progress,
			    	    GError	   	**error)
{
	AwnTaskManagerPrivate *priv;
	AwnDBusTerm term;

	priv = AWN_TASK_MANAGER_GET_PRIVATE (task_manager);

	__find_by_name (priv, &term, name);
	g_print ("%d\n", progress);
	if (term.task == NULL) {
		g_print (" task not found\n");
		return TRUE;
	}

	awn_task_set_progress (term.task, progress);
	return TRUE;
}

static gboolean
awn_task_manager_set_task_info (AwnTaskManager *task_manager,
			    	gchar		*name,
			    	gchar		*info,
			    	GError   	**error)
{
	AwnTaskManagerPrivate *priv;
	AwnDBusTerm term;

	priv = AWN_TASK_MANAGER_GET_PRIVATE (task_manager);

	__find_by_name (priv, &term, name);

	if (term.task == NULL) {
		g_print (" task not found\n");
		return TRUE;
	}
	if (info == NULL)
		 awn_task_unset_info (term.task);
	else
		awn_task_set_info (term.task, info);
	return TRUE;
}

/* Depreciated calls */
static gboolean
awn_task_manager_set_task_icon_by_name (AwnTaskManager *task_manager,
			    		gchar 		*name,
			    		gchar 	  	*icon_path,
			        	GError   	**error)
{
	AwnTaskManagerPrivate *priv;
	AwnDBusTerm term;
	GdkPixbuf *icon = NULL;

	priv = AWN_TASK_MANAGER_GET_PRIVATE (task_manager);

	__find_by_name (priv, &term, name);

	if (term.task == NULL) {
		g_print (" task not found\n");
		return TRUE;
	}

	/* Try and load icon from path */
	if (icon_path) {
		icon = gdk_pixbuf_new_from_file_at_scale (icon_path,
	                                                  priv->settings->bar_height,
	                                                  priv->settings->bar_height,
	                                                  TRUE,
	                                                  NULL);
	}
	if (icon)
		awn_task_set_custom_icon (term.task, icon);

	//g_free (icon_path);
	return TRUE;
}

static gboolean
awn_task_manager_set_task_icon_by_xid (AwnTaskManager *task_manager,
			    		gint64		xid,
			    		gchar 	  	*icon_path,
			        	GError   	**error)
{
	AwnTaskManagerPrivate *priv;
	AwnDBusTerm term;
	GdkPixbuf *icon = NULL;

	priv = AWN_TASK_MANAGER_GET_PRIVATE (task_manager);

	__find_by_xid (priv, &term, xid);

	if (term.task == NULL) {
		g_print (" task not found\n");
		return TRUE;
	}

	/* Try and load icon from path */
	if (icon_path) {
		icon = gdk_pixbuf_new_from_file_at_scale (icon_path,
	                                                  priv->settings->bar_height,
	                                                  priv->settings->bar_height,
	                                                  TRUE,
	                                                  NULL);
	}
	if (icon)
		awn_task_set_custom_icon (term.task, icon);

	//g_free (icon_path);
	return TRUE;
}

static gboolean
awn_task_manager_unset_task_icon_by_name (AwnTaskManager *task_manager,
			    		gchar 		*name,
			    		GError   	**error)
{
	AwnTaskManagerPrivate *priv;
	AwnDBusTerm term;

	priv = AWN_TASK_MANAGER_GET_PRIVATE (task_manager);

	__find_by_name (priv, &term, name);

	if (term.task == NULL) {
		g_print (" task not found\n");
		return TRUE;
	}
	awn_task_unset_custom_icon (term.task);

	//g_free (icon_path);
	return TRUE;
}

static gboolean
awn_task_manager_unset_task_icon_by_xid (AwnTaskManager *task_manager,
			    		gint64		xid,
			    		GError   	**error)
{
	AwnTaskManagerPrivate *priv;
	AwnDBusTerm term;

	priv = AWN_TASK_MANAGER_GET_PRIVATE (task_manager);

	__find_by_xid (priv, &term, xid);

	if (term.task == NULL) {
		g_print (" task not found\n");
		return TRUE;
	}
	awn_task_unset_custom_icon (term.task);

	//g_free (icon_path);
	return TRUE;
}

static gboolean
awn_task_manager_set_progress_by_name (AwnTaskManager *task_manager,
			    		gchar 		*name,
			    		gint		progress,
			    		GError   	**error)
{
	AwnTaskManagerPrivate *priv;
	AwnDBusTerm term;

	priv = AWN_TASK_MANAGER_GET_PRIVATE (task_manager);

	__find_by_name (priv, &term, name);

	if (term.task == NULL) {
		g_print (" task not found\n");
		return TRUE;
	}

	awn_task_set_progress (term.task, progress);

	return TRUE;
}

static gboolean
awn_task_manager_set_progress_by_xid (AwnTaskManager *task_manager,
			    		gint64		xid,
			    		gint		progress,
			    		GError   	**error)
{
	AwnTaskManagerPrivate *priv;
	AwnDBusTerm term;

	priv = AWN_TASK_MANAGER_GET_PRIVATE (task_manager);

	__find_by_xid (priv, &term, xid);

	if (term.task == NULL) {
		g_print (" task not found\n");
		return TRUE;
	}
	awn_task_set_progress (term.task, progress);
	return TRUE;
}

static gboolean
awn_task_manager_set_info_by_name (AwnTaskManager *task_manager,
			    		gchar 		*name,
			    		gchar		*info,
			    		GError   	**error)
{
	AwnTaskManagerPrivate *priv;
	AwnDBusTerm term;

	priv = AWN_TASK_MANAGER_GET_PRIVATE (task_manager);

	__find_by_name (priv, &term, name);

	if (term.task == NULL) {
		g_print (" task not found\n");
		return TRUE;
	}
	awn_task_set_info (term.task, info);

	return TRUE;
}

static gboolean
awn_task_manager_set_info_by_xid (AwnTaskManager *task_manager,
			    		gint64		xid,
			    		gchar		*info,
			    		GError   	**error)
{
	AwnTaskManagerPrivate *priv;
	AwnDBusTerm term;

	priv = AWN_TASK_MANAGER_GET_PRIVATE (task_manager);

	__find_by_xid (priv, &term, xid);

	if (term.task == NULL) {
		g_print (" task not found\n");
		return TRUE;
	}
	awn_task_set_info (term.task, info);
	return TRUE;
}

static gboolean
awn_task_manager_unset_info_by_name (AwnTaskManager *task_manager,
			    		gchar 		*name,
			    		GError   	**error)
{
	AwnTaskManagerPrivate *priv;
	AwnDBusTerm term;

	priv = AWN_TASK_MANAGER_GET_PRIVATE (task_manager);

	__find_by_name (priv, &term, name);

	if (term.task == NULL) {
		g_print (" task not found\n");
		return TRUE;
	}
	awn_task_unset_info (term.task);

	return TRUE;
}

static gboolean
awn_task_manager_unset_info_by_xid (AwnTaskManager *task_manager,
			    		gint64		xid,
			    		GError   	**error)
{
	AwnTaskManagerPrivate *priv;
	AwnDBusTerm term;

	priv = AWN_TASK_MANAGER_GET_PRIVATE (task_manager);

	__find_by_xid (priv, &term, xid);

	if (term.task == NULL) {
		g_print (" task not found\n");
		return TRUE;
	}
	awn_task_unset_info (term.task);
	return TRUE;
}

static gboolean
awn_task_manager_add_task_menu_item_by_name (AwnTaskManager *task_manager,
			    		     gchar		*name,
			    		     gchar		*stock_id,
			    		     gchar		*item_name,
			    		     gint		**id,
			    		     GError      	**error)
{
	AwnTaskManagerPrivate *priv;
	AwnDBusTerm term;

	priv = AWN_TASK_MANAGER_GET_PRIVATE (task_manager);

	__find_by_name (priv, &term, name);

	if (term.task == NULL) {
		g_print (" task not found\n");
		return TRUE;
	}
	GtkWidget *item;
	item = NULL;

	if (strlen (stock_id) > 4) {
		item = gtk_image_menu_item_new_from_stock (stock_id, NULL);

	} else if (item_name) {
		item = gtk_menu_item_new_with_mnemonic (item_name);

	} else {
		;
	}
	if (item)
		*id = (gint *) awn_task_add_menu_item (term.task, GTK_MENU_ITEM (item));
	else
		*id = 0;


	if (item && *id == 0)
		gtk_widget_destroy (GTK_WIDGET (item));
	return TRUE;
}

static gboolean
awn_task_manager_add_task_check_item_by_name (AwnTaskManager *task_manager,
			    		     gchar		*name,
			    		     gchar		*item_name,
			    		     gboolean		active,
			    		     gint		**id,
			    		     GError      	**error)
{
	AwnTaskManagerPrivate *priv;
	AwnDBusTerm term;

	priv = AWN_TASK_MANAGER_GET_PRIVATE (task_manager);

	__find_by_name (priv, &term, name);

	if (term.task == NULL) {
		g_print (" task not found\n");
		return TRUE;
	}
	GtkWidget *item;
	item = NULL;

	item = gtk_check_menu_item_new_with_mnemonic  (item_name);
	gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (item), active);

	if (item)
		*id = (gint *) awn_task_add_check_item (term.task, GTK_MENU_ITEM (item));
	else
		*id = 0;


	if (item && *id == 0)
		gtk_widget_destroy (GTK_WIDGET (item));
	return TRUE;
}

static gboolean
awn_task_manager_set_task_check_item_by_name (AwnTaskManager *task_manager,
			    		     gchar		*name,
			    		     gint		id,
			    		     gboolean		active,
			    		     GError      	**error)
{
	AwnTaskManagerPrivate *priv;
	AwnDBusTerm term;

	priv = AWN_TASK_MANAGER_GET_PRIVATE (task_manager);

	__find_by_name (priv, &term, name);

	if (term.task == NULL) {
		g_print (" task not found\n");
		return TRUE;
	}

	awn_task_set_check_item (term.task, id, active);
	return TRUE;
}
/********************* /DBUS   ********************************/

/********************* awn_task_manager_new * *******************/

#include "awn-dbus-glue.h"

static void
awn_task_manager_dispose (GObject *object)
{
        AwnTaskManagerPrivate *priv;
        priv = AWN_TASK_MANAGER_GET_PRIVATE (object);

        if (priv->applet_man_proxy) {
                dbus_g_proxy_disconnect_signal (priv->applet_man_proxy,
                                               "HeightChanged",
                                               G_CALLBACK (on_height_changed),
                                               (gpointer)object);
                priv->applet_man_proxy = NULL;
        }
        if (priv->applet_man_connection) {
                dbus_g_connection_unref (priv->applet_man_connection);
                priv->applet_man_connection = NULL;
        }

        // oh yea, this one is disgusting, but it basically only
        //  disconnects a few signals
        if (priv->signal_handlers[0]) {
                gint i;
                for (i = 0; i < G_N_ELEMENTS(priv->signal_handlers); i++) {
                        gpointer instance = i <= 3 ? (priv->screen) : 
                        (i == 4 ? priv->settings->window : (gpointer)gtk_icon_theme_get_default ());
                        if (priv->signal_handlers[i]) {
                                g_signal_handler_disconnect(instance,
                                        priv->signal_handlers[i]);
                                priv->signal_handlers[i] = 0;
                        }
                }
        }

	// hide the separator
	awn_bar_set_draw_separator (priv->settings->bar, 0);

	G_OBJECT_CLASS (awn_task_manager_parent_class)->dispose (object);
}

static void
awn_task_manager_class_init (AwnTaskManagerClass *class)
{
	GObjectClass *obj_class;
	GtkWidgetClass *widget_class;

	obj_class = G_OBJECT_CLASS (class);
	obj_class->dispose = awn_task_manager_dispose;
	widget_class = GTK_WIDGET_CLASS (class);

	g_type_class_add_private (obj_class, sizeof (AwnTaskManagerPrivate));

	awn_task_manager_signals[MENU_ITEM_CLICKED] =
		g_signal_new ("menu_item_clicked",
			      G_OBJECT_CLASS_TYPE (obj_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (AwnTaskManagerClass, menu_item_clicked),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__UINT,
			      G_TYPE_NONE,
			      1,
			      G_TYPE_UINT);

	awn_task_manager_signals[CHECK_ITEM_CLICKED] =
		g_signal_new ("check_item_clicked",
			      G_OBJECT_CLASS_TYPE (obj_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (AwnTaskManagerClass, check_item_clicked),
			      NULL, NULL,
			      _awn_marshal_VOID__UINT_BOOLEAN,
			      G_TYPE_NONE,
			      2,
			      G_TYPE_UINT, G_TYPE_BOOLEAN);

	dbus_g_object_type_install_info (G_TYPE_FROM_CLASS (class),
					 &dbus_glib_awn_task_manager_object_info);
}

static void 
awn_task_manager_refresh_launchers (AwnConfigClientNotifyEntry *entry, 
                                   AwnTaskManager *task_manager)
{
	AwnTaskManagerPrivate *priv;
	GSList *list, *l;
	GList *t;
	GSList *launchers = NULL, *old_launchers;

        g_return_if_fail (AWN_IS_TASK_MANAGER (task_manager));
	priv = AWN_TASK_MANAGER_GET_PRIVATE (task_manager);

        list = entry->value.list_val;
 
        for (l = list; l != NULL; l = l->next) {
                gchar *string = g_strdup ((gchar*) (l->data));
                launchers = g_slist_append (launchers, string);
        }
        
        old_launchers = priv->settings->launchers;
        priv->settings->launchers = launchers;
        
        gint i = 0;
        for (l = launchers; l != NULL; l = l->next) {
                AwnTask *task = NULL;
                for (t = priv->launchers; t != NULL; t = t->next) {
                        AwnDesktopItem *item, *old_item;
                        item = awn_task_get_item (AWN_TASK (t->data));
                        gchar *file = awn_desktop_item_get_filename (item);
                        if (g_strcmp0 (file, l->data) == 0) {
                                task = AWN_TASK (t->data);
                                g_free (file);
                                // refresh the desktop item
                                old_item = item;
                                item = awn_desktop_item_new ((gchar*)l->data);
                                if (item) {
                                        awn_task_set_launcher(task, item);
                                        awn_desktop_item_free(old_item);
                                }
                                break;
                        }
                        g_free (file);
                }
                if (task) {
                        gtk_box_reorder_child (GTK_BOX (priv->launcher_box),
                                               GTK_WIDGET (task),
                                               i++);
                } else {
			// added launcher
			GtkWidget *task;
			AwnDesktopItem *item = awn_desktop_item_new (l->data);

			if (item == NULL) {
				g_print("Error : Could not load the desktop file!");
				continue;
			}
			g_print("LOADED : %s\n", (char*)l->data);

			task = awn_task_new(task_manager, priv->settings);
			awn_task_set_title (AWN_TASK(task), AWN_TITLE(priv->title_window));
			if (awn_task_set_launcher (AWN_TASK (task), item)) {

				g_signal_connect (G_OBJECT(task), "drag-data-received",
						  G_CALLBACK(_task_manager_drag_data_recieved), (gpointer)task_manager);
				g_signal_connect (G_OBJECT(task), "menu_item_clicked",
						  G_CALLBACK(_task_manager_menu_item_clicked), (gpointer)
						  task_manager);
				g_signal_connect (G_OBJECT(task), "check_item_clicked",
						  G_CALLBACK(_task_manager_check_item_clicked), (gpointer)
						  task_manager);

				priv->launchers = g_list_append(priv->launchers, (gpointer)task);
				gtk_box_pack_start(GTK_BOX(priv->launcher_box), task, FALSE, FALSE, 0);
                                gtk_box_reorder_child (GTK_BOX (priv->launcher_box),
                                                       task,
                                                       i++);
				gtk_widget_show(task);
			} else {
				gtk_widget_destroy(task);
				awn_desktop_item_free (item);
				g_print("FAILED : %s\n", (char*)l->data);
			}
                }
        }

        // make a list of removed launchers
        l = old_launchers;
        while (l) {
                gboolean found = FALSE;
                for (list = launchers; list; list = list->next) {
                        if (strcmp(list->data, l->data) == 0) {
                                found = TRUE;
                                break;
                        }
                }
                if (found) {
                        old_launchers = g_slist_delete_link(old_launchers, l);
                        l = old_launchers;
                } else {
                        l = l->next;
                }
        }
        // old_launchers now contains list of removed paths
        for (l = old_launchers; l; l = l->next) {
                AwnTask *task = NULL;
                for (t = priv->launchers; t != NULL; t = t->next) {
                        AwnDesktopItem *item;
                        item = awn_task_get_item (AWN_TASK (t->data));
                        gchar *file = awn_desktop_item_get_filename (item);
                        if (strcmp (file, l->data) == 0) {
                                task = AWN_TASK (t->data);
                        }
                        g_free (file);
                        
                }
                if (task) {
                        // remove from launchers
		        g_print("REMOVED : %s\n", (char*)l->data);
                        if (awn_task_get_window (task)) {
                                // move to tasks
                                GtkWidget *w = GTK_WIDGET (task);
                                awn_task_manager_remove_launcher (task_manager, task);
                                awn_task_set_launcher (task, NULL);
                                priv->tasks = g_list_append(priv->tasks, (gpointer)task);
                                gtk_widget_ref(w);
                                gtk_container_remove(GTK_CONTAINER(priv->launcher_box), w);
                                gtk_box_pack_start(GTK_BOX(priv->tasks_box), w, FALSE, FALSE, 0);
                                gtk_widget_unref(w);
                        } else {
                                // destroy completely
                                awn_task_manager_remove_launcher (task_manager,
                                                                  task);
                                awn_task_remove (task);
                        }
                }
        }

        g_slist_free (old_launchers);
        
	awn_task_manager_update_separator_position (task_manager);
       	_refresh_box(task_manager);
}

gboolean
awn_task_manager_screen_has_viewports (AwnTaskManager *task_manager)
{
        AwnTaskManagerPrivate *priv;

        g_return_val_if_fail (AWN_IS_TASK_MANAGER (task_manager), FALSE);
        priv = AWN_TASK_MANAGER_GET_PRIVATE (task_manager);

	return priv->got_viewport;
}

static void 
awn_task_manager_update_activate_behavior (AwnConfigClientNotifyEntry *entry,
                                   AwnTaskManager *task_manager)
{
        AwnTaskManagerPrivate *priv;

        g_return_if_fail (AWN_IS_TASK_MANAGER (task_manager));
        priv = AWN_TASK_MANAGER_GET_PRIVATE (task_manager);

	priv->activate_behavior = entry->value.int_val;
}

gint
awn_task_manager_get_activate_behavior (AwnTaskManager *task_manager)
{
        AwnTaskManagerPrivate *priv;

        g_return_val_if_fail (AWN_IS_TASK_MANAGER (task_manager), 0);
        priv = AWN_TASK_MANAGER_GET_PRIVATE (task_manager);

	return priv->activate_behavior;
}

static void
on_height_changed (DBusGProxy *proxy, gint height, AwnTaskManager *manager)
{
	AwnTaskManagerPrivate *priv;
	GList *l;
	
	g_return_if_fail (AWN_IS_TASK_MANAGER (manager));
	priv = AWN_TASK_MANAGER_GET_PRIVATE (manager);
	
	for (l = priv->launchers; l; l = l->next) {
	
		awn_task_set_width (AWN_TASK (l->data), height+12);
	}
	
	for (l = priv->tasks; l; l = l->next) {
	
		awn_task_set_width (AWN_TASK (l->data), height+12);
	}
}

static void
awn_task_manager_init (AwnTaskManager *task_manager)
{
	AwnTaskManagerPrivate *priv;
	AwnConfigClient *client = awn_config_client_new ();
        DBusGConnection *connection;
        DBusGProxy *proxy = NULL;
        GError *error = NULL;
	
	priv = AWN_TASK_MANAGER_GET_PRIVATE (task_manager);

	priv->screen = wnck_screen_get_default();

	priv->title_window = NULL;
	priv->launchers = NULL;
	priv->tasks = NULL;
	
	/* Setup GConf to notify us if the launchers list changes */
	awn_config_client_notify_add (client, "window_manager", "launchers", 
                (AwnConfigClientNotifyFunc)awn_task_manager_refresh_launchers, 
                task_manager);

	priv->activate_behavior = awn_config_client_get_int (client,
		"window_manager", "activate_behavior", NULL);
	awn_config_client_notify_add (client,
		"window_manager", "activate_behavior",
		(AwnConfigClientNotifyFunc)
		awn_task_manager_update_activate_behavior, task_manager);

        connection = dbus_g_bus_get (DBUS_BUS_SESSION, &error);
        if (!connection) {
		if (error) {
			g_warning ("Failed to make connection to session bus: %s",
				   error->message);
			g_error_free (error);
		}
                return;
	}
        priv->applet_man_connection = connection;

        proxy = dbus_g_proxy_new_for_name (connection,
                                          "com.google.code.Awn.AppletManager",
                                          "/com/google/code/Awn/AppletManager",
                                          "com.google.code.Awn.AppletManager");
        if (!proxy) {
                g_warning ("Cannot connect to applet manager\n");
                return;
        }
        priv->applet_man_proxy = proxy;
	dbus_g_proxy_add_signal (proxy, "HeightChanged", G_TYPE_INT, G_TYPE_INVALID);
        dbus_g_proxy_connect_signal (proxy, 
                                     "HeightChanged",
                                     G_CALLBACK (on_height_changed),
                                     (gpointer)task_manager, 
                                     NULL);
}

static gboolean
awn_task_manager_realized (gpointer data)
{
        AwnTaskManager *task_manager = AWN_TASK_MANAGER(data);
        AwnTaskManagerPrivate *priv = AWN_TASK_MANAGER_GET_PRIVATE (data);

        GList *l;
        GList *list = wnck_screen_get_windows (priv->screen);
        for (l = list; l != NULL; l = l->next) {
                _task_manager_window_opened (priv->screen,
                                             (WnckWindow*)l->data,
                                             task_manager);
        }
        // fix the height
        on_height_changed (NULL, priv->settings->bar_height, task_manager);

        return FALSE;
}

GtkWidget *
awn_task_manager_new (AwnSettings *settings)
{
	TASKMAN_OWN_PID = getpid();

	GtkWidget *task_manager;
	AwnTaskManagerPrivate *priv;

	task_manager = g_object_new (AWN_TYPE_TASK_MANAGER,
			     "homogeneous", FALSE,
			     "spacing", 0 ,
			     NULL);
	priv = AWN_TASK_MANAGER_GET_PRIVATE (task_manager);

	// this must be here, so get_active_workspace doesn't return NULL
	// but it makes us loose the window-opened signals, so we use
	// g_idle_add to enumerate the windows
	wnck_screen_force_update(priv->screen);
	WnckWorkspace *wrksp = wnck_screen_get_active_workspace(priv->screen);
	if (wrksp) priv->got_viewport = wnck_workspace_is_virtual(wrksp);

#ifdef HAVE_LIBWNCK_220
	if (!priv->got_viewport &&
		wnck_screen_get_window_manager_name(priv->screen))
	{
		if (strcmp(wnck_screen_get_window_manager_name(priv->screen), "compiz") == 0)
		priv->got_viewport = TRUE;
	}
#endif

        // wnck hack -> we want the window-opened signal
        g_idle_add (awn_task_manager_realized, task_manager);

	priv->settings = settings;
	priv->launcher_box = gtk_hbox_new(FALSE, 0);
	priv->tasks_box = gtk_hbox_new(FALSE, 0);
	priv->eb = gtk_event_box_new();
	gtk_widget_set_size_request(priv->eb, 4, -1);
	gtk_event_box_set_visible_window (GTK_EVENT_BOX(priv->eb), FALSE);

	gtk_box_pack_start(GTK_BOX(task_manager), priv->launcher_box, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(task_manager), priv->eb, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(task_manager), priv->tasks_box, FALSE, FALSE, 0);

	gtk_widget_show(priv->launcher_box);
	gtk_widget_show(priv->tasks_box);

	priv->title_window = awn_title_get_default ();
	settings->title = priv->title_window;

	_task_manager_load_launchers(AWN_TASK_MANAGER (task_manager));

	/* LIBWNCK SIGNALS */
        priv->signal_handlers[0] =
            g_signal_connect (G_OBJECT(priv->screen), "window-opened",
	                      G_CALLBACK (_task_manager_window_opened),
	                      (gpointer)task_manager);

        priv->signal_handlers[1] =
            g_signal_connect (G_OBJECT(priv->screen), "window-closed",
	                      G_CALLBACK(_task_manager_window_closed),
	                      (gpointer)task_manager);

        priv->signal_handlers[2] =
            g_signal_connect (G_OBJECT(priv->screen), "active-window-changed",
	                      G_CALLBACK(_task_manager_window_activate),
	                      (gpointer)task_manager);

#ifdef HAVE_LIBWNCK_220
        priv->signal_handlers[3] =
            g_signal_connect (G_OBJECT(priv->screen), "viewports-changed",
	                      G_CALLBACK(_task_manager_viewports_changed),
	                      (gpointer)task_manager);
#else
        priv->signal_handlers[3] = 0;
#endif

	/* CONNECT D&D CODE */

        priv->signal_handlers[4] =
            g_signal_connect (G_OBJECT(settings->window), "drag-data-received",
                              G_CALLBACK(_task_manager_drag_data_recieved), (gpointer)task_manager);

	/* THEME CHANGED CODE */
	GtkIconTheme *theme = gtk_icon_theme_get_default ();

        priv->signal_handlers[5] =
            g_signal_connect (G_OBJECT(theme), "changed",
                              G_CALLBACK(_task_manager_icon_theme_changed), (gpointer)task_manager);

	/* SEP EXPOSE EVENT */

	g_signal_connect (G_OBJECT(priv->eb), "expose-event",
			  G_CALLBACK(awn_bar_separator_expose_event), (gpointer)settings->bar);			  

        #define A_NAMESPACE "com.google.code.Awn"
        #define A_OBJECT_PATH "/com/google/code/Awn"
        DBusGConnection *connection;
        DBusGProxy *proxy;
        GError *error = NULL;
        guint32 ret;

        /* Get the connection and ensure the name is not used yet */
        connection = dbus_g_bus_get (DBUS_BUS_SESSION, &error);
        if (connection == NULL) {
                g_warning ("Failed to make connection to session bus: %s",
                           error->message);
                g_error_free (error);
                return task_manager;
        }

        proxy = dbus_g_proxy_new_for_name (connection, DBUS_SERVICE_DBUS,
                                           DBUS_PATH_DBUS, DBUS_INTERFACE_DBUS);
        if (!org_freedesktop_DBus_request_name (proxy, A_NAMESPACE,
                                                0, &ret, &error)) {
                g_warning ("There was an error requesting the name: %s",
                           error->message);
                g_error_free (error);
                return task_manager;
        }
        if (ret != DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER) {
                /* Someone else registered the name before us */
                return task_manager;
        }
        /* Register the task manager on the bus */
        dbus_g_connection_register_g_object (connection,
                                             A_OBJECT_PATH,
                                             G_OBJECT (task_manager));


	return task_manager;
}

