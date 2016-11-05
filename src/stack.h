/*
 * stack.h
 *
 *  Created on: Oct 30, 2016
 *      Author: Robert
 */

#ifndef SRC_STACK_H_
#define SRC_STACK_H_

extern uint32_t data_stack[1024];
extern uint32_t *sp;

inline void reset() {
    sp = data_stack;
}

inline void push(uint32_t *&sp, uint32_t val) {
    *sp++ = val;
}

inline uint32_t pop(uint32_t *&sp) {
    return *--sp;
}

#endif /* SRC_STACK_H_ */
