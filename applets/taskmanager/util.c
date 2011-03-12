/* 
 * Copyright (C) 2009 Rodney Cryderman <rcryderman@gmail.com> 
 *  
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor Boston, MA 02110-1301,  USA
 *
 */

#include <glib.h>
#undef G_DISABLE_SINGLE_INCLUDES
#include <glibtop/procargs.h>
#include <glibtop/procuid.h>

#include "util.h"


//#define DEBUG 1


/*
 TODO: Split/cleanup this and related code in taskmanager-c into a separater 
      object/lib once it comes time to do a caching of results + tracking of app 
      use (early in 0.6).  
  
      The various uses are kind of obvious.  Such as use by shinyswitcher. And 
      use your imagination.
 
      Special casing info should be moved into separate datafiles/db.
 
      Tool to analyze and special case windows by advanced users ala xprop 
      (point and click) and analyze.

      For 0.6:

        start making use of WM_WINDOW_ROLE when it is available
        Make use of _NET_WM_ICON_NAME
        Make use of _NET_WM_NAME
 
 
 */

static gchar * generate_id_from_cmd(gchar *cmd,gchar *res_name,
                                    gchar *class_name, gchar*title);

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
  const void * id;
}WindowMatch;

typedef struct
{
  const gchar * cmd;
  const gchar * res_name;
  const gchar * class_name;
  const gchar * title;
  const gchar * desktop;
}WindowToDesktopMatch;


typedef struct
{
  const gchar * res_name;
  const gchar * class_name;
  const gchar * title;
  guint wait;
}WindowWait;

typedef struct
{
  const gchar * cmd;
  const gchar * res_name;
  const gchar * class_name;
  const gchar * title;
  const WinIconUse  use;
}IconUse;


const gchar * blacklist[] = {"prism",
                       NULL};

const gchar * no_display_override_list[] = {"nautilus.desktop",
                                             NULL};

typedef gchar *(*fn_gen_id)(const gchar *,const gchar*,const gchar*,const gchar*);


/*Assign an id to a desktop file
 
 exec field,name field,desktop filename,id
 */
static DesktopMatch desktop_regexes[] = 
{
  {".*eclipse","[Ee]clipse","eclipse","Eclipse"},  
  {".*ooffice.*-writer.*",NULL,NULL,"OpenOffice-Writer"},
  {".*ooffice.*-draw.*",NULL,NULL,"OpenOffice-Draw"},
  {".*ooffice.*-impress.*",NULL,NULL,"OpenOffice-Impress"},
  {".*ooffice.*-calc.*",NULL,NULL,"OpenOffice-Calc"},
  {".*ooffice.*-math.*",NULL,NULL,"OpenOffice-Math"},
  {".*ooffice.*-base.*",NULL,NULL,"OpenOffice-Base"},  

  {".*libre.*-writer.*",NULL,NULL,"LibreOffice-Writer"},
  {".*libre.*-draw.*",NULL,NULL,"LibreOffice-Draw"},
  {".*libre.*-impress.*",NULL,NULL,"LibreOffice-Impress"},
  {".*libre.*-calc.*",NULL,NULL,"LibreOffice-Calc"},
  {".*libre.*-math.*",NULL,NULL,"LibreOffice-Math"},
  {".*libre.*-base.*",NULL,NULL,"LibreOffice-Base"},  
  
  {".*amsn.*","aMSN",".*amsn.*desktop.*","aMSN"},
  {".*prism-google-calendar",".*Google.*Calendar.*","prism-google-calendar","prism-google-calendar"},
  {".*prism-google-analytics",".*Google.*Analytics.*","prism-google-analytics","prism-google-analytics"},
  {".*prism-google-docs",".*Google.*Docs.*","prism-google-docs","prism-google-docs"},
  {".*prism-google-groups",".*Google.*Groups.*","prism-google-groups","prism-google-groups"},
  {".*prism-google-mail",".*Google.*Mail.*","prism-google-mail","prism-google-mail"},
  {".*prism-google-reader",".*Google.*Reader.*","prism-google-reader","prism-google-reader"},
  {".*prism-google-talk",".*Google.*Talk.*","prism-google-talk","prism-google-talk"},
  {"TERMINATOR",NULL,NULL,NULL}
};

/*
 cmd, res name, class name, window title, id
 */
static  WindowMatch window_regexes[] = 
{
  {".*eclipse","\\.","\\.",NULL,"Eclipse"},    
  {NULL,"[eE]clipse","[eE]clipse",NULL,"Eclipse"},    
  /*Do not bother trying to parse an open office command line for the type of window*/
  {".*prism.*google.*calendar.*","Prism","Navigator",".*[Cc]alendar.*","prism-google-calendar"},
  {".*prism.*google.*analytics.*","Prism","Navigator",".*[Aa]nalytics.*","prism-google-analytics"},
  {".*prism.*google.*docs.*","Prism","Navigator",".*[Dd]ocs.*","prism-google-docs"},
  {".*prism.*google.*groups.*","Prism","Navigator",".*[Gg]roups.*","prism-google-groups"},
  {".*prism.*google.*mail.*","Prism","Navigator",".*[Mm]ail.*","prism-google-mail"},
  {".*prism.*google.*reader.*","Prism","Navigator",".*[Rr]eader.*","prism-google-reader"},
  {".*prism.*google.*talk.*","Prism","Navigator",".*[Tt]alk.*","prism-google-talk"},

  {NULL,"Prism","Webrunner",NULL,generate_id_from_cmd},
  
  {".*office.*",".*OpenOffice.*",".*VCLSalFrame.*",".*Writer.*","OpenOffice-Writer"},
  {".*office.*",".*OpenOffice.*",".*VCLSalFrame.*",".*Draw.*","OpenOffice-Draw"},
  {".*office.*",".*OpenOffice.*",".*VCLSalFrame.*",".*Impress.*","OpenOffice-Impress"},
  {".*office.*",".*OpenOffice.*",".*VCLSalFrame.*",".*Calc.*","OpenOffice-Calc"},
  {".*office.*",".*OpenOffice.*",".*VCLSalFrame.*",".*Math.*","OpenOffice-Math"},
  {".*office.*",".*OpenOffice.*",".*VCLSalFrame.*",".*Base.*","OpenOffice-Base"},
  {".*office.*",".*OpenOffice.*",".*VCLSalFrame.*","^Database.*Wizard$","OpenOffice-Base"},      

  {".*office.*",".*LibreOffice.*",".*VCLSalFrame.*",".*Writer.*","LibreOffice-Writer"},
  {".*office.*",".*LibreOffice.*",".*VCLSalFrame.*",".*Draw.*","LibreOffice-Draw"},
  {".*office.*",".*LibreOffice.*",".*VCLSalFrame.*",".*Impress.*","LibreOffice-Impress"},
  {".*office.*",".*LibreOffice.*",".*VCLSalFrame.*",".*Calc.*","LibreOffice-Calc"},
  {".*office.*",".*LibreOffice.*",".*VCLSalFrame.*",".*Math.*","LibreOffice-Math"},
  {".*office.*",".*LibreOffice.*",".*VCLSalFrame.*",".*Base.*","LibreOffice-Base"},
  {".*office.*",".*LibreOffice.*",".*VCLSalFrame.*","^Database.*Wizard$","LibreOffice-Base"},      

  {NULL,"Amsn","amsn",".*aMSN.*","aMSN"},
  {NULL,"Chatwindow","container.*",".*Buddies.*Chat.*","aMSN"},
  {NULL,"Chatwindow","container.*",".*Untitled.*[wW]indow.*","aMSN"},
  {NULL,"Chatwindow","container.*",".*Offline.*Messaging.*","aMSN"},  
  {NULL,"Chatwindow","container.*",NULL,"aMSN"},
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
  {"TERMINATOR",NULL,NULL,NULL,NULL}
};

/*
 cmd, res name, class name, title, desktop
 */
static  WindowToDesktopMatch window_to_desktop_regexes[] = 
{
  {".*eclipse.*",".*",".*","eclipse","eclipse"},
  /*Do not bother trying to parse an open office command line for the type of window*/  
  {".*prism.*google.*calendar.*","Prism","Navigator",".*[Cc]alendar.*","prism-google-calendar"},
  {".*prism.*google.*analytics.*","Prism","Navigator",".*[Aa]nalytics.*","prism-google-analytics"},
  {".*prism.*google.*docs.*","Prism","Navigator",".*[Dd]ocs.*","prism-google-docs"},
  {".*prism.*google.*groups.*","Prism","Navigator",".*[Gg]roups.*","prism-google-groups"},
  {".*prism.*google.*mail.*","Prism","Navigator",".*[Mm]ail.*","prism-google-mail"},
  {".*prism.*google.*reader.*","Prism","Navigator",".*[Rr]eader.*","prism-google-reader"},
  {".*prism.*google.*talk.*","Prism","Navigator",".*[Tt]alk.*","prism-google-talk"},

  /*Debian*/
  {".*office.*",".*OpenOffice.*",".*VCLSalFrame.*",".*Writer.*","openoffice.org-writer"},
  {".*office.*",".*OpenOffice.*",".*VCLSalFrame.*",".*Draw.*","openoffice.org-draw"},
  {".*office.*",".*OpenOffice.*",".*VCLSalFrame.*",".*Impress.*","openoffice.org-impress"},
  {".*office.*",".*OpenOffice.*",".*VCLSalFrame.*",".*Calc.*","openoffice.org-calc"},
  {".*office.*",".*OpenOffice.*",".*VCLSalFrame.*",".*Math.*","openoffice.org-math"},
  {".*office.*",".*OpenOffice.*",".*VCLSalFrame.*",".*Base.*","openoffice.org-base"},
  {".*office.*",".*OpenOffice.*",".*VCLSalFrame.*","^Database.*Wizard$","openoffice.org-base"},
  
    /*Ubuntu*/
  {".*office.*",".*OpenOffice.*",".*VCLSalFrame.*",".*Writer.*","ooo-writer"},
  {".*office.*",".*OpenOffice.*",".*VCLSalFrame.*",".*Draw.*","ooo-draw"},
  {".*office.*",".*OpenOffice.*",".*VCLSalFrame.*",".*Impress.*","ooo-impress"},
  {".*office.*",".*OpenOffice.*",".*VCLSalFrame.*",".*Calc.*","ooo-calc"},
  {".*office.*",".*OpenOffice.*",".*VCLSalFrame.*",".*Math.*","ooo-math"},
  {".*office.*",".*OpenOffice.*",".*VCLSalFrame.*",".*Base.*","ooo-base"},  
  {".*office.*",".*OpenOffice.*",".*VCLSalFrame.*","^Database.*Wizard$","ooo-base"},
  
  {".*office.*",".*LibreOffice.*",".*VCLSalFrame.*",".*Writer.*","libreoffice3-writer"},
  {".*office.*",".*LibreOffice.*",".*VCLSalFrame.*",".*Draw.*","libreoffice3-draw"},
  {".*office.*",".*LibreOffice.*",".*VCLSalFrame.*",".*Impress.*","libreoffice3-impress"},
  {".*office.*",".*LibreOffice.*",".*VCLSalFrame.*",".*Calc.*","libreoffice3-calc"},
  {".*office.*",".*LibreOffice.*",".*VCLSalFrame.*",".*Math.*","libreoffice3-math"},
  {".*office.*",".*LibreOffice.*",".*VCLSalFrame.*",".*Base.*","libreoffice3-base"},  
  {".*office.*",".*LibreOffice.*",".*VCLSalFrame.*","^Database.*Wizard$","libreoffice3-base"},

  {".*gimp.*",".*Gimp.*",".*gimp.*",".*GNU.*Image.*Manipulation.*Program.*","gimp"},
  {".*system-config-printer.*applet.*py.*",".*Applet.*py.*",".*applet.*",".*Print.*Status.*","redhat-manage-print-jobs"},  
  {".*amsn","Amsn","amsn",".*aMSN.*","amsn"},
  {NULL,"Chatwindow","container.*",".*Buddies.*Chat.*","amsn"},
  {NULL,"Chatwindow","container.*",".*Untitled.*window.*","amsn"},  
  {".*linuxdcpp","Linuxdcpp","linuxdcpp","LinuxDC\\+\\+","dc++"},    
  {NULL,"tvtime","TVWindow","^tvtime","net-tvtime"},
  {NULL,"VirtualBox",NULL,".*VirtualBox.*","virtualbox-ose"},
  {NULL,"VirtualBox",NULL,".*VirtualBox.*","virtualbox"},
  {NULL,"[Nn]autilus","[Nn]autilus",NULL,"nautilus"},
  {NULL,"[Nn]autilus","[Nn]autilus",NULL,"nautilus-browser"},
  {NULL,"[Nn]autilus","[Nn]autilus",NULL,"nautilus-home"},
  {NULL,NULL,NULL,"Moovida.*Media.*Cent.*","moovida"},
  {"TERMINATOR",NULL,NULL,NULL,NULL}
};

static  WindowWait windows_to_wait[] = 
{
  {".*OpenOffice.*",".*VCLSalFrame.*","^OpenOffice\\.org.*",1000},
  {".*LibreOffice.*",".*VCLSalFrame.*","^LibreOffice.*",1000},
  {"TERMINATOR",NULL,NULL,0}
};


/*
 Only set something to USE_NEVER if the app sets it to something truly, truly,
 ugly (There are multiple bug reports about just how ugly it is ), as this will
 override the display of the app window icon even when the user has configured
 taskman to always use them.  USE_ALWAYS is disregarded (for overlays) if the 
 icons are sufficiently similar.
 */
static  IconUse icon_regexes[] = 
{
  {NULL,".*OpenOffice.*",".*VCLSalFrame.*",NULL,USE_NEVER},
  {NULL,".*LibreOffice.*",".*VCLSalFrame.*",NULL,USE_NEVER},
  {NULL,"Pidgin","pidgin",NULL,USE_ALWAYS},
  {".*gimp.*",".*Gimp.*",".*gimp.*",NULL,USE_ALWAYS},  
  {NULL,NULL,NULL,NULL,USE_DEFAULT}
};


static gchar *
generate_id_from_cmd(gchar *cmd,gchar *res_name,gchar *class_name, gchar*title)
{
  if (cmd)
  {
    return g_strdup (cmd);
  }
  return NULL;
}

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
      gchar * exec = NULL;
      if (desktop_agnostic_fdo_desktop_entry_key_exists (entry,"Exec"))
      {
        exec = desktop_agnostic_fdo_desktop_entry_get_string (entry, "Exec");
      }
      if (!exec)
      {
        continue;
      }
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
      gchar * name = NULL;
      /*We do not want localized values*/
      if (desktop_agnostic_fdo_desktop_entry_key_exists (entry,"Name"))
      {
        name = desktop_agnostic_fdo_desktop_entry_get_string (entry, "Name");
      }
      else
      {
        continue;
      }
        
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
  g_assert ( g_strcmp0 (iter->exec,"TERMINATOR")==0);
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
    g_debug ("%s:  Special cased Window ID: '%s'",__func__,(gchar *)iter->id);
#endif

    if ( iter->id && (iter->id != generate_id_from_cmd) )
    {
      return g_strdup (iter->id);
    }
    else if (iter->id == generate_id_from_cmd)
    {
      fn_gen_id fn = iter->id;
      /*conditional operator*/
      return fn(iter->cmd,iter->res_name,iter->class_name,iter->title);
    }
    
  }
  g_assert ( g_strcmp0 (iter->cmd,"TERMINATOR")==0);
  return NULL;
}

GSList *
get_special_desktop_from_window_data (gchar * cmd, gchar *res_name, gchar * class_name,const gchar *title)
{
  /*
   Exec,Name,filename, special_id.  If all in the first 3 match then the 
   special_id is returned.

   TODO  put data into a separate file
   
   TODO  optimize the regex handling.
   */
  GSList * result=NULL;
  WindowToDesktopMatch  *iter;
#ifdef DEBUG
  g_debug ("%s: cmd = '%s', res = '%s', class = '%s', title = '%s'",__func__,cmd,res_name,class_name,title);
  g_debug ("%p, %p", window_to_desktop_regexes,window_to_desktop_regexes->desktop);
#endif
  for (iter = window_to_desktop_regexes; iter->desktop; iter++)
  {
    gboolean  match = TRUE;
    if (iter->cmd)
    {
#ifdef DEBUG
      g_debug ("%s: ite 10835r->cmd = %s, cmd = %s",__func__,iter->cmd,cmd);
#endif
      match = cmd && g_regex_match_simple(iter->cmd, cmd,0,0);
      if (!match)
        continue;
    }
    if (iter->res_name)
    {
#ifdef DEBUG
      g_debug ("%s: iter->res_name = %s, res_name = %s",__func__,iter->res_name,res_name);
#endif
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
    g_debug ("%s:  Special cased desktop: '%s'",__func__,iter->desktop);
#endif
    result = g_slist_append (result, (gchar*)iter->desktop);
  }
  g_assert ( g_strcmp0 (iter->cmd,"TERMINATOR")==0);
  return result;
}

gboolean
get_special_wait_from_window_data (gchar *res_name, gchar * class_name,const gchar *title)
{
  /*
   Exec,Name,filename, special_id.  If all in the first 3 match then the 
   special_id is returned.

   TODO  put data into a separate file
   
   TODO  optimize the regex handling.
   */
  WindowWait  *iter;
  if (!res_name && !class_name)
  {
    return TRUE;
  }
  
  for (iter = windows_to_wait; iter->wait; iter++)
  {
    gboolean  match = TRUE;
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
    g_debug ("%s:  Special Wait Window ID:",__func__);
#endif
    return TRUE;
  }
  g_assert ( g_strcmp0 (iter->res_name,"TERMINATOR")==0);
  return FALSE;
}

WinIconUse
get_win_icon_use (gchar * cmd,gchar *res_name, gchar * class_name,const gchar *title)
{
  IconUse  *iter;
  for (iter = icon_regexes; iter->use != USE_DEFAULT; iter++)
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
    g_debug ("%s: setting to %d for %s",__func__,iter->use,title);
#endif
    return iter->use;
  }
  return USE_DEFAULT;
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
check_no_display_override (const gchar * fname)
{
  const gchar ** iter;
  for (iter = no_display_override_list; *iter;iter++)
  {
    if (g_strcmp0 (fname,*iter) == 0)
    {
      return TRUE;
    }
  }
  return FALSE;

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

static gdouble
compute_mse (GdkPixbuf *i1, GdkPixbuf *i2)
{
  int i, j;
  int width, height, row_stride, has_alpha;
  guchar *i1_pixels, *i2_pixels;
  gdouble result = 0.0;

  g_return_val_if_fail (GDK_IS_PIXBUF (i1) && GDK_IS_PIXBUF (i2), 0.0);

  has_alpha = gdk_pixbuf_get_has_alpha(i1);
  width = gdk_pixbuf_get_width (i1);
  height = gdk_pixbuf_get_height (i1);
  row_stride = gdk_pixbuf_get_rowstride (i1);

  g_return_val_if_fail (
    has_alpha == gdk_pixbuf_get_has_alpha (i2) &&
    width == gdk_pixbuf_get_width (i2) &&
    height == gdk_pixbuf_get_height (i2) &&
    row_stride == gdk_pixbuf_get_rowstride (i2),
    0.0
  );

  i1_pixels = gdk_pixbuf_get_pixels (i1);
  i2_pixels = gdk_pixbuf_get_pixels (i2);

  for (i = 0; i < height; i++)
  {
    guchar *it1, *it2;
    it1 = i1_pixels + i * row_stride;
    it2 = i2_pixels + i * row_stride;
    for (j = 0; j < width; j++)
    {
      gdouble inc = 0.0;
      gint delta_r = *(it1++);
      delta_r -= *(it2++);
      gint delta_g = *(it1++);
      delta_g -= *(it2++);
      gint delta_b = *(it1++);
      delta_b -= *(it2++);
      inc += delta_r * delta_r + delta_g * delta_g + delta_b * delta_b;

      if (has_alpha)
      {
        gint delta_alpha = *it1 - *it2;
        inc += delta_alpha * delta_alpha;
        if (abs(delta_alpha) <= 10 && *it1 <= 10)
        {
          // alpha and alpha difference is very small - don't sum up this pixel
          it1++; it2++;
          continue;
        }
        it1++; it2++;
      }
      result += inc;
    }
  }

  return result / width / height / (has_alpha ? 4 : 3);
}

static gdouble
compute_psnr (gdouble MSE, gint max_val)
{
  return 10 * log10 (max_val * max_val / MSE);
}

gboolean
utils_gdk_pixbuf_similar_to (GdkPixbuf *i1, GdkPixbuf *i2)
{
  gdouble MSE = compute_mse (i1, i2);

  if (MSE < 0.01)
  {
#ifdef DEBUG
    g_debug ("Same images...");
#endif
    return TRUE;
  }

  gdouble PSNR = compute_psnr (MSE, 255);
#ifdef DEBUG
  g_debug ("PSNR: %g", PSNR);
#endif
  return PSNR >= 11;
}

gboolean
usable_desktop_entry (  DesktopAgnosticFDODesktopEntry * entry)
{
  // FIXME: add support for Link-type entries, not just Applications
  if (  !desktop_agnostic_fdo_desktop_entry_key_exists (entry, "Name")
      ||
        !desktop_agnostic_fdo_desktop_entry_key_exists (entry, "Exec") )
  {
    return FALSE;
  }
  return TRUE;
}

gboolean
usable_desktop_file_from_path ( const gchar * path)
{
  DesktopAgnosticVFSFile *file;
  GError *error = NULL;
  DesktopAgnosticFDODesktopEntry * entry;
  
  file = desktop_agnostic_vfs_file_new_for_path (path, &error);

  if (error)
  {
    g_critical ("Error when trying to load the launcher: %s", error->message);
    g_error_free (error);
    return FALSE;
  }

  if (file == NULL || !desktop_agnostic_vfs_file_exists (file))
  {
    if (file)
    {
      g_object_unref (file);
    }
    g_critical ("File not found: '%s'", path);
    return FALSE;
  }

  entry = desktop_agnostic_fdo_desktop_entry_new_for_file (file, &error);
  
  if (error)
  {
    g_critical ("Error when trying to load the launcher: %s", error->message);
    g_error_free (error);
    g_object_unref (file);    
    return FALSE;
  }

  if (!usable_desktop_entry (entry) )
  {
    g_object_unref (entry);
    return FALSE;
  }
  g_object_unref (entry);
  return TRUE;
}

char* _desktop_entry_get_localized_name (DesktopAgnosticFDODesktopEntry *entry)
{
  char *result;

  result = desktop_agnostic_fdo_desktop_entry_get_localestring (entry,
                                                                "Name",
                                                                NULL);

  // workaround for older versions of lda which don't support NULL locale param
  if (result == NULL)
  {
    const gchar* const * languages = g_get_language_names ();
    int i = 0;

    while (result == NULL && languages[i] != NULL)
    {
      result =
        desktop_agnostic_fdo_desktop_entry_get_localestring (entry,
                                                             "Name",
                                                             languages[i]);
      i++;
    }
  }

  return result ? result : desktop_agnostic_fdo_desktop_entry_get_name (entry);
}

