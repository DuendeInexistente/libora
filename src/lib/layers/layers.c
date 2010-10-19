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


#include <stdlib.h>
#include <png.h>

#include "../zip/unzip.h"
#include "../zip/zip.h"
#include "layers.h"

void _ora_png_error_fn(png_structp png_ptr, png_const_charp error_msg)
{
    ora_document* document;

    document = (ora_document*) png_get_error_ptr(png_ptr);

    ORA_SET_ERROR(document, document->magic == _ORA_MAGIC_READ ? ORA_ERROR_WRITE : ORA_ERROR_READ);
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

void _ora_png_write_data(png_structp png_ptr, png_bytep data, png_size_t length)
{
    zipFile zip;

    zip = (zipFile) png_get_io_ptr(png_ptr);

    zipWriteInFileInZip (zip, data, length);
}

void _ora_png_flush_data(png_structp png_ptr)
{
    //TODO: flushing
}

int ora_read_raster(ora_document* document, unzFile zip, ubyte** data, int* width, int* height, int* format, ora_progress_callback callback)
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
    {
        ORA_SET_ERROR(document, ORA_ERROR_PNGLIB);
        return ORA_ERROR;
    }

    info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr)
    {
        png_destroy_read_struct(&png_ptr, (png_infopp)NULL, (png_infopp)NULL);
        ORA_SET_ERROR(document, ORA_ERROR_PNGLIB);
        return ORA_ERROR;
    }

    end_info = png_create_info_struct(png_ptr);
    if (!end_info)
    {
        png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)NULL);
        ORA_SET_ERROR(document, ORA_ERROR_PNGLIB);
        return ORA_ERROR;
    }

    if (setjmp(png_jmpbuf(png_ptr)))
    {
        png_destroy_read_struct(&png_ptr, &info_ptr,
           (png_infopp)NULL);
        ORA_SET_ERROR(document, ORA_ERROR_PNGLIB);
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
        ORA_SET_ERROR(document, ORA_ERROR_READ);
        return ORA_ERROR;
    }


    *width = png_get_image_width(png_ptr, info_ptr);
    *height = png_get_image_height(png_ptr, info_ptr);
    bit_depth = png_get_bit_depth(png_ptr, info_ptr);
    color_type = png_get_color_type(png_ptr,info_ptr);

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


int ora_write_raster(ora_document* document, zipFile zip, ubyte* data, int width, int height, int format, ora_progress_callback callback)
{
    png_structp png_ptr;
    png_infop info_ptr;
    int line_size;
    int current_line;
    int current_progress = 0, p;

    if (!(format & ORA_FORMAT_RASTER) || !data)
        return ORA_ERROR;

    png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, (png_voidp)document, _ora_png_error_fn, _ora_png_warning_fn);
    if (!png_ptr) 
    {
        ORA_SET_ERROR(document, ORA_ERROR_PNGLIB);
        return ORA_ERROR;
    }

    info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr)
    {
        png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
        ORA_SET_ERROR(document, ORA_ERROR_PNGLIB);
        return ORA_ERROR;
    }

    if (setjmp(png_jmpbuf(png_ptr)))
    {
        png_destroy_write_struct(&png_ptr, &info_ptr);
        ORA_SET_ERROR(document, ORA_ERROR_PNGLIB);
        return ORA_ERROR;
    }

    png_set_write_fn(png_ptr, zip, _ora_png_write_data, _ora_png_flush_data);

    // no need for slow compression, because everything gets compressed with the
    // same algorithm anyway


    png_set_compression_level(png_ptr, Z_NO_COMPRESSION); //Z_BEST_SPEED
    //png_set_compression_level(png_ptr, Z_NO_COMPRESSION); // <-- slower (because of bigger memcpy, maybe?)

    /* "The following functions are mainly for testing, but may be
       useful in some cases, like if you need to write PNG files
       extremely fast and are willing to give up some compression, [...]"

       Description of the types can be found at http://en.wikipedia.org/wiki/Portable_Network_Graphics#Filtering

    png_set_filter(png_ptr, 0,
       PNG_FILTER_NONE  | PNG_FILTER_VALUE_NONE |
       PNG_FILTER_SUB   | PNG_FILTER_VALUE_SUB  |
       PNG_FILTER_UP    | PNG_FILTER_VALUE_UP   |
       PNG_FILTER_AVE   | PNG_FILTER_VALUE_AVE  |
       PNG_FILTER_PAETH | PNG_FILTER_VALUE_PAETH|
       PNG_ALL_FILTERS);
    */

    // default (all filters enabled):                 1350ms, 3.4MB
    //png_set_filter(png_ptr, 0, PNG_FILTER_NONE);  // 790ms, 3.8MB
    //png_set_filter(png_ptr, 0, PNG_FILTER_PAETH); // 980ms, 3.5MB
    png_set_filter(png_ptr, 0, PNG_FILTER_SUB);     // 760ms, 3.4MB

    /* "The png_set_compression_*() functions interface to the zlib
       compression library, and should mostly be ignored unless you
       really know what you are doing. The only generally useful call
       is png_set_compression_level() [...]"
    */

    png_set_IHDR(png_ptr, info_ptr, width, height, (format & ORA_FORMAT_DOUBLE ? 16 : 8),
        (format & ORA_FORMAT_ALPHA ? PNG_COLOR_TYPE_RGB_ALPHA : PNG_COLOR_TYPE_RGB),
         PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

    png_write_info(png_ptr, info_ptr);

    png_set_flush(png_ptr, 20);

    line_size = width * (format & ORA_FORMAT_DOUBLE ? 2 : 1) * (format & ORA_FORMAT_ALPHA ? 4 : 3);

    for (current_line = 0; current_line < height; current_line++)
    {
        png_write_row(png_ptr, data + line_size * current_line);

        if (callback)
        {
            p = (current_line * 100) / (height-1);
            if (current_progress != p) 
            {
                callback(p);
                current_progress = p;
            }
        }


        if (document->error)
            break;
    }

    png_write_end(png_ptr, info_ptr);
    png_destroy_write_struct(&png_ptr, &info_ptr);

    return ORA_OK;
}

