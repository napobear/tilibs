/* Hey EMACS -*- linux-c -*- */
/* $Id$ */

/*  libtifiles - Ti File Format library, a part of the TiLP project
 *  Copyright (C) 1999-2005  Romain Lievin
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef __TIFILES_FILES8X_H__
#define __TIFILES_FILES8X_H__

#include "stdints.h"
#include "files8x.h"

/* Structures */

/**
 * Ti8xVarEntry:
 *
 * Alias to @TiVarEntry. Common to all calcs.
 **/
typedef TiVarEntry Ti8xVarEntry;

/**
 * Ti8xRegular:
 *
 * Alias to TiRegular. Common to all calcs.
 **/
typedef TiRegular Ti8xRegular;

/**
 * Ti8xBackup:
 * @model: calculator model.
 * @comment: comment embedded in file.
 * @type: a variable type ID for backup.
 * @mem_address: memory address for backup
 * @data_lengthX: length of data part #X
 * @data_partX: pure backup data #X.
 * @checksum: checksum of file.
 *
 * A generic structure used to store the content of a TI8x backup file.
 **/
typedef struct 
{
  TiCalcModel model;

  char		comment[43];
  uint8_t	type;
  uint16_t	mem_address;
  uint16_t	data_length1;
  uint8_t*	data_part1;
  uint16_t	data_length2;
  uint8_t*	data_part2;
  uint16_t	data_length3;
  uint8_t*	data_part3;
  uint16_t	data_length4;	// TI86 only
  uint8_t*	data_part4;
  uint16_t	checksum;

} Ti8xBackup;

/**
 * Ti8xFlashPage:
 * @offset: FLASH offset (see TI link guide).
 * @page: FLASH page (see TI link guide).
 * @flag: see link guide.
 * @length: length of pure data.
 * @data: pure FLASH data.
 *
 * A generic structure used to store the content of a TI8x memory page for FLASH.
 **/
typedef struct 
{
  uint16_t	addr;
  uint16_t	page;
  uint8_t	flag;
  uint16_t	size;
  uint8_t*	data;

} Ti8xFlashPage;

/**
 * Ti8xFlash:
 * @model: a calculator model.
 * @revision_major:
 * @revision_minor:
 * @flags:
 * @object_type:
 * @revision_day:
 * @revision_month:
 * @revision_year: 
 * @name: name of FLASH app or "basecode" for OS
 * @device_type: a device ID (TI89: 0x88, TI92+:0x98)
 * @data_type: var type ID (app, os, certificate, ...)
 * @num_pages: number of memory pages (size of the %pages array).
 * @pages: array of memory pages for FLASH device.
 *
 * A generic structure used to store the content of a TI8x FLASH file (os or app).
 **/
typedef struct ti8x_flash Ti8xFlash;
struct ti8x_flash 
{
  TiCalcModel	model;

  uint8_t		revision_major;
  uint8_t		revision_minor;
  uint8_t		flags;
  uint8_t		object_type;
  uint8_t		revision_day;
  uint8_t		revision_month;
  uint16_t		revision_year;
  char			name[9];
  uint8_t		device_type;
  uint8_t		data_type;
  uint32_t		data_length;

  int			 num_pages;
  Ti8xFlashPage* pages;
};

#define DEVICE_TYPE_83P 0x73
#define DEVICE_TYPE_73  0x74

/* Functions */

// allocating
TIEXPORT Ti8xRegular* TICALL ti8x_content_create_regular(void);
TIEXPORT Ti8xBackup*  TICALL ti8x_content_create_backup(void);
TIEXPORT Ti8xFlash*   TICALL ti8x_content_create_flash(void);
// freeing
TIEXPORT void TICALL ti8x_content_free_regular(Ti8xRegular *content);
TIEXPORT void TICALL ti8x_content_free_backup(Ti8xBackup *content);
TIEXPORT void TICALL ti8x_content_free_flash(Ti8xFlash *content);
// displaying
TIEXPORT int TICALL ti8x_content_display_regular(Ti8xRegular *content);
TIEXPORT int TICALL ti8x_content_display_backup(Ti8xBackup *content);
TIEXPORT int TICALL ti8x_content_display_flash(Ti8xFlash *content);

// reading
TIEXPORT int TICALL ti8x_file_read_regular(const char *filename, Ti8xRegular *content);
TIEXPORT int TICALL ti8x_file_read_backup(const char *filename, Ti8xBackup *content);
TIEXPORT int TICALL ti8x_file_read_flash(const char *filename, Ti8xFlash *content);
// writing
TIEXPORT int TICALL ti8x_file_write_regular(const char *filename, Ti8xRegular *content, char **filename2);
TIEXPORT int TICALL ti8x_file_write_backup(const char *filename, Ti8xBackup *content);
TIEXPORT int TICALL ti8x_file_write_flash(const char *filename, Ti8xFlash *content);
// displaying
TIEXPORT int TICALL ti8x_file_display(const char *filename);

#endif
