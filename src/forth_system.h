/*
 * forth_system.h
 *
 *  Created on: Nov 4, 2016
 *      Author: Robert
 */

#ifndef SRC_FORTH_SYSTEM_H_
#define SRC_FORTH_SYSTEM_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

extern uint32_t _sheap;
extern uint32_t _eheap;
extern uint32_t _estack;

extern uint32_t *sp;
extern uint32_t data_stack[];

extern uint32_t* forth_enter(uint32_t* param_stack, uint32_t const* forth_word);

extern uint32_t forth_2drop;
extern uint32_t forth_2dup;
extern uint32_t forth_2swap;
extern uint32_t forth_add;
extern uint32_t forth_addstore;
extern uint32_t forth_and;
extern uint32_t forth_base;
extern uint32_t forth_branch;
extern uint32_t forth_brancheq;
extern uint32_t forth_char;
extern uint32_t forth_char_newline;
extern uint32_t forth_char_space;
extern uint32_t forth_code_field_addr_of_next_word;
extern uint32_t forth_compile_def;
extern uint32_t forth_compile_mode;
extern uint32_t forth_create;
extern uint32_t forth_dec;
extern uint32_t forth_dec4;
extern uint32_t forth_div;
extern uint32_t forth_divmod;
extern uint32_t forth_do_colon;
extern uint32_t forth_drop;
extern uint32_t forth_dup;
extern uint32_t forth_end_compile_def;
extern uint32_t forth_eq;
extern uint32_t forth_eqz;
extern uint32_t forth_exit;
extern uint32_t forth_fetch;
extern uint32_t forth_fetch_char;
extern uint32_t forth_find;
extern uint32_t forth_ge;
extern uint32_t forth_gez;
extern uint32_t forth_gt;
extern uint32_t forth_gtz;
extern uint32_t forth_here;
extern uint32_t forth_immediate_mode;
extern uint32_t forth_inc;
extern uint32_t forth_inc4;
extern uint32_t forth_interpret;
extern uint32_t forth_key;
extern uint32_t forth_latest;
extern uint32_t forth_le;
extern uint32_t forth_lez;
extern uint32_t forth_lit;
extern uint32_t forth_literal;
extern uint32_t forth_lt;
extern uint32_t forth_ltz;
extern uint32_t forth_maybe_dup;
extern uint32_t forth_memcpy;
extern uint32_t forth_memmove;
extern uint32_t forth_mul;
extern uint32_t forth_ne;
extern uint32_t forth_nez;
extern uint32_t forth_not;
extern uint32_t forth_nrot;
extern uint32_t forth_number;
extern uint32_t forth_or;
extern uint32_t forth_over;
extern uint32_t forth_quit;
extern uint32_t forth_rot;
extern uint32_t forth_store;
extern uint32_t forth_store_char;
extern uint32_t forth_store_to_here;
extern uint32_t forth_sub;
extern uint32_t forth_substore;
extern uint32_t forth_swap;
extern uint32_t forth_to_code_field_addr;
extern uint32_t forth_to_data_field_addr;
extern uint32_t forth_toggle_hidden;
extern uint32_t forth_word;
extern uint32_t forth_xor;

extern uint32_t forth_var_HERE;
extern uint32_t forth_var_LATEST;
extern uint32_t forth_var_STATE;
extern uint32_t forth_var_STDIN;
extern uint32_t forth_var_STDIN_COUNT;

#ifdef __cplusplus
}
#endif

#endif /* SRC_FORTH_SYSTEM_H_ */
