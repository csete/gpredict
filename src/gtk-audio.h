#ifndef __GTK_AUDIO_H
#define __GTK_AUDIO_H 1

#include <glib.h>

#ifdef __cplusplus
extern "C" {
#endif

/**< Play audio file. E.g. .wav 
     uri can be a relative pathname or URI.   
     E.g. ../path/file.wav, file:///path/something.wav or http://server/file.wav
     Supported file formats depend on which plugins are installed. 
     */  
void audio_play_uri(gchar *uri);

#ifdef __cplusplus
}
#endif

#endif 
