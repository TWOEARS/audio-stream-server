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
#include <stdio.h>
#include <stdint.h>
#include <errno.h>

#include "AudioCapture.h"
#include "capture_c_types.h"

static int32_t *buffer; /* the type is closely related to ids->params->format */


/* initConstantParameters --------------------------------------------------- */

int initConstantParameters(capture_ids *ids)
{
    ids->params = malloc(sizeof(capture_alsaParams_t));
    ids->params->format = SND_PCM_FORMAT_S32_LE;
    ids->params->sizeof_format = 
                        snd_pcm_format_physical_width(ids->params->format)/8;
    ids->params->channels = 2;
    ids->params->stream = SND_PCM_STREAM_CAPTURE;
    ids->params->access = SND_PCM_ACCESS_RW_INTERLEAVED;
    return 0;
}

/* initVariableParameters --------------------------------------------------- */

int initVariableParameters(capture_ids *ids, char *device, 
                           uint32_t transfer_rate, uint32_t chunk_time, 
                           uint32_t Port_chunks)
{
    ids->device = strdup(device);
    ids->transfer_rate = transfer_rate;
    ids->chunk_time = chunk_time;
    ids->Port_chunks = Port_chunks;
    ids->params->rate = (unsigned int) transfer_rate;
    ids->params->period_time = (unsigned int) chunk_time*1000;
    ids->params->buffer_time = 3*ids->params->period_time;
    ids->params->first = 1;
    return 0;
}

/* createCapture ------------------------------------------------------------ */

int createCapture(capture_ids *ids)
{
    int err;

    /* Open the device */
    if ((err = snd_pcm_open(&(ids->params->handle), ids->device, 
                                        ids->params->stream, 0)) < 0) {
        printf("Open error: %s\n", snd_strerror(err));
        return err;
    }

    /* Set hardware parameters */
    snd_pcm_hw_params_alloca(&(ids->params->hwparams));
    if ((err = setHwparams(ids)) < 0) {
        printf("Setting of hwparams failed: %s\n", snd_strerror(err));
        return err;
    }

    /* Set software parameters */
    snd_pcm_sw_params_alloca(&(ids->params->swparams));
    if ((err = setSwparams(ids)) < 0) {
        printf("Setting of swparams failed: %s\n", snd_strerror(err));
        return err;
    }

    /* Allocate memory for receiving data from ALSA buffer */
    buffer = malloc(ids->params->channels * ids->params->sizeof_format *
                                                     ids->params->period_size);
    if (buffer == NULL) {
        printf("Could not allocate memory for the transfer chunk\n");
        return -1;
    }

    /* Prepare the device */
    if ((err = snd_pcm_prepare (ids->params->handle)) < 0) {
        fprintf (stderr, "cannot prepare audio interface for use (%s)\n",
             snd_strerror (err));
        return -1;
    }

    return 0;
}

/* setHwparams -------------------------------------------------------------- */

int setHwparams(capture_ids *ids)
{
    int err, dir;
    snd_pcm_uframes_t size;

    /* Choose all parameters */
    if ((err = snd_pcm_hw_params_any(ids->params->handle, 
                                            ids->params->hwparams)) < 0) {
        printf("Broken configuration: no configurations available: %s\n", 
                                                            snd_strerror(err));
        return err;
    }

    /* Set the access */
    if ((err = snd_pcm_hw_params_set_access(ids->params->handle, 
                        ids->params->hwparams, ids->params->access)) < 0) {
        printf("Access type not available: %s\n", snd_strerror(err));
        return err;
    }

    /* Set the sample format */
    if ((err = snd_pcm_hw_params_set_format(ids->params->handle, 
                        ids->params->hwparams, ids->params->format)) < 0) {
        printf("Sample format not available: %s\n", snd_strerror(err));
        return err;
    }

    /* Set the count of channels */
    if ((err = snd_pcm_hw_params_set_channels(ids->params->handle, 
                        ids->params->hwparams, ids->params->channels)) < 0) {
        printf("Channels count (%i) not available: %s\n", 
                            ids->params->channels, snd_strerror(err));
        return err;
    }

    /* Set the stream rate */
    unsigned int rrate = ids->params->rate;
    err = snd_pcm_hw_params_set_rate_near(ids->params->handle, 
                                    ids->params->hwparams, &rrate, 0);
    if (err < 0) {
        printf("Rate %iHz not available: %s\n", ids->params->rate, 
                                                        snd_strerror(err));
        return err;
    }
    if (rrate != ids->params->rate) {
        printf("Rate doesn't match (requested %i Hz, get %i Hz)\n", 
                                                    ids->params->rate, rrate);
        return -EINVAL;
    }

    /* Set the ALSA ring buffer time */
    unsigned int rbuffer_time = (unsigned int) ids->params->buffer_time;
    if ((err = snd_pcm_hw_params_set_buffer_time_near(ids->params->handle, 
                            ids->params->hwparams, &rbuffer_time, &dir)) < 0) {
        printf("Unable to set buffer time %i: %s\n", ids->params->buffer_time, 
                                                            snd_strerror(err));
        return err;
    }
    if (rbuffer_time != ids->params->buffer_time) {
        printf("Buffer time doesn't match (requested %i microseconds, \
            get %i microseconds)\n", ids->params->buffer_time, rbuffer_time);
        return -EINVAL;
    }
    if ((err = snd_pcm_hw_params_get_buffer_size(ids->params->hwparams,
                                                            &size)) < 0) {
        printf("Unable to get buffer size: %s\n", snd_strerror(err));
        return err;
    }
    ids->params->buffer_size = (snd_pcm_sframes_t) size;

    /* Set the period time (transfer chunk) */
    unsigned int rperiod_time = ids->params->period_time;
    if ((err = snd_pcm_hw_params_set_period_time_near(ids->params->handle, 
                        ids->params->hwparams, &rperiod_time, &dir)) < 0) {
        printf("Unable to set period time %i: %s\n", ids->params->period_time, 
                                                            snd_strerror(err));
        return err;
    }
    if (rperiod_time != ids->params->period_time) {
        printf("Period time doesn't match (requested %i microseconds, \
              get %i microseconds)\n", ids->params->period_time, rperiod_time);
        return -EINVAL;
    }
    if ((err = snd_pcm_hw_params_get_period_size(ids->params->hwparams, &size, 
                                                                &dir)) < 0) {
        printf("Unable to get period size: %s\n", snd_strerror(err));
        return err;
    }
    ids->params->period_size = (snd_pcm_sframes_t) size;

    /* Write the parameters to device */
    if ((err = snd_pcm_hw_params(ids->params->handle, 
                                                ids->params->hwparams)) < 0) {
        printf("Unable to set hw params: %s\n", snd_strerror(err));
        return err;
    }

    return 0;
}

/* setSwparams -------------------------------------------------------------- */

int setSwparams(capture_ids *ids)
{
    int err;

    /* Get the current swparams */
    if ((err = snd_pcm_sw_params_current(ids->params->handle, 
                                                ids->params->swparams)) < 0) {
        printf("Unable to determine current swparams: %s\n", snd_strerror(err));
        return err;
    }

    /* Set the minimum available count to the period size (transfer chunk) */
    if ((err = snd_pcm_sw_params_set_avail_min (ids->params->handle, 
                    ids->params->swparams, ids->params->period_size)) < 0) {
        fprintf (stderr, "cannot set minimum available count (%s)\n",
             snd_strerror (err));
        exit (1);
    }

    if ((err = snd_pcm_sw_params_set_start_threshold(ids->params->handle, 
                                            ids->params->swparams, 0U)) < 0) {
        printf("Unable to set start threshold mode: %s\n", snd_strerror(err));
        return err;
    }

    /* Write the parameters to the playback device */
    if ((err = snd_pcm_sw_params(ids->params->handle, 
                                                ids->params->swparams)) < 0) {
        printf("Unable to set sw params: %s\n", snd_strerror(err));
        return err;
    }

    return 0;
}

/* runCapture --------------------------------------------------------------- */

int runCapture(capture_ids *ids)
{    
    int err, ii;
    snd_pcm_sframes_t frames_to_deliver;

    if (ids->params->first) {
        ids->params->first = 0;
        if ((err = snd_pcm_start(ids->params->handle)) < 0) {
            printf("Start error: %s\n", snd_strerror(err));
            return err;
        }

        /* Get the trigger timestamp */
        snd_pcm_status_t *status;
        snd_htimestamp_t ts;
        snd_pcm_status_alloca(&status);
        if ((err = snd_pcm_status(ids->params->handle, status)) < 0) {
            printf("Stream status error: %s\n", snd_strerror(err));
            return err;
        }
        snd_pcm_status_get_trigger_htstamp(status, &ts);
        //printf("Trigger timestamp: sec %d nsec %d\n", ts.tv_sec, ts.tv_nsec);
        ids->current_chunk.ts.tv_sec = ts.tv_sec;
        ids->current_chunk.ts.tv_nsec = ts.tv_nsec;

    } else {
        if ((err = snd_pcm_wait (ids->params->handle, -1)) < 0) {
            fprintf (stderr, "poll failed (%s)\n", strerror (errno));
            return err;
        }
    }

    /* Find out how much data is available */
    if ((frames_to_deliver = snd_pcm_avail_update(ids->params->handle)) < 0) {
        if (frames_to_deliver == -EPIPE) {
            fprintf (stderr, "an xrun occured\n");
            return err;
        } else {
            fprintf (stderr, "unknown ALSA avail update return value (%d)\n",
                     (int) frames_to_deliver);
            return err;
        }
    }
    /*printf("Frames available: %d (maximum delivered: %d)\n", 
                    (int) frames_to_deliver, (int) ids->params->period_size);*/
    frames_to_deliver = frames_to_deliver > ids->params->period_size ? 
                                ids->params->period_size : frames_to_deliver;

    /* Read the data from ALSA buffer */
    if ((err = snd_pcm_readi(ids->params->handle, 
                                            buffer, frames_to_deliver)) < 0) {
        fprintf (stderr, "write failed (%s)\n", snd_strerror (err));
        return err;
    }

    /* Copy the current chunk of data to the IDS */
    ids->current_chunk.nbframes = (uint32_t) frames_to_deliver;
    for(ii = 0; ii < ids->current_chunk.nbframes; ii++) {
        ids->current_chunk.left._buffer[ii] = buffer[2*ii];
        ids->current_chunk.right._buffer[ii] = buffer[2*ii+1];
    }

    /* Process the new timestamp of the last frame */
    double tmp;
    tmp = (double)ids->current_chunk.nbframes/(double)ids->transfer_rate;
    tmp *= 1000000000;
    ids->current_chunk.ts.tv_nsec += (int) tmp;
    if (ids->current_chunk.ts.tv_nsec > 1000000000) {
        ids->current_chunk.ts.tv_sec += 
                                ids->current_chunk.ts.tv_nsec / 1000000000;
        ids->current_chunk.ts.tv_nsec %= 1000000000;
    };

    return err;
}

/* endCapture --------------------------------------------------------------- */

int endCapture(capture_ids *ids)
{
    /* Close the device */
    snd_pcm_close(ids->params->handle);

    free(buffer);
    return 0;
}

