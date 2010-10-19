/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * Redistribution and use in source and binary forms, with or without modification, are
 * permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this list of
 *    conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list
 *    of conditions and the following disclaimer in the documentation and/or other materials
 *    provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY LIBORA DEVELOPERS ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL LIBORA DEVELOPERS OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 * The views and conclusions contained in the software and documentation are those of the
 * authors and should not be interpreted as representing official policies, either expressed
 * or implied, of libora developers.
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

