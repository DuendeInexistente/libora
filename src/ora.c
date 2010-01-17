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


#include "ora.h"

#define _ORA_MAGIC_READ 0x00
#define _ORA_MAGIC_WRITE 0xFF

#include <stdlib.h>
#include <string.h>
#include <libxml/tree.h>
#include <libxml/parser.h>

#include "internal.h"
#include "zip/unzip.h"
#include "layers/layers.h"

#define _GET_READ_DOCUMENT(ora, doc) if (!ora || (((ora_document*)ora)->magic) != _ORA_MAGIC_READ) \
    doc = NULL; else doc = (ora_document_read*) ora;

#define _GET_WRITE_DOCUMENT(ora, doc) if (!ora || (((ora_document*)ora)->magic) != _ORA_MAGIC_WRITE) \
    doc = NULL; else doc = (ora_document_read*) ora;

#define _BUFFER_LENGTH 256

struct _ora_stack_node_d;

typedef struct ora_document_read_d
{
    char magic;
    int flags;
    int error;
    unzFile file;
    struct _ora_stack_node_d* stack;
    struct _ora_stack_node_d* current;
    int width;
    int height;
    
} ora_document_read;

typedef struct ora_document_write_d
{
    char magic;
    int flags;
    int error;
    int width;
    int height;
    
} ora_document_write;

typedef struct 
{
    int x;
    int y;
    xmlChar* name;
} _ora_stack_stack;

typedef struct 
{
    int x;
    int y;
    xmlChar* name;
    xmlChar* src;
    float opacity;
} _ora_stack_layer;

typedef struct _ora_stack_node_d
{
    struct _ora_stack_node_d* parent;    // parent node
    struct _ora_stack_node_d* children;  // first child node
    struct _ora_stack_node_d* sibling;   // next sibling node
    int type;
    void* data;
} _ora_stack_node;

xmlChar* _ora_get_as_string(xmlNodePtr node, char* attribute) {

    xmlChar *val = xmlGetProp(node, (const xmlChar *) attribute);
    if (!val) 
        return NULL;

    return val;
}

float _ora_get_as_float(xmlNodePtr node, char* attribute) {

    xmlChar *val = xmlGetProp(node, (const xmlChar *) attribute);
    if (!val) 
        return 0;

    float f = atof((char*) val);

    xmlFree(val);
    return f;
}

int _ora_get_as_int(xmlNodePtr node, char* attribute) {

    xmlChar *val = xmlGetProp(node, (const xmlChar *) attribute);
    if (!val) 
        return 0;

    int f = atoi((char*) val);
    xmlFree(val);
    return f;
}

int _ora_get_as_boolean(xmlNodePtr node, char* attribute) {

    xmlChar *val = xmlGetProp(node, (const xmlChar *) attribute);
    if (!val) 
        return 0;

    int f = strcasecmp((char*) val, "true") == 0;

    xmlFree(val);
    return f;
}

void _ora_free_stack(_ora_stack_node* node) 
{
    _ora_stack_node* child;

    if (!node)
        return;

    for (child = node->children; child; child = child->sibling) 
    {
        _ora_free_stack(child);
    }

    node->children = NULL;

    if (node->data)
    {
        switch (node->type) 
        {
        case ORA_TYPE_STACK:
            if (((_ora_stack_stack*)node->data)->name)
                xmlFree(((_ora_stack_stack*)node->data)->name);
            break;
        case ORA_TYPE_LAYER:
            if (((_ora_stack_layer*)node->data)->name)
                xmlFree(((_ora_stack_layer*)node->data)->name);
            if (((_ora_stack_layer*)node->data)->src)
                xmlFree(((_ora_stack_layer*)node->data)->src);
            break;
        }
        free(node->data);
        node->data = NULL;
    }

    node->parent = NULL;


    free(node);

}

_ora_stack_node* _ora_stack_traverse(xmlNode * node, _ora_stack_node* parent, int* error)
{
    xmlNode *cur_node = NULL;
    _ora_stack_node* fnode = NULL;
    _ora_stack_node* snode = NULL;
    _ora_stack_node* pnode = NULL;
    _ora_stack_node* cnode = NULL;
    void* data = NULL;
    int type = 100;

    for (cur_node = node; cur_node; cur_node = cur_node->next) 
    {
        if (cur_node->type != XML_ELEMENT_NODE) 
            continue;

        if (xmlStrcmp(cur_node->name, (const xmlChar *)"stack") == 0) 
        {
            type = ORA_TYPE_STACK;
            if (parent && parent->type == ORA_TYPE_LAYER)
            {
                ORA_DEBUG("Error: Invalid parent for node stack.");
                if (error) *error = ORA_ERROR_STACK;
            }

            data = malloc(sizeof(_ora_stack_stack));
            ((_ora_stack_stack*)data)->x = _ora_get_as_int(cur_node, "x");
            ((_ora_stack_stack*)data)->y = _ora_get_as_int(cur_node, "y");
            ((_ora_stack_stack*)data)->name = _ora_get_as_string(cur_node, "name");
        } else
        if (xmlStrcmp(cur_node->name, (const xmlChar *)"layer") == 0) 
        {
            type = ORA_TYPE_LAYER;
            if (!parent || parent->type != ORA_TYPE_STACK)
            {
                ORA_DEBUG("Error: Invalid parent for node layer.");
                if (error) *error = ORA_ERROR_STACK;
            }
            data = malloc(sizeof(_ora_stack_layer));
            ((_ora_stack_layer*)data)->x = _ora_get_as_int(cur_node, "x");
            ((_ora_stack_layer*)data)->y = _ora_get_as_int(cur_node, "y");
            ((_ora_stack_layer*)data)->name = _ora_get_as_string(cur_node, "name");
            ((_ora_stack_layer*)data)->src = _ora_get_as_string(cur_node, "src");
            ((_ora_stack_layer*)data)->opacity = _ora_get_as_float(cur_node, "opacity");

            if (!((_ora_stack_layer*)data)->src) 
            {
                
                ORA_DEBUG("Error: Layer source specification required.");
                if (error) *error = ORA_ERROR_STACK;
            }

        } else
        if (xmlStrcmp(cur_node->name, (const xmlChar *)"filter") == 0) 
        {
            ORA_DEBUG("Warning: filters unsupported at the moment. Skipping.");
            continue;
        }
        ORA_DEBUG("XML parsing: element %s\n", cur_node->name);

        snode = (_ora_stack_node*) malloc(sizeof(_ora_stack_node));
        snode->type = type;
        snode->data = data;
        snode->parent = parent;
        snode->sibling = NULL;
        snode->children = NULL;

        if (cur_node->children)
        {
            cnode = _ora_stack_traverse(cur_node->children, snode, error);
            snode->children = cnode;

        }

        if (pnode)
            pnode->sibling = snode;

        pnode = snode;


        if (!fnode)
            fnode = snode;
    }

    return fnode;
}

_ora_stack_node* _ora_parse_stack(unzFile zip, int* width, int* height, int* error) 
{
    unz_file_info file_info;
    char buffer[_BUFFER_LENGTH];
    char* stack_buffer = NULL;
    int stack_buffer_length = 0;
    xmlDocPtr xml = NULL;
    xmlNodePtr node = NULL;
    _ora_stack_node* stack;
    int int_error = 0;

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

    xml = xmlReadMemory(stack_buffer, stack_buffer_length, "stack.xml", NULL, 0);

    if (xml == NULL ) 
    {
        ORA_DEBUG("stack.xml not parsed successfully. \n");
        if (error) *error = ORA_ERROR_STACK;
        return NULL;
    }

    free(stack_buffer);

    node = xmlDocGetRootElement(xml);

    if (xmlStrcmp(node->name, (const xmlChar *)"image") != 0) 
    {
        xmlFreeDoc(xml);
        ORA_DEBUG("stack.xml not valid. \n");
        if (error) *error = ORA_ERROR_STACK;
        return NULL;
    }

    stack = _ora_stack_traverse(node->children, NULL, &int_error);

    if (error)
        *error = int_error;

    *width = _ora_get_as_int(node, "w");
    *height = _ora_get_as_int(node, "h");

    xmlFreeDoc(xml);

    if (int_error)
    {
        _ora_free_stack(stack);
        return NULL;
    }


    if (*width < 1 || *height < 1)
    {
        _ora_free_stack(stack);
        ORA_DEBUG("Illegal image dimensions: %dx%d. \n", *width, *height);
        if (error) *error = ORA_ERROR_STACK;
        return NULL;
    }


    return stack;
    //TODO: call this in ora_cleanup() - xmlCleanupParser();

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

extern int ora_open(const char* filename, int flags, ORA* ora)
{
    int error = 0;

    if (!ora)
        return ORA_ERROR;

    ORA_DEBUG("Opening file %s\n", filename);

    if (flags & ORA_FILE_READ)
        *ora = _ora_read_file(filename, flags, &error);

    if (error)
        return error;

    return ORA_OK;
}


extern int ora_close(ORA ora) 
{
    ora_document_read* read_struct;

    if (!ora)
        return -1;

    ORA_CLEAR_ERROR(ora);

    ORA_DEBUG("Closing file\n");

    if (((ora_document_read*)ora)->magic == _ORA_MAGIC_READ) {

        read_struct = (ora_document_read*)ora;

        unzClose(read_struct->file);
        _ora_free_stack(read_struct->stack);
        read_struct->stack = NULL;
        read_struct->current = NULL;
        read_struct->file = NULL;

        free(read_struct);
    }


}


extern int ora_stack_reset(ORA ora) 
{
    ora_document_read* read_struct;

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

extern int ora_read_layer(ORA ora, ubyte** data, ora_rectangle* geometry, int* format, progress_callback callback)
{
    unz_file_info file_info;
    ora_document_read* read_struct;
    _ora_stack_layer* layer_data;
    unzFile zip;

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

    if (ora_read_raster((ora_document*)ora, zip, data, &(geometry->width), &(geometry->height), format, callback) < 0)
    {
        ORA_SET_ERROR(ora, ORA_ERROR_READ);
        return ORA_ERROR;
    }

    if (geometry) 
    {
        geometry->x = layer_data->x;
        geometry->y = layer_data->y;
    }

    if (unzCloseCurrentFile (zip) != UNZ_OK)
    {
        ORA_SET_ERROR(ora, ORA_ERROR_CORRUPTED);
        return ORA_ERROR;
    }


    return ORA_OK;
}

extern int ora_error(ORA ora) 
{
    if (!ora)
        return 0;

    if (!(((ora_document*) ora)->magic == _ORA_MAGIC_READ || ((ora_document*) ora)->magic == _ORA_MAGIC_WRITE))
        return 0;

    return ((ora_document*) ora)->error;
}

