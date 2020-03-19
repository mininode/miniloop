/* miniloop - Micro event loop library
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

#include <errno.h>
#include <signal.h>
#include <sys/signalfd.h>
#include <unistd.h>		/* close(), read() */

#include "miniloop.h"

/**
 * Create a signal watcher
 * @param ctx    A valid miniloop context
 * @param w      Pointer to an ml_t watcher
 * @param cb     Signal callback
 * @param arg    Optional callback argument
 * @param signo  Signal to watch for
 *
 * @return POSIX OK(0) or non-zero with @param errno set on error.
 */
int ml_signal_init(ml_ctx_t *ctx, ml_t *w, ml_cb_t *cb, void *arg, int signo)
{
	sigset_t mask;
	int fd;

	if (!w || !ctx) {
		errno = EINVAL;
		return -1;
	}
	w->fd = -1;

	sigemptyset(&mask);
	fd = signalfd(-1, &mask, SFD_NONBLOCK | SFD_CLOEXEC);
	if (fd < 0)
		return -1;

	if (_ml_watcher_init(ctx, w, MINILOOP_SIGNAL_TYPE, cb, arg, fd, MINILOOP_READ))
		goto exit;

	if (ml_signal_set(w, signo)) {
		_ml_watcher_stop(w);
	exit:
		close(fd);
		return -1;
	}

	return 0;
}

/**
 * Reset a signal watcher
 * @param w      Watcher to reset
 * @param signo  New signal to watch for
 *
 * @return POSIX OK(0) or non-zero with @param errno set on error.
 */
int ml_signal_set(ml_t *w, int signo)
{
	sigset_t mask;

	/* Every watcher must be registered to a context */
	if (!w || !w->ctx) {
		errno = EINVAL;
		return -1;
	}

	/* Remember for callbacks and start/stop */
	w->signo = signo;

	/* Handle stopped signal watchers */
	if (w->fd < 0) {
		if (ml_signal_init(w->ctx, w, (ml_cb_t *)w->cb, w->arg, signo))
			return -1;
	}

	sigemptyset(&mask);
	sigaddset(&mask, signo);

	/* Block signals so that they aren't handled
	   according to their default dispositions */
	if (sigprocmask(SIG_BLOCK, &mask, NULL) == -1)
		return -1;

	if (signalfd(w->fd, &mask, SFD_NONBLOCK) < 0)
		return -1;

	return _ml_watcher_start(w);
}


/**
 * Start a stopped signal watcher
 * @param w  Watcher to start (again)
 *
 * @return POSIX OK(0) or non-zero with @param errno set on error.
 */
int ml_signal_start(ml_t *w)
{
	if (!w) {
		errno = EINVAL;
		return -1;
	}

	if (-1 != w->fd)
		ml_signal_stop(w);

	return ml_signal_set(w, w->signo);
}

/**
 * Stop a signal watcher
 * @param w  Watcher to stop
 *
 * @return POSIX OK(0) or non-zero with @param errno set on error.
 */
int ml_signal_stop(ml_t *w)
{
	if (!_ml_watcher_active(w))
		return 0;

	if (_ml_watcher_stop(w))
		return -1;

	close(w->fd);
	w->fd = -1;

	return 0;
}

/**
 * Local Variables:
 *  indent-tabs-mode: t
 *  c-file-style: "linux"
 * End:
 */
