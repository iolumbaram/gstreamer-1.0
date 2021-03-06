#include <string.h>
#include <gst/gst.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
/* this script
	can:
		. live-stream for only sometime (upon initial launch)
	cant:
		. live-streaming
		. smooth display

******************* final! do this! *******************
gst-launch-1.0 -v v4l2src ! videoconvert ! vp8enc deadline=1 ! webmmux ! filesink location=/home/hazel/Desktop/live-gst05.webm

this doesnt work
gst-launch-1.0 autovideosrc horizontal-speed=1 is-live=true ! videoconvert ! vp8enc cpu-used=5 deadline=1 keyframe-max-dist=10 ! queue leaky=1 ! webmmux ! queue leaky=1 ! filesink location=/home/hazel/live-gst02.webm

gst-launch-1.0 v4l2src ! x264enc ! mp4mux ! filesink location=/home/hazel/Desktop/okay.264 -e

reference from:
https://gist.github.com/CreaRo/8c71729ed58c4c134cac44db74ea1754
output will record webcam data encode it into webm format
*/

static GMainLoop *loop;
static GstElement *pipeline, *src, *encoder, *muxer, *sink;
static GstBus *bus;
static gboolean is_live;

#define CAPS "vp8enc, deadline=1"

static gboolean
message_cb (GstBus * bus, GstMessage * message, gpointer user_data)
{
  switch (GST_MESSAGE_TYPE (message)) {
    case GST_MESSAGE_ERROR:{
      GError *err = NULL;
      gchar *name, *debug = NULL;

      name = gst_object_get_path_string (message->src);
      gst_message_parse_error (message, &err, &debug);

      g_printerr ("ERROR: from element %s: %s\n", name, err->message);
      if (debug != NULL)
        g_printerr ("Additional debug info:\n%s\n", debug);

      g_error_free (err);
      g_free (debug);
      g_free (name);

      g_main_loop_quit (loop);
      break;
    }
    case GST_MESSAGE_WARNING:{
		GError *err = NULL;
		gchar *name, *debug = NULL;

		name = gst_object_get_path_string (message->src);
		gst_message_parse_warning (message, &err, &debug);

		g_printerr ("ERROR: from element %s: %s\n", name, err->message);
		if (debug != NULL)
		g_printerr ("Additional debug info:\n%s\n", debug);

		g_error_free (err);
		g_free (debug);
		g_free (name);
		break;
    }

	case GST_MESSAGE_BUFFERING:{
		gint percent = 0;
		gst_message_parse_buffering (message, &percent);
		g_print ("Buffering (%3d%%)\r", percent);
		
		if(is_live) break;

		if (percent < 100)
        	gst_element_set_state (pipeline, GST_STATE_PAUSED);
      	else
        	gst_element_set_state (pipeline, GST_STATE_PLAYING);
      	break;
	}

//    case GST_MESSAGE_CLOCK_LOST:
//    	/* Get a new clock */
//      	gst_element_set_state (pipeline, GST_STATE_PAUSED);
//      	gst_element_set_state (pipeline, GST_STATE_PLAYING);
//      	break;
//    }

    case GST_MESSAGE_EOS:{
		g_print ("Got EOS\n");
		g_main_loop_quit (loop);
		gst_element_set_state (pipeline, GST_STATE_READY);
		g_main_loop_unref (loop);
		gst_object_unref (pipeline);
		exit(0);
		break;
	}
    default:
		break;
  }

  return TRUE;
}

//int sigintHandler(int unused) {
//	g_print("You ctrl-c-ed! Sending EoS");
//	gst_element_send_event(pipeline, gst_event_new_eos()); 
//	return 0;
//}

int main(int argc, char *argv[])
{	
//	GstCaps *caps;

	//signal(SIGINT, sigintHandler);
	gst_init (&argc, &argv);
	GstStateChangeReturn ret;

	pipeline = gst_pipeline_new(NULL);
	src = gst_element_factory_make("v4l2src", NULL);
	//converter = gst_element_factory_make("videoconverter", NULL);
	encoder = gst_element_factory_make("vp8enc", NULL);
	muxer = gst_element_factory_make("webmmux", NULL);
	sink = gst_element_factory_make("filesink", NULL);

	if (!pipeline || !src || !encoder || !muxer || !sink) {
		g_error("Failed to create elements");
		return -1;
	}

	/*output file location for sink element*/
	g_object_set(sink, "location", "/home/hazel/Desktop/rec.webm", NULL);
	/*add message handler*/
	//bus = 

	/*adding elements to pipelines*/
	gst_bin_add_many(GST_BIN(pipeline), src, encoder, muxer, sink, NULL);

	/*specify caps for encoder*/
//	caps = gst_caps_new_full(
//		gst_structure_new("vp8enc", 
//			"deadline", G_TYPE_INT, 1, NULL), 
//		NULL);
 
	/*linking all the elements*/
	//gst_element_link(src, encoder);	
	//gst_element_link_many (src, encoder, muxer, sink, NULL);
	/*check elements are linked?*/
	if (!gst_element_link_many(src, encoder, muxer, sink, NULL)){
		g_error("Failed to link elements");
		return -2;
	}

	loop = g_main_loop_new(NULL, FALSE);

	/* adding properties for element, encoder*/
	//*encoder.set_property("deadline",1);
	//g_print(print);
	g_object_set(encoder, "deadline", 2, NULL);
	//link elements and caps
	//gst_element_link_filtered(encoder, muxer, caps);
//	if (!gst_element_link_filtered(encoder, muxer, caps))
//	{
//		gst_object_unref(pipeline);
//		g_print("caps for encoder not linked");
//		g_print(" closing pipeline");
//		return 0;
//	}
	
	/*build pipeline*/
	bus = gst_pipeline_get_bus(GST_PIPELINE (pipeline));

	/*start playing*/
	ret = gst_element_set_state (pipeline, GST_STATE_PLAYING);
	if(ret == GST_STATE_CHANGE_FAILURE){
		gst_object_unref(pipeline);
		return -1;
	}else if (ret==GST_STATE_CHANGE_NO_PREROLL){
		is_live = TRUE;
		g_print("live streaming..");
	}

	gst_bus_add_signal_watch(bus);
	g_signal_connect(G_OBJECT(bus), "message", G_CALLBACK(message_cb), NULL);
	gst_object_unref(GST_OBJECT(bus));

	gst_element_set_state(pipeline, GST_STATE_PLAYING);

	g_print("Starting loop");
	g_main_loop_run(loop);

	return 0;
}
