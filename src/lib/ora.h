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

/**
 * \mainpage LibORA documentation
 *
 * \section introduction Introduction
 *
 * This series of documents describes the code of the OpenRaster Reference Library or libora. 
 * You can find more about the project by checking 
 * <a href="http://create.freedesktop.org/wiki/OpenRaster/Reference_Library">this link</a>.
 *
 * \section license License
 * <pre>
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
 * </pre>
 */
/** \file ora.h
 * 
 * This file contains the public libora API. 
 * Only this file should be installed for using the library in some application. 
*/

#ifndef _ORAAPI_H
#define _ORAAPI_H

/// @brief Definition of the ORA mimetyle string. 
#define ORA_MIMETYPE "image/openraster"

/// @brief This value is returned when a function is completed successfuly. 
#define ORA_OK 0

/// @brief This value is returned when a function encountered an error. 
#define ORA_ERROR -1

/// @brief Open ORA file for reading. 
#define ORA_FILE_READ 1

/// @brief Open ORA file for writing. 
#define ORA_FILE_WRITE 2

/// @brief Do not read stack immediately 
/// Do not read the stack information immediately (lazy loading). 
/// Useful for extracting thumbnails or accessing other information that do not require stack.
#define ORA_FILE_READ_NO_STACK 4

/// @brief Null pointer error. 
#define ORA_ERROR_NULL 1

/// @brief Invalid document reference. 
#define ORA_ERROR_INVALID 2

/// @brief 	Corrupted data. 
#define ORA_ERROR_CORRUPTED 3

/// @brief Error while reading data. 
#define ORA_ERROR_READ 4 

/// @brief Missing data error. 
#define ORA_ERROR_MISSING 5

/// @brief 	libpng error (decoding/encoding layer) 
#define ORA_ERROR_PNGLIB 6

/// @brief Error while writing data. 
#define ORA_ERROR_WRITE 7

/// @brief 
#define ORA_ERROR_MODE 10

/// @brief Stack error. 
#define ORA_ERROR_STACK 11

/// @brief End of the stack. 
#define ORA_ERROR_STACK_END 12

/// @brief 	Illegal position in stack. 
#define ORA_ERROR_STACK_POSITION 13
//#define ORA_ERROR_STACK_ 10

/// @brief Go to the next sibling in the stack tree. 
#define ORA_NEXT_SIBLING 1

/// @brief 	Climb up in the stack tree. 
#define ORA_NEXT_CLIMB 2

/// @brief 	Go to the next element that is not a stack. 
#define ORA_NEXT_NO_STACK 4

/// @brief 	Go to the next element that is not a layer. 
#define ORA_NEXT_NO_LAYER 8

/// @brief Raster layer format. 
#define ORA_FORMAT_RASTER 1

/// @brief Vector layer format. 
#define ORA_FORMAT_VECTOR 2

/// @brief Text layer format. 
#define ORA_FORMAT_TEXT 4

/// @brief This is a background layer (the data is actually a pattern). 
#define ORA_FORMAT_BACKGROUND 128

/// @brief 
#define ORA_FORMAT_DOUBLE 256

/// @brief 	Raster format also contains an alpha channel. 
#define ORA_FORMAT_ALPHA 512

typedef char ubyte;

/// @brief Reference to a ORA document. 
typedef void* ORA;

/// Progress callback prototype
typedef void (*ora_progress_callback) (int progress);

//typedef void (*tile_read_callback) (int x, int y, ubyte* data);

//typedef ubyte* (*tile_write_callback) (int x, int y);

/// @brief This structure contains the layer geometry information (size and offset).
typedef struct _ora_rectangle
{
    int x; /**< x offset  */
    int y; /**< y offset  */
    int width; /**< width width of the layer  */
    int height; /**< height height of the layer */
} ora_rectangle;

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Open ORA document from disk. This function does not read the entire file but provides an reference, that can be used to perform individual access operations. 
 *
 *
 * @param filename 	- the filename string
 * @param flags 	- flags (legal values ORA_FILE_WRITE or ORA_FILE_READ and possibly ORA_FILE_READ_NO_STACK)
 * @param ora 	output parameter that gets initialized with internal ORA document data. Serves as a reference to a specific document. 
 */
extern int ora_open(const char* filename, int flags, ORA* ora);

/** @brief Resets the current stack element pointer to the first element in the stack.
 *
 * @param ora 	the document 
 */
extern int ora_stack_reset(ORA ora);


/** @brief Move to the next element in the ORA stack. Read mode function. 
 *
 * @param ora   the document
 * @param flags defines different modes of stack traversal. Possible flags are ORA_NEXT_SIBLING, ORA_NEXT_CLIMB, ORA_NEXT_NO_STACK, ORA_NEXT_NO_LAYER
 */
extern int ora_stack_next(ORA ora, int flags);

/** @brief Closes the currently opened substack. Write mode function. 
 *
 * @param ora 	the document 
 */
extern int ora_stack_level(ORA ora);

/** @brief Returns the current level in the ORA stack or a negative value in case of an error. Read mode function. 
 *
 * @param ora   the document
 */
extern int ora_stack_type(ORA ora);

//extern int ora_read_thumbnail(ORA ora, ubyte** data, int* width, int* height, int* format, progress_callback callback);

/** @brief Read data of the current layer. Read mode function.
 *
 * If the current element in the stack is a raster layer this function reads its raw data. Only possible if the given document has been opened for reading.
 *
 * @param ora 	the document
 * @param data 	the output parameter that will contain the data of the layer
 * @param geometry 	the output parameter that will contain the geometry of the layer
 * @param format 	the output parameter that will contain the format of the layer
 * @param opacity 	the output parameter that will contain the opacity information of the layer
 * @param callback 	the optional callback function that can be used to retrieve progress information of the operation. 
 */
extern int ora_read_layer(ORA ora, ubyte** data, ora_rectangle* geometry, int* format, float* opacity, ora_progress_callback callback);

/** @brief Closes the given ORA document and frees allocated memory.
 *
 * This function has to be called when the ORA document is no longer needed to finalize all the IO  
 * operations. In case of write access this is especially important as the stack information is only 
 * written in this function. Note that the ORA document reference is no longer valid after this function 
 * is called.
 *
 * @param ora 	the document 
 */
extern int ora_close(ORA ora);

/** @brief Opens a new substack. Write mode function. 
 *
 * @param ora 	the document
 * @param name 	the name of the stack
 * @param x 	offset x
 * @param y 	offset y 
 */
extern int ora_open_stack(ORA ora, const char* name, int x, int y);

/** @brief Closes the currently opened substack. Write mode function. 
 *
 * @param ora 	the document 
 */
extern int ora_close_stack(ORA ora);

/** @brief Writes the layer to the file and adds it to the stack. Write mode function. 
 *
 * @param ora 	the document
 * @param name 	the name of the layer
 * @param geometry 	the size and the offset of the layer
 * @param format 	the format of the layer
 * @param opacity 	the opacity of the layer between 0 and 1
 * @param data 	raw data of the layer
 * @param callback 	the optional callback function that can be used to retrieve progress information of the operation. 
 */
extern int ora_write_layer(ORA ora, const char* name, ora_rectangle geometry, int format, float opacity, ubyte* data, ora_progress_callback callback);
/*
extern int ora_write_tiles(ORA ora, const char* name, ora_rectangle geometry, int format, int tile_size, tile_write_callback tile_source, ora_progress_callback callback);
*/

/** @brief Returns the error code of the last error encountered when dealing with a particular ORA document. 
 *
 * @param ora 	the document 
 */
extern int ora_error(ORA ora);

#ifdef __cplusplus
}
#endif

#endif
