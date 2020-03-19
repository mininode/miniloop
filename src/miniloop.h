/* libuEv - Micro event loop library
 *
 * Copyright (c) 2012       Flemming Madsen <flemming!madsen()madsensoft!dk>
 * Copyright (c) 2013-2019  Joachim Nilsson <troglobit()gmail!com>
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

#ifndef MINILOOP_H_
#define MINILOOP_H_

#include "private.h"

/* Max. number of simulateneous events */
#define MINILOOP_MAX_EVENTS  10

/* I/O events, signal and timer revents are always MINILOOP_READ */
#define MINILOOP_NONE        0
#define MINILOOP_ERROR       EPOLLERR
#define MINILOOP_READ        EPOLLIN
#define MINILOOP_WRITE       EPOLLOUT
#define MINILOOP_PRI         EPOLLPRI
#define MINILOOP_HUP         EPOLLHUP
#define MINILOOP_RDHUP       EPOLLRDHUP
#define MINILOOP_EDGE        EPOLLET
#define MINILOOP_ONESHOT     EPOLLONESHOT

/* Run flags */
#define MINILOOP_ONCE        1
#define MINILOOP_NONBLOCK    2

/* Macros */
#define ml_io_active(w)     _ml_watcher_active(w)
#define ml_signal_active(w) _ml_watcher_active(w)
#define ml_timer_active(w)  _ml_watcher_active(w)
#define ml_event_active(w)  _ml_watcher_active(w)

/* Event watcher */
typedef struct ml {
	/* Private data for miniloop internal engine */
	ml_private_t   type;

	/* Public data for users to reference  */
	int             signo;
	int             fd;
	ml_ctx_t      *ctx;
} ml_t;

/*
 * Generic callback for watchers, @events holds %MINILOOP_READ and/or %MINILOOP_WRITE
 * with optional %MINILOOP_PRI (priority data available to read) and any of the
 * %MINILOOP_HUP and/or %MINILOOP_RDHUP, which may be used to signal hang-up events.
 *
 * Note: MINILOOP_ERROR conditions must be handled by all callbacks!
 *       I/O watchers may also need to check MINILOOP_HUP.  Appropriate action,
 *       e.g. restart the watcher, is up to the application and is thus
 *       delegated to the callback.
 */
typedef void (ml_cb_t)(ml_t *w, void *arg, int events);

/* Public interface */
int ml_init          (ml_ctx_t *ctx, int maxevents);
int ml_exit           (ml_ctx_t *ctx);
int ml_run            (ml_ctx_t *ctx, int flags);

int ml_io_init        (ml_ctx_t *ctx, ml_t *w, ml_cb_t *cb, void *arg, int fd, int events);
int ml_io_set         (ml_t *w, int fd, int events);
int ml_io_start       (ml_t *w);
int ml_io_stop        (ml_t *w);

int ml_timer_init     (ml_ctx_t *ctx, ml_t *w, ml_cb_t *cb, void *arg, int timeout, int period);
int ml_timer_set      (ml_t *w, int timeout, int period);
int ml_timer_start    (ml_t *w);
int ml_timer_stop     (ml_t *w);

int ml_signal_init    (ml_ctx_t *ctx, ml_t *w, ml_cb_t *cb, void *arg, int signo);
int ml_signal_set     (ml_t *w, int signo);
int ml_signal_start   (ml_t *w);
int ml_signal_stop    (ml_t *w);

int ml_event_init     (ml_ctx_t *ctx, ml_t *w, ml_cb_t *cb, void *arg);
int ml_event_post     (ml_t *w);
int ml_event_stop     (ml_t *w);

#endif /* MINILOOP_H_ */

/**
 * Local Variables:
 *  indent-tabs-mode: t
 *  c-file-style: "linux"
 * End:
 */
