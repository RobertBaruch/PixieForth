/*
 * This is the core PixieForth system, which closely follows
 * the JonesForth tut by Richard Jones available at
 * http://git.annexia.org/?p=jonesforth.git;a=summary.
 */

.syntax unified

/* This macro writes the header of the function. */
.macro __new_func name
    .text
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
 *  r10 is the next word to execute. Define this better!
 *  r11 is the parameter stack pointer. Appears in dumps as fp.
 *  r12 is the instruction pointer. Appears in dumps as ip.
 *    Define this better!
 *    Note that during execution of an instruction, r12 points to
 *    the next instruction (as long as the current instruction is
 *    not EXIT).
 *  r8 is the saved stack pointer from C. TODO: Change to r9
 *  sp ("r13") is the return stack pointer.
 *  lr ("r14") is the address to return to in C
 *
 *  Note that C is under no obligation to save r12, so all calls to C
 *  must save and restore r12.
 *
 *  Forth may freely use r0-r7, r9, r10.
 *  Forth may clobber r0-r7, r9-r12, sp, lr. All are saved/restored except r0 and r1.
 */

.set F_IMMED,0x80
.set F_UNUSED,0x40
.set F_HIDDEN,0x20
.set F_LENMASK,0x1f // length mask

// Stores the chain of links.
.set link, 0

/*
 * The format of a word definition is:
 *   forth_name_<label>: (the definition address)
 *     .4byte <pointer to next definition>
 *     .4byte <pointer to forth_<label>>
 *     .byte <len of name>
 *     .ascii <name, with enough padding at end for alignment>
 *   forth_<label>: (the code field address)
 *     .4byte <code field address of do_colon = forth_do_colon>
 *                  (the data field address)
 *     .4byte <code field addresses of words in the definition>
 *     ... <more words>
 *     .4byte forth_exit
 */
.macro __defword name, flags=0, label
    .section .data
    .type forth_name_\label\(), %object
    .align 2
    .global forth_name_\label
forth_name_\label\():
    .4byte link
    .4byte forth_\label\()
    .set link, forth_name_\label
    .byte forth_end_name_\label\()-.-1+\flags
    .ascii "\name"
forth_end_name_\label\():
    .align 2, 0
    .size forth_name_\label\(), .-forth_name_\label\()
    .global forth_\label
    .type forth_\label\(), %object
forth_\label\():
    .4byte forth_do_colon
    // list of word pointers go here, use __word macro.
.endm

/* This macro writes the footer of a word definition. */
.macro __end_defword label
    .4byte forth_exit
    .size forth_\label\(), .-forth_\label\()
.endm

/* This macro writes a word pointer. */
.macro __word label
    .4byte forth_\label
.endm

/*
 * The format of a native definition is:
 *   forth_name_<label>: (the definition address)
 *     .4byte <pointer to next definition>
 *     .4byte <pointer to forth_<label>>
 *     .byte <len of name>
 *     .ascii <name, with enough padding at end for alignment>
 *   forth_<label>: (the code field address)
 *     .4byte <address of forth_code_<label>>
 *   forth_code_<label>: (the data field address)
 *     <native code>
 *     __next
 */
.macro __defnative name, flags=0, label
    .section .data
    .type forth_name_\label\(), %object
    .align 2
    .global forth_name_\label
forth_name_\label\():
    .4byte link
    .4byte forth_\label\()
    .set link, forth_name_\label
    .byte forth_end_name_\label\()-.-1+\flags
    .ascii "\name"
forth_end_name_\label\():
    .align 2, 0
    .size forth_name_\label\(), .-forth_name_\label\()
    .global forth_\label
    .type forth_\label\(), %object
forth_\label\():
    .4byte forth_code_\label
    .size forth_\label\(), 4

    .text
    .global forth_code_\label
    .thumb_func
    .type forth_code_\label\(), %function
forth_code_\label\():
.endm

/* This macro writes the footer of a native definition. */
.macro __end_defnative label
    __next
    .pool
    .size forth_code_\label\(), .-forth_code_\label\()
.endm

/*
 * This macro creates a native definition for a global variable.
 */
.macro __defvar name, flags=0, label, initial=0
    __defnative \name\(),\flags\(),\label\()
    ldr r0, =forth_var_\name\()
    ldr r0, [r0]
    __pushreg r0
    __end_defnative \label

    .section .data
    .type forth_var_\name\(), %object
    .align 2
    .global forth_var_\name
forth_var_\name\():
    .4byte \initial
    .size forth_var_\name\(), .-forth_var_\name\()
.endm

/*
 * Some useful macros for writing native definitions.
 */

/* Pushes the given register onto the parameter stack. */
.macro __pushreg reg
     str \reg\(), [r11], #4
.endm

/* Pushes reg1 then reg2 onto the parameter stack. */
.macro __pushreg2 reg1, reg2
    strd \reg1\(), \reg2\(), [r11], #8
.endm

/* Pops the parameter stack into the given register. */
.macro __popreg reg
     ldr \reg\(), [r11, #-4]!
.endm

/* Pops reg1 then reg2 from the parameter stack. */
.macro __popreg2 reg1, reg2
     ldrd \reg2\(), \reg1\(), [r11, #-8]!
.endm

/* Loads the given register with the top of the parameter stack. */
.macro __peekreg reg
    ldr \reg\(), [r11, #-4]
.endm

/* Stores the given register to the top of the parameter stack. */
.macro __putreg reg
    str \reg\(), [r11, #-4]
.endm

/* Loads the given variable into the given register. */
.macro __loadvar name, reg
    ldr \reg\(), =forth_var_\name
    ldr \reg\(), [\reg\()]
.endm

/*
 * Stores the given register into the given variable. This requires
 * a scratch register.
 */
.macro __storevar reg, name, scratchreg
    ldr \scratchreg\(), =forth_var_\name
    str \reg\(), [\scratchreg\()]
.endm

/*
 * Saves r12 before calling into C.
 * The ARM ABI requires that subroutines save r4-r11 if
 * they use them. If we are calling into C, then we are responsible
 * for saving any other registers we use.
 */
.macro __begin_c_call
    push {r12}
.endm

/* Restores r12 after returning from C. */
.macro __end_c_call
    pop {r12}
.endm

 /*
  * This macro implements the NEXT fragment. All native
  * routines must end in NEXT. The beginning of all words
  * is a pointer to the routine to start that word. Sometimes
  * it is just a pointer to native assembly, if the word was
  * native. Otherwise, it is a pointer to standard
  * "interpret" routine.
  */
.macro __next
    ldr r10, [r12], #4 // r10 <- word to execute, next_word_ptr++
    ldr r9, [r10]  // r9 <- code for word to execute
    bx r9
.endm

__defvar "BASE",,base,10 // current base for interpreting text numbers
__defvar "HERE",,here,_sheap // the addr of free data
__defvar "STATE",,state,0 // the Forth state: 0 = interpreting, 1 = compiling.
__defvar "STDIN",,stdin,0 // the source of input: 0 = usb serial, otherwise address in memory
__defvar "STDIN_COUNT",,stdin_count,0 // bytes remaining in stdin, for memory stdin.

/*
 * Accepts control from C.
 * r0 (first parameter) is the pointer to the parameter stack.
 * r1 (second parameter) is the address of the Forth routine to jump to,
 *     which is forth_<x>, not forth_code_<x> or forth_name_<x>.
 * Because we don't trust the return stack or the parameter stack, we
 * store SP in r8 for later restoration, and are very careful NEVER to clobber r8!
 * TODO: change this to a struct so we can pass and return all sorts of things,
 *       like the HERE pointer.
 */
__new_func forth_enter
    push {r2-r12, lr}
    mov r8, sp
    mov r11, r0 // r11 <- parameter stack addr
    ldr r0, =forth_quit
    push {r0}
    push {r1}
    // Because machine stack grows backwards in memory, this
    // is laid out in memory as follows:
    // <addr of program> <forth_quit>
    //       SP^             SP+4^
    mov r12, sp
    __next
__end_func forth_enter

/* Returns control to C, returning r11 so we know where we stopped. */
__defnative "QUIT",,quit
    mov r0, r11
    mov sp, r8
    pop {r2-r12, lr}
    bx lr
__end_defnative quit

/*
 * This is the "interpret" routine for non-native words.
 */
__new_func forth_do_colon
    push {r12} // r12 is the instruction coming next in the caller
    adds r12, r10, #4 // point to next instruction
    __next
__end_func forth_do_colon

/* Returns control to the Forth caller. */
__defnative "EXIT",,exit
    pop {r12}
__end_defnative exit

/* ( -- x ) */
__defnative "LIT",,lit
    ldr r0, [r12], #4
    __pushreg r0
__end_defnative lit

/* ( x -- ) */
__defnative "DROP",,drop
    subs r11, #4
__end_defnative drop

/* ( x y -- ) */
__defnative "2DROP",,2drop
    subs r11, #8
__end_defnative 2drop

/* ( x y -- y x ) */
__defnative "SWAP",,swap
    ldmdb r11!, {r0, r1} // r0,r1 <- x,y
    __pushreg2 r1, r0
__end_defnative swap

/* ( a b c d -- c d a b ) */
__defnative "2SWAP",,2swap
    ldmdb r11!, {r0, r1, r2, r3} // r0,r1,r2,r3 <- a,b,c,d
    __pushreg2 r2, r3
    __pushreg2 r0, r1
__end_defnative 2swap

/* ( x -- x x ) */
__defnative "DUP",,dup
    __peekreg r0
    __pushreg r0
__end_defnative dup

/* ( x y -- x y x y ) */
__defnative "2DUP",,2dup
    ldmdb r11, {r0, r1} // r0,r1 <- x,y
    __pushreg2 r0, r1
__end_defnative 2dup

/* ( x -- 0 | x x ) */
__defnative "?DUP",,maybe_dup
    __peekreg r0
    cbz r0, .L_skip_maybe_dup
    __pushreg r0
.L_skip_maybe_dup:
__end_defnative maybe_dup

/* ( x y -- x y x ) */
__defnative "OVER",,over
    ldr r0, [r11, #-8]
    __pushreg r0
__end_defnative over

/* ( x y z -- y z x ) */
__defnative "ROT",,rot
    ldmdb r11!, {r0, r1, r2} // r0,r1,r2 <- x,y,z
    __pushreg2 r1, r2
    __pushreg r0
__end_defnative rot

/* ( x y z -- z x y ) */
__defnative "-ROT",,nrot
    ldmdb r11!, {r0, r1, r2} // r0,r1,r2 <- x,y,z
    __pushreg2 r2, r0
    __pushreg r1
__end_defnative nrot

/* ( x -- x+1 ) */
__defnative "1+",,inc
    __peekreg r0
    adds r0, #1
    __putreg r0
__end_defnative inc

/* ( x -- x-1 ) */
__defnative "1-",,dec
    __peekreg r0
    subs r0, #1
    __putreg r0
__end_defnative dec

/* ( x -- x+4 ) */
__defnative "4+",,inc4
    __peekreg r0
    adds r0, #4
    __putreg r0
__end_defnative inc4

/* ( x -- x-4 ) */
__defnative "4-",,dec4
    __peekreg r0
    subs r0, #4
    __putreg r0
__end_defnative dec4

/* ( x y -- x+y ) */
__defnative "+",,add
    ldmdb r11!, {r0, r1} // r0,r1 <- x,y
    adds r1, r0
    __pushreg r1
__end_defnative add

/* ( x y -- x-y ) */
__defnative "-",,sub
    ldmdb r11!, {r0, r1} // r0,r1 <- x,y
    sub r0, r1
    __pushreg r0
__end_defnative sub

/* ( x y -- x*y ) */
__defnative "*",,mul
    ldmdb r11!, {r0, r1} // r0,r1 <- x,y
    mul r1, r0
    __pushreg r1
__end_defnative mul

/* ( x y -- x%y x/y ) */
__defnative "/MOD",,divmod
    ldmdb r11!, {r0, r1} // r0,r1 <- x,y
    sdiv r2, r0, r1  // q <- x/y
    mls r1, r2, r1, r0 // r, q, y, x: r <- x - q*y
    __pushreg2 r1, r2
__end_defnative divmod

/* Signed division. */
/* ( x y -- x/y ) */
__defnative "/",,div
    ldmdb r11!, {r0, r1} // r0,r1 <- x,y
    sdiv r0, r1
    __pushreg r0
__end_defnative div

/* ( x y -- 0 | 0xffffffff ) */
__defnative "=",,eq
    ldmdb r11!, {r0, r1} // r0,r1 <- x,y
    cmp r0, r1
    ite eq
    mvneq r0, #0
    movne r0, #0
    __pushreg r0
__end_defnative eq

/* ( x y -- 0 | 0xffffffff ) */
__defnative "<>",,ne
    ldmdb r11!, {r0, r1} // r0,r1 <- x,y
    cmp r0, r1
    ite ne
    mvnne r0, #0
    moveq r0, #0
    __pushreg r0
__end_defnative ne

/* Signed comparison, x < y */
/* ( x y -- 0 | 0xffffffff ) */
__defnative "<",,lt
    ldmdb r11!, {r0, r1} // r0,r1 <- x,y
    cmp r0, r1
    ite lt
    mvnlt r0, #0
    movge r0, #0
    __pushreg r0
__end_defnative lt

/* Signed comparison, x > y */
/* ( x y -- 0 | 0xffffffff ) */
__defnative ">",,gt
    ldmdb r11!, {r0, r1} // r0,r1 <- x,y
    cmp r0, r1
    ite gt
    mvngt r0, #0
    movle r0, #0
    __pushreg r0
__end_defnative gt

/* Signed comparison, x <= y */
/* ( x y -- 0 | 0xffffffff ) */
__defnative "<=",,le
    ldmdb r11!, {r0, r1} // r0,r1 <- x,y
    cmp r0, r1
    ite le
    mvnle r0, #0
    movgt r0, #0
    __pushreg r0
__end_defnative le

/* Signed comparison, x >= y */
/* ( x y -- 0 | 0xffffffff ) */
__defnative ">=",,ge
    ldmdb r11!, {r0, r1} // r0,r1 <- x,y
    cmp r0, r1
    ite ge
    mvnge r0, #0
    movlt r0, #0
    __pushreg r0
__end_defnative ge

/* ( x -- 0 | 0xffffffff ) */
__defnative "0=",,eqz
    __peekreg r0
    cmp r0, #0
    ite eq
    mvneq r0, #0
    movne r0, #0
    __putreg r0
__end_defnative eqz

/* ( x -- 0 | 0xffffffff ) */
__defnative "0<>",,nez
    __peekreg r0
    cbz r0, .L_skip_nez
    mvn r0, #0
    __putreg r0
.L_skip_nez:
__end_defnative nez

/* Signed comparison, x < 0 */
/* ( x -- 0 | 0xffffffff ) */
__defnative "0<",,ltz
    __peekreg r0
    cmp r0, #0
    ite lt
    mvnlt r0, #0
    movge r0, #0
    __putreg r0
__end_defnative ltz

/* Signed comparison, x > 0 */
/* ( x -- 0 | 0xffffffff ) */
__defnative "0>",,gtz
    __peekreg r0
    cmp r0, #0
    ite gt
    mvngt r0, #0
    movle r0, #0
    __putreg r0
__end_defnative gtz

/* Signed comparison, x <= 0 */
/* ( x -- 0 | 0xffffffff ) */
__defnative "0<=",,lez
    __peekreg r0
    cmp r0, #0
    ite le
    mvnle r0, #0
    movgt r0, #0
    __putreg r0
__end_defnative lez

/* Signed comparison, x >= 0 */
/* ( x -- 0 | 0xffffffff ) */
__defnative "0>=",,gez
    __peekreg r0
    cmp r0, #0
    ite ge
    mvnge r0, #0
    movlt r0, #0
    __putreg r0
__end_defnative gez

/* ( x y -- x&y ) */
__defnative "AND",,and
    ldmdb r11!, {r0, r1} // r0,r1 <- x,y
    ands r1, r0
    __pushreg r1
__end_defnative and

/* ( x y -- x|y ) */
__defnative "OR",,or
    ldmdb r11!, {r0, r1} // r0,r1 <- x,y
    orrs r1, r0
    __pushreg r1
__end_defnative or

/* ( x y -- x^y ) */
__defnative "XOR",,xor
    ldmdb r11!, {r0, r1} // r0,r1 <- x,y
    eors r1, r0
    __pushreg r1
__end_defnative xor

/* ( x -- ~x ) */
__defnative "INVERT",,not
    __peekreg r0
    mvn r0, r0
    __putreg r0
__end_defnative not

/* ( x addr -- ) */
__defnative "!",,store
    ldmdb r11!, {r0, r1} // r0,r1 <- x,addr
    str r0, [r1]
__end_defnative store

/* ( x addr -- ) */
__defnative "C!",,store_char
    ldmdb r11!, {r0, r1} // r0,r1 <- x,addr
    strb r0, [r1]
__end_defnative store_char

/* ( addr -- x ) */
__defnative "\@",,fetch
    __peekreg r0
    ldr r0, [r0]
    __putreg r0
__end_defnative fetch

/* ( addr -- x ) */
__defnative "C\@",,fetch_char
    __peekreg r0
    ldrb r0, [r0]
    __putreg r0
__end_defnative fetch_char

/* ( x addr -- ) */
__defnative "+!",,addstore
    ldmdb r11!, {r0, r1} // r0,r1 <- x,addr
    ldr r2, [r1]
    adds r2, r0
    str r2, [r1]
__end_defnative addstore

/* ( x addr -- ) */
__defnative "-!",,substore
    ldmdb r11!, {r0, r1} // r0,r1 <- x,addr
    ldr r2, [r1]
    subs r2, r0
    str r2, [r1]
__end_defnative substore

/* Block move only for non-overlapping regions. If the blocks overlap, the result is undefined. */
/* ( src-addr dest-addr len -- ) */
/* Which is backwards from memcpy, which is dest, src, len. */
__defnative "MEMCOPY",,memcpy
    __begin_c_call
    // Want r0 = dest, r1 = src, r2 = len.
    __popreg2 r2, r0
    __popreg r1
    bl memcpy
    __end_c_call
__end_defnative memcpy

/*
 * Block move with overlapping regions allowed. Because memcpy
 * does a forward copy, it is okay to use it if and only if
 * dest >= src + len or dest <= src.
 */
// TODO: Make this more time-efficient
/* ( src-addr dest-addr len -- ) */
__defnative "MEMMOVE",,memmove
    __begin_c_call
    // Want r0 = dest, r1 = src, r2 = len.
    __popreg2 r2, r0
    __popreg r1
    cmp r0, r1
    ble .L_loop_fwd_memmove // fwd if dest <= src
    adds r3, r1, r2 // r3 <- src + len
    cmp r0, r3
    bge .L_loop_fwd_memmove  // fwd if dest >= src + len

    adds r0, r2
    adds r1, r2
.L_loop_bwd_memmove:
    cbz r2, .L_end_memmove
    ldrb r3, [r1, #-1]!
    strb r3, [r0, #-1]!
    subs r2, #1
    b .L_loop_bwd_memmove

.L_loop_fwd_memmove:
    bl memcpy
.L_end_memmove:
    __end_c_call
__end_defnative memmove

/* Pop param stack and push onto return stack */
/* ( addr -- ) */
__defnative ">R",,param_to_return
    __popreg r0
    push {r0}
__end_defnative param_to_return

/* Pop return stack and push onto param stack */
/* ( -- addr ) */
__defnative "R>",,return_to_param
    pop {r0}
    __pushreg r0
__end_defnative return_to_param

/* Fetch top of return stack, push onto param stack */
/* ( -- addr ) */
__defnative "R\@",,fetch_return
    ldr r0, [sp]
    __pushreg r0
__end_defnative fetch_return

/* Replace top of return stack with popped value from param stack */
/* ( addr -- ) */
__defnative "R!",,store_return
    __popreg r0
    str r0, [sp]
__end_defnative store_return

/* Waits for a byte from serial or a memory buffer. -1 is EOF. */
/* ( -- k ) */
__defnative "KEY",,key
    bl _forth_key
    __pushreg r0
__end_defnative key

/*
 * Subroutine version of KEY.
 * Input: --
 * Output: r0 = key or -1 on EOF.
 * Note: r12 and lr must be saved in case we call into C.
 */
__new_func _forth_key
    push {r1, r2, r3, r12, lr}
    __loadvar "STDIN", r1
    cbz r1, .L_await_serial_key

    __loadvar "STDIN_COUNT", r2
    cmp r2, #0
    itt eq
    mvneq r0, #0
    beq .L_return

    ldrb r0, [r1], #1
    subs r2, #1
    __storevar r1, "STDIN", r3
    __storevar r2, "STDIN_COUNT", r1
    b .L_return

.L_await_serial_key:
    bl usb_serial_getchar
    cmn r0, #1
    beq .L_await_serial_key

.L_return:
    pop {r1, r2, r3, r12, lr}
    bx lr
__end_func _forth_key

/* Outputs a byte to serial. */
/* ( k -- ) */
__defnative "EMIT",,emit
    __popreg r0
    bl _forth_emit
__end_defnative emit

/*
 * Subroutine version of EMIT.
 * Input: r0 = char
 * Output: --
 */
__new_func _forth_emit
    push {r12, lr}
    bl usb_serial_putchar
    pop {r12, lr}
    bx lr
__end_func _forth_emit

/*
 * Waits for a "word" from serial. A word starts with any non-whitespace
 * character except backslash, and continues to any whitespace character.
 * While waiting for a word, characters between backslash and newline
 * (inclusive) are treated as whitespace.
 * The maximum size of a word if F_LENMASK. We accept characters after that,
 * but ignore them.
 */
    .section .data
    .type forth_word_buffer, %object
    .align 2
    .global forth_word_buffer
forth_word_buffer:
    .space F_LENMASK
    .size forth_word_buffer, .-forth_word_buffer

/* ( -- buff-addr len ) */
__defnative "WORD",,word
    bl _forth_word
    __pushreg2 r0, r1
__end_defnative word

/*
 * Subroutine version of WORD.
 * Input: --
 * Output: r10 = buff_addr, r1 = len
 */
__new_func _forth_word
    push {r4, lr}
    ldr r4, =forth_word_buffer
    movs r1, #0

.L_await_word:
    bl _forth_key
    cmn r0, #1
    itt eq
    moveq r1, #0
    beq .L_end_word
    cmp r0, ' '
    beq .L_await_word
    cmp r0, '\t'
    beq .L_await_word
    cmp r0, '\n'
    beq .L_await_word
    cmp r0, '\r'
    beq .L_await_word
    cmp r0, '\\'
    beq .L_skip_comment

.L_start_word:
    cmp r1, F_LENMASK // We ignore anything that would overflow the buffer
    beq .L_getchar_for_word
    strb r0, [r4, r1]
    adds r1, #1

.L_getchar_for_word:
    bl _forth_key
    cmn r0, #1
    beq .L_end_word
    cmp r0, ' '
    beq .L_end_word
    cmp r0, '\t'
    beq .L_end_word
    cmp r0, '\n'
    beq .L_end_word
    cmp r0, '\r'
    beq .L_end_word
    b .L_start_word

.L_skip_comment:
    bl _forth_key
    cmp r0, '\n'
    bne .L_skip_comment
    b .L_getchar_for_word

.L_end_word:
    mov r0, r4
    pop {r4, lr}
    bx lr
__end_func _forth_word

/* ( buff-addr len -- number unconverted-char-count ) */
__defnative "NUMBER",,number
    ldmdb r11!, {r0, r1} // r0,r1 <- buff_addr,len
    bl _forth_number
    __pushreg2 r0, r1
__end_defnative number

/*
 * Subroutine version of NUMBER.
 * Input: r0 = buff_addr, r1 = len.
 * Output: r0 = number, r1 = unconverted char count.
 */
__new_func _forth_number
    push {r2, r3, r4, r5}
    movs r5, #0 // r5 = sign: 0 = positive, nz = negative
    movs r2, #0 // r2 = number
    __loadvar "BASE", r4 // r4 = base
    cbz r1, .L_end_number
    ldrb r3, [r0]
    cmp r3, '-'  // first char '-'? skip over it and set flag
    bne .L_is_hex_number
    movs r5, #1
    adds r0, #1
    subs r1, #1
    b .L_next_number

.L_is_hex_number:
    cmp r3, '$'  // first char '$'? skip over it and override base
    ittt eq
    moveq r4, #16
    addeq r0, #1
    subeq r1, #1

.L_next_number:
    cbz r1, .L_end_number
    ldrb r3, [r0], #1 // r3 = character to convert
    cmp r3, '0'
    blo .L_end_number
    cmp r3, '9'
    bhi .L_convert_az_number
    subs r3, '0'
    b .L_check_number

.L_convert_az_number:
    cmp r3, 'a'
    it hs
    subhs r3, #0x20 // converts lower case to upper case
    cmp r3, 'A'
    blo .L_end_number
    subs r3, 'A' - 10

.L_check_number:
    cmp r3, r4
    ittt lo
    mlalo r2, r2, r4, r3 // num <- num * base + digit
    sublo r1, #1
    blo .L_next_number

.L_end_number:
    cmp r5, #1
    it eq
    rsbeq r2, #0
    mov r0, r2
    pop {r2, r3, r4, r5}
    bx lr
__end_func _forth_number

/*
 * Finds the given definition in the dictionary by word name,
 * returning its address (forth_name_<label>), or 0 if not found.
 * Note that if len > F_LENMASK, the results of this function are
 * undefined.
 *
 * The format of a definition is:
 *   forth_name_<label>:
 *     .4byte <pointer to next definition>
 *     .4byte <pointer to forth_<label>>
 *     .byte len+flags
 *     .ascii <name, with enough padding at end for alignment>
 *   forth_<label>:
 */
/* ( buff-addr len -- 0 | defn-addr ) */
__defnative "FIND",,find
    ldmdb r11!, {r0, r1} // r0,r1 <- buff-addr,len
    bl _forth_find
    __pushreg r0
__end_defnative find

/*
 * Subroutine version of FIND.
 * Input: r0 = buff_addr, r1 = len
 * Output: r0 = defn_addr or 0 if not found
 */
__new_func _forth_find
    push {r2, r4, r5, r6, r7, r9}
    __loadvar "LATEST", r2 // r2 <- current definition address

.L_check_current_find:
    cbz r2, .L_end_find
    ldrb r4, [r2, #8] // r4 <- definition name length + flags
    ands r4, F_HIDDEN | F_LENMASK
    cmp r4, r1 // skip hidden entries and entries not matching the string length
    itt ne
    ldrne r2, [r2]
    bne .L_check_current_find
    mov r5, r0 // r5 <- ptr to buffer
    adds r6, r2, #9 // r6 <- ptr to name

.L_check_name_find:
    ldrb r7, [r5], #1
    ldrb r9, [r6], #1
    cmp r7, r9
    itt ne
    ldrne r2, [r2]
    bne .L_check_current_find
    subs r4, #1
    cbz r4, .L_end_find
    b .L_check_name_find
.L_end_find:
    mov r0, r2
    pop {r2, r4, r5, r6, r7, r9}
    bx lr
__end_func _forth_find

/* ( defn-addr -- code-addr ) */
__defnative ">CFA",,to_code_field_addr
    __peekreg r0
    bl _forth_to_code_field_addr
    __putreg r0
__end_defnative to_code_field_addr

/*
 * Subroutine version of >CFA.
 * Input: r0 = defn_addr
 * Output: r0 = code_addr
 */
__new_func _forth_to_code_field_addr
    ldr r0, [r0, #4]
    bx lr
__end_func _forth_to_code_field_addr

/*
 * Returns the data field address for a definition. For a
 * native definition, this is the beginning of the native
 * routine. For a non-native definition, this is the word
 * immediately after the do_colon word.
 */
/* ( defn-addr -- data-addr ) */
__defnative ">DFA",,to_data_field_addr
    __peekreg r0
    ldr r0, [r0, #4]
    adds r0, #4
    __putreg r0
__end_defnative to_data_field_addr

/*
 * Creates the header for a definition and places it at
 * HERE, updating HERE and LATEST. The header is the link
 * to the next definition, and the name (and name length).
 * the flags are always zero.
 */
/* ( buff-addr len -- ) */
__defnative "CREATE",,create
    ldmdb r11!, {r0, r1} // r0,r1 <- buff-addr,len
    __loadvar "HERE", r2 // r2 <- HERE
    __loadvar "LATEST", r3 // r3 <- LATEST
    __storevar r2, "LATEST", r4 // update LATEST to HERE
    str r3, [r2], #8 // Write link. Skip over the pointer to code for now
    strb r1, [r2], #1 // writes the len byte
    ands r4, r1, #3 // r4 = amount of padding required at end.
    // Because of the len byte, the amount of padding to get to
    // 4-byte alignment based on the last 2 bits of the length are:
    // 0: 3 bytes
    // 1: 2 bytes
    // 2: 1 byte
    // 3: 0 bytes
    rsb r4, #3 // so r4 = 3-(len&3)
.L_copy_name_create:
    cbz r1, .L_pad_name_create
    ldrb r5, [r0], #1
    strb r5, [r2], #1
    subs r1, #1
    b .L_copy_name_create
.L_pad_name_create:
    teq r4, #0
    ittt ne
    strbne r1, [r2], #1 // r1 was zero!
    subne r4, #1
    bne .L_pad_name_create
    __loadvar "LATEST", r0 // r0 <- beginning of the header
    str r2, [r0, #4] // now we can write the code address
    __storevar r2, "HERE", r0 // and update HERE
__end_defnative create

/*
 * Stores a value into HERE, and increments HERE by 4. The
 * implication is that HERE has special semantics: by definition
 * it points to available memory. Thus, it makes sense that once we
 * write to the memory at HERE, we should increment HERE to point to
 * the next available location.
 */
/* ( x -- ) */
__defnative ",",,store_to_here
    __popreg r0
    bl _forth_store_to_here
__end_defnative store_to_here

/*
 * Subroutine version of ,
 * Input: r0 = byte to store
 * Output: --
 */
__new_func _forth_store_to_here
    push {r1, r2}
    ldr r1, =forth_var_HERE
    ldr r2, [r1] // r2 <- HERE
    str r0, [r2], #4
    str r2, [r1] // update HERE
    pop {r1, r2}
    bx lr
__end_func _forth_store_to_here

/* Switch to immediate mode, immediately. */
__defnative "[",F_IMMED,immediate_mode
    movs r0, #0
    __storevar r0, "STATE", r1
__end_defnative immediate_mode

/* Switch to compile mode. */
__defnative "]",,compile_mode
    movs r0, #1
    __storevar r0, "STATE", r1
__end_defnative compile_mode

/* ( defn-addr -- ) */
__defnative "HIDDEN",,toggle_hidden
    __popreg r0
    ldrb r1, [r0, #8]!
    eors r1, F_HIDDEN
    strb r1, [r0]
__end_defnative toggle_hidden

/* Toggles F_IMMED on LATEST word. */
__defnative "IMMEDIATE",,toggle_immediate
    __loadvar "LATEST", r0
    ldrb r1, [r0, #8]!
    eors r1, F_IMMED
    strb r1, [r0]
__end_defnative toggle_immediate

/*
 * Add the offset in the next word to the instruction pointer
 * of the caller, so that when this word returns, the caller
 * is executing there.
 */
__defnative "BRANCH",,branch
    ldr r0, [r12], #4 // r1 <- offset, ptr += 4
    lsl r0, #2 // r1 *= 4
    adds r12, r0 // so offset zero just goes to next word
__end_defnative branch

/*
 * If the parameter on the stack is zero, add the offset in the
 * next word to the instruction pointer of the caller, so that
 * when this word returns, the caller is executing there.
 * Otherwise, do nothing.
 */
/* ( x -- ) */
__defnative "0BRANCH",,brancheq
    __popreg r1
    ldr r0, [r12], #4 // r1 <- offset, ptr += 4
    teq r1, #0
    itt eq  // add offset only if parameter was zero
    lsleq r0, #2 // r1 *= 4
    addeq r12, r0 // so offset zero just goes to next word
__end_defnative brancheq

/* ( -- c ) */
__defnative "CHAR",,char
    bl _forth_word // r0, r1 <- buff_addr, len
    ldrb r0, [r0]
    __pushreg r0
__end_defnative char

/* ( -- 10 ) */
__defnative "'\\n'",,char_newline
    mov r0, #10
    __pushreg r0
__end_defnative char_newline

__defnative "NL",,emit_newline
    mov r0, #10
    bl _forth_emit
__end_defnative emit_newline

/* ( -- 32 ) */
__defnative "BL",,char_space
    mov r0, ' '
    __pushreg r0
__end_defnative char_space

__defnative "SPACE",,emit_space
    mov r0, ' '
    bl _forth_emit
__end_defnative emit_space

/* Compile a definition. */
__defword ":",,compile_def
    __word word
    __word create
    __word lit
    __word do_colon
    __word store_to_here
    __word latest
    __word toggle_hidden
    __word compile_mode
__end_defword compile_def

/* End compilation of a definition. */
__defword ";",F_IMMED,end_compile_def
    __word lit
    __word exit
    __word store_to_here
    __word latest
    __word toggle_hidden
    __word immediate_mode
__end_defword end_compile_def

/* Toggle hidden on the next word. */
__defword "HIDE",,hide
    __word word
    __word find
    __word toggle_hidden
__end_defword hide

/* Gets the code field address of the next word. */
/* ( -- code-addr ) */
__defword "'",,code_field_addr_of_next_word
    __word word
    __word find
    __word to_code_field_addr
__end_defword code_field_addr_of_next_word

/* Compile LIT <x>. */
/* ( x -- ) */
__defword "LITERAL",F_IMMED,literal
    __word lit
    __word lit
    __word store_to_here
    __word store_to_here
__end_defword literal

/*
 * Get a word, find it in the dictionary, get its code address,
 * and either execute it (if it's immediate or we are in immediate
 * mode) or append it to user memory (i.e. we're compiling).
 *
 * If we didn't find a word, try to interpret it as a number. If
 * successful, push it on the stack (in immediate mode) or append
 * LIT and the number to user memory (i.e. we're comiling).
 *
 * If we couldn't interpret as a number, error out.
 */
__defnative "INTERPRET",,interpret
    // Get word, find in dictionary
    bl _forth_word // r0, r1 <- buff_addr, len
    mov r2, r0 // r2 <- buff_addr
    bl _forth_find // r0 <- 0 | defn-addr
    cbz r0, .L_number

    // Found in dictionary. See if it's an immediate word, or if
    // we're in immediate mode. If so, execute it immediately.
    ldrb r2, [r0, #8] // r2 <- len + flags
    tst r2, F_IMMED
    bne .L_execute_word
    __loadvar "STATE", r2 // r2 <- state
    cbz r2, .L_execute_word
    // Add to currently compiling definition
    ldr r0, [r0, #4]
    bl _forth_store_to_here
    __next

.L_execute_word:
    bl _forth_to_code_field_addr // r0 <- code-addr
    mov r10, r0
    ldr r0, [r0]
    bx r0 // execute the word

.L_number:
    mov r0, r2 // r0 <- buff_addr (r1 still contains len)
    mov r3, r1 // r3 <- len, in case we need to print the buffer later
    bl _forth_number // r0, r1 <- number, unconverted-char-count
    cbnz r1, .L_error

    __loadvar "STATE", r1 // r1 <- state
    cbnz r1, .L_compile_number
    __pushreg r0
    __next // The number is now on the stack for immediate mode.

.L_compile_number:
    // Append LIT and then the number
    mov r1, r0 // r1 <- number
    ldr r0, =forth_lit
    bl _forth_store_to_here
    mov r0, r1
    bl _forth_store_to_here
    __next

.L_error:
    __pushreg2 r0, r1
__end_defnative interpret

/*
 * The initial value of LATEST must be the last name in the builtins. All
 * new builtins, therefore, must be defined before this one.
 */
__defvar "LATEST",,latest,forth_name_latest // the addr of the last definition.
