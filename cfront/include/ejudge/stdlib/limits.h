/* -*- c -*- */
/* $Id$ */

#ifndef __RCC_LIMITS_H__
#define __RCC_LIMITS_H__

/* Copyright (C) 2002-2004 Alexander Chernov */

/*
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 */

#include <features.h>

#include <linux/limits.h>

int enum { MB_LEN_MAX = 16 };
#define MB_LEN_MAX MB_LEN_MAX

/* Minimum and maximum values a `char' can hold.  */
#ifdef __CHAR_UNSIGNED__
int enum
{
#defconst CHAR_MIN 0
#defconst CHAR_MAX 255
};
#else
int enum
{
#defconst CHAR_MIN (-128)
#defconst CHAR_MAX 127
};
#endif

int enum
{
#defconst SCHAR_MIN (-128)
#defconst SCHAR_MAX 127
#defconst UCHAR_MAX 255
};

int enum
{
#defconst SHRT_MIN (-32768)
#defconst SHRT_MAX 32767
#defconst USHRT_MAX 65535
};

int enum
{
#defconst INT_MAX 2147483647
#defconst INT_MIN (-INT_MAX - 1)
};

unsigned int enum
{
#defconst UINT_MAX 4294967295U
};

long enum
{
#defconst LONG_MAX 2147483647L
#defconst LONG_MIN (-LONG_MAX - 1L)
};

unsigned long enum
{
#defconst ULONG_MAX 4294967295U
};

long long enum
{
#defconst LONG_LONG_MAX 9223372036854775807LL
#defconst LONG_LONG_MIN (-LONG_LONG_MAX - 1LL)
};

unsigned long long enum
{
#defconst ULONG_LONG_MAX 18446744073709551615ULL
};

#define LLONG_MIN LONG_LONG_MIN
#define LLONG_MAX LONG_LONG_MAX
#define ULLONG_MAX ULONG_LONG_MAX

int enum
{
  _POSIX_AIO_LISTIO_MAX = 2,
#define _POSIX_AIO_LISTIO_MAX _POSIX_AIO_LISTIO_MAX
  _POSIX_AIO_MAX = 1,
#define _POSIX_AIO_MAX _POSIX_AIO_MAX
  _POSIX_ARG_MAX = 4096,
#define _POSIX_ARG_MAX _POSIX_ARG_MAX
  _POSIX_CHILD_MAX = 6,
#define _POSIX_CHILD_MAX _POSIX_CHILD_MAX
  _POSIX_DELAYTIMER_MAX = 32,
#define _POSIX_DELAYTIMER_MAX _POSIX_DELAYTIMER_MAX
  _POSIX_LINK_MAX = 8,
#define _POSIX_LINK_MAX _POSIX_LINK_MAX
  _POSIX_MAX_CANON = 255,
#define _POSIX_MAX_CANON _POSIX_MAX_CANON
  _POSIX_MAX_INPUT = 255,
#define _POSIX_MAX_INPUT _POSIX_MAX_INPUT
  _POSIX_MQ_OPEN_MAX = 8,
#define _POSIX_MQ_OPEN_MAX _POSIX_MQ_OPEN_MAX
  _POSIX_MQ_PRIO_MAX = 32,
#define _POSIX_MQ_PRIO_MAX _POSIX_MQ_PRIO_MAX
  _POSIX_NGROUPS_MAX = 0,
#define _POSIX_NGROUPS_MAX _POSIX_NGROUPS_MAX
  _POSIX_OPEN_MAX = 16,
#define _POSIX_OPEN_MAX _POSIX_OPEN_MAX
  _POSIX_FD_SETSIZE = _POSIX_OPEN_MAX,
#define _POSIX_FD_SETSIZE _POSIX_FD_SETSIZE
  _POSIX_NAME_MAX = 14,
#define _POSIX_NAME_MAX _POSIX_NAME_MAX
  _POSIX_PATH_MAX = 256,
#define _POSIX_PATH_MAX _POSIX_PATH_MAX
  _POSIX_PIPE_BUF = 512,
#define _POSIX_PIPE_BUF _POSIX_PIPE_BUF
  _POSIX_RTSIG_MAX = 8,
#define _POSIX_RTSIG_MAX _POSIX_RTSIG_MAX
  _POSIX_SEM_NSEMS_MAX = 256,
#define _POSIX_SEM_NSEMS_MAX _POSIX_SEM_NSEMS_MAX
  _POSIX_SEM_VALUE_MAX = 32767,
#define _POSIX_SEM_VALUE_MAX _POSIX_SEM_VALUE_MAX
  _POSIX_SIGQUEUE_MAX = 32,
#define _POSIX_SIGQUEUE_MAX _POSIX_SIGQUEUE_MAX
  _POSIX_SSIZE_MAX = 32767,
#define _POSIX_SSIZE_MAX _POSIX_SSIZE_MAX
  _POSIX_STREAM_MAX = 8,
#define _POSIX_STREAM_MAX _POSIX_STREAM_MAX
  _POSIX_TZNAME_MAX = 6,
#define _POSIX_TZNAME_MAX _POSIX_TZNAME_MAX
  _POSIX_QLIMIT = 1,
#define _POSIX_QLIMIT _POSIX_QLIMIT
  _POSIX_HIWAT = _POSIX_PIPE_BUF,
#define _POSIX_HIWAT _POSIX_HIWAT
  _POSIX_UIO_MAXIOV = 16,
#define _POSIX_UIO_MAXIOV _POSIX_UIO_MAXIOV
  _POSIX_TTY_NAME_MAX = 9,
#define _POSIX_TTY_NAME_MAX _POSIX_TTY_NAME_MAX
  _POSIX_TIMER_MAX = 32,
#define _POSIX_TIMER_MAX _POSIX_TIMER_MAX
  _POSIX_LOGIN_NAME_MAX = 9,
#define _POSIX_LOGIN_NAME_MAX _POSIX_LOGIN_NAME_MAX
  _POSIX_CLOCKRES_MIN = 20000000,
#define _POSIX_CLOCKRES_MIN _POSIX_CLOCKRES_MIN
};

int enum
{
  _POSIX_THREAD_KEYS_MAX = 128,
#define _POSIX_THREAD_KEYS_MAX _POSIX_THREAD_KEYS_MAX
  PTHREAD_KEYS_MAX = 1024,
#define PTHREAD_KEYS_MAX PTHREAD_KEYS_MAX
  _POSIX_THREAD_DESTRUCTOR_ITERATIONS = 4,
#define _POSIX_THREAD_DESTRUCTOR_ITERATIONS _POSIX_THREAD_DESTRUCTOR_ITERATIONS
  PTHREAD_DESTRUCTOR_ITERATIONS = _POSIX_THREAD_DESTRUCTOR_ITERATIONS,
#define PTHREAD_DESTRUCTOR_ITERATIONS PTHREAD_DESTRUCTOR_ITERATIONS
  _POSIX_THREAD_THREADS_MAX = 64,
#define _POSIX_THREAD_THREADS_MAX _POSIX_THREAD_THREADS_MAX
  PTHREAD_THREADS_MAX = 16384,
#define PTHREAD_THREADS_MAX PTHREAD_THREADS_MAX
  AIO_PRIO_DELTA_MAX = 20,
#define AIO_PRIO_DELTA_MAX AIO_PRIO_DELTA_MAX
  PTHREAD_STACK_MIN = 16384,
#define PTHREAD_STACK_MIN PTHREAD_STACK_MIN
  TIMER_MAX = 256,
#define TIMER_MAX TIMER_MAX
  DELAYTIMER_MAX = 2147483647,
#define DELAYTIMER_MAX DELAYTIMER_MAX
  TTY_NAME_MAX = 32,
#define TTY_NAME_MAX TTY_NAME_MAX
  LOGIN_NAME_MAX = 256,
#define LOGIN_NAME_MAX LOGIN_NAME_MAX
};

#ifndef SSIZE_MAX
int enum { SSIZE_MAX = LONG_MAX };
#define SSIZE_MAX SSIZE_MAX
#endif

#ifndef NGROUPS_MAX
int enum { NGROUPS_MAX = _POSIX_NGROUPS_MAX };
#define NGROUPS_MAX NGROUPS_MAX
#endif

int enum
{
  _POSIX2_BC_BASE_MAX = 99,
#define _POSIX2_BC_BASE_MAX _POSIX2_BC_BASE_MAX
  _POSIX2_BC_DIM_MAX = 2048,
#define _POSIX2_BC_DIM_MAX _POSIX2_BC_DIM_MAX
  _POSIX2_BC_SCALE_MAX = 99,
#define _POSIX2_BC_SCALE_MAX _POSIX2_BC_SCALE_MAX
  _POSIX2_BC_STRING_MAX = 1000,
#define _POSIX2_BC_STRING_MAX _POSIX2_BC_STRING_MAX
  _POSIX2_COLL_WEIGHTS_MAX = 2,
#define _POSIX2_COLL_WEIGHTS_MAX _POSIX2_COLL_WEIGHTS_MAX
  _POSIX2_EXPR_NEST_MAX = 32,
#define _POSIX2_EXPR_NEST_MAX _POSIX2_EXPR_NEST_MAX
  _POSIX2_LINE_MAX = 2048,
#define _POSIX2_LINE_MAX _POSIX2_LINE_MAX
  _POSIX2_RE_DUP_MAX = 255,
#define _POSIX2_RE_DUP_MAX _POSIX2_RE_DUP_MAX
  _POSIX2_CHARCLASS_NAME_MAX = 14,
#define _POSIX2_CHARCLASS_NAME_MAX _POSIX2_CHARCLASS_NAME_MAX
};

#ifndef BC_BASE_MAX
int enum { BC_BASE_MAX = _POSIX2_BC_BASE_MAX };
#define BC_BASE_MAX BC_BASE_MAX
#endif

#ifndef BC_DIM_MAX
int enum { BC_DIM_MAX = _POSIX2_BC_DIM_MAX };
#define BC_DIM_MAX BC_DIM_MAX
#endif

#ifndef BC_SCALE_MAX
int enum { BC_SCALE_MAX = _POSIX2_BC_SCALE_MAX };
#define BC_SCALE_MAX BC_SCALE_MAX
#endif

#ifndef BC_STRING_MAX
int enum { BC_STRING_MAX = _POSIX2_BC_STRING_MAX };
#define BC_STRING_MAX BC_STRING_MAX
#endif

#ifndef COLL_WEIGHTS_MAX
int enum { COLL_WEIGHTS_MAX = 255 };
#define COLL_WEIGHTS_MAX COLL_WEIGHTS_MAX
#endif

#ifndef EXPR_NEST_MAX
int enum { EXPR_NEST_MAX = _POSIX2_EXPR_NEST_MAX };
#define EXPR_NEST_MAX EXPR_NEST_MAX
#endif

#ifndef LINE_MAX
int enum { LINE_MAX = _POSIX2_LINE_MAX };
#define LINE_MAX LINE_MAX
#endif

#ifndef CHARCLASS_NAME_MAX
int enum { CHARCLASS_NAME_MAX = 2048 };
#define CHARCLASS_NAME_MAX CHARCLASS_NAME_MAX
#endif

#ifdef RE_DUP_MAX
#undef RE_DUP_MAX
#endif
#define RE_DUP_MAX (0x7fff)

int enum
{
  L_tmpnam = 20,
#define L_tmpnam L_tmpnam
  TMP_MAX = 238328,
#define TMP_MAX TMP_MAX
#ifndef FILENAME_MAX
#defconst FILENAME_MAX 4096
#endif /* FILENAME_MAX is defined */
  L_ctermid = 9,
#define L_ctermid L_ctermid
  L_cuserid = 9,
#define L_cuserid L_cuserid
  FOPEN_MAX = 16,
#define FOPEN_MAX FOPEN_MAX
  IOV_MAX = 1024,
#define IOV_MAX IOV_MAX
  _XOPEN_IOV_MAX = _POSIX_UIO_MAXIOV,
#define _XOPEN_IOV_MAX _XOPEN_IOV_MAX
  NL_ARGMAX = _POSIX_ARG_MAX,
#define NL_ARGMAX NL_ARGMAX
  NL_LANGMAX = _POSIX2_LINE_MAX,
#define NL_LANGMAX NL_LANGMAX
  NL_MSGMAX = INT_MAX,
#define NL_MSGMAX NL_MSGMAX
  NL_NMAX = INT_MAX,
#define NL_NMAX NL_NMAX
  NL_SETMAX = INT_MAX,
#define NL_SETMAX NL_SETMAX
  NL_TEXTMAX = INT_MAX,
#define NL_TEXTMAX NL_TEXTMAX
  NZERO = 20,
#define NZERO NZERO
};

int enum
{
#defconst CHAR_BIT      8
#defconst SHORT_BIT     16
#defconst INT_BIT       16
#defconst WORD_BIT      32
#defconst LONG_BIT      32
#defconst LONG_LONG_BIT 64
#defconst __WORDSIZE    32
};

#endif /* __RCC_LIMITS_H__ */