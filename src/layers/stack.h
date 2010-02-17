/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * Copyright 2010 libora developers. All rights reserved. 
 * 
 * For more details see the LICENSE.txt file.
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
