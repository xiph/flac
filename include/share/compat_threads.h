/* libFLAC - Free Lossless Audio Codec library
 * Copyright (C) 2025 Jörn Heusipp
 * Copyright (C) 2025 Xiph.Org Foundation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 *
 * - Neither the name of the Xiph.org Foundation nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef FLAC__SHARE__COMPAT_THREADS_H
#define FLAC__SHARE__COMPAT_THREADS_H

#if defined(HAVE_C11THREADS) || defined(HAVE_PTHREAD)
#define FLAC__USE_THREADS
#endif

#ifdef FLAC__USE_THREADS
#if defined(HAVE_C11THREADS)
#include <threads.h>
#elif defined(HAVE_PTHREAD)
#include <errno.h>
#include <stddef.h>
#include <pthread.h>
#endif
#endif


#ifdef FLAC__USE_THREADS

#if defined(HAVE_C11THREADS)

#define FLAC__thrd_success                   thrd_success
#define FLAC__thrd_busy                      thrd_busy
#define FLAC__thrd_nomem                     thrd_nomem

#define FLAC__thrd_t                         thrd_t
#define FLAC__thrd_create(thread, func, arg) thrd_create(thread, func, arg)
#define FLAC__thrd_join(thread, result)      thrd_join(thread, result)

#define FLAC__mtx_plain                      mtx_plain
#define FLAC__mtx_t                          mtx_t
#define FLAC__mtx_init(mutex, type)          mtx_init(mutex, type)
#define FLAC__mtx_trylock(mutex)             mtx_trylock(mutex)
#define FLAC__mtx_lock(mutex)                mtx_lock(mutex)
#define FLAC__mtx_unlock(mutex)              mtx_unlock(mutex)
#define FLAC__mtx_destroy(mutex)             mtx_destroy(mutex)

#define FLAC__cnd_t                          cnd_t
#define FLAC__cnd_init(cv)                   cnd_init(cv)
#define FLAC__cnd_broadcast(cv)              cnd_broadcast(cv)
#define FLAC__cnd_signal(cv)                 cnd_signal(cv)
#define FLAC__cnd_wait(cv, mutex)            cnd_wait(cv, mutex)
#define FLAC__cnd_destroy(cv)                cnd_destroy(cv)

#define FLAC__thread_return_type             int
#define FLAC__thread_default_return_value    0

#elif defined(HAVE_PTHREAD)

/* This is not meant to be a full implementation of C11 threads on top of
 * pthreads.
 * Just enough is implemented to provide the interfaces that libFLAC uses
 * currently.
 * A full implementation would certainly be possible, but there already are
 * various other open source projects dedicated to providing pthread <-> C11
 * threads wrappers both ways. FLAC wants to avoid the additional dependency
 * here, and provides this reduced wrapper itself.
 */

#define FLAC__thrd_success                   0
#define FLAC__thrd_busy                      EBUSY
#define FLAC__thrd_nomem                     ENOMEM

#define FLAC__thrd_t                         pthread_t
#define FLAC__thrd_create(thread, func, arg) pthread_create(thread, NULL, func, arg)
#define FLAC__thrd_join(thread, result)      pthread_join(thread, result)

#define FLAC__mtx_plain                      NULL
#define FLAC__mtx_t                          pthread_mutex_t
#define FLAC__mtx_init(mutex, type)          ((pthread_mutex_init(mutex, type) == 0) ? FLAC__thrd_success : FLAC__thrd_nomem)
#define FLAC__mtx_trylock(mutex)             ((pthread_mutex_trylock(mutex) == 0) ? FLAC__thrd_success : FLAC__thrd_busy)
#define FLAC__mtx_lock(mutex)                pthread_mutex_lock(mutex)
#define FLAC__mtx_unlock(mutex)              pthread_mutex_unlock(mutex)
#define FLAC__mtx_destroy(mutex)             pthread_mutex_destroy(mutex)

#define FLAC__cnd_t                          pthread_cond_t
#define FLAC__cnd_init(cv)                   ((pthread_cond_init(cv, NULL) == 0) ? FLAC__thrd_success : FLAC__thrd_nomem)
#define FLAC__cnd_broadcast(cv)              pthread_cond_broadcast(cv)
#define FLAC__cnd_signal(cv)                 pthread_cond_signal(cv)
#define FLAC__cnd_wait(cv, mutex)            pthread_cond_wait(cv, mutex)
#define FLAC__cnd_destroy(cv)                pthread_cond_destroy(cv)

#define FLAC__thread_return_type             void *
#define FLAC__thread_default_return_value    NULL

#endif

#endif


#endif /* FLAC__SHARE__COMPAT_THREADS_H */
