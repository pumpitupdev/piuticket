
#include <string.h>
#include "util.h"


// Resolve proper PCM Endpoint
void sound_resolve_pcm_endpoint(char* pcm_endpoint){
    strcpy(pcm_endpoint,sound_device);
}

