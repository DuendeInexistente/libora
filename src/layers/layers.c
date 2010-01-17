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



#include <stdlib.h>
#include <png.h>

#include "../zip/unzip.h"
#include "layers.h"

void _ora_png_error_fn(png_structp png_ptr, png_const_charp error_msg)
{
    ora_document* document;

    document = (ora_document*) png_get_error_ptr(png_ptr);

    ORA_SET_ERROR(document, ORA_ERROR_READ);
    ORA_DEBUG("PNG read: %s \n", error_msg);
}

void _ora_png_warning_fn(png_structp png_ptr, png_const_charp warning_msg)
{
    ORA_DEBUG("PNG read: %s \n", warning_msg);
}

void _ora_png_read_data(png_structp png_ptr, png_bytep data, png_size_t length)
{
    unzFile zip;

    zip = (unzFile) png_get_io_ptr(png_ptr);

    if (unzReadCurrentFile (zip, data, length) < 0) 
    {

    }


}
/*
void _ora_png_write_data(png_structp png_ptr, png_bytep data, png_size_t length)
{


}

void _ora_png_flush_data(png_structp png_ptr)
{





}
*/
int ora_read_raster(ora_document* document, unzFile zip, ubyte** data, int* width, int* height, int* format, progress_callback callback)
{
    png_structp png_ptr;
    png_infop info_ptr;
    png_infop end_info;
    int bit_depth;
    int color_type;
    long image_size;
    int line_size;
    ubyte* image_data;
    int current_line;
    int current_progress = 0, p;
/*
    FILE *fp = fopen(file_name, "rb");
    if (!fp)
    {
        return (ERROR);
    }
    fread(header, 1, number, fp);
    is_png = !png_sig_cmp(header, 0, number);
    if (!is_png)
    {
        return (NOT_PNG);
    }
png_set_sig_bytes(png_ptr, number);


*/
    png_ptr = png_create_read_struct
       (PNG_LIBPNG_VER_STRING, (png_voidp)document, _ora_png_error_fn, _ora_png_warning_fn);
    if (!png_ptr)
        return ORA_ERROR;

    info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr)
    {
        png_destroy_read_struct(&png_ptr, (png_infopp)NULL, (png_infopp)NULL);
        return ORA_ERROR;
    }

    end_info = png_create_info_struct(png_ptr);
    if (!end_info)
    {
        png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)NULL);
        return ORA_ERROR;
    }

    if (setjmp(png_jmpbuf(png_ptr)))
    {
        png_destroy_read_struct(&png_ptr, &info_ptr,
           (png_infopp)NULL);
        return ORA_ERROR;
    }

    png_set_read_fn(png_ptr, zip, _ora_png_read_data);

    png_read_info(png_ptr, info_ptr);

    //png_read_png(png_ptr, info_ptr, png_transforms, NULL);

    if (png_get_interlace_type(png_ptr, info_ptr) != 0)
    {
        // interlaced PNGs not supported (no real 
        // benefits in OpenRaster context)

        png_destroy_read_struct(&png_ptr, &info_ptr,
           (png_infopp)NULL);
        return ORA_ERROR;
    }


    *width = png_get_image_width(png_ptr, info_ptr);
    *height = png_get_image_height(png_ptr, info_ptr);
    bit_depth = png_get_bit_depth(png_ptr, info_ptr);
    color_type = png_get_color_type(png_ptr,info_ptr);

    printf("colortype: %d \n", color_type);

    if (color_type == PNG_COLOR_TYPE_PALETTE)
        png_set_palette_to_rgb(png_ptr);

    if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
        png_set_expand_gray_1_2_4_to_8(png_ptr);

    if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
        png_set_tRNS_to_alpha(png_ptr);

    line_size = (*width) * (bit_depth < 16 ? 1 : 2) * (color_type == PNG_COLOR_TYPE_RGBA ? 4 : 3);
    image_size = line_size * (*height);

    image_data = (ubyte*) malloc(sizeof(ubyte) * image_size);

    for (current_line = 0; current_line < *height; current_line++)
    {
        png_read_row(png_ptr, image_data + line_size * current_line, NULL);

        if (callback)
        {
            p = (current_line * 100) / (*height - 1);
            if (current_progress != p) 
            {
                callback(p);
                current_progress = p;
            }
        }

        if (document->error)
            break;
    }
/*
    if (bit_depth == 16)
        png_set_strip_16(png_ptr);
    if (bit_depth == 16)
        png_set_swap(png_ptr);
*/

    png_read_end(png_ptr, end_info);
    png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);

    if (document->error)
    {
        free(image_data);


        return ORA_ERROR;
    }

    *data = image_data;
    *format = ORA_FORMAT_RASTER | (color_type == PNG_COLOR_TYPE_RGBA ? ORA_FORMAT_ALPHA : 0) | (bit_depth < 16 ? 0 : ORA_FORMAT_DOUBLE);

    return ORA_OK;
}




