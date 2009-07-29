/* 
 * Copyright (C) 2009 Rodney Cryderman <rcryderman@gmail.com> 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as 
 * published by the Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <glib.h>
#undef G_DISABLE_SINGLE_INCLUDES
#include <glibtop/procargs.h>
#include <glibtop/procuid.h>

#include "util.h"

typedef struct
{
  const gchar * exec;
  const gchar * name;
  const gchar * filename;
  const gchar * id;
}DesktopMatch;

typedef struct
{
  const gchar * cmd;
  const gchar * res_name;
  const gchar * class_name;
  const gchar * title;
  const gchar * id;
}WindowMatch;

typedef struct
{
  const gchar * cmd;
  const gchar * res_name;
  const gchar * class_name;
  const gchar * title;
  const gchar * desktop;
}WindowToDesktopMatch;

static DesktopMatch desktop_regexes[] = 
{
  {".*ooffice.*-writer.*",".*OpenOffice.*",NULL,"OpenOffice-Writer"},
  {".*ooffice.*-draw.*",".*OpenOffice.*",NULL,"OpenOffice-Draw"},
  {".*ooffice.*-impress.*",".*OpenOffice.*",NULL,"OpenOffice-Impress"},
  {".*ooffice.*-calc.*",".*OpenOffice.*",NULL,"OpenOffice-Calc"},    
  {NULL,NULL,NULL,NULL}
};


static  WindowMatch window_regexes[] = 
{
  /*Do not bother trying to parse an open office command line for the type of window*/
  {".*office.*",".*OpenOffice.*",".*VCLSalFrame.*DocumentWindow.*",".*Writer.*","OpenOffice-Writer"},
  {".*office.*",".*OpenOffice.*",".*VCLSalFrame.*DocumentWindow.*",".*Draw.*","OpenOffice-Draw"},
  {".*office.*",".*OpenOffice.*",".*VCLSalFrame.*DocumentWindow.*",".*Impress.*","OpenOffice-Impress"},
  {".*office.*",".*OpenOffice.*",".*VCLSalFrame.*DocumentWindow.*",".*Calc.*","OpenOffice-Calc"},    
  {NULL,NULL,NULL,NULL,NULL}
};

static  WindowToDesktopMatch window_to_desktop_regexes[] = 
{
  /*Do not bother trying to parse an open office command line for the type of window*/
  {".*office.*",".*OpenOffice.*",".*VCLSalFrame.*DocumentWindow.*",".*Writer.*","ooo-writer"},
  {".*office.*",".*OpenOffice.*",".*VCLSalFrame.*DocumentWindow.*",".*Draw.*","ooo-draw"},
  {".*office.*",".*OpenOffice.*",".*VCLSalFrame.*DocumentWindow.*",".*Impress.*","ooo-impress"},
  {".*office.*",".*OpenOffice.*",".*VCLSalFrame.*DocumentWindow.*",".*Calc.*","ooo-calc"},
  {".*gimp.*",".*Gimp.*",".*gimp.*",".*GNU.*Image.*Manipulation.*Program.*","gimp"},
  {".*system-config-printer.*applet.*py.*",".*Applet.*py.*",".*applet.*",".*Print.*Status.*","redhat-manage-print-jobs"},  
  {NULL,NULL,NULL,NULL,NULL}
};

/*
 Special Casing should NOT be used for anything but a last resort.  
 Other matching algororithms are NOT used if something is special cased.
*/
gchar *
get_special_id_from_desktop (AwnDesktopItem * item)
{
  /*
   Exec,Name,filename, special_id.  If all in the first 3 match then the 
   special_id is returned.
   
   TODO  put data into a separate file
   
   TODO  optimize the regex handling.
   */
  DesktopMatch  *iter;
  for (iter = desktop_regexes; iter->id; iter++)
  {
    gboolean  match = TRUE;
    if (iter->exec)
    {
      gchar * exec = awn_desktop_item_get_exec (item);
      match = g_regex_match_simple(iter->exec, exec,0,0);
      g_free (exec);
      if (!match)
        continue;
    }
    if (iter->name)
    {
      gchar * name = awn_desktop_item_get_name (item);
      match = g_regex_match_simple(iter->name, name,0,0);
      g_free (name);
      if (!match)
        continue;
    }
    if (iter->filename)
    {
      const gchar * filename = awn_desktop_item_get_filename (item);
      match = g_regex_match_simple(iter->filename, filename,0,0);
      if (!match)
        continue;
    }
    g_debug ("%s:  Special cased ID: '%s'",__func__,iter->id);
    return g_strdup (iter->id);
  }
  return NULL;
}

/*
 Special Casing should NOT be used for anything but a last resort.  
 Other matching algororithms are NOT used if something is special cased.
*/

gchar *
get_special_id_from_window_data (gchar * cmd, gchar *res_name, gchar * class_name,const gchar *title)
{
  /*
   Exec,Name,filename, special_id.  If all in the first 3 match then the 
   special_id is returned.

   TODO  put data into a separate file
   
   TODO  optimize the regex handling.
   */
  WindowMatch  *iter;
  for (iter = window_regexes; iter->id; iter++)
  {
    gboolean  match = TRUE;
    if (iter->cmd)
    {
      match = cmd && g_regex_match_simple(iter->cmd, cmd,0,0);
      if (!match)
        continue;
    }
    if (iter->res_name)
    {
      match = res_name && g_regex_match_simple(iter->res_name, res_name,0,0); 
      if (!match)
        continue;
    }
    if (iter->class_name)
    {
      match = class_name && g_regex_match_simple(iter->class_name, class_name,0,0);
      if (!match)
        continue;
    }        
    if (iter->title)
    {
      match = title && g_regex_match_simple(iter->title, title,0,0);
      if (!match)
        continue;
    }    
    g_debug ("%s:  Special cased Window ID: '%s'",__func__,iter->id);
    return g_strdup (iter->id);
  }
  return NULL;
}

gchar *
get_special_desktop_from_window_data (gchar * cmd, gchar *res_name, gchar * class_name,const gchar *title)
{
  /*
   Exec,Name,filename, special_id.  If all in the first 3 match then the 
   special_id is returned.

   TODO  put data into a separate file
   
   TODO  optimize the regex handling.
   */
  WindowToDesktopMatch  *iter;
//#define DEBUG
#ifdef DEBUG
  g_debug ("%s: cmd = '%s', res = '%s', class = '%s', title = '%s'",__func__,cmd,res_name,class_name,title);
#endif
  for (iter = window_to_desktop_regexes; iter->desktop; iter++)
  {
    gboolean  match = TRUE;
    if (iter->cmd)
    {
      match = cmd && g_regex_match_simple(iter->cmd, cmd,0,0);
      if (!match)
        continue;
    }
    if (iter->res_name)
    {
      match = res_name && g_regex_match_simple(iter->res_name, res_name,0,0); 
      if (!match)
        continue;
    }
    if (iter->class_name)
    {
      match = class_name && g_regex_match_simple(iter->class_name, class_name,0,0);
      if (!match)
        continue;
    }        
    if (iter->title)
    {
      match = title && g_regex_match_simple(iter->title, title,0,0);
      if (!match)
        continue;
    }    
    g_debug ("%s:  Special cased desktop: '%s'",__func__,iter->desktop);
    return g_strdup (iter->desktop);
  }
  return NULL;
}



gchar * 
get_full_cmd_from_pid (gint pid)
{
  gchar   * full_cmd = NULL;
  gchar   **cmd_argv;
  glibtop_proc_args buf;
  gchar * temp;
  
  cmd_argv = glibtop_get_proc_argv (&buf,pid,1024);
  if (cmd_argv)
  {
    gchar ** iter;
    for (iter = cmd_argv;*iter;iter++)
    {
      temp = full_cmd;
      full_cmd = g_strdup_printf("%s%s%s",full_cmd?full_cmd:"",full_cmd?" ":"",*iter);
      g_free (temp);
    }
  }
  g_strfreev (cmd_argv);   
  return full_cmd;
}