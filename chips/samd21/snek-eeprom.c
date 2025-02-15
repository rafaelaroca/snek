/*
 * Copyright © 2019 Keith Packard <keithp@keithp.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 */

#include <ao.h>
#include <ao-snek.h>
#include <ao-flash.h>
#include <snek.h>
#include <snek-io.h>

snek_poly_t
snek_builtin_eeprom_write(void)
{
	uint8_t c;
	snek_offset_t addr = 0;

	ao_flash_write_init();
	for (;;) {
		c = snek_raw_getc(stdin);
		if (c == ('d' & 0x1f))
			c = 0xff;
		ao_flash_write_byte(c);
		if (c == 0xff)
			break;
		++addr;
	}
	ao_flash_write_flush();
	return SNEK_NULL;
}

snek_poly_t
snek_builtin_eeprom_show(uint8_t nposition, uint8_t nnamed, snek_poly_t *args)
{
	uint8_t c;

	(void) nnamed;
	(void) args;
	if (nposition)
		putc('b' & 0x1f, stdout);
	ao_flash_read_init();
	for (;;) {
		c = ao_flash_read_byte();
		if (c == 0xff)
			break;
		putc(c, stdout);
	}
	if (nposition)
		putc('c' & 0x1f, stdout);
	return SNEK_NULL;
}

static int (*save_getc)(FILE *file);

snek_poly_t
snek_builtin_eeprom_erase(void)
{
	ao_flash_erase_all();
	return SNEK_NULL;
}

void
snek_eeprom_load(void)
{
	snek_interactive = false;
	ao_flash_read_init();
	save_getc = __iob[0]->get;
	__iob[0]->get = snek_eeprom_getchar;
}

int
snek_eeprom_getchar(FILE *stream)
{
	(void) stream;
	if (!snek_abort) {
		uint8_t c = ao_flash_read_byte();
		if (c != 0xff)
			return c;
	}
	snek_interactive = true;
	__iob[0]->get = save_getc;
	return EOF;
}
