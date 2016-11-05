.syntax unified

/* This macro writes the header of the function. */
.macro __new_func name
    .global \name
    .thumb_func
    .align 2
    .type \name\(), %function
\name\():
.endm

/* This macro writes the footer of the function. */
.macro __end_func name
    .pool
    .size \name\(), .-\name\()
.endm

/*
 * The ABI for words:
 *  r0 must contain the stack pointer.
 *  lr must contain the address to return to.
 */

/* Takes the word to push in r1. */
/* Not a forth word ( -- x) */
__new_func special_word_push
    str r1, [r0], #4
    bx lr
__end_func special_word_push

/* ! (x addr -- ) */
__new_func forth_word_store
    ldmdb r0!, {r1, r2} // r2 <- x, r1 <- addr
    str r2, [r1]
    bx lr
__end_func forth_word_store

/* @ (addr -- x) */
__new_func forth_word_load
    ldr r1, [r0, #-4]!
    ldr r2, [r1]
    str r2, [r0], #4
    bx lr
__end_func forth_word_load

/* POP (x -- ) */
__new_func forth_word_pop
    sub r0, #4
    bx lr
__end_func forth_word_pop

/* + (n1 n2 -- n3) */
__new_func forth_word_add
    ldmdb r0!, {r1, r2} // r2 <- n1, r1 <- n2
    add r1, r2
    str r1, [r0], #4
    bx lr
__end_func forth_word_add
