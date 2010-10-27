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

/** \file layers.h
 *  This file contains functions that handle internal layer I/O. 
 */

#ifndef _ORALAYER_H
#define _ORALAYER_H

#include "../ora.h"
#include "../internal.h"

/**
 * @brief Internal function that handles reading of a raster layer. 
 *
 * @param document  the document
 * @param zip 	    the ZIP archive
 * @param layer     the layer to read data into
 * @param callback 	an optional callback function that can be used to retrieve progress information of the operation. 
 *
 */
int ora_read_raster(ora_document* document, unzFile zip, ora_layer* layer, ora_progress_callback callback);

/**
 * @brief Internal function that handles writing the raster layer to the file.
 * 
 * @param document 	the document
 * @param zip 	    the ZIP archive
 * @param data 	    raw data of the raster
 * @param width 	the width of the raster
 * @param height 	the height of the raster
 * @param format 	the format of the raster
 * @param callback 	an optional callback function that can be used to retrieve progress information of the operation. 
 */
int ora_write_raster(ora_document* document, zipFile zip, ubyte* data, int width, int height, int format, ora_progress_callback callback);


#endif
