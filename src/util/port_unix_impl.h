/*!
 * port_unix_impl.h - unix port for rdb
 * Copyright (c) 2022, Christopher Jeffrey (MIT License).
 * https://github.com/chjj/rdb
 */

#include <errno.h>
#include <limits.h> /* PTHREAD_STACK_MIN */
#include <stdlib.h>
#include <string.h>
#include <unistd.h> /* getpagesize */
/* #include <pthread.h> */
#include "internal.h"
#include "port.h"

#if defined(__APPLE__) || defined(__linux__)
#  include <sys/resource.h> /* getrlimit */
#endif

/*
 * Structs
 */

typedef struct rdb_args_s {
  void (*start)(void *);
  void *arg;
} rdb_args_t;

/*
 * Mutex
 */

void
rdb_mutex_init(rdb_mutex_t *mtx) {
  if (pthread_mutex_init(&mtx->handle, NULL) != 0)
    abort(); /* LCOV_EXCL_LINE */
}

void
rdb_mutex_destroy(rdb_mutex_t *mtx) {
  if (pthread_mutex_destroy(&mtx->handle) != 0)
    abort(); /* LCOV_EXCL_LINE */
}

void
rdb_mutex_lock(rdb_mutex_t *mtx) {
  if (pthread_mutex_lock(&mtx->handle) != 0)
    abort(); /* LCOV_EXCL_LINE */
}

void
rdb_mutex_unlock(rdb_mutex_t *mtx) {
  if (pthread_mutex_unlock(&mtx->handle) != 0)
    abort(); /* LCOV_EXCL_LINE */
}

/*
 * Conditional
 */

void
rdb_cond_init(rdb_cond_t *cond) {
  if (pthread_cond_init(&cond->handle, NULL) != 0)
    abort(); /* LCOV_EXCL_LINE */
}

void
rdb_cond_destroy(rdb_cond_t *cond) {
  if (pthread_cond_destroy(&cond->handle) != 0)
    abort(); /* LCOV_EXCL_LINE */
}

void
rdb_cond_signal(rdb_cond_t *cond) {
  if (pthread_cond_signal(&cond->handle) != 0)
    abort(); /* LCOV_EXCL_LINE */
}

void
rdb_cond_broadcast(rdb_cond_t *cond) {
  if (pthread_cond_broadcast(&cond->handle) != 0)
    abort(); /* LCOV_EXCL_LINE */
}

void
rdb_cond_wait(rdb_cond_t *cond, rdb_mutex_t *mtx) {
  if (pthread_cond_wait(&cond->handle, &mtx->handle) != 0)
    abort(); /* LCOV_EXCL_LINE */
}

/*
 * Thread
 */

/* Set a sane stack size for thread (from libuv). */
#if defined(__APPLE__) || defined(__linux__)
int getpagesize(void);

static size_t
thread_stack_size(void) {
  struct rlimit lim;

  if (getrlimit(RLIMIT_STACK, &lim) != 0)
    abort(); /* LCOV_EXCL_LINE */

  if (lim.rlim_cur != RLIM_INFINITY) {
    lim.rlim_cur -= (lim.rlim_cur % (rlim_t)getpagesize());

#if defined(PTHREAD_STACK_MIN)
    if (lim.rlim_cur >= PTHREAD_STACK_MIN)
#else
    if (lim.rlim_cur >= (16 << 10))
#endif
      return lim.rlim_cur;
  }

#if !defined(__linux__)
  return 0;
#elif defined(__PPC__) || defined(__ppc__) || defined(__powerpc__)
  return 4 << 20;
#else
  return 2 << 20;
#endif
}
#endif

static void *
rdb_thread_run(void *ptr) {
  rdb_args_t args = *((rdb_args_t *)ptr);

  free(ptr);

  args.start(args.arg);

  return NULL;
}

void
rdb_thread_create(rdb_thread_t *thread, void (*start)(void *), void *arg) {
  rdb_args_t *args = rdb_malloc(sizeof(rdb_args_t));
  pthread_attr_t *attr = NULL;

#if defined(__APPLE__) || defined(__linux__)
  size_t stack_size = thread_stack_size();
  pthread_attr_t tmp;

  if (stack_size > 0) {
    attr = &tmp;

    if (pthread_attr_init(attr) != 0)
      abort(); /* LCOV_EXCL_LINE */

    if (pthread_attr_setstacksize(attr, stack_size) != 0)
      abort(); /* LCOV_EXCL_LINE */
  }
#endif

  args->start = start;
  args->arg = arg;

  if (pthread_create(&thread->handle, attr, rdb_thread_run, args) != 0)
    abort(); /* LCOV_EXCL_LINE */
}

void
rdb_thread_detach(rdb_thread_t *thread) {
  if (pthread_detach(thread->handle) != 0)
    abort(); /* LCOV_EXCL_LINE */
}

void
rdb_thread_join(rdb_thread_t *thread) {
  if (pthread_join(thread->handle, NULL) != 0)
    abort(); /* LCOV_EXCL_LINE */
}
