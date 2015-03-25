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
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
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
#include "bass_c_types.h"
#include "AudioCapture.h"

/* initCurrentChunk --------------------------------------------------------- */

int initCurrentChunk(bass_ids *ids, uint32_t size)
{
    int ii;

    if (genom_sequence_reserve(&(ids->currentChunk.left), size) ||
        genom_sequence_reserve(&(ids->currentChunk.right), size))
        return -1;

    ids->currentChunk.left._length = size;
    ids->currentChunk.right._length = size;
    ids->currentChunk.nFramesPerChunk = size;

    for (ii = 0; ii < size; ii++) {
        ids->currentChunk.left._buffer[ii] = 0;
        ids->currentChunk.right._buffer[ii] = 0;
    }
    return 0;
}

/* initPort ----------------------------------------------------------------- */

int initPort(const bass_Audio *Audio, uint32_t sampleRate,
             uint32_t nChunksOnPort, uint32_t nFramesPerChunk,
             genom_context self)
{
    uint32_t fop;
    int ii;

    fop =  nFramesPerChunk * nChunksOnPort;
    if (genom_sequence_reserve(&(Audio->data(self)->left), fop) ||
        genom_sequence_reserve(&(Audio->data(self)->right), fop))
        return -1;

    Audio->data(self)->left._length = fop;
    Audio->data(self)->right._length = fop;

    for (ii = 0; ii < fop; ii++) {
        Audio->data(self)->left._buffer[ii] = 0;
        Audio->data(self)->right._buffer[ii] = 0;
    }
    Audio->data(self)->sampleRate = sampleRate;
    Audio->data(self)->nChunksOnPort = nChunksOnPort;
    Audio->data(self)->nFramesPerChunk = nFramesPerChunk;
    Audio->data(self)->lastChunkIndex = 0;
    Audio->write(self);
    return 0;
}

/* publishPort ------------------------------------------------------------- */

int publishPort(const bass_Audio *Audio, bass_ids *ids, genom_context self)
{
    uint32_t fop;

    fop = Audio->data(self)->nChunksOnPort *
          Audio->data(self)->nFramesPerChunk;

    memmove(Audio->data(self)->left._buffer,
            Audio->data(self)->left._buffer +
            ids->currentChunk.nFramesPerChunk,
            (fop - ids->currentChunk.nFramesPerChunk)*
            ids->params->sizeof_format);
    memmove(Audio->data(self)->right._buffer,
            Audio->data(self)->right._buffer +
            ids->currentChunk.nFramesPerChunk,
            (fop - ids->currentChunk.nFramesPerChunk)*
            ids->params->sizeof_format);

    /*   **1**  **2**  **3**  **4**  **5** **6**   */
    /*   | <-   |                          |new    */
    /*   memmove                           memcpy  */
    /*   **2**  **3**  **4**  **5**  **6** **7**   */

    memcpy(Audio->data(self)->left._buffer + fop -
           ids->currentChunk.nFramesPerChunk,
           ids->currentChunk.left._buffer,
           ids->currentChunk.nFramesPerChunk*ids->params->sizeof_format);
    memcpy(Audio->data(self)->right._buffer + fop -
           ids->currentChunk.nFramesPerChunk,
           ids->currentChunk.right._buffer,
           ids->currentChunk.nFramesPerChunk*ids->params->sizeof_format);

    Audio->data(self)->lastChunkIndex++;
    Audio->write(self);
    return 0;
}


