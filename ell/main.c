/*
 *
 *  Embedded Linux library
 *
 *  Copyright (C) 2011-2014  Intel Corporation. All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <limits.h>
#include <sys/epoll.h>

#include "signal.h"
#include "queue.h"
#include "log.h"
#include "util.h"
#include "main.h"
#include "private.h"

/**
 * SECTION:main
 * @short_description: Main loop handling
 *
 * Main loop handling
 */

#define MAX_EPOLL_EVENTS 10

#define IDLE_FLAG_DISPATCHING	1
#define IDLE_FLAG_DESTROYED	2

static int epoll_fd;
static bool epoll_running;
static bool epoll_terminate;
static int idle_id;
static int exit_status = EXIT_FAILURE;

static struct l_queue *idle_list;

struct watch_data {
	int fd;
	uint32_t events;
	watch_event_cb_t callback;
	watch_destroy_cb_t destroy;
	void *user_data;
};

#define DEFAULT_WATCH_ENTRIES 128

static unsigned int watch_entries;
static struct watch_data **watch_list;

struct idle_data {
	idle_event_cb_t callback;
	idle_destroy_cb_t destroy;
	void *user_data;
	uint32_t flags;
	int id;
};

static inline bool __attribute__ ((always_inline)) create_epoll(void)
{
	unsigned int i;

	if (likely(epoll_fd))
		return true;

	epoll_fd = epoll_create1(EPOLL_CLOEXEC);
	if (epoll_fd < 0) {
		epoll_fd = 0;
		return false;
	}

	watch_list = malloc(DEFAULT_WATCH_ENTRIES * sizeof(void *));
	if (!watch_list)
		goto close_epoll;

	idle_list = l_queue_new();
	if (!idle_list)
		goto free_watch_list;

	idle_id = 0;

	watch_entries = DEFAULT_WATCH_ENTRIES;

	for (i = 0; i < watch_entries; i++)
		watch_list[i] = NULL;

	return true;

free_watch_list:
	free(watch_list);
	watch_list = NULL;

close_epoll:
	close(epoll_fd);
	epoll_fd = 0;

	return false;
}

int watch_add(int fd, uint32_t events, watch_event_cb_t callback,
				void *user_data, watch_destroy_cb_t destroy)
{
	struct watch_data *data;
	struct epoll_event ev;
	int err;

	if (unlikely(fd < 0 || !callback))
		return -EINVAL;

	if (!create_epoll())
		return -EIO;

	if ((unsigned int) fd > watch_entries - 1)
		return -ERANGE;

	data = l_new(struct watch_data, 1);

	data->fd = fd;
	data->events = events;
	data->callback = callback;
	data->destroy = destroy;
	data->user_data = user_data;

	memset(&ev, 0, sizeof(ev));
	ev.events = events;
	ev.data.ptr = data;

	err = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, data->fd, &ev);
	if (err < 0) {
		l_free(data);
		return err;
	}

	watch_list[fd] = data;

	return 0;
}

int watch_modify(int fd, uint32_t events, bool force)
{
	struct watch_data *data;
	struct epoll_event ev;
	int err;

	if (unlikely(fd < 0))
		return -EINVAL;

	if ((unsigned int) fd > watch_entries - 1)
		return -ERANGE;

	data = watch_list[fd];
	if (!data)
		return -ENXIO;

	if (data->events == events && !force)
		return 0;

	memset(&ev, 0, sizeof(ev));
	ev.events = events;
	ev.data.ptr = data;

	err = epoll_ctl(epoll_fd, EPOLL_CTL_MOD, data->fd, &ev);
	if (err < 0)
		return err;

	data->events = events;

	return 0;
}

int watch_remove(int fd)
{
	struct watch_data *data;
	int err;

	if (unlikely(fd < 0))
		return -EINVAL;

	if ((unsigned int) fd > watch_entries - 1)
		return -ERANGE;

	data = watch_list[fd];
	if (!data)
		return -ENXIO;

	watch_list[fd] = NULL;

	err = epoll_ctl(epoll_fd, EPOLL_CTL_DEL, data->fd, NULL);

	if (data->destroy)
		data->destroy(data->user_data);

	l_free(data);

	return err;
}

static bool idle_remove_by_id(void *data, void *user_data)
{
	struct idle_data *idle = data;
	int id = L_PTR_TO_INT(user_data);

	if (idle->id != id)
		return false;

	if (idle->destroy)
		idle->destroy(idle->user_data);

	if (idle->flags & IDLE_FLAG_DISPATCHING) {
		idle->flags |= IDLE_FLAG_DESTROYED;
		return false;
	}

	l_free(idle);

	return true;
}

static bool idle_prune(void *data, void *user_data)
{
	struct idle_data *idle = data;

	if ((idle->flags & IDLE_FLAG_DESTROYED) == 0)
		return false;

	l_free(idle);

	return true;
}

int idle_add(idle_event_cb_t callback, void *user_data,
		idle_destroy_cb_t destroy)
{
	struct idle_data *data;

	if (unlikely(!callback))
		return -EINVAL;

	if (!create_epoll())
		return -EIO;

	data = l_new(struct idle_data, 1);

	data->callback = callback;
	data->destroy = destroy;
	data->user_data = user_data;

	if (!l_queue_push_tail(idle_list, data)) {
		l_free(data);
		return -ENOMEM;
	}

	data->id = idle_id++;

	if (idle_id == INT_MAX)
		idle_id = 0;

	return data->id;
}

void idle_remove(int id)
{
	l_queue_foreach_remove(idle_list, idle_remove_by_id,
					L_INT_TO_PTR(id));
}

static void idle_destroy(void *data)
{
	struct idle_data *idle = data;

	l_error("Dangling idle descriptor %p, %d found", data, idle->id);

	if (idle->destroy)
		idle->destroy(idle->user_data);

	l_free(idle);
}

static void idle_dispatch(void *data, void *user_data)
{
	struct idle_data *idle = data;

	if (!idle->callback)
		return;

	idle->flags |= IDLE_FLAG_DISPATCHING;
	idle->callback(idle->user_data);
	idle->flags &= ~IDLE_FLAG_DISPATCHING;
}

/**
 * l_main_run:
 *
 * Run the main loop
 *
 * Returns: #EXIT_SCUCESS after successful execution or #EXIT_FAILURE in
 *          case of failure
 **/
LIB_EXPORT int l_main_run(void)
{
	unsigned int i;

	if (unlikely(epoll_running))
		return EXIT_FAILURE;

	if (!create_epoll())
		return EXIT_FAILURE;

	epoll_terminate = false;

	epoll_running = true;

	for (;;) {
		struct epoll_event events[MAX_EPOLL_EVENTS];
		int n, nfds;
		int timeout;

		if (epoll_terminate)
			break;

		timeout = l_queue_isempty(idle_list) ? -1 : 0;
		nfds = epoll_wait(epoll_fd, events, MAX_EPOLL_EVENTS, timeout);

		for (n = 0; n < nfds; n++) {
			struct watch_data *data = events[n].data.ptr;

			data->callback(data->fd, events[n].events,
							data->user_data);
		}

		l_queue_foreach(idle_list, idle_dispatch, NULL);
		l_queue_foreach_remove(idle_list, idle_prune, NULL);
	}

	for (i = 0; i < watch_entries; i++) {
		struct watch_data *data = watch_list[i];

		if (!data)
			continue;

		epoll_ctl(epoll_fd, EPOLL_CTL_DEL, data->fd, NULL);

		if (data->destroy)
			data->destroy(data->user_data);
		else
			l_error("Dangling file descriptor %d found", data->fd);

		l_free(data);
	}

	watch_entries = 0;

	free(watch_list);
	watch_list = NULL;

	l_queue_destroy(idle_list, idle_destroy);
	idle_list = NULL;

	epoll_running = false;

	close(epoll_fd);
	epoll_fd = 0;

	return exit_status;
}

/**
 * l_main_quit:
 *
 * Teminate the running main loop
 *
 * Returns: #true when terminating the main loop or #false in case of failure
 **/
LIB_EXPORT bool l_main_quit(void)
{
	if (unlikely(!epoll_running))
		return false;

	exit_status = EXIT_SUCCESS;
	epoll_terminate = true;

	return true;
}

struct signal_data {
	l_main_signal_cb_t callback;
	void *user_data;
};

static void signal_handler(struct l_signal *signal, uint32_t signo,
							void *user_data)
{
	struct signal_data *data = user_data;

	if (data->callback)
		data->callback(signo, data->user_data);
}

/**
 * l_main_run_with_signal:
 *
 * Run the main loop with signal handling for SIGINT and SIGTERM
 *
 * Returns: #EXIT_SCUCESS after successful execution or #EXIT_FAILURE in
 *          case of failure
 **/
LIB_EXPORT
int l_main_run_with_signal(l_main_signal_cb_t callback, void *user_data)
{
	struct signal_data *data;
	struct l_signal *signal;
	sigset_t mask;
	int result;

	data = l_new(struct signal_data, 1);

	data->callback = callback;
	data->user_data = user_data;

	sigemptyset(&mask);
	sigaddset(&mask, SIGINT);
	sigaddset(&mask, SIGTERM);

	signal = l_signal_create(&mask, signal_handler, data, l_free);

	result = l_main_run();

	l_signal_remove(signal);

	return result;
}

/**
 * l_main_exit:
 *
 * Teminate the running main loop with exit status
 *
 **/
LIB_EXPORT
void l_main_exit(int status)
{
	if (unlikely(!epoll_running))
		return;

	exit_status = status;
	epoll_terminate = true;
}
