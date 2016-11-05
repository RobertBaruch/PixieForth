#include "WProgram.h"
#include <usb_serial.h>
#include <PicturedNumericArea.h>
#include <DataStack.h>
#include <System.h>

extern "C" void forth_do_colon(void);
extern "C" uint32_t forth_testing;
extern "C" uint32_t* forth_enter(uint32_t* param_stack, uint32_t* forth_word);

extern "C" int main(void)
{
    delay(2000); // delay for USB to get ready

    forth::System& system = forth::System::instance();

    uint32_t **sp_ptr = system.GetSPPtr();

    // Make sure you set your terminal to send and receive LF
    // as the newline character, and to local echo.
    Serial.println("\nPIXIEFORTH READY");

    pinMode(0, OUTPUT);
    while (1) {
        //delay(200);
        digitalWriteFast(0, HIGH);
        *sp_ptr = forth_enter(*sp_ptr, &forth_testing);
        //delay(200);
        digitalWriteFast(0, LOW);
        *sp_ptr = forth_enter(*sp_ptr, &forth_testing);
    }
}

