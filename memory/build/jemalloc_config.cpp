/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifdef MOZ_JEMALLOC4

#define MOZ_JEMALLOC_IMPL

/* mozmemory_wrap.h needs to be included before MFBT headers */
#include "mozmemory_wrap.h"
#include <mozilla/Assertions.h>
#include "mozilla/TaggedAnonymousMemory.h"
// Note: MozTaggedAnonymousMmap() could call an LD_PRELOADed mmap,
// use only MozTagAnonymousMemory().
#include "mozilla/Types.h"

#define DLLEXPORT
#include "jemalloc/jemalloc.h"

#ifdef XP_WIN
#include <windows.h>
#endif
#ifdef XP_DARWIN
#include <sys/mman.h>
#endif

/* Override some jemalloc defaults */
#ifdef DEBUG
#define MOZ_MALLOC_BUILD_OPTIONS ",junk:true"
#else
#define MOZ_MALLOC_BUILD_OPTIONS ",junk:free"
#endif

#define MOZ_MALLOC_OPTIONS "narenas:1,tcache:false"
MFBT_DATA const char* je_(malloc_conf) =
  MOZ_MALLOC_OPTIONS MOZ_MALLOC_BUILD_OPTIONS;

#ifdef ANDROID
#include <android/log.h>

static void
_je_malloc_message(void* cbopaque, const char* s)
{
  __android_log_print(ANDROID_LOG_INFO, "GeckoJemalloc", "%s", s);
}

void (*je_(malloc_message))(void*, const char* s) = _je_malloc_message;
#endif

/* Jemalloc supports hooks that are called on chunk
 * allocate/deallocate/commit/decommit/purge/etc.
 *
 * We currently only hook alloc, commit, decommit and purge. We do this for
 * tagging anonymous memory for finer-grained reporting where supported
 * (alloc, commit and decommit hooks), and to tweak the way chunks are
 * handled so that RSS stays lower than it normally would with the default
 * jemalloc uses (commit, decommit and purge).
 * This somewhat matches the behavior of mozjemalloc, except it doesn't
 * rely on a double purge on mac, instead purging directly. (Yes, this
 * means we can get rid of jemalloc_purge_freed_pages at some point)
 *
 * The default for jemalloc is to do the following:
 * - commit, decommit: nothing
 * - purge: MEM_RESET on Windows, MADV_FREE on Mac/BSD, MADV_DONTNEED on Linux
 *
 * The hooks we setup do the following:
 * on Windows:
 * - commit: MEM_COMMIT
 * - decommit: MEM_DECOMMIT
 * on Mac:
 * - purge: mmap new anonymous memory on top of the chunk
 *
 * We only set the above hooks, others are left with the default.
 */
class JemallocInit {
public:
  JemallocInit()
  {
    chunk_hooks_t hooks;
    size_t hooks_len;
    unsigned narenas;
    size_t mib[3];
    size_t size;

    size = sizeof(narenas);
    je_(mallctl)("arenas.narenas", &narenas, &size, nullptr, 0);

    size = sizeof(mib) / sizeof(mib[0]);
    je_(mallctlnametomib)("arena.0.chunk_hooks", mib, &size);
    je_(mallctlbymib)(mib, size, &sDefaultChunkHooks, &hooks_len, nullptr, 0);

    /* Set the hooks on all the existing arenas. */
    for (unsigned arena = 0; arena < narenas; arena++) {
      mib[1] = arena;
      hooks_len = sizeof(hooks);
      je_(mallctlbymib)(mib, size, &hooks, &hooks_len, nullptr, 0);

#ifndef XP_WIN
      hooks.alloc = AllocHook;
#endif
#ifdef XP_DARWIN
      hooks.purge = PurgeHook;
#endif
      hooks.commit = CommitHook;
      hooks.decommit = DecommitHook;

      je_(mallctlbymib)(mib, size, nullptr, nullptr, &hooks, hooks_len);
    }
  }

private:
  static chunk_hooks_t sDefaultChunkHooks;

#ifdef XP_WIN
  static bool
  CommitHook(void* chunk, size_t size, size_t offset, size_t length,
             unsigned arena_ind)
  {
    void* addr = reinterpret_cast<void*>(
      reinterpret_cast<uintptr_t>(chunk) + static_cast<uintptr_t>(offset));

    if (!VirtualAlloc(addr, length, MEM_COMMIT, PAGE_READWRITE))
      return true;

    return false;
  }

  static bool
  DecommitHook(void* chunk, size_t size, size_t offset, size_t length,
               unsigned arena_ind)
  {
    void* addr = reinterpret_cast<void*>(
      reinterpret_cast<uintptr_t>(chunk) + static_cast<uintptr_t>(offset));

    if (!VirtualFree(addr, length, MEM_DECOMMIT))
      MOZ_CRASH();

    return false;
  }
#endif

#ifndef XP_WIN
  /* Hooks that delegate to the default hooks, but tag anonymous memory.
   * MozTagAnonymousMemory is targeted at Android, and is not defined at all
   * Windows, but defaults to a noop on all other platforms where support is
   * absent.
   */

  static void*
  AllocHook(void* chunk, size_t size, size_t alignment, bool* zero,
            bool* commit, unsigned arena_ind)
  {
    chunk = sDefaultChunkHooks.alloc(chunk, size, alignment, zero, commit,
                                     arena_ind);
    if (chunk) {
      MozTagAnonymousMemory(chunk, size, "jemalloc");
    }
    return chunk;
  }

  static bool
  CommitHook(void* chunk, size_t size, size_t offset, size_t length,
             unsigned arena_ind)
  {
    void* addr = reinterpret_cast<void*>(
      reinterpret_cast<uintptr_t>(chunk) + static_cast<uintptr_t>(offset));

    if (!sDefaultChunkHooks.commit(chunk, size, offset, length, arena_ind)) {
      MozTagAnonymousMemory(addr, length, "jemalloc");
      return false;
    }
    return true;
  }

  static bool
  DecommitHook(void* chunk, size_t size, size_t offset, size_t length,
               unsigned arena_ind)
  {
    void* addr = reinterpret_cast<void*>(
      reinterpret_cast<uintptr_t>(chunk) + static_cast<uintptr_t>(offset));

    if (!sDefaultChunkHooks.decommit(chunk, size, offset, length, arena_ind)) {
      MozTagAnonymousMemory(addr, length, "jemalloc-decommitted");
      return false;
    }
    return true;
  }
#endif

#ifdef XP_DARWIN
  static bool
  PurgeHook(void* chunk, size_t size, size_t offset, size_t length,
            unsigned arena_ind)
  {
    void* addr = reinterpret_cast<void*>(
      reinterpret_cast<uintptr_t>(chunk) + static_cast<uintptr_t>(offset));

    void* new_addr = mmap(addr, length, PROT_READ | PROT_WRITE,
                          MAP_FIXED | MAP_PRIVATE | MAP_ANON, -1, 0);
    return (new_addr != addr);
  }
#endif
};

/* For the static constructor from the class above */
chunk_hooks_t JemallocInit::sDefaultChunkHooks;
JemallocInit gJemallocInit;

#else
#include <mozilla/Assertions.h>
#endif /* MOZ_JEMALLOC4 */

/* Provide an abort function for use in jemalloc code */
extern "C" void moz_abort() {
  MOZ_CRASH();
}
