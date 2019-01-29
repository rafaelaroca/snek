/*
 * Copyright © 2018 Keith Packard <keithp@keithp.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 */

#include "snek.h"

snek_frame_t	*snek_globals;
snek_frame_t	*snek_frame;

static inline snek_frame_t *snek_pick_frame(bool globals)
{
	if (globals)
		return snek_globals;
	return snek_frame;
}

static snek_frame_t *
snek_frame_realloc(bool globals, snek_offset_t nvariables)
{
	snek_frame_t *frame = snek_alloc(sizeof (snek_frame_t) +
					 nvariables * sizeof (snek_variable_t));

	if (!frame)
		return NULL;

	snek_frame_t *old_frame = snek_pick_frame(globals);
	memcpy(frame, old_frame, sizeof (snek_frame_t));
	frame->nvariables = nvariables;
	return frame;
}

static inline snek_variable_t *
snek_variable_insert(bool globals)
{
	snek_frame_t	*old_frame = snek_pick_frame(globals);
	snek_frame_t	*frame;
	snek_offset_t	nvariables = old_frame->nvariables + 1;

	frame = snek_frame_realloc(globals, old_frame->nvariables + 1);
	if (!frame)
		return NULL;
	old_frame = snek_pick_frame(globals);
	memcpy(frame->variables,
	       old_frame->variables,
	       old_frame->nvariables * sizeof (snek_variable_t));

	if (globals)
		snek_globals = frame;
	else
		snek_frame = frame;
	return &frame->variables[nvariables-1];
}

static inline void
snek_variable_delete(snek_offset_t i)
{
	snek_frame_t	*frame;

	frame = snek_frame_realloc(true, snek_globals->nvariables - 1);
	memcpy(&frame->variables[0],
	       &snek_globals->variables[0],
	       i * sizeof (snek_variable_t));
	memcpy(&frame->variables[i],
	       &snek_globals->variables[i+1],
	       snek_globals->nvariables - i - 1);
}

static snek_variable_t *
snek_variable_lookup(bool globals, snek_id_t id, bool insert)
{
	snek_offset_t	i;
	snek_frame_t	*frame;

	frame = snek_pick_frame(globals);
	for (i = 0; i < frame->nvariables; i++) {
		if (frame->variables[i].id == id)
			return &frame->variables[i];
	}
	if (!insert)
		return NULL;

	snek_variable_t *v = snek_variable_insert(globals);
	if (!v)
		return NULL;

	v->id = id;
	return v;
}

static snek_variable_t *
snek_frame_lookup(snek_id_t id, bool insert)
{
	snek_variable_t	*v = NULL;

	if (snek_frame && (v = snek_variable_lookup(false, id, insert))) {
		if (!snek_is_global(v->value))
			return v;
	}
	if (insert && !snek_globals)
		snek_globals = snek_alloc(sizeof (snek_frame_t));
	if (snek_globals && (v = snek_variable_lookup(true, id, insert)))
		return v;
	return v;
}

bool
snek_frame_mark_global(snek_id_t id)
{
	if (snek_frame) {
		snek_variable_t *v;

		v = snek_variable_lookup(false, id, true);
		if (!v)
			return false;
		v->value = SNEK_GLOBAL;
	}
	return true;
}

snek_frame_t *
snek_frame_push(snek_code_t *code, snek_offset_t ip, snek_offset_t nformal)
{
	snek_frame_t *f;

	snek_code_stash(code);
	f = snek_alloc(sizeof (snek_frame_t) + nformal * sizeof (snek_variable_t));
	code = snek_code_fetch();
	if (!f)
		return NULL;
	f->nvariables = nformal;
	f->code = snek_pool_offset(code);
	f->ip = ip;
	f->prev = snek_pool_offset(snek_frame);
	snek_frame = f;
	return f;
}

snek_code_t *
snek_frame_pop(snek_offset_t *ip_p)
{
	if (!snek_frame)
		return NULL;

	snek_code_t	*code = snek_pool_ref(snek_frame->code);
	snek_offset_t	ip = snek_frame->ip;

	snek_frame = snek_pool_ref(snek_frame->prev);

	*ip_p = ip;
	return code;
}

snek_poly_t *
snek_id_ref(snek_id_t id, bool insert)
{
	snek_variable_t *v = snek_frame_lookup(id, insert);
	if (!v)
		return NULL;
	return &v->value;
}

bool
snek_id_is_local(snek_id_t id)
{
	return snek_variable_lookup(false, id, false) != NULL;
}

bool
snek_id_del(snek_id_t id)
{
	snek_offset_t i;

	for (i = 0; i < snek_globals->nvariables; i++)
		if (snek_globals->variables[i].id == id) {
			snek_variable_delete(i);
			return true;
		}

	return false;
}

static snek_offset_t
snek_frame_size(void *addr)
{
	snek_frame_t *frame = addr;

	return sizeof (snek_frame_t) + frame->nvariables * sizeof (snek_variable_t);
}

static void
snek_frame_mark(void *addr)
{
	snek_frame_t *f = addr;

	for (;;) {
		debug_memory("\t\tframe mark vars %d code %d prev %d\n",
			     f->variables, f->code, f->prev);
		snek_offset_t i;
		for (i = 0; i < f->nvariables; i++)
			if (!snek_is_global(f->variables[i].value))
				snek_poly_mark(f->variables[i].value);
		if (f->code)
			snek_mark_offset(&snek_code_mem, f->code);
		if (!f->prev || snek_mark_block_offset(&snek_frame_mem, f->prev))
			break;
		f = snek_pool_ref(f->prev);
	}
}

static void
snek_frame_move(void *addr)
{
	snek_frame_t *f = addr;

	for (;;) {
		debug_memory("\t\tframe move vars %d code %d prev %d\n",
			     f->variables, f->code, f->prev);
		snek_offset_t i;
		for (i = 0; i < f->nvariables; i++)
			if (!snek_is_global(f->variables[i].value))
				snek_poly_move(&f->variables[i].value);
		if (f->code)
			snek_move_offset(&snek_code_mem, &f->code);
		if (!f->prev || snek_move_block_offset(&f->prev))
			break;
		f = snek_pool_ref(f->prev);
	}
}

const snek_mem_t SNEK_MEM_DECLARE(snek_frame_mem) = {
	.size = snek_frame_size,
	.mark = snek_frame_mark,
	.move = snek_frame_move,
	SNEK_MEM_DECLARE_NAME("frame")
};