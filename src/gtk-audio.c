/* Functions to play audio files (.wav etc).
 *
 * This uses the gstreamer library & requires the playbin and wav plugins.
 * On Windows/msys2, these can be installed with:
 * pacman -S mingw-w64-i686-gstreamer mingw-w64-i686-gst-plugins-base mingw-w64-i686-gst-plugins-good
 * On Centos 8:
 * yum install gstreamer1 streamer1-plugins-good gstreamer1-devel
 */

#ifdef HAVE_CONFIG_H
#include <build-config.h>
#endif

#include <glib.h>
#include <glib/gi18n.h>
#ifdef HAS_LIBGSTREAMER
#include <gst/gst.h>
#endif

#include "gtk-audio.h"
#include "sat-log.h"

/* Convert path to absolute path */
gchar *
absolute_path (const gchar *filename)
{
    gchar *abs_path;

    if (!g_path_is_absolute (filename))
    {
        gchar *cwd = NULL;

        cwd = g_get_current_dir();
        abs_path = g_build_filename(cwd, filename, NULL);
        g_free(cwd);
    }
    else
        abs_path = g_strdup(filename);

    return abs_path;
}

#ifdef HAS_LIBGSTREAMER
void audio_play_uri(gchar *uri)
{
    GstElement *pipeline;

    /* playbin requires an absolute URI. E.g. file:///absolute/path.
       We try to see if the input uri is actually a file path. If so, we make it
       into a uri.
     */
    if (g_file_test(uri, G_FILE_TEST_EXISTS))
    {
        gchar *filename;
        GError *error = NULL;

        filename = absolute_path(uri);
        uri = g_filename_to_uri(filename, NULL, &error);
        g_free(filename);
    }

    /* Play audio using playbin plugin. */
    pipeline = gst_element_factory_make("playbin", NULL);
    if (!pipeline) {
        /* This will fail if the playbin plugin isn't found. */
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s: Failed to create playbin pipeline. Are gst plugins installed?"),
                    __func__);
        return;
    }
    g_object_set(pipeline, "uri", uri, NULL);
    gst_element_set_state(pipeline, GST_STATE_PLAYING);
}
#else
void audio_play_uri(gchar *uri)
{
    sat_log_log(SAT_LOG_LEVEL_WARN,
        _("%s: Request to play audio \"%s\", but gpredict was compiled without libgstreamer"),
        __func__, uri);
}
#endif
