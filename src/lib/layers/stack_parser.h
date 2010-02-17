
/*
 * OpenRaster stack XML parser
 * Generated with genesx alpha
 *
 * This is an auto-generated file! Do not edit!
 *
 */

#ifndef __H_STACK_PARSER
#define __H_STACK_PARSER

int stack_parse(char* source, int len, void* data);

void* stack_image_handle_open(void *userData , int arg_w  , int arg_h  , const XML_Char* arg_name  );
void* stack_image_handle_close(void *userData, XML_Char* text);
void* stack_stack_handle_open(void *userData , int arg_x  , int arg_y  , const XML_Char* arg_name  );
void* stack_stack_handle_close(void *userData, XML_Char* text);
void* stack_layer_handle_open(void *userData , int arg_x  , int arg_y  , const XML_Char* arg_name  , const XML_Char* arg_src  , float arg_opacity  );
void* stack_layer_handle_close(void *userData, XML_Char* text);

#endif 
