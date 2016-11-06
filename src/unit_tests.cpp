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

struct Test {
  const char* name;
  const uint32_t setup_stack_size; // in 4-byte words
  const uint32_t* setup_stack;
  const uint32_t* program;
  const uint32_t expected_stack_size; // in 4-byte words
  const uint32_t* expected_stack;
};

Test tests[] = {
    {
        "QUIT",
        0, (uint32_t[]) { },
        (uint32_t[]) {
          (uint32_t) &forth_quit
        },
        0, (uint32_t[] ) { }
    },
    {
        "LIT",
        0, (uint32_t[]) { },
        (uint32_t[]) {
          (uint32_t)&forth_literal,
          0x1234abcd,
          (uint32_t)&forth_quit
        },
        1, (uint32_t[] ) { 0x1234abcd }
    },
    {
        "DROP",
        1, (uint32_t[]) { 1 },
        (uint32_t[]) {
          (uint32_t)&forth_drop,
          (uint32_t)&forth_quit
        },
        0, (uint32_t[] ) { }
    },
    {
        "2DROP",
        2, (uint32_t[]) { 1, 2 },
        (uint32_t[]) {
          (uint32_t)&forth_2drop,
          (uint32_t)&forth_quit
        },
        0, (uint32_t[] ) { }
    },
    {
        "SWAP",
        2, (uint32_t[]) { 1, 2 },
        (uint32_t[]) {
          (uint32_t)&forth_swap,
          (uint32_t)&forth_quit
        },
        2, (uint32_t[] ) { 2, 1 }
    },
    {
        "2SWAP",
        4, (uint32_t[]) { 1, 2, 3, 4 },
        (uint32_t[]) {
          (uint32_t)&forth_2swap,
          (uint32_t)&forth_quit
        },
        4, (uint32_t[] ) { 3, 4, 1, 2 }
    },
    {
        "DUP",
        1, (uint32_t[]) { 1 },
        (uint32_t[]) {
          (uint32_t)&forth_dup,
          (uint32_t)&forth_quit
        },
        2, (uint32_t[] ) { 1, 1 }
    },
    {
        "2DUP",
        2, (uint32_t[]) { 1, 2 },
        (uint32_t[]) {
          (uint32_t)&forth_2dup,
          (uint32_t)&forth_quit
        },
        4, (uint32_t[] ) { 1, 2, 1, 2 }
    },
    {
        "?DUP (+)",
        1, (uint32_t[]) { 1 },
        (uint32_t[]) {
          (uint32_t)&forth_maybe_dup,
          (uint32_t)&forth_quit
        },
        2, (uint32_t[] ) { 1, 1 }
    },
    {
        "?DUP (-)",
        1, (uint32_t[]) { 0 },
        (uint32_t[]) {
          (uint32_t)&forth_maybe_dup,
          (uint32_t)&forth_quit
        },
        1, (uint32_t[] ) { 0 }
    },
    {
        "OVER",
        2, (uint32_t[]) { 1, 2 },
        (uint32_t[]) {
          (uint32_t)&forth_over,
          (uint32_t)&forth_quit
        },
        3, (uint32_t[] ) { 1, 2, 1 }
    },
    {
        "ROT",
        3, (uint32_t[]) { 1, 2, 3 },
        (uint32_t[]) {
          (uint32_t)&forth_rot,
          (uint32_t)&forth_quit
        },
        3, (uint32_t[] ) { 2, 3, 1 }
    },
    {
        "-ROT",
        3, (uint32_t[]) { 1, 2, 3 },
        (uint32_t[]) {
          (uint32_t)&forth_nrot,
          (uint32_t)&forth_quit
        },
        3, (uint32_t[] ) { 3, 1, 2 }
    },
    {
        "1+",
        1, (uint32_t[]) { 1 },
        (uint32_t[]) {
          (uint32_t)&forth_inc,
          (uint32_t)&forth_quit
        },
        1, (uint32_t[] ) { 2 }
    },
    {
        "1-",
        1, (uint32_t[]) { 1 },
        (uint32_t[]) {
          (uint32_t)&forth_dec,
          (uint32_t)&forth_quit
        },
        1, (uint32_t[] ) { 0 }
    },
    {
        "4+",
        1, (uint32_t[]) { 1 },
        (uint32_t[]) {
          (uint32_t)&forth_inc4,
          (uint32_t)&forth_quit
        },
        1, (uint32_t[] ) { 5 }
    },
    {
        "4-",
        1, (uint32_t[]) { 5 },
        (uint32_t[]) {
          (uint32_t)&forth_dec4,
          (uint32_t)&forth_quit
        },
        1, (uint32_t[] ) { 1 }
    },
    {
        "+",
        2, (uint32_t[]) { 1, 2 },
        (uint32_t[]) {
          (uint32_t)&forth_add,
          (uint32_t)&forth_quit
        },
        1, (uint32_t[] ) { 3 }
    },
    {
        "-",
        2, (uint32_t[]) { 3, 2 },
        (uint32_t[]) {
          (uint32_t)&forth_sub,
          (uint32_t)&forth_quit
        },
        1, (uint32_t[] ) { 1 }
    },
    {
        "*",
        2, (uint32_t[]) { 3, 2 },
        (uint32_t[]) {
          (uint32_t)&forth_mul,
          (uint32_t)&forth_quit
        },
        1, (uint32_t[] ) { 6 }
    },
    {
        "/MOD",
        2, (uint32_t[]) { 7, 2 },
        (uint32_t[]) {
          (uint32_t)&forth_divmod,
          (uint32_t)&forth_quit
        },
        2, (uint32_t[] ) { 1, 3 }
    },
    {
        "/",
        2, (uint32_t[]) { 7, 2 },
        (uint32_t[]) {
          (uint32_t)&forth_div,
          (uint32_t)&forth_quit
        },
        1, (uint32_t[] ) { 3 }
    },
    {
        "= (+)",
        2, (uint32_t[]) { 2, 2 },
        (uint32_t[]) {
          (uint32_t)&forth_eq,
          (uint32_t)&forth_quit
        },
        1, (uint32_t[] ) { 0xffffffff }
    },
    {
        "= (-)",
        2, (uint32_t[]) { 2, 1 },
        (uint32_t[]) {
          (uint32_t)&forth_eq,
          (uint32_t)&forth_quit
        },
        1, (uint32_t[] ) { 0 }
    },
    {
        "<> (+)",
        2, (uint32_t[]) { 2, 1 },
        (uint32_t[]) {
          (uint32_t)&forth_ne,
          (uint32_t)&forth_quit
        },
        1, (uint32_t[] ) { 0xffffffff }
    },
    {
        "<> (-)",
        2, (uint32_t[]) { 2, 2 },
        (uint32_t[]) {
          (uint32_t)&forth_ne,
          (uint32_t)&forth_quit
        },
        1, (uint32_t[] ) { 0 }
    },
    {
        "< (+)",
        2, (uint32_t[]) { 1, 2 },
        (uint32_t[]) {
          (uint32_t)&forth_lt,
          (uint32_t)&forth_quit
        },
        1, (uint32_t[] ) { 0xffffffff }
    },
    {
        "< (+signed)",
        2, (uint32_t[]) { 0xffffffff, 0 },
        (uint32_t[]) {
          (uint32_t)&forth_lt,
          (uint32_t)&forth_quit
        },
        1, (uint32_t[] ) { 0xffffffff }
    },
    {
        "< (-)",
        2, (uint32_t[]) { 2, 2 },
        (uint32_t[]) {
          (uint32_t)&forth_lt,
          (uint32_t)&forth_quit
        },
        1, (uint32_t[] ) { 0 }
    },
    {
        "> (+)",
        2, (uint32_t[]) { 2, 1 },
        (uint32_t[]) {
          (uint32_t)&forth_gt,
          (uint32_t)&forth_quit
        },
        1, (uint32_t[] ) { 0xffffffff }
    },
    {
        "> (+signed)",
        2, (uint32_t[]) { 0, 0xffffffff },
        (uint32_t[]) {
          (uint32_t)&forth_gt,
          (uint32_t)&forth_quit
        },
        1, (uint32_t[] ) { 0xffffffff }
    },
    {
        "> (-)",
        2, (uint32_t[]) { 2, 2 },
        (uint32_t[]) {
          (uint32_t)&forth_gt,
          (uint32_t)&forth_quit
        },
        1, (uint32_t[] ) { 0 }
    },
    {
        "<= (+)",
        2, (uint32_t[]) { 1, 2 },
        (uint32_t[]) {
          (uint32_t)&forth_le,
          (uint32_t)&forth_quit
        },
        1, (uint32_t[] ) { 0xffffffff }
    },
    {
        "<= (+signed)",
        2, (uint32_t[]) { 0xffffffff, 0 },
        (uint32_t[]) {
          (uint32_t)&forth_le,
          (uint32_t)&forth_quit
        },
        1, (uint32_t[] ) { 0xffffffff }
    },
    {
        "<= (+eq)",
        2, (uint32_t[]) { 2, 2 },
        (uint32_t[]) {
          (uint32_t)&forth_le,
          (uint32_t)&forth_quit
        },
        1, (uint32_t[] ) { 0xffffffff }
    },
    {
        "<= (-)",
        2, (uint32_t[]) { 3, 2 },
        (uint32_t[]) {
          (uint32_t)&forth_le,
          (uint32_t)&forth_quit
        },
        1, (uint32_t[] ) { 0 }
    },
    {
        ">= (+)",
        2, (uint32_t[]) { 2, 1 },
        (uint32_t[]) {
          (uint32_t)&forth_ge,
          (uint32_t)&forth_quit
        },
        1, (uint32_t[] ) { 0xffffffff }
    },
    {
        ">= (+signed)",
        2, (uint32_t[]) { 0, 0xffffffff },
        (uint32_t[]) {
          (uint32_t)&forth_ge,
          (uint32_t)&forth_quit
        },
        1, (uint32_t[] ) { 0xffffffff }
    },
    {
        ">= (+eq)",
        2, (uint32_t[]) { 2, 2 },
        (uint32_t[]) {
          (uint32_t)&forth_ge,
          (uint32_t)&forth_quit
        },
        1, (uint32_t[] ) { 0xffffffff }
    },
    {
        ">= (-)",
        2, (uint32_t[]) { 2, 3 },
        (uint32_t[]) {
          (uint32_t)&forth_ge,
          (uint32_t)&forth_quit
        },
        1, (uint32_t[] ) { 0 }
    },
    {
        "0= (+)",
        1, (uint32_t[]) { 0 },
        (uint32_t[]) {
          (uint32_t)&forth_eqz,
          (uint32_t)&forth_quit
        },
        1, (uint32_t[] ) { 0xffffffff }
    },
    {
        "0= (-)",
        1, (uint32_t[]) { 2 },
        (uint32_t[]) {
          (uint32_t)&forth_eqz,
          (uint32_t)&forth_quit
        },
        1, (uint32_t[] ) { 0 }
    },
    {
        "0<> (+)",
        1, (uint32_t[]) { 2 },
        (uint32_t[]) {
          (uint32_t)&forth_nez,
          (uint32_t)&forth_quit
        },
        1, (uint32_t[] ) { 0xffffffff }
    },
    {
        "0<> (-)",
        1, (uint32_t[]) { 0 },
        (uint32_t[]) {
          (uint32_t)&forth_nez,
          (uint32_t)&forth_quit
        },
        1, (uint32_t[] ) { 0 }
    },
    {
        "0< (+signed)",
        1, (uint32_t[]) { 0xffffffff },
        (uint32_t[]) {
          (uint32_t)&forth_ltz,
          (uint32_t)&forth_quit
        },
        1, (uint32_t[] ) { 0xffffffff }
    },
    {
        "0< (-)",
        1, (uint32_t[]) { 0 },
        (uint32_t[]) {
          (uint32_t)&forth_ltz,
          (uint32_t)&forth_quit
        },
        1, (uint32_t[] ) { 0 }
    },
    {
        "0> (+)",
        1, (uint32_t[]) { 2 },
        (uint32_t[]) {
          (uint32_t)&forth_gtz,
          (uint32_t)&forth_quit
        },
        1, (uint32_t[] ) { 0xffffffff }
    },
    {
        "0> (-signed)",
        1, (uint32_t[]) { 0xffffffff },
        (uint32_t[]) {
          (uint32_t)&forth_gtz,
          (uint32_t)&forth_quit
        },
        1, (uint32_t[] ) { 0 }
    },
    {
        "0> (-)",
        1, (uint32_t[]) { 0 },
        (uint32_t[]) {
          (uint32_t)&forth_gtz,
          (uint32_t)&forth_quit
        },
        1, (uint32_t[] ) { 0 }
    },
    {
        "0<= (+)",
        1, (uint32_t[]) { 0 },
        (uint32_t[]) {
          (uint32_t)&forth_lez,
          (uint32_t)&forth_quit
        },
        1, (uint32_t[] ) { 0xffffffff }
    },
    {
        "0<= (+signed)",
        1, (uint32_t[]) { 0xffffffff },
        (uint32_t[]) {
          (uint32_t)&forth_lez,
          (uint32_t)&forth_quit
        },
        1, (uint32_t[] ) { 0xffffffff }
    },
    {
        "0<= (-)",
        1, (uint32_t[]) { 1 },
        (uint32_t[]) {
          (uint32_t)&forth_lez,
          (uint32_t)&forth_quit
        },
        1, (uint32_t[] ) { 0 }
    },
    {
        "0>= (+)",
        1, (uint32_t[]) { 1 },
        (uint32_t[]) {
          (uint32_t)&forth_gez,
          (uint32_t)&forth_quit
        },
        1, (uint32_t[] ) { 0xffffffff }
    },
    {
        "0>= (+eq)",
        1, (uint32_t[]) { 0 },
        (uint32_t[]) {
          (uint32_t)&forth_gez,
          (uint32_t)&forth_quit
        },
        1, (uint32_t[] ) { 0xffffffff }
    },
    {
        "0>= (-)",
        1, (uint32_t[]) { 0xffffffff },
        (uint32_t[]) {
          (uint32_t)&forth_gez,
          (uint32_t)&forth_quit
        },
        1, (uint32_t[] ) { 0 }
    },
    {
        "AND",
        2, (uint32_t[]) { 0xf0f0f0f0, 0xffff0000 },
        (uint32_t[]) {
          (uint32_t)&forth_and,
          (uint32_t)&forth_quit
        },
        1, (uint32_t[] ) { 0xf0f00000 }
    },
    {
        "OR",
        2, (uint32_t[]) { 0xf0f0f0f0, 0xffff0000 },
        (uint32_t[]) {
          (uint32_t)&forth_or,
          (uint32_t)&forth_quit
        },
        1, (uint32_t[] ) { 0xfffff0f0 }
    },
    {
        "XOR",
        2, (uint32_t[]) { 0xf0f0f0f0, 0xffff0000 },
        (uint32_t[]) {
          (uint32_t)&forth_xor,
          (uint32_t)&forth_quit
        },
        1, (uint32_t[] ) { 0x0f0ff0f0 }
    },
    {
        "INVERT",
        1, (uint32_t[]) { 0xf0f0f0f0 },
        (uint32_t[]) {
          (uint32_t)&forth_not,
          (uint32_t)&forth_quit
        },
        1, (uint32_t[] ) { 0x0f0f0f0f }
    },
    {
        "!",
        3, (uint32_t[]) { 0xdeadbeef, 0xfeedface, (uint32_t)data_stack },
        (uint32_t[]) {
          (uint32_t)&forth_store,
          (uint32_t)&forth_quit
        },
        1, (uint32_t[] ) { 0xfeedface }
    },
    {
        "C!",
        3, (uint32_t[]) { 0xdeadbeef, 0xfeedface, (uint32_t)data_stack },
        (uint32_t[]) {
          (uint32_t)&forth_store_char,
          (uint32_t)&forth_quit
        },
        1, (uint32_t[] ) { 0xdeadbece }
    },
    {
        "@",
        2, (uint32_t[]) { 0xdeadbeef, (uint32_t)data_stack },
        (uint32_t[]) {
          (uint32_t)&forth_fetch,
          (uint32_t)&forth_quit
        },
        2, (uint32_t[] ) { 0xdeadbeef, 0xdeadbeef }
    },
    {
        "C@",
        2, (uint32_t[]) { 0xdeadbeef, (uint32_t)data_stack },
        (uint32_t[]) {
          (uint32_t)&forth_fetch_char,
          (uint32_t)&forth_quit
        },
        2, (uint32_t[] ) { 0xdeadbeef, 0xef }
    },
    {
        "+!",
        3, (uint32_t[]) { 1, 2, (uint32_t)data_stack },
        (uint32_t[]) {
          (uint32_t)&forth_addstore,
          (uint32_t)&forth_quit
        },
        1, (uint32_t[] ) { 3 }
    },
    {
        "-!",
        3, (uint32_t[]) { 2, 1, (uint32_t)data_stack },
        (uint32_t[]) {
          (uint32_t)&forth_substore,
          (uint32_t)&forth_quit
        },
        1, (uint32_t[] ) { 1 }
    },
    {
        "MEMCPY",
        9, (uint32_t[]) { 1, 2, 3, 4, 5, 6, (uint32_t)(data_stack+6), (uint32_t)(data_stack+3), 12 },
        (uint32_t[]) {
          (uint32_t)&forth_memcpy,
          (uint32_t)&forth_quit
        },
        6, (uint32_t[] ) { 1, 2, 3, 1, 2, 3 }
    },
};

void fail_unit_test(int i) {
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

  for (int i = 0; i < sizeof(tests)/sizeof(Test); i++) {
    Serial.print(tests[i].name);
    Serial.print("...");
    for (int j = 0; j < 20 - strlen(tests[i].name); j++) Serial.print(' ');

    sp = data_stack;
    const uint32_t *setup_ptr = tests[i].setup_stack;
    for (int j = 0; j < tests[i].setup_stack_size; j++, sp++, setup_ptr++) {
      *sp = *setup_ptr;
    }

    __disable_irq();
    uint32_t count_start = ARM_DWT_CYCCNT;

    sp = forth_enter(sp, tests[i].program);

    uint32_t count_end = ARM_DWT_CYCCNT;
    __enable_irq();

    uint32_t cycle_count = count_end - count_start - base_cycle_count;
    uint32_t actual_stack_size = sp - data_stack;

    if (actual_stack_size != tests[i].expected_stack_size) {
      fail_unit_test(i);
    } else if (memcmp(data_stack, tests[i].expected_stack, actual_stack_size * 4)) {
      fail_unit_test(i);
    } else {
      Serial.print("[PASS] cycles: ");
      Serial.println(cycle_count);
    }

    if (i == 0) base_cycle_count = cycle_count;
  }
}
