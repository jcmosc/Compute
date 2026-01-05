#pragma once

#if __APPLE__

#include <os/lock.h>

typedef os_unfair_lock platform_lock;

#define PLATFORM_LOCK_INIT ((os_unfair_lock)OS_UNFAIR_LOCK_INIT)

#define platform_lock_lock(LP) ({ (void)os_unfair_lock_lock(LP); })
#define platform_lock_unlock(LP) ({ (void)os_unfair_lock_unlock(LP); })
#define platform_lock_trylock(LP) ({ os_unfair_lock_trylock(LP); })

#else

#include <pthread.h>

typedef pthread_mutex_t platform_lock;

#define PLATFORM_LOCK_INIT ((pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER)

#define platform_lock_lock(LP) ({ (void)pthread_mutex_lock(LP); })
#define platform_lock_unlock(LP) ({ (void)pthread_mutex_unlock(LP); })
#define platform_lock_trylock(LP) ({ pthread_mutex_trylock(LP) == 0; })

#endif
