/*
 * flash.h
 *
 * Flash manual access routines
 *
 * Copyright (C) 2021  Sylvain Munaut <tnt@246tNt.com>
 * SPDX-License-Identifier: MIT
 */

#pragma once

uint8_t  flash_read_sr1(void);
uint64_t flash_unique_id(void);
void     flash_write_enable(void);
void     flash_sector_erase(uint32_t addr);
void     flash_page_write(uint32_t addr, uint32_t *buf);

