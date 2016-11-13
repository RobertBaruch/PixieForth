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
extern char forth_word_buffer;

static char *forth_word_buffer_ptr = &forth_word_buffer;
static uint32_t original_var_here;
static uint32_t original_var_latest;
static const char* original_var_stdin;
static uint32_t original_var_stdin_count;

struct Stack {
  const uint32_t size; // in 4-byte words
  const uint32_t* data;
};

struct Buff {
  const uint32_t size;
  const char* data;
};

struct Setup {
  const uint32_t* program; // effectively the body of a word.
  const Stack stack;
  const Buff stdin_; // _ to not conflict with stdin
  const Buff user_mem;
  const uint32_t state;
};

struct Expected {
  const Stack stack;
  const uint32_t stdin_left;
  const Buff word_buff;
  const uint32_t state;
  const uint32_t latest;
};

struct Test {
  const char* name;
  const Setup setup;
  const Expected expected;
};


typedef uint32_t Data[];
static const Stack empty_stack { 0, Data { } };

/*
 *  All tests must begin with forth_do_colon and end with forth_exit,
 *  just like all non-native forth words do.
 */
static Test tests[] {
    {
        "EXIT",
        {
            Data {
              (uint32_t)&forth_do_colon,
              (uint32_t)&forth_exit
            },
            empty_stack,
        },
        {
            empty_stack
        }
    },
    {
        "QUIT",
        {
            Data {
              (uint32_t)&forth_do_colon,
              (uint32_t)&forth_quit // immediately aborts the Forth system
            },
            empty_stack,
        },
        {
            empty_stack
        }
    },
    {
        "BASE",
        {
            Data {
              (uint32_t)&forth_do_colon,
              (uint32_t)&forth_base,
              (uint32_t)&forth_exit
            },
            empty_stack
        },
        {
            { 1, Data { 10 } }
        }
    },
    {
        "HERE",
        {
            Data {
              (uint32_t)&forth_do_colon,
              (uint32_t)&forth_here,
              (uint32_t)&forth_exit
            },
            empty_stack
        },
        {
            { 1, Data { (uint32_t)&_sheap } }
        }
    },
    {
        "LATEST",
        {
            Data {
              (uint32_t)&forth_do_colon,
              (uint32_t)&forth_latest,
              (uint32_t)&forth_exit
            },
            empty_stack
        },
        {
            { 1, Data { (uint32_t)&forth_name_latest } }
        }
    },
    {
        "LIT",
        {
            Data {
              (uint32_t)&forth_do_colon,
              (uint32_t)&forth_lit,
              0x1234abcd,
              (uint32_t)&forth_exit
            },
            empty_stack
        },
        {
            { 1, Data { 0x1234abcd } }
        }
    },
    {
        "DROP",
        {
            Data {
              (uint32_t)&forth_do_colon,
              (uint32_t)&forth_drop,
              (uint32_t)&forth_exit
            },
            { 1, Data { 1 } }
        },
        {
            empty_stack
        }
    },
    {
        "2DROP",
        {
            Data {
              (uint32_t)&forth_do_colon,
              (uint32_t)&forth_2drop,
              (uint32_t)&forth_exit
            },
            { 2, Data { 1, 2 } }
        },
        {
            empty_stack
        }
    },
    {
        "SWAP",
        {
            Data {
              (uint32_t)&forth_do_colon,
              (uint32_t)&forth_swap,
              (uint32_t)&forth_exit
            },
            { 2, Data { 1, 2 } }
        },
        {
            { 2, Data { 2, 1 } }
        }
    },
    {
        "2SWAP",
        {
            Data {
              (uint32_t)&forth_do_colon,
              (uint32_t)&forth_2swap,
              (uint32_t)&forth_exit
            },
            { 4, Data { 1, 2, 3, 4 } }
        },
        {
            { 4, Data { 3, 4, 1, 2 } }
        }
    },
    {
        "DUP",
        {
            Data {
              (uint32_t)&forth_do_colon,
              (uint32_t)&forth_dup,
              (uint32_t)&forth_exit
            },
            { 1, Data { 1 } }
        },
        {
            { 2, Data { 1, 1 } }
        }
    },
    {
        "2DUP",
        {
            Data {
              (uint32_t)&forth_do_colon,
              (uint32_t)&forth_2dup,
              (uint32_t)&forth_exit
            },
            { 2, Data { 1, 2 } }
        },
        {
            { 4, Data { 1, 2, 1, 2 } }
        }
    },
    {
        "?DUP (+)",
        {
            Data {
              (uint32_t)&forth_do_colon,
              (uint32_t)&forth_maybe_dup,
              (uint32_t)&forth_exit
            },
            { 1, Data { 1 } }
        },
        {
            { 2, Data { 1, 1 } }
        }
    },
    {
        "?DUP (-)",
        {
            Data {
              (uint32_t)&forth_do_colon,
              (uint32_t)&forth_maybe_dup,
              (uint32_t)&forth_exit
            },
            { 1, Data { 0 } }
        },
        {
            { 1, Data { 0 } }
        }
    },
    {
        "OVER",
        {
            Data {
              (uint32_t)&forth_do_colon,
              (uint32_t)&forth_over,
              (uint32_t)&forth_exit
            },
            { 2, Data { 1, 2 } }
        },
        {
            { 3, Data { 1, 2, 1 } }
        }
    },
    {
        "ROT",
        {
            Data {
              (uint32_t)&forth_do_colon,
              (uint32_t)&forth_rot,
              (uint32_t)&forth_exit
            },
            { 3, Data { 1, 2, 3 } }
        },
        {
            { 3, Data { 2, 3, 1 } }
        }
    },
    {
        "-ROT",
        {
            Data {
              (uint32_t)&forth_do_colon,
              (uint32_t)&forth_nrot,
              (uint32_t)&forth_exit
            },
            { 3, Data { 1, 2, 3 } }
        },
        {
            { 3, Data { 3, 1, 2 } }
        }
    },
    {
        "1+",
        {
            Data {
              (uint32_t)&forth_do_colon,
              (uint32_t)&forth_inc,
              (uint32_t)&forth_exit
            },
            { 1, Data { 1 } }
        },
        {
            { 1, Data { 2 } }
        }
    },
    {
        "1-",
        {
            Data {
              (uint32_t)&forth_do_colon,
              (uint32_t)&forth_dec,
              (uint32_t)&forth_exit
            },
            { 1, Data { 1 } }
        },
        {
            { 1, Data { 0 } }
        }
    },
    {
        "4+",
        {
            Data {
              (uint32_t)&forth_do_colon,
              (uint32_t)&forth_inc4,
              (uint32_t)&forth_exit
            },
            { 1, Data { 1 } }
        },
        {
            { 1, Data { 5 } }
        }
    },
    {
        "4-",
        {
            Data {
              (uint32_t)&forth_do_colon,
              (uint32_t)&forth_dec4,
              (uint32_t)&forth_exit
            },
            { 1, Data { 5 } }
        },
        {
            { 1, Data { 1 } }
        }
    },
    {
        "+",
        {
            Data {
              (uint32_t)&forth_do_colon,
              (uint32_t)&forth_add,
              (uint32_t)&forth_exit
            },
            { 2, Data { 1, 2 } }
        },
        {
            { 1, Data { 3 } }
        }
    },
    {
        "-",
        {
            Data {
              (uint32_t)&forth_do_colon,
              (uint32_t)&forth_sub,
              (uint32_t)&forth_exit
            },
            { 2, Data { 3, 2 } }
        },
        {
            { 1, Data { 1 } }
        }
    },
    {
        "*",
        {
            Data {
              (uint32_t)&forth_do_colon,
              (uint32_t)&forth_mul,
              (uint32_t)&forth_exit
            },
            { 2, Data { 3, 2 } }
        },
        {
            { 1, Data { 6 } }
        }
    },
    {
        "/MOD",
        {
            Data {
              (uint32_t)&forth_do_colon,
              (uint32_t)&forth_divmod,
              (uint32_t)&forth_exit
            },
            { 2, Data { 7, 2 } }
        },
        {
            { 2, Data { 1, 3 } }
        }
    },
    {
        "/",
        {
            Data {
              (uint32_t)&forth_do_colon,
              (uint32_t)&forth_div,
              (uint32_t)&forth_exit
            },
            { 2, Data { 7, 2 } }
        },
        {
            { 1, Data { 3 } }
        }
    },
    {
        "= (+)",
        {
            Data {
              (uint32_t)&forth_do_colon,
              (uint32_t)&forth_eq,
              (uint32_t)&forth_exit
            },
            { 2, Data { 2, 2 } }
        },
        {
            { 1, Data { 0xffffffff } }
        }
    },
    {
        "= (-)",
        {
            Data {
              (uint32_t)&forth_do_colon,
              (uint32_t)&forth_eq,
              (uint32_t)&forth_exit
            },
            { 2, Data { 2, 1 } }
        },
        {
            { 1, Data { 0 } }
        }
    },
    {
        "<> (+)",
        {
            Data {
              (uint32_t)&forth_do_colon,
              (uint32_t)&forth_ne,
              (uint32_t)&forth_exit
            },
            { 2, Data { 2, 1 } }
        },
        {
            { 1, Data { 0xffffffff } }
        }
    },
    {
        "<> (-)",
        {
            Data {
              (uint32_t)&forth_do_colon,
              (uint32_t)&forth_ne,
              (uint32_t)&forth_exit
            },
            { 2, Data { 2, 2 } }
        },
        {
            { 1, Data { 0 } }
        }
    },
    {
        "< (+)",
        {
            Data {
              (uint32_t)&forth_do_colon,
              (uint32_t)&forth_lt,
              (uint32_t)&forth_exit
            },
            { 2, Data { 1, 2 } }
        },
        {
            { 1, Data { 0xffffffff } }
        }
    },
    {
        "< (+signed)",
        {
            Data {
              (uint32_t)&forth_do_colon,
              (uint32_t)&forth_lt,
              (uint32_t)&forth_exit
            },
            { 2, Data { 0xffffffff, 0 } }
        },
        {
            { 1, Data { 0xffffffff } }
        }
    },
    {
        "< (-)",
        {
            Data {
              (uint32_t)&forth_do_colon,
              (uint32_t)&forth_lt,
              (uint32_t)&forth_exit
            },
            { 2, Data { 2, 2 } }
        },
        {
            { 1, Data { 0 } }
        }
    },
    {
        "> (+)",
        {
            Data {
              (uint32_t)&forth_do_colon,
              (uint32_t)&forth_gt,
              (uint32_t)&forth_exit
            },
            { 2, Data { 2, 1 } }
        },
        {
            { 1, Data { 0xffffffff } }
        }
    },
    {
        "> (+signed)",
        {
            Data {
              (uint32_t)&forth_do_colon,
              (uint32_t)&forth_gt,
              (uint32_t)&forth_exit
            },
            { 2, Data { 0, 0xffffffff } }
        },
        {
            { 1, Data { 0xffffffff } }
        }
    },
    {
        "> (-)",
        {
            Data {
              (uint32_t)&forth_do_colon,
              (uint32_t)&forth_gt,
              (uint32_t)&forth_exit
            },
            { 2, Data { 2, 2 } }
        },
        {
            { 1, Data { 0 } }
        }
    },
    {
        "<= (+)",
        {
            Data {
              (uint32_t)&forth_do_colon,
              (uint32_t)&forth_le,
              (uint32_t)&forth_exit
            },
            { 2, Data { 1, 2 } }
        },
        {
            { 1, Data { 0xffffffff } }
        }
    },
    {
        "<= (+signed)",
        {
            Data {
              (uint32_t)&forth_do_colon,
              (uint32_t)&forth_le,
              (uint32_t)&forth_exit
            },
            { 2, Data { 0xffffffff, 0 } }
        },
        {
            { 1, Data { 0xffffffff } }
        }
    },
    {
        "<= (+eq)",
        {
            Data {
              (uint32_t)&forth_do_colon,
              (uint32_t)&forth_le,
              (uint32_t)&forth_exit
            },
            {2, Data { 2, 2 } }
        },
        {
            {1, Data { 0xffffffff } }
        }
    },
    {
        "<= (-)",
        {
            Data {
              (uint32_t)&forth_do_colon,
              (uint32_t)&forth_le,
              (uint32_t)&forth_exit
            },
            { 2, Data { 3, 2 } }
        },
        {
            { 1, Data { 0 } }
        }
    },
    {
        ">= (+)",
        {
            Data {
              (uint32_t)&forth_do_colon,
              (uint32_t)&forth_ge,
              (uint32_t)&forth_exit
            },
            {2, Data { 2, 1 } }
        },
        {
            {1, Data { 0xffffffff } }
        }
    },
    {
        ">= (+signed)",
        {
            Data {
              (uint32_t)&forth_do_colon,
              (uint32_t)&forth_ge,
              (uint32_t)&forth_exit
            },
            { 2, Data { 0, 0xffffffff } }
        },
        {
            { 1, Data { 0xffffffff } }
        }
    },
    {
        ">= (+eq)",
        {
            Data {
              (uint32_t)&forth_do_colon,
              (uint32_t)&forth_ge,
              (uint32_t)&forth_exit
            },
            { 2, Data { 2, 2 } }
        },
        {
            { 1, Data { 0xffffffff } }
        }
    },
    {
        ">= (-)",
        {
            Data {
              (uint32_t)&forth_do_colon,
              (uint32_t)&forth_ge,
              (uint32_t)&forth_exit
            },
            { 2, Data { 2, 3 } }
        },
        {
            { 1, Data { 0 } }
        }
    },
    {
        "0= (+)",
        {
            Data {
              (uint32_t)&forth_do_colon,
              (uint32_t)&forth_eqz,
              (uint32_t)&forth_exit
            },
            { 1, Data { 0 } }
        },
        {
            { 1, Data { 0xffffffff } }
        }
    },
    {
        "0= (-)",
        {
            Data {
              (uint32_t)&forth_do_colon,
              (uint32_t)&forth_eqz,
              (uint32_t)&forth_exit
            },
            { 1, Data { 2 } }
        },
        {
            { 1, Data { 0 } }
        }
    },
    {
        "0<> (+)",
        {
            Data {
              (uint32_t)&forth_do_colon,
              (uint32_t)&forth_nez,
              (uint32_t)&forth_exit
            },
            { 1, Data { 2 } }
        },
        {
            { 1, Data { 0xffffffff } }
        }
    },
    {
        "0<> (-)",
        {
            Data {
              (uint32_t)&forth_do_colon,
              (uint32_t)&forth_nez,
              (uint32_t)&forth_exit
            },
            { 1, Data { 0 } }
        },
        {
            { 1, Data { 0 } }
        }
    },
    {
        "0< (+signed)",
        {
            Data {
              (uint32_t)&forth_do_colon,
              (uint32_t)&forth_ltz,
              (uint32_t)&forth_exit
            },
            { 1, Data { 0xffffffff } }
        },
        {
            { 1, Data { 0xffffffff } }
        }
    },
    {
        "0< (-)",
        {
            Data {
              (uint32_t)&forth_do_colon,
              (uint32_t)&forth_ltz,
              (uint32_t)&forth_exit
            },
            { 1, Data { 0 } }
        },
        {
            { 1, Data { 0 } }
        }
    },
    {
        "0> (+)",
        {
            Data {
              (uint32_t)&forth_do_colon,
              (uint32_t)&forth_gtz,
              (uint32_t)&forth_exit
            },
            { 1, Data { 2 } }
        },
        {
            { 1, Data { 0xffffffff } }
        }
    },
    {
        "0> (-signed)",
        {
            Data {
              (uint32_t)&forth_do_colon,
              (uint32_t)&forth_gtz,
              (uint32_t)&forth_exit
            },
            { 1, Data { 0xffffffff } }
        },
        {
            { 1, Data { 0 } }
        }
    },
    {
        "0> (-)",
        {
            Data {
              (uint32_t)&forth_do_colon,
              (uint32_t)&forth_gtz,
              (uint32_t)&forth_exit
            },
            { 1, Data { 0 } }
        },
        {
            { 1, Data { 0 } }
        }
    },
    {
        "0<= (+)",
        {
            Data {
              (uint32_t)&forth_do_colon,
              (uint32_t)&forth_lez,
              (uint32_t)&forth_exit
            },
            { 1, Data { 0 } }
        },
        {
            { 1, Data { 0xffffffff } }
        }
    },
    {
        "0<= (+signed)",
        {
            Data {
              (uint32_t)&forth_do_colon,
              (uint32_t)&forth_lez,
              (uint32_t)&forth_exit
            },
            { 1, Data { 0xffffffff } }
        },
        {
            { 1, Data { 0xffffffff } }
        }
    },
    {
        "0<= (-)",
        {
            Data {
              (uint32_t)&forth_do_colon,
              (uint32_t)&forth_lez,
              (uint32_t)&forth_exit
            },
            { 1, Data { 1 } }
        },
        {
            { 1, Data { 0 } }
        }
    },
    {
        "0>= (+)",
        {
            Data {
              (uint32_t)&forth_do_colon,
              (uint32_t)&forth_gez,
              (uint32_t)&forth_exit
            },
            { 1, Data { 1 } }
        },
        {
            { 1, Data { 0xffffffff } }
        }
    },
    {
        "0>= (+eq)",
        {
            Data {
              (uint32_t)&forth_do_colon,
              (uint32_t)&forth_gez,
              (uint32_t)&forth_exit
            },
            { 1, Data { 0 } }
        },
        {
            { 1, Data { 0xffffffff } }
        }
    },
    {
        "0>= (-)",
        {
            Data {
              (uint32_t)&forth_do_colon,
              (uint32_t)&forth_gez,
              (uint32_t)&forth_exit
            },
            { 1, Data { 0xffffffff } }
        },
        {
            { 1, Data { 0 } }
        }
    },
    {
        "AND",
        {
            Data {
              (uint32_t)&forth_do_colon,
              (uint32_t)&forth_and,
              (uint32_t)&forth_exit
            },
            { 2, Data { 0xf0f0f0f0, 0xffff0000 } }
        },
        {
            { 1, Data { 0xf0f00000 } }
        }
    },
    {
        "OR",
        {
            Data {
              (uint32_t)&forth_do_colon,
              (uint32_t)&forth_or,
              (uint32_t)&forth_exit
            },
            { 2, Data { 0xf0f0f0f0, 0xffff0000 } }
        },
        {
            { 1, Data { 0xfffff0f0 } }
        }
    },
    {
        "XOR",
        {
            Data {
              (uint32_t)&forth_do_colon,
              (uint32_t)&forth_xor,
              (uint32_t)&forth_exit
            },
            { 2, Data { 0xf0f0f0f0, 0xffff0000 } }
        },
        {
            { 1, Data { 0x0f0ff0f0 } }
        }
    },
    {
        "INVERT",
        {
            Data {
              (uint32_t)&forth_do_colon,
              (uint32_t)&forth_not,
              (uint32_t)&forth_exit
            },
            { 1, Data { 0xf0f0f0f0 } }
        },
        {
            { 1, Data { 0x0f0f0f0f } }
        }
    },
    {
        "!",
        {
            Data {
              (uint32_t)&forth_do_colon,
              (uint32_t)&forth_store,
              (uint32_t)&forth_exit
            },
            { 3, Data { 0xdeadbeef, 0xfeedface, (uint32_t)data_stack } }
        },
        {
            { 1, Data { 0xfeedface } }
        }
    },
    {
        "C!",
        {
            Data {
              (uint32_t)&forth_do_colon,
              (uint32_t)&forth_store_char,
              (uint32_t)&forth_exit
            },
            { 3, Data { 0xdeadbeef, 0xfeedface, (uint32_t)data_stack } }
        },
        {
            { 1, Data { 0xdeadbece } }
        }
    },
    {
        "@",
        {
            Data {
              (uint32_t)&forth_do_colon,
              (uint32_t)&forth_fetch,
              (uint32_t)&forth_exit
            },
            { 2, Data { 0xdeadbeef, (uint32_t)data_stack } }
        },
        {
            { 2, Data { 0xdeadbeef, 0xdeadbeef } }
        }
    },
    {
        "C@",
        {
            Data {
              (uint32_t)&forth_do_colon,
              (uint32_t)&forth_fetch_char,
              (uint32_t)&forth_exit
            },
            { 2, Data { 0xdeadbeef, (uint32_t)data_stack } }
        },
        {
            { 2, Data { 0xdeadbeef, 0xef } }
        }
    },
    {
        "+!",
        {
            Data {
              (uint32_t)&forth_do_colon,
              (uint32_t)&forth_addstore,
              (uint32_t)&forth_exit
            },
            { 3, Data { 1, 2, (uint32_t)data_stack } }
        },
        {
            { 1, Data { 3 } }
        }
    },
    {
        "-!",
        {
            Data {
              (uint32_t)&forth_do_colon,
              (uint32_t)&forth_substore,
              (uint32_t)&forth_exit
            },
            { 3, Data { 2, 1, (uint32_t)data_stack } }
        },
        {
            { 1, Data { 1 } }
        }
    },
    {
        "MEMCPY",
        {
            Data {
              (uint32_t)&forth_do_colon,
              (uint32_t)&forth_memcpy,
              (uint32_t)&forth_exit
            },
            { 9, Data { 1, 2, 3, 4, 5, 6, (uint32_t)data_stack, (uint32_t)(data_stack+3), 12 } }
        },
        {
            { 6, Data { 1, 2, 3, 1, 2, 3 } }
        }
    },
    {
        "MEMMOVE (overlap)",
        {
            Data {
              (uint32_t)&forth_do_colon,
              (uint32_t)&forth_memmove,
              (uint32_t)&forth_exit
            },
            { 9, Data { 1, 2, 3, 4, 5, 6, (uint32_t)data_stack, (uint32_t)(data_stack+1), 12 } }
        },
        {
            { 6, Data { 1, 1, 2, 3, 5, 6 } }
        }
    },
    {
        "MEMMOVE (nonoverlap)",
        {
            Data {
              (uint32_t)&forth_do_colon,
              (uint32_t)&forth_memmove,
              (uint32_t)&forth_exit
            },
            { 9, Data { 1, 2, 3, 4, 5, 6, (uint32_t)(data_stack+1), (uint32_t)data_stack, 12 } }
        },
        {
            { 6, Data { 2, 3, 4, 4, 5, 6 } }
        }
    },
    {
        "NUMBER", // "12"
        {
            Data {
              (uint32_t)&forth_do_colon,
              (uint32_t)&forth_number,
              (uint32_t)&forth_exit
            },
            { 3, Data { 0x00003231, (uint32_t)data_stack, 2 } }
        },
        {
            { 3, Data { 0x00003231, 12, 0 } }
        }
    },
    {
        "NUMBER (neg)", // "-12"
        {
            Data {
              (uint32_t)&forth_do_colon,
              (uint32_t)&forth_number,
              (uint32_t)&forth_exit
            },
            { 3, Data { 0x0032312D, (uint32_t)data_stack, 3 } }
        },
        {
            { 3, Data { 0x0032312D, (uint32_t)-12, 0 } }
        }
    },
    {
        "NUMBER (hex)", // "$1aB"
        {
            Data {
              (uint32_t)&forth_do_colon,
              (uint32_t)&forth_number,
              (uint32_t)&forth_exit
            },
            { 3, Data { 0x42613124, (uint32_t)data_stack, 4 } }
        },
        {
            { 3, Data { 0x42613124, 0x1ab, 0 } }
        }
    },
    {
        "NUMBER (bad)", // "1a"
        {
            Data {
              (uint32_t)&forth_do_colon,
              (uint32_t)&forth_number,
              (uint32_t)&forth_exit
            },
            { 3, Data { 0x00006131, (uint32_t)data_stack, 2 } }
        },
        {
            { 3, Data { 0x00006131, 1, 1 } }
        }
    },
    {
        "FIND (+)", // "BASE"
        {
            Data {
              (uint32_t)&forth_do_colon,
              (uint32_t)&forth_find,
              (uint32_t)&forth_exit
            },
            { 3, Data { 0x45534142, (uint32_t)data_stack, 4 } }
        },
        {
            { 2, Data { 0x45534142, (uint32_t)&forth_name_base } }
        }
    },
    {
        "FIND (-)", // "BASF"
        {
            Data {
              (uint32_t)&forth_do_colon,
              (uint32_t)&forth_find,
              (uint32_t)&forth_exit
            },
            { 3, Data { 0x46534142, (uint32_t)data_stack, 4 } }
        },
        {
            { 2, Data { 0x46534142, 0 } }
        }
    },
    {
        ">CFA",
        {
            Data {
              (uint32_t)&forth_do_colon,
              (uint32_t)&forth_to_code_field_addr,
              (uint32_t)&forth_exit
            },
            { 1, Data { (uint32_t)&forth_name_base } }
        },
        {
            { 1, Data { (uint32_t)&forth_base } }
        }
    },
    {
        ">DFA",
        {
            Data {
              (uint32_t)&forth_do_colon,
              (uint32_t)&forth_to_data_field_addr,
              (uint32_t)&forth_exit
            },
            { 1, Data { (uint32_t)&forth_name_base } }
        },
        {
            { 1, Data { 4 + (uint32_t)&forth_base } }
        }
    },
    {
        "CREATE", // "BBBB"
        {
            Data {
              (uint32_t)&forth_do_colon,
              (uint32_t)&forth_create,
              (uint32_t)&forth_exit
            },
            { 3, Data { 0x42424242, (uint32_t)data_stack, 4 } }
        },
        {
            { 1, Data { 0x42424242 } },
            0, // stdin_left
            { 0, "" }, // word_buff
            0, // state
            forth_var_HERE // latest
        }
    },
    {
        ",",
        {
            Data {
              (uint32_t)&forth_do_colon,
              (uint32_t)&forth_store_to_here,
              (uint32_t)&forth_exit
            },
            { 1, Data { 1 } }
        },
        {
            empty_stack
        }
    },
    {
        "[",
        {
            Data {
              (uint32_t)&forth_do_colon,
              (uint32_t)&forth_immediate_mode,
              (uint32_t)&forth_exit
            },
            empty_stack
        },
        {
            empty_stack
        }
    },
    {
        "]",
        {
            Data {
              (uint32_t)&forth_do_colon,
              (uint32_t)&forth_compile_mode,
              (uint32_t)&forth_exit
            },
            empty_stack
        },
        {
            empty_stack,
            0, // stdin_left
            { 0, "" }, // word_buff
            1 // state
        }
    },
    {
        "HIDDEN", // create "BBBB", hide it
        {
            Data {
              (uint32_t)&forth_do_colon,
              (uint32_t)&forth_create,
              (uint32_t)&forth_latest,
              (uint32_t)&forth_toggle_hidden,
              (uint32_t)&forth_exit
            },
            { 3, Data { 0x42424242, (uint32_t)data_stack, 4 } }
        },
        {
            { 1, Data { 0x42424242 } },
            0, // stdin_left
            { 0, "" }, // word_buff
            0, // state
            forth_var_HERE // latest
        }
    },
    {
        "HIDDEN (toggle)", // create "BBBB", hide it, unhide it
        {
            Data {
              (uint32_t)&forth_do_colon,
              (uint32_t)&forth_create,
              (uint32_t)&forth_latest,
              (uint32_t)&forth_toggle_hidden,
              (uint32_t)&forth_latest,
              (uint32_t)&forth_toggle_hidden,
              (uint32_t)&forth_exit
            },
            { 3, Data { 0x42424242, (uint32_t)data_stack, 4 } }
        },
        {
            { 1, Data { 0x42424242 } },
            0, // stdin_left
            { 0, "" }, // word_buff
            0, // state
            forth_var_HERE // latest
        }
    },
    {
        "BRANCH",
        {
            Data {
              (uint32_t)&forth_do_colon,
              (uint32_t)&forth_branch,
              1,
              (uint32_t)&forth_add, // this should be skipped
              (uint32_t)&forth_exit
            },
            { 2, Data { 1, 2 } }
        },
        {
            { 2, Data { 1, 2 } }
        }
    },
    {
        "0BRANCH (+)",
        {
            Data {
              (uint32_t)&forth_do_colon,
              (uint32_t)&forth_brancheq,
              1,
              (uint32_t)&forth_add, // this should be skipped
              (uint32_t)&forth_exit
            },
            { 3, Data { 1, 2, 0 } }
        },
        {
            { 2, Data { 1, 2 } }
        }
    },
    {
        "0BRANCH (-)",
        {
            Data {
              (uint32_t)&forth_do_colon,
              (uint32_t)&forth_brancheq,
              1,
              (uint32_t)&forth_add, // this should not be skipped
              (uint32_t)&forth_exit
            },
            { 3, Data { 1, 2, 1 } }
        },
        {
            { 1, Data { 3 } }
        }
    },
    {
        "CHAR",
        {
            Data {
              (uint32_t)&forth_do_colon,
              (uint32_t)&forth_char,
              (uint32_t)&forth_exit
            },
            empty_stack,
            { 5, "0123 " }
        },
        {
            { 1, Data { (uint32_t) '0' } },
            0, // stdin_left
            { 4, "0123" } // word_buff
        }
    },
    {
        "'\\n'",
        {
            Data {
              (uint32_t)&forth_do_colon,
              (uint32_t)&forth_char_newline,
              (uint32_t)&forth_exit
            },
            empty_stack
        },
        {
            { 1, Data { 10 } }
        }
    },
    {
        "BL",
        {
            Data {
              (uint32_t)&forth_do_colon,
              (uint32_t)&forth_char_space,
              (uint32_t)&forth_exit
            },
            empty_stack
        },
        {
            { 1, Data { ' ' } }
        }
    },
    {
        "KEY",
        {
          Data {
            (uint32_t)&forth_do_colon,
            (uint32_t)&forth_key,
            (uint32_t)&forth_exit
          },
          empty_stack,
          { 2, "01" }
        },
        {
          { 1, Data { '0' } },
          1
        }
    },
    {
        "KEY (EOF)",
        {
            Data {
              (uint32_t)&forth_do_colon,
              (uint32_t)&forth_key,
              (uint32_t)&forth_exit
            },
            empty_stack,
            { 0, "" }
        },
        {
            { 1, Data { 0xffffffff } },
            0
        }
    },
    {
        "WORD",
        {
            Data {
              (uint32_t)&forth_do_colon,
              (uint32_t)&forth_word,
              (uint32_t)&forth_exit
            },
            empty_stack,
            { 5, "0123 " }
        },
        {
            { 2, Data { (uint32_t) &forth_word_buffer, 4 } },
            0, // stdin_left
            { 4, "0123" } // word_buff
        }
    },
    {
        "WORD (whitespace)",
        {
            Data {
              (uint32_t)&forth_do_colon,
              (uint32_t)&forth_word,
              (uint32_t)&forth_exit
            },
            empty_stack,
            { 9, " \t\r\n0123 " }
        },
        {
            { 2, Data { (uint32_t) &forth_word_buffer, 4 } },
            0, // stdin_left
            { 4, "0123" } // word_buff
        }
    },
    {
        "WORD (backslash)",
        {
            Data {
              (uint32_t)&forth_do_colon,
              (uint32_t)&forth_word,
              (uint32_t)&forth_exit
            },
            empty_stack,
            { 10, " \\m \n0123 " }
        },
        {
            { 2, Data { (uint32_t) &forth_word_buffer, 4 } },
            0, // stdin_left
            { 4, "0123" } // word_buff
        }
    },
    {
        "INTERPRET (number)",
        {
            Data {
              (uint32_t)&forth_do_colon,
              (uint32_t)&forth_interpret,
              (uint32_t)&forth_exit
            },
            empty_stack,
            { 3, "123" }
        },
        {
            { 1, Data { 123 } },
        }
    },
    {
        "INTERPRET (native word)",
        {
            Data {
              (uint32_t)&forth_do_colon,
              (uint32_t)&forth_interpret,
              (uint32_t)&forth_exit
            },
            { 2, Data { 1, 2 } },
            { 1, "+" }
        },
        {
            { 1, Data { 3 } },
        }
    },
    {
        "INTERPRET (forth word)",
        {
            Data {
              (uint32_t)&forth_do_colon,
              (uint32_t)&forth_interpret,
              (uint32_t)&forth_exit
            },
            empty_stack,
            { 3, "' +" }
        },
        {
            { 1, Data { (uint32_t) &forth_add } },
        }
    },
    {
        "INTERPRET (comp num)",
        {
            Data {
              (uint32_t)&forth_do_colon,
              (uint32_t)&forth_interpret,
              (uint32_t)&forth_exit
            },
            empty_stack,
            { 3, "123" },
            { },
            1 // compile mode
        },
        {
            empty_stack,
            0, // stdin_left
            { 0, "" }, // word_buff
            1 // state
        }
    },
    {
        "INTERPRET (comp word)",
        {
            Data {
              (uint32_t)&forth_do_colon,
              (uint32_t)&forth_interpret,
              (uint32_t)&forth_exit
            },
            empty_stack,
            { 1, "+" },
            { },
            1 // compile mode
        },
        {
            empty_stack,
            0, // stdin_left
            { 0, "" }, // word_buff
            1 // state
        }
    },
    {
        ":",
        {
            Data {
              (uint32_t)&forth_do_colon,
              (uint32_t)&forth_compile_def,
              (uint32_t)&forth_exit
            },
            empty_stack,
            { 3, "123" },
            { }
        },
        {
            empty_stack,
            0, // stdin_left
            { 0, "" }, // word_buff
            1, // state = compile_mode
            forth_var_HERE // latest
        }
    },
    {
        ";",
        {
            Data {
              (uint32_t)&forth_do_colon,
              (uint32_t)&forth_compile_def,
              (uint32_t)&forth_end_compile_def,
              (uint32_t)&forth_exit
            },
            empty_stack,
            { 3, "123" },
            { }
        },
        {
            empty_stack,
            0, // stdin_left
            { 0, "" }, // word_buff
            0, // state
            forth_var_HERE // latest
        }
    },
    {
        "'",
        {
            Data {
              (uint32_t)&forth_do_colon,
              (uint32_t)&forth_code_field_addr_of_next_word,
              (uint32_t)&forth_exit
            },
            empty_stack,
            { 1, "+" }
        },
        {
            { 1, Data { (uint32_t) &forth_add } },
        }
    },
    {
        "LITERAL",
        {
            Data {
              (uint32_t)&forth_do_colon,
              (uint32_t)&forth_literal,
              (uint32_t)&forth_exit
            },
            { 1, Data { (uint32_t) &forth_add } },
        },
        {
            empty_stack
        }
    },
};

static Buff *get_expected_user_mem(const char *test_name) {
  if (!strcmp(test_name, "CREATE")) {
    static char create_data[] { 0, 0, 0, 0, 0, 0, 0, 0, 4, 'B', 'B', 'B', 'B', 0, 0, 0 };
    static Buff create_user_mem = { 16, create_data };

    if (*(uint32_t *)create_user_mem.data == 0) {
      *(uint32_t *)create_user_mem.data = original_var_latest;
      *(uint32_t *)(create_user_mem.data + 4) = original_var_here + 16;
    }
    return &create_user_mem;
  }

  if (!strcmp(test_name, "HIDDEN")) {
    static char create_data[] { 0, 0, 0, 0, 0, 0, 0, 0, 0x24, 'B', 'B', 'B', 'B', 0, 0, 0 };
    static Buff create_user_mem = { 16, create_data };

    if (*(uint32_t *)create_user_mem.data == 0) {
      *(uint32_t *)create_user_mem.data = original_var_latest;
      *(uint32_t *)(create_user_mem.data + 4) = original_var_here + 16;
    }
    return &create_user_mem;
  }

  if (!strcmp(test_name, "HIDDEN (toggle)")) {
    static char create_data[] { 0, 0, 0, 0, 0, 0, 0, 0, 4, 'B', 'B', 'B', 'B', 0, 0, 0 };
    static Buff create_user_mem = { 16, create_data };

    if (*(uint32_t *)create_user_mem.data == 0) {
      *(uint32_t *)create_user_mem.data = original_var_latest;
      *(uint32_t *)(create_user_mem.data + 4) = original_var_here + 16;
    }
    return &create_user_mem;
  }

  if (!strcmp(test_name, ",")) {
    static uint32_t store_to_here_data = 1;
    static Buff store_to_here_user_mem = { 4, (char *)&store_to_here_data };

    return &store_to_here_user_mem;
  }

  if (!strcmp(test_name, "INTERPRET (comp num)")) {
    static uint32_t interpret_comp_num_data[] = { (uint32_t) &forth_lit, 123 };
    static Buff interpret_comp_num_user_mem = { 8, (char *)&interpret_comp_num_data };

    return &interpret_comp_num_user_mem;
  }

  if (!strcmp(test_name, "INTERPRET (comp word)")) {
    static uint32_t interpret_comp_word_data[] = { (uint32_t) &forth_add };
    static Buff interpret_comp_word_user_mem = { 4, (char *)&interpret_comp_word_data };

    return &interpret_comp_word_user_mem;
  }

  if (!strcmp(test_name, ":")) {
    static char create_data[] { 0, 0, 0, 0, 0, 0, 0, 0, 0x23, '1', '2', '3', 0, 0, 0, 0 };
    static Buff create_user_mem = { 16, create_data };

    if (*(uint32_t *)create_user_mem.data == 0) {
      *(uint32_t *)create_user_mem.data = original_var_latest;
      *(uint32_t *)(create_user_mem.data + 4) = original_var_here + 12;
      *(uint32_t *)(create_user_mem.data + 12) = (uint32_t)&forth_do_colon;
    }
    return &create_user_mem;
  }

  if (!strcmp(test_name, ";")) {
    static char create_data[] { 0, 0, 0, 0, 0, 0, 0, 0, 3, '1', '2', '3', 0, 0, 0, 0, 0, 0, 0, 0 };
    static Buff create_user_mem = { 20, create_data };

    if (*(uint32_t *)create_user_mem.data == 0) {
      *(uint32_t *)create_user_mem.data = original_var_latest;
      *(uint32_t *)(create_user_mem.data + 4) = original_var_here + 12;
      *(uint32_t *)(create_user_mem.data + 12) = (uint32_t)&forth_do_colon;
      *(uint32_t *)(create_user_mem.data + 16) = (uint32_t)&forth_exit;
    }
    return &create_user_mem;
  }

  if (!strcmp(test_name, "LITERAL")) {
    static uint32_t literal_data[] = { 0, 0, 0, 0, 0, 0, 0, 0 };
    static Buff literal_user_mem = { 8, (char *)&literal_data };

    if (*(uint32_t *)literal_user_mem.data == 0) {
      *(uint32_t *)literal_user_mem.data = (uint32_t)&forth_lit;
      *(uint32_t *)(literal_user_mem.data + 4) = (uint32_t)&forth_add;
    }
    return &literal_user_mem;
  }

  return nullptr;
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
  original_var_stdin = (const char *) forth_var_STDIN;
  original_var_stdin_count = forth_var_STDIN_COUNT;

  for (int i = 0; i < sizeof(tests)/sizeof(Test); i++) {
    Serial.print(tests[i].name);
    Serial.print("...");
    for (int j = 0; j < 30 - strlen(tests[i].name); j++) Serial.print(' ');

    sp = data_stack;
    const uint32_t *setup_ptr = tests[i].setup.stack.data;
    for (int j = 0; j < tests[i].setup.stack.size; j++, sp++, setup_ptr++) {
      *sp = *setup_ptr;
    }

    // Reset the system
    forth_var_HERE = original_var_here;
    forth_var_LATEST = original_var_latest;
    forth_var_STATE = tests[i].setup.state;
    forth_var_STDIN = (uint32_t) tests[i].setup.stdin_.data;
    forth_var_STDIN_COUNT = tests[i].setup.stdin_.size;

    __disable_irq();
    uint32_t count_start = ARM_DWT_CYCCNT;

    sp = forth_enter(sp, tests[i].setup.program);

    uint32_t count_end = ARM_DWT_CYCCNT;
    __enable_irq();

    // For the QUIT test, we are using less cycles than "normal", so
    // we need a negative number here.
    int32_t cycle_count = int32_t(count_end - count_start) - base_cycle_count;
    uint32_t actual_stack_size = sp - data_stack;

    bool failed = false;
    bool failed_stack = false;
    bool failed_stdin = false;
    bool failed_user_mem = false;
    bool failed_state = false;
    bool failed_latest = false;

    if (actual_stack_size != tests[i].expected.stack.size) {
      failed = true;
      failed_stack = true;
    } else if (memcmp(data_stack, tests[i].expected.stack.data, actual_stack_size * 4)) {
      failed = true;
      failed_stack = true;
    }

    if (forth_var_STDIN_COUNT != tests[i].expected.stdin_left) {
      failed = true;
      failed_stdin = true;
    } else if (forth_var_STDIN + forth_var_STDIN_COUNT
        != (uint32_t) tests[i].setup.stdin_.data + tests[i].setup.stdin_.size) {
      failed = true;
      failed_stdin = true;
    }

    Buff *expected_user_mem = get_expected_user_mem(tests[i].name);
    if (expected_user_mem != nullptr) {
      if (forth_var_HERE != original_var_here + expected_user_mem->size) {
        failed = true;
        failed_user_mem = true;
      } else if (memcmp((void *)original_var_here, expected_user_mem->data, expected_user_mem->size)) {
        failed = true;
        failed_user_mem = true;
      }
    }

    if (forth_var_STATE != tests[i].expected.state) {
      failed = true;
      failed_state = true;
    }

    if (tests[i].expected.latest != 0 && forth_var_LATEST != tests[i].expected.latest) {
      failed = true;
      failed_latest = true;
    }

    if (failed) {
      Serial.println("[FAIL]");
    }

    if (failed_stack) {
      Serial.print("  expected stack (");
      Serial.print(tests[i].expected.stack.size);
      Serial.print("): -- ");
      for (int j = 0; j < tests[i].expected.stack.size; j++) {
        Serial.print(tests[i].expected.stack.data[j], 16);
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

    if (failed_stdin) {
      Serial.print("  expected STDIN to be len ");
      Serial.print(tests[i].expected.stdin_left);
      Serial.print(" @ ");
      Serial.print(((uint32_t) tests[i].setup.stdin_.data) + tests[i].setup.stdin_.size - tests[i].expected.stdin_left, 16);
      Serial.print(" but was len ");
      Serial.print(forth_var_STDIN_COUNT);
      Serial.print(" @ ");
      Serial.println(forth_var_STDIN, 16);
    }

    if (failed_state) {
      Serial.print("  expected STATE to be ");
      Serial.print(tests[i].expected.state);
      Serial.print(" but was ");
      Serial.println(forth_var_STATE);
    }

    if (failed_latest) {
      Serial.print("  expected LATEST to be ");
      Serial.print(tests[i].expected.latest, 16);
      Serial.print(" but was ");
      Serial.println(forth_var_LATEST, 16);
    }

    if (failed_user_mem) {
      Serial.print("  expected HERE to be ");
      Serial.print(original_var_here + expected_user_mem->size, 16);
      Serial.print(" but was ");
      Serial.println(forth_var_HERE, 16);
      Serial.print("  expected user mem: ");
      for (int j = 0; j < expected_user_mem->size; j++) {
        Serial.print((uint8_t)expected_user_mem->data[j], 16);
        Serial.print(" ");
      }
      Serial.println();
      Serial.print("    actual user mem: ");
      for (int j = 0; j < expected_user_mem->size; j++) {
        Serial.print(*(uint8_t *)(original_var_here + j), 16);
        Serial.print(" ");
      }
      Serial.println();
    }

    if (!failed) {
      Serial.print("[PASS] cycles: ");
      Serial.println(cycle_count);
    }

    if (i == 0) base_cycle_count = cycle_count;
  }
}
