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

#include <stdio.h>

#include <ell/ell.h>

static void print_modalias(struct l_hwdb *hwdb, const char *format, ...)
{
	struct l_hwdb_entry *entries, *entry;
	va_list args;

	va_start(args, format);
	vprintf(format, args);
	va_end(args);

	printf("\n");

	va_start(args, format);
	entries = l_hwdb_lookup_valist(hwdb, format, args);
	va_end(args);

	for (entry = entries; entry; entry = entry->next)
		printf(" %s=%s\n", entry->key, entry->value);

	l_hwdb_lookup_free(entries);

	printf("\n");
}

static void print_entry(const char *modalias,
				struct l_hwdb_entry *entries, void *user_data)
{
	struct l_hwdb_entry *entry;

	printf("%s\n", modalias);

	for (entry = entries; entry; entry = entry->next)
		printf(" %s=%s\n", entry->key, entry->value);

	printf("\n");
}

int main(int argc, char *argv[])
{
	struct l_hwdb *hwdb;

	hwdb = l_hwdb_new_default();
	if (!hwdb)
		return 0;

	l_hwdb_foreach(hwdb, print_entry, NULL);

	/* Bluetooth Interest Group Inc. */
	print_modalias(hwdb, "OUI:000F79");

	/* Bluetooth SIG, Inc. */
	print_modalias(hwdb, "bluetooth:v%04X", 0x003f);

	/* Nike+ FuelBand */
	print_modalias(hwdb, "bluetooth:v%04Xp%04X", 0x0078, 0x0001);

	/* Bluetooth Type-A standard interface */
	print_modalias(hwdb, "sdio:c02");

	l_hwdb_unref(hwdb);

	return 0;
}