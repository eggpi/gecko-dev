#ifndef vm_PartitionAllocator_h
#define vm_PartitionAllocator_h

#include <PartitionAlloc.h>
#include <string.h>

#include "js/Utility.h"

PartitionAllocatorGeneric *GetPartitionAllocator(void);

static inline void* js_malloc(size_t bytes)
{
    JS_OOM_POSSIBLY_FAIL();
    return partitionAllocGeneric(GetPartitionAllocator()->root(), bytes);
}

static inline void* js_calloc(size_t bytes)
{
    JS_OOM_POSSIBLY_FAIL();
    void *mem = partitionAllocGeneric(GetPartitionAllocator()->root(), bytes);
    if (mem) memset(mem, 0, bytes);
    return mem;
}

static inline void* js_calloc(size_t nmemb, size_t size)
{
    JS_OOM_POSSIBLY_FAIL();
    // FIXME overflow check here!
    size_t bytes = nmemb * size;
    void *mem = partitionAllocGeneric(GetPartitionAllocator()->root(), bytes);
    if (mem) memset(mem, 0, bytes);
    return mem;
}

static inline void* js_realloc(void* p, size_t bytes)
{
    JS_OOM_POSSIBLY_FAIL();
    return partitionReallocGeneric(GetPartitionAllocator()->root(), p, bytes);
}

static inline void js_free(void* p)
{
    partitionFreeGeneric(GetPartitionAllocator()->root(), p);
}

static inline char* js_strdup(const char* s)
{
    JS_OOM_POSSIBLY_FAIL();
    char* ret = (char *) js_malloc(strlen(s) + 1);
    if (ret) {
      strcpy(ret, s);
    }
    return ret;
}

// FIXME when to call partitionAllocGenericShutdown() ?
#endif /* vm_PartitionAllocator_h */
