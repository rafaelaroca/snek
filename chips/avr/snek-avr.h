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

#ifndef _SNEK_AVR_H_
#define _SNEK_AVR_H_

#include <avr/io.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <math.h>

#define sqrtf snek_sqrtf

static inline float
sqrtf(float x) {
	return powf(x, 0.5f);
}

#define SNEK_DEBUG	0
#define strtof(a,b) strtod(a,b)
#define SNEK_POOL		1024
#define SNEK_MAX_TOKEN		63
#define VALUE_STACK_SIZE	16
#define PARSE_STACK_SIZE	128
#define SNEK_STACK		32
#define PARSE_TABLE_DECLARATION(t) 	PROGMEM t
#define PARSE_TABLE_FETCH_TOKEN(a)	((token_key_t) pgm_read_byte(a))
#define PARSE_TABLE_FETCH_INDEX(a)	((uint8_t) pgm_read_byte(a))
#define ERROR_FETCH_FORMAT_CHAR(a)	((char) pgm_read_byte(a))

/* no sense linking both functions */
#define memcpy(a,b,c) memmove(a,b,c)

#define snek_error_name snek_internal_error
#define snek_error(fmt, args...) ({				\
		static const char PROGMEM __fmt__[] = (fmt);	\
		snek_internal_error(__fmt__, ## args);		\
	})

#define snek_error_0_name snek_internal_error_0
#define snek_error_0(string) ({						\
			static const char PROGMEM __string__[] = (string); \
			snek_internal_error_0(__string__);		\
		})

#define fprintf(file, fmt, args...) ({				\
		static const char PROGMEM __fmt__[] = (fmt);	\
		fprintf_P(file, __fmt__, ## args);		\
	})

#define sprintf_const(dst, fmt, args...) ({			\
		static const char PROGMEM __fmt__[] = (fmt);	\
		sprintf_P(dst, __fmt__, ##args);		\
	})
#define strfromf(dst, len, fmt, val) sprintf_const(dst, fmt, val)

#define SNEK_BUILTIN_NAMES_DECLARE(n) 	PROGMEM n
#define SNEK_BUILTIN_NAMES(a)		((uint8_t) pgm_read_byte(&snek_builtin_names[a]))
#define SNEK_BUILTIN_NAMES_CMP(a,b)	strcmp_P(a,b)

#define SNEK_BUILTIN_DECLARE(n)	PROGMEM n
#define SNEK_BUILTIN_NFORMAL(b) ((int8_t) pgm_read_byte(&(b)->nformal))
#define SNEK_BUILTIN_FUNCV(b)	((snek_poly_t(*)(uint8_t, uint8_t, snek_poly_t *)) pgm_read_ptr(&(b)->funcv))
#define SNEK_BUILTIN_FUNC0(b) 	((snek_poly_t(*)(void)) pgm_read_ptr(&(b)->func0))
#define SNEK_BUILTIN_FUNC1(b) 	((snek_poly_t(*)(snek_poly_t)) pgm_read_ptr(&(b)->func1))
#define SNEK_BUILTIN_FUNC2(b) 	((snek_poly_t(*)(snek_poly_t, snek_poly_t)) pgm_read_ptr(&(b)->func2))
#define SNEK_BUILTIN_FUNC3(b) 	((snek_poly_t(*)(snek_poly_t, snek_poly_t, snek_poly_t)) pgm_read_ptr(&(b)->func3))
#define SNEK_BUILTIN_FUNC4(b) 	((snek_poly_t(*)(snek_poly_t, snek_poly_t, snek_poly_t, snek_poly_t)) pgm_read_ptr(&(b)->func4))
#define SNEK_BUILTIN_VALUE(b)	((snek_poly_t)(uint32_t)pgm_read_dword(&(b)->value))

#define SNEK_ROOT_DECLARE(n)	PROGMEM n
#define SNEK_ROOT_TYPE(n) 	((const snek_mem_t *) pgm_read_ptr(&(n)->type))
#define SNEK_ROOT_ADDR(n) 	((void **) pgm_read_ptr(&(n)->addr))

static inline const char *
avr_snek_builtin_names_return(const uint8_t *bits)
{
	static char ret[SNEK_BUILTIN_NAMES_MAX_LEN + 1];
	char *r = ret;

	while ((*r++ = (char) pgm_read_byte(bits++)))
		;
	return ret;
}

#define snek_builtin_names_return(a) avr_snek_builtin_names_return(a)
#define snek_builtin_names_len(a) strnlen_P(a, SNEK_BUILTIN_NAMES_MAX_LEN+1)

#define SNEK_MEM_DECLARE(n) 	PROGMEM n
#define SNEK_MEM_SIZE(m)	((snek_offset_t (*)(void *addr)) pgm_read_word(&(m)->size))
#define SNEK_MEM_MARK(m)	((void (*)(void *addr)) pgm_read_ptr(&(m)->mark))
#define SNEK_MEM_MOVE(m)	((void (*)(void *addr)) pgm_read_ptr(&(m)->move))

#define SNEK_MEMS_DECLARE(n)	PROGMEM n

int
snek_eeprom_getchar(FILE *stream);

#define SNEK_IO_LINEBUF 80

extern FILE snek_avr_file;

#endif /* _SNEK_AVR_H_ */
