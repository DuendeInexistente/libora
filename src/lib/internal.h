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
 * FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> OR
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
/**
 * \file internal.h
 * This file contains common internal structures and functions, used in other files. 
 */
#ifndef _ORACOMMON_H
#define _ORACOMMON_H

#define _ORA_MAGIC_READ 0x00
#define _ORA_MAGIC_WRITE 0x0F
#define _ORA_MAGIC_CLOSED 0x0A

#define ORA_CLEAR_ERROR(ora) { if ((((ora_document*) ora)->magic == _ORA_MAGIC_READ || ((ora_document*) ora)->magic == _ORA_MAGIC_WRITE)) ((ora_document *)ora)->error = 0; }
#define ORA_SET_ERROR(ora, id) ((ora_document *)ora)->error = id

#ifdef DEBUG
    #define ORA_DEBUG(...) { fprintf(stderr, "ORA(%s,%d): ",  __FUNCTION__, __LINE__); fprintf(stderr, __VA_ARGS__); }
//  #define _ORA_EXIT_ERROR(ora, id, retval) {ORA_SET_ERROR(ora, id); ORA_DEBUG("Error id: %d\n", id) return retval;}
#else
  #define ORA_DEBUG(...) {  }
#endif

#define ORA_PING { fprintf(stderr, "ORA(%s,%d): ping",  __FUNCTION__, __LINE__); }

#define CLAMP(A,B,C) A < B ? B : (A > C ? C : A)

#ifndef MIN
#define MIN(A,B) A < B ? A : B
#endif 

#ifndef MAX
#define MAX(A,B) A < B ? B : A
#endif 

typedef struct ora_document_d
{
    char magic;
    int flags;
    int error;
} ora_document;

char* strclone(const char* string);

#endif
