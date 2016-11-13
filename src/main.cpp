#include "WProgram.h"
#include <stdint.h>
#include <usb_serial.h>
#include <forth_system.h>

extern void run_unit_tests();
extern void interpret();
extern const char *bootstrap;

static constexpr int stack_size = 1024;
uint32_t *sp;
uint32_t data_stack[stack_size];

inline uint32_t word_at(uint32_t addr) {
  return *(uint32_t *)addr;
}

void dump_stack() {
  uint32_t *ptr = data_stack;
  Serial.print("-- ");
  while (ptr != sp) {
    Serial.print(*ptr, 16);
    Serial.print(' ');
    ptr++;
  }
  Serial.println();
  Serial.print("  stdin: ");
  Serial.println((char *)forth_var_STDIN);
}

uint32_t word_for(uint32_t word) {
  if (word == (uint32_t)&forth_do_colon) {
    return 0;
  }
  uint32_t word_ptr = forth_var_LATEST;
  for (uint32_t word_ptr = forth_var_LATEST; word_ptr != 0; word_ptr = word_at(word_ptr)) {
    if (word == word_at(word_ptr + 4)) return word_ptr;
  }
  return 0;
}

void dump_word(const char *word) {
  uint32_t word_ptr = forth_var_LATEST;
  uint32_t word_len = strlen(word);

  for (uint32_t word_ptr = forth_var_LATEST; word_ptr != 0; word_ptr = word_at(word_ptr)) {
    uint8_t len = *(uint8_t *)(word_ptr + 8);
    if ((len & 31) != word_len) continue;
    if (memcmp((void *)(word_ptr + 9), word, word_len)) continue;
    Serial.print("Found word ");
    Serial.print(word);
    Serial.print(" at ");
    Serial.print(word_ptr, 16);
    Serial.print(" [");
    Serial.print(len, 16);
    Serial.println("]");
    if ((len & 0x20) == 0x20) {
      Serial.println(" (It is hidden)");
      return;
    }

    word_ptr = word_at(word_ptr + 4);
    if (word_at(word_ptr) != (uint32_t)&forth_do_colon) {
      Serial.println("  It is a native word.");
      return;
    }

    while (word_at(word_ptr) != (uint32_t)&forth_exit) {
      uint32_t def_ptr = word_for(word_at(word_ptr));
      if (def_ptr != 0) {
        len = 31 & *(uint8_t *)(def_ptr + 8);
        for (uint8_t i = 0; i < len; i++) Serial.print(*(char *)(def_ptr + 9 + i));
      } else {
        Serial.print(word_at(word_ptr), 16);
      }
      Serial.print(' ');
      word_ptr += 4;
    }
    Serial.println();
    return;
  }
  Serial.print("Word ");
  Serial.print(word);
  Serial.println(" not found.");
}

extern "C" int main(void)
{
  delay(2000); // delay for USB to get enumerated

  sp = data_stack;

  // Make sure you set your terminal to send and receive LF
  // as the newline character, and to local echo.
  Serial.println();
  Serial.println("PIXIEFORTH READY");
  Serial.println();
  Serial.print("HEAP START     : ");
  Serial.println((uint32_t)&_sheap, 16);
  Serial.print("HEAP END       : ");
  Serial.println((uint32_t)&_eheap, 16);
  Serial.print("RAM END        : ");
  Serial.println((uint32_t)&_estack, 16);
  Serial.print("AVAILABLE HEAP : ");
  Serial.print((uint32_t)&_eheap - (uint32_t)&_sheap, 16);
  Serial.print(" (");
  Serial.print(((uint32_t)&_eheap - (uint32_t)&_sheap)/1000);
  Serial.println(" kiB)");
  Serial.print("AVAILABLE STACK: ");
  Serial.print((uint32_t)&_estack - (uint32_t)&_eheap, 16);
  Serial.print(" (");
  Serial.print(((uint32_t)&_estack - (uint32_t)&_eheap)/1000);
  Serial.println(" kiB)");
  Serial.println();

  //run_unit_tests();
  forth_var_STDIN = (uint32_t)bootstrap;
  forth_var_STDIN_COUNT = strlen(bootstrap);
  while (true) {
    interpret();
    //dump_stack();
    //dump_word("OK");
  }

  pinMode(0, OUTPUT);
  while (1) {
    delay(1000);
    digitalWriteFast(0, HIGH);
    delay(100);
    digitalWriteFast(0, LOW);
  }
}

static uint32_t interpret_loop[] = { (uint32_t)&forth_do_colon, (uint32_t)&forth_interpret, (uint32_t)&forth_exit };
void interpret() {
  sp = forth_enter(sp, interpret_loop);
}

const char *bootstrap = R"END(
  : OK 
    [ CHAR O ] LITERAL EMIT
    [ CHAR K ] LITERAL EMIT
    NL
  ;
  NL
  CHAR N EMIT NL
  OK
)END";
