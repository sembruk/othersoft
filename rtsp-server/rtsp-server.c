/* GStreamer
 * Copyright (C) 2008 Wim Taymans <wim.taymans at gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <gst/gst.h>

#include <gst/rtsp-server/rtsp-server.h>
#include <gst/rtsp-server/rtsp-session-pool.h>

#define SESSION_TIMEOUT 3

static gboolean
check_session (GstRTSPSessionPool * pool, GstRTSPSession * session, GstRTSPServer * server)
{
  //return GST_RTSP_FILTER_REMOVE;
  guint session_timeout = gst_rtsp_session_get_timeout (session);
  if (session_timeout > SESSION_TIMEOUT)
  {
    gst_rtsp_session_set_timeout(session, SESSION_TIMEOUT);
  }
  return GST_RTSP_FILTER_KEEP;
}

/* this timeout is periodically run to clean up the expired sessions from the
 * pool. This needs to be run explicitly currently but might be done
 * automatically as part of the mainloop. */
static gboolean
timeout (GstRTSPServer * server)
{
  GstRTSPSessionPool *pool;

  pool = gst_rtsp_server_get_session_pool (server);
  guint n_sessions = gst_rtsp_session_pool_get_n_sessions(pool);
  g_print("%d\n", n_sessions);

  gst_rtsp_session_pool_filter (pool, check_session, server);

  gst_rtsp_session_pool_cleanup (pool);
  g_object_unref (pool);

  return TRUE;
}

int
main (int argc, char *argv[])
{
  GMainLoop *loop;
  GstRTSPServer *server;
  GstRTSPMediaMapping *mapping;
  GstRTSPMediaFactory *factory;

  gst_init (&argc, &argv);

  if (argc < 4) {
    g_print ("usage: %s port path <launch line> \n"
        "example: %s 554 /video \"( videotestsrc ! x264enc ! rtph264pay name=pay0 pt=96 )\"\n",
        argv[0], argv[0]);
    return -1;
  }

  g_print("Starting RTSP server on :%s%s\n\n", argv[1], argv[2]);

  loop = g_main_loop_new (NULL, FALSE);

  /* create a server instance */
  server = gst_rtsp_server_new ();

  /* get the mapping for this server, every server has a default mapper object
   * that be used to map uri mount points to media factories */
  mapping = gst_rtsp_server_get_media_mapping (server);
  gst_rtsp_server_set_service(server, argv[1]);

  /* make a media factory for a test stream. The default media factory can use
   * gst-launch syntax to create pipelines. 
   * any launch line works as long as it contains elements named pay%d. Each
   * element with pay%d names will be a stream */
  factory = gst_rtsp_media_factory_new ();
  gst_rtsp_media_factory_set_launch (factory, argv[3]);

  /* attach the test factory to the /test url */
  gst_rtsp_media_mapping_add_factory (mapping, argv[2], factory);

  /* don't need the ref to the mapper anymore */
  g_object_unref (mapping);

  /* attach the server to the default maincontext */
  gst_rtsp_server_attach (server, NULL);

  /* add a timeout for the session cleanup */
  g_timeout_add_seconds (2, (GSourceFunc) timeout, server);

  /* start serving */
  g_main_loop_run (loop);

  return 0;
}
