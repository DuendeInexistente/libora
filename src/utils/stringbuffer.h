/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * Copyright (C) Luka Cehovin 2010 <luka@tnode.com>
 * 
 * libora is free software.
 * 
 * You may redistribute it and/or modify it under the terms of the
 * GNU Lesser General Public License, as published by the Free 
 * Software Foundation; either version 2 of the License, 
 * or (at your option) any later version.
 * 
 * main.c is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public 
 * License along with the source. If not, write to:
 * The Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor
 * Boston, MA  02110-1301, USA.
 */


#ifndef _STRINGBUFFER_H
#define _STRINGBUFFER_H

#include <stdarg.h>

#define _SB_BLOCK_LENGTH 64
#define _SB_FORMATTER_LENGTH 256

typedef struct _sb_block
{
  char data[_SB_BLOCK_LENGTH];
  struct _sb_block* next;
} sb_block;

typedef struct _sb_buffer
{
  sb_block *first, *last;
  int cnt;
  int numChunks;
} sb_buffer;

/* Create a new string buffer object */
extern sb_buffer* sb_create();

/* Append a string to string buffer */
extern void sb_print(sb_buffer* sb, const char*);

extern void sb_printf(sb_buffer* sb, const char*, ...)
#ifdef __GNUC__
 __attribute__ ((format (printf, 2, 3)))
#endif
;

/* Retreive string buffer content as a string */
extern char* sb_string(sb_buffer* buf);

/* Retreive string buffer current lentght */
extern int sb_length(sb_buffer* buf);

/* Release a string buffer object */
extern void sb_free(sb_buffer* sb);

#endif

