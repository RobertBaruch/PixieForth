#include "WProgram.h"
#include <usb_serial.h>
#include <PicturedNumericArea.h>
#include <DataStack.h>
#include <System.h>

extern "C" void forth_do_colon(void);
extern "C" uint32_t forth_testing;
extern "C" uint32_t* forth_enter(uint32_t* param_stack, uint32_t* forth_word);

typedef struct {
    const char *name;
    void (*fn)();
} fndef;

fndef defs[] = {
        {"+", forth::System::Add},
        {"!", forth::System::Store},
        {nullptr, nullptr}
};

bool parse_hex(char *buff) {
    uint32_t num = 0;
    char *ptr = buff;
    while (*ptr) {
        if (*ptr >= '0' && *ptr <= '9') {
            num = num * 16 + (*ptr - '0');
        } else if (*ptr >= 'a' && *ptr <= 'f') {
            num = num * 16 + 10 + (*ptr - 'a');
        } else if (*ptr >= 'A' && *ptr <= 'F') {
            num = num * 16 + 10 + (*ptr - 'A');
        } else {
            while (*ptr) ptr++; // skip the rest, ambiguous condition
            return false;
        }
        ptr++;
    }
    forth::System::Push(num);
    return true;
}

bool parse_dec(char *buff) {
    uint32_t num = 0;
    char *ptr = buff;
    while (*ptr) {
        if (*ptr >= '0' && *ptr <= '9') {
            num = num * 10 + (*ptr - '0');
        } else {
            while (*ptr) ptr++; // skip the rest, ambiguous condition
            return false;
        }
        ptr++;
    }
    forth::System::Push(num);
    return true;
}

bool parse_neg_dec(char *buff) {
    if (parse_dec(buff)) {
        int32_t num = (int32_t) forth::System::Pop();
        forth::System::Push(-num);
        return true;
    }
    return false;
}

bool parse2(char *buff, int len) {
    if (len <= 0) return true;

    char *start = buff;
    while (*start == ' ') start++;

    char *end = start;
    while (*end && *end != ' ') end++;
    *end = 0;
    if (start == end) return true;
    len -= end - buff + 1;
    buff = end + 1;
    Serial.print("parse: ");
    Serial.println(start);

    fndef *defptr = defs;
    while (defptr->name != nullptr) {
        const char *name_ptr = defptr->name;
        char *word_ptr = start;
        while (*name_ptr && *word_ptr && *name_ptr == *word_ptr) {
            name_ptr++;
            word_ptr++;
        }
        if (*name_ptr == *word_ptr) {
            defptr->fn();
            return parse2(buff, len);
        }
        defptr++;
    }

    if (*start == '$') {
        if (!parse_hex(start + 1)) return false;
    } else if (*start == '-') {
        if (!parse_neg_dec(start + 1)) return false;
    } else {
        if (!parse_dec(start)) return false;
    }
    return parse2(buff, len);
}

char word_buff[128];

extern "C" int main(void)
{
    delay(2000); // delay for USB to get ready

    forth::System& system = forth::System::instance();

    char buff[] = "$a -2 ADD";
    int len = strlen(buff);
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

