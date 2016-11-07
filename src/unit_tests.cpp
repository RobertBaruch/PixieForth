/*
 * unit_tests.cpp
 *
 *  Created on: Nov 5, 2016
 *      Author: Robert
 */

#include <forth_system.h>
#include "WProgram.h"
#include <usb_serial.h>
#include <kinetis.h>

extern uint32_t forth_name_base;
extern uint32_t forth_name_latest;

static uint32_t original_var_here;
static uint32_t original_var_latest;

struct Test {
  const char* name;
  const uint32_t setup_stack_size; // in 4-byte words
  const uint32_t* setup_stack;
  const uint32_t* program; // effectively the body of a word.
  const uint32_t expected_stack_size; // in 4-byte words
  const uint32_t* expected_stack;
};

/*
 *  All tests must begin with forth_do_colon and end with forth_exit,
 *  just like all non-native forth words do.
 */
static Test tests[] = {
    {
        "EXIT",
        0, (uint32_t[]) { },
        (uint32_t[]) {
          (uint32_t)&forth_do_colon,
          (uint32_t)&forth_exit
        },
        0, (uint32_t[] ) { }
    },
    {
        "QUIT",
        0, (uint32_t[]) { },
        (uint32_t[]) {
          (uint32_t)&forth_do_colon,
          (uint32_t)&forth_quit // immediately aborts the Forth system
        },
        0, (uint32_t[] ) { }
    },
    {
        "BASE",
        0, (uint32_t[]) { },
        (uint32_t[]) {
          (uint32_t)&forth_do_colon,
          (uint32_t)&forth_base,
          (uint32_t)&forth_exit
        },
        1, (uint32_t[] ) { 10 }
    },
    {
        "HERE",
        0, (uint32_t[]) { },
        (uint32_t[]) {
          (uint32_t)&forth_do_colon,
          (uint32_t)&forth_here,
          (uint32_t)&forth_exit
        },
        1, (uint32_t[] ) { (uint32_t)&_sheap }
    },
    {
        "LATEST",
        0, (uint32_t[]) { },
        (uint32_t[]) {
          (uint32_t)&forth_do_colon,
          (uint32_t)&forth_latest,
          (uint32_t)&forth_exit
        },
        1, (uint32_t[] ) { (uint32_t)&forth_name_latest }
    },
    {
        "LIT",
        0, (uint32_t[]) { },
        (uint32_t[]) {
          (uint32_t)&forth_do_colon,
          (uint32_t)&forth_literal,
          0x1234abcd,
          (uint32_t)&forth_exit
        },
        1, (uint32_t[] ) { 0x1234abcd }
    },
    {
        "DROP",
        1, (uint32_t[]) { 1 },
        (uint32_t[]) {
          (uint32_t)&forth_do_colon,
          (uint32_t)&forth_drop,
          (uint32_t)&forth_exit
        },
        0, (uint32_t[] ) { }
    },
    {
        "2DROP",
        2, (uint32_t[]) { 1, 2 },
        (uint32_t[]) {
          (uint32_t)&forth_do_colon,
          (uint32_t)&forth_2drop,
          (uint32_t)&forth_exit
        },
        0, (uint32_t[] ) { }
    },
    {
        "SWAP",
        2, (uint32_t[]) { 1, 2 },
        (uint32_t[]) {
          (uint32_t)&forth_do_colon,
          (uint32_t)&forth_swap,
          (uint32_t)&forth_exit
        },
        2, (uint32_t[] ) { 2, 1 }
    },
    {
        "2SWAP",
        4, (uint32_t[]) { 1, 2, 3, 4 },
        (uint32_t[]) {
          (uint32_t)&forth_do_colon,
          (uint32_t)&forth_2swap,
          (uint32_t)&forth_exit
        },
        4, (uint32_t[] ) { 3, 4, 1, 2 }
    },
    {
        "DUP",
        1, (uint32_t[]) { 1 },
        (uint32_t[]) {
          (uint32_t)&forth_do_colon,
          (uint32_t)&forth_dup,
          (uint32_t)&forth_exit
        },
        2, (uint32_t[] ) { 1, 1 }
    },
    {
        "2DUP",
        2, (uint32_t[]) { 1, 2 },
        (uint32_t[]) {
          (uint32_t)&forth_do_colon,
          (uint32_t)&forth_2dup,
          (uint32_t)&forth_exit
        },
        4, (uint32_t[] ) { 1, 2, 1, 2 }
    },
    {
        "?DUP (+)",
        1, (uint32_t[]) { 1 },
        (uint32_t[]) {
          (uint32_t)&forth_do_colon,
          (uint32_t)&forth_maybe_dup,
          (uint32_t)&forth_exit
        },
        2, (uint32_t[] ) { 1, 1 }
    },
    {
        "?DUP (-)",
        1, (uint32_t[]) { 0 },
        (uint32_t[]) {
          (uint32_t)&forth_do_colon,
          (uint32_t)&forth_maybe_dup,
          (uint32_t)&forth_exit
        },
        1, (uint32_t[] ) { 0 }
    },
    {
        "OVER",
        2, (uint32_t[]) { 1, 2 },
        (uint32_t[]) {
          (uint32_t)&forth_do_colon,
          (uint32_t)&forth_over,
          (uint32_t)&forth_exit
        },
        3, (uint32_t[] ) { 1, 2, 1 }
    },
    {
        "ROT",
        3, (uint32_t[]) { 1, 2, 3 },
        (uint32_t[]) {
          (uint32_t)&forth_do_colon,          
          (uint32_t)&forth_rot,
          (uint32_t)&forth_exit
        },
        3, (uint32_t[] ) { 2, 3, 1 }
    },
    {
        "-ROT",
        3, (uint32_t[]) { 1, 2, 3 },
        (uint32_t[]) {
          (uint32_t)&forth_do_colon,
          (uint32_t)&forth_nrot,
          (uint32_t)&forth_exit
        },
        3, (uint32_t[] ) { 3, 1, 2 }
    },
    {
        "1+",
        1, (uint32_t[]) { 1 },
        (uint32_t[]) {
          (uint32_t)&forth_do_colon,
          (uint32_t)&forth_inc,
          (uint32_t)&forth_exit
        },
        1, (uint32_t[] ) { 2 }
    },
    {
        "1-",
        1, (uint32_t[]) { 1 },
        (uint32_t[]) {
          (uint32_t)&forth_do_colon,
          (uint32_t)&forth_dec,
          (uint32_t)&forth_exit
        },
        1, (uint32_t[] ) { 0 }
    },
    {
        "4+",
        1, (uint32_t[]) { 1 },
        (uint32_t[]) {
          (uint32_t)&forth_do_colon,
          (uint32_t)&forth_inc4,
          (uint32_t)&forth_exit
        },
        1, (uint32_t[] ) { 5 }
    },
    {
        "4-",
        1, (uint32_t[]) { 5 },
        (uint32_t[]) {
          (uint32_t)&forth_do_colon,
          (uint32_t)&forth_dec4,
          (uint32_t)&forth_exit
        },
        1, (uint32_t[] ) { 1 }
    },
    {
        "+",
        2, (uint32_t[]) { 1, 2 },
        (uint32_t[]) {
          (uint32_t)&forth_do_colon,
          (uint32_t)&forth_add,
          (uint32_t)&forth_exit
        },
        1, (uint32_t[] ) { 3 }
    },
    {
        "-",
        2, (uint32_t[]) { 3, 2 },
        (uint32_t[]) {
          (uint32_t)&forth_do_colon,
          (uint32_t)&forth_sub,
          (uint32_t)&forth_exit
        },
        1, (uint32_t[] ) { 1 }
    },
    {
        "*",
        2, (uint32_t[]) { 3, 2 },
        (uint32_t[]) {
          (uint32_t)&forth_do_colon,
          (uint32_t)&forth_mul,
          (uint32_t)&forth_exit
        },
        1, (uint32_t[] ) { 6 }
    },
    {
        "/MOD",
        2, (uint32_t[]) { 7, 2 },
        (uint32_t[]) {
          (uint32_t)&forth_do_colon,
          (uint32_t)&forth_divmod,
          (uint32_t)&forth_exit
        },
        2, (uint32_t[] ) { 1, 3 }
    },
    {
        "/",
        2, (uint32_t[]) { 7, 2 },
        (uint32_t[]) {
          (uint32_t)&forth_do_colon,
          (uint32_t)&forth_div,
          (uint32_t)&forth_exit
        },
        1, (uint32_t[] ) { 3 }
    },
    {
        "= (+)",
        2, (uint32_t[]) { 2, 2 },
        (uint32_t[]) {
          (uint32_t)&forth_do_colon,
          (uint32_t)&forth_eq,
          (uint32_t)&forth_exit
        },
        1, (uint32_t[] ) { 0xffffffff }
    },
    {
        "= (-)",
        2, (uint32_t[]) { 2, 1 },
        (uint32_t[]) {
          (uint32_t)&forth_do_colon,
          (uint32_t)&forth_eq,
          (uint32_t)&forth_exit
        },
        1, (uint32_t[] ) { 0 }
    },
    {
        "<> (+)",
        2, (uint32_t[]) { 2, 1 },
        (uint32_t[]) {
          (uint32_t)&forth_do_colon,
          (uint32_t)&forth_ne,
          (uint32_t)&forth_exit
        },
        1, (uint32_t[] ) { 0xffffffff }
    },
    {
        "<> (-)",
        2, (uint32_t[]) { 2, 2 },
        (uint32_t[]) {
          (uint32_t)&forth_do_colon,
          (uint32_t)&forth_ne,
          (uint32_t)&forth_exit
        },
        1, (uint32_t[] ) { 0 }
    },
    {
        "< (+)",
        2, (uint32_t[]) { 1, 2 },
        (uint32_t[]) {
          (uint32_t)&forth_do_colon,
          (uint32_t)&forth_lt,
          (uint32_t)&forth_exit
        },
        1, (uint32_t[] ) { 0xffffffff }
    },
    {
        "< (+signed)",
        2, (uint32_t[]) { 0xffffffff, 0 },
        (uint32_t[]) {
          (uint32_t)&forth_do_colon,
          (uint32_t)&forth_lt,
          (uint32_t)&forth_exit
        },
        1, (uint32_t[] ) { 0xffffffff }
    },
    {
        "< (-)",
        2, (uint32_t[]) { 2, 2 },
        (uint32_t[]) {
          (uint32_t)&forth_do_colon,
          (uint32_t)&forth_lt,
          (uint32_t)&forth_exit
        },
        1, (uint32_t[] ) { 0 }
    },
    {
        "> (+)",
        2, (uint32_t[]) { 2, 1 },
        (uint32_t[]) {
          (uint32_t)&forth_do_colon,
          (uint32_t)&forth_gt,
          (uint32_t)&forth_exit
        },
        1, (uint32_t[] ) { 0xffffffff }
    },
    {
        "> (+signed)",
        2, (uint32_t[]) { 0, 0xffffffff },
        (uint32_t[]) {
          (uint32_t)&forth_do_colon,
          (uint32_t)&forth_gt,
          (uint32_t)&forth_exit
        },
        1, (uint32_t[] ) { 0xffffffff }
    },
    {
        "> (-)",
        2, (uint32_t[]) { 2, 2 },
        (uint32_t[]) {
          (uint32_t)&forth_do_colon,
          (uint32_t)&forth_gt,
          (uint32_t)&forth_exit
        },
        1, (uint32_t[] ) { 0 }
    },
    {
        "<= (+)",
        2, (uint32_t[]) { 1, 2 },
        (uint32_t[]) {
          (uint32_t)&forth_do_colon,
          (uint32_t)&forth_le,
          (uint32_t)&forth_exit
        },
        1, (uint32_t[] ) { 0xffffffff }
    },
    {
        "<= (+signed)",
        2, (uint32_t[]) { 0xffffffff, 0 },
        (uint32_t[]) {
          (uint32_t)&forth_do_colon,
          (uint32_t)&forth_le,
          (uint32_t)&forth_exit
        },
        1, (uint32_t[] ) { 0xffffffff }
    },
    {
        "<= (+eq)",
        2, (uint32_t[]) { 2, 2 },
        (uint32_t[]) {
          (uint32_t)&forth_do_colon,
          (uint32_t)&forth_le,
          (uint32_t)&forth_exit
        },
        1, (uint32_t[] ) { 0xffffffff }
    },
    {
        "<= (-)",
        2, (uint32_t[]) { 3, 2 },
        (uint32_t[]) {
          (uint32_t)&forth_do_colon,
          (uint32_t)&forth_le,
          (uint32_t)&forth_exit
        },
        1, (uint32_t[] ) { 0 }
    },
    {
        ">= (+)",
        2, (uint32_t[]) { 2, 1 },
        (uint32_t[]) {
          (uint32_t)&forth_do_colon,
          (uint32_t)&forth_ge,
          (uint32_t)&forth_exit
        },
        1, (uint32_t[] ) { 0xffffffff }
    },
    {
        ">= (+signed)",
        2, (uint32_t[]) { 0, 0xffffffff },
        (uint32_t[]) {
          (uint32_t)&forth_do_colon,
          (uint32_t)&forth_ge,
          (uint32_t)&forth_exit
        },
        1, (uint32_t[] ) { 0xffffffff }
    },
    {
        ">= (+eq)",
        2, (uint32_t[]) { 2, 2 },
        (uint32_t[]) {
          (uint32_t)&forth_do_colon,
          (uint32_t)&forth_ge,
          (uint32_t)&forth_exit
        },
        1, (uint32_t[] ) { 0xffffffff }
    },
    {
        ">= (-)",
        2, (uint32_t[]) { 2, 3 },
        (uint32_t[]) {
          (uint32_t)&forth_do_colon,
          (uint32_t)&forth_ge,
          (uint32_t)&forth_exit
        },
        1, (uint32_t[] ) { 0 }
    },
    {
        "0= (+)",
        1, (uint32_t[]) { 0 },
        (uint32_t[]) {
          (uint32_t)&forth_do_colon,
          (uint32_t)&forth_eqz,
          (uint32_t)&forth_exit
        },
        1, (uint32_t[] ) { 0xffffffff }
    },
    {
        "0= (-)",
        1, (uint32_t[]) { 2 },
        (uint32_t[]) {
          (uint32_t)&forth_do_colon,
          (uint32_t)&forth_eqz,
          (uint32_t)&forth_exit
        },
        1, (uint32_t[] ) { 0 }
    },
    {
        "0<> (+)",
        1, (uint32_t[]) { 2 },
        (uint32_t[]) {
          (uint32_t)&forth_do_colon,
          (uint32_t)&forth_nez,
          (uint32_t)&forth_exit
        },
        1, (uint32_t[] ) { 0xffffffff }
    },
    {
        "0<> (-)",
        1, (uint32_t[]) { 0 },
        (uint32_t[]) {
          (uint32_t)&forth_do_colon,
          (uint32_t)&forth_nez,
          (uint32_t)&forth_exit
        },
        1, (uint32_t[] ) { 0 }
    },
    {
        "0< (+signed)",
        1, (uint32_t[]) { 0xffffffff },
        (uint32_t[]) {
          (uint32_t)&forth_do_colon,
          (uint32_t)&forth_ltz,
          (uint32_t)&forth_exit
        },
        1, (uint32_t[] ) { 0xffffffff }
    },
    {
        "0< (-)",
        1, (uint32_t[]) { 0 },
        (uint32_t[]) {
          (uint32_t)&forth_do_colon,
          (uint32_t)&forth_ltz,
          (uint32_t)&forth_exit
        },
        1, (uint32_t[] ) { 0 }
    },
    {
        "0> (+)",
        1, (uint32_t[]) { 2 },
        (uint32_t[]) {
          (uint32_t)&forth_do_colon,
          (uint32_t)&forth_gtz,
          (uint32_t)&forth_exit
        },
        1, (uint32_t[] ) { 0xffffffff }
    },
    {
        "0> (-signed)",
        1, (uint32_t[]) { 0xffffffff },
        (uint32_t[]) {
          (uint32_t)&forth_do_colon,
          (uint32_t)&forth_gtz,
          (uint32_t)&forth_exit
        },
        1, (uint32_t[] ) { 0 }
    },
    {
        "0> (-)",
        1, (uint32_t[]) { 0 },
        (uint32_t[]) {
          (uint32_t)&forth_do_colon,
          (uint32_t)&forth_gtz,
          (uint32_t)&forth_exit
        },
        1, (uint32_t[] ) { 0 }
    },
    {
        "0<= (+)",
        1, (uint32_t[]) { 0 },
        (uint32_t[]) {
          (uint32_t)&forth_do_colon,
          (uint32_t)&forth_lez,
          (uint32_t)&forth_exit
        },
        1, (uint32_t[] ) { 0xffffffff }
    },
    {
        "0<= (+signed)",
        1, (uint32_t[]) { 0xffffffff },
        (uint32_t[]) {
          (uint32_t)&forth_do_colon,
          (uint32_t)&forth_lez,
          (uint32_t)&forth_exit
        },
        1, (uint32_t[] ) { 0xffffffff }
    },
    {
        "0<= (-)",
        1, (uint32_t[]) { 1 },
        (uint32_t[]) {
          (uint32_t)&forth_do_colon,
          (uint32_t)&forth_lez,
          (uint32_t)&forth_exit
        },
        1, (uint32_t[] ) { 0 }
    },
    {
        "0>= (+)",
        1, (uint32_t[]) { 1 },
        (uint32_t[]) {
          (uint32_t)&forth_do_colon,
          (uint32_t)&forth_gez,
          (uint32_t)&forth_exit
        },
        1, (uint32_t[] ) { 0xffffffff }
    },
    {
        "0>= (+eq)",
        1, (uint32_t[]) { 0 },
        (uint32_t[]) {
          (uint32_t)&forth_do_colon,
          (uint32_t)&forth_gez,
          (uint32_t)&forth_exit
        },
        1, (uint32_t[] ) { 0xffffffff }
    },
    {
        "0>= (-)",
        1, (uint32_t[]) { 0xffffffff },
        (uint32_t[]) {
          (uint32_t)&forth_do_colon,
          (uint32_t)&forth_gez,
          (uint32_t)&forth_exit
        },
        1, (uint32_t[] ) { 0 }
    },
    {
        "AND",
        2, (uint32_t[]) { 0xf0f0f0f0, 0xffff0000 },
        (uint32_t[]) {
          (uint32_t)&forth_do_colon,
          (uint32_t)&forth_and,
          (uint32_t)&forth_exit
        },
        1, (uint32_t[] ) { 0xf0f00000 }
    },
    {
        "OR",
        2, (uint32_t[]) { 0xf0f0f0f0, 0xffff0000 },
        (uint32_t[]) {
          (uint32_t)&forth_do_colon,
          (uint32_t)&forth_or,
          (uint32_t)&forth_exit
        },
        1, (uint32_t[] ) { 0xfffff0f0 }
    },
    {
        "XOR",
        2, (uint32_t[]) { 0xf0f0f0f0, 0xffff0000 },
        (uint32_t[]) {
          (uint32_t)&forth_do_colon,
          (uint32_t)&forth_xor,
          (uint32_t)&forth_exit
        },
        1, (uint32_t[] ) { 0x0f0ff0f0 }
    },
    {
        "INVERT",
        1, (uint32_t[]) { 0xf0f0f0f0 },
        (uint32_t[]) {
          (uint32_t)&forth_do_colon,
          (uint32_t)&forth_not,
          (uint32_t)&forth_exit
        },
        1, (uint32_t[] ) { 0x0f0f0f0f }
    },
    {
        "!",
        3, (uint32_t[]) { 0xdeadbeef, 0xfeedface, (uint32_t)data_stack },
        (uint32_t[]) {
          (uint32_t)&forth_do_colon,
          (uint32_t)&forth_store,
          (uint32_t)&forth_exit
        },
        1, (uint32_t[] ) { 0xfeedface }
    },
    {
        "C!",
        3, (uint32_t[]) { 0xdeadbeef, 0xfeedface, (uint32_t)data_stack },
        (uint32_t[]) {
          (uint32_t)&forth_do_colon,
          (uint32_t)&forth_store_char,
          (uint32_t)&forth_exit
        },
        1, (uint32_t[] ) { 0xdeadbece }
    },
    {
        "@",
        2, (uint32_t[]) { 0xdeadbeef, (uint32_t)data_stack },
        (uint32_t[]) {
          (uint32_t)&forth_do_colon,
          (uint32_t)&forth_fetch,
          (uint32_t)&forth_exit
        },
        2, (uint32_t[] ) { 0xdeadbeef, 0xdeadbeef }
    },
    {
        "C@",
        2, (uint32_t[]) { 0xdeadbeef, (uint32_t)data_stack },
        (uint32_t[]) {
          (uint32_t)&forth_do_colon,
          (uint32_t)&forth_fetch_char,
          (uint32_t)&forth_exit
        },
        2, (uint32_t[] ) { 0xdeadbeef, 0xef }
    },
    {
        "+!",
        3, (uint32_t[]) { 1, 2, (uint32_t)data_stack },
        (uint32_t[]) {
          (uint32_t)&forth_do_colon,
          (uint32_t)&forth_addstore,
          (uint32_t)&forth_exit
        },
        1, (uint32_t[] ) { 3 }
    },
    {
        "-!",
        3, (uint32_t[]) { 2, 1, (uint32_t)data_stack },
        (uint32_t[]) {
          (uint32_t)&forth_do_colon,
          (uint32_t)&forth_substore,
          (uint32_t)&forth_exit
        },
        1, (uint32_t[] ) { 1 }
    },
    {
        "MEMCPY",
        9, (uint32_t[]) { 1, 2, 3, 4, 5, 6, (uint32_t)data_stack, (uint32_t)(data_stack+3), 12 },
        (uint32_t[]) {
          (uint32_t)&forth_do_colon,
          (uint32_t)&forth_memcpy,
          (uint32_t)&forth_exit
        },
        6, (uint32_t[] ) { 1, 2, 3, 1, 2, 3 }
    },
    {
        "MEMMOVE (overlap)",
        9, (uint32_t[]) { 1, 2, 3, 4, 5, 6, (uint32_t)data_stack, (uint32_t)(data_stack+1), 12 },
        (uint32_t[]) {
          (uint32_t)&forth_do_colon,
          (uint32_t)&forth_memmove,
          (uint32_t)&forth_exit
        },
        6, (uint32_t[] ) { 1, 1, 2, 3, 5, 6 }
    },
    {
        "MEMMOVE (nonoverlap)",
        9, (uint32_t[]) { 1, 2, 3, 4, 5, 6, (uint32_t)(data_stack+1), (uint32_t)data_stack, 12 },
        (uint32_t[]) {
          (uint32_t)&forth_do_colon,
          (uint32_t)&forth_memmove,
          (uint32_t)&forth_exit
        },
        6, (uint32_t[] ) { 2, 3, 4, 4, 5, 6 }
    },
    {
        "NUMBER", // "12"
        3, (uint32_t[]) { 0x00003231, (uint32_t)data_stack, 2 },
        (uint32_t[]) {
          (uint32_t)&forth_do_colon,
          (uint32_t)&forth_number,
          (uint32_t)&forth_exit
        },
        3, (uint32_t[] ) { 0x00003231, 12, 0 }
    },
    {
        "NUMBER (neg)", // "-12"
        3, (uint32_t[]) { 0x0032312D, (uint32_t)data_stack, 3 },
        (uint32_t[]) {
          (uint32_t)&forth_do_colon,
          (uint32_t)&forth_number,
          (uint32_t)&forth_exit
        },
        3, (uint32_t[] ) { 0x0032312D, (uint32_t)-12, 0 }
    },
    {
        "NUMBER (hex)", // "$1aB"
        3, (uint32_t[]) { 0x42613124, (uint32_t)data_stack, 4 },
        (uint32_t[]) {
          (uint32_t)&forth_do_colon,
          (uint32_t)&forth_number,
          (uint32_t)&forth_exit
        },
        3, (uint32_t[] ) { 0x42613124, 0x1ab, 0 }
    },
    {
        "NUMBER (bad)", // "1a"
        3, (uint32_t[]) { 0x00006131, (uint32_t)data_stack, 2 },
        (uint32_t[]) {
          (uint32_t)&forth_do_colon,
          (uint32_t)&forth_number,
          (uint32_t)&forth_exit
        },
        3, (uint32_t[] ) { 0x00006131, 1, 1 }
    },
    {
        "FIND (+)", // "BASE"
        3, (uint32_t[]) { 0x45534142, (uint32_t)data_stack, 4 },
        (uint32_t[]) {
          (uint32_t)&forth_do_colon,
          (uint32_t)&forth_find,
          (uint32_t)&forth_exit
        },
        2, (uint32_t[] ) { 0x45534142, (uint32_t)&forth_name_base }
    },
    {
        "FIND (-)", // "BASF"
        3, (uint32_t[]) { 0x46534142, (uint32_t)data_stack, 4 },
        (uint32_t[]) {
          (uint32_t)&forth_do_colon,
          (uint32_t)&forth_find,
          (uint32_t)&forth_exit
        },
        2, (uint32_t[] ) { 0x46534142, 0 }
    },
    {
        ">CFA",
        1, (uint32_t[]) { (uint32_t)&forth_name_base },
        (uint32_t[]) {
          (uint32_t)&forth_do_colon,
          (uint32_t)&forth_to_code_field_addr,
          (uint32_t)&forth_exit
        },
        1, (uint32_t[] ) { (uint32_t)&forth_base }
    },
    {
        ">DFA",
        1, (uint32_t[]) { (uint32_t)&forth_name_base },
        (uint32_t[]) {
          (uint32_t)&forth_do_colon,
          (uint32_t)&forth_to_data_field_addr,
          (uint32_t)&forth_exit
        },
        1, (uint32_t[] ) { 4 + (uint32_t)&forth_base }
    },
    {
        "CREATE", // "BBBB"
        3, (uint32_t[]) { 0x42424242, (uint32_t)data_stack, 4 },
        (uint32_t[]) {
          (uint32_t)&forth_do_colon,
          (uint32_t)&forth_create,
          (uint32_t)&forth_exit
        },
        1, (uint32_t[] ) { 0x42424242 }
    },
    {
        "HERE!",
        1, (uint32_t[]) { 1 },
        (uint32_t[]) {
          (uint32_t)&forth_do_colon,
          (uint32_t)&forth_store_to_here,
          (uint32_t)&forth_exit
        },
        0, (uint32_t[] ) { }
    },
    {
        "[",
        0, (uint32_t[]) { },
        (uint32_t[]) {
          (uint32_t)&forth_do_colon,
          (uint32_t)&forth_immediate_mode,
          (uint32_t)&forth_exit
        },
        0, (uint32_t[] ) { }
    },
    {
        "]",
        0, (uint32_t[]) { },
        (uint32_t[]) {
          (uint32_t)&forth_do_colon,
          (uint32_t)&forth_compile_mode,
          (uint32_t)&forth_exit
        },
        0, (uint32_t[] ) { }
    },
    {
        "BRANCH",
        2, (uint32_t[]) { 1, 2 },
        (uint32_t[]) {
          (uint32_t)&forth_do_colon,
          (uint32_t)&forth_branch,
          1,
          (uint32_t)&forth_add, // this should be skipped
          (uint32_t)&forth_exit
        },
        2, (uint32_t[] ) { 1, 2 }
    },
    {
        "0BRANCH (+)",
        3, (uint32_t[]) { 1, 2, 0 },
        (uint32_t[]) {
          (uint32_t)&forth_do_colon,
          (uint32_t)&forth_brancheq,
          1,
          (uint32_t)&forth_add, // this should be skipped
          (uint32_t)&forth_exit
        },
        2, (uint32_t[] ) { 1, 2 }
    },
    {
        "0BRANCH (-)",
        3, (uint32_t[]) { 1, 2, 1 },
        (uint32_t[]) {
          (uint32_t)&forth_do_colon,
          (uint32_t)&forth_brancheq,
          1,
          (uint32_t)&forth_add, // this should not be skipped
          (uint32_t)&forth_exit
        },
        1, (uint32_t[] ) { 3 }
    },
};

static bool additional_verification_create() {
  if (original_var_latest == forth_var_LATEST) {
    Serial.println("[FAIL]");
    Serial.println("  expected LATEST to change, but did not");
    return false;
  }
  if (original_var_here == forth_var_HERE) {
    Serial.println("[FAIL]");
    Serial.println("  expected HERE to change, but did not");
    return false;
  }
  if (forth_var_LATEST != original_var_here) {
    Serial.println("[FAIL]");
    Serial.println("  expected LATEST to be HERE, but was not");
    Serial.print("    HERE: ");
    Serial.println(forth_var_HERE, 16);
    Serial.print("    LATEST: ");
    Serial.println(forth_var_LATEST, 16);
    return false;
  }
  if (*(uint32_t *)forth_var_LATEST != original_var_latest) {
    Serial.println("[FAIL]");
    Serial.println("  expected [LATEST] to be prev LATEST, but was not");
    Serial.print("    [LATEST]: ");
    Serial.println(*(uint32_t *)forth_var_LATEST, 16);
    Serial.print("    prev LATEST: ");
    Serial.println(original_var_latest, 16);
    return false;
  }
  if (*((uint8_t *)forth_var_LATEST + 8) != 4) {
    Serial.println("[FAIL]");
    Serial.print("  expected len to be 4 but was ");
    Serial.print(*((uint8_t *)forth_var_LATEST + 8), 16);
    Serial.println(".");
    return false;
  }
  char *name_ptr = ((char *)forth_var_LATEST) + 9;
  if (memcmp(name_ptr, "BBBB", 4)) {
    Serial.println("[FAIL]");
    Serial.print("  expected name to be 42 42 42 42 but was ");
    Serial.print(*name_ptr, 16);
    Serial.print(" ");
    Serial.print(*(name_ptr + 1), 16);
    Serial.print(" ");
    Serial.print(*(name_ptr + 2), 16);
    Serial.print(" ");
    Serial.print(*(name_ptr + 3), 16);
    Serial.println(".");
    return false;
  }
  return true;
}

static bool additional_verification_store_to_here() {
  if (forth_var_HERE != original_var_here + 4) {
    Serial.println("[FAIL]");
    Serial.println("  expected HERE to be incremented by 4 but was not");
    Serial.print("    HERE: ");
    Serial.println(forth_var_HERE, 16);
    Serial.print("    previous HERE: ");
    Serial.println(original_var_here, 16);
    return false;
  }
  return true;
}

static bool additional_verification_immediate_mode() {
  if (forth_var_STATE != 0) {
    Serial.println("[FAIL]");
    Serial.print("  expected STATE to be 0 but was ");
    Serial.print(forth_var_STATE, 16);
    Serial.println(".");
    return false;
  }
  return true;
}

static bool additional_verification_compile_mode() {
  if (forth_var_STATE != 1) {
    Serial.println("[FAIL]");
    Serial.print("  expected STATE to be 1 but was ");
    Serial.print(forth_var_STATE, 16);
    Serial.println(".");
    return false;
  }
  return true;
}

static void fail_unit_test(int i) {
  Serial.println("[FAIL]");
  Serial.print("  expected stack (");
  Serial.print(tests[i].expected_stack_size);
  Serial.print("): -- ");
  for (int j = 0; j < tests[i].expected_stack_size; j++) {
    Serial.print(tests[i].expected_stack[j], 16);
    Serial.print(" ");
  }
  Serial.println();
  Serial.print("    actual stack (");
  Serial.print((uint32_t) (sp - data_stack));
  Serial.print("): -- ");
  for (uint32_t *ptr = data_stack; ptr < sp; ptr++) {
    Serial.print(*ptr, 16);
    Serial.print(" ");
  }
  Serial.println();
}

void run_unit_tests() {
  // Set up cycle counting

  ARM_DEMCR |= ARM_DEMCR_TRCENA; // enable debugging and monitoring blocks
  ARM_DWT_CYCCNT = 0; // reset the cycle count
  ARM_DWT_CTRL |= ARM_DWT_CTRL_CYCCNTENA; // enable cycle count

  Serial.println("UNIT TESTING");
  Serial.println();

  // This will contain the cycle count for the QUIT test. All other tests
  // will have this base amount removed from their cycle count to get a
  // somewhat better insight into the cycles used just by the test.
  // Be warned, though, that the cycle count does seem to vary by
  // a few on every run.
  uint32_t base_cycle_count = 0;
  original_var_here = forth_var_HERE;
  original_var_latest = forth_var_LATEST;

  for (int i = 0; i < sizeof(tests)/sizeof(Test); i++) {
    Serial.print(tests[i].name);
    Serial.print("...");
    for (int j = 0; j < 20 - strlen(tests[i].name); j++) Serial.print(' ');

    sp = data_stack;
    const uint32_t *setup_ptr = tests[i].setup_stack;
    for (int j = 0; j < tests[i].setup_stack_size; j++, sp++, setup_ptr++) {
      *sp = *setup_ptr;
    }

    // Reset the system
    forth_var_HERE = original_var_here;
    forth_var_LATEST = original_var_latest;
    forth_var_STATE = 0; // force immediate mode

    __disable_irq();
    uint32_t count_start = ARM_DWT_CYCCNT;

    sp = forth_enter(sp, tests[i].program);

    uint32_t count_end = ARM_DWT_CYCCNT;
    __enable_irq();

    // For the QUIT test, we are using less cycles than "normal", so
    // we need a negative number here.
    int32_t cycle_count = int32_t(count_end - count_start) - base_cycle_count;
    uint32_t actual_stack_size = sp - data_stack;

    if (actual_stack_size != tests[i].expected_stack_size) {
      fail_unit_test(i);
    } else if (memcmp(data_stack, tests[i].expected_stack, actual_stack_size * 4)) {
      fail_unit_test(i);
    } else {
      bool pass = true;
      if (!strcmp(tests[i].name, "CREATE")) {
        pass = additional_verification_create();
      } else if (!strcmp(tests[i].name, "HERE!")) {
        pass = additional_verification_store_to_here();
      } else if (!strcmp(tests[i].name, "[")) {
        pass = additional_verification_immediate_mode();
      } else if (!strcmp(tests[i].name, "]")) {
        pass = additional_verification_compile_mode();
      }
      if (pass) {
        Serial.print("[PASS] cycles: ");
        Serial.println(cycle_count);
      }
    }

    if (i == 0) base_cycle_count = cycle_count;
  }
}
