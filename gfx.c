
#include "gfx.h"


// Patches valuemask for Non-Nvidia Cards
unsigned long gfx_patch_valuemask(unsigned long valuemask){
    valuemask |= GCTileStipYOrigin;
    return valuemask;
}
