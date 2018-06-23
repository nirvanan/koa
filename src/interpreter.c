/*
 * interpreter.c
 * This file is part of koa
 *
 * Copyright (C) 2018 - Gordon Li
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 O*
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "interpreter.h"
#include "stack.h"
#include "frame.h"
#include "parser.h"
#include "code.h"
#include "boolobject.h"
#include "intobject.h"
#include "vecobject.h"
#include "funcobject.h"
#include "error.h"

static frame_t *g_current;

static stack_t *g_s;

static int
interpreter_play (code_t *code, int global)
{
	opcode_t opcode;
	op_t op;
	para_t para;
	object_t *a;
	object_t *b;
	object_t *c;
	object_t *d;
	object_t *e;
	object_t *r;

	g_current = frame_new (code, g_current, STACK_SP (g_s), global);
	if (g_current == NULL) {
		fatal_error ("failed to play code.");
	}

	while ((opcode = frame_next_opcode (g_current))) {
		op = OPCODE_OP (opcode);
		para = OPCODE_PARA (opcode);
		r = NULL;
		switch (op) {
		case OP_LOAD_CONST:
			r = code_get_const (code, para);
			break;
		case OP_STORE_LOCAL:
			a = code_get_varname (code, para);
			b = (object_t *) stack_pop (g_s);
			if (!frame_store_local (g_current, a, b)) {
				return 0;
			}
			break;
		case OP_STORE_VAR:
			a = code_get_varname (code, para);
			b = (object_t *) stack_top (g_s);
			if ((c = frame_store_var (g_current, a, b)) == NULL) {
				return 0;
			}
			object_unref (c);
			break;
		case OP_LOAD_VAR:
			a = code_get_varname (code, para);
			if ((r = frame_get_var (g_current, a)) == NULL) {
				return 0;
			}
			break;
		case OP_TYPE_CAST:
			a = (object_t *) stack_pop (g_s);
			if ((r = object_cast (a, (object_type_t) para)) == NULL) {
				object_free (a);

				return 0;
			}
			object_free (a);
			break;
		case OP_VAR_INC:
		case OP_VAR_DEC:
		case OP_VAR_POINC:
		case OP_VAR_PODEC:
			a = code_get_varname (code, para);
			if ((b = frame_get_var (g_current, a)) == NULL) {
				return 0;
			}
			if (op == OP_VAR_INC || op == OP_VAR_POINC) {
				c = intobject_new (1, NULL);
			}
			else {
				c = intobject_new (-1, NULL);
			}
			if (c == NULL) {
				return 0;
			}
			d = object_add (b, c);
			object_free (c);
			if (d == NULL) {
				return 0;
			}
			if (frame_store_var (g_current, a, d) != b) {
				return 0;
			}
			if (op == OP_VAR_INC || op == OP_VAR_DEC) {
				object_unref (b);
				r = d;
			}
			else {
				object_unref_without_free (b);
				r = b;
			}
			break;
		case OP_NEGATIVE:
			a = (object_t *) stack_pop (g_s);
			if ((r = object_neg (a)) == NULL) {
				object_free (b);

				return 0;
			}
			object_free (a);
			break;
		case OP_BIT_NOT:
			a = (object_t *) stack_pop (g_s);
			if ((r = object_bit_not (a)) == NULL) {
				object_free (b);

				return 0;
			}
			object_free (a);
			break;
		case  OP_LOGIC_NOT:
			a = (object_t *) stack_pop (g_s);
			if ((r = object_logic_not (a)) == NULL) {
				object_free (a);

				return 0;
			}
			object_free (a);
			break;
		case OP_POP_STACK:
			a = (object_t *) stack_pop (g_s);
			/* Need free because it might be zero refed. */
			object_free (a);
			break;
		case OP_LOAD_INDEX:
			b = (object_t *) stack_pop (g_s);
			a = (object_t *) stack_pop (g_s);
			if ((r = object_index (a, b)) == NULL) {
				object_free (b);

				return 0;
			}
			object_free (b);
			break;
		case OP_STORE_INDEX:
			b = (object_t *) stack_pop (g_s);
			a = (object_t *) stack_pop (g_s);
			c = (object_t *) stack_pop (g_s);
			if ((r = object_ipindex (a, b, c)) == NULL) {
				object_free (b);

				return 0;
			}
			object_free (b);
			break;
		case OP_INDEX_INC:
		case OP_INDEX_DEC:
		case OP_INDEX_POINC:
		case OP_INDEX_PODEC:
			b = (object_t *) stack_pop (g_s);
			a = (object_t *) stack_pop (g_s);
			if (op == OP_INDEX_INC || op == OP_INDEX_POINC) {
				c = intobject_new (1, NULL);
			}
			else {
				c = intobject_new (-1, NULL);
			}
			if (c == NULL) {
				return 0;
			}
			d = object_index (a, b);
			if (OBJECT_TYPE (d) == OBJECT_TYPE_NULL) {
				object_free (b);
				object_free (c);
				error ("null object can not be modified.");

				return 0;
			}
			if ((e = object_add (d, c)) == NULL) {
				object_free (b);
				object_free (c);

				return 0;
			}
			if ((r = object_ipindex (a, b, e)) == NULL) {
				object_free (b);
				object_free (c);

				return 0;
			}
			object_free (b);
			object_free (c);
			if (op == OP_INDEX_INC || op == OP_INDEX_DEC) {
				object_unref (d);
				r = e;
			}
			else {
				object_unref_without_free (d);
				r = d;
			}
			break;
		case OP_MAKE_VEC:
			r = vecobject_new ((size_t) para, NULL);
			for (int i = 0; i < para; i++) {
				b = intobject_new (i, NULL);
				c = (object_t *) stack_pop (g_s);
				if (object_ipindex (r, b, c) == NULL) {
					object_free (b);

					return 0;
				}
				object_free (b);
			}
			break;
		case OP_CALL_FUNC:
			a = (object_t *) stack_pop (g_s);
			if (OBJECT_TYPE (a) != OBJECT_TYPE_FUNC) {
				if (OBJECT_TYPE (a) != OBJECT_TYPE_VEC) {
					error ("only func object is callable.");
				}
				b = a;
				a = (object_t *) stack_pop (g_s);
				if (OBJECT_TYPE (a) != OBJECT_TYPE_FUNC) {
					error ("only func object is callable.");
				}
				if (!stack_push (g_s, (void *) b)) {
					return 0;
				}
			}
			if (!interpreter_play (funcobject_get_value (a), 0)) {
				return 0;
			}
			break;
		case OP_BIND_ARGS:
			a = (object_t *) stack_pop (g_s);
			if (a == NULL || OBJECT_TYPE (a) != OBJECT_TYPE_VEC) {
				object_free (a);
				error ("no argument passed.");

				return 0;
			}
			if (!frame_bind_args (g_current, a)) {
				object_free (a);

				return 0;
			}
			object_free (a);
			break;
		case OP_CON_SEL:
			c = (object_t *) stack_pop (g_s);
			b = (object_t *) stack_pop (g_s);
			a = (object_t *) stack_pop (g_s);
			if (!object_is_zero (a)) {
				r = b;
				object_free (a);
				object_free (c);
			}
			else {
				r = c;
				object_free (a);
				object_free (b);
			}
			break;
		case OP_LOGIC_OR:
			b = (object_t *) stack_pop (g_s);
			a = (object_t *) stack_pop (g_s);
			if ((r = object_logic_or (a, b)) == NULL) {
				object_free (a);
				object_free (b);

				return 0;
			}
			object_free (a);
			object_free (b);
			break;
		case OP_LOGIC_AND:
			b = (object_t *) stack_pop (g_s);
			a = (object_t *) stack_pop (g_s);
			if ((r = object_logic_and (a, b)) == NULL) {
				object_free (a);
				object_free (b);

				return 0;
			}
			object_free (a);
			object_free (b);
			break;
		case OP_BIT_OR:
			b = (object_t *) stack_pop (g_s);
			a = (object_t *) stack_pop (g_s);
			if ((r = object_bit_or (a, b)) == NULL) {
				object_free (a);
				object_free (b);

				return 0;
			}
			object_free (a);
			object_free (b);
			break;
		case OP_BIT_XOR:
			b = (object_t *) stack_pop (g_s);
			a = (object_t *) stack_pop (g_s);
			if ((r = object_bit_xor (a, b)) == NULL) {
				object_free (a);
				object_free (b);

				return 0;
			}
			object_free (a);
			object_free (b);
			break;
		case OP_BIT_AND:
			b = (object_t *) stack_pop (g_s);
			a = (object_t *) stack_pop (g_s);
			if ((r = object_bit_and (a, b)) == NULL) {
				object_free (a);
				object_free (b);

				return 0;
			}
			object_free (a);
			object_free (b);
			break;
		case OP_EQUAL:
			b = (object_t *) stack_pop (g_s);
			a = (object_t *) stack_pop (g_s);
			if ((r = object_equal (a, b)) == NULL) {
				object_free (a);
				object_free (b);

				return 0;
			}
			object_free (a);
			object_free (b);
			break;
		case OP_NOT_EQUAL:
			b = (object_t *) stack_pop (g_s);
			a = (object_t *) stack_pop (g_s);
			c = object_equal (a, b);
			object_free (a);
			object_free (b);
			if (c == NULL) {
				return 0;
			}
			r = object_is_zero (c)?
				boolobject_new (true, NULL):
				boolobject_new (false, NULL);
			object_free (c);
			if (r == NULL) {
				return 0;
			}
			break;
		case OP_LESS_THAN:
			b = (object_t *) stack_pop (g_s);
			a = (object_t *) stack_pop (g_s);
			c = object_compare (a, b);
			object_free (a);
			object_free (b);
			if (c == NULL) {
				return 0;
			}
			r = object_get_integer (c) < 0?
				boolobject_new (true, NULL):
				boolobject_new (false, NULL);
			object_free (c);
			if (r == NULL) {
				return 0;
			}
			break;
		case OP_LARGER_THAN:
			b = (object_t *) stack_pop (g_s);
			a = (object_t *) stack_pop (g_s);
			c = object_compare (a, b);
			object_free (a);
			object_free (b);
			if (c == NULL) {
				return 0;
			}
			r = object_get_integer (c) > 0?
				boolobject_new (true, NULL):
				boolobject_new (false, NULL);
			object_free (c);
			if (r == NULL) {
				return 0;
			}
			break;
		case OP_LESS_EQUAL:
			b = (object_t *) stack_pop (g_s);
			a = (object_t *) stack_pop (g_s);
			c = object_compare (a, b);
			object_free (a);
			object_free (b);
			if (c == NULL) {
				return 0;
			}
			r = object_get_integer (c) <= 0?
				boolobject_new (true, NULL):
				boolobject_new (false, NULL);
			object_free (c);
			if (r == NULL) {
				return 0;
			}
			break;
		case OP_LARGER_EQUAL:
			b = (object_t *) stack_pop (g_s);
			a = (object_t *) stack_pop (g_s);
			c = object_compare (a, b);
			object_free (a);
			object_free (b);
			if (c == NULL) {
				return 0;
			}
			r = object_get_integer (c) >= 0?
				boolobject_new (true, NULL):
				boolobject_new (false, NULL);
			object_free (c);
			if (r == NULL) {
				return 0;
			}
			break;
		case OP_LEFT_SHIFT:
			b = (object_t *) stack_pop (g_s);
			a = (object_t *) stack_pop (g_s);
			if ((r = object_left_shift (a, b)) == NULL) {
				object_free (a);
				object_free (b);

				return 0;
			}
			object_free (a);
			object_free (b);
			break;
		case OP_RIGHT_SHIFT:
			b = (object_t *) stack_pop (g_s);
			a = (object_t *) stack_pop (g_s);
			if ((r = object_right_shift (a, b)) == NULL) {
				object_free (a);
				object_free (b);

				return 0;
			}
			object_free (a);
			object_free (b);
			break;
		case OP_ADD:
			b = (object_t *) stack_pop (g_s);
			a = (object_t *) stack_pop (g_s);
			if ((r = object_add (a, b)) == NULL) {
				object_free (a);
				object_free (b);

				return 0;
			}
			object_free (a);
			object_free (b);
			break;
		case OP_SUB:
			b = (object_t *) stack_pop (g_s);
			a = (object_t *) stack_pop (g_s);
			if ((r = object_sub (a, b)) == NULL) {
				object_free (a);
				object_free (b);

				return 0;
			}
			object_free (a);
			object_free (b);
			break;
		case OP_MUL:
			b = (object_t *) stack_pop (g_s);
			a = (object_t *) stack_pop (g_s);
			if ((r = object_mul (a, b)) == NULL) {
				object_free (a);
				object_free (b);

				return 0;
			}
			object_free (a);
			object_free (b);
			break;
		case OP_DIV:
			b = (object_t *) stack_pop (g_s);
			a = (object_t *) stack_pop (g_s);
			if ((r = object_div (a, b)) == NULL) {
				object_free (a);
				object_free (b);

				return 0;
			}
			object_free (a);
			object_free (b);
			break;
		case OP_MOD:
			b = (object_t *) stack_pop (g_s);
			a = (object_t *) stack_pop (g_s);
			if ((r = object_mod (a, b)) == NULL) {
				object_free (a);
				object_free (b);

				return 0;
			}
			object_free (a);
			object_free (b);
			break;
		case OP_VAR_IPMUL:
			a = code_get_varname (code, para);
			b = (object_t *) stack_pop (g_s);
			if ((c = frame_get_var (g_current, a)) == NULL) {
				return 0;
			}
			r = object_mul (c, b);
			object_free (b);
			if (r == NULL) {
				return 0;
			}
			if ((b = frame_store_var (g_current, a, r)) == NULL) {
				return 0;
			}
			object_unref (b);
			break;
		case OP_VAR_IPDIV:
			a = code_get_varname (code, para);
			b = (object_t *) stack_pop (g_s);
			if ((c = frame_get_var (g_current, a)) == NULL) {
				return 0;
			}
			r = object_div (c, b);
			object_free (b);
			if (r == NULL) {
				return 0;
			}
			if ((b = frame_store_var (g_current, a, r)) == NULL) {
				return 0;
			}
			object_unref (b);
			break;
		case OP_VAR_IPMOD:
			a = code_get_varname (code, para);
			b = (object_t *) stack_pop (g_s);
			if ((c = frame_get_var (g_current, a)) == NULL) {
				return 0;
			}
			r = object_mod (c, b);
			object_free (b);
			if (r == NULL) {
				return 0;
			}
			if ((b = frame_store_var (g_current, a, r)) == NULL) {
				return 0;
			}
			object_unref (b);
			break;
		case OP_VAR_IPADD:
			a = code_get_varname (code, para);
			b = (object_t *) stack_pop (g_s);
			if ((c = frame_get_var (g_current, a)) == NULL) {
				return 0;
			}
			r = object_add (c, b);
			object_free (b);
			if (r == NULL) {
				return 0;
			}
			if ((b = frame_store_var (g_current, a, r)) == NULL) {
				return 0;
			}
			object_unref (b);
			break;
		case OP_VAR_IPSUB:
			a = code_get_varname (code, para);
			b = (object_t *) stack_pop (g_s);
			if ((c = frame_get_var (g_current, a)) == NULL) {
				return 0;
			}
			r = object_sub (c, b);
			object_free (b);
			if (r == NULL) {
				return 0;
			}
			if ((b = frame_store_var (g_current, a, r)) == NULL) {
				return 0;
			}
			object_unref (b);
			break;
		case OP_VAR_IPLS:
			a = code_get_varname (code, para);
			b = (object_t *) stack_pop (g_s);
			if ((c = frame_get_var (g_current, a)) == NULL) {
				return 0;
			}
			r = object_left_shift (c, b);
			object_free (b);
			if (r == NULL) {
				return 0;
			}
			if ((b = frame_store_var (g_current, a, r)) == NULL) {
				return 0;
			}
			object_unref (b);
			break;
		case OP_VAR_IPRS:
			a = code_get_varname (code, para);
			b = (object_t *) stack_pop (g_s);
			if ((c = frame_get_var (g_current, a)) == NULL) {
				return 0;
			}
			r = object_right_shift (c, b);
			object_free (b);
			if (r == NULL) {
				return 0;
			}
			if ((b = frame_store_var (g_current, a, r)) == NULL) {
				return 0;
			}
			object_unref (b);
			break;
		case OP_VAR_IPAND:
			a = code_get_varname (code, para);
			b = (object_t *) stack_pop (g_s);
			if ((c = frame_get_var (g_current, a)) == NULL) {
				return 0;
			}
			r = object_bit_and (c, b);
			object_free (b);
			if (r == NULL) {
				return 0;
			}
			if ((b = frame_store_var (g_current, a, r)) == NULL) {
				return 0;
			}
			object_unref (b);
			break;
		case OP_VAR_IPXOR:
			a = code_get_varname (code, para);
			b = (object_t *) stack_pop (g_s);
			if ((c = frame_get_var (g_current, a)) == NULL) {
				return 0;
			}
			r = object_bit_xor (c, b);
			object_free (b);
			if (r == NULL) {
				return 0;
			}
			if ((b = frame_store_var (g_current, a, r)) == NULL) {
				return 0;
			}
			object_unref (b);
			break;
		case OP_VAR_IPOR:
			a = code_get_varname (code, para);
			b = (object_t *) stack_pop (g_s);
			if ((c = frame_get_var (g_current, a)) == NULL) {
				return 0;
			}
			r = object_bit_or (c, b);
			object_free (b);
			if (r == NULL) {
				return 0;
			}
			if ((b = frame_store_var (g_current, a, r)) == NULL) {
				return 0;
			}
			object_unref (b);
			break;
		case OP_INDEX_IPMUL:
			b = (object_t *) stack_pop (g_s);
			a = (object_t *) stack_pop (g_s);
			c = (object_t *) stack_pop (g_s);
			d = object_index (a, b);
			if (d == NULL) {
				object_free (b);
				object_free (c);

				return 0;
			}
			r = object_mul (d, c);
			object_free (c);
			if (r == NULL) {
				object_free (b);

				return 0;
			}
			if (object_ipindex (a, b, r) != d) {
				object_free (b);
				object_free (r);

				return 0;
			}
			object_free (b);
			object_unref (d);
			break;
		case OP_INDEX_IPDIV:
			b = (object_t *) stack_pop (g_s);
			a = (object_t *) stack_pop (g_s);
			c = (object_t *) stack_pop (g_s);
			d = object_index (a, b);
			if (d == NULL) {
				object_free (b);
				object_free (c);

				return 0;
			}
			r = object_div (d, c);
			object_free (c);
			if (r == NULL) {
				object_free (b);

				return 0;
			}
			if (object_ipindex (a, b, r) != d) {
				object_free (b);
				object_free (r);

				return 0;
			}
			object_free (b);
			object_unref (d);
			break;
		case OP_INDEX_IPMOD:
			b = (object_t *) stack_pop (g_s);
			a = (object_t *) stack_pop (g_s);
			c = (object_t *) stack_pop (g_s);
			d = object_index (a, b);
			if (d == NULL) {
				object_free (b);
				object_free (c);

				return 0;
			}
			r = object_mod (d, c);
			object_free (c);
			if (r == NULL) {
				object_free (b);

				return 0;
			}
			if (object_ipindex (a, b, r) != d) {
				object_free (b);
				object_free (r);

				return 0;
			}
			object_free (b);
			object_unref (d);
			break;
		case OP_INDEX_IPADD:
			b = (object_t *) stack_pop (g_s);
			a = (object_t *) stack_pop (g_s);
			c = (object_t *) stack_pop (g_s);
			d = object_index (a, b);
			if (d == NULL) {
				object_free (b);
				object_free (c);

				return 0;
			}
			r = object_add (d, c);
			object_free (c);
			if (r == NULL) {
				object_free (b);

				return 0;
			}
			if (object_ipindex (a, b, r) != d) {
				object_free (b);
				object_free (r);

				return 0;
			}
			object_free (b);
			object_unref (d);
			break;
		case OP_INDEX_IPSUB:
			b = (object_t *) stack_pop (g_s);
			a = (object_t *) stack_pop (g_s);
			c = (object_t *) stack_pop (g_s);
			d = object_index (a, b);
			if (d == NULL) {
				object_free (b);
				object_free (c);

				return 0;
			}
			r = object_sub (d, c);
			object_free (c);
			if (r == NULL) {
				object_free (b);

				return 0;
			}
			if (object_ipindex (a, b, r) != d) {
				object_free (b);
				object_free (r);

				return 0;
			}
			object_free (b);
			object_unref (d);
			break;
		case OP_INDEX_IPLS:
			b = (object_t *) stack_pop (g_s);
			a = (object_t *) stack_pop (g_s);
			c = (object_t *) stack_pop (g_s);
			d = object_index (a, b);
			if (d == NULL) {
				object_free (b);
				object_free (c);

				return 0;
			}
			r = object_left_shift (d, c);
			object_free (c);
			if (r == NULL) {
				object_free (b);

				return 0;
			}
			if (object_ipindex (a, b, r) != d) {
				object_free (b);
				object_free (r);

				return 0;
			}
			object_free (b);
			object_unref (d);
			break;
		case OP_INDEX_IPRS:
			b = (object_t *) stack_pop (g_s);
			a = (object_t *) stack_pop (g_s);
			c = (object_t *) stack_pop (g_s);
			d = object_index (a, b);
			if (d == NULL) {
				object_free (b);
				object_free (c);

				return 0;
			}
			r = object_right_shift (d, c);
			object_free (c);
			if (r == NULL) {
				object_free (b);

				return 0;
			}
			if (object_ipindex (a, b, r) != d) {
				object_free (b);
				object_free (r);

				return 0;
			}
			object_free (b);
			object_unref (d);
			break;
		case OP_INDEX_IPAND:
			b = (object_t *) stack_pop (g_s);
			a = (object_t *) stack_pop (g_s);
			c = (object_t *) stack_pop (g_s);
			d = object_index (a, b);
			if (d == NULL) {
				object_free (b);
				object_free (c);

				return 0;
			}
			r = object_bit_and (d, c);
			object_free (c);
			if (r == NULL) {
				object_free (b);

				return 0;
			}
			if (object_ipindex (a, b, r) != d) {
				object_free (b);
				object_free (r);

				return 0;
			}
			object_free (b);
			object_unref (d);
			break;
		case OP_INDEX_IPXOR:
			b = (object_t *) stack_pop (g_s);
			a = (object_t *) stack_pop (g_s);
			c = (object_t *) stack_pop (g_s);
			d = object_index (a, b);
			if (d == NULL) {
				object_free (b);
				object_free (c);

				return 0;
			}
			r = object_bit_xor (d, c);
			object_free (c);
			if (r == NULL) {
				object_free (b);

				return 0;
			}
			if (object_ipindex (a, b, r) != d) {
				object_free (b);
				object_free (r);

				return 0;
			}
			object_free (b);
			object_unref (d);
			break;
		case OP_INDEX_IPOR:
			b = (object_t *) stack_pop (g_s);
			a = (object_t *) stack_pop (g_s);
			c = (object_t *) stack_pop (g_s);
			d = object_index (a, b);
			if (d == NULL) {
				object_free (b);
				object_free (c);

				return 0;
			}
			r = object_bit_or (d, c);
			object_free (c);
			if (r == NULL) {
				object_free (b);

				return 0;
			}
			if (object_ipindex (a, b, r) != d) {
				object_free (b);
				object_free (r);

				return 0;
			}
			object_free (b);
			object_unref (d);
			break;
		case OP_JUMP_FALSE:
			a = (object_t *) stack_pop (g_s);
			if (object_is_zero (a)) {
				frame_jump (g_current, para);
			}
			object_free (a);
			break;
		case OP_JUMP_FORCE:
		case OP_JUMP_CONTINUE:
		case OP_JUMP_BREAK:
			frame_jump (g_current, para);
			break;
		case OP_ENTER_BLOCK:
			if (!frame_enter_block (g_current)) {
				return 0;
			}
			break;
		case OP_LEAVE_BLOCK:
			if (!frame_leave_block (g_current)) {
				return 0;
			}
			break;
		case OP_RETURN:
			a = (object_t *) stack_pop (g_s);
			/* Check return type and try cast. */
			if (OBJECT_TYPE (a) != FUNC_RET_TYPE (code)) {
				b = object_cast (a, FUNC_RET_TYPE (code));
				if (b == NULL) {
					return 0;
				}

				r = b;
				object_free (a);
			}
			else {
				r = a;
			}
			g_current = frame_free (g_current);
			return stack_push (g_s, (void *) r);
		case OP_PUSH_BLOCKS:
			for (para_t i = 0; i < para; i++) {
				if (!frame_enter_block (g_current)) {
					return 0;
				}
			}
			break;
		case OP_POP_BLOCKS:
			for (para_t i = 0; i < para; i++) {
				if (!frame_leave_block (g_current)) {
					return 0;
				}
			}
			break;
		case OP_JUMP_CASE:
			b = (object_t *) stack_pop (g_s);
			a = (object_t *) stack_pop (g_s);
			c = object_equal (a, b);
			object_free (b);
			if (c == NULL) {
				return 0;
			}
			if (object_is_zero (c)) {
				frame_jump (g_current, para);
				if (!stack_push (g_s, (void *) a)) {
					return 0;
				}
			}
			else {
				object_free (a);
			}
			break;
		case OP_JUMP_DEFAULT:
			a = (object_t *) stack_pop (g_s);
			object_free (a);
			frame_jump (g_current, para);
			break;
		case OP_JUMP_TRUE:
			a = (object_t *) stack_pop (g_s);
			if (!object_is_zero (a)) {
				frame_jump (g_current, para);
			}
			object_free (a);
			break;
		case OP_END_PROGRAM:
			g_current = frame_free (g_current);
			return 1;
		}

		if (r != NULL && !stack_push (g_s, (void *) r)) {
			return 0;
		}
	}

	return 1;
}

void
interpreter_execute (const char *path)
{
	code_t *code;

	code = parser_load_file (path);
	if (code == NULL) {
		return;
	}

	if (!interpreter_play (code, 1)) {
		interpreter_traceback ();
	}

	code_free (code);
}

void
interpreter_traceback ()
{
	if (g_current != NULL) {
		frame_traceback (g_current);
	}
}

void
interpreter_init ()
{
	g_s = stack_new ();
	if (g_s == NULL) {
		fatal_error ("failed to init interpreter.");
	}
}