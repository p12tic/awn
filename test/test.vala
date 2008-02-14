/*
 * Tests the Vala bindings for Awn.
 *
 * Copyright (C) 2007 Mark Lee <avant-wn@lazymalevolence.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301  USA.
 *
 * Author : Mark Lee <avant-wn@lazymalevolence.com>
 */

using Awn;
using GLib;

public class AwnTest : Object {
	static int main (string[] args) {
		weak SList<string> applets;
		ConfigClient client = new ConfigClient ();
		applets = client.get_list ("DEFAULT", "applets_list", ConfigListType.STRING);
		stdout.printf ("Applet list:\n");
		for (int i = 0; i < applets.length (); i++) {
			string path;
			path = applets.nth_data (i);
			stdout.printf (" * %s\n", path);
		}
	}
}

/* vim: set ft=cs noet ts=8 sts=8 sw=8 : */
