#include "WProgram.h"
#include <stdint.h>
#include <usb_serial.h>
#include <forth_system.h>

extern void run_unit_tests();

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

  run_unit_tests();

  pinMode(0, OUTPUT);
  while (1) {
    delay(1000);
    digitalWriteFast(0, HIGH);
    delay(100);
    digitalWriteFast(0, LOW);
  }
}

