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
#include <stdio.h>
#include <png.h>
#include "../ora.h"

int tool_write_png(const char* filename, ubyte* image_data, int width, int height, int format, progress_callback callback)
{
    png_structp png_ptr;
    png_infop info_ptr;
    int line_size;
    int current_line;
    int current_progress = 0, p;
    FILE *fp;
    

    if (!(format & ORA_FORMAT_RASTER))
        return ORA_ERROR;

    fp = fopen(filename, "wb");
    if (!fp)
    {
       return ORA_ERROR;
    }


    png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_ptr)
       return ORA_ERROR;

    info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr)
    {
       png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
       return ORA_ERROR;
    }

    if (setjmp(png_jmpbuf(png_ptr)))
    {
       png_destroy_write_struct(&png_ptr, &info_ptr);
       fclose(fp);
       return ORA_ERROR;
    }

    png_init_io(png_ptr, fp);

    png_set_compression_level(png_ptr,
        Z_BEST_COMPRESSION);

    png_set_compression_mem_level(png_ptr, 8);
    png_set_compression_strategy(png_ptr, Z_DEFAULT_STRATEGY);
    png_set_compression_window_bits(png_ptr, 15);
    png_set_compression_method(png_ptr, 8);
    png_set_compression_buffer_size(png_ptr, 8192);

    png_set_IHDR(png_ptr, info_ptr, width, height, (format & ORA_FORMAT_DOUBLE ? 16 : 8),
        (format & ORA_FORMAT_ALPHA ? PNG_COLOR_TYPE_RGB_ALPHA : PNG_COLOR_TYPE_RGB),
         PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

    png_write_info(png_ptr, info_ptr);

    png_set_flush(png_ptr, 20);

    printf("format %d \n", format);

    line_size = width * (format & ORA_FORMAT_DOUBLE ? 2 : 1) * (format & ORA_FORMAT_ALPHA ? 4 : 3);

    for (current_line = 0; current_line < height; current_line++)
    {
        png_write_row(png_ptr, image_data + line_size * current_line);

        if (callback)
        {
            p = (current_line * 100) / (height-1);
            if (current_progress != p) 
            {
                callback(p);
                current_progress = p;
            }
        }
    }

    png_write_end(png_ptr, info_ptr);
    png_destroy_write_struct(&png_ptr, &info_ptr);


    fclose(fp);
}


void tool_progress_callback(int progress)
{
    if (progress % 10 == 0)
        printf("%d ", progress);

    if (progress == 100)
        printf("\n");

    fflush(stdout);
}


int main( int argc, char** argv) { 
    char filename[256];
    ORA ora;
    ubyte* image;
    int format;
    ora_rectangle geometry;
    int i;

    if (argc < 2) {
        fprintf(stderr,"Enter a file name!\n");
        return -1;
    }


    if (ora_open(argv[1], ORA_FILE_READ, &ora) != ORA_OK)
        return -1;

    i = 0;

    while (1)
    {
        i++;
        if (ora_stack_next(ora, ORA_NEXT_NO_STACK) > 0)
        {
            printf("Reading layer: %d", i);
            ora_read_layer(ora, &image, &geometry, &format, tool_progress_callback);
            sprintf(filename, "layer%03d.png", i);
            printf("Saving: %s", filename);
            tool_write_png(filename, image, geometry.width, geometry.height, format, tool_progress_callback);
            free(image);
        } else break;
    }

    ora_close(ora);



    return 0;
}
