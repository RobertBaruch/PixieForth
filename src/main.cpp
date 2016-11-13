#include "WProgram.h"
#include <cstdint>
#include <usb_serial.h>
#include <forth_system.h>

extern void run_unit_tests();
extern void interpret();
extern const char *bootstrap;

static constexpr int stack_size = 1024;
uint32_t *sp;
uint32_t data_stack[stack_size];

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

  // run_unit_tests();
  forth_var_STDIN = (uint32_t)bootstrap;
  forth_var_STDIN_COUNT = strlen(bootstrap);
  while (true) {
    interpret();
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
    CHAR O EMIT
    CHAR K EMIT
    NL
  ;
  OK
)END";
