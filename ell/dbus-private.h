/*
 *
 *  Embedded Linux library
 *
 *  Copyright (C) 2011-2012  Intel Corporation. All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License version 2.1 as published by the Free Software Foundation.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

struct l_string;
struct l_dbus_interface;
struct _dbus_method;
struct _dbus_signal;
struct _dbus_property;

struct l_dbus_message *dbus_message_build(const void *data, size_t size);
bool dbus_message_compare(struct l_dbus_message *message,
					const void *data, size_t size);

const char *_dbus_signature_end(const char *signature);

bool _dbus_valid_object_path(const char *path);
bool _dbus_valid_signature(const char *sig);
bool _dbus_valid_interface(const char *interface);
bool _dbus_valid_method(const char *method);

void _dbus_method_introspection(struct _dbus_method *info,
					struct l_string *buf);
void _dbus_signal_introspection(struct _dbus_signal *info,
					struct l_string *buf);
void _dbus_property_introspection(struct _dbus_property *info,
						struct l_string *buf);
void _dbus_interface_introspection(struct l_dbus_interface *interface,
						struct l_string *buf);

struct l_dbus_interface *_dbus_interface_new(const char *interface);
void _dbus_interface_free(struct l_dbus_interface *interface);

struct _dbus_method *_dbus_interface_find_method(struct l_dbus_interface *i,
							const char *method);
struct _dbus_signal *_dbus_interface_find_signal(struct l_dbus_interface *i,
							const char *signal);
struct _dbus_property *_dbus_interface_find_property(struct l_dbus_interface *i,
						const char *property);

struct _dbus_object_tree *_dbus_object_tree_new();
void _dbus_object_tree_free(struct _dbus_object_tree *tree);

bool _dbus_object_tree_register(struct _dbus_object_tree *tree,
				const char *path, const char *interface,
				void (*setup_func)(struct l_dbus_interface *),
				void *user_data, void (*destroy) (void *));
bool _dbus_object_tree_unregister(struct _dbus_object_tree *tree,
					const char *path,
					const char *interface);
