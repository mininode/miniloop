/* libuEv - Private methods and data types, do not use directly!
 *
 * Copyright (c) 2012       Flemming Madsen <flemming!madsen()madsensoft!dk>
 * Copyright (c) 2013-2019  Joachim Nilsson <troglobit()gmail!com>
 * Copyright (c) 2020       Alex Caudill <alex.caudill@pm.me>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef LIBMINILOOP_PRIVATE_H_
#define LIBMINILOOP_PRIVATE_H_

#include <stdio.h>
#include <sys/epoll.h>

/*
 * List functions.
 */
#define _MINILOOP_FOREACH(node, list)		\
	for (typeof (node) next, node = list;	\
	     node && (next = node->next, 1);	\
	     node = next)

#define _MINILOOP_INSERT(node, list) do {		\
	typeof (node) next;			\
	next       = list;			\
	list       = node;			\
	if (next)				\
		next->prev = node;		\
	node->next = next;			\
	node->prev = NULL;			\
} while (0)

#define _MINILOOP_REMOVE(node, list) do {		\
	typeof (node) prev, next;		\
	prev = node->prev;			\
	next = node->next;			\
	if (prev)				\
		prev->next = next;		\
	if (next)				\
		next->prev = prev;		\
	node->prev = NULL;			\
	node->next = NULL;			\
	if (list == node)			\
		list = next;			\
} while (0)

/* I/O, fs, timer, or signal watcher */
typedef enum {
	MINILOOP_IO_TYPE = 1,
	MINILOOP_SIGNAL_TYPE,
	MINILOOP_TIMER_TYPE,
  MINILOOP_FS_TYPE,
	MINILOOP_EVENT_TYPE,
} ml_type_t;

/* Event mask, used internally only. */
#define MINILOOP_EVENT_MASK  (MINILOOP_ERROR | MINILOOP_READ | MINILOOP_WRITE | MINILOOP_PRI |	\
			 MINILOOP_RDHUP | MINILOOP_HUP | MINILOOP_EDGE | MINILOOP_ONESHOT)

/* Main miniloop context type */
typedef struct {
	int             running;
  int             inotify_fd; /* Take a guess what this is for... */
	int             fd;	        /* For epoll() */
	int             maxevents;  /* For epoll() */
	struct ml      *watchers;
	uint32_t        workaround; /* For workarounds, e.g. redirected stdin */
} ml_ctx_t;

/* Forward declare due to dependencies, don't try this at home kids. */
struct ml;

/* This is used to hide all private data members in ml_t */
#define ml_private_t                                           \
	struct ml     *next, *prev;				\
								\
	int             active;                                 \
	int             events;                                 \
								\
	/* Watcher callback with optional argument */           \
	void          (*cb)(struct ml *, void *, int);         \
	void           *arg;                                    \
								\
	/* Arguments for different watchers */			\
	union {							\
		/* Cron watchers */				\
		struct {					\
			time_t when;				\
			time_t interval;			\
		} c;						\
								\
		/* Timer watchers, time in milliseconds */	\
		struct {					\
			int timeout;				\
			int period;				\
		} t;						\
	} u;							\
								\
	/* Watcher type */					\
	ml_type_t

/* Internal API for dealing with generic watchers */
int _ml_watcher_init  (ml_ctx_t *ctx, struct ml *w, ml_type_t type,
			void (*cb)(struct ml *, void *, int), void *arg,
			int fd, int events);
int _ml_watcher_start (struct ml *w);
int _ml_watcher_stop  (struct ml *w);
int _ml_watcher_active(struct ml *w);
int _ml_watcher_rearm (struct ml *w);

#endif /* LIBMINILOOP_PRIVATE_H_ */

/**
 * Local Variables:
 *  indent-tabs-mode: t
 *  c-file-style: "linux"
 * End:
 */
