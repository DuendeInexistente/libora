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

#define ORA_ERROR_NULL 1
#define ORA_ERROR_INVALID 2
#define ORA_ERROR_CORRUPTED 3
#define ORA_ERROR_READ 4
#define ORA_ERROR_MISSING 5
#define ORA_ERROR_PNGLIB 6
#define ORA_ERROR_WRITE 7

#define ORA_ERROR_MODE 10
#define ORA_ERROR_STACK 11
#define ORA_ERROR_STACK_END 12
#define ORA_ERROR_STACK_POSITION 13
//#define ORA_ERROR_STACK_ 10

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

typedef char ubyte;

typedef void* ORA;

typedef void (*progress_callback) (int progress);

typedef void (*tile_read_callback) (int x, int y, ubyte* data);

typedef ubyte* (*tile_write_callback) (int x, int y);

typedef struct _ora_rectangle
{
    int x;
    int y;
    int width;
    int height;
} ora_rectangle;

#ifdef __cplusplus
extern "C" {
#endif


extern int ora_open(const char* filename, int flags, ORA* ora);

extern int ora_stack_reset(ORA ora);

extern int ora_stack_next(ORA ora, int flags);

extern int ora_stack_level(ORA ora);

extern int ora_stack_type(ORA ora);

//extern int ora_read_thumbnail(ORA ora, ubyte** data, int* width, int* height, int* format, progress_callback callback);

extern int ora_read_layer(ORA ora, ubyte** data, ora_rectangle* geometry, int* format, float* opacity, progress_callback callback);

extern int ora_close(ORA ora);

extern int ora_open_stack(ORA ora, const char* name, int x, int y);

extern int ora_close_stack(ORA ora);

extern int ora_write_layer(ORA ora, const char* name, ora_rectangle geometry, int format, float opacity, ubyte* data, progress_callback callback);
/*
extern int ora_write_tiles(ORA ora, const char* name, ora_rectangle geometry, int format, int tile_size, tile_write_callback tile_source, progress_callback callback);
*/
extern int ora_error(ORA ora);

#ifdef __cplusplus
}
#endif

#endif
