/*
 * unit_tests.cpp
 *
 *  Created on: Nov 5, 2016
 *      Author: Robert
 */

#include <forth_system.h>
#include "WProgram.h"
#include <usb_serial.h>

struct Test {
  const char* name;
  const uint32_t* program;
  const uint32_t expected_stack_size; // in 4-byte words
  const uint32_t* expected_stack;
};

Test tests[] = {
    {
        "QUIT",
        (uint32_t[]) {
          (uint32_t) &forth_quit
        },
        0, (uint32_t[] ) { }
    },
    {
        "LIT",
        (uint32_t[]) {
          (uint32_t)&forth_literal,
          0x1234abcd,
          (uint32_t)&forth_quit
        },
        1, (uint32_t[] ) { 0x1234abcd }
    },
    {
        "DROP",
        (uint32_t[]) {
          (uint32_t)&forth_literal,
          1,
          (uint32_t)&forth_drop,
          (uint32_t)&forth_quit
        },
        0, (uint32_t[] ) { }
    },
    {
        "2DROP",
        (uint32_t[]) {
          (uint32_t)&forth_literal,
          1,
          (uint32_t)&forth_literal,
          2,
          (uint32_t)&forth_2drop,
          (uint32_t)&forth_quit
        },
        0, (uint32_t[] ) { }
    },
    {
        "SWAP",
        (uint32_t[]) {
          (uint32_t)&forth_literal,
          1,
          (uint32_t)&forth_literal,
          2,
          (uint32_t)&forth_swap,
          (uint32_t)&forth_quit
        },
        2, (uint32_t[] ) { 2, 1 }
    },
    {
        "2SWAP",
        (uint32_t[]) {
          (uint32_t)&forth_literal,
          1,
          (uint32_t)&forth_literal,
          2,
          (uint32_t)&forth_literal,
          3,
          (uint32_t)&forth_literal,
          4,
          (uint32_t)&forth_2swap,
          (uint32_t)&forth_quit
        },
        4, (uint32_t[] ) { 3, 4, 1, 2 }
    },
    {
        "DUP",
        (uint32_t[]) {
          (uint32_t)&forth_literal,
          1,
          (uint32_t)&forth_dup,
          (uint32_t)&forth_quit
        },
        2, (uint32_t[] ) { 1, 1 }
    },
    {
        "2DUP",
        (uint32_t[]) {
          (uint32_t)&forth_literal,
          1,
          (uint32_t)&forth_literal,
          2,
          (uint32_t)&forth_2dup,
          (uint32_t)&forth_quit
        },
        4, (uint32_t[] ) { 1, 2, 1, 2 }
    },
    {
        "?DUP (+)",
        (uint32_t[]) {
          (uint32_t)&forth_literal,
          1,
          (uint32_t)&forth_maybe_dup,
          (uint32_t)&forth_quit
        },
        2, (uint32_t[] ) { 1, 1 }
    },
    {
        "?DUP (-)",
        (uint32_t[]) {
          (uint32_t)&forth_literal,
          0,
          (uint32_t)&forth_maybe_dup,
          (uint32_t)&forth_quit
        },
        1, (uint32_t[] ) { 0 }
    },
    {
        "OVER",
        (uint32_t[]) {
          (uint32_t)&forth_literal,
          1,
          (uint32_t)&forth_literal,
          2,
          (uint32_t)&forth_over,
          (uint32_t)&forth_quit
        },
        3, (uint32_t[] ) { 1, 2, 1 }
    },
    {
        "ROT",
        (uint32_t[]) {
          (uint32_t)&forth_literal,
          1,
          (uint32_t)&forth_literal,
          2,
          (uint32_t)&forth_literal,
          3,
          (uint32_t)&forth_rot,
          (uint32_t)&forth_quit
        },
        3, (uint32_t[] ) { 2, 3, 1 }
    },
    {
        "-ROT",
        (uint32_t[]) {
          (uint32_t)&forth_literal,
          1,
          (uint32_t)&forth_literal,
          2,
          (uint32_t)&forth_literal,
          3,
          (uint32_t)&forth_nrot,
          (uint32_t)&forth_quit
        },
        3, (uint32_t[] ) { 3, 1, 2 }
    },
    {
        "1+",
        (uint32_t[]) {
          (uint32_t)&forth_literal,
          1,
          (uint32_t)&forth_inc,
          (uint32_t)&forth_quit
        },
        1, (uint32_t[] ) { 2 }
    },
    {
        "1-",
        (uint32_t[]) {
          (uint32_t)&forth_literal,
          1,
          (uint32_t)&forth_dec,
          (uint32_t)&forth_quit
        },
        1, (uint32_t[] ) { 0 }
    },
    {
        "4+",
        (uint32_t[]) {
          (uint32_t)&forth_literal,
          1,
          (uint32_t)&forth_inc4,
          (uint32_t)&forth_quit
        },
        1, (uint32_t[] ) { 5 }
    },
    {
        "4-",
        (uint32_t[]) {
          (uint32_t)&forth_literal,
          5,
          (uint32_t)&forth_dec4,
          (uint32_t)&forth_quit
        },
        1, (uint32_t[] ) { 1 }
    },
    {
        "+",
        (uint32_t[]) {
          (uint32_t)&forth_literal,
          1,
          (uint32_t)&forth_literal,
          2,
          (uint32_t)&forth_add,
          (uint32_t)&forth_quit
        },
        1, (uint32_t[] ) { 3 }
    },
    {
        "-",
        (uint32_t[]) {
          (uint32_t)&forth_literal,
          3,
          (uint32_t)&forth_literal,
          2,
          (uint32_t)&forth_sub,
          (uint32_t)&forth_quit
        },
        1, (uint32_t[] ) { 1 }
    },
    {
        "*",
        (uint32_t[]) {
          (uint32_t)&forth_literal,
          3,
          (uint32_t)&forth_literal,
          2,
          (uint32_t)&forth_mul,
          (uint32_t)&forth_quit
        },
        1, (uint32_t[] ) { 6 }
    },
    {
        "/MOD",
        (uint32_t[]) {
          (uint32_t)&forth_literal,
          7,
          (uint32_t)&forth_literal,
          2,
          (uint32_t)&forth_divmod,
          (uint32_t)&forth_quit
        },
        2, (uint32_t[] ) { 1, 3 }
    },
    {
        "/",
        (uint32_t[]) {
          (uint32_t)&forth_literal,
          7,
          (uint32_t)&forth_literal,
          2,
          (uint32_t)&forth_div,
          (uint32_t)&forth_quit
        },
        1, (uint32_t[] ) { 3 }
    },
    {
        "eq (+)",
        (uint32_t[]) {
          (uint32_t)&forth_literal,
          2,
          (uint32_t)&forth_literal,
          2,
          (uint32_t)&forth_eq,
          (uint32_t)&forth_quit
        },
        1, (uint32_t[] ) { 0xffffffff }
    },
    {
        "eq (-)",
        (uint32_t[]) {
          (uint32_t)&forth_literal,
          2,
          (uint32_t)&forth_literal,
          1,
          (uint32_t)&forth_eq,
          (uint32_t)&forth_quit
        },
        1, (uint32_t[] ) { 0 }
    },
    {
        "ne (+)",
        (uint32_t[]) {
          (uint32_t)&forth_literal,
          2,
          (uint32_t)&forth_literal,
          1,
          (uint32_t)&forth_ne,
          (uint32_t)&forth_quit
        },
        1, (uint32_t[] ) { 0xffffffff }
    },
    {
        "ne (-)",
        (uint32_t[]) {
          (uint32_t)&forth_literal,
          2,
          (uint32_t)&forth_literal,
          2,
          (uint32_t)&forth_ne,
          (uint32_t)&forth_quit
        },
        1, (uint32_t[] ) { 0 }
    },
    {
        "lt (+)",
        (uint32_t[]) {
          (uint32_t)&forth_literal,
          1,
          (uint32_t)&forth_literal,
          2,
          (uint32_t)&forth_lt,
          (uint32_t)&forth_quit
        },
        1, (uint32_t[] ) { 0xffffffff }
    },
    {
        "lt (+signed)",
        (uint32_t[]) {
          (uint32_t)&forth_literal,
          0xffffffff,
          (uint32_t)&forth_literal,
          0,
          (uint32_t)&forth_lt,
          (uint32_t)&forth_quit
        },
        1, (uint32_t[] ) { 0xffffffff }
    },
    {
        "lt (-)",
        (uint32_t[]) {
          (uint32_t)&forth_literal,
          2,
          (uint32_t)&forth_literal,
          2,
          (uint32_t)&forth_lt,
          (uint32_t)&forth_quit
        },
        1, (uint32_t[] ) { 0 }
    },
    {
        "gt (+)",
        (uint32_t[]) {
          (uint32_t)&forth_literal,
          2,
          (uint32_t)&forth_literal,
          1,
          (uint32_t)&forth_gt,
          (uint32_t)&forth_quit
        },
        1, (uint32_t[] ) { 0xffffffff }
    },
    {
        "gt (+signed)",
        (uint32_t[]) {
          (uint32_t)&forth_literal,
          0,
          (uint32_t)&forth_literal,
          0xffffffff,
          (uint32_t)&forth_gt,
          (uint32_t)&forth_quit
        },
        1, (uint32_t[] ) { 0xffffffff }
    },
    {
        "gt (-)",
        (uint32_t[]) {
          (uint32_t)&forth_literal,
          2,
          (uint32_t)&forth_literal,
          2,
          (uint32_t)&forth_gt,
          (uint32_t)&forth_quit
        },
        1, (uint32_t[] ) { 0 }
    },
    {
        "le (+)",
        (uint32_t[]) {
          (uint32_t)&forth_literal,
          1,
          (uint32_t)&forth_literal,
          2,
          (uint32_t)&forth_le,
          (uint32_t)&forth_quit
        },
        1, (uint32_t[] ) { 0xffffffff }
    },
    {
        "le (+signed)",
        (uint32_t[]) {
          (uint32_t)&forth_literal,
          0xffffffff,
          (uint32_t)&forth_literal,
          0,
          (uint32_t)&forth_le,
          (uint32_t)&forth_quit
        },
        1, (uint32_t[] ) { 0xffffffff }
    },
    {
        "le (+eq)",
        (uint32_t[]) {
          (uint32_t)&forth_literal,
          2,
          (uint32_t)&forth_literal,
          2,
          (uint32_t)&forth_le,
          (uint32_t)&forth_quit
        },
        1, (uint32_t[] ) { 0xffffffff }
    },
    {
        "le (-)",
        (uint32_t[]) {
          (uint32_t)&forth_literal,
          3,
          (uint32_t)&forth_literal,
          2,
          (uint32_t)&forth_le,
          (uint32_t)&forth_quit
        },
        1, (uint32_t[] ) { 0 }
    },
    {
        "ge (+)",
        (uint32_t[]) {
          (uint32_t)&forth_literal,
          2,
          (uint32_t)&forth_literal,
          1,
          (uint32_t)&forth_ge,
          (uint32_t)&forth_quit
        },
        1, (uint32_t[] ) { 0xffffffff }
    },
    {
        "ge (+signed)",
        (uint32_t[]) {
          (uint32_t)&forth_literal,
          0,
          (uint32_t)&forth_literal,
          0xffffffff,
          (uint32_t)&forth_ge,
          (uint32_t)&forth_quit
        },
        1, (uint32_t[] ) { 0xffffffff }
    },
    {
        "ge (+eq)",
        (uint32_t[]) {
          (uint32_t)&forth_literal,
          2,
          (uint32_t)&forth_literal,
          2,
          (uint32_t)&forth_ge,
          (uint32_t)&forth_quit
        },
        1, (uint32_t[] ) { 0xffffffff }
    },
    {
        "ge (-)",
        (uint32_t[]) {
          (uint32_t)&forth_literal,
          2,
          (uint32_t)&forth_literal,
          3,
          (uint32_t)&forth_ge,
          (uint32_t)&forth_quit
        },
        1, (uint32_t[] ) { 0 }
    },
    {
        "eqz (+)",
        (uint32_t[]) {
          (uint32_t)&forth_literal,
          0,
          (uint32_t)&forth_eqz,
          (uint32_t)&forth_quit
        },
        1, (uint32_t[] ) { 0xffffffff }
    },
    {
        "eqz (-)",
        (uint32_t[]) {
          (uint32_t)&forth_literal,
          2,
          (uint32_t)&forth_eqz,
          (uint32_t)&forth_quit
        },
        1, (uint32_t[] ) { 0 }
    },
    {
        "nez (+)",
        (uint32_t[]) {
          (uint32_t)&forth_literal,
          2,
          (uint32_t)&forth_nez,
          (uint32_t)&forth_quit
        },
        1, (uint32_t[] ) { 0xffffffff }
    },
    {
        "nez (-)",
        (uint32_t[]) {
          (uint32_t)&forth_literal,
          0,
          (uint32_t)&forth_nez,
          (uint32_t)&forth_quit
        },
        1, (uint32_t[] ) { 0 }
    },
    {
        "ltz (+signed)",
        (uint32_t[]) {
          (uint32_t)&forth_literal,
          0xffffffff,
          (uint32_t)&forth_ltz,
          (uint32_t)&forth_quit
        },
        1, (uint32_t[] ) { 0xffffffff }
    },
    {
        "ltz (-)",
        (uint32_t[]) {
          (uint32_t)&forth_literal,
          0,
          (uint32_t)&forth_ltz,
          (uint32_t)&forth_quit
        },
        1, (uint32_t[] ) { 0 }
    },
    {
        "gtz (+)",
        (uint32_t[]) {
          (uint32_t)&forth_literal,
          2,
          (uint32_t)&forth_gtz,
          (uint32_t)&forth_quit
        },
        1, (uint32_t[] ) { 0xffffffff }
    },
    {
        "gtz (-signed)",
        (uint32_t[]) {
          (uint32_t)&forth_literal,
          0xffffffff,
          (uint32_t)&forth_gtz,
          (uint32_t)&forth_quit
        },
        1, (uint32_t[] ) { 0 }
    },
    {
        "gtz (-)",
        (uint32_t[]) {
          (uint32_t)&forth_literal,
          0,
          (uint32_t)&forth_gtz,
          (uint32_t)&forth_quit
        },
        1, (uint32_t[] ) { 0 }
    },
    {
        "lez (+)",
        (uint32_t[]) {
          (uint32_t)&forth_literal,
          0,
          (uint32_t)&forth_lez,
          (uint32_t)&forth_quit
        },
        1, (uint32_t[] ) { 0xffffffff }
    },
    {
        "lez (+signed)",
        (uint32_t[]) {
          (uint32_t)&forth_literal,
          0xffffffff,
          (uint32_t)&forth_lez,
          (uint32_t)&forth_quit
        },
        1, (uint32_t[] ) { 0xffffffff }
    },
    {
        "lez (-)",
        (uint32_t[]) {
          (uint32_t)&forth_literal,
          1,
          (uint32_t)&forth_lez,
          (uint32_t)&forth_quit
        },
        1, (uint32_t[] ) { 0 }
    },
    {
        "gez (+)",
        (uint32_t[]) {
          (uint32_t)&forth_literal,
          1,
          (uint32_t)&forth_gez,
          (uint32_t)&forth_quit
        },
        1, (uint32_t[] ) { 0xffffffff }
    },
    {
        "gez (+eq)",
        (uint32_t[]) {
          (uint32_t)&forth_literal,
          0,
          (uint32_t)&forth_gez,
          (uint32_t)&forth_quit
        },
        1, (uint32_t[] ) { 0xffffffff }
    },
    {
        "gez (-)",
        (uint32_t[]) {
          (uint32_t)&forth_literal,
          0xffffffff,
          (uint32_t)&forth_gez,
          (uint32_t)&forth_quit
        },
        1, (uint32_t[] ) { 0 }
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
  Serial.println("UNIT TESTING");
  Serial.println();

  for (int i = 0; i < sizeof(tests)/sizeof(Test); i++) {
    Serial.print(tests[i].name);
    Serial.print("...");
    for (int j = 0; j < 20 - strlen(tests[i].name); j++) Serial.print(' ');

    sp = data_stack;
    sp = forth_enter(sp, tests[i].program);

    uint32_t actual_stack_size = sp - data_stack;

    if (actual_stack_size != tests[i].expected_stack_size) {
      fail_unit_test(i);
    } else if (memcmp(data_stack, tests[i].expected_stack, actual_stack_size * 4)) {
      fail_unit_test(i);
    } else {
      Serial.println("[PASS]");
    }
  }
}
