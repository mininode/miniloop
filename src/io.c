/* miniloop - Micro event loop library
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

#include <errno.h>
#include "miniloop.h"

/**
 * Create an I/O watcher
 * @param ctx     A valid miniloop context
 * @param w       Pointer to an ml_t watcher
 * @param cb      I/O callback
 * @param arg     Optional callback argument
 * @param fd      File descriptor to watch, or -1 to register an empty watcher
 * @param events  Events to watch for: %MINILOOP_READ, %MINILOOP_WRITE, %MINILOOP_EDGE, %MINILOOP_ONESHOT
 *
 * @return POSIX OK(0) or non-zero with @param errno set on error.
 */
int ml_io_init(ml_ctx_t *ctx, ml_t *w, ml_cb_t *cb, void *arg, int fd, int events)
{
	if (fd < 0) {
		errno = EINVAL;
		return -1;
	}

	if (_ml_watcher_init(ctx, w, MINILOOP_IO_TYPE, cb, arg, fd, events))
		return -1;

	return _ml_watcher_start(w);
}

/**
 * Reset an I/O watcher
 * @param w       Pointer to an ml_t watcher
 * @param fd      New file descriptor to monitor
 * @param events  Requested events to watch for, a mask of %MINILOOP_READ and %MINILOOP_WRITE
 *
 * @return POSIX OK(0) or non-zero with @param errno set on error.
 */
int ml_io_set(ml_t *w, int fd, int events)
{
	if ((events & MINILOOP_ONESHOT) && _ml_watcher_active(w))
		return _ml_watcher_rearm(w);

	/* Ignore any errors, only to clean up anything lingering ... */
	ml_io_stop(w);

	return ml_io_init(w->ctx, w, (ml_cb_t *)w->cb, w->arg, fd, events);
}

/**
 * Start an I/O watcher
 * @param w  Watcher to start (again)
 *
 * @return POSIX OK(0) or non-zero with @param errno set on error.
 */
int ml_io_start(ml_t *w)
{
	return ml_io_set(w, w->fd, w->events);
}

/**
 * Stop an I/O watcher
 * @param w  Watcher to stop
 *
 * @return POSIX OK(0) or non-zero with @param errno set on error.
 */
int ml_io_stop(ml_t *w)
{
	return _ml_watcher_stop(w);
}

/**
 * Local Variables:
 *  indent-tabs-mode: t
 *  c-file-style: "linux"
 * End:
 */
