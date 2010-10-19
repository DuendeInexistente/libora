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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "stringbuffer.h"

static void createChunk(sb_buffer* buffer)
{
  buffer->last = buffer->last->next = (sb_block*)malloc(sizeof(sb_block));
  buffer->cnt = 0;
  buffer->numChunks++;
}

sb_buffer* sb_create()
{
  sb_buffer* res = (sb_buffer*)malloc(sizeof(sb_buffer));
  
  res->numChunks = 0;
  res->first = res->last = (sb_block*)malloc(sizeof(sb_block));
  res->cnt = 0;
  return (void*)res;
}

void sb_print(sb_buffer* sb, const char* str)
{
  int len = strlen(str), delta = _SB_BLOCK_LENGTH - 1 - sb->cnt;
  char* dup = sb->last->data + sb->cnt;
  
  while (sb->cnt + len > _SB_BLOCK_LENGTH - 1)
  {
    strncpy(dup, str, delta);
    sb->last->data[_SB_BLOCK_LENGTH - 1] = 0;
    str += delta;
    len -= delta;
    createChunk(sb);
    dup = sb->last->data;
    delta = _SB_BLOCK_LENGTH - 1;
  }
  
  strcpy(dup, str);
  
  sb->cnt += len&(_SB_BLOCK_LENGTH - 1);
}

void sb_printf(sb_buffer* sb, const char *fmt, ...)
{
    va_list ap;
    char buffer[_SB_FORMATTER_LENGTH];

    va_start(ap, fmt);
    vsnprintf(buffer, _SB_FORMATTER_LENGTH-1, fmt, ap);
    va_end(ap);
    return sb_print(sb, buffer);

}

char* sb_string(sb_buffer* sb)
{
  int len = sb_length(sb);
  char* res = (char*)malloc((len+1)*sizeof(char));
  sb_block* act = sb->first;
  
  while (1)
  {
    strcpy(res, act->data);
    if (act == sb->last)
    {
      res += sb->cnt;
      break;
    }
    
    res += _SB_BLOCK_LENGTH - 1;
    act = act->next;
  }
  
  res -= len;
  return res;
}

int sb_length(sb_buffer* sb)
{
  return sb->numChunks * (_SB_BLOCK_LENGTH - 1) + sb->cnt;
}

void sb_free(sb_buffer* sb)
{
  sb_block *next;
  
  while (sb->first != sb->last)
  {
    next = sb->first->next;
    free(sb->first);
    sb->first = next;
  }
  
  free(sb->first);
  free(sb);
}


