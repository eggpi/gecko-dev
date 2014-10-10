#include "js/jscustomallocator.h"

static PartitionAllocatorGeneric gPartitionAllocator;
static bool gInitialized;

PartitionAllocatorGeneric *
GetPartitionAllocator(void) {
    if (!gInitialized) {
        gPartitionAllocator.init();
        gInitialized = true;
    }

    return &gPartitionAllocator;
}
