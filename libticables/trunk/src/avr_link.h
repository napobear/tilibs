/*  tilp - link program for TI calculators
 *  Copyright (C) 1999-2001  Romain Lievin
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

#ifndef AVRLINK_H
#define AVRLINK_H

#include "typedefs.h"

int avr_init_port();
int avr_open_port();
int avr_put(byte data);
int avr_get(byte *data);
int avr_probe_port();
int avr_close_port();
int avr_term_port();
int avr_check_port(int *status);

int avr_set_red_wire(int b);
int avr_set_white_wire(int b);
int avr_get_red_wire();
int avr_get_white_wire();

int avr_supported();

#endif
