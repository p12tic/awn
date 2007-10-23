#!/usr/bin/env python
#
#  Copyright (C) 2007 Mark Lee <avant-wn@lazymalevolence.com>
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301  USA.
#
#  Author : Mark Lee <avant-wn@lazymalevolence.com>

import sys
import awn

GROUP = 'test'

def print_err(err):
    sys.stderr.write(err + '\n')

def test_option_bool(client):
    print '* set bool...',
    val = True
    try:
        client.set_bool(GROUP, 'bool', val)
    except Exception, err:
        print_err(str(err))
        return 1
    print 'done.'
    print '* get bool...',
    val = False
    try:
        val = client.get_bool(GROUP, 'bool')
    except Exception, err:
        print_err(str(err))
        return 1
    print 'done.'
    if val == True:
        return 0
    else:
        return 1

def test_option_float(client):
    print '* set float...',
    val = 0.5
    try:
        client.set_float(GROUP, 'float', val)
    except Exception, err:
        print_err(str(err))
        return 1
    print 'done.'
    print '* get float...',
    val = 0
    try:
        val = client.get_float(GROUP, 'float')
    except Exception, err:
        print_err(str(err))
        return 1
    print 'done.'
    if val == 0.5:
        return 0
    else:
        return 1

def test_option_int(client):
    print '* set int...',
    val = 4
    try:
        client.set_int(GROUP, 'int', val)
    except Exception, err:
        print_err(str(err))
        return 1
    print 'done.'
    print '* get int...',
    val = 0
    try:
        val = client.get_int(GROUP, 'int')
    except Exception, err:
        print_err(str(err))
        return 1
    print 'done.'
    if val == 4:
        return 0
    else:
        return 1

def test_option_string(client):
    print '* set string...',
    val = 'test'
    try:
        client.set_string(GROUP, 'str', val)
    except Exception, err:
        print_err(str(err))
        return 1
    print 'done.'
    print '* get string...',
    val = ''
    try:
        val = client.get_string(GROUP, 'str')
    except Exception, err:
        print_err(str(err))
        return 1
    print 'done.'
    if val == 'test':
        return 0
    else:
        return 1

def check_lists(list_get, list_set):
    retval = 0
    if len(list_get) != len(list_set):
        retval = 1
    else:
        list_len = len(list_set)
        for i in range(list_len):
            if list_get[i] != list_set[i]:
                print_err('The lists differ at list index %s: get="%s", set="%s"' % (i, list_get[i], list_set[i]))
                retval = 1
    return retval

def test_option_list_bool(client):
    print '* set list (bool)...',
    list_set = [True, False, True]
    try:
        client.set_list(GROUP, 'list_bool', awn.CONFIG_LIST_BOOL, list_set)
    except Exception, err:
        print_err(str(err))
        return 1
    print 'done.'
    print '* get list (bool)...',
    try:
        list_get = client.get_list(GROUP, 'list_bool', awn.CONFIG_LIST_BOOL)
    except Exception, err:
        print_err(str(err))
        return 1
    print 'done.'
    return check_lists(list_get, list_set)

def test_option_list_float(client):
    print '* set list (float)...',
    list_set = [0.2, 0.4, 0.6]
    try:
        client.set_list(GROUP, 'list_float', awn.CONFIG_LIST_FLOAT, list_set)
    except Exception, err:
        print_err(str(err))
        return 1
    print 'done.'
    print '* get list (float)...',
    try:
        list_get = client.get_list(GROUP, 'list_float', awn.CONFIG_LIST_FLOAT)
    except Exception, err:
        print_err(str(err))
        return 1
    print 'done.'
    return check_lists(list_get, list_set)

def test_option_list_int(client):
    print '* set list (int)...',
    list_set = [1, 3, 5]
    try:
        client.set_list(GROUP, 'list_int', awn.CONFIG_LIST_INT, list_set)
    except Exception, err:
        print_err(str(err))
        return 1
    print 'done.'
    print '* get list (int)...',
    try:
        list_get = client.get_list(GROUP, 'list_int', awn.CONFIG_LIST_INT)
    except Exception, err:
        print_err(str(err))
        return 1
    print 'done.'
    return check_lists(list_get, list_set)

def test_option_list_string(client):
    print '* set list (string)...',
    list_set = ['One', 'Two', 'Three']
    try:
        client.set_list(GROUP, 'list_str', awn.CONFIG_LIST_STRING, list_set)
    except Exception, err:
        print_err(str(err))
        return 1
    print 'done.'
    print '* get list (string)...',
    try:
        list_get = client.get_list(GROUP, 'list_str', awn.CONFIG_LIST_STRING)
    except Exception, err:
        print_err(str(err))
        return 1
    print 'done.'
    return check_lists(list_get, list_set)

def test_option_types(client):
    retval = 0
    retval |= test_option_bool(client)
    retval |= test_option_float(client)
    retval |= test_option_int(client)
    retval |= test_option_string(client)
    retval |= test_option_list_bool(client)
    retval |= test_option_list_float(client)
    retval |= test_option_list_int(client)
    retval |= test_option_list_string(client)
    return retval

def test_awn_config():
    retval = 0
    print 'Main config client'
    client = awn.Config()
    if client is None:
        return 1
    retval |= test_option_types(client)
    return retval

def test_applet_config():
    retval = 0
    print 'Applet config client'
    client = awn.Config('test')
    if client is None:
        return 1
    retval |= test_option_types(client)
    return retval

def main():
    retval = 0
    retval |= test_awn_config()
    retval |= test_applet_config()
    return retval

if __name__ == '__main__':
    sys.exit(main())
