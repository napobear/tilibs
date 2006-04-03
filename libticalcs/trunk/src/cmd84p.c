/* Hey EMACS -*- linux-c -*- */
/* $Id: cmd84p.c 2077 2006-03-31 21:16:19Z roms $ */

/*  libticalcs - Ti Calculator library, a part of the TiLP project
 *  Copyright (C) 1999-2005  Romain Li�vin
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

/*
  This unit handles TI84+ commands with DirectLink.
*/

// Some functions should be renamed...

#include <string.h>

#include "ticalcs.h"
#include "logging.h"
#include "error.h"
#include "macros.h"

#include "dusb_vpkt.h"
#include "cmd84p.h"

// Ping or Set Mode
int ti84p_mode_set(CalcHandle *h)
{
	ModeSet mode = { 0 };
	VirtualPacket* pkt;

	mode.arg1 = 3;	// normal operation mode
	mode.arg2 = 1;
	mode.arg5 = 0x07d0;

	TRYF(dusb_buffer_size_request(h));
	TRYF(dusb_buffer_size_alloc(h));

	pkt = vtl_pkt_new(sizeof(mode), VPKT_PING);
	pkt->data[0] = MSB(mode.arg1);
	pkt->data[1] = LSB(mode.arg1);
	pkt->data[2] = MSB(mode.arg2);
	pkt->data[3] = LSB(mode.arg2);
	pkt->data[4] = MSB(mode.arg3);
	pkt->data[5] = LSB(mode.arg3);
	pkt->data[6] = MSB(mode.arg4);
	pkt->data[7] = LSB(mode.arg4);
	pkt->data[8] = MSB(mode.arg5);
	pkt->data[9] = LSB(mode.arg5);

	TRYF(dusb_send_data(h, pkt));	// set mode
	TRYF(dusb_recv_data(h, pkt));	// ack

	if(pkt->type != VPKT_MODE_SET)
		return ERR_INVALID_PACKET;

	vtl_pkt_del(pkt);
	return 0;
}

CalcParam*	cp_new(uint16_t id, uint16_t size)
{
	CalcParam* cp = calloc(1, sizeof(CalcParam));

	cp->id = id;
	cp->size = size;
	cp->data = calloc(1, size);

	return cp;
}

void		cp_del(CalcParam* cp)
{
	free(cp->data);
	free(cp);
}

void cp_del_array(int nparams, CalcParam *params)
{
	int i;

	for(i = 0; i < nparams; i++)
		if(params[i].ok)
			free(params[i].data);
	free(params);
}

CalcAttr*	ca_new(uint16_t id, uint16_t size)
{
	CalcAttr* cp = calloc(1, sizeof(CalcAttr));

	cp->id = id;
	cp->size = size;
	cp->data = calloc(1, size);

	return cp;
}

void		ca_del(CalcAttr* cp)
{
	free(cp->data);
	free(cp);
}

// Request one or more calc parameters
int ti84p_params_request(CalcHandle *h, int nparams, uint16_t *pids)
{
	VirtualPacket* pkt;
	int i;

	pkt = vtl_pkt_new((nparams + 1) * sizeof(uint16_t), VPKT_PARM_REQ);

	pkt->data[0] = MSB(nparams);
	pkt->data[1] = LSB(nparams);

	for(i = 0; i < nparams; i++)
	{
		pkt->data[2*(i+1) + 0] = MSB(pids[i]);
		pkt->data[2*(i+1) + 1] = LSB(pids[i]);
	}

	TRYF(dusb_send_data(h, pkt));	// param request
	TRYF(dusb_recv_data(h, pkt));	// ack
	if(pkt->type != VPKT_PARM_ACK)
		return ERR_INVALID_PACKET;

	vtl_pkt_del(pkt);
	return 0;
}

int ti84p_params_get(CalcHandle *h, int nparams, CalcParam **params)
{
	VirtualPacket* pkt;
	int i, j;

	pkt = vtl_pkt_new(0, 0);
	TRYF(dusb_recv_data(h, pkt));	// param data

	if(pkt->type != VPKT_PARM_DATA)
		return ERR_INVALID_PACKET;

	if(((pkt->data[j=0] << 8) | pkt->data[j=1]) != nparams)
		return ERR_INVALID_PACKET;

	*params = (CalcParam *)calloc(nparams + 1, sizeof(CalcParam));
	for(i = 0, j = 2; i < nparams; i++)
	{
		CalcParam *s = *params + i;

		s->id = pkt->data[j++] << 8; s->id |= pkt->data[j++];
		s->ok = !pkt->data[j++];
		if(s->ok)
		{
			s->size = pkt->data[j++] << 8; s->size |= pkt->data[j++];
			s->data = (uint8_t *)calloc(1, s->size);
			memcpy(s->data, &pkt->data[j], s->size);
			j += s->size;
		}
	}
	
	vtl_pkt_del(pkt);
	return 0;
}

// Set one  calc parameter
int ti84p_params_set(CalcHandle *h, const CalcParam *param)
{
	VirtualPacket* pkt;

	pkt = vtl_pkt_new((2 + 2 + param->size) * sizeof(uint16_t), VPKT_PARM_SET);

	pkt->data[0] = MSB(param->id);
	pkt->data[1] = LSB(param->id);
	pkt->data[2] = MSB(param->size);
	pkt->data[3] = LSB(param->size);
	memcpy(pkt->data + 4, param->data, param->size);

	TRYF(dusb_send_data(h, pkt));	// param set
	TRYF(dusb_recv_data(h, pkt));	// ack
	if(pkt->type != VPKT_DATA_ACK)
		return ERR_INVALID_PACKET;

	vtl_pkt_del(pkt);

	return 0;
}

// Request dirlist
int ti84p_dirlist_request(CalcHandle *h, int n, uint16_t *aids)
{
	VirtualPacket* pkt;
	int i;

	pkt = vtl_pkt_new(4 + 2*n + 7, VPKT_DIR_REQ);

	pkt->data[0] = MSB(MSW(n));
	pkt->data[1] = LSB(MSW(n));
	pkt->data[2] = MSB(LSW(n));
	pkt->data[3] = LSB(LSW(n));

	for(i = 0; i < n; i++)
	{
		pkt->data[4+2*i+0] = MSB(aids[i]);
		pkt->data[4+2*i+1] = LSB(aids[i]);
	}

	i = 2*i;
	i += 4;
	pkt->data[i+0] = 0x00; pkt->data[i+0] = 0x01;
	pkt->data[i+0] = 0x00; pkt->data[i+0] = 0x01;
	pkt->data[i+0] = 0x00; pkt->data[i+0] = 0x01;
	pkt->data[i+0] = 0x01;

	TRYF(dusb_send_data(h, pkt));

	vtl_pkt_del(pkt);
	return 0;
}

// name is utf-8 => 18 chars max
int ti84p_var_header(CalcHandle *h, char *name, CalcAttr **attr)
{
	VirtualPacket* pkt;
	uint16_t name_length;
	int nattr;
	int i, j;

	pkt = vtl_pkt_new(0, 0);
	TRYF(dusb_recv_data(h, pkt));	// variable header

	if(pkt->type == VPKT_EOT)
	{
		vtl_pkt_del(pkt);
		return ERR_EOT;
	}
	else if(pkt->type != VPKT_VAR_HDR)
		return ERR_INVALID_PACKET;

	name_length = (pkt->data[0] << 8) | pkt->data[1];
	memcpy(name, pkt->data + 2, name_length+1);

	nattr = (pkt->data[name_length+3] << 8) | pkt->data[name_length+4];
	*attr = (CalcAttr *)calloc(nattr + 1, sizeof(CalcAttr));

	for(i = 0, j = name_length+5; i < nattr; i++)
	{
		CalcAttr *s = *attr + i;

		s->id = pkt->data[j++] << 8; s->id |= pkt->data[j++];
		s->ok = !pkt->data[j++];
		if(s->ok)
		{
			s->size = pkt->data[j++] << 8; s->size |= pkt->data[j++];
			s->data = (uint8_t *)calloc(1, s->size);
			memcpy(s->data, &pkt->data[j], s->size);
			j += s->size;
		}
	}
	
	vtl_pkt_del(pkt);
	return 0;
}