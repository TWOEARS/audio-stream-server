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

#include <stdio.h>
#include <stdint.h>
#include <errno.h>

#include "AudioCapture.h"
#include "bass_c_types.h"

/*
 * Alsa examples and tutorials:
 * http://www.alsa-project.org/alsa-doc/alsa-lib/_2test_2pcm_8c-example.html
 * http://equalarea.com/paul/alsa-audio.html
 */


/* initCapture -------------------------------------------------------------- */

int initCapture(bass_captureStruct **pcap, const char *device,
                uint32_t sampleRate, uint32_t chunkTime,
                uint32_t nChunksOnPort)
{
    bass_captureStruct *cap;
    *pcap = malloc(sizeof(bass_captureStruct));
    cap = *pcap;

    /* Constant parameters, hardcoded (should not be changed) */
    cap->stream = SND_PCM_STREAM_CAPTURE;
    cap->access = SND_PCM_ACCESS_RW_INTERLEAVED;
    cap->format = SND_PCM_FORMAT_S32_LE; /* expected format for the audio
                                            device and the port */
    cap->nBytesPerSample = snd_pcm_format_physical_width(cap->format)/8;
    cap->channels = 2; /* binaural: left and right channels on the port */

    /* Variable parameters, defined at runtime */
    cap->handle = NULL;
    cap->device = strdup(device);
    cap->buff = NULL;
    cap->rate = (unsigned int) sampleRate;
    cap->chunkTime = (unsigned int) chunkTime*1000;
    cap->arbTime = ARBSIZE_ON_CHUNKSIZE * cap->chunkTime;
    return 0;
}

/* createCapture ------------------------------------------------------------ */

int createCapture(bass_captureStruct *cap)
{
    int err;

    /* Open the device */
    if ((err = snd_pcm_open(&(cap->handle), cap->device, cap->stream, 0)) < 0) {
        fprintf(stderr, "Open error: %s\n", snd_strerror(err));
        return -E_DEVICE;
    }

    /* Set hardware parameters */
    if ((err = setHwparams(cap)) < 0) {
        fprintf(stderr, "Setting of hwparams failed: %s\n", snd_strerror(err));
        return -E_HWPARAMS;
    }

    /* Set software parameters */
    if ((err = setSwparams(cap)) < 0) {
        fprintf(stderr, "Setting of swparams failed: %s\n", snd_strerror(err));
        return -E_SWPARAMS;
    }

    /* Allocate memory for storing received data from the alsa ring buffer */
    if ((cap->buff = malloc(cap->channels * cap->nBytesPerSample *
                            cap->chunkSize)) == NULL) {
        fprintf(stderr, "Could not allocate memory for the transfer chunk\n");
        return -E_NOMEM;
    }

    /* Prepare the device */
    if ((err = snd_pcm_prepare(cap->handle)) < 0) {
        fprintf(stderr, "cannot prepare audio interface for use (%s)\n",
             snd_strerror(err));
        return -E_DEVICE;
    }

    if ((err = snd_pcm_start(cap->handle)) < 0) {
        fprintf(stderr, "Start error: %s\n", snd_strerror(err));
        return -E_DEVICE;
    }

    return 0;
}

/* setHwparams -------------------------------------------------------------- */

int setHwparams(bass_captureStruct *cap)
{
    int err, dir;
    snd_pcm_uframes_t size;
    unsigned int rrate, rarbTime, rchunkTime;

    snd_pcm_hw_params_alloca(&(cap->hwparams));

    /* Choose all parameters */
    if ((err = snd_pcm_hw_params_any(cap->handle, cap->hwparams)) < 0) {
        fprintf(stderr,
                "Broken configuration: no configurations available: %s\n",
                snd_strerror(err));
        return err;
    }

    /* Set the access */
    if ((err = snd_pcm_hw_params_set_access(cap->handle, cap->hwparams,
                                            cap->access)) < 0) {
        fprintf(stderr, "Access type not available: %s\n", snd_strerror(err));
        return err;
    }

    /* Set the sample format */
    if ((err = snd_pcm_hw_params_set_format(cap->handle, cap->hwparams,
                                            cap->format)) < 0) {
        fprintf(stderr, "Sample format not available: %s\n", snd_strerror(err));
        return err;
    }

    /* Set the count of channels */
    if ((err = snd_pcm_hw_params_set_channels(cap->handle, cap->hwparams,
                                              cap->channels)) < 0) {
        fprintf(stderr, "Channels count (%i) not available: %s\n",
                cap->channels, snd_strerror(err));
        return err;
    }

    /* Set the stream rate */
    rrate = cap->rate;
    if ((err = snd_pcm_hw_params_set_rate_near(cap->handle, cap->hwparams,
                                               &rrate, 0)) < 0) {
        fprintf(stderr, "Rate %iHz not available: %s\n", cap->rate,
                snd_strerror(err));
        return err;
    }
    if (rrate != cap->rate) {
        fprintf(stderr, "Rate doesn't match (requested %i Hz, get %i Hz)\n",
                cap->rate, rrate);
        return -EINVAL;
    }

    /* Set the ALSA ring buffer time */
    rarbTime = (unsigned int) cap->arbTime;
    if ((err = snd_pcm_hw_params_set_buffer_time_near(cap->handle,
                                                      cap->hwparams,
                                                      &rarbTime, &dir))
        < 0) {
        fprintf(stderr, "Unable to set buffer time %i: %s\n", cap->arbTime,
                snd_strerror(err));
        return err;
    }
    if (rarbTime != cap->arbTime) {
        fprintf(stderr, "Buffer time doesn't match "
                "(requested %i microseconds, get %i microseconds)\n",
                cap->arbTime, rarbTime);
        return -EINVAL;
    }
    if ((err = snd_pcm_hw_params_get_buffer_size(cap->hwparams, &size)) < 0) {
        fprintf(stderr, "Unable to get buffer size: %s\n", snd_strerror(err));
        return err;
    }
    cap->arbSize = (snd_pcm_sframes_t) size;

    /* Set the period time (transfer chunk) */
    rchunkTime = cap->chunkTime;
    if ((err = snd_pcm_hw_params_set_period_time_near(cap->handle,
                                                      cap->hwparams,
                                                      &rchunkTime, &dir))
        < 0) {
        fprintf(stderr, "Unable to set period time %i: %s\n", cap->chunkTime,
                snd_strerror(err));
        return err;
    }
    if (rchunkTime != cap->chunkTime) {
        fprintf(stderr, "Period time doesn't match "
                "(requested %i microseconds, get %i microseconds)\n",
                cap->chunkTime, rchunkTime);
        return -EINVAL;
    }
    if ((err = snd_pcm_hw_params_get_period_size(cap->hwparams, &size, &dir))
        < 0) {
        fprintf(stderr, "Unable to get period size: %s\n", snd_strerror(err));
        return err;
    }
    cap->chunkSize = (snd_pcm_sframes_t) size;

    /* Write the parameters to the device */
    if ((err = snd_pcm_hw_params(cap->handle, cap->hwparams)) < 0) {
        fprintf(stderr, "Unable to set hw params: %s\n", snd_strerror(err));
        return err;
    }

    return 0;
}

/* setSwparams -------------------------------------------------------------- */

int setSwparams(bass_captureStruct *cap)
{
    int err;

    snd_pcm_sw_params_alloca(&(cap->swparams));

    /* Get the current swparams */
    if ((err = snd_pcm_sw_params_current(cap->handle, cap->swparams)) < 0) {
        fprintf(stderr, "Unable to determine current swparams: %s\n",
                snd_strerror(err));
        return err;
    }

    /* Set the minimum available count to period size: the interface will
       interrupt the kernel every cap->chunkSize frames, and ALSA will wake
       up this program very soon after that. */
    if ((err = snd_pcm_sw_params_set_avail_min(cap->handle, cap->swparams,
                                               cap->chunkSize)) < 0) {
        fprintf(stderr, "cannot set minimum available count (%s)\n",
                snd_strerror(err));
        return err;
    }

    if ((err = snd_pcm_sw_params_set_start_threshold(cap->handle,
                                                     cap->swparams, 0U)) < 0) {
        fprintf(stderr, "Unable to set start threshold mode: %s\n",
                snd_strerror(err));
        return err;
    }

    /* Write the parameters to the device */
    if ((err = snd_pcm_sw_params(cap->handle, cap->swparams)) < 0) {
        fprintf(stderr, "Unable to set sw params: %s\n", snd_strerror(err));
        return err;
    }

    return 0;
}

/* runCapture --------------------------------------------------------------- */

int runCapture(bass_captureStruct *cap)
{
    int err;
    snd_pcm_sframes_t ftr; /* Frames To Read */

    if ((err = snd_pcm_wait(cap->handle, -1)) < 0) {
        fprintf(stderr, "poll failed (%s)\n", strerror(err));
        return -E_DEVICE;
    }

    /* Find out how much data is available */
    if ((ftr = snd_pcm_avail_update(cap->handle)) < cap->chunkSize) {
        if (ftr > 0) {
            fprintf(stderr, "Only %d frames available (min allowed %d).\n",
                    (int) ftr, (int) cap->chunkSize);
            return -E_DEVICE;
            } else if (ftr == -EPIPE) {
            fprintf(stderr, "an xrun occured\n");
            return -E_DEVICE;
        } else {
            fprintf(stderr, "unknown ALSA avail update return value (%d)\n",
                    (int) ftr);
            return -E_DEVICE;
        }
    }
    /*printf("ftr: %8d | fpc: %8d\n", (int) ftr, (int) cap->chunkSize);*/

    /* Read chunkSize frames from the alsa ring buffer */
    if ((err = snd_pcm_readi(cap->handle, cap->buff, cap->chunkSize)) < 0) {
        fprintf(stderr, "read failed (%s)\n", snd_strerror(err));
        return -E_DEVICE;
    }

    return 0;
}

/* endCapture --------------------------------------------------------------- */

int endCapture(bass_captureStruct **pcap)
{
    bass_captureStruct *cap;
    cap = *pcap;

    if (cap != NULL) {
        if (cap->handle != NULL) {
            snd_pcm_close(cap->handle);
            cap->handle = NULL;
        }
        if (cap->device != NULL) {
            free(cap->device);
            cap->device = NULL;
        }
        if (cap->buff != NULL) {
            free(cap->buff);
            cap->buff = NULL;
        }
        free(cap);
        *pcap = NULL;
    }
    return 0;
}

