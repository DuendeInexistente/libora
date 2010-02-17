
/*
 * OpenRaster stack XML parser
 * Generated with genesx alpha
 *
 * This is an auto-generated file! Do not edit!
 *
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <expat.h>

#include "stack_parser.h"

#define  STATE_IMAGE 0
#define  STATE__START 1
#define  STATE_STACK 2
#define  STATE_LAYER 3

typedef struct _stack_stack 
{
    int element;
    XML_Char* text;
    struct _stack_stack* prev;
    XML_CharacterDataHandler text_handler;
    XML_StartElementHandler start_handler;
    XML_EndElementHandler end_handler;
    int* counters;
} stack_stack;

typedef struct _stack_state 
{
    stack_stack* stack;
    char* error;
    int error_line;
    int error_offset;
    int state;
    XML_Parser parser;
    void* userData;
} stack_state;

void _stack_image_handle(stack_state* state, const XML_Char *name, const XML_Char **atts);
void _stack_image_begin(void *userData, const XML_Char *name, const XML_Char **atts);
void _stack_image_end(void *userData, const XML_Char *name);
void _stack_image_text (void *userData, const XML_Char *s, int len);
void _stack__start_handle(stack_state* state, const XML_Char *name, const XML_Char **atts);
void _stack__start_begin(void *userData, const XML_Char *name, const XML_Char **atts);
void _stack__start_end(void *userData, const XML_Char *name);
void _stack__start_text (void *userData, const XML_Char *s, int len);
void _stack_stack_handle(stack_state* state, const XML_Char *name, const XML_Char **atts);
void _stack_stack_begin(void *userData, const XML_Char *name, const XML_Char **atts);
void _stack_stack_end(void *userData, const XML_Char *name);
void _stack_stack_text (void *userData, const XML_Char *s, int len);
void _stack_layer_handle(stack_state* state, const XML_Char *name, const XML_Char **atts);
void _stack_layer_begin(void *userData, const XML_Char *name, const XML_Char **atts);
void _stack_layer_end(void *userData, const XML_Char *name);
void _stack_layer_text (void *userData, const XML_Char *s, int len);

stack_stack* _stack_stack_push(stack_state* state, int id, int counters, XML_StartElementHandler start_handler, XML_EndElementHandler end_handler, XML_CharacterDataHandler text_handler)
{
    stack_stack* element;

    element = (stack_stack*) malloc(sizeof(stack_stack));

    element->element = id;
    element->prev = state->stack;
    element->text = NULL;
    element->start_handler = start_handler;
    element->end_handler = end_handler;
    element->text_handler = text_handler;
    element->counters = (counters) ? (int*) malloc(sizeof(int) * counters) : NULL;

    if (element->counters)
        memset(element->counters, 0, sizeof(int) * counters);

    state->stack = element;

    XML_SetElementHandler(state->parser, start_handler, end_handler);
    XML_SetCharacterDataHandler(state->parser, text_handler);

    return element;
}

void _stack_stack_pop(stack_state* state)
{
    stack_stack* element;
    XML_StartElementHandler start_handler = NULL;
    XML_EndElementHandler end_handler = NULL;
    XML_CharacterDataHandler text_handler = NULL;

    element = state->stack;

    if (element) {

        state->stack = element->prev;

        if (element->text)
            free(element->text);
        if (element->counters)
            free(element->counters);

        element->text = NULL;

        free(element);
    }
    if (state->stack) {
        start_handler = state->stack->start_handler;
        end_handler = state->stack->end_handler;
        text_handler = state->stack->text_handler;
    }

    XML_SetElementHandler(state->parser, start_handler, end_handler);
    XML_SetCharacterDataHandler(state->parser, text_handler);
}

stack_stack* _stack_stack_peek(stack_state* state)
{
    return state->stack;
}

void _stack_stop(stack_state* state, const char* error)
{
    state->error_line = XML_GetCurrentLineNumber(state->parser);
    state->error_offset = XML_GetCurrentColumnNumber(state->parser);
    XML_StopParser(state->parser, 0);
    state->error = (char*) malloc(sizeof(char) * (strlen(error) + 1));
    strcpy(state->error, error);
}

void _stack_stradd(XML_Char** src, const XML_Char* add, int len) 
{
    XML_Char* dst;
    int offset = 0;

    if (!*src) 
    {
        dst = (XML_Char*) malloc(sizeof(XML_Char) * (len+1));
        offset = 0;
    } else {
        offset = strlen(*src);
        dst = (XML_Char*) realloc(*src, offset + sizeof(XML_Char) * (len+1));
    }

    memcpy(dst + offset, add, len);

    dst[offset+len+1] = '\0';

    *src = dst;
}

int _stack_strcmp(const XML_Char* str1, const char* str2) 
{
    return strcmp(str1, str2) == 0 ? 1 : 0;
}

int _stack_hasarg(const XML_Char** arguments, const char* name) 
{
    int i = 0;
    while (1)
    {
        if (!arguments[i])
            return 0;
        if (_stack_strcmp(arguments[i], name))
            return 1;
        i += 2;
    }
}

const XML_Char* _stack_get_string(const XML_Char** arguments, const char* name) 
{
    int i = 0;
    while (1)
    {
        if (!arguments[i])
            return NULL;
        if (_stack_strcmp(arguments[i], name))
            return arguments[i+1];
        i += 2;
    }
}

float _stack_get_float(const XML_Char** arguments, const char* name) 
{

    const XML_Char* val = _stack_get_string(arguments, name);
    if (!val) 
        return 0;

    float f = atof((char*) val);

    return f;
}

int _stack_get_int(const XML_Char** arguments, const char* name) 
{

    const XML_Char* val = _stack_get_string(arguments, name);
    if (!val) 
        return 0;

    int f = atoi((char*) val);
    return f;
}

int _stack_get_boolean(const XML_Char** arguments, const char* name) 
{

    const XML_Char* val = _stack_get_string(arguments, name);
    if (!val) 
        return 0;

    int f = strcasecmp((char*) val, "true") == 0;

    return f;
}


/*
 * ---- event handlers for image (layer) tag ----
 * Root tag for stack
 */
void _stack_image_handle(stack_state* state, const XML_Char *name, const XML_Char **atts) 
{
        int arg_w = 0;

        int arg_h = 0;

        XML_Char* arg_name = NULL;


        if (_stack_hasarg(atts, "w"))
        {
            arg_w  = _stack_get_int(atts, "w");
        } 
        else {
            _stack_stop(state, "Attribute w is mandatory for element image");
            return;
        }
        if (_stack_hasarg(atts, "h"))
        {
            arg_h  = _stack_get_int(atts, "h");
        } 
        else {
            _stack_stop(state, "Attribute h is mandatory for element image");
            return;
        }
        if (_stack_hasarg(atts, "name"))
        {
            arg_name  = (XML_Char*) _stack_get_string(atts, "name");
        } 

    state->userData = stack_image_handle_open(state->userData , arg_w  , arg_h  , arg_name  );
}


void _stack_image_begin(void *userData, const XML_Char *name, const XML_Char **atts) 
{

#ifdef DEBUG
    printf("XML (image) new element %s \n", name);
#endif

    stack_state* state = (stack_state*) userData;
    if (_stack_strcmp(name, "stack")) 
    {
        state->stack->counters[0]++;

        if ( state->stack->counters[0] > 1 )
        {
            _stack_stop(state, "Maximum number of stack elements exceeded.");
            return;
        }

        _stack_stack_push(state, STATE_STACK, 2, _stack_stack_begin, _stack_stack_end, _stack_stack_text);

        _stack_stack_handle(state, name, atts);

        return;
    }

    _stack_stop(state, "Unexpected element.");
    return;
}

void _stack_image_end(void *userData, const XML_Char *name) 
{
    stack_state* state = (stack_state*) userData;
    if (!_stack_strcmp(name, "image")) 
    {
        _stack_stop(state, "Unexpected element closure.");
        return;
    }

    if ( state->stack->counters[0] < 1 )
    {
        _stack_stop(state, "Not enough elements of type stack .");
        return;
    }
    state->userData = stack_image_handle_close(state->userData, state->stack->text);
    _stack_stack_pop(state);

}

void _stack_image_text (void *userData, const XML_Char *s, int len)
{
}



/*
 * ---- event handlers for _start (layer) tag ----
 * 
 */

void _stack__start_begin(void *userData, const XML_Char *name, const XML_Char **atts) 
{

#ifdef DEBUG
    printf("XML (_start) new element %s \n", name);
#endif

    stack_state* state = (stack_state*) userData;
    if (_stack_strcmp(name, "image")) 
    {
        state->stack->counters[0]++;

        if ( state->stack->counters[0] > 1 )
        {
            _stack_stop(state, "Maximum number of image elements exceeded.");
            return;
        }

        _stack_stack_push(state, STATE_IMAGE, 1, _stack_image_begin, _stack_image_end, _stack_image_text);

        _stack_image_handle(state, name, atts);

        return;
    }

    _stack_stop(state, "Unexpected element.");
    return;
}

void _stack__start_end(void *userData, const XML_Char *name) 
{
    stack_state* state = (stack_state*) userData;
    if (!_stack_strcmp(name, "_start")) 
    {
        _stack_stop(state, "Unexpected element closure.");
        return;
    }

    if ( state->stack->counters[0] < 1 )
    {
        _stack_stop(state, "Not enough elements of type image .");
        return;
    }
    _stack_stack_pop(state);

}

void _stack__start_text (void *userData, const XML_Char *s, int len)
{
}



/*
 * ---- event handlers for stack (layer) tag ----
 * Stack element
 */
void _stack_stack_handle(stack_state* state, const XML_Char *name, const XML_Char **atts) 
{
        int arg_x = 0;

        int arg_y = 0;

        XML_Char* arg_name = NULL;


        if (_stack_hasarg(atts, "x"))
        {
            arg_x  = _stack_get_int(atts, "x");
        } 
        if (_stack_hasarg(atts, "y"))
        {
            arg_y  = _stack_get_int(atts, "y");
        } 
        if (_stack_hasarg(atts, "name"))
        {
            arg_name  = (XML_Char*) _stack_get_string(atts, "name");
        } 

    state->userData = stack_stack_handle_open(state->userData , arg_x  , arg_y  , arg_name  );
}


void _stack_stack_begin(void *userData, const XML_Char *name, const XML_Char **atts) 
{

#ifdef DEBUG
    printf("XML (stack) new element %s \n", name);
#endif

    stack_state* state = (stack_state*) userData;
    if (_stack_strcmp(name, "layer")) 
    {
        state->stack->counters[0]++;


        _stack_stack_push(state, STATE_LAYER, 0, _stack_layer_begin, _stack_layer_end, _stack_layer_text);

        _stack_layer_handle(state, name, atts);

        return;
    }
    if (_stack_strcmp(name, "stack")) 
    {
        state->stack->counters[1]++;


        _stack_stack_push(state, STATE_STACK, 2, _stack_stack_begin, _stack_stack_end, _stack_stack_text);

        _stack_stack_handle(state, name, atts);

        return;
    }

    _stack_stop(state, "Unexpected element.");
    return;
}

void _stack_stack_end(void *userData, const XML_Char *name) 
{
    stack_state* state = (stack_state*) userData;
    if (!_stack_strcmp(name, "stack")) 
    {
        _stack_stop(state, "Unexpected element closure.");
        return;
    }

    state->userData = stack_stack_handle_close(state->userData, state->stack->text);
    _stack_stack_pop(state);

}

void _stack_stack_text (void *userData, const XML_Char *s, int len)
{
}



/*
 * ---- event handlers for layer (layer) tag ----
 * Layer element
 */
void _stack_layer_handle(stack_state* state, const XML_Char *name, const XML_Char **atts) 
{
        int arg_x = 0;

        int arg_y = 0;

        XML_Char* arg_name = NULL;

        XML_Char* arg_src = NULL;

        float arg_opacity = 0;


        if (_stack_hasarg(atts, "x"))
        {
            arg_x  = _stack_get_int(atts, "x");
        } 
        if (_stack_hasarg(atts, "y"))
        {
            arg_y  = _stack_get_int(atts, "y");
        } 
        if (_stack_hasarg(atts, "name"))
        {
            arg_name  = (XML_Char*) _stack_get_string(atts, "name");
        } 
        if (_stack_hasarg(atts, "src"))
        {
            arg_src  = (XML_Char*) _stack_get_string(atts, "src");
        } 
        else {
            _stack_stop(state, "Attribute src is mandatory for element layer");
            return;
        }
        if (_stack_hasarg(atts, "opacity"))
        {
            arg_opacity  = _stack_get_float(atts, "opacity");
        } 

    state->userData = stack_layer_handle_open(state->userData , arg_x  , arg_y  , arg_name  , arg_src  , arg_opacity  );
}


void _stack_layer_begin(void *userData, const XML_Char *name, const XML_Char **atts) 
{

#ifdef DEBUG
    printf("XML (layer) new element %s \n", name);
#endif

    stack_state* state = (stack_state*) userData;

    _stack_stop(state, "Unexpected element.");
    return;
}

void _stack_layer_end(void *userData, const XML_Char *name) 
{
    stack_state* state = (stack_state*) userData;
    if (!_stack_strcmp(name, "layer")) 
    {
        _stack_stop(state, "Unexpected element closure.");
        return;
    }

    state->userData = stack_layer_handle_close(state->userData, state->stack->text);
    _stack_stack_pop(state);

}

void _stack_layer_text (void *userData, const XML_Char *s, int len)
{
}



int stack_parse(char* source, int len, void* data)
{
    int error;
    XML_Parser parser = NULL;
    stack_state* state;

    parser = XML_ParserCreate((const XML_Char*) "UTF-8");

    state = (stack_state*) malloc(sizeof(stack_state));
    state->stack=NULL;
    state->error = NULL;
    state->parser = parser;
    state->userData = data;

    _stack_stack_push(state, STATE__START, 1, _stack__start_begin, _stack__start_end, _stack__start_text);

    XML_SetUserData(parser, state);

    error = XML_Parse(parser, source, len, 1);

    while (state->stack)
    {
        _stack_stack_pop(state);
    }

    if (state->error)
    {
        error = -1;
#ifdef DEBUG
        printf("XML parse error: %s (%d, %d)\n", state->error, state->error_line, state->error_offset);
        printf("XML buffer dump: %s\n", source);
#endif
        free(state->error);
    } else error = 0;

    free(state);

    XML_ParserFree(parser);

    return error;
}

