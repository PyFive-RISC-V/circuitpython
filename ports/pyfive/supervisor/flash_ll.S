/*
 * flash.S
 *
 * Flash manual access routines
 *
 * Copyright (C) 2021  Sylvain Munaut <tnt@246tNt.com>
 * SPDX-License-Identifier: MIT
 */

#include "config.h"

#define MSTATUS_MIE	0x00000008

#define QSPI_BASE 	0x81000000

#define SPI_CSR		(4 * 0x00)
#define SPI_RF		(4 * 0x03)
#define SPI_CMD(t,l)	(4 * (0x10 + ((t)<<2) + ((l)-1)))

#define CMD_SPI		0
#define CMD_QPI_RD	1
#define CMD_QPI_WR	2
#define CMD_QPI_CMD	3


	.section .ramtext.flash, "a"

_flash_done:
	// Wait until we actually got control an a chance to execute
	// the commands we sent
1:
	lw	t1, SPI_CSR(a4)
	andi	t1, t1, (1 << 2)
	beq	t1, zero, 1b

	// Release manual control of the SPI controller
	li	t0, 0x00000004
	sw	t0, SPI_CSR(a4)

	// Re-enable interrupts
	csrs	mstatus, MSTATUS_MIE

	// Done
	ret


//
// uint8_t flash_read_sr1(void)
//

	.global flash_read_sr1
flash_read_sr1:

	// Disable interrupts
        csrc	mstatus, MSTATUS_MIE

	// Get manual control of the SPI controller
	li	a4, QSPI_BASE

	li	t0, 0x00000002
	sw	t0, SPI_CSR(a4)

	// Issue QPI "Read Status Register-1"
	li	t0, 0x05000000
	sw	t0, SPI_CMD(CMD_QPI_CMD, 1)(a4)

	// Issue QPI Read request
	sw	zero, SPI_CMD(CMD_QPI_RD, 1)(a4)

	// Get response
	lw	a0, SPI_RF(a4)

	// Done
	j	_flash_done


//
// uint64_t flash_unique_id()
//

	.global flash_unique_id
flash_unique_id:

	// Disable interrupts
        csrc	mstatus, MSTATUS_MIE

	// Load QSPI_BASE
	li	a4, QSPI_BASE

	// Disable QPI

		// Get manual control of the SPI controller
	li	t0, 0x00000002
	sw	t0, SPI_CSR(a4)

		// Issue QPI exit
	li	t0, 0xff000000
	sw	t0, SPI_CMD(CMD_QPI_CMD, 1)(a4)

		// Wait to make sure it got executed
1:
	lw	t1, SPI_CSR(a4)
	andi	t1, t1, (1 << 2)
	beq	t1, zero, 1b

		// Release manual control (and CS_n)
	li	t0, 0x00000004
	sw	t0, SPI_CSR(a4)

	// Read the actual ID using "Read Unique ID" in SPI mode

		// Get manual control of the SPI controller
	li	t0, 0x00000002
	sw	t0, SPI_CSR(a4)

		// Issue SPI command
	li	t0, 0x4b000000
	sw	t0, SPI_CMD(CMD_SPI, 1)(a4)
	lw	t0, SPI_RF(a4)

		// Read out 4 dummy bytes
	sw	zero, SPI_CMD(CMD_SPI, 4)(a4)
	lw	t0, SPI_RF(a4)

		// Read out 8 byte ID
	sw	zero, SPI_CMD(CMD_SPI, 4)(a4)
	lw	a1, SPI_RF(a4)
	sw	zero, SPI_CMD(CMD_SPI, 4)(a4)
	lw	a0, SPI_RF(a4)

		// Release manual control (and CS_n)
	li	t0, 0x00000004
	sw	t0, SPI_CSR(a4)

	// Re-enable QPI

		// Get manual control of the SPI controller
	li	t0, 0x00000002
	sw	t0, SPI_CSR(a4)

		// Issue SPI command
	li	t0, 0x38000000
	sw	t0, SPI_CMD(CMD_SPI, 1)(a4)
	lw	t0, SPI_RF(a4)

		// Release manual control of the SPI controller
	li	t0, 0x00000004
	sw	t0, SPI_CSR(a4)

	// Re-enable interrupts
	csrs	mstatus, MSTATUS_MIE

	// Done
	ret


//
// void flash_write_enable(void)
//

	.global flash_write_enable
flash_write_enable:

	// Disable interrupts
        csrc	mstatus, MSTATUS_MIE

	// Get manual control of the SPI controller
	li	a4, QSPI_BASE

	li	t0, 0x00000002
	sw	t0, SPI_CSR(a4)

	// Issue QPI "Write Enable" command
	li	t0, 0x06000000
	sw	t0, SPI_CMD(CMD_QPI_CMD, 1)(a4)

	// Done
	j	_flash_done


//
// void flash_sector_erase(uint32_t addr)
//

	.global flash_sector_erase
flash_sector_erase:

	// Disable interrupts
        csrc	mstatus, MSTATUS_MIE

	// Get manual control of the SPI controller
	li	a4, QSPI_BASE

	li	t0, 0x00000002
	sw	t0, SPI_CSR(a4)

	// Issue QPI "Sector Erase" command
	li	t0, 0x20000000
	or	t0, t0, a0
	sw	t0, SPI_CMD(CMD_QPI_CMD, 4)(a4)

	// Done
	j	_flash_done


//
// void flash_page_write(uint32_t addr, uint32_t *buf)
//

	.global flash_page_write
flash_page_write:

	// Copy data to out local buffer
	la	a2, _buffer
	addi	a3, a2, 256
1:
	lw	t0, 0(a1)
	lw	t1, 4(a1)
	sw	t0, 0(a2)
	sw	t1, 4(a2)
	addi	a1, a1, 8
	addi	a2, a2, 8
	bne	a2, a3, 1b

	// Disable interrupts
        csrc	mstatus, MSTATUS_MIE

	// Get manual control of the SPI controller
	li	a4, QSPI_BASE

	li	t0, 0x00000002
	sw	t0, SPI_CSR(a4)

	// Issue QPI write command
	li	t0, 0x02000000
	or	t0, t0, a0
	sw	t0, SPI_CMD(CMD_QPI_CMD, 4)(a4)

	la	a2, _buffer
1:
	lb	t0, 0(a2)
	slli	t0, t0, 24
	sw	t0, SPI_CMD(CMD_QPI_WR, 1)(a4)
	addi	a2, a2, 1
	bne	a2, a3, 1b

	// Done
	j	_flash_done

_buffer:
	.space	256
