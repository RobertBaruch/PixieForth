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
 *  r11 is the parameter stack pointer. Appears in dumps as fp.
 *  r12 is the instruction pointer. Appears in dumps as ip.
 *    Note that during execution of an instruction, r12 points to
 *    the next instruction (as long as the current instruction is
 *    not EXIT).
 *  r8 is the saved stack pointer from C. TODO: change to r10.
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
 *   forth_name_<label>:
 *     .4byte <pointer to next definition>
 *     .4byte <pointer to forth_<label>>
 *     .byte <len of name>
 *     .ascii <name, with enough padding at end for alignment>
 *   forth_<label>:
 *     .4byte <address of forth_do_colon>
 *     .4byte <words in the definition>
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
 *   forth_name_<label>:
 *     .4byte <pointer to next definition>
 *     .4byte <pointer to forth_<label>>
 *     .byte <len of name>
 *     .ascii <name, with enough padding at end for alignment>
 *   forth_<label>:
 *     .4byte <address of forth_code_<label>>
 *   forth_code_<label>:
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
.macro __popreg2 reg1 reg2
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
.macro __loadvar name reg
    ldr \reg\(), =forth_var_\name
    ldr \reg\(), [\reg\()]
.endm

/*
 * Stores the given register into the given variable. This requires
 * a scratch register.
 */
.macro __storevar reg name scratchreg
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
// The initial value must be the last name in the builtins.
__defvar "LATEST",,latest,forth_name_testing // the addr of the last definition.
__defvar "HERE",,here // the addr of free data
__defvar "STATE",,state // the Forth state: 0 = interpreting, 1 = compiling.

/*
 * Accepts control from C.
 * r0 (first parameter) is the pointer to the parameter stack.
 * r1 (second parameter) is the address of the Forth routine to jump to,
 *     which is forth_<x>, not code_forth_<x>.
 * Because we don't trust the return stack or the parameter stack, we
 * store SP in r8 for later restoration, and are very careful NEVER to clobber r8!
 * TODO: change this to a struct so we can pass and return all sorts of things,
 *       like the HERE pointer.
 */
__new_func forth_enter
    push {r2-r12, lr}
    mov r8, sp
    // push {r1}
    mov r11, r0 // r11 <- parameter stack addr
    mov r12, r1 // r12 <- addr of next word
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
__defnative "LIT",,literal
    ldr r0, [r12], #4
    __pushreg r0
__end_defnative literal

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
    cbz r0, skip_maybe_dup
    __pushreg r0
skip_maybe_dup:
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
    cbz r0, skip_nez
    mvn r0, #0
    __putreg r0
skip_nez:
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
    // Want r0 = dest, r1 = src, r2 = len.
    __popreg2 r2, r0
    __popreg r1
    bl memcpy
__end_defnative memcpy

/* Block move with overlapping regions allowed. */
/* ( src-addr dest-addr len -- ) */
__defnative "MEMMOVE",,memmove
    // Want r0 = dest, r1 = src, r2 = len.
    __popreg2 r2, r0
    __popreg r1
    cmp r2, #0
    blt end_memmove
loop_memmove:
    cbz r2, end_memmove
    ldrb r3, [r1], #1
    strb r3, [r2], #1
    subs r2, #1
    b loop_memmove
end_memmove:
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

/* Waits for a byte from serial. */
/* ( -- k ) */
__defnative "KEY",,key
    __begin_c_call
await_key:
    bl usb_serial_getchar
    cmn r0, #1
    beq await_key
    __pushreg r0
    __end_c_call
__end_defnative key

/* Outputs a byte to serial. */
/* ( k -- ) */
__defnative "EMIT",,emit
    __begin_c_call
    __popreg r0
    bl usb_serial_putchar
    __end_c_call
__end_defnative emit

/*
 * Waits for a "word" from serial. A word starts with any non-whitespace
 * character except backslash, and continues to any whitespace character.
 * Characters between backslash and newline (inclusive) are ignored.
 * The maximum size of a word if F_LENMASK. We accept characters after that,
 * but ignore them.
 */
/* ( -- buff-addr len ) */
    .section .data
    .type forth_word_buffer, %object
    .align 2
    .global forth_word_buffer
forth_word_buffer:
    .space F_LENMASK
    .size forth_word_buffer, .-forth_word_buffer

__defnative "WORD",,word
    __begin_c_call
    ldr r4, =forth_word_buffer
    __pushreg r4
    movs r5, #0
await_word:
    bl usb_serial_getchar
    cmn r0, #1
    beq await_word
    cmp r0, ' '
    beq await_word
    cmp r0, '\t'
    beq await_word
    cmp r0, '\n'
    beq await_word
    cmp r0, '\r'
    beq await_word
start_word:
    cmp r0, '\\'
    beq await_end_comment_word
    cmp r5, F_LENMASK // We ignore anything that would overflow the buffer
    beq getchar_for_word
    strb r0, [r4], #1
    adds r5, #1
getchar_for_word:
    bl usb_serial_getchar
    cmn r0, #1
    beq getchar_for_word
    cmp r0, ' '
    beq end_word
    cmp r0, '\t'
    beq end_word
    cmp r0, '\n'
    beq end_word
    cmp r0, '\r'
    beq end_word
    b start_word
await_end_comment_word:
    bl usb_serial_getchar
    cmp r0, '\n'
    bne await_end_comment_word
    b getchar_for_word
end_word:
    __pushreg r5
    __end_c_call
__end_defnative word

/* ( buff-addr len -- number unconverted-char-count ) */
__defnative "NUMBER",,number
    ldmdb r11!, {r0, r1} // r0,r1 <- buff_addr,len
    movs r5, #0 // r5 = sign: 0 = positive, nz = negative
    movs r2, #0 // r2 = number
    __loadvar "BASE", r4 // r4 = base
    cbz r1, end_number
    ldrb r3, [r0]
    cmp r3, '-'  // first char '-'? skip over it and set flag
    bne is_hex_number
    movs r5, #1
    adds r0, #1
    subs r1, #1
    b next_number
is_hex_number:
    cmp r3, '$'  // first char '$'? skip over it and override base
    ittt eq
    moveq r4, #16
    addeq r0, #1
    subeq r1, #1
next_number:
    cbz r1, end_number
    ldrb r3, [r0], #1 // r3 = character to convert
    cmp r3, '0'
    blo end_number
    cmp r3, '9'
    bhi convert_az_number
    subs r3, '0'
    b check_number
convert_az_number:
    cmp r3, 'a'
    it hs
    subhs r3, #0x20 // converts lower case to upper case
    cmp r3, 'A'
    blo end_number
    subs r3, 'A' - 10
check_number:
    cmp r3, r4
    ittt lo
    mlalo r2, r2, r4, r3 // num <- num * base + digit
    sublo r1, #1
    blo next_number
end_number:
    cmp r5, #1
    it eq
    rsbeq r2, #0
    __pushreg2 r2, r1
__end_defnative number

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
    __loadvar "LATEST", r2 // r2 <- current definition address
check_current_find:
    cbz r2, end_find
    ldr r3, [r2] // r3 <- next definition address
    ldrb r4, [r2, #8] // r4 <- definition name length + flags
    ands r4, F_HIDDEN | F_LENMASK
    cmp r4, r1 // skip hidden entries and entries not matching the string length
    itt ne
    movne r2, r3
    bne check_current_find
    mov r5, r0 // r5 <- ptr to buffer
    adds r6, r2, #9 // r6 <- ptr to name
check_name_find:
    ldrb r7, [r5], #1
    ldrb r9, [r6], #1
    cmp r7, r9
    itt ne
    movne r2, r3
    bne check_current_find
    subs r4, #1
    cbz r4, end_find
    b check_name_find
end_find:
    __pushreg r2
__end_defnative find

/* ( defn-addr -- code-addr ) */
__defnative ">CFA",,to_code_field_addr
    __peekreg r0
    ldr r0, [r0, #4]
    __putreg r0
__end_defnative to_code_field_addr

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
copy_name_create:
    cbz r1, pad_name_create
    ldrb r5, [r0], #1
    strb r5, [r2], #1
    subs r1, #1
    b copy_name_create
pad_name_create:
    teq r4, #0
    ittt ne
    strbne r1, [r2], #1 // r1 was zero!
    subne r4, #1
    bne pad_name_create
    __loadvar "LATEST", r0 // r0 <- beginning of the header
    str r2, [r0, #4] // now we can write the code address
    __storevar r2, "HERE", r0 // and update HERE
__end_defnative create

/*
 * Stores a value into HERE, and increments HERE by 4. The
 * implication is that HERE has special semantics: it points to
 * available memory, but the content of that memory is undefined.
 * Thus, it makes sense that once we write to the memory at
 * HERE, we should increment HERE to point to the next available
 * location.
 */
/* ( x -- ) */
__defnative "HERE!",,store_to_here
    __popreg r0
    ldr r1, =forth_var_HERE
    ldr r2, [r1] // r2 <- HERE
    str r0, [r2], #4
    str r2, [r1] // update HERE
__end_defnative store_to_here

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
__defnative "TOGGLE_HIDDEN",,toggle_hidden
    __popreg r0
    ldrb r1, [r0, #8]!
    eors r1, F_HIDDEN
    strb r1, [r0]
__end_defnative toggle_hidden

__defnative "TOGGLE_IMMEDIATE",,toggle_immediate
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
    pop {r0}
    ldr r1, [r0], #4 // r1 <- offset, ptr += 4
    lsl r1, #2 // r1 *= 4
    adds r1, r0 // so offset zero just goes to next word
    push {r1}
__end_defnative branch

/*
 * If the parameter on the stack is zero, add the offset in the
 * next word to the instruction pointer of the caller, so that
 * when this word returns, the caller is executing there.
 * Otherwise, do nothing.
 */
/* ( x -- ) */
__defnative "0BRANCH",,brancheq
    __popreg r2
    pop {r0}
    ldr r1, [r0], #4 // r1 <- offset, ptr += 4
    teq r2, #0
    itt eq  // add offset only if parameter was zero
    lsleq r1, #2 // r1 *= 4
    addeq r1, r0 // so offset zero just goes to next word
    push {r1}
__end_defnative brancheq

/* Compile a definition. */
__defword ":",,compile
    __word word
    __word create
    __word literal
    __word do_colon
    __word store_to_here
    __word latest
    __word fetch
    __word toggle_hidden
    __word compile_mode
__end_defword compile

/* End compilation of a definition. */
__defword ";",F_IMMED,end_compile
    __word literal
    __word exit
    __word store_to_here
    __word latest
    __word fetch
    __word toggle_hidden
    __word immediate_mode
__end_defword end_compile

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

__defword "TESTING",,testing
    __word word
    __word number
    __word quit
__end_defword testing
