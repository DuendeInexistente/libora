
#include <stdio.h>
#include <stdlib.h>
#include <expat.h>
#include <string.h>
#include "stack.h"
#include "stack_parser.h"
#include "utils/stringbuffer.h"

typedef struct __ora_parser_state
{
    _ora_stack_node* stack;
    _ora_stack_node* parent;
    _ora_stack_node* current;
    int width;
    int height;
    char* name;
} _ora_parser_state;

char* clone_xml_string(const XML_Char* src)
{
    char* result;

    if (!src)
        return NULL;

    result = (char*) malloc(sizeof(XML_Char) * (strlen(src) + 1));

    strcpy(result, src);

    return result;
}

void* stack_image_handle_open(void *userData , int arg_w  , int arg_h  , const XML_Char* arg_name  ) 
{
    _ora_parser_state* state = (_ora_parser_state*) userData;

    state->width = arg_w;
    state->height = arg_h;

    return state;
}

void* stack_image_handle_close(void *userData, XML_Char* text)
{
    _ora_parser_state* state = (_ora_parser_state*) userData;

    return state;
}

void* stack_stack_handle_open(void *userData , int arg_x  , int arg_y  , const XML_Char* arg_name  )
{
    _ora_parser_state* state = (_ora_parser_state*) userData;

    _ora_stack_node* node = (_ora_stack_node*) malloc(sizeof(_ora_stack_node));
    _ora_stack_stack* stack = (_ora_stack_stack*) malloc(sizeof(_ora_stack_stack));
printf("parent \n");
    // is this the master stack?
    if (!state->parent)
    {

        state->stack = node;
        state->current = NULL;
        state->parent = node;
        node->parent = NULL;
    } else
    {
        // if not master stack
        node->parent = state->parent;

        // is this a sibling ...
        if (state->current) {
            // attach to the end of the sibling list
            state->current->sibling = node;
            state->current = node;
        } else 
        {
            // start a list
            state->parent->children = node;
            state->current = node;
        }

        state->parent = node;
    }

    node->sibling = NULL;
    node->children = NULL;

    node->type = ORA_TYPE_STACK;

    stack->x = arg_x;
    stack->y = arg_y;
    stack->name = clone_xml_string(arg_name);
    node->data = stack;

    return state;
}

void* stack_stack_handle_close(void *userData, XML_Char* text)
{
    _ora_parser_state* state = (_ora_parser_state*) userData;

    // step up
    state->current = NULL;

    return state;
}

void* stack_layer_handle_open(void *userData , int arg_x  , int arg_y  , const XML_Char* arg_name  , const XML_Char* arg_src  , float arg_opacity  )
{
    _ora_parser_state* state = (_ora_parser_state*) userData;

    _ora_stack_node* node = (_ora_stack_node*) malloc(sizeof(_ora_stack_node));
    _ora_stack_layer* stack = (_ora_stack_layer*) malloc(sizeof(_ora_stack_layer));

    node->parent = state->parent;

    // is this a sibling ...
    if (state->current) {
        // attach to the end of the sibling list
        state->current->sibling = node;
        state->current = node;
    } else 
    {
        // start a list
        state->parent->children = node;
        state->current = node;
    }


    node->sibling = NULL;
    node->children = NULL;

    node->type = ORA_TYPE_LAYER;

    stack->bounds.x = arg_x;
    stack->bounds.y = arg_y;
    stack->bounds.width = -1;
    stack->bounds.height = -1;
    stack->name = clone_xml_string(arg_name);
    stack->src = clone_xml_string(arg_src);
    stack->opacity = arg_opacity;

    node->data = stack;

    return state;
}

void* stack_layer_handle_close(void *userData, XML_Char* text)
{
    _ora_parser_state* state = (_ora_parser_state*) userData;

    return state;
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
                free(((_ora_stack_stack*)node->data)->name);
            break;
        case ORA_TYPE_LAYER:
            if (((_ora_stack_layer*)node->data)->name)
                free(((_ora_stack_layer*)node->data)->name);
            if (((_ora_stack_layer*)node->data)->src)
                free(((_ora_stack_layer*)node->data)->src);
            break;
        }
        free(node->data);
        node->data = NULL;
    }

    node->parent = NULL;


    free(node);

}

void _ora_stack_bounds(_ora_stack_node* node, ora_rectangle* bounds)
{
    _ora_stack_node* cur_node = NULL;
    ora_rectangle local_bounds, child_bounds;

    local_bounds.x = 0;
    local_bounds.y = 0;
    local_bounds.width = 0;
    local_bounds.height = 0;

    void* data = NULL;

    for (cur_node = node; cur_node; cur_node = cur_node->sibling) 
    {
        int x = 0;
        int y = 0;
        //data = cur_node->data;
        switch (cur_node->type)
        {
        case ORA_TYPE_STACK:
            if (cur_node->children)
            {
                _ora_stack_bounds(cur_node->children, &child_bounds);
                local_bounds.width = MAX(child_bounds.x + child_bounds.width + x, local_bounds.width);
                local_bounds.height = MAX(child_bounds.y + child_bounds.height + y, local_bounds.height);
            }
            break;
        case ORA_TYPE_LAYER:
            local_bounds.width = MAX(x + ((_ora_stack_layer*)data)->bounds.width, local_bounds.width);
            local_bounds.height = MAX(y + ((_ora_stack_layer*)data)->bounds.height, local_bounds.height);

            break;
        }
    }
}


_ora_stack_node* _ora_xml_to_stack(char* source, int len, ora_rectangle* bounds, int* error)
{
    _ora_stack_node* result;
    int perror;
    _ora_parser_state* state = (_ora_parser_state*) malloc(sizeof(_ora_parser_state));
    state->stack = NULL;
    state->current = NULL;
    state->parent = NULL;
    state->name = NULL;

    perror = stack_parse(source, len, state);

    result = state->stack;

    if (perror) {
        _ora_free_stack(result);
        free(state);
        return NULL;
    }

    if (bounds) {
        bounds->x = 0;
        bounds->y = 0;
        bounds->width = state->width;
        bounds->height = state->height;
    }

    free(state);

    return result;
}

void _ora_stack_to_xml_recursive(_ora_stack_node* node, sb_buffer* buffer)
{
    _ora_stack_node* cur_node = NULL;

    void* data = NULL;

    for (cur_node = node; cur_node; cur_node = cur_node->sibling) 
    {
        int x = 0;
        int y = 0;
        data = cur_node->data;

        switch (cur_node->type)
        {
        case ORA_TYPE_STACK:
            sb_print(buffer, "<stack ");

            if (((_ora_stack_stack*)data)->name)
            {
                sb_print(buffer, "name=\"");
                sb_print(buffer, ((_ora_stack_stack*)data)->name);
                sb_print(buffer, "\" ");
            }

            x = ((_ora_stack_stack*)data)->x;
            y = ((_ora_stack_stack*)data)->y;
            sb_printf(buffer, "x=\"%d\" y=\"%d\">\n", x, y);

            if (cur_node->children)
            {
                _ora_stack_to_xml_recursive(cur_node->children, buffer);
            }

            sb_print(buffer, "</stack>\n");

            break;
        case ORA_TYPE_LAYER:
            sb_print(buffer, "<layer ");

            if (((_ora_stack_layer*)data)->name)
            {
                sb_print(buffer, "name=\"");
                sb_print(buffer, ((_ora_stack_layer*)data)->name);
                sb_print(buffer, "\" ");
            }

            x = ((_ora_stack_layer*)data)->bounds.x;
            y = ((_ora_stack_layer*)data)->bounds.y;
            sb_printf(buffer, "x=\"%d\" y=\"%d\" src=\"%s\" opacity=\"%f\" />\n", x, y, ((_ora_stack_layer*)data)->src, ((_ora_stack_layer*)data)->opacity);

            break;
        }

    }

}

char* _ora_stack_to_xml(_ora_stack_node* node, ora_rectangle bounds, int* error) 
{
    sb_buffer* buffer;
    char* xml;

    if (error)
        *error = 0;

    buffer = sb_create();

    sb_printf(buffer, "<image w=\"%d\" h=\"%d\">\n", bounds.width, bounds.height);

    _ora_stack_to_xml_recursive(node, buffer);

    sb_print(buffer, "</image>");

    xml = sb_string(buffer);

    sb_free(buffer);

    return xml;
}


