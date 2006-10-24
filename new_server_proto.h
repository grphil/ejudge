/* -*- c -*- */
/* $Id$ */

#ifndef __NEW_SERVER_PROTO_H__
#define __NEW_SERVER_PROTO_H__

/* Copyright (C) 2006 Alexander Chernov <cher@ejudge.ru> */

/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "ej_types.h"

#include <stdio.h>
#include <time.h>

#define NEW_SERVER_PROT_PACKET_MAGIC (0xe352)
struct new_server_prot_packet
{
  unsigned short magic;
  short id;
};

// client->serve requests
enum
{
  NEW_SRV_CMD_PASS_FD = 1,
  NEW_SRV_CMD_HTTP_REQUEST,

  NEW_SRV_CMD_LAST,
};

// serve->client replies
enum
{
  NEW_SRV_RPL_OK = 0,

  NEW_SRV_RPL_LAST,
};

// serve error message codes
enum
{
  NEW_SRV_ERR_NO_ERROR = 0,
  NEW_SRV_ERR_UNKNOWN_ERROR,
  NEW_SRV_ERR_BAD_SOCKET_NAME,
  NEW_SRV_ERR_SYSTEM_ERROR,
  NEW_SRV_ERR_CONNECT_FAILED,
  NEW_SRV_ERR_WRITE_ERROR,
  NEW_SRV_ERR_NOT_CONNECTED,
  NEW_SRV_ERR_READ_ERROR,
  NEW_SRV_ERR_UNEXPECTED_EOF,
  NEW_SRV_ERR_PACKET_TOO_BIG,
  NEW_SRV_ERR_PACKET_TOO_SMALL,
  NEW_SRV_ERR_PROTOCOL_ERROR,
  NEW_SRV_ERR_PARAM_OUT_OF_RANGE,

  NEW_SRV_ERR_INV_USER_ID,
  NEW_SRV_ERR_REGISTRATION_FAILED,
  NEW_SRV_ERR_USER_REMOVAL_FAILED,
  NEW_SRV_ERR_USER_STATUS_CHANGE_FAILED,
  NEW_SRV_ERR_USER_FLAGS_CHANGE_FAILED,
  NEW_SRV_ERR_INV_USER_LOGIN,
  NEW_SRV_ERR_USER_LOGIN_NONEXISTANT,
  NEW_SRV_ERR_PRIV_USER_REMOVAL_FAILED,
  NEW_SRV_ERR_PRIV_USER_ROLE_ADD_FAILED,
  NEW_SRV_ERR_PRIV_USER_ROLE_DEL_FAILED,
  NEW_SRV_ERR_INV_USER_ROLE,
  NEW_SRV_ERR_INV_TIME_SPEC,
  NEW_SRV_ERR_CONTEST_ALREADY_FINISHED,
  NEW_SRV_ERR_CONTEST_ALREADY_STARTED,
  NEW_SRV_ERR_INV_DUR_SPEC,
  NEW_SRV_ERR_DUR_TOO_SMALL,
  NEW_SRV_ERR_PERMISSION_DENIED,
  NEW_SRV_ERR_CONTEST_NOT_STARTED,
  NEW_SRV_ERR_CANNOT_CONTINUE_CONTEST,
  NEW_SRV_ERR_CONTEST_NOT_FINISHED,
  NEW_SRV_ERR_INSUFFICIENT_DURATION,
  NEW_SRV_ERR_INV_LOCALE_ID,
  NEW_SRV_ERR_SESSION_UPDATE_FAILED,
  NEW_SRV_ERR_LANG_DISABLED,
  NEW_SRV_ERR_LANG_NOT_AVAIL_FOR_PROBLEM,
  NEW_SRV_ERR_LANG_DISABLED_FOR_PROBLEM,
  NEW_SRV_ERR_CANNOT_DETECT_CONTENT_TYPE,
  NEW_SRV_ERR_CONTENT_TYPE_NOT_AVAILABLE,
  NEW_SRV_ERR_CONTENT_TYPE_DISABLED,
  NEW_SRV_ERR_RUNLOG_UPDATE_FAILED,
  NEW_SRV_ERR_DISK_WRITE_ERROR,
  NEW_SRV_ERR_USER_ID_NONEXISTANT,
  NEW_SRV_ERR_CONFLICTING_USER_ID_LOGIN,
  NEW_SRV_ERR_SUBJECT_TOO_LONG,
  NEW_SRV_ERR_SUBJECT_EMPTY,
  NEW_SRV_ERR_MESSAGE_TOO_LONG,
  NEW_SRV_ERR_MESSAGE_EMPTY,
  NEW_SRV_ERR_CLARLOG_UPDATE_FAILED,
  NEW_SRV_ERR_INV_CLAR_ID,
  NEW_SRV_ERR_CANNOT_REPLY_TO_JUDGE,
  NEW_SRV_ERR_DISK_READ_ERROR,
  NEW_SRV_ERR_INV_STATUS,
  NEW_SRV_ERR_NO_RUNS_TO_REJUDGE,
  NEW_SRV_ERR_RUN_TO_COMPARE_UNSPECIFIED,
  NEW_SRV_ERR_INV_RUN_TO_COMPARE,
  NEW_SRV_ERR_RUN_COMPARE_FAILED,
  NEW_SRV_ERR_INV_PROB_ID,
  NEW_SRV_ERR_SOURCE_UNAVAILABLE,
  NEW_SRV_ERR_SOURCE_NONEXISTANT,
  NEW_SRV_ERR_INV_LANG_ID,
  NEW_SRV_ERR_INV_TEST,
  NEW_SRV_ERR_OLD_PWD_TOO_LONG,
  NEW_SRV_ERR_NEW_PWD_MISMATCH,
  NEW_SRV_ERR_NEW_PWD_TOO_LONG,
  NEW_SRV_ERR_PWD_UPDATE_FAILED,
  NEW_SRV_ERR_RUN_ID_UNDEFINED,
  NEW_SRV_ERR_INV_RUN_ID,
  NEW_SRV_ERR_RUN_ID_OUT_OF_RANGE,
  NEW_SRV_ERR_RUNLOG_READ_FAILED,
  NEW_SRV_ERR_PRINTING_DISABLED,
  NEW_SRV_ERR_ALREADY_PRINTED,
  NEW_SRV_ERR_PRINT_QUOTA_EXCEEDED,
  NEW_SRV_ERR_PRINTING_FAILED,
  NEW_SRV_ERR_CLIENTS_SUSPENDED,
  NEW_SRV_ERR_RUN_QUOTA_EXCEEDED,
  NEW_SRV_ERR_PROB_UNAVAILABLE,
  NEW_SRV_ERR_PROB_DEADLINE_EXPIRED,
  NEW_SRV_ERR_VARIANT_UNASSIGNED,
  NEW_SRV_ERR_DUPLICATE_SUBMIT,
  NEW_SRV_ERR_PROB_ALREADY_SOLVED,
  NEW_SRV_ERR_NOT_ALL_REQ_SOLVED,
  NEW_SRV_ERR_CLARS_DISABLED,
  NEW_SRV_ERR_CLAR_QUOTA_EXCEEDED,
  NEW_SRV_ERR_SOURCE_VIEW_DISABLED,
  NEW_SRV_ERR_REPORT_UNAVAILABLE,
  NEW_SRV_ERR_REPORT_VIEW_DISABLED,
  NEW_SRV_ERR_REPORT_NONEXISTANT,
  NEW_SRV_ERR_TEST_NONEXISTANT,
  NEW_SRV_ERR_CHECKSUMMING_FAILED,
  NEW_SRV_ERR_OUTPUT_ERROR,
  NEW_SRV_ERR_TEST_UNAVAILABLE,
  NEW_SRV_ERR_INV_VARIANT,
  NEW_SRV_ERR_PWD_GENERATION_FAILED,
  NEW_SRV_ERR_TEAM_PWD_DISABLED,
  NEW_SRV_ERR_APPEALS_DISABLED,
  NEW_SRV_ERR_APPEALS_FINISHED,

  NEW_SRV_ERR_LAST,
};

struct new_server_prot_http_request
{
  struct new_server_prot_packet b;
  int arg_num;
  int env_num;
  int param_num;
};

const unsigned char *ns_proto_strerror(int n);
void ns_error(FILE *log_f, int code, ...);
const unsigned char *ns_strerror(int code, ...);
const unsigned char *ns_strerror_r(unsigned char *buf, size_t size,
                                   int code, ...);

#endif /* __NEW_SERVER_PROTO_H__ */
