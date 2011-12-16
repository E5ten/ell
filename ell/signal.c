/*
 *
 *  Embedded Linux library
 *
 *  Copyright (C) 2011  Intel Corporation. All rights reserved.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/signalfd.h>

#include "signal.h"
#include "private.h"

struct l_signal {
	int fd;
	sigset_t oldmask;
	l_signal_notify_cb_t callback;
	l_signal_destroy_cb_t destroy;
	void *user_data;
};

static void signal_destroy(void *user_data)
{
	struct l_signal *signal = user_data;

	close(signal->fd);

	if (signal->destroy)
		signal->destroy(signal->user_data);

	free(signal);
}

static void signal_callback(int fd, uint32_t events, void *user_data)
{
	struct l_signal *signal = user_data;
	struct signalfd_siginfo si;
	ssize_t result;

	result = read(signal->fd, &si, sizeof(si));
	if (result != sizeof(si))
		return;

	if (signal->callback)
		signal->callback(signal, si.ssi_signo, signal->user_data);
}

LIB_EXPORT struct l_signal *l_signal_create(const sigset_t *mask,
			l_signal_notify_cb_t callback,
			void *user_data, l_signal_destroy_cb_t destroy)
{
	struct l_signal *signal;

	signal = malloc(sizeof(struct l_signal));
	if (!signal)
		return NULL;

	memset(signal, 0, sizeof(struct l_signal));
	signal->callback = callback;
	signal->destroy = destroy;
	signal->user_data = user_data;

	if (sigprocmask(SIG_BLOCK, mask, &signal->oldmask) < 0) {
		free(signal);
		return NULL;
	}

	signal->fd = signalfd(-1, mask, SFD_NONBLOCK | SFD_CLOEXEC);
	if (signal->fd < 0) {
		free(signal);
		return NULL;
	}

	watch_add(signal->fd, EPOLLIN, signal_callback,
					signal, signal_destroy);

	return signal;
}

LIB_EXPORT void l_signal_remove(struct l_signal *signal)
{
	if (!signal)
		return;

	watch_remove(signal->fd);
}
