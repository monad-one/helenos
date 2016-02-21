/*
 * Copyright (c) 2016 Jiri Svoboda
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * - The name of the author may not be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/** @addtogroup netecho
 * @{
 */
/** @file
 */

#include <byteorder.h>
#include <stdbool.h>
#include <errno.h>
#include <fibril.h>
#include <inet/dnsr.h>
#include <inet/endpoint.h>
#include <inet/udp.h>
#include <macros.h>
#include <stdio.h>
#include <stdlib.h>
#include <str_error.h>
#include <sys/types.h>

#include "comm.h"
#include "netecho.h"

static udp_t *udp;
static udp_assoc_t *assoc;
static inet_ep_t remote;

#define RECV_BUF_SIZE 1024
static uint8_t recv_buf[RECV_BUF_SIZE];

static void comm_udp_recv_msg(udp_assoc_t *, udp_rmsg_t *);
static void comm_udp_recv_err(udp_assoc_t *, udp_rerr_t *);
static void comm_udp_link_state(udp_assoc_t *, udp_link_state_t);

static udp_cb_t comm_udp_cb = {
	.recv_msg = comm_udp_recv_msg,
	.recv_err = comm_udp_recv_err,
	.link_state = comm_udp_link_state
};

static void comm_udp_recv_msg(udp_assoc_t *assoc, udp_rmsg_t *rmsg)
{
	size_t size;
	size_t pos;
	size_t now;
	int rc;

	size = udp_rmsg_size(rmsg);
	pos = 0;
	while (pos < size) {
		now = min(size - pos, RECV_BUF_SIZE);
		rc = udp_rmsg_read(rmsg, pos, recv_buf, now);
		if (rc != EOK) {
			printf("Error reading message.\n");
			return;
		}

		netecho_received(recv_buf, now);
		pos += now;
	}
}

static void comm_udp_recv_err(udp_assoc_t *assoc, udp_rerr_t *rerr)
{
	printf("Got ICMP error message.\n");
}

static void comm_udp_link_state(udp_assoc_t *assoc, udp_link_state_t lstate)
{
	const char *sstate = NULL;

	switch (lstate) {
	case udp_ls_down:
		sstate = "Down";
		break;
	case udp_ls_up:
		sstate = "Up";
		break;
	}

	printf("Link state change: %s.\n", sstate);
}

int comm_open(const char *host, const char *port_s)
{
	inet_ep2_t epp;
	inet_addr_t iaddr;
	int rc;

	if (host != NULL) {
		/* Interpret as address */
		inet_addr_t iaddr;
		int rc = inet_addr_parse(host, &iaddr);

		if (rc != EOK) {
			/* Interpret as a host name */
			dnsr_hostinfo_t *hinfo = NULL;
			rc = dnsr_name2host(host, &hinfo, ip_any);

			if (rc != EOK) {
				printf("Error resolving host '%s'.\n", host);
				goto error;
			}

			iaddr = hinfo->addr;
		}
	}

	char *endptr;
	uint16_t port = strtol(port_s, &endptr, 10);
	if (*endptr != '\0') {
		printf("Invalid port number %s\n", port_s);
		goto error;
	}

	inet_ep2_init(&epp);
	if (host != NULL) {
		/* Talk to remote host */
		remote.addr = iaddr;
		remote.port = port;

		printf("Talking to host %s port %u\n", host, port);
	} else {
		/* Listen on local port */
		epp.local.port = port;

		printf("Listening on port %u\n", port);
	}

	rc = udp_create(&udp);
	if (rc != EOK)
		goto error;

	rc = udp_assoc_create(udp, &epp, &comm_udp_cb, NULL, &assoc);
	if (rc != EOK)
		goto error;

	return EOK;
error:
	udp_assoc_destroy(assoc);
	udp_destroy(udp);

	return EIO;
}

void comm_close(void)
{
	if (assoc != NULL)
		udp_assoc_destroy(assoc);
	if (udp != NULL)
		udp_destroy(udp);
}

int comm_send(void *data, size_t size)
{
	int rc = udp_assoc_send_msg(assoc, &remote, data, size);
	if (rc != EOK)
		return EIO;

	return EOK;
}

/** @}
 */
