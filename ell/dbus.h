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

#ifndef __ELL_DBUS_H
#define __ELL_DBUS_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

enum l_dbus_bus {
	L_DBUS_SYSTEM_BUS,
	L_DBUS_SESSION_BUS,
};

struct l_dbus;

typedef void (*l_dbus_ready_func_t) (void *user_data);
typedef void (*l_dbus_disconnect_func_t) (void *user_data);

typedef void (*l_dbus_debug_func_t) (const char *str, void *user_data);
typedef void (*l_dbus_destroy_func_t) (void *user_data);

struct l_dbus *l_dbus_new(enum l_dbus_bus bus);
void l_dbus_destroy(struct l_dbus *dbus);

bool l_dbus_set_ready_handler(struct l_dbus *dbus, l_dbus_ready_func_t function,
				void *user_data, l_dbus_destroy_func_t destroy);
bool l_dbus_set_disconnect_handler(struct l_dbus *dbus,
				l_dbus_disconnect_func_t function,
				void *user_data, l_dbus_destroy_func_t destroy);

bool l_dbus_set_debug(struct l_dbus *dbus, l_dbus_debug_func_t function,
				void *user_data, l_dbus_destroy_func_t destroy);

struct l_dbus_message;

struct l_dbus_message *l_dbus_message_new_method_call(const char *destination,
		const char *path, const char *interface, const char *method);

struct l_dbus_message *l_dbus_message_ref(struct l_dbus_message *message);
void l_dbus_message_unref(struct l_dbus_message *message);

const char *l_dbus_message_get_path(struct l_dbus_message *message);
const char *l_dbus_message_get_interface(struct l_dbus_message *message);
const char *l_dbus_message_get_member(struct l_dbus_message *message);
const char *l_dbus_message_get_destination(struct l_dbus_message *message);
const char *l_dbus_message_get_sender(struct l_dbus_message *message);

typedef void (*l_dbus_message_func_t) (struct l_dbus_message *message,
							void *user_data);

uint32_t l_dbus_send(struct l_dbus *dbus, struct l_dbus_message *message,
				l_dbus_message_func_t function,
				void *user_data, l_dbus_destroy_func_t destroy);
bool l_dbus_cancel(struct l_dbus *dbus, uint32_t serial);

unsigned int l_dbus_register(struct l_dbus *dbus,
				l_dbus_message_func_t function,
				void *user_data, l_dbus_destroy_func_t destroy);
bool l_dbus_unregister(struct l_dbus *dbus, unsigned int id);

#ifdef __cplusplus
}
#endif

#endif /* __ELL_DBUS_H */
