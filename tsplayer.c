/* test source


*/

#include <stdio.h>
#include <gst/gst.h>
#include <gio/gio.h>

#define SRT_URI "srt://127.0.0.1:8888?mode=listener"

typedef struct _CustomData
{
  GstElement *pipeline;
  GMainLoop *loop;

} CustomData;

static void bus_message_cb (GstBus * bus, GstMessage * msg, CustomData * data);
static void keyboard_message_cb(GIOChannel *source, GIOCondition cond, CustomData *data);
static GstFlowReturn new_sample_cb (GstElement * sink, CustomData * data);


// --------------------------------------------------------------------------------------
//                                     buffer Manage                                         
// --------------------------------------------------------------------------------------
/* The appsink has received a buffer */
static GstFlowReturn
new_sample_cb (GstElement *sink, CustomData *data)
{
  GstSample *sample;
  static long signed int nTotallen = 0;
g_print("-");
  /* Retrieve the buffer */
  g_signal_emit_by_name (sink, "pull-sample", &sample);

  if (sample) {
    GstBuffer *buffer;
    GstMemory *memory;
    GstMapInfo map_info;

    /* refer to API document to handle byte stream
     *   https://gstreamer.freedesktop.org/documentation/gstreamer/gstbuffer.html?gi-language=c
     */
    buffer = gst_sample_get_buffer (sample);
    memory = gst_buffer_get_all_memory (buffer);



    if (gst_memory_map (memory, &map_info, GST_MAP_READ)) {

      gssize wlen;

      /*
       * map_info.data(guint8 *):  a pointer to the mapped data
       * map_info.size(gsize)   :  the valid size in data
       */

     // wlen = g_output_stream_write (G_OUTPUT_STREAM (data->ostream),
     //     map_info.data, map_info.size, NULL, NULL);
//nTotallen += map_info.size;
      g_print ("received %lu bytes\n", map_info.size);

      gst_memory_unmap (memory, &map_info);
    } else {
      ;// failed to get memory info
    }

    gst_memory_unref (memory);
    gst_sample_unref (sample);

    return GST_FLOW_OK;
  }
}

// --------------------------------------------------------------------------------------
//                                     Main                                         
// -------------------------------------------------------------------------------------- 
int 
main (int argc, char *argv[])
{
    GstBus *bus;
    GstMessage *msg;

    GstFlowReturn ret;
    GIOChannel *io_stdin;

    GstElement *src;
    GstElement *sink;

    CustomData data;

    g_print("start app\n");

    /* Initialize our data structure */
    memset (&data, 0, sizeof (data));

    gst_init (&argc, &argv);
 
    /* Build the pipeline */
    data.pipeline = gst_parse_launch
    //("playbin uri=https://www.freedesktop.org/software/gstreamer-sdk/data/media/sintel_trailer-480p.webm",
    //("playbin uri=rtsp://admin:admin@192.168.30.220:554/1/stream2/Profile1",
    //("playbin uri = srtsrc name=src latency=30000 ! queue ! tsparse" ,
    //("playbin uri = srtsrc name=src ! queue ! tsparse ! appsink name=sink",
    //("srtsrc name=src ! queue ! tsparse ! appsink name=sink",
    //("srtsrc name=src ! queue ! tsdemux ! tee name=t ! queue ! decodebin ! videoscale ! video/x-raw,width=320,height=240 ! videoconvert ! autovideosink ! appsink name=sink",
     //("srtsrc name=src ! queue ! tee name=t t. ! queue ! tsdemux !  decodebin ! videoscale ! video/x-raw,width=320,height=240 ! videoconvert ! autovideosink t. ! queue ! tsparse ! appsink name=sink",
     //("srtsrc name=src ! queue ! tsdemux ! tee name t t. ! queue ! decodebin ! videoscale ! video/x-raw,width=320,height=240 ! videoconvert ! autovideosink t. ! queue ! appsink name=sink",
     //("srtsrc name=src ! queue ! tee name=t t. ! queue ! tsdemux !  decodebin ! videoscale ! video/x-raw,width=320,height=240 ! videoconvert ! autovideosink t. ! queue ! tsparse ! appsink name=sink",
     //("srtsrc name=src ! queue ! tee name=t t. ! queue ! tsdemux !  decodebin ! videoscale ! video/x-raw,width=320,height=240 ! videoconvert ! autovideosink t. ! queue ! tsparse ! tsdemux ! jpegparse ! appsink name=sink", //multifilesink location=frame%d.png", 
     //---save frames by jpeg files 
     //("srtsrc name=src ! queue ! tsdemux ! h264parse ! avdec_h264 ! videoconvert ! jpegenc !  multifilesink location=frames%d.jpg", 
     //---stream to mjpeg (udp)
     ("srtsrc name=src ! queue ! tsdemux ! h264parse ! avdec_h264 ! videoconvert ! jpegenc ! rtpjpegpay ! udpsink host=127.0.0.1 port=5000", 
     NULL);
    
   
    src = gst_bin_get_by_name (GST_BIN (data.pipeline), "src");
    
    sink = gst_bin_get_by_name (GST_BIN (data.pipeline), "sink");
    
    /* set srt uri */
    g_object_set (src, "uri", SRT_URI, NULL);
    
    /* Configure appsink */
    g_object_set(sink, "emit-signals", TRUE, "async", FALSE, "sync", FALSE, NULL);
    
    g_signal_connect(sink, "new-sample", G_CALLBACK (new_sample_cb), &data);
    
    gst_object_unref (src);
    gst_object_unref (sink);
     
    /* Add a keyboard watch so we get notified of keystrokes */
#ifdef G_OS_WIN32
    io_stdin = g_io_channel_win32_new_fd (fileno (stdin));
#else
    io_stdin = g_io_channel_unix_new(fileno(stdin));
#endif

    g_io_add_watch(io_stdin, G_IO_IN, (GIOFunc)keyboard_message_cb, &data); 
    
    /* watch signal : bus */
    bus = gst_element_get_bus (data.pipeline);
    gst_bus_add_signal_watch (bus);
    g_signal_connect (bus, "message", G_CALLBACK (bus_message_cb), &data);
    gst_object_unref (bus);
  g_print("start\n");  
     /* Start playing */
    ret = gst_element_set_state (data.pipeline, GST_STATE_PLAYING);

    if (ret == GST_STATE_CHANGE_FAILURE) {
        g_printerr ("Unable to set the pipeline to the playing state.\n");
        gst_object_unref (data.pipeline);
    }
    
    /* play start */
    data.loop = g_main_loop_new (NULL, FALSE);
    g_main_loop_run (data.loop);
    
    g_printerr ("---- end -----\n");
    gst_element_set_state (data.pipeline, GST_STATE_NULL);
    g_main_loop_unref (data.loop);
    g_clear_object (&data.pipeline);
    g_io_channel_unref(io_stdin);
    
    
    return 0;
}

// --------------------------------------------------------------------------------------
//                                     key Manage                                        
// -------------------------------------------------------------------------------------- 
static void keyboard_message_cb(GIOChannel *source, GIOCondition cond, CustomData *data)
{
    gchar *str = NULL;

    while(g_io_channel_read_line (source, &str, NULL, NULL, NULL) == G_IO_STATUS_NORMAL) {
       
        int index = g_ascii_strtoull(str, NULL, 0);

        switch (index) {

            case 1: /*pause*/
                g_print("pause\n");
                gst_element_set_state (data->pipeline, GST_STATE_PAUSED);               
            break;

            case 2: /*replay*/
                g_print("play\n");
                gst_element_set_state (data->pipeline, GST_STATE_PLAYING);               
            break;
            
            case 3: /*program end*/
                 g_print("end\n");
                 g_main_loop_quit (data->loop);
            break;  
        }
        if(index >= 3)
            break;
        
    }
    g_free(str);
}

// --------------------------------------------------------------------------------------
//                                     bus Manage                                         
// --------------------------------------------------------------------------------------
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

