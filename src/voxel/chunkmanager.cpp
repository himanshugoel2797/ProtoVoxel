#include "chunkmanager.h"

//for now just a fixed size grid
//chunkjobmanager class manages per thread resources for chunks like temporary memory to which chunks are decompressed
//only chunks in a small radius around an active object are kept uncompressed at any time
//