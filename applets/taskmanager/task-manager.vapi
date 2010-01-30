/* task-manager.vapi stub (manually generated) */

[CCode (type_id = "TASK_TYPE_MANAGER", cheader_filename = "task-manager.h")]
public class TaskManager : Awn.Applet, Atk.Implementor, Gtk.Buildable {
  [CCode (type = "AwnApplet*", has_construct_function = false)]
  public TaskManager (string name, string uid, int panel_id);

  /*[CCode (array_length = false, array_null_terminated = true)]
  public string[] get_gstrv ();*/
}
