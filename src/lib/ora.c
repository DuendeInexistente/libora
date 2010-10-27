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

#include "ora.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>
#include <time.h>

#include "internal.h"
#include "zip/unzip.h"
#include "zip/zip.h"
#include "layers/layers.h"
#include "layers/stack.h"

#define ORA_COMMENT "Created with libora"

#define _GET_READ_DOCUMENT(ora, doc) if (!ora || (((ora_document*)ora)->magic) != _ORA_MAGIC_READ) \
    doc = NULL; else doc = (ora_document_read*) ora;

#define _GET_WRITE_DOCUMENT(ora, doc) if (!ora) doc = NULL; else if ((((ora_document*)ora)->magic) != _ORA_MAGIC_WRITE) \
    {doc = NULL; ORA_SET_ERROR(ora, ORA_ERROR_INVALID);} else doc = (ora_document_write*) ora;

#define _BUFFER_LENGTH 256

typedef struct ora_document_read_d
{
    char magic;
    int flags;
    int error;
    int width;
    int height;
    unzFile file;
    _ora_stack_node* stack;
    _ora_stack_node* current;
} ora_document_read;

typedef struct ora_document_write_d
{
    char magic;
    int flags;
    int error;
    int width;
    int height;
    zipFile file;
    _ora_stack_node* stack;
    _ora_stack_node* parent;
    _ora_stack_node* sibling;
    int layer_number;
} ora_document_write;

tm_zip _ora_current_timestamp()
{
    tm_zip ts;
    time_t rawtime;
    struct tm * tm;

    time ( &rawtime );
    tm = localtime ( &rawtime );

    ts.tm_sec = tm->tm_sec;
    ts.tm_min = tm->tm_min;
    ts.tm_hour = tm->tm_hour;
    ts.tm_mday = tm->tm_mday;
    ts.tm_mon = tm->tm_mon;
    ts.tm_year = tm->tm_year;

    return ts;
}

zip_fileinfo _ora_default_fileinfo() 
{
    zip_fileinfo info;

    info.tmz_date = _ora_current_timestamp();
    info.dosDate = 0;
    info.internal_fa = 0;
    info.external_fa = 0;

    return info;
}

_ora_stack_node* _ora_parse_stack(unzFile zip, int* width, int* height, int* error) 
{
    unz_file_info file_info;
    char buffer[_BUFFER_LENGTH];
    char* stack_buffer = NULL;
    int stack_buffer_length = 0;
    _ora_stack_node* stack;
    ora_rectangle bounds;

    ORA_DEBUG("Parsing and validating layer stack.\n");

    if (unzLocateFile(zip, "stack.xml") != UNZ_OK)
    {
        if (error) *error = ORA_ERROR_CORRUPTED;
        return NULL;
    }

    if (unzGetCurrentFileInfo (zip, &file_info, buffer, _BUFFER_LENGTH, NULL, 0, NULL, 0) != UNZ_OK)
    {
        if (error) *error = ORA_ERROR_READ;
        return NULL;
    }

    if (unzOpenCurrentFile (zip) != UNZ_OK)
    {
        if (error) *error = ORA_ERROR_READ;
        return NULL;
    }

    stack_buffer = (char*) malloc((file_info.uncompressed_size + 1) * sizeof(char));

    if ((stack_buffer_length = unzReadCurrentFile (zip, stack_buffer, file_info.uncompressed_size)) < 0) 
    {
        free(stack_buffer);
        if (error) *error = ORA_ERROR_READ;
        return NULL;
    }

    if (unzCloseCurrentFile (zip) != UNZ_OK)
    {
        free(stack_buffer);
        if (error) *error = ORA_ERROR_CORRUPTED;
        return NULL;
    }

    if (!stack_buffer_length)
        stack_buffer_length = file_info.uncompressed_size;

    //xml = xmlReadMemory(stack_buffer, stack_buffer_length, "stack.xml", NULL, 0);
    stack = _ora_xml_to_stack(stack_buffer, stack_buffer_length, &bounds, error);

    free(stack_buffer);

    if (stack == NULL) 
    {
        ORA_DEBUG("stack.xml not parsed successfully. \n");
        //if (error) *error = ORA_ERROR_STACK;
        return NULL;
    }

    *width = bounds.width;
    *height = bounds.height;


    if (*width < 1 || *height < 1)
    {
        _ora_free_stack(stack);
        ORA_DEBUG("Illegal image dimensions: %dx%d. \n", *width, *height);
        if (error) *error = ORA_ERROR_STACK;
        return NULL;
    }

    return stack;

}

void _ora_write_stack(ora_document_write* doc) 
{
    zipFile zip;
    zip_fileinfo info;
    _ora_stack_node* stack;
    int int_error = 0;
    char* xmlbuff = NULL;
    ora_rectangle bounds;

    zip = doc->file;

    bounds.width = doc->width;
    bounds.height = doc->height;

    xmlbuff = _ora_stack_to_xml(doc->stack, bounds, &int_error);
    //printf("%s", xmlbuff);

    ORA_DEBUG("Writing layer stack.\n");

    info = _ora_default_fileinfo();

    if (zipOpenNewFileInZip (zip, "stack.xml", &info, NULL, 0, NULL, 0, NULL, Z_DEFLATED, Z_DEFAULT_COMPRESSION) != ZIP_OK)
    {
        free(xmlbuff);
        ORA_SET_ERROR(doc, ORA_ERROR_WRITE);
        return;
    }

    if (zipWriteInFileInZip(zip, xmlbuff, strlen(xmlbuff)) < 0) 
    {
        free(xmlbuff);
        zipClose(zip, NULL);
        ORA_SET_ERROR(doc, ORA_ERROR_WRITE);
        return;
    }

    free(xmlbuff);

    if (zipCloseFileInZip (zip) != ZIP_OK)
    {
        ORA_SET_ERROR(doc, ORA_ERROR_WRITE);
        return;
    }

}

ORA _ora_read_file(const char* filename, int flags, int* error) {
    unzFile zip;
    unz_file_info file_info;
    char buffer[_BUFFER_LENGTH];
    _ora_stack_node* stack;
    ora_document_read* read_struct;
    int width, height;

    zip = unzOpen (filename, NULL);

    if (!zip) 
    {
        if (error) *error = ORA_ERROR_READ;
        return NULL;
    }

    if (unzGoToFirstFile(zip) != UNZ_OK) 
    {
        unzClose(zip);
        if (error) *error = ORA_ERROR_READ;
        return NULL;
    }

    if (unzGetCurrentFileInfo (zip, &file_info, buffer, _BUFFER_LENGTH, NULL, 0, NULL, 0) != UNZ_OK)
    {
        unzClose(zip);
        if (error) *error = ORA_ERROR_READ;
        return NULL;
    }

    if (strcmp(buffer, "mimetype") != 0 || file_info.compression_method != 0)
    {
        unzClose(zip);
        if (error) *error = ORA_ERROR_CORRUPTED;
        return NULL;
    }

    if (unzOpenCurrentFile (zip) != UNZ_OK)
    {
        unzClose(zip);
        if (error) *error = ORA_ERROR_READ;
        return NULL;
    }

    memset(buffer, 0, _BUFFER_LENGTH);

    if (unzReadCurrentFile (zip, buffer, file_info.uncompressed_size) < 0) 
    {
        unzClose(zip);
        if (error) *error = ORA_ERROR_READ;
        return NULL;
    }

    if (strcmp(buffer, ORA_MIMETYPE) != 0)
    {
        unzClose(zip);
        if (error) *error = ORA_ERROR_CORRUPTED;
        return NULL;
    }

    if (unzCloseCurrentFile (zip) != UNZ_OK)
    {
        unzClose(zip);
        if (error) *error = ORA_ERROR_CORRUPTED;
        return NULL;
    }

    if (!(flags & ORA_FILE_READ_NO_STACK)) 
    {
        stack = _ora_parse_stack(zip, &width, &height, error);
        if (!stack)
        {
           unzClose(zip);
           return NULL;
        }
    }

    read_struct = (ora_document_read*) malloc(sizeof(ora_document_read));

    read_struct->magic = _ORA_MAGIC_READ;
    read_struct->flags = flags;
    read_struct->width = width;
    read_struct->height = height;
    read_struct->file = zip;
    read_struct->stack = stack;
    read_struct->current = stack;
    read_struct->error = 0;

    ORA_DEBUG("Done with initial reading.\n");

    return read_struct;
}

ORA _ora_write_file(const char* filename, int flags, int* error) {
    zipFile zip;
    zip_fileinfo info;
    char buffer[_BUFFER_LENGTH];
    ora_document_write* write_struct;
    _ora_stack_node* stack;
    int int_error = 0;

    zip = zipOpen (filename, APPEND_STATUS_CREATE);

    if (!zip) 
    {
        if (error) *error = ORA_ERROR_WRITE;
        return NULL;
    }

    ORA_DEBUG("Writing layer stack.\n");

    info = _ora_default_fileinfo();

    if (zipOpenNewFileInZip (zip, "mimetype", &info, NULL, 0, NULL, 0, NULL, 0, 0) != ZIP_OK)
    {
        zipClose(zip, NULL);
        if (error) *error = ORA_ERROR_WRITE;
        return NULL;
    }

    if (zipWriteInFileInZip(zip, ORA_MIMETYPE, strlen(ORA_MIMETYPE)) < 0) 
    {
        zipClose(zip, NULL);
        if (error) *error = ORA_ERROR_WRITE;
        return NULL;
    }

    if (zipCloseFileInZip (zip) != ZIP_OK)
    {
        zipClose(zip, NULL);
        if (error) *error = ORA_ERROR_WRITE;
        return NULL;
    }

    write_struct = (ora_document_write*) malloc(sizeof(ora_document_write));

    write_struct->magic = _ORA_MAGIC_WRITE;
    write_struct->flags = flags;
    write_struct->width = -1;
    write_struct->height = -1;
    write_struct->file = zip;
    write_struct->stack = (_ora_stack_node*) malloc(sizeof(_ora_stack_node));
    write_struct->stack->type = ORA_TYPE_STACK;
    write_struct->stack->data = malloc(sizeof(_ora_stack_stack));
    memset(write_struct->stack->data, 0, sizeof(_ora_stack_stack));
    write_struct->stack->parent = NULL;
    write_struct->stack->sibling = NULL;
    write_struct->stack->children = NULL;
    write_struct->parent = write_struct->stack;
    write_struct->sibling = NULL;
    write_struct->error = 0;
    write_struct->layer_number = 0;
    ORA_DEBUG("Done with initial writing.\n");

    return write_struct;
}

extern int ora_open(const char* filename, int flags, ORA* ora)
{
    int error = 0;

    if (!ora)
        return ORA_ERROR;

    ORA_DEBUG("Opening file %s\n", filename);

    if (flags & ORA_FILE_READ)
        *ora = _ora_read_file(filename, flags, &error);
    else if (flags & ORA_FILE_WRITE)
        *ora = _ora_write_file(filename, flags, &error);

    if (error)
        return error;

    return ORA_OK;
}


extern int ora_close(ORA ora) 
{
    ora_document_read* read_struct;
    ora_document_write* write_struct;

    if (!ora)
        return ORA_ERROR;

    ORA_CLEAR_ERROR(ora);

    if (((ora_document_read*)ora)->magic == _ORA_MAGIC_READ) {

        ORA_DEBUG("Closing file (read)\n");

        read_struct = (ora_document_read*)ora;

        unzClose(read_struct->file);
        _ora_free_stack(read_struct->stack);
        read_struct->stack = NULL;
        read_struct->current = NULL;
        read_struct->file = NULL;

        free(read_struct);

        return ORA_OK;
    }

    if (((ora_document_write*)ora)->magic == _ORA_MAGIC_WRITE) {

        ORA_DEBUG("Closing file (write)\n");

        write_struct = (ora_document_write*)ora;

        _ora_write_stack(write_struct);

        zipClose(write_struct->file, NULL); // ORA_COMMENT);
        _ora_free_stack(write_struct->stack);
        write_struct->stack = NULL;
        write_struct->file = NULL;

        free(write_struct);

        return ORA_OK;
    }
}



extern int ora_stack_reset(ORA ora) 
{
    ora_document_read* read_struct;

    if (!ora)
        return ORA_ERROR;

    ORA_CLEAR_ERROR(ora);
    _GET_READ_DOCUMENT(ora, read_struct);

    if (!read_struct)
        return ORA_ERROR;

    if (!read_struct->stack) 
    {
        read_struct->stack = _ora_parse_stack(read_struct->file, &(read_struct->width), &(read_struct->height), &(read_struct->error));
    }

    read_struct->current = read_struct->stack;

}

extern int ora_stack_next(ORA ora, int flags)
{
    ora_document_read* read_struct;
    _ora_stack_node* node;
    int moves = 0;

    if (!ora)
        return ORA_ERROR;

    ORA_CLEAR_ERROR(ora);
    _GET_READ_DOCUMENT(ora, read_struct);

    if (!read_struct)
        return ORA_ERROR;

    if (!read_struct->stack) 
    {
        read_struct->stack = _ora_parse_stack(read_struct->file, &(read_struct->width), &(read_struct->height), &(read_struct->error));
        read_struct->current = read_struct->stack;
    }

    if (!read_struct->current) {
        ORA_SET_ERROR(ora, ORA_ERROR_STACK_END);
        return ORA_ERROR;
    }

    node = read_struct->current;
    while (1)
    {
        if (node->children && !(flags & ORA_NEXT_SIBLING) && !(flags & ORA_NEXT_CLIMB)) {
            node = node->children;
        } else if (node->sibling && !(flags & ORA_NEXT_CLIMB)) {
            node = node->sibling;
        } else if (node->parent) {
            node = node->parent->sibling;
            flags = flags & ORA_NEXT_CLIMB ? flags - ORA_NEXT_CLIMB : flags;
        } else {
            read_struct->current = NULL;
            ORA_SET_ERROR(ora, ORA_ERROR_STACK_END);
            return ORA_ERROR;
        }

        if (!node) {
            ORA_SET_ERROR(ora, ORA_ERROR_STACK_END);
            return ORA_ERROR;
        }

        moves++;

        if ((flags & ORA_NEXT_NO_STACK) && node->type == ORA_TYPE_STACK)
            continue;
        if ((flags & ORA_NEXT_NO_LAYER) && node->type == ORA_TYPE_LAYER)
            continue;

        break;
    }

    read_struct->current = node;
    return moves;
}

extern int ora_stack_level(ORA ora)
{
    ora_document_read* read_struct;
    _ora_stack_node* node;
    int level = 0;

    if (!ora)
        return ORA_ERROR;

    ORA_CLEAR_ERROR(ora);
    _GET_READ_DOCUMENT(ora, read_struct);

    if (!read_struct)
        return ORA_ERROR;

    if (!read_struct->stack) 
    {
        read_struct->stack = _ora_parse_stack(read_struct->file, &(read_struct->width), &(read_struct->height), &(read_struct->error));
        read_struct->current = read_struct->stack;
        return 1;
    }

    if (!read_struct->current) {
        ORA_SET_ERROR(ora, ORA_ERROR_STACK_END);
        return ORA_ERROR;
    }

    node = read_struct->current;

    while (node) {
        node = node->parent;
        level++;
    }

    return level;
}

extern int ora_stack_type(ORA ora)
{
    ora_document_read* read_struct;

    if (!ora)
        return ORA_ERROR;

    ORA_CLEAR_ERROR(ora);
    _GET_READ_DOCUMENT(ora, read_struct);

    if (!read_struct)
        return ORA_ERROR;

    if (!read_struct->stack) 
    {
        read_struct->stack = _ora_parse_stack(read_struct->file, &(read_struct->width), &(read_struct->height), &(read_struct->error));
        read_struct->current = read_struct->stack;
    }

    if (read_struct->current)
        return read_struct->current->type;

    ORA_SET_ERROR(ora, ORA_ERROR_STACK_END);
    return ORA_ERROR;

}

extern int ora_read_layer(ORA ora, ora_layer* layer, ora_progress_callback callback)
{
    unz_file_info file_info;
    ora_document_read* read_struct;
    _ora_stack_layer* layer_data;
    unzFile zip;

    if (!ora)
        return ORA_ERROR;

    ORA_CLEAR_ERROR(ora);
    _GET_READ_DOCUMENT(ora, read_struct);

    if (!read_struct)
        return ORA_ERROR;

    if (!read_struct->stack) 
    {
        read_struct->stack = _ora_parse_stack(read_struct->file, &(read_struct->width), &(read_struct->height), &(read_struct->error));
        read_struct->current = read_struct->stack;
    }

    if (!read_struct->current) {
        ORA_SET_ERROR(ora, ORA_ERROR_STACK_END);
        return ORA_ERROR;
    }

    if (read_struct->current->type != ORA_TYPE_LAYER) {
        ORA_SET_ERROR(ora, ORA_ERROR_STACK_POSITION);
        return ORA_ERROR;
    }

    zip = read_struct->file;
    layer_data = (_ora_stack_layer*) read_struct->current->data;

    if (!layer_data->src || unzLocateFile(zip, layer_data->src) != UNZ_OK)
    {
        ORA_SET_ERROR(ora, ORA_ERROR_MISSING);
        return ORA_ERROR;
    }

    ORA_DEBUG("Reading layer from file %s\n", layer_data->src);


    if (unzGetCurrentFileInfo (zip, &file_info, NULL, 0, NULL, 0, NULL, 0) != UNZ_OK)
    {
        ORA_SET_ERROR(ora, ORA_ERROR_READ);
        return ORA_ERROR;
    }

    if (unzOpenCurrentFile (zip) != UNZ_OK)
    {
        ORA_SET_ERROR(ora, ORA_ERROR_READ);
        return ORA_ERROR;
    }

    if (ora_read_raster((ora_document*)ora, zip, layer, callback) < 0)
    {
        ORA_SET_ERROR(ora, ORA_ERROR_READ);
        return ORA_ERROR;
    }

    if (unzCloseCurrentFile (zip) != UNZ_OK)
    {
        ORA_SET_ERROR(ora, ORA_ERROR_CORRUPTED);
        return ORA_ERROR;
    }


    return ORA_OK;
}

extern int ora_open_stack(ORA ora, const char* name, int x, int y)
{
    ora_document_write* write_struct;
    _ora_stack_node* node;

    if (!ora)
        return ORA_ERROR;

    ORA_CLEAR_ERROR(ora);
    _GET_WRITE_DOCUMENT(ora, write_struct);

    if (!write_struct)
        return ORA_ERROR;

    node = (_ora_stack_node*) malloc(sizeof(_ora_stack_node));
    node->type = ORA_TYPE_STACK;
    node->data = malloc(sizeof(_ora_stack_stack));
    ((_ora_stack_stack*)node->data)->name = NULL;
    ((_ora_stack_stack*)node->data)->x = x;
    ((_ora_stack_stack*)node->data)->y = y;

    node->sibling = NULL;
    node->parent = write_struct->parent;

    if (write_struct->sibling)
        write_struct->sibling->sibling = node;
    else
        write_struct->parent->children = node;

    node->children = NULL;

    write_struct->parent = node;
    write_struct->sibling = NULL;
}

extern int ora_close_stack(ORA ora)
{
    ora_document_write* write_struct;

    if (!ora)
        return ORA_ERROR;

    ORA_CLEAR_ERROR(ora);
    _GET_WRITE_DOCUMENT(ora, write_struct);

    if (!write_struct)
        return ORA_ERROR;

    if (write_struct->parent->type != ORA_TYPE_STACK)
    {
        ORA_SET_ERROR(ora, ORA_ERROR_STACK_POSITION);
        return ORA_ERROR;
    }

    if (!write_struct->parent->parent)
    {
        ORA_SET_ERROR(ora, ORA_ERROR_STACK_END);
        return ORA_ERROR;
    }

    write_struct->sibling = write_struct->parent;
    write_struct->parent = write_struct->parent->parent;

}

extern int ora_write_layer(ORA ora, const char* name, ora_rectangle geometry, int format, float opacity, ubyte* data, ora_progress_callback callback)
{
    ora_document_write* write_struct;
    zip_fileinfo info;
    _ora_stack_node* node;
    char src[_BUFFER_LENGTH];

    if (!ora)
        return ORA_ERROR;

    ORA_CLEAR_ERROR(ora);
    _GET_WRITE_DOCUMENT(ora, write_struct);

    if (!write_struct)
        return ORA_ERROR;

    if (write_struct->parent->type != ORA_TYPE_STACK)
    {
        ORA_SET_ERROR(ora, ORA_ERROR_STACK_POSITION);
        return ORA_ERROR;
    }

    sprintf(src, "data/layer%04d.png", write_struct->layer_number);

    ORA_DEBUG("Writing layer stack.\n");

    info = _ora_default_fileinfo();

    if (zipOpenNewFileInZip (write_struct->file, src, &info, NULL, 0, NULL, 0, NULL, Z_DEFLATED, Z_BEST_SPEED) != ZIP_OK) //Z_DEFAULT_COMPRESSION
    {
        ORA_SET_ERROR(write_struct, ORA_ERROR_WRITE);
        return ORA_ERROR;
    }

    if (ora_write_raster((ora_document *)write_struct, write_struct->file, data, geometry.width, geometry.height, format, callback) != ORA_OK) 
    {
        return ORA_ERROR;
    }

    if (zipCloseFileInZip (write_struct->file) != ZIP_OK)
    {
        ORA_SET_ERROR(write_struct, ORA_ERROR_WRITE);
        return ORA_ERROR;
    }

    node = (_ora_stack_node*) malloc(sizeof(_ora_stack_node));
    node->type = ORA_TYPE_LAYER;
    node->data = malloc(sizeof(_ora_stack_layer));
    ((_ora_stack_layer*)node->data)->name = NULL;
    ((_ora_stack_layer*)node->data)->bounds = geometry;
    ((_ora_stack_layer*)node->data)->opacity = CLAMP(opacity,0,1);

    ((_ora_stack_layer*)node->data)->src = strclone(src);

    node->sibling = NULL;
    node->parent = write_struct->parent;
    if (write_struct->sibling)
        write_struct->sibling->sibling = node;
    else
        write_struct->parent->children = node;

    node->children = NULL;

    write_struct->sibling = node;
    write_struct->layer_number++;

    return ORA_OK;

}

extern int ora_error(ORA ora) 
{
    if (!ora)
        return 0;

    if (!(((ora_document*) ora)->magic == _ORA_MAGIC_READ || 
            ((ora_document*) ora)->magic == _ORA_MAGIC_WRITE ||
             ((ora_document*) ora)->magic == _ORA_MAGIC_CLOSED))
        return 0;

    return ((ora_document*) ora)->error;
}

int ora_get_document_size(ORA ora, int* width, int* height) {

    ora_document_read* doc = (ora_document_read*)ora;
    *width = doc->width;
    *height = doc->height;

    return 0;
}

