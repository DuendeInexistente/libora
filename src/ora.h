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



#ifndef _ORAAPI_H
#define _ORAAPI_H

#define ORA_MIMETYPE "image/openraster"

#define ORA_OK 0
#define ORA_ERROR -1

#define ORA_FILE_READ 1
#define ORA_FILE_WRITE 2
#define ORA_FILE_READ_NO_STACK 4

#define ORA_ERROR_INVALID 1
#define ORA_ERROR_CORRUPTED 2
#define ORA_ERROR_READ 3
#define ORA_ERROR_MISSING 3

#define ORA_ERROR_MODE 10
#define ORA_ERROR_STACK 11
#define ORA_ERROR_STACK_END 12
#define ORA_ERROR_STACK_POSITION 13

#define ORA_TYPE_STACK 1
#define ORA_TYPE_LAYER 2
#define ORA_TYPE_FILTER 3

#define ORA_NEXT_SIBLING 1
#define ORA_NEXT_CLIMB 2
#define ORA_NEXT_NO_STACK 4
#define ORA_NEXT_NO_LAYER 8

#define ORA_FORMAT_RASTER 1
#define ORA_FORMAT_VECTOR 2
#define ORA_FORMAT_TEXT 4

#define ORA_FORMAT_BACKGROUND 128
#define ORA_FORMAT_DOUBLE 256
#define ORA_FORMAT_ALPHA 512

typedef void* ORA;

typedef void (*progress_callback) (int progress);

typedef char ubyte;

typedef struct _ora_rectangle
{
    int x;
    int y;
    int width;
    int height;
} ora_rectangle;

extern int ora_open(const char* filename, int flags, ORA* ora);

extern int ora_stack_reset(ORA ora);

extern int ora_stack_next(ORA ora, int flags);

extern int ora_stack_level(ORA ora);

extern int ora_stack_type(ORA ora);

//extern int ora_read_thumbnail(ORA ora, ubyte** data, int* width, int* height, int* format, progress_callback callback);

extern int ora_read_layer(ORA ora, ubyte** data, ora_rectangle* geometry, int* format, progress_callback callback);

extern int ora_close(ORA ora);

extern int ora_error(ORA ora);



#endif
