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

#ifndef _ORASTACK_H
#define _ORASTACK_H

#include "../ora.h"
#include "../internal.h"

#define ORA_TYPE_STACK 1
#define ORA_TYPE_LAYER 2
#define ORA_TYPE_FILTER 3

struct _ora_stack_node_d;

typedef struct 
{
    int x;
    int y;
    char* name;
} _ora_stack_stack;

typedef struct 
{
    ora_rectangle bounds;
    char* name;
    char* src;
    float opacity;
    int visibility;
} _ora_stack_layer;

typedef struct _ora_stack_node_d
{
    struct _ora_stack_node_d* parent;    // parent node
    struct _ora_stack_node_d* children;  // first child node
    struct _ora_stack_node_d* sibling;   // next sibling node
    int type;
    void* data;
} _ora_stack_node;

void _ora_free_stack(_ora_stack_node* node);

_ora_stack_node* _ora_xml_to_stack(char* source, int len, ora_rectangle* bounds, int* error);

char* _ora_stack_to_xml(_ora_stack_node* node, ora_rectangle bounds, int* error);

#endif
