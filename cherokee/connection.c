/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* Cherokee
 *
 * Authors:
 *      Alvaro Lopez Ortega <alvaro@alobbs.com>
 *
 * Copyright (C) 2001-2011 Alvaro Lopez Ortega
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of version 2 of the GNU General Public
 * License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

#include "common-internal.h"
#include "connection.h"


ret_t
cherokee_connection_init (cherokee_connection_t *conn)
{
	INIT_LIST_HEAD (&conn->list_entry);
	INIT_LIST_HEAD (&conn->requests);

	cherokee_socket_init (&conn->socket);
	cherokee_protocol_init (&conn->protocol, conn);
	cherokee_connection_poll_init (&conn->polling_aim);

	conn->timeout        = -1;
	conn->timeout_lapse  = -1;
	conn->timeout_header = NULL;
	conn->keepalive      = 0;

	conn->rx             = 0;
	conn->tx             = 0;
	conn->rx_partial     = 0;
	conn->tx_partial     = 0;
	conn->traffic_next   = 0;

	cherokee_buffer_init (&conn->buffer_in);
	cherokee_buffer_init (&conn->buffer_out);

	return ret_ok;
}


static void
reuse_requests (cherokee_connection_t *conn)
{
	cherokee_thread_t *thd = CONN_THREAD(conn);
	cherokee_list_t   *i, *tmp;

	list_for_each_safe (i, tmp, &conn->requests) {
		if (thd != NULL) {
			cherokee_thread_recycle_request (thd, REQ(i));
		} else {
			cherokee_request_free (REQ(i));
		}
	}
}


ret_t
cherokee_connection_mrproper (cherokee_connection_t *conn)
{
	/* Polling aim */
	if (conn->polling_aim.fd != -1) {
                cherokee_fd_close (conn->polling_aim.fd);
	}

	cherokee_connection_poll_mrproper (&conn->polling_aim);

	/* Properties */
	cherokee_socket_mrproper (&conn->socket);
	cherokee_protocol_mrproper (&conn->protocol);

	/* Free requests */
	reuse_requests (conn);

	/* Buffers */
	cherokee_buffer_mrproper (&conn->buffer_in);
	cherokee_buffer_mrproper (&conn->buffer_out);

	return ret_ok;
}


ret_t
cherokee_connection_clean (cherokee_connection_t *conn)
{
	cherokee_list_t *i, *tmp;

	/* Item of a list */
	INIT_LIST_ENTRY (&conn->list_entry);

	/* Polling aim */
	if (conn->polling_aim.fd != -1) {
                cherokee_fd_close (conn->polling_aim.fd);
	}

	cherokee_connection_poll_clean (&conn->polling_aim);

	/* Socket */
	cherokee_socket_close (&conn->socket);
	cherokee_socket_clean (&conn->socket);

	/* Free requests */
	reuse_requests (conn);

	/* Timeout */
	conn->timeout        = -1;
	conn->timeout_lapse  = -1;
	conn->timeout_header = NULL;

	/* Traffic */
	conn->rx             = 0;
	conn->tx             = 0;
	conn->rx_partial     = 0;
	conn->tx_partial     = 0;
	conn->traffic_next   = 0;

	/* Buffers */
	cherokee_buffer_clean (&conn->buffer_in);
	cherokee_buffer_clean (&conn->buffer_out);

	/* Protocol */
	cherokee_protocol_clean (&conn->protocol);

	return ret_ok;
}


void
cherokee_connection_rx_add (cherokee_connection_t *conn, ssize_t rx)
{
	if (likely (rx > 0)) {
		conn->rx += rx;
		conn->rx_partial += rx;
	}
}


void
cherokee_connection_tx_add (cherokee_connection_t *conn, ssize_t tx)
{
	cuint_t to_sleep;

	/* Count it
	 */
	conn->tx += tx;
	conn->tx_partial += tx;

	/* Traffic shaping
	 */
	if (conn->limit_rate) {
		to_sleep = (tx * 1000) / conn->limit_bps;

		/* It's meant to sleep for a while */
		conn->limit_blocked_until = cherokee_bogonow_msec + to_sleep;
	}
}
