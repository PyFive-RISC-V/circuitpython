/*
 * crt0.S
 *
 * Startup code taken from picosoc/picorv32 and adapted for use here
 *
 * Copyright (C) 2017 Clifford Wolf <clifford@clifford.at>
 * Copyright (C) 2019 Sylvain Munaut <tnt@246tNt.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#define UART_BASE 0x82000000
#define BOOT_DEBUG

	.section .text.start
	.global _start
_start:

#ifdef BOOT_DEBUG
	li	a0, UART_BASE
	li	a1, 46
	sw	a1, 4(a0)
	li	a1, 65
	sw	a1, 0(a0)
#endif

	// Init .ramtext section from flash
	la	a0, _siramtext
	la	a1, _sramtext
	la	a2, _eramtext
	bge	a1, a2, end_init_ramtext
loop_init_ramtext:
	lw	a3, 0(a0)
	sw	a3, 0(a1)
	addi	a0, a0, 4
	addi	a1, a1, 4
	blt	a1, a2, loop_init_ramtext
end_init_ramtext:

	// Init .data section from flash
	la	a0, _sidata
	la	a1, _sdata
	la	a2, _edata
	bge	a1, a2, end_init_data
loop_init_data:
	lw	a3, 0(a0)
	sw	a3, 0(a1)
	addi	a0, a0, 4
	addi	a1, a1, 4
	blt	a1, a2, loop_init_data
end_init_data:

	// Clear .bss section
	la	a0, _sbss
	la	a1, _ebss
	bge	a0, a1, end_init_bss
loop_init_bss:
	sw	zero, 0(a0)
	addi	a0, a0, 4
	blt	a0, a1, loop_init_bss
end_init_bss:

	// Set stack pointer
	la	sp, _estack

	// Set trap handler
	la	a0, _trap_entry
	csrw	mtvec, a0

#ifdef BOOT_DEBUG
	li	a0, UART_BASE
	li	a1, 66
	sw	a1, 0(a0)
#endif

	// call main
	call	main

	.global	_exit
_exit:
	j	_exit


	.section .ramtext.trap, "a"
	.align	4
_trap_entry:
	addi	sp,sp, -16*4
	sw	x1,   0*4(sp)
	sw	x5,   1*4(sp)
	sw	x6,   2*4(sp)
	sw	x7,   3*4(sp)
	sw	x10,  4*4(sp)
	sw	x11,  5*4(sp)
	sw	x12,  6*4(sp)
	sw	x13,  7*4(sp)
	sw	x14,  8*4(sp)
	sw	x15,  9*4(sp)
	sw	x16, 10*4(sp)
	sw	x17, 11*4(sp)
	sw	x28, 12*4(sp)
	sw	x29, 13*4(sp)
	sw	x30, 14*4(sp)
	sw	x31, 15*4(sp)

	call	trap

	lw	x1,   0*4(sp)
	lw	x5,   1*4(sp)
	lw	x6,   2*4(sp)
	lw	x7,   3*4(sp)
	lw	x10,  4*4(sp)
	lw	x11,  5*4(sp)
	lw	x12,  6*4(sp)
	lw	x13,  7*4(sp)
	lw	x14,  8*4(sp)
	lw	x15,  9*4(sp)
	lw	x16, 10*4(sp)
	lw	x17, 11*4(sp)
	lw	x28, 12*4(sp)
	lw	x29, 13*4(sp)
	lw	x30, 14*4(sp)
	lw	x31, 15*4(sp)
	addi	sp,sp, 16*4
	mret
