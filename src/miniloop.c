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
#include <fcntl.h>		/* O_CLOEXEC */
#include <string.h>		/* memset() */
#include <sys/epoll.h>
#include <sys/inotify.h>
#include <sys/ioctl.h>
#include <sys/select.h>		/* for select() workaround */
#include <sys/signalfd.h>	/* struct signalfd_siginfo */
#include <unistd.h>		/* close(), read() */

#include "miniloop.h"


static int _init(ml_ctx_t *ctx, int close_old)
{
	int fd;
	int inotify_fd;

	fd = epoll_create1(EPOLL_CLOEXEC);
  inotify_fd = inotify_init1(IN_NONBLOCK | IN_CLOEXEC);

  /* 
   * We should only need one inotify_fd,
   * subsequent watches can be added to it.
   * */

  if (fd < 0 || inotify_fd < 0) {
		return -1;
  }

	if (close_old) {
		close(ctx->inotify_fd);
		close(ctx->fd);
  }

	ctx->inotify_fd = inotify_fd;
	ctx->fd = fd;

	return 0;
}

/* Used by file i/o workaround when epoll => EPERM */
static int has_data(int fd)
{
	struct timeval timeout = { 0, 0 };
	fd_set fds;
	int n = 0;

	FD_ZERO(&fds);
	FD_SET(fd, &fds);

	if (select(1, &fds, NULL, NULL, &timeout) > 0)
		return ioctl(0, FIONREAD, &n) == 0 && n > 0;

	return 0;
}

/* Private to miniloop, do not use directly! */
int _ml_watcher_init(ml_ctx_t *ctx, ml_t *w, ml_type_t type, ml_cb_t *cb, void *arg, int fd, int events)
{
	if (!ctx || !w) {
		errno = EINVAL;
		return -1;
	}

	w->ctx    = ctx;
	w->type   = type;
	w->active = 0;
	w->fd     = fd;
	w->cb     = cb;
	w->arg    = arg;
	w->events = events;

	return 0;
}

/* Private to miniloop, do not use directly! */
int _ml_watcher_start(ml_t *w)
{
	struct epoll_event ev;

	if (!w || w->fd < 0 || !w->ctx) {
		errno = EINVAL;
		return -1;
	}

	if (_ml_watcher_active(w))
		return 0;

	ev.events   = w->events | EPOLLRDHUP;
	ev.data.ptr = w;
	if (epoll_ctl(w->ctx->fd, EPOLL_CTL_ADD, w->fd, &ev) < 0) {
		if (errno != EPERM)
			return -1;

		/* Handle special case: `application < file.txt` */
		if (w->type != MINILOOP_IO_TYPE || w->events != MINILOOP_READ)
			return -1;

		/* Only allow this special handling for stdin */
		if (w->fd != STDIN_FILENO)
			return -1;

		w->ctx->workaround = 1;
		w->active = -1;
	} else {
		w->active = 1;
	}

	/* Add to internal list for bookkeeping */
	_MINILOOP_INSERT(w, w->ctx->watchers);

	return 0;
}

/* Private to miniloop, do not use directly! */
int _ml_watcher_stop(ml_t *w)
{
	if (!w) {
		errno = EINVAL;
		return -1;
	}

	if (!_ml_watcher_active(w))
		return 0;

	w->active = 0;

	/* Remove from internal list */
	_MINILOOP_REMOVE(w, w->ctx->watchers);

	/* Remove from kernel */
	if (epoll_ctl(w->ctx->fd, EPOLL_CTL_DEL, w->fd, NULL) < 0)
		return -1;

	return 0;
}

/* Private to miniloop, do not use directly! */
int _ml_watcher_active(ml_t *w)
{
	if (!w)
		return 0;

	return w->active > 0;
}

/* Private to miniloop, do not use directly! */
int _ml_watcher_rearm(ml_t *w)
{
	struct epoll_event ev;

	if (!w || w->fd < 0) {
		errno = EINVAL;
		return -1;
	}

	ev.events   = w->events | EPOLLRDHUP;
	ev.data.ptr = w;
	if (epoll_ctl(w->ctx->fd, EPOLL_CTL_MOD, w->fd, &ev) < 0)
		return -1;

	return 0;
}

/**
 * Create an event loop context
 * @param ctx  Pointer to an ml_ctx_t context to be initialized
 * @param maxevents Maximum number of events in event cache
 *
 * This function is the same as @func ml_init() except for the
 * @param maxevents argument, which controls the number of events
 * in the event cache returned to the main loop.
 *
 * In cases where you have multiple events pending in the cache and some
 * event may cause later ones, already sent by the kernel to userspace,
 * to be deleted the pointer returned to the event loop for this later
 * event may be deleted.
 *
 * There are two ways around this (accessing deleted memory); 1) use
 * this function to initialize your event loop and set @param maxevents
 * to 1, 2) use a free list in you application that you garbage collect
 * at intervals relevant to your application.
 *
 * @return POSIX OK(0) on success, or non-zero on error.
 */
int ml_init(ml_ctx_t *ctx, int maxevents)
{
	if (!ctx || maxevents < 1) {
		errno = EINVAL;
		return -1;
	}

	memset(ctx, 0, sizeof(*ctx));
	ctx->maxevents = maxevents;

	return _init(ctx, 0);
}

/**
 * Terminate the event loop
 * @param ctx  A valid miniloop context
 *
 * @return POSIX OK(0) or non-zero with @param errno set on error.
 */
int ml_exit(ml_ctx_t *ctx)
{
	ml_t *w;

	if (!ctx) {
		errno = EINVAL;
		return -1;
	}

	_MINILOOP_FOREACH(w, ctx->watchers) {
		/* Remove from internal list */
		_MINILOOP_REMOVE(w, ctx->watchers);

		if (!_ml_watcher_active(w))
			continue;

		switch (w->type) {
    case MINILOOP_TIMER_TYPE:
			ml_timer_stop(w);
			break;

		case MINILOOP_IO_TYPE:
			ml_io_stop(w);
			break;

		case MINILOOP_SIGNAL_TYPE:
			ml_signal_stop(w);
			break;

    case MINILOOP_FS_TYPE:
      // FIXME
      break;

		case MINILOOP_EVENT_TYPE:
			ml_event_stop(w);
			break;
		}
	}

	ctx->watchers = NULL;
	ctx->running = 0;
	if (ctx->fd > -1)
		close(ctx->fd);
	ctx->fd = -1;

	return 0;
}

/**
 * Start the event loop
 * @param ctx    A valid miniloop context
 * @param flags  A mask of %MINILOOP_ONCE and %MINILOOP_NONBLOCK, or zero
 *
 * With @flags set to %MINILOOP_ONCE the event loop returns after the first
 * event has been served, useful for instance to set a timeout on a file
 * descriptor.  If @flags also has the %MINILOOP_NONBLOCK flag set the event
 * loop will return immediately if no event is pending, useful when run
 * inside another event loop.
 *
 * @return POSIX OK(0) upon successful termination of the event loop, or
 * non-zero on error.
 */
int ml_run(ml_ctx_t *ctx, int flags)
{
	ml_t *w;
	int timeout = -1;

        if (!ctx || ctx->fd < 0) {
		errno = EINVAL;
                return -1;
	}

	if (flags & MINILOOP_NONBLOCK)
		timeout = 0;

	/* Start the event loop */
	ctx->running = 1;

	/* Start all dormant timers */
	_MINILOOP_FOREACH(w, ctx->watchers) {
		if (MINILOOP_TIMER_TYPE == w->type)
			ml_timer_set(w, w->u.t.timeout, w->u.t.period);
	}

	while (ctx->running && ctx->watchers) {
		struct epoll_event ee[MINILOOP_MAX_EVENTS];
		int i, nfds, rerun = 0;

		/* Handle special case: `application < file.txt` */
		if (ctx->workaround) {
			_MINILOOP_FOREACH(w, ctx->watchers) {
				if (w->active != -1 || !w->cb)
					continue;

				if (!has_data(w->fd)) {
					w->active = 0;
					_MINILOOP_REMOVE(w, ctx->watchers);
				}

				rerun++;
				w->cb(w, w->arg, MINILOOP_READ);
			}
		}

		if (rerun)
			continue;
		ctx->workaround = 0;

		while ((nfds = epoll_wait(ctx->fd, ee, ctx->maxevents, timeout)) < 0) {
			if (!ctx->running)
				break;

			if (EINTR == errno)
				continue; /* Signalled, try again */

			/* Unrecoverable error, cleanup and exit with error. */
			ml_exit(ctx);

			return -2;
		}

		for (i = 0; ctx->running && i < nfds; i++) {
			struct signalfd_siginfo fdsi;
			uint32_t events;
			uint64_t exp;
			ssize_t sz = sizeof(fdsi);

			w = (ml_t *)ee[i].data.ptr;
			events = ee[i].events;

			switch (w->type) {
			case MINILOOP_IO_TYPE:
				if (events & (EPOLLHUP | EPOLLERR))
					ml_io_stop(w);
				break;

			case MINILOOP_SIGNAL_TYPE:
				if (read(w->fd, &fdsi, sz) != sz) {
					if (ml_signal_start(w)) {
						ml_signal_stop(w);
						events = MINILOOP_ERROR;
					}
				}
				break;

			case MINILOOP_TIMER_TYPE:
				if (read(w->fd, &exp, sizeof(exp)) != sizeof(exp)) {
					ml_timer_stop(w);
					events = MINILOOP_ERROR;
				}

				if (!w->u.t.period)
					w->u.t.timeout = 0;
				if (!w->u.t.timeout)
					ml_timer_stop(w);
				break;

      case MINILOOP_FS_TYPE:
        // FIXME
        break;

			case MINILOOP_EVENT_TYPE:
				if (read(w->fd, &exp, sizeof(exp)) != sizeof(exp))
					events = MINILOOP_HUP;
				break;
			}

			/*
			 * NOTE: Must be last action for watcher, the
			 *       callback may delete itself.
			 */
			if (w->cb)
				w->cb(w, w->arg, events & MINILOOP_EVENT_MASK);
		}

		if (flags & MINILOOP_ONCE)
			break;
	}

	return 0;
}

/**
 * Local Variables:
 *  indent-tabs-mode: t
 *  c-file-style: "linux"
 * End:
 */
