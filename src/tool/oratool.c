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

#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <png.h>
#include "ora.h"

int tool_write_png(const char* filename, ubyte* image_data, int width, int height, int format, ora_progress_callback callback)
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

    fflush(stdout);
}


int main( int argc, char** argv) { 
    char filename[256];
    ORA ora_in, ora_out;
    ubyte* image;
    int format;
    ora_rectangle geometry;
    float opacity;
    int i;
    long timer, total, read, write;

    if (argc < 2) {
        fprintf(stderr,"Enter a file name!\n");
        return -1;
    }

    total = clock();
    if (ora_open(argv[1], ORA_FILE_READ, &ora_in) != ORA_OK)
        return -1;

    if (ora_open("test.ora", ORA_FILE_WRITE, &ora_out) != ORA_OK)
        return -1;

    i = 0;

    read = 0;
    write = 0;

    while (1)
    {
        i++;
        if (ora_stack_next(ora_in, ORA_NEXT_NO_STACK) > 0)
        {
            printf("Reading layer: %d - ", i);

            timer = clock();
            image = NULL;
            ora_read_layer(ora_in, &image, &geometry, &format, &opacity, tool_progress_callback);

            timer = clock() - timer;
            timer = timer > 0 ? timer : 0;
            printf("\nDone (%ld ms)\n", ((timer) / (CLOCKS_PER_SEC / 1000)));
            read += timer;

            //sprintf(filename, "layer%03d.png", i);
            //printf("Saving: %s", filename);
            //tool_write_png(filename, image, geometry.width, geometry.height, format, tool_progress_callback);
            printf("Writing layer: %d - ", i);

            timer = clock();
            ora_write_layer(ora_out, NULL, geometry, format, opacity, image, tool_progress_callback);

            timer = clock() - timer;
            timer = timer > 0 ? timer : 0;
            printf("\nDone (%ld ms)\n", (timer / (CLOCKS_PER_SEC / 1000)));
            write += timer;

            if (image)
                free(image);
        } else 
            break;
    }

    ora_close(ora_in);
    ora_close(ora_out);

    total = clock() - total;
    total = total > 0 ? total : 0;
    printf("Total: %ld ms, Reading layers: %ld ms, Writing layers: %ld ms.\n", 
            ((total) / (CLOCKS_PER_SEC / 1000)), ((read) / (CLOCKS_PER_SEC / 1000)), ((write) / (CLOCKS_PER_SEC / 1000)));

    return 0;
}
