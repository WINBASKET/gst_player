#include <stdio.h>
#include <gst/gst.h>
#include <gio/gio.h>



typedef struct _CustomData
{
  GstElement *pipeline;
  GMainLoop *loop;

} CustomData;

static void bus_message_cb (GstBus * bus, GstMessage * msg, CustomData * data);
static void keyboard_message_cb(GIOChannel *source, GIOCondition cond, CustomData *data);


int 
main (int argc, char *argv[])
{
    GstElement *pipeline;
    GstBus *bus;
    GstMessage *msg;

    GstFlowReturn ret;
    GIOChannel *io_stdin;

    CustomData data;

    g_print("start test app\n");

    /* Initialize our data structure */
    memset (&data, 0, sizeof (data));

    gst_init (&argc, &argv);
 
    /* Build the pipeline */
    data.pipeline = gst_parse_launch
    //("playbin uri=https://www.freedesktop.org/software/gstreamer-sdk/data/media/sintel_trailer-480p.webm",
    ("playbin uri=rtsp://admin:admin@192.168.30.220:554/1/stream2/Profile1",
    
    NULL);

     
      /* Add a keyboard watch so we get notified of keystrokes */
#ifdef G_OS_WIN32
  io_stdin = g_io_channel_win32_new_fd (fileno (stdin));
#else
  io_stdin = g_io_channel_unix_new(fileno(stdin));
#endif

  g_io_add_watch(io_stdin, G_IO_IN, (GIOFunc)keyboard_message_cb, &data); 
    
g_printerr ("1\n");

    bus = gst_element_get_bus (data.pipeline);
    gst_bus_add_signal_watch (bus);
    g_signal_connect (bus, "message", G_CALLBACK (bus_message_cb), &data);
    gst_object_unref (bus);
    
     /* Start playing */
    ret = gst_element_set_state (data.pipeline, GST_STATE_PLAYING);

    if (ret == GST_STATE_CHANGE_FAILURE) {
        g_printerr ("Unable to set the pipeline to the playing state.\n");
        gst_object_unref (data.pipeline);
    }
    
g_printerr ("2\n");

    data.loop = g_main_loop_new (NULL, FALSE);
    g_main_loop_run (data.loop);

g_printerr ("3\n");    
    gst_element_set_state (data.pipeline, GST_STATE_NULL);
    g_clear_object (&data.pipeline);
    g_io_channel_unref(io_stdin);
g_printerr ("4\n");
    return 0;
}

static void keyboard_message_cb(GIOChannel *source, GIOCondition cond, CustomData *data)
{
    gchar *str = NULL;

    while(g_io_channel_read_line (source, &str, NULL, NULL, NULL) == G_IO_STATUS_NORMAL) {
       
        int index = g_ascii_strtoull(str, NULL, 0);

        g_print("input = %d\n", index);

        switch (index) {

            case 1:
                gst_element_set_state (data->pipeline, GST_STATE_PAUSED);
                
            break;
            case 2:
                gst_element_set_state (data->pipeline, GST_STATE_PLAYING);
                g_main_loop_quit (data->loop);
            break;
            
            case 3:
                 g_print("key : end value \n");
                 g_main_loop_quit (data->loop);
            break;  

        if(index >= 3)
            break;

        }
        
    }
    g_free(str);
}

static void
bus_message_cb (GstBus * bus, GstMessage * msg, CustomData * data)
{

  switch (GST_MESSAGE_TYPE (msg)) {

    case GST_MESSAGE_ERROR:{
      GError *err;
      gchar *debug;

      gst_message_parse_error (msg, &err, &debug);
      g_print ("Error: %s\n", err->message);
      g_error_free (err);
      g_free (debug);

      gst_element_set_state (data->pipeline, GST_STATE_READY);
      g_main_loop_quit (data->loop);
      break;
    }

    case GST_MESSAGE_STATE_CHANGED:{
      if (GST_MESSAGE_SRC (msg) == GST_OBJECT (data->pipeline)) {
        GstState old_state, new_state, pending_state;
        gst_message_parse_state_changed (msg, &old_state, &new_state,
            &pending_state);
        g_print ("Pipeline state changed from %s to %s:\n",
            gst_element_state_get_name (old_state),
            gst_element_state_get_name (new_state));
      }
      break;
    }

    case GST_MESSAGE_EOS: {
        g_print("state changed to eos \n");
        g_main_loop_quit (data->loop);
    }
      break;
    default:
      break;
  }
}