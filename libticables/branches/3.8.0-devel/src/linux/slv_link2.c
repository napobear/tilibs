/* Hey EMACS -*- linux-c -*- */
/* $Id: slv_link.c 370 2004-03-22 18:47:32Z roms $ */

/*  libticables - Ti Link Cable library, a part of the TiLP project
 *  Copyright (C) 1999-2004  Romain Lievin
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

/* TI-GRAPH LINK USB support (lib-usb) */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "intl.h"
#include "export.h"
#include "cabl_def.h"
#include "cabl_err.h"
#include "verbose.h"
#include "logging.h"
#include "externs.h"
#include "timeout.h"

/* 
   Some important remarks... (http://lpg.ticalc.org/prj_usb/index.html)
   
   This link cable use Bulk mode with packets. The max size of a packet is 
   32 bytes (MAX_PACKET_SIZE/BULKUSB_MAX_TRANSFER_SIZE). 
   
   This is transparent for the user because the driver manages all these 
   things for us. Nethertheless, this fact has some consequences:
   - it is better (for USB & OS performances) to read/write a set of bytes 
   rather than byte per byte.
   - for reading, we have to read up to 32 bytes at a time (even if we need 
   only 1 byte) and to store them in a buffer for subsequent acesses. 
   In fact, if we try and get byte per byte, it will not work.
   - for writing, we don't store bytes in a buffer. It seems better to send
   data byte per byte (latency ?!).
   - another particular effect (quirk): sometimes (usually when calc need to 
   reply and takes a while), a read call can returns with no data or timeout. 
   Simply retry a read call and it works fine.
*/


#include <fcntl.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#ifdef HAVE_STDINT_H
# include <stdint.h>
#else
# include <inttypes.h>
#endif

//#define BUFFERED_W /* enable buffered write operations */ 
#define BUFFERED_R		/* enable buffered read operations (default) */

#define MAX_PACKET_SIZE 32	// 32 bytes max per packet
static int nBytesWrite = 0;
#ifdef BUFFERED_W
static uint8_t wBuf[MAX_PACKET_SIZE];
#endif
static int nBytesRead = 0;
static uint8_t rBuf[MAX_PACKET_SIZE];
#endif

/*********************************/
/* Linux   : libusb support      */
/* Author  : Julien BLACHE       */
/* Contact : jb@technologeek.org */
/* Date    : 20011126            */
/*********************************/

#if defined(HAVE_LIBUSB)

#include <usb.h>

#define TIGL_VENDOR_ID  0x0451	/* Texas Instruments, Inc.        */
#define TIGL_PRODUCT_ID 0xE001	/* TI-GRAPH LINK USB (SilverLink) */

#define TIGL_BULK_IN    0x81	// 0x81?
#define TIGL_BULK_OUT   0x02

struct usb_bus *bus = NULL;
struct usb_device *dev = NULL;
struct usb_device *tigl_dev = NULL;
usb_dev_handle *tigl_han = NULL;

#define DISPLAY_ERR DISPLAY_ERROR

static void find_tigl_device(void)
{
  /* loop taken from testlibusb.c */
  for (bus = usb_busses; bus; bus = bus->next) {
    for (dev = bus->devices; dev; dev = dev->next) {
      if ((dev->descriptor.idVendor == TIGL_VENDOR_ID) &&
	  (dev->descriptor.idProduct == TIGL_PRODUCT_ID)) {
	/* keep track of the TIGL device */
	DISPLAY(_("TIGL-USB found with libusb.\n"));

	tigl_dev = dev;
	break;
      }
    }

    /* if we found the device, then stop... */
    if (tigl_dev != NULL)
      break;
  }
}

static int enumerate_tigl_device(void)
{
  int ret = 0;

  /* init the libusb */
  usb_init();

  /* find all usb busses on the system */
  ret = usb_find_busses();
  if (ret < 0) {
    DISPLAY_ERR(_("usb_find_busses: %s\n"), usb_strerror());
    return ERR_LIBUSB_OPEN;
  }

  /* find all usb devices on all discovered busses */
  ret = usb_find_devices();
  if (ret < 0) {
    DISPLAY_ERR(_("usb_find_devices: %s\n"), usb_strerror());
    return ERR_LIBUSB_OPEN;
  }

  /* iterate through the busses/devices */
  find_tigl_device();

  /* if we didn't find our TIGL USB, then slv_init() and retry... */
  if (tigl_dev != NULL) {
    tigl_han = usb_open(tigl_dev);
    if (tigl_han != NULL) {
      /* interface 0, configuration 1 */
      ret = usb_claim_interface(tigl_han, 0);
      if (ret < 0) {
	DISPLAY_ERR("usb_claim_interface: %s\n", usb_strerror());
	return ERR_LIBUSB_INIT;
      }

      ret = usb_set_configuration(tigl_han, 1);
      if (ret < 0) {
	DISPLAY_ERR("usb_set_configuration: %s\n", usb_strerror());
	return ERR_LIBUSB_INIT;
      }

      return 0;
    } else
      return ERR_LIBUSB_OPEN;
  }

  if (tigl_han == NULL)
    return ERR_LIBUSB_OPEN;

  return 0;
}

int slv_init2()
{
  START_LOGGING();

  return enumerate_tigl_device();
}

int slv_open2()
{
  int ret = 0;

  if (tigl_han == NULL) {
    if (slv_init2() != 0)
      return ERR_LIBUSB_OPEN;
  }

  /* Flush buffer */
  /*
     ret = usb_bulk_read(tigl_han, TIGL_BULK_IN, rBuf, 
     MAX_PACKET_SIZE, (time_out * 10));
   */

#if !defined(__BSD__)
  /* Reset endpoints */
  ret = usb_clear_halt(tigl_han, TIGL_BULK_OUT);
  if (ret < 0) {
    DISPLAY_ERR("usb_clear_halt: %s\n", usb_strerror());

    ret = usb_resetep(tigl_han, TIGL_BULK_OUT);
    if (ret < 0) {
      DISPLAY_ERR("usb_resetep: %s\n", usb_strerror());

      ret = usb_reset(tigl_han);
      if (ret < 0) {
	DISPLAY_ERR("usb_reset: %s\n", usb_strerror());
	return ERR_LIBUSB_RESET;
      }
    }
  }

  ret = usb_clear_halt(tigl_han, TIGL_BULK_IN);
  if (ret < 0) {
    DISPLAY_ERR("usb_clear_halt: %s\n", usb_strerror());

    ret = usb_resetep(tigl_han, TIGL_BULK_OUT);
    if (ret < 0) {
      DISPLAY_ERR("usb_resetep: %s\n", usb_strerror());

      ret = usb_reset(tigl_han);
      if (ret < 0) {
	DISPLAY_ERR("usb_reset: %s\n", usb_strerror());
	return ERR_LIBUSB_RESET;
      }
    }
  }
#endif

  /* Reset buffers */
  nBytesRead = 0;
  nBytesWrite = 0;

  tdr.count = 0;
  toSTART(tdr.start);

  return 0;
}

int slv_close2()
{
  return 0;
}

int slv_exit2()
{
  tigl_dev = NULL;

  STOP_LOGGING();

  if (tigl_han != NULL) {
    usb_release_interface(tigl_han, 0);
    usb_close(tigl_han);
    tigl_han = NULL;
  }

  return 0;
}

int slv_put2(uint8_t data)
{
  int ret = 0;

  tdr.count++;
  LOG_DATA(data);
#ifndef BUFFERED_W
  /* Byte per byte */
  ret = usb_bulk_write(tigl_han, TIGL_BULK_OUT, &data, 1, (time_out * 10));
  if (ret <= 0) {
    DISPLAY_ERR("usb_bulk_write: %s\n", usb_strerror());
    return ERR_WRITE_ERROR;
  }
#else
  /* Packets (up to 32 bytes) */
  wBuf[nBytesWrite++] = data;
  if (nBytesWrite == MAX_PACKET_SIZE) {
    ret =
	usb_bulk_write(tigl_han, TIGL_BULK_OUT, wBuf,
		       nBytesWrite, (time_out * 10));
    if (ret <= 0) {
      DISPLAY_ERR("usb_bulk_write: %s\n", usb_strerror());
      return ERR_WRITE_ERROR;
    }
    nBytesWrite = 0;
  }
#endif

  return 0;
}

int slv_get2(uint8_t * data)
{
  int ret = 0;
  tiTIME clk;
  static uint8_t *rBufPtr;

  //printf(".");

  tdr.count++;
#ifdef BUFFERED_W
  /* Flush write buffer */
  if (nBytesWrite > 0) {
    ret =
	usb_bulk_write(tigl_han, TIGL_BULK_OUT, wBuf,
		       nBytesWrite, (time_out * 10));
    nBytesWrite = 0;
    if (ret <= 0) {
      DISPLAY_ERR("usb_bulk_write: %s\n", usb_strerror());
      return ERR_WRITE_ERROR;
    }
  }
#endif

  if (nBytesRead <= 0) {
    toSTART(clk);
    do {
      ret = usb_bulk_read(tigl_han, TIGL_BULK_IN, rBuf,
			  MAX_PACKET_SIZE, (time_out * 10));
      if (toELAPSED(clk, time_out))
	return ERR_READ_TIMEOUT;
      if (ret == 0)
	DISPLAY_ERR
	    (_
	     ("usb_bulk_read returns without any data. Retrying for circumventing quirk...\n"));
    }
    while (!ret);

    if (ret < 0) {
      DISPLAY_ERR("usb_bulk_read: %s\n", usb_strerror());
      nBytesRead = 0;
      return ERR_READ_ERROR;
    }
    nBytesRead = ret;
    rBufPtr = rBuf;
  }

  *data = *rBufPtr++;
  nBytesRead--;
  LOG_DATA(*data);

  return 0;
}

int slv_probe2()
{
  if (tigl_dev != NULL)
    return 0;
  else
    return ERR_PROBE_FAILED;
}

int slv_check2(int *status)
{
  tiTIME clk;
  int ret = 0;

  /* Since the select function does not work, I do it myself ! */
  *status = STATUS_NONE;

  if (tigl_han != NULL) {
    if (nBytesRead > 0) {
      *status = STATUS_RX;
      return 0;
    }

    toSTART(clk);
    do {
      ret = usb_bulk_read(tigl_han, TIGL_BULK_IN, rBuf,
			  MAX_PACKET_SIZE, (time_out * 10));
      if (toELAPSED(clk, time_out))
	return ERR_READ_TIMEOUT;
      if (ret == 0)
	DISPLAY_ERR
	    ("usb_bulk_read returns without any data. Retrying...\n");
    }
    while (!ret);

    if (ret > 0) {
      nBytesRead = ret;
      *status = STATUS_RX;
      return 0;
    } else {
      nBytesRead = 0;
      *status = STATUS_NONE;
      return 0;
    }
  }

  return 0;
}

#define swap_bits(a) (((a&2)>>1) | ((a&1)<<1))	// swap the 2 lowest bits

int slv_set_red_wire2(int b)
{
  return 0;
}

int slv_set_white_wire2(int b)
{
  return 0;
}

int slv_get_red_wire2()
{
  return 0;
}

int slv_get_white_wire2()
{
  return 0;
}

int slv_supported2()
{				/* HELL YES IT'S SUPPORTED ! :-) */
  return SUPPORT_ON;
}

int slv_register_cable2(TicableLinkCable * lc, TicableMethod method)
{
  lc->init = slv_init2;
  lc->open = slv_open2;
  lc->put = slv_put2;
  lc->get = slv_get2;
  lc->close = slv_close2;
  lc->exit = slv_exit2;
  lc->probe = slv_probe2;
  lc->check = slv_check2;

  lc->set_red_wire = NULL;
  lc->set_white_wire = NULL;
  lc->get_red_wire = NULL;
  lc->get_white_wire = NULL;

  return 0;
}

int slv_unregister_cable2(TicableLinkCable * lc)
{
	memset(lc, 0, sizeof(lc));
	
	return 0;
}