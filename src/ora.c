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

#include <stdlib.h>
#include <string.h>
#include <libxml/tree.h>
#include <libxml/parser.h>
#include <zlib.h>
#include <time.h>

#include "internal.h"
#include "zip/unzip.h"
#include "zip/zip.h"
#include "layers/layers.h"

#define ORA_COMMENT "Created with libora"

#define _GET_READ_DOCUMENT(ora, doc) if (!ora || (((ora_document*)ora)->magic) != _ORA_MAGIC_READ) \
    doc = NULL; else doc = (ora_document_read*) ora;

#define _GET_WRITE_DOCUMENT(ora, doc) if (!ora) doc = NULL; else if ((((ora_document*)ora)->magic) != _ORA_MAGIC_WRITE) \
    {doc = NULL; ORA_SET_ERROR(ora, ORA_ERROR_INVALID);} else doc = (ora_document_write*) ora;

#define _BUFFER_LENGTH 256

struct _ora_stack_node_d;

typedef struct ora_document_read_d
{
    char magic;
    int flags;
    int error;
    int width;
    int height;
    unzFile file;
    struct _ora_stack_node_d* stack;
    struct _ora_stack_node_d* current;
} ora_document_read;

typedef struct ora_document_write_d
{
    char magic;
    int flags;
    int error;
    int width;
    int height;
    zipFile file;
    struct _ora_stack_node_d* stack;
    struct _ora_stack_node_d* parent;
    struct _ora_stack_node_d* sibling;
    int layer_number;
} ora_document_write;

typedef struct 
{
    int x;
    int y;
    xmlChar* name;
} _ora_stack_stack;

typedef struct 
{
    ora_rectangle bounds;
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

void _ora_set_as_string(xmlNodePtr node, char* attribute, char* value) {

    xmlSetProp(node, (const xmlChar *) attribute, (const xmlChar *) value);

}

void _ora_set_as_float(xmlNodePtr node, char* attribute, float value) {
    char str[_BUFFER_LENGTH];

    sprintf(str, "%f", value);

    xmlSetProp(node, (const xmlChar *) attribute, (const xmlChar *) str);
}

void _ora_set_as_int(xmlNodePtr node, char* attribute, int value) {
    char str[_BUFFER_LENGTH];

    sprintf(str, "%d", value);

    xmlSetProp(node, (const xmlChar *) attribute, (const xmlChar *) str);
}

void _ora_set_as_boolean(xmlNodePtr node, char* attribute, int value) {
    xmlSetProp(node, (const xmlChar *) attribute, (const xmlChar *) (value ? "true" : "false"));
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

_ora_stack_node* _ora_xml_to_stack(xmlNode * node, _ora_stack_node* parent, int* error)
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
            ((_ora_stack_layer*)data)->bounds.x = _ora_get_as_int(cur_node, "x");
            ((_ora_stack_layer*)data)->bounds.y = _ora_get_as_int(cur_node, "y");
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
            cnode = _ora_xml_to_stack(cur_node->children, snode, error);
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

xmlNode* _ora_stack_to_xml(_ora_stack_node* node, ora_rectangle* bounds)
{
    _ora_stack_node* cur_node = NULL;
    xmlNodePtr xml_node = NULL;
    xmlNodePtr xml_node_n = NULL;
    xmlNodePtr xml_node_f = NULL;
    xmlNodePtr xml_node_c = NULL;
    ora_rectangle local_bounds, child_bounds;

    local_bounds.x = 0;
    local_bounds.y = 0;
    local_bounds.width = 0;
    local_bounds.height = 0;

    void* data = NULL;
    int type = 100;

    for (cur_node = node; cur_node; cur_node = cur_node->sibling) 
    {
        int x = 0;
        int y = 0;
        xml_node = NULL;
        data = cur_node->data;

        switch (cur_node->type)
        {
        case ORA_TYPE_STACK:
            xml_node = xmlNewNode(NULL, BAD_CAST "stack");

            if (((_ora_stack_stack*)data)->name)
                _ora_set_as_string(xml_node, "name", ((_ora_stack_stack*)data)->name);

            x = ((_ora_stack_stack*)data)->x;
            y = ((_ora_stack_stack*)data)->y;
            _ora_set_as_int(xml_node, "x", x);
            _ora_set_as_int(xml_node, "y", y);

            if (cur_node->children)
            {
                xml_node_c = _ora_stack_to_xml(cur_node->children, &child_bounds);
                xmlAddChildList(xml_node, xml_node_c);

                local_bounds.width = MAX(child_bounds.x + child_bounds.width + x, local_bounds.width);
                local_bounds.height = MAX(child_bounds.y + child_bounds.height + y, local_bounds.height);

            }

            break;
        case ORA_TYPE_LAYER:
            xml_node = xmlNewNode(NULL, BAD_CAST "layer");

            if (((_ora_stack_layer*)data)->name)
                _ora_set_as_string(xml_node, "name", ((_ora_stack_layer*)data)->name);

            _ora_set_as_string(xml_node, "src", ((_ora_stack_layer*)data)->src);

            x = ((_ora_stack_layer*)data)->bounds.x;
            y = ((_ora_stack_layer*)data)->bounds.y;
            _ora_set_as_int(xml_node, "x", x);
            _ora_set_as_int(xml_node, "y", y);
            _ora_set_as_float(xml_node, "opacity", ((_ora_stack_layer*)data)->opacity);

            local_bounds.width = MAX(x + ((_ora_stack_layer*)data)->bounds.width, local_bounds.width);
            local_bounds.height = MAX(y + ((_ora_stack_layer*)data)->bounds.height, local_bounds.height);
            break;
        }



        if (xml_node_n)
            xmlAddNextSibling(xml_node_n, xml_node);

        xml_node_n = xml_node;

        if (!xml_node_f)
            xml_node_f = xml_node;
    }

    *bounds = local_bounds;
    return xml_node_f;
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

    stack = _ora_xml_to_stack(node->children, NULL, &int_error);

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

void _ora_write_stack(ora_document_write* doc) 
{
    zipFile zip;
    zip_fileinfo info;
    xmlDocPtr xml = NULL;
    xmlNodePtr root = NULL;
    xmlNodePtr node = NULL;
    _ora_stack_node* stack;
    int int_error = 0;
    xmlChar* xmlbuff = NULL;
    int buffersize;
    ora_rectangle bounds;

    zip = doc->file;

    xml = xmlNewDoc(BAD_CAST "1.0");
    root = xmlNewNode(NULL, BAD_CAST "image");

    node = _ora_stack_to_xml(doc->stack, &bounds);

    _ora_set_as_int(root, "w", doc->width > -1 ? doc->width : bounds.width);
    _ora_set_as_int(root, "h", doc->height > -1 ? doc->height : bounds.height);

    xmlDocSetRootElement(xml, root);

    xmlAddChild(root, node);

    xmlDocDumpFormatMemory(xml, &xmlbuff, &buffersize, 1);
    //printf("%s", (char *) xmlbuff);

    xmlFreeDoc(xml);

    ORA_DEBUG("Writing layer stack.\n");

    info = _ora_default_fileinfo();

    if (zipOpenNewFileInZip (zip, "stack.xml", &info, NULL, 0, NULL, 0, NULL, Z_DEFLATED, Z_DEFAULT_COMPRESSION) != ZIP_OK)
    {
        xmlFree(xmlbuff);
        ORA_SET_ERROR(doc, ORA_ERROR_WRITE);
        return;
    }

    if (zipWriteInFileInZip(zip, xmlbuff, buffersize) < 0) 
    {
        xmlFree(xmlbuff);
        zipClose(zip, NULL);
        ORA_SET_ERROR(doc, ORA_ERROR_WRITE);
        return;
    }

    xmlFree(xmlbuff);

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

    zip = zipOpen (filename, APPEND_STATUS_CREATE);

    if (!zip) 
    {
        if (error) *error = ORA_ERROR_WRITE;
        return NULL;
    }

    xmlDocPtr xml = NULL;
    xmlNodePtr node = NULL;
    _ora_stack_node* stack;
    int int_error = 0;

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
        return -1;

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
        read_struct->stack = NULL;
        read_struct->current = NULL;
        read_struct->file = NULL;

        free(write_struct);

        return ORA_OK;
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

extern int ora_read_layer(ORA ora, ubyte** data, ora_rectangle* geometry, int* format, float* opacity, progress_callback callback)
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
        geometry->x = layer_data->bounds.x;
        geometry->y = layer_data->bounds.y;
    }

    if (opacity)
        *opacity = layer_data->opacity;

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

extern int ora_write_layer(ORA ora, const char* name, ora_rectangle geometry, int format, float opacity, ubyte* data, progress_callback callback)
{
    ora_document_write* write_struct;
    zip_fileinfo info;
    _ora_stack_node* node;
    char src[_BUFFER_LENGTH];

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

    if (zipOpenNewFileInZip (write_struct->file, src, &info, NULL, 0, NULL, 0, NULL, Z_DEFLATED, Z_DEFAULT_COMPRESSION) != ZIP_OK)
    {
        ORA_SET_ERROR(write_struct, ORA_ERROR_WRITE);
        return ORA_ERROR;
    }

    if (ora_write_raster(write_struct, write_struct->file, data, geometry.width, geometry.height, format, callback) != ORA_OK) 
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

