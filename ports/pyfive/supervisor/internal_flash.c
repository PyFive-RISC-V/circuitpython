/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * SPDX-FileCopyrightText: Copyright (c) 2013, 2014 Damien P. George
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
#include "supervisor/internal_flash.h"

#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#include "extmod/vfs.h"
#include "extmod/vfs_fat.h"
#include "py/mphal.h"
#include "py/obj.h"
#include "py/runtime.h"
#include "lib/oofatfs/ff.h"

#include "supervisor/flash.h"

#include "flash_ll.h"


#define NO_CACHE        0xffffffff

#if 1
static bool     _flash_cache_dirty;
static uint8_t  _flash_cache_data[FLASH_SECTOR_SIZE] __attribute__((aligned(4)));
static uint32_t _flash_cache_addr = NO_CACHE;


static inline void *
lba2mem(uint32_t block)
{
	/* Memory pointer to memory mapped flash */
	return (void*)(0x40000000 + FLASH_PARTITION_OFFSET_BYTES + (block * FILESYSTEM_BLOCK_SIZE));
}

static inline void *
addr2mem(uint32_t addr)
{
	/* Memory pointer to memory mapped flash */
	return (void*)(0x40000000 + addr);
}

static inline uint32_t
lba2addr(uint32_t block)
{
	/* Flash address */
	return FLASH_PARTITION_OFFSET_BYTES + (block * FILESYSTEM_BLOCK_SIZE);
}


void
supervisor_flash_init(void)
{
}

uint32_t
supervisor_flash_get_block_size(void)
{
	return FILESYSTEM_BLOCK_SIZE;
}

uint32_t
supervisor_flash_get_block_count(void)
{
	return FLASH_SIZE / FILESYSTEM_BLOCK_SIZE;
}

void
port_internal_flash_flush(void)
{
	// Skip if data is the same, or if there is no data in the cache
	if (_flash_cache_addr == NO_CACHE) {
		return;
	}
	if (!_flash_cache_dirty) {
		return;
	}

	// Erase sector
	flash_do_sector_erase(_flash_cache_addr);

	// Write all the pages
	for (uint32_t ofs=0; ofs<FLASH_SECTOR_SIZE; ofs+=FLASH_PAGE_SIZE)
	{
		flash_do_page_write(_flash_cache_addr+ofs, (void*)&_flash_cache_data[ofs]);
	}

	// Hack for cache
	memcpy(
		addr2mem(_flash_cache_addr),
		_flash_cache_data,
		FLASH_SECTOR_SIZE
	);

	// All clean
	_flash_cache_dirty = false;
}

mp_uint_t
supervisor_flash_read_blocks(uint8_t *dest, uint32_t lba, uint32_t num_blocks)
{
	// Must write out anything in cache before trying to read.
	supervisor_flash_flush();

	// Just read as memory mapped
	memcpy(
		dest,
		lba2mem(lba),
		FILESYSTEM_BLOCK_SIZE * num_blocks
	);

	return 0;
}

mp_uint_t
supervisor_flash_write_blocks(const uint8_t *src, uint32_t lba, uint32_t num_blocks)
{
	while (num_blocks)
	{
		uint32_t const addr = lba2addr(lba);
		uint32_t const sector_addr = addr & ~(FLASH_SECTOR_SIZE - 1);
		uint32_t const sector_ofs  = addr &  (FLASH_SECTOR_SIZE - 1);

		if (sector_addr != _flash_cache_addr) {
			// Write out anything in cache before overwriting it.
			supervisor_flash_flush();

			_flash_cache_addr  = sector_addr;
			_flash_cache_dirty = false;

			// Copy the current contents of the entire page into the cache.
			memcpy(_flash_cache_data, addr2mem(sector_addr), FLASH_SECTOR_SIZE);
		}

		// Overwrite part or all of the page cache with the src data, but only if it's changed.
		bool changed = _flash_cache_dirty || memcmp(&_flash_cache_data[sector_ofs], src, FILESYSTEM_BLOCK_SIZE);

		if (changed) {
			memcpy(&_flash_cache_data[sector_ofs], src, FILESYSTEM_BLOCK_SIZE);
			_flash_cache_dirty = true;
		}

		// next
		src        += FILESYSTEM_BLOCK_SIZE;
		lba        += 1;
		num_blocks -= 1;
	}

	return 0; // success
}

void
supervisor_flash_release_cache(void)
{
}

#else


// ----------------------------------------------
// RAM impl
// ----------------------------------------------

static uint8_t memdisk[FLASH_SIZE] __attribute__((aligned(4)));
static bool    memdisk_init_done = false;

void
supervisor_flash_init(void)
{
	if (memdisk_init_done)
		return;

	memdisk_init_done = true;

	memcpy(
		memdisk,
		(void*)(0x40000000 + FLASH_PARTITION_OFFSET_BYTES),
		FLASH_SIZE
	);
}

uint32_t
supervisor_flash_get_block_size(void)
{
	return FILESYSTEM_BLOCK_SIZE;
}

uint32_t
supervisor_flash_get_block_count(void)
{
	return FLASH_SIZE / FILESYSTEM_BLOCK_SIZE;
}

void
port_internal_flash_flush(void)
{
}

mp_uint_t
supervisor_flash_read_blocks(uint8_t *dest, uint32_t block, uint32_t num_blocks)
{
	// Just read as memory mapped
	memcpy(
		dest,
		&memdisk[block * FILESYSTEM_BLOCK_SIZE],
		FILESYSTEM_BLOCK_SIZE * num_blocks
	);

	return 0;
}

mp_uint_t
supervisor_flash_write_blocks(const uint8_t *src, uint32_t lba, uint32_t num_blocks)
{
	memcpy(
		&memdisk[lba * FILESYSTEM_BLOCK_SIZE],
		src,
		FILESYSTEM_BLOCK_SIZE * num_blocks
	);

	return 0; // success
}

void
supervisor_flash_release_cache(void)
{
}
#endif
