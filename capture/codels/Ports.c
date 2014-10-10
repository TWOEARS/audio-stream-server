/* Copyright (c) 2014, LAAS/CNRS
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions are met:
 * 
 * 1. Redistributions of source code must retain the above copyright notice, 
 *    this list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright notice, 
 *    this list of conditions and the following disclaimer in the documentation 
 *    and/or other materials provided with the distribution.
 * 
 * 3. Neither the name of the copyright holder nor the names of its contributors 
 *    may be used to endorse or promote products derived from this software 
 *    without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE 
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include <string.h>

#include "Ports.h"
#include "capture_c_types.h"
#include "AudioCapture.h"

/* initCurrentChunk --------------------------------------------------------- */

int initCurrentChunk(capture_ids *ids, uint32_t size)
{
    if (genom_sequence_reserve(&(ids->current_chunk.left), size) || 
        genom_sequence_reserve(&(ids->current_chunk.right), size))
        return -1;

    ids->current_chunk.left._length = size;
    ids->current_chunk.right._length = size;
    ids->current_chunk.nbframes = size;

    int ii;
    for (ii = 0; ii < size; ii++) {
        ids->current_chunk.left._buffer[ii] = 0;
        ids->current_chunk.right._buffer[ii] = 0;
    }
    return 0;
}

/* initPort ----------------------------------------------------------------- */

int initPort(const capture_Port *Port, uint32_t size, uint32_t transfer_rate, 
             genom_context self)
{
    if (genom_sequence_reserve(&(Port->data(self)->left), size) || 
        genom_sequence_reserve(&(Port->data(self)->right), size))
        return -1;

    Port->data(self)->left._length = size;
    Port->data(self)->right._length = size;
    Port->data(self)->nbframes = size;

    int ii;
    for (ii = 0; ii < size; ii++) {
        Port->data(self)->left._buffer[ii] = 0;
        Port->data(self)->right._buffer[ii] = 0;
    }
    Port->data(self)->transfer_rate = transfer_rate;
    Port->write(self);
    return 0;
}

/* publishPort ------------------------------------------------------------- */

int publishPort(const capture_Port *Port, capture_ids *ids, 
                 genom_context self)
{
    memmove(Port->data(self)->left._buffer, 
            Port->data(self)->left._buffer + ids->current_chunk.nbframes, 
            (Port->data(self)->nbframes - ids->current_chunk.nbframes)*
            ids->params->sizeof_format);
    memmove(Port->data(self)->right._buffer, 
            Port->data(self)->right._buffer + ids->current_chunk.nbframes, 
            (Port->data(self)->nbframes - ids->current_chunk.nbframes)*
            ids->params->sizeof_format);

    /*   *****  *****  *****  *****  ***** *****   */
    /*   | <-   |                          |new    */
    /*   memmove                           memcpy  */

    memcpy(Port->data(self)->left._buffer + Port->data(self)->nbframes - 
           ids->current_chunk.nbframes,
           ids->current_chunk.left._buffer,
           ids->current_chunk.nbframes*ids->params->sizeof_format);
    memcpy(Port->data(self)->right._buffer + Port->data(self)->nbframes - 
           ids->current_chunk.nbframes,
           ids->current_chunk.right._buffer,
           ids->current_chunk.nbframes*ids->params->sizeof_format);

    Port->data(self)->ts.tv_sec = ids->current_chunk.ts.tv_sec;
    Port->data(self)->ts.tv_nsec = ids->current_chunk.ts.tv_nsec;
    Port->data(self)->index++;
    Port->write(self);
    return 0;
}


