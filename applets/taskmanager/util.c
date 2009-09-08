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


//#define DEBUG


/*
 TODO: Split/cleanup this and related code in taskmanager-c into a separater 
      object/lib once it comes time to do a caching of results + tracking of app 
      use (early in 0.6).  
  
      The various uses are kind of obvious.  Such as use by shinyswitcher. And 
      use your imagination.
 
      Special casing info should be moved into separate datafiles/db.
 
      Tool to analyze and special case windows by advanced users ala xprop 
      (point and click) and analyze.
 */

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


const gchar * blacklist[] = {"prism",
                       NULL};
/*Assign an id to a desktop file
 
 exec field,name field,desktop filename,id
 */
static DesktopMatch desktop_regexes[] = 
{
  {".*ooffice.*-writer.*",".*OpenOffice.*",NULL,"OpenOffice-Writer"},
  {".*ooffice.*-draw.*",".*OpenOffice.*",NULL,"OpenOffice-Draw"},
  {".*ooffice.*-impress.*",".*OpenOffice.*",NULL,"OpenOffice-Impress"},
  {".*ooffice.*-calc.*",".*OpenOffice.*",NULL,"OpenOffice-Calc"},
  {".*amsn.*","aMSN",".*amsn.*desktop.*","aMSN"},
  {".*prism-google-calendar",".*Google.*Calendar.*","prism-google-calendar","prism-google-calendar"},
  {".*prism-google-analytics",".*Google.*Analytics.*","prism-google-analytics","prism-google-analytics"},
  {".*prism-google-docs",".*Google.*Docs.*","prism-google-docs","prism-google-docs"},
  {".*prism-google-groups",".*Google.*Groups.*","prism-google-groups","prism-google-groups"},
  {".*prism-google-mail",".*Google.*Mail.*","prism-google-mail","prism-google-mail"},
  {".*prism-google-reader",".*Google.*Reader.*","prism-google-reader","prism-google-reader"},
  {".*prism-google-talk",".*Google.*Talk.*","prism-google-talk","prism-google-talk"},
  {NULL,NULL,NULL,NULL}
};

/*
 cmd, res name, class name, window title, id
 */
static  WindowMatch window_regexes[] = 
{
  /*Do not bother trying to parse an open office command line for the type of window*/
  {".*prism.*google.*calendar.*","Prism","Navigator",".*[Cc]alendar.*","prism-google-calendar"},
  {".*prism.*google.*analytics.*","Prism","Navigator",".*[Aa]nalytics.*","prism-google-analytics"},
  {".*prism.*google.*docs.*","Prism","Navigator",".*[Dd]ocs.*","prism-google-docs"},
  {".*prism.*google.*groups.*","Prism","Navigator",".*[Gg]roups.*","prism-google-groups"},
  {".*prism.*google.*mail.*","Prism","Navigator",".*[Mm]ail.*","prism-google-mail"},
  {".*prism.*google.*reader.*","Prism","Navigator",".*[Rr]eader.*","prism-google-reader"},
  {".*prism.*google.*talk.*","Prism","Navigator",".*[Tt]alk.*","prism-google-talk"},
  {".*office.*",".*OpenOffice.*",".*VCLSalFrame.*DocumentWindow.*",".*Writer.*","OpenOffice-Writer"},
  {".*office.*",".*OpenOffice.*",".*VCLSalFrame.*DocumentWindow.*",".*Draw.*","OpenOffice-Draw"},
  {".*office.*",".*OpenOffice.*",".*VCLSalFrame.*DocumentWindow.*",".*Impress.*","OpenOffice-Impress"},
  {".*office.*",".*OpenOffice.*",".*VCLSalFrame.*DocumentWindow.*",".*Calc.*","OpenOffice-Calc"},
  {NULL,"Amsn","amsn",".*aMSN.*","aMSN"},
  {NULL,"Chatwindow","container.*",".*Buddies.*Chat.*","aMSN"},
  {NULL,"Chatwindow","container.*",".*Untitled.*[wW]indow.*","aMSN"},  
  {NULL,"Toplevel","cfg",".*Preferences.*-.*Config.*","aMSN"},
  {NULL,"Toplevel","plugin_selector",".*Select.*Plugins.*","aMSN"},
  {NULL,"Toplevel","skin_selector",".*Please.*select.*skin.*","aMSN"},
  {NULL,"Toplevel","eventlog_hist",".*History.*eventlog.*","aMSN"},
  {NULL,"Toplevel","alarm_cfg.*",".*Alarm.*settings.*contact.*","aMSN"},
  {NULL,"Toplevel","dpbrowser",".*Display.*Pictures.*Browser.*","aMSN"},
  {NULL,"Toplevel","change_name",".*Change.*Nick.*aMSN.*","aMSN"},  
  {NULL,"Toplevel","_listchoose","Send.*File","aMSN"},
  {NULL,"Toplevel","_listchoose","Send.*Message","aMSN"},
  {NULL,"Toplevel","_listchoose","Send.*to.*Mobile.*Device","aMSN"},
  {NULL,"Toplevel","_listchoose","Send.*E-mail","aMSN"},
  {NULL,"Toplevel","_listchoose","Send.*Webcam","aMSN"},
  {NULL,"Toplevel","_listchoose","Ask.*to.*Receive.*Webcam","aMSN"},
  {NULL,"Toplevel","globalnick","Global.*Nickname","aMSN"},
  {NULL,"Toplevel","addcontact","Add.*Contact.*aMSN","aMSN"},
  {NULL,"Toplevel","_listchoose","^Delete$","aMSN"},
  {NULL,"Toplevel","_listchoose","^Properties$","aMSN"},
  {NULL,"Toplevel","_listchoose","^Properties$","aMSN"},
  {NULL,"Toplevel","^dlgag$","^Add.*Group$","aMSN"},
  {NULL,"Toplevel",".*_hist$","^History.*","aMSN"},
  {NULL,"Toplevel","savecontacts","^Options$","aMSN"},  
  {NULL,NULL,NULL,NULL,NULL}
};

/*
 cmd, res name, class name, title, desktop
 */
static  WindowToDesktopMatch window_to_desktop_regexes[] = 
{
  /*Do not bother trying to parse an open office command line for the type of window*/
  {".*prism.*google.*calendar.*","Prism","Navigator",".*[Cc]alendar.*","prism-google-calendar"},
  {".*prism.*google.*analytics.*","Prism","Navigator",".*[Aa]nalytics.*","prism-google-analytics"},
  {".*prism.*google.*docs.*","Prism","Navigator",".*[Dd]ocs.*","prism-google-docs"},
  {".*prism.*google.*groups.*","Prism","Navigator",".*[Gg]roups.*","prism-google-groups"},
  {".*prism.*google.*mail.*","Prism","Navigator",".*[Mm]ail.*","prism-google-mail"},
  {".*prism.*google.*reader.*","Prism","Navigator",".*[Rr]eader.*","prism-google-reader"},
  {".*prism.*google.*talk.*","Prism","Navigator",".*[Tt]alk.*","prism-google-talk"},
  {".*office.*",".*OpenOffice.*",".*VCLSalFrame.*DocumentWindow.*",".*Writer.*","ooo-writer"},
  {".*office.*",".*OpenOffice.*",".*VCLSalFrame.*DocumentWindow.*",".*Draw.*","ooo-draw"},
  {".*office.*",".*OpenOffice.*",".*VCLSalFrame.*DocumentWindow.*",".*Impress.*","ooo-impress"},
  {".*office.*",".*OpenOffice.*",".*VCLSalFrame.*DocumentWindow.*",".*Calc.*","ooo-calc"},
  {".*gimp.*",".*Gimp.*",".*gimp.*",".*GNU.*Image.*Manipulation.*Program.*","gimp"},
  {".*system-config-printer.*applet.*py.*",".*Applet.*py.*",".*applet.*",".*Print.*Status.*","redhat-manage-print-jobs"},  
  {".*amsn","Amsn","amsn",".*aMSN.*","amsn"},
  {NULL,"Chatwindow","container.*",".*Buddies.*Chat.*","amsn"},
  {NULL,"Chatwindow","container.*",".*Untitled.*window.*","amsn"},  
  {".*linuxdcpp","Linuxdcpp","linuxdcpp","LinuxDC\\+\\+","dc++"},    
  {".*thunderbird-bin","Thunderbird-bin","gecko",".*Thunderbird.*","thunderbird"},    
  {NULL,NULL,NULL,NULL,NULL}
};

/*
 Special Casing should NOT be used for anything but a last resort.  
 Other matching algororithms are NOT used if something is special cased.
*/
gchar *
get_special_id_from_desktop (DesktopAgnosticFDODesktopEntry * entry)
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
      gchar * exec = desktop_agnostic_fdo_desktop_entry_get_string (entry, "Exec");
#ifdef DEBUG      
      g_debug ("%s: iter->exec = %s, exec = %s",__func__,iter->exec,exec);
#endif
      match = g_regex_match_simple(iter->exec, exec,0,0);
      g_free (exec);
      if (!match)
        continue;
    }
    if (iter->name)
    {
      gchar * name = desktop_agnostic_fdo_desktop_entry_get_name (entry);
#ifdef DEBUG      
      g_debug ("%s: iter->name = %s, name = %s",__func__,iter->name,name);      
#endif
      match = g_regex_match_simple(iter->name, name,0,0);
      g_free (name);
      if (!match)
        continue;
    }
    if (iter->filename)
    {
      DesktopAgnosticVFSFile *file = desktop_agnostic_fdo_desktop_entry_get_file (entry);
      gchar *filename = desktop_agnostic_vfs_file_get_path (file);
#ifdef DEBUG      
      g_debug ("%s: iter->filename = %s, filename = %s",__func__,iter->filename,filename);
#endif
      match = g_regex_match_simple(iter->filename, filename,0,0);
      g_free (filename);
      if (!match)
        continue;
    }
#ifdef DEBUG
    g_debug ("%s:  Special cased ID: '%s'",__func__,iter->id);
#endif
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
#ifdef DEBUG      
      g_debug ("%s: iter->cmd = %s, cmd = %s",__func__,iter->cmd,cmd);
#endif
    if (iter->cmd)
    {
      match = cmd && g_regex_match_simple(iter->cmd, cmd,0,0);
      if (!match)
        continue;
    }
#ifdef DEBUG      
      g_debug ("%s: iter->res_name = %s, res_name = %s",__func__,iter->res_name,res_name);
#endif    
    if (iter->res_name)
    {
      match = res_name && g_regex_match_simple(iter->res_name, res_name,0,0); 
      if (!match)
        continue;
    }
#ifdef DEBUG      
      g_debug ("%s: iter->class_name = %s, class_name = %s",__func__,iter->class_name,class_name);
#endif
    if (iter->class_name)
    {
      match = class_name && g_regex_match_simple(iter->class_name, class_name,0,0);
      if (!match)
        continue;
    } 
#ifdef DEBUG      
      g_debug ("%s: iter->title = %s, title = %s",__func__,iter->title,title);
#endif
    if (iter->title)
    {
      match = title && g_regex_match_simple(iter->title, title,0,0);
      if (!match)
        continue;
    } 
#ifdef DEBUG    
    g_debug ("%s:  Special cased Window ID: '%s'",__func__,iter->id);
#endif
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
#ifdef DEBUG    
    g_debug ("%s:  Special cased desktop: '%s'",__func__,iter->desktop);
#endif
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

gboolean 
check_if_blacklisted (gchar * name)
{
  const gchar ** iter;
  for (iter = blacklist; *iter;iter++)
  {
    if (g_strcmp0 (name,*iter) == 0)
    {
      return TRUE;
    }
  }
  return FALSE;
}