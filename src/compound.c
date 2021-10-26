/*
 * compound.c
 * This file is part of koa
 *
 * Copyright (C) 2018 - Gordon Li
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <string.h>

#include "compound.h"
#include "pool.h"
#include "error.h"

compound_t *
compound_new (const char *name)
{
	compound_t *meta;

	meta = (compound_t *) pool_calloc (1, sizeof (compound_t));
	if (meta == NULL) {
		fatal_error ("out of memory.");
	}

	meta->name = str_new (name, strlen (name));
	meta->fields = vec_new (0);

	return meta;
}

void
compound_free (compound_t *meta)
{
	size_t fields_len;

	str_free (meta->name);
	fields_len = vec_size (meta->fields);
	for (size_t i = 0; i < fields_len; i++) {
		field_t *field;

		field = (field_t *) vec_pos (meta->fields, (integer_value_t) i);
		if (field != NULL) {
			str_free (field->name);
			pool_free ((void *) field);
		}
	}
	vec_free (meta->fields);
	pool_free ((void *) meta);
}

void
compound_push_field (compound_t *meta, const char *name, object_type_t type)
{
	field_t *field;

	field = (field_t *) pool_calloc (1, sizeof (field_t));
	if (field == NULL) {
		fatal_error ("out of memory.");
	}

	field->name = str_new (name, strlen (name));
	field->type = type;
	if (!vec_push_back (meta->fields, (void *) field)) {
		fatal_error ("failed to add field.");
	}

	return;
}

static str_t *
compound_name_load_binary (FILE *f)
{
	size_t name_len;
	char *name;
	str_t *str;

	/* Read name. */
	if (fread (&name_len, sizeof (size_t), 1, f) != 1) {
		error ("failed to load size while load name.");

		return NULL;
	}

	/* Read content. */
	name = (char *) pool_alloc (name_len);
	if (name == NULL) {
		fatal_error ("out of memory.");
	}

	if (fread (name, sizeof (char), name_len, f) != name_len) {
		pool_free ((void *) name);
		error ("failed to load compound name content.");

		return NULL;
	}

	str = str_new (name, name_len);
	pool_free ((void *) name);

	return str;
}

static str_t *
compound_name_load_buf (const char **buf, size_t *len)
{
	size_t name_len;
	char *name;
	str_t *str;

	/* Read name. */
	if (*len < sizeof (size_t)) {
		error ("failed to load size while load name.");

		return NULL;
	}

	name_len = *(size_t *) *buf;
	*buf += sizeof (size_t);
	*len -= sizeof (size_t);

	/* Read content. */
	name = (char *) pool_alloc (name_len);
	if (name == NULL) {
		fatal_error ("out of memory.");
	}

	if (*len < name_len) {
		pool_free ((void *) name);
		error ("failed to load compound name content.");

		return NULL;
	}

	memcpy ((void *) name, *buf, name_len);

	str = str_new (name, name_len);
	pool_free ((void *) name);

	return str;
}

static field_t *
compound_field_load_binary (FILE *f)
{
	field_t *field;

	field = (field_t *) pool_calloc (1, sizeof (field_t));
	if (field == NULL) {
		fatal_error ("out of memory.");
	}

	/* Read name. */
	field->name = compound_name_load_binary (f);
	if (field->name == NULL) {
		pool_free ((void *) field);

		return NULL;
	}

	/* Read type. */
	if (fread (&field->type, sizeof (object_type_t), 1, f) != 1) {
		str_free (field->name);
		pool_free ((void *) field);
		error ("failed to load type while load compound field.");

		return NULL;
	}

	return field;
}

static field_t *
compound_field_load_buf (const char **buf, size_t *len)
{
	field_t *field;

	field = (field_t *) pool_calloc (1, sizeof (field_t));
	if (field == NULL) {
		fatal_error ("out of memory.");
	}

	/* Read name. */
	field->name = compound_name_load_buf (buf, len);
	if (field->name == NULL) {
		pool_free ((void *) field);

		return NULL;
	}

	/* Read type. */
	if (*len < sizeof (object_type_t)) {
		str_free (field->name);
		pool_free ((void *) field);
		error ("failed to load type while load compound field.");

		return NULL;
	}

	field->type = *(object_type_t *) *buf;
	*buf += sizeof (object_type_t);
	*len -= sizeof (object_type_t);

	return field;
}

compound_t *
compound_load_binary (FILE *f)
{
	compound_t *meta;
	size_t fields_len;

	meta = (compound_t *) pool_calloc (1, sizeof (compound_t));
	if (meta == NULL) {
		fatal_error ("out of memory.");
	}

	/* Read name. */
	meta->name = compound_name_load_binary (f);
	if (meta->name == NULL) {
		pool_free ((void *) meta);

		return NULL;
	}
	/* Read fields length. */
	if (fread (&fields_len, sizeof (size_t), 1, f) != 1) {
		str_free (meta->name);
		pool_free ((void *) meta);
		error ("failed to load size while load compound fields.");

		return NULL;
	}
	meta->fields = vec_new (fields_len);
	/* Read fields content. */
	for (size_t i = 0; i < fields_len; i++) {
		field_t *field;

		field = compound_field_load_binary (f);
		if (field == NULL) {
			compound_free (meta);

			return NULL;
		}
		UNUSED (vec_set (meta->fields, (integer_value_t) i, (void *) field));
	}

	return meta;
}

compound_t *
compound_load_buf (const char **buf, size_t *len)
{
	compound_t *meta;
	size_t fields_len;

	meta = (compound_t *) pool_calloc (1, sizeof (compound_t));
	if (meta == NULL) {
		fatal_error ("out of memory.");
	}

	/* Read name. */
	meta->name = compound_name_load_buf (buf, len);
	if (meta->name == NULL) {
		pool_free ((void *) meta);

		return NULL;
	}

	/* Read fields length. */
	if (*len < sizeof (size_t)) {
		str_free (meta->name);
		pool_free ((void *) meta);
		error ("failed to load size while load compound fields.");

		return NULL;
	}

	fields_len = *(size_t *) *buf;
	*buf += sizeof (size_t);
	*len -= sizeof (size_t);

	meta->fields = vec_new (fields_len);
	/* Read fields content. */
	for (size_t i = 0; i < fields_len; i++) {
		field_t *field;

		field = compound_field_load_buf (buf, len);
		if (field == NULL) {
			compound_free (meta);

			return NULL;
		}
		UNUSED (vec_set (meta->fields, (integer_value_t) i, (void *) field));
	}

	return meta;
}

static void *
compound_save_name (str_t *str, void *pos)
{
	size_t name_len;

	name_len = str_len (str);
	memcpy (pos, (void *) &name_len, (unsigned) sizeof (size_t));
	pos +=  sizeof (size_t);
	memcpy (pos, (void *) str_c_str (str), (unsigned) name_len);
	pos += name_len;

	return pos;
}

str_t *
compound_to_binary (compound_t *meta)
{
	size_t fields_len;
	size_t total;
	char *buf;
	void *pos;
	str_t *str;

	total = sizeof (size_t) + str_len (meta->name) + sizeof (size_t);
	fields_len = vec_size (meta->fields);
	for (size_t i = 0; i < fields_len; i++) {
		field_t *field;

		field = (field_t *) vec_pos (meta->fields, (integer_value_t) i);
		total += sizeof (size_t) + str_len (field->name) + sizeof (int);
	}

	buf = (char *) pool_alloc (total);
	pos = (void *) buf;
	pos = compound_save_name (meta->name, pos);
	memcpy (pos, (void *) &fields_len, (unsigned) sizeof (size_t));
	pos += sizeof (size_t);
	for (size_t i = 0; i < fields_len; i++) {
		field_t *field;

		field = (field_t *) vec_pos (meta->fields, (integer_value_t) i);
		pos = compound_save_name (field->name, pos);
		memcpy (pos, (void *) &field->type, (unsigned) sizeof (int));
		pos += sizeof (int);
	}

	str = str_new (buf, total);
	pool_free ((void *) buf);

	return str;
}

str_t *
compound_get_name (compound_t *meta)
{
	return meta->name;
}

str_t *
compound_get_field_name (compound_t *meta, integer_value_t pos)
{
	field_t *field;

	field = (field_t *) vec_pos (meta->fields, pos);
	if (field == NULL) {
		return NULL;
	}

	return field->name;
}

object_type_t
compound_get_field_type (compound_t *meta, integer_value_t pos)
{
	field_t *field;

	field = (field_t *) vec_pos (meta->fields, pos);
	if (field == NULL) {
		return OBJECT_TYPE_ERR;
	}

	return field->type;
}

integer_value_t
compound_find_field (compound_t *meta, str_t *name)
{
	size_t fields_len;

	fields_len = vec_size (meta->fields);
	for (size_t i = 0; i < fields_len; i++) {
		field_t *field;

		field = (field_t *) vec_pos (meta->fields, (integer_value_t) i);
		if (str_cmp (field->name, name) == 0) {
			return (integer_value_t) i;
		}
	}

	return (integer_value_t) -1;
}

size_t
compound_size (compound_t *meta)
{
	return vec_size (meta->fields);
}
