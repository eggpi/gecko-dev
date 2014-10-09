#ifndef PartitionAlloc_mutex_h
#define PartitionAlloc_mutex_h

#ifndef MOZ_MEMORY_WINDOWS
#include <pthread.h>
#ifdef MOZ_MEMORY_DARWIN
#include <libkern/OSAtomic.h>
#define _pthread_mutex_init pthread_mutex_init
#define _pthread_mutex_lock pthread_mutex_lock
#define _pthread_mutex_unlock pthread_mutex_unlock
#endif
#endif

#ifdef MOZ_MEMORY_WINDOWS
#define _CRT_SPINCOUNT 5000
#define __crtInitCritSecAndSpinCount InitializeCriticalSectionAndSpinCount
#include <windows.h>
#endif

/*
 * Mutexes based on spinlocks.  We can't use normal pthread spinlocks in all
 * places, because they require malloc()ed memory, which causes bootstrapping
 * issues in some cases.
 */
#if defined(MOZ_MEMORY_WINDOWS)
#define malloc_mutex_t CRITICAL_SECTION
#elif defined(MOZ_MEMORY_DARWIN)
typedef struct {
	OSSpinLock	lock;
} malloc_mutex_t;
#elif defined(MOZ_MEMORY)
typedef pthread_mutex_t malloc_mutex_t;
#endif

static bool
malloc_mutex_init(malloc_mutex_t *mutex)
{
#if defined(MOZ_MEMORY_WINDOWS)
	if (! __crtInitCritSecAndSpinCount(mutex, _CRT_SPINCOUNT))
		return (true);
#elif defined(MOZ_MEMORY_DARWIN)
	mutex->lock = OS_SPINLOCK_INIT;
#elif defined(MOZ_MEMORY_LINUX) && !defined(MOZ_MEMORY_ANDROID)
	pthread_mutexattr_t attr;
	if (pthread_mutexattr_init(&attr) != 0)
		return (true);
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ADAPTIVE_NP);
	if (pthread_mutex_init(mutex, &attr) != 0) {
		pthread_mutexattr_destroy(&attr);
		return (true);
	}
	pthread_mutexattr_destroy(&attr);
#elif defined(MOZ_MEMORY)
	if (pthread_mutex_init(mutex, NULL) != 0)
		return (true);
#endif
	return (false);
}

static inline void
malloc_mutex_lock(malloc_mutex_t *mutex)
{

#if defined(MOZ_MEMORY_WINDOWS)
	EnterCriticalSection(mutex);
#elif defined(MOZ_MEMORY_DARWIN)
	OSSpinLockLock(&mutex->lock);
#elif defined(MOZ_MEMORY)
	pthread_mutex_lock(mutex);
#endif
}

static inline void
malloc_mutex_unlock(malloc_mutex_t *mutex)
{

#if defined(MOZ_MEMORY_WINDOWS)
	LeaveCriticalSection(mutex);
#elif defined(MOZ_MEMORY_DARWIN)
	OSSpinLockUnlock(&mutex->lock);
#elif defined(MOZ_MEMORY)
	pthread_mutex_unlock(mutex);
#endif
}

class MallocMutex {
	public:
		MallocMutex() {
			malloc_mutex_init(&mMutex);
		}

		void Lock(void) {
			malloc_mutex_lock(&mMutex);
			return;
		}

		void Unlock(void) {
			malloc_mutex_unlock(&mMutex);
			return;
		}
	private:
		malloc_mutex_t mMutex;
};

#endif // PartitionAlloc_mutex_h
