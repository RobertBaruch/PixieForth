/*
 * parse.c
 *
 *  Created on: Oct 21, 2016
 *      Author: Robert
 */

#include <stdint.h>

// Note that dict_root is a *thing*, not an address. To get its
// address we need &dict_root. This is the true pointer
// to dict_root.
extern uint32_t dict_root;

uint32_t* parse(char *start) {
    while (*start == ' ')
        start++;

    char *end = start;
    while (*end && *end != ' ')
        end++;
    *end = 0;

    uint32_t* dict_ptr = &dict_root;
    while (dict_ptr) {
        char *name_ptr = (char *) (dict_ptr + 2);
        char *word_ptr = start;
        while (*name_ptr && *word_ptr && *name_ptr == *word_ptr) {
            name_ptr++;
            word_ptr++;
        }
        if (*name_ptr == *word_ptr) {
            return dict_ptr + 1;
        }
        dict_ptr = (uint32_t*) *dict_ptr;
    }
    return 0;
}
