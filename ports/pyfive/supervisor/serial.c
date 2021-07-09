/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2017, 2018 Scott Shawcroft for Adafruit Industries
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

#include "py/mphal.h"
#include <string.h>
#include "supervisor/serial.h"

#include "config.h"

struct wb_uart {
	uint32_t data;
	uint32_t ctrl;
} __attribute__((packed,aligned(4)));

static volatile struct wb_uart * const uart_regs = (void*)(UART_BASE);


void
serial_early_init(void)
{
	uart_regs->ctrl = 46; /* 1 Mbaud with clk=48 MHz */
	serial_write("BOOT\n");
}

void
serial_init(void)
{
}

bool
serial_connected(void)
{
	return true;
}

char
serial_read(void)
{
	int32_t c;
	c = uart_regs->data;
	return c & 0x80000000 ? 0 : (c & 0xff);
}

bool
serial_bytes_available(void)
{
	return (uart_regs->ctrl & (1 << 31)) ? false : true;
}

void
serial_write(const char *text)
{
	serial_write_substring(text, strlen(text));
}

void
serial_write_substring(const char *text, uint32_t len)
{
	if (len == 0)
		return;
	for (unsigned int i=0; i<len; i++)
		uart_regs->data = text[i];
}

void
supervisor_workflow_reset(void)
{
}

bool
supervisor_workflow_active(void)
{
	return false;
}
