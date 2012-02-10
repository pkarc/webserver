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

#ifndef CHEROKEE_REQUEST_PROTECTED_H
#define CHEROKEE_REQUEST_PROTECTED_H

#include "common-internal.h"

#ifdef HAVE_SYS_TIME_H
# include <sys/time.h>
#else
# include <time.h>
#endif

#ifdef HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif

#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif

#ifdef HAVE_NETINET_IN_H
# include <netinet/in.h>
#endif

#ifdef HAVE_ARPA_INET_H
# include <arpa/inet.h>
#endif

#ifdef HAVE_SYS_UIO_H
# include <sys/uio.h>
#endif

#include "http.h"
#include "list.h"
#include "avl.h"
#include "socket.h"
#include "header.h"
#include "logger.h"
#include "handler.h"
#include "encoder.h"
#include "iocache.h"
#include "post.h"
#include "header-protected.h"
#include "regex.h"
#include "bind.h"
#include "bogotime.h"
#include "config_entry.h"
#include "connection-poll.h"

typedef enum {
	phase_nothing,
	phase_tls_handshake,
	phase_reading_header,
	phase_processing_header,
	phase_setup_connection,
	phase_init,
	phase_reading_post,
	phase_add_headers,
	phase_send_headers,
	phase_stepping,
	phase_shutdown,
	phase_lingering
} cherokee_request_phase_t;


#define conn_op_nothing            0
#define conn_op_root_index        (1 << 1)
#define conn_op_tcp_cork          (1 << 2)
#define conn_op_document_root     (1 << 3)
#define conn_op_cant_encoder      (1 << 4)
#define conn_op_got_eof           (1 << 5)
#define conn_op_chunked_formatted (1 << 6)

typedef cuint_t cherokee_request_options_t;


struct cherokee_request {
	cherokee_list_t               list_node;

	/* References
	 */
	void                         *server;
	void                         *vserver;
	void                         *thread;
	void                         *conn;
	cherokee_bind_t              *bind;
	cherokee_config_entry_ref_t   config_entry;

	/* ID
	 */
	culong_t                      id;
	cherokee_buffer_t             self_trace;

	/* Socket stuff
	 */
	cherokee_http_upgrade_t       upgrade;
	cherokee_request_options_t    options;
	cherokee_handler_t           *handler;

	cherokee_logger_t            *logger_ref;
	cherokee_buffer_t             logger_real_ip;

	/* Buffers
	 */
	/* cherokee_buffer_t             header_buffer;    /\* <- header, -> post data *\/ */
	/* cherokee_buffer_t             buffer;           /\* <- data                 *\/ */

	/* State
	 */
	cherokee_request_phase_t      phase;
	cherokee_http_t               error_code;
	cherokee_buffer_t             error_internal_url;
	cherokee_buffer_t             error_internal_qs;
	cherokee_http_t               error_internal_code;

	/* Headers
	 */
	cherokee_header_t             header;
	cherokee_buffer_t             header_buffer_in;
	cherokee_buffer_t             header_buffer_out;

	/* Encoders
	 */
	encoder_func_new_t            encoder_new_func;
	cherokee_encoder_props_t     *encoder_props;
	cherokee_encoder_t           *encoder;
	cherokee_buffer_t             encoder_buffer;

	/* Eg:
	 * http://www.alobbs.com/cherokee/dir/file/param1
	 */
	cherokee_buffer_t             local_directory;     /* /var/www/  or  /home/alo/public_html/ */
	cherokee_buffer_t             web_directory;       /* /cherokee/ */
	cherokee_buffer_t             request;             /* /dir/file */
	cherokee_buffer_t             pathinfo;            /* /param1 */
	cherokee_buffer_t             userdir;             /* 'alo' in http://www.alobbs.com/~alo/thing */
	cherokee_buffer_t             query_string;
	cherokee_avl_t               *arguments;

	cherokee_buffer_t             host;
	cherokee_buffer_t             host_port;
	cherokee_buffer_t             effective_directory;
	cherokee_buffer_t             request_original;
	cherokee_buffer_t             query_string_original;

	/* Authentication
	 */
	cherokee_validator_t         *validator;           /* Validator object */
	cherokee_http_auth_t          auth_type;           /* Auth type of the resource */
	cherokee_http_auth_t          req_auth_type;       /* Auth type of the request  */

	/* Post info
	 */
	cherokee_post_t               post;

	/* Polling
	 */

	off_t                         range_start;
	off_t                         range_end;

	void                         *mmaped;
	off_t                         mmaped_len;
	cherokee_iocache_entry_t     *io_entry_ref;

	/* Front-line cache
	 */
	cherokee_flcache_conn_t       flcache;

	/* Regular expressions
	 */
	int                           regex_ovector[OVECTOR_LEN];
	int                           regex_ovecsize;
	int                           regex_host_ovector[OVECTOR_LEN];
	int                           regex_host_ovecsize;

	/* Content Expiration
	 */
	cherokee_expiration_t         expiration;
	time_t                        expiration_time;
	cherokee_expiration_props_t   expiration_prop;

	/* Chunked encoding
	 */
	cherokee_boolean_t            chunked_encoding;
	cherokee_boolean_t            chunked_last_package;
	cherokee_buffer_t             chunked_len;
	size_t                        chunked_sent;
	struct iovec                  chunks[3];
	uint16_t                      chunksn;

	/* Redirections
	 */
	cherokee_buffer_t             redirect;
	cuint_t                       respins;

	/* Traffic-shaping
	 */
	cherokee_boolean_t            limit_rate;
	cuint_t                       limit_bps;
	cherokee_msec_t               limit_blocked_until;
};

#define REQ_CONN(r)   (CONN(REQ(r)->conn))
#define REQ_SRV(r)    (SRV(REQ(r)->server))
#define REQ_HDR(r)    (HDR(REQ(r)->header))
#define REQ_SOCK(r)   (SOCKET(REQ(r)->socket))
#define REQ_VSRV(r)   (VSERVER(REQ(r)->vserver))
#define REQ_THREAD(r) (THREAD(REQ(r)->thread))
#define REQ_BIND(r)   (BIND(REQ(r)->bind))

#define TRACE_REQ(c)  TRACE("conn", "%s", cherokee_request_print(c));

/* Basic functions
 */
ret_t cherokee_request_new                    (cherokee_request_t **conn);
ret_t cherokee_request_free                   (cherokee_request_t  *conn);
ret_t cherokee_request_clean                  (cherokee_request_t  *conn, cherokee_boolean_t reuse);
ret_t cherokee_request_clean_close            (cherokee_request_t  *conn);

/* Close
 */
ret_t cherokee_request_shutdown_wr            (cherokee_request_t *conn);
ret_t cherokee_request_linger_read            (cherokee_request_t *conn);

/* Connection I/O
 */
ret_t cherokee_request_send                   (cherokee_request_t *conn);
ret_t cherokee_request_send_header            (cherokee_request_t *conn);
ret_t cherokee_request_send_header_and_mmaped (cherokee_request_t *conn);
ret_t cherokee_request_recv                   (cherokee_request_t *conn, cherokee_buffer_t *buffer, off_t to_read, off_t *len);

/* Internal
 */
ret_t cherokee_request_create_handler         (cherokee_request_t *conn, cherokee_config_entry_t *config_entry);
ret_t cherokee_request_create_encoder         (cherokee_request_t *conn, cherokee_avl_t *accept_enc);
ret_t cherokee_request_setup_error_handler    (cherokee_request_t *conn);
ret_t cherokee_request_setup_hsts_handler     (cherokee_request_t *conn);
ret_t cherokee_request_check_authentication   (cherokee_request_t *conn, cherokee_config_entry_t *config_entry);
ret_t cherokee_request_check_ip_validation    (cherokee_request_t *conn, cherokee_config_entry_t *config_entry);
ret_t cherokee_request_check_only_secure      (cherokee_request_t *conn, cherokee_config_entry_t *config_entry);
ret_t cherokee_request_check_http_method      (cherokee_request_t *conn, cherokee_config_entry_t *config_entry);
ret_t cherokee_request_set_rate               (cherokee_request_t *conn, cherokee_config_entry_t *config_entry);
void  cherokee_request_set_keepalive          (cherokee_request_t *conn);
void  cherokee_request_set_chunked_encoding   (cherokee_request_t *conn);
int   cherokee_request_should_include_length  (cherokee_request_t *conn);
ret_t cherokee_request_instance_encoder       (cherokee_request_t *conn);
ret_t cherokee_request_sleep                  (cherokee_request_t *conn, cherokee_msec_t msecs);
void  cherokee_request_update_timeout         (cherokee_request_t *conn);
void  cherokee_request_add_expiration_header  (cherokee_request_t *conn, cherokee_buffer_t *buffer, cherokee_boolean_t use_maxage);
ret_t cherokee_request_build_host_string      (cherokee_request_t *conn, cherokee_buffer_t *buf);
ret_t cherokee_request_build_host_port_string (cherokee_request_t *conn, cherokee_buffer_t *buf);

/* Iteration
 */
ret_t cherokee_request_open_request           (cherokee_request_t *conn);
ret_t cherokee_request_reading_check          (cherokee_request_t *conn);
ret_t cherokee_request_step                   (cherokee_request_t *conn);

/* Headers
 */
ret_t cherokee_request_read_post              (cherokee_request_t *conn);
ret_t cherokee_request_build_header           (cherokee_request_t *conn);
ret_t cherokee_request_get_request            (cherokee_request_t *conn);
ret_t cherokee_request_parse_range            (cherokee_request_t *conn);
int   cherokee_request_is_userdir             (cherokee_request_t *conn);

ret_t cherokee_request_build_local_directory  (cherokee_request_t *conn, cherokee_virtual_server_t *vsrv);
ret_t cherokee_request_build_local_directory_userdir (cherokee_request_t *conn, cherokee_virtual_server_t *vsrv);
ret_t cherokee_request_set_custom_droot       (cherokee_request_t   *conn, cherokee_config_entry_t *entry);

ret_t cherokee_request_clean_error_headers    (cherokee_request_t *conn);
ret_t cherokee_request_set_redirect           (cherokee_request_t *conn, cherokee_buffer_t *address);

ret_t cherokee_request_clean_for_respin       (cherokee_request_t *conn);
int   cherokee_request_use_webdir             (cherokee_request_t *conn);

/* Log
 */
ret_t cherokee_request_log                    (cherokee_request_t *conn);
ret_t cherokee_request_update_vhost_traffic   (cherokee_request_t *conn);
char *cherokee_request_print                  (cherokee_request_t *conn);

#endif /* CHEROKEE_REQUEST_PROTECTED_H */