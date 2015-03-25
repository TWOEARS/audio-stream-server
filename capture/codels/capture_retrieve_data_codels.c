/*  Copyright (c) 2014, LAAS/CNRS
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
#include "accapture.h"

#include "capture_c_types.h"

#include <stdint.h>
#include "AudioCapture.h"
#include "Ports.h"



/* --- Task retrieve_data ----------------------------------------------- */


/** Codel start_retrieve_data of task retrieve_data.
 *
 * Triggered by capture_start.
 * Yields to capture_ether.
 */
genom_event
start_retrieve_data(capture_ids *ids, genom_context self)
{
    /* Set constant capture parameters */
    initConstantParameters(ids);

    printf("Ready to start capture.\n");
    return capture_ether;
}


/** Codel stop_retrieve_data of task retrieve_data.
 *
 * Triggered by capture_stop.
 * Yields to capture_ether.
 */
genom_event
stop_retrieve_data(capture_ids *ids, genom_context self)
{
    //free(ids->device);
    free(ids->params);
    return capture_ether;
}


/* --- Activity StartCapture -------------------------------------------- */

/** Codel scStart of activity StartCapture.
 *
 * Triggered by capture_start.
 * Yields to capture_exec, capture_ether.
 * Throws capture_INVALID_CHUNK_TIME, capture_ERROR_SEQUENCE_ENOMEM.
 */
genom_event
scStart(capture_ids *ids, const char *device, uint32_t transfer_rate,
        uint32_t chunk_time, uint32_t Port_chunks,
        const capture_Audio *Audio, genom_context self)
{
    int err;

    /* Set variable capture parameters */
    initVariableParameters(ids, device, transfer_rate, chunk_time, Port_chunks);

    /* Prepare the Port and the IDS */
    uint32_t chunk_size = transfer_rate*chunk_time/1000;
    if (initCurrentChunk(ids, chunk_size) ||
        initPort(Audio, transfer_rate, Port_chunks, chunk_size, self))
        return capture_ERROR_SEQUENCE_ENOMEM(self);

    /* Start the capture */
    if ((err = createCapture(ids)) < 0) {
        printf("Could not start capture.\n");
        return capture_ether;
    }
    printf("Capture started.\n");
    return capture_exec;
}


/** Codel scExec of activity StartCapture.
 *
 * Triggered by capture_exec.
 * Yields to capture_exec, capture_stop.
 * Throws capture_INVALID_CHUNK_TIME, capture_ERROR_SEQUENCE_ENOMEM.
 */
genom_event
scExec(capture_ids *ids, const capture_Audio *Audio,
       genom_context self)
{
    int err;

    /* Get the data */
    if ((err = runCapture(ids)) < 0)
        return capture_stop;

    /* Publish the data on the Port */
    publishPort(Audio, ids, self);

    return capture_exec;
}

/** Codel scStop of activity StartCapture.
 *
 * Triggered by capture_stop.
 * Yields to capture_ether.
 * Throws capture_INVALID_CHUNK_TIME, capture_ERROR_SEQUENCE_ENOMEM.
 */
genom_event
scStop(capture_ids *ids, genom_context self)
{
    endCapture(ids);
    printf("Capture stopped.\n");
    return capture_ether;
}

