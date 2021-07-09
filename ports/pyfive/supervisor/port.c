/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2017 Scott Shawcroft for Adafruit Industries
 * Copyright (c) 2019 Lucian Copeland for Adafruit Industries
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <stdint.h>
#include "supervisor/board.h"
#include "supervisor/port.h"
#include "supervisor/shared/tick.h"

#include "riscv.h"
#include "config.h"

#include "shared-bindings/microcontroller/__init__.h"

// Platform
struct platform {
	struct {
		uint32_t lo;
		uint32_t hi;
	} mtime;
	struct {
		uint32_t lo;
		uint32_t hi;
	} mtimecmp;
	struct {
		uint32_t ena;
		uint32_t active;	// Write 1 to ack (for edge triggered)
	} irq;
} __attribute__((packed,aligned(4)));

static volatile struct platform * const platform_regs = (void*)(PLATFORM_BASE);

#define TICK_INC	46875


// Global millisecond tick count. 1024 per second because most RTCs are clocked with 32.768khz
// crystals.
volatile uint64_t raw_ticks = 0;

// Next hw ticks interrupt time
static uint64_t time_next_irq;


static uint64_t
plat_time_now(void)
{
	uint32_t lo, hi[2];

	do {
		hi[0] = platform_regs->mtime.hi;
		lo    = platform_regs->mtime.lo;
		hi[1] = platform_regs->mtime.hi;
	} while (hi[0] != hi[1]);

	return (((uint64_t)hi[0]) << 32) | lo;
}

static void
plat_time_next_irq(uint64_t t)
{
	platform_regs->mtimecmp.lo = 0xffffffff;
	platform_regs->mtimecmp.hi = t >> 32;
	platform_regs->mtimecmp.lo = t & 0xffffffff;
}

static void
tick_schedule_next(void)
{
	time_next_irq += TICK_INC;
	plat_time_next_irq(time_next_irq);
}

static void
tick_init(void)
{
	time_next_irq = plat_time_now();
	tick_schedule_next();
	csr_set(mie, MIE_MTIE);
}

void 
SysTick_Handler(void)
{
	tick_schedule_next();
	raw_ticks += 1;
	supervisor_tick();
}

safe_mode_t
port_init(void)
{
	csr_write(mie, MIE_MEIE);
	csr_write(mstatus, MSTATUS_MIE);
	tick_init();
	platform_regs->irq.ena = 1;
	return NO_SAFE_MODE;
}

extern uint32_t _ebss;
extern uint32_t _heap_start;
extern uint32_t _estack;

void
reset_port(void)
{
	/* FIXME */
}

void
reset_to_bootloader(void)
{
	/* FIXME */
	while (1);
}

void
reset_cpu(void)
{
	/* FIXME */
	while (1);
}

bool
port_has_fixed_stack(void)
{
	return false;
}

uint32_t *
port_heap_get_bottom(void)
{
	return port_stack_get_limit();
}

uint32_t *
port_heap_get_top(void)
{
	return port_stack_get_top();
}

uint32_t *
port_stack_get_limit(void)
{
	return &_ebss;
}

uint32_t *
port_stack_get_top(void)
{
	return &_estack;
}

// Place the word to save just after our BSS section that gets blanked.
void
port_set_saved_word(uint32_t value)
{
	_ebss = value;
}

uint32_t
port_get_saved_word(void)
{
	return _ebss;
}

uint64_t
port_get_raw_ticks(uint8_t *subticks)
{
	// Reading 64 bits may take two loads, so turn of interrupts while we do it.
	common_hal_mcu_disable_interrupts();
	uint64_t raw_tick_snapshot = raw_ticks;
	common_hal_mcu_enable_interrupts();
	return raw_tick_snapshot;
}

// Enable 1/1024 second tick.
void port_enable_tick(void) {
}

// Disable 1/1024 second tick.
void port_disable_tick(void) {
}

void port_interrupt_after_ticks(uint32_t ticks) {
}

// TODO: Add sleep support if the SoC supports sleep.
void port_idle_until_interrupt(void) {
}
