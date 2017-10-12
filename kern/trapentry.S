/* See COPYRIGHT for copyright information. */

#include <inc/mmu.h>
#include <inc/memlayout.h>
#include <inc/trap.h>

#include <kern/picirq.h>


###################################################################
# exceptions/interrupts
###################################################################

/* TRAPHANDLER defines a globally-visible function for handling a trap.
 * It pushes a trap number onto the stack, then jumps to _alltraps.
 * Use TRAPHANDLER for traps where the CPU automatically pushes an error code.
 *
 * You shouldn't call a TRAPHANDLER function from C, but you may
 * need to _declare_ one in C (for instance, to get a function pointer
 * during IDT setup).  You can declare the function with
 *   void NAME();
 * where NAME is the argument passed to TRAPHANDLER.
 */
#define TRAPHANDLER(name, num)						\
.data;\
    .long name;\
.text;\
	.globl name;		/* define global symbol for 'name' */	\
	.type name, @function;	/* symbol type is function */		\
	.align 2;		/* align function definition */		\
	name:			/* function starts here */		\
	pushl $(num);							\
	jmp _alltraps

/* Use TRAPHANDLER_NOEC for traps where the CPU doesn't push an error code.
 * It pushes a 0 in place of the error code, so the trap frame has the same
 * format in either case.
 */
#define TRAPHANDLER_NOEC(name, num)					\
.data;\
    .long name;\
.text;\
	.globl name;							\
	.type name, @function;						\
	.align 2;							\
	name:								\
	pushl $0;							\
	pushl $(num);							\
	jmp _alltraps

.data
    .globl handler
    .align 4
handler:
.text

/*
 * Lab 3: Your code here for generating entry points for the different traps.
 */
TRAPHANDLER_NOEC(func0, T_DIVIDE);
TRAPHANDLER_NOEC(func1, T_DEBUG);
TRAPHANDLER_NOEC(func2, T_NMI);
TRAPHANDLER_NOEC(func3, T_BRKPT);
TRAPHANDLER_NOEC(func4, T_OFLOW);
TRAPHANDLER_NOEC(func5, T_BOUND);
TRAPHANDLER_NOEC(func6, T_ILLOP);
TRAPHANDLER_NOEC(func7, T_DEVICE);
TRAPHANDLER(func8, T_DBLFLT);
.data;
    .long 0;
TRAPHANDLER(func10, T_TSS);
TRAPHANDLER(func11, T_SEGNP);
TRAPHANDLER(func12, T_STACK);
TRAPHANDLER(func13, T_GPFLT);
TRAPHANDLER(func14, T_PGFLT);
.data;
    .long 0;
TRAPHANDLER_NOEC(func16, T_FPERR);
TRAPHANDLER_NOEC(func17, T_ALIGN);
TRAPHANDLER_NOEC(func18, T_MCHK);
TRAPHANDLER_NOEC(func19, T_SIMDERR);
.data;
    .long 0;
.data;
    .long 0;
.data;
    .long 0;
.data;
    .long 0;
.data;
    .long 0;
.data;
    .long 0;
.data;
    .long 0;
.data;
    .long 0;
.data;
    .long 0;
.data;
    .long 0;
.data;
    .long 0;
.data;
    .long 0;
TRAPHANDLER_NOEC(func32, IRQ_OFFSET);
TRAPHANDLER_NOEC(func33, IRQ_OFFSET+1);
TRAPHANDLER_NOEC(func34, IRQ_OFFSET+2);
TRAPHANDLER_NOEC(func35, IRQ_OFFSET+3);
TRAPHANDLER_NOEC(func36, IRQ_OFFSET+4);
TRAPHANDLER_NOEC(func37, IRQ_OFFSET+5);
TRAPHANDLER_NOEC(func38, IRQ_OFFSET+6);
TRAPHANDLER_NOEC(func39, IRQ_OFFSET+7);
TRAPHANDLER_NOEC(func40, IRQ_OFFSET+8);
TRAPHANDLER_NOEC(func41, IRQ_OFFSET+9);
TRAPHANDLER_NOEC(func42, IRQ_OFFSET+10);
TRAPHANDLER_NOEC(func43, IRQ_OFFSET+11);
TRAPHANDLER_NOEC(func44, IRQ_OFFSET+12);
TRAPHANDLER_NOEC(func45, IRQ_OFFSET+13);
TRAPHANDLER_NOEC(func46, IRQ_OFFSET+14);
TRAPHANDLER_NOEC(func47, IRQ_OFFSET+15);

TRAPHANDLER_NOEC(func48, T_SYSCALL);

/*
 * Lab 3: Your code here for _alltraps
 */
_alltraps:
    pushl %ds
    pushl %es
    pushal

    movw $(GD_KD), %ax
    movw %ax, %ds
    movw %ax, %es

    pushl %esp
    call trap
