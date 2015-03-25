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

#ifndef AUDIOCAPTURE_H
#define AUDIOCAPTURE_H

#include <alsa/asoundlib.h>
#include <stdint.h>
#include "bass_c_types.h"

struct bass_alsaParamsStruct
{
    char *device;
    snd_pcm_format_t format; /* sample format */
    unsigned int sizeof_format;
    unsigned int rate ; /* stream rate */
    unsigned int channels; /* count of channels */
    unsigned int period_time; /* period time in microseconds (transfer chunk) */
    unsigned int buffer_time; /* ring buffer length in microseconds */
    snd_pcm_sframes_t period_size;
    snd_pcm_sframes_t buffer_size;
    int first; /* used in function runCapture */

    snd_pcm_t *handle;
    snd_pcm_stream_t stream;
    snd_pcm_access_t access;
    snd_pcm_hw_params_t *hwparams;
    snd_pcm_sw_params_t *swparams;
};

int initConstantParameters(bass_ids *ids);
int initVariableParameters(bass_ids *ids, const char *device,
                           uint32_t sampleRate, uint32_t chunkTime,
                           uint32_t nChunksOnPort);
int createCapture(bass_ids *ids);
int setHwparams(bass_ids *ids);
int setSwparams(bass_ids *ids);
int runCapture(bass_ids *ids);
int endCapture(bass_ids *ids);

#endif /* AUDIOCAPTURE_H */

