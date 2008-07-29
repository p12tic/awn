/*
 *  Copyright (C) 2007, 2008 Mark Lee <avant-wn@lazymalevolence.com>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the
 *  Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 *  Boston, MA 02111-1307, USA.
 *
 *  Author : Mark Lee <avant-wn@lazymalevolence.com>
 */

#include <sys/file.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <string.h>

#if GLIB_CHECK_VERSION(2,15,0)
#include <glib/gchecksum.h>
#else
#include "egg/eggchecksum.h"
#endif

#ifndef LOCK_SH
#define   LOCK_SH   1    /* shared lock */
#define   LOCK_EX   2    /* exclusive lock */
#define   LOCK_NB   4    /* don't block when locking */
#define   LOCK_UN   8    /* unlock */
#endif

static gpointer
_awn_config_client_copy (gpointer boxed)
{
	return boxed;
}

static void
_awn_config_client_free (gpointer boxed)
{
	if (boxed) {
		awn_config_client_free (AWN_CONFIG_CLIENT (boxed));
	}
}

GType awn_config_client_get_type (void)
{
	static GType type = 0;

	if (type == 0) {
		type = g_boxed_type_register_static ("AwnConfigClient",
						     _awn_config_client_copy,
						     _awn_config_client_free);
	}

	return type;
}

/**
 * awn_config_client_key_lock_open:
 * @group: The group name of the entry.
 * @key: The key name of the entry.
 *
 * Creates a locking file and file descriptor associated with the (group, key) pair.
 * Returns: file descriptor for the locking file. -1 on error.
 */
int awn_config_client_key_lock_open (const gchar *group, const gchar *key)
{
	int fd;
	gchar *data     = g_strdup_printf ("%s-%s", group, key);
	gchar *checksum = g_compute_checksum_for_string (G_CHECKSUM_SHA256, data, strlen (data));
	gchar *filename = g_strdup_printf ("%s/awn-lock%s.lock", g_get_tmp_dir (), checksum);
	fd = open (filename, O_CREAT | O_RDWR, S_IRWXU);
	g_free (checksum);
	g_free (data);
	g_free (filename);
	return fd;
}

/**
 * awn_config_client_key_lock:
 * @fd: File descriptor provided by awn_config_client_key_lock_open().
 * @operation: as per 4.4BSD flock().
 *
 * Attempts to attain a lock as per flock() semantics.
 * Returns: On success, zero is returned.  On error, -1 is returned, and errno is set appropriately.
 */
int awn_config_client_key_lock (int fd, int operation)
{
        struct flock f;

        memset(&f, 0, sizeof (f));
        if (operation & LOCK_UN)
            f.l_type = F_UNLCK;
        if (operation & LOCK_SH)
            f.l_type = F_RDLCK;
        if (operation & LOCK_EX)
            f.l_type = F_WRLCK;
        f.l_start = 0 ;
        f.l_whence = SEEK_SET;
        f.l_len = 0 ;
        f.l_pid = getpid() ;

        return fcntl(fd, (operation & LOCK_NB) ? F_SETLK : F_SETLKW, &f);
}

/**
 * awn_config_client_key_lock_close:
 * @fd: File descriptor provided by awn_config_client_key_lock_open().
 *
 * Attempts to close the file descriptor obtained with awn_config_client_key_lock_open().
 * Returns: On success, zero is returned.  On error, -1 is returned, and errno is set appropriately.
 */
int awn_config_client_key_lock_close (int fd)
{
	return close (fd);
}

