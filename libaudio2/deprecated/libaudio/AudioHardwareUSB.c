/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "usb_audio_hw"
//#define LOG_NDEBUG 0

#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <sys/time.h>
#include <stdlib.h>

#include <cutils/log.h>
#include <cutils/str_parms.h>
#include <cutils/properties.h>

#include <hardware/hardware.h>
#include <system/audio.h>
#include <hardware/audio.h>

#include <tinyalsa/asoundlib.h>

#ifndef CARD_ID
#define CARD_ID 1
#endif

struct pcm_config pcm_config = {
    .channels = 2,
    .rate = 44100,
    .period_size = 1024,
    .period_count = 4,
    .format = PCM_FORMAT_S16_LE,
};

struct pcm_config pcm_config_in = {
    .channels = 1,
    .rate = 8000,
    .period_size = 1024,
    .period_count = 4,
    .format = PCM_FORMAT_S16_LE,
};

struct audio_device {
    struct audio_hw_device hw_device;

    pthread_mutex_t lock; /* see note below on mutex acquisition order */
    int card;
    int device;
    bool standby;
};

struct stream_out {
    struct audio_stream_out stream;

    pthread_mutex_t lock; /* see note below on mutex acquisition order */
    struct pcm *pcm;
    struct mixer *mix;
    bool standby;

    struct audio_device *dev;
};

struct stream_in {
    struct audio_stream_in stream;

    pthread_mutex_t lock; /* see note below on mutex acquisition order */
    struct pcm *pcm;
    bool standby;

    struct audio_device *dev;
};

float master_volume = 0.0f;

/**
 * NOTE: when multiple mutexes have to be acquired, always respect the
 * following order: hw device > out stream
 */

/* out_set_parameters() sets this to 1 whilst another output device is active in order
   to prevent it from later resetting the output device back to the hard coded default */
static int out_override = 0;
static int in_override = 0;

/* Helper functions */

/* must be called with hw device and output stream mutexes locked */
static int start_output_stream(struct stream_out *out)
{
    struct audio_device *adev = out->dev;
    int i;

    ALOGD("%s ->", __func__);
    if ((adev->card < 0) || (adev->device < 0))
        return -EINVAL;

    //ALOGD("start_output_stream()");
#ifdef USE_MMAP
    out->pcm = pcm_open(adev->card, adev->device, PCM_OUT | PCM_MMAP | PCM_NOIRQ , &pcm_config);
#else
    out->pcm = pcm_open(adev->card, adev->device, PCM_OUT, &pcm_config);
#endif

    if (out->pcm && !pcm_is_ready(out->pcm)) {
        ALOGE("pcm_open() failed: %s", pcm_get_error(out->pcm));
        pcm_close(out->pcm);
        return -ENOMEM;
    }

    out->mix = mixer_open(adev->card);

    ALOGD("mixer: %s", mixer_get_name(out->mix));

#if 0
    for (i = 0; i < mixer_get_num_ctls(out->mix); i++)
    {
        struct mixer_ctl *mix_ctl = mixer_get_ctl(out->mix, i);
        ALOGD("mixer_ctl: %s [%s]", mixer_ctl_get_name(mix_ctl), mixer_ctl_get_type_string(mix_ctl));
    }
#endif
    ALOGD("%s <-", __func__);

    return 0;
}

/* API functions */

static uint32_t out_get_sample_rate(const struct audio_stream *stream)
{
    //ALOGD("%s", __func__);
    return pcm_config.rate;
}

static int out_set_sample_rate(struct audio_stream *stream, uint32_t rate)
{
    ALOGD("%s", __func__);
    return 0;
}

static size_t out_get_buffer_size(const struct audio_stream *stream)
{
    ALOGD("%s", __func__);
    return pcm_config.period_size *
           audio_stream_frame_size((struct audio_stream *)stream);
}

static uint32_t out_get_channels(const struct audio_stream *stream)
{
    ALOGD("%s", __func__);
    return AUDIO_CHANNEL_OUT_STEREO;
}

static audio_format_t out_get_format(const struct audio_stream *stream)
{
    ALOGD("%s", __func__);
    return AUDIO_FORMAT_PCM_16_BIT;
}

static int out_set_format(struct audio_stream *stream, audio_format_t format)
{
    ALOGD("%s", __func__);
    return 0;
}

static int out_standby(struct audio_stream *stream)
{
    struct stream_out *out = (struct stream_out *)stream;

    ALOGD("%s ->", __func__);
    pthread_mutex_lock(&out->dev->lock);
    pthread_mutex_lock(&out->lock);

    ALOGD("out_standby");
    if (!out->standby) {
        mixer_close(out->mix);
        pcm_close(out->pcm);
        out->pcm = NULL;
        out->standby = true;
    }

    pthread_mutex_unlock(&out->lock);
    pthread_mutex_unlock(&out->dev->lock);
    ALOGD("%s <-", __func__);

    return 0;
}

static int out_dump(const struct audio_stream *stream, int fd)
{
    ALOGD("%s", __func__);
    return 0;
}

static int out_set_parameters(struct audio_stream *stream, const char *kvpairs)
{
    struct stream_out *out = (struct stream_out *)stream;
    struct audio_device *adev = out->dev;
    struct str_parms *parms;
    char value[32];
    int ret;
    int routing = 0;
    int closing = 0;

    ALOGD("%s ->", __func__);
    parms = str_parms_create_str(kvpairs);
    pthread_mutex_lock(&adev->lock);

    ret = str_parms_get_str(parms, "closing", value, sizeof(value));
    if (ret >= 0)
    {
        closing = (strcmp(value, "true") == 0);
        out_override = !closing;
    }

    ret = str_parms_get_str(parms, "card", value, sizeof(value));
    if (ret >= 0) {
        adev->card = atoi(value);
        out_override = 1;
    }

    ret = str_parms_get_str(parms, "device", value, sizeof(value));
    if (ret >= 0) {
        adev->device = atoi(value);
        out_override = 1;
    }

    if (out_override == 0) {
        adev->card = CARD_ID;
        adev->device = 0;
    }

    ALOGD("out_set_parameters card [%d] device[%d] out_override[%d]", adev->card, adev->device, out_override);
    pthread_mutex_unlock(&adev->lock);
    str_parms_destroy(parms);
    ALOGD("%s <-", __func__);

    return 0;
}

static char * out_get_parameters(const struct audio_stream *stream, const char *keys)
{
    ALOGD("%s", __func__);
    return strdup("");
}

static uint32_t out_get_latency(const struct audio_stream_out *stream)
{
    //ALOGD("%s", __func__);
    return (pcm_config.period_size * pcm_config.period_count * 1000) /
            out_get_sample_rate(&stream->common);
}

static int out_set_volume(struct audio_stream_out *stream, float left,
                          float right)
{
    struct stream_out *out = (struct stream_out *)stream;
    struct mixer_ctl *mix_vol_ctl = mixer_get_ctl_by_name(out->mix, "Speaker Playback Volume");
    
    ALOGD("%s %f %f - %d", __func__, left, right, mixer_ctl_get_percent(mix_vol_ctl, 0));
    
    mixer_ctl_set_percent(mix_vol_ctl, 0, (int)(left * 100));
    return 0;
}

static ssize_t out_write(struct audio_stream_out *stream, const void* buffer,
                         size_t bytes)
{
    int ret;
    struct stream_out *out = (struct stream_out *)stream;

    //ALOGD("%s ->", __func__);
    //ALOGD("out_write() USB HAL writing %d", bytes);
    pthread_mutex_lock(&out->dev->lock);
    pthread_mutex_lock(&out->lock);
    if (out->standby) {
        ret = start_output_stream(out);
        if (ret != 0) {
            goto err;
        }
        out->standby = false;
    }

#ifdef USE_MMAP
    ret = pcm_mmap_write(out->pcm, (void *)buffer, bytes);
#else
    ret = pcm_write(out->pcm, (void *)buffer, bytes);
#endif

    pthread_mutex_unlock(&out->lock);
    pthread_mutex_unlock(&out->dev->lock);
    
    //ALOGD("%s <- %d", __func__, ret);

#ifdef USE_MMAP
    return ret == 0 ? bytes : ret;
#else
    return ret == 0 ? bytes : 0;
#endif

err:
    ALOGE("out_write() ERR");
    pthread_mutex_unlock(&out->lock);
    pthread_mutex_unlock(&out->dev->lock);
    if (ret != 0) {
        usleep(bytes * 1000000 / audio_stream_frame_size(&stream->common) /
               out_get_sample_rate(&stream->common));
    }

    return bytes;
}

static int out_get_render_position(const struct audio_stream_out *stream,
                                   uint32_t *dsp_frames)
{
    ALOGD("%s", __func__);
    return -EINVAL;
}

static int out_add_audio_effect(const struct audio_stream *stream, effect_handle_t effect)
{
    ALOGD("%s", __func__);
    return 0;
}

static int out_remove_audio_effect(const struct audio_stream *stream, effect_handle_t effect)
{
    ALOGD("%s", __func__);
    return 0;
}

static int out_get_next_write_timestamp(const struct audio_stream_out *stream,
                                        int64_t *timestamp)
{
    //ALOGD("%s", __func__);
    return -EINVAL;
}

static int adev_open_output_stream(struct audio_hw_device *dev,
                                   audio_io_handle_t handle,
                                   audio_devices_t devices,
                                   audio_output_flags_t flags,
                                   struct audio_config *config,
                                   struct audio_stream_out **stream_out)
{
    struct audio_device *adev = (struct audio_device *)dev;
    struct stream_out *out;
    int ret;

    ALOGD("%s ->", __func__);
    out = (struct stream_out *)calloc(1, sizeof(struct stream_out));
    if (!out)
        return -ENOMEM;

    out->stream.common.get_sample_rate = out_get_sample_rate;
    out->stream.common.set_sample_rate = out_set_sample_rate;
    out->stream.common.get_buffer_size = out_get_buffer_size;
    out->stream.common.get_channels = out_get_channels;
    out->stream.common.get_format = out_get_format;
    out->stream.common.set_format = out_set_format;
    out->stream.common.standby = out_standby;
    out->stream.common.dump = out_dump;
    out->stream.common.set_parameters = out_set_parameters;
    out->stream.common.get_parameters = out_get_parameters;
    out->stream.common.add_audio_effect = out_add_audio_effect;
    out->stream.common.remove_audio_effect = out_remove_audio_effect;
    out->stream.get_latency = out_get_latency;
    out->stream.set_volume = out_set_volume;
    out->stream.write = out_write;
    out->stream.get_render_position = out_get_render_position;
    out->stream.get_next_write_timestamp = out_get_next_write_timestamp;

    out->dev = adev;

    config->format = out_get_format(&out->stream.common);
    config->channel_mask = out_get_channels(&out->stream.common);
    config->sample_rate = out_get_sample_rate(&out->stream.common);

    out->standby = true;

    adev->card = -1;
    adev->device = -1;

    *stream_out = &out->stream;
    ALOGD("%s <-", __func__);
    return 0;

err_open:
    free(out);
    *stream_out = NULL;
    return ret;
}

static void adev_close_output_stream(struct audio_hw_device *dev,
                                     struct audio_stream_out *stream)
{
    struct stream_out *out = (struct stream_out *)stream;

    ALOGD("%s ->", __func__);
    out_standby(&stream->common);
    free(stream);
    ALOGD("%s <-", __func__);
}

static int adev_set_parameters(struct audio_hw_device *dev, const char *kvpairs)
{
    ALOGD("%s", __func__);
    return 0;
}

static char * adev_get_parameters(const struct audio_hw_device *dev,
                                  const char *keys)
{
    ALOGD("%s", __func__);
    return strdup("");
}

static int adev_init_check(const struct audio_hw_device *dev)
{
    ALOGD("%s", __func__);
    return 0;
}

static int adev_set_voice_volume(struct audio_hw_device *dev, float volume)
{
    ALOGD("%s", __func__);
    return -ENOSYS;
}

static int adev_set_master_volume(struct audio_hw_device *dev, float volume)
{
    ALOGD("%s volume: %f", __func__, volume);
    
    master_volume = volume;
    
    return 0;
}

static int adev_set_mode(struct audio_hw_device *dev, audio_mode_t mode)
{
    ALOGD("%s", __func__);
    return 0;
}

static int adev_set_mic_mute(struct audio_hw_device *dev, bool state)
{
    ALOGD("%s", __func__);
    return -ENOSYS;
}

static int adev_get_mic_mute(const struct audio_hw_device *dev, bool *state)
{
    ALOGD("%s", __func__);
    return -ENOSYS;
}

static size_t adev_get_input_buffer_size(const struct audio_hw_device *dev,
                                         const struct audio_config *config)
{
    ALOGD("%s", __func__);
    return 0;
}

/* Helper functions */

/* must be called with hw device and output stream mutexes locked */
static int start_input_stream(struct stream_in *in)
{
    struct audio_device *adev = in->dev;
    int i;

    ALOGD("%s ->", __func__);
    if ((adev->card < 0) || (adev->device < 0))
        return -EINVAL;

#ifdef USE_MMAP
    in->pcm = pcm_open(adev->card, adev->device, PCM_IN | PCM_MMAP | PCM_NOIRQ , &pcm_config_in);
#else
    in->pcm = pcm_open(adev->card, adev->device, PCM_IN, &pcm_config_in);
#endif

    if (in->pcm && !pcm_is_ready(in->pcm)) {
        ALOGE("pcm_open() failed: %s", pcm_get_error(in->pcm));
        pcm_close(in->pcm);
        return -ENOMEM;
    }

    ALOGD("%s <-", __func__);
    return 0;
}

/* API functions */

static uint32_t in_get_sample_rate(const struct audio_stream *stream)
{
    ALOGD("%s", __func__);
    return pcm_config_in.rate;
}

static int in_set_sample_rate(struct audio_stream *stream, uint32_t rate)
{
    ALOGD("%s", __func__);
    
    pcm_config_in.rate = rate;
    
    return 0;
}

static size_t in_get_buffer_size(const struct audio_stream *stream)
{
    ALOGD("%s", __func__);
    return pcm_config_in.period_size *
           audio_stream_frame_size((struct audio_stream *)stream);
}

static uint32_t in_get_channels(const struct audio_stream *stream)
{
    ALOGD("%s", __func__);
    return AUDIO_CHANNEL_IN_MONO;
}

static audio_format_t in_get_format(const struct audio_stream *stream)
{
    ALOGD("%s", __func__);
    return AUDIO_FORMAT_PCM_16_BIT;
}

static int in_set_format(struct audio_stream *stream, audio_format_t format)
{
    ALOGD("%s", __func__);
    return 0;
}

static int in_standby(struct audio_stream *stream)
{
    struct stream_in *in = (struct stream_in *)stream;

    ALOGD("%s ->", __func__);
    pthread_mutex_lock(&in->dev->lock);
    pthread_mutex_lock(&in->lock);

    if (!in->standby) {
        pcm_close(in->pcm);
        in->pcm = NULL;
        in->standby = true;
    }

    pthread_mutex_unlock(&in->lock);
    pthread_mutex_unlock(&in->dev->lock);
    ALOGD("%s <-", __func__);

    return 0;
}

static int in_dump(const struct audio_stream *stream, int fd)
{
    ALOGD("%s", __func__);
    return 0;
}

static int in_set_parameters(struct audio_stream *stream, const char *kvpairs)
{
    struct stream_in *in = (struct stream_in *)stream;
    struct audio_device *adev = in->dev;
    struct str_parms *parms;
    char value[32];
    int ret;
    int routing = 0;
    int closing = 0;

    ALOGD("%s ->", __func__);
    parms = str_parms_create_str(kvpairs);
    pthread_mutex_lock(&adev->lock);

    ret = str_parms_get_str(parms, "closing", value, sizeof(value));
    if (ret >= 0)
    {
        closing = (strcmp(value, "true") == 0);
        in_override = !closing;
    }

    ret = str_parms_get_str(parms, "card", value, sizeof(value));
    if (ret >= 0) {
        adev->card = atoi(value);
        in_override = 1;
    }

    ret = str_parms_get_str(parms, "device", value, sizeof(value));
    if (ret >= 0) {
        adev->device = atoi(value);
        in_override = 1;
    }

    if (in_override == 0) {
        adev->card = CARD_ID;
        adev->device = 0;
    }

    ALOGD("in_set_parameters card [%d] device[%d] in_override[%d]", adev->card, adev->device, in_override);
    pthread_mutex_unlock(&adev->lock);
    str_parms_destroy(parms);
    ALOGD("%s <-", __func__);

    return 0;
}

static char * in_get_parameters(const struct audio_stream *stream, const char *keys)
{
    ALOGD("%s", __func__);
    return strdup("");
}

static int in_add_audio_effect(const struct audio_stream *stream, effect_handle_t effect)
{
    ALOGD("%s", __func__);
    return 0;
}

static int in_remove_audio_effect(const struct audio_stream *stream, effect_handle_t effect)
{
    ALOGD("%s", __func__);
    return 0;
}

static int in_set_gain(struct audio_stream_in *stream, float gain)
{
    return 0;
}

static ssize_t in_read(struct audio_stream_in *stream, void* buffer, size_t bytes)
{
    int ret;
    struct stream_in *in = (struct stream_in *)stream;

    //ALOGD("%s ->", __func__);
    //ALOGD("in_read() USB HAL reading %d", bytes);
    pthread_mutex_lock(&in->dev->lock);
    pthread_mutex_lock(&in->lock);
    if (in->standby) {
        ret = start_input_stream(in);
        if (ret != 0) {
            goto err;
        }
        in->standby = false;
    }

#ifdef USE_MMAP
    ret = pcm_mmap_read(in->pcm, (void *)buffer, bytes);
#else
    ret = pcm_read(in->pcm, (void *)buffer, bytes);
#endif

    pthread_mutex_unlock(&in->lock);
    pthread_mutex_unlock(&in->dev->lock);

    //ALOGD("%s <- %d", __func__, ret);

#ifdef USE_MMAP
    return ret == 0 ? bytes : ret;
#else
    return ret == 0 ? bytes : 0;
#endif

err:
    ALOGE("in_read() ERR");
    pthread_mutex_unlock(&in->lock);
    pthread_mutex_unlock(&in->dev->lock);
    if (ret != 0) {
        usleep(bytes * 1000000 / audio_stream_frame_size(&stream->common) /
               in_get_sample_rate(&stream->common));
    }

    return bytes;
}

static uint32_t in_get_input_frames_lost(struct audio_stream_in *stream)
{
    return 0;
}

static int adev_open_input_stream(struct audio_hw_device *dev,
                                  audio_io_handle_t handle,
                                  audio_devices_t devices,
                                  struct audio_config *config,
                                  struct audio_stream_in **stream_in)
{
    struct audio_device *adev = (struct audio_device *)dev;
    struct stream_in *in;
    int ret;

    ALOGD("%s ->", __func__);
    in = (struct stream_in *)calloc(1, sizeof(struct stream_in));
    if (!in)
        return -ENOMEM;

    in->stream.common.get_sample_rate = in_get_sample_rate;//
    in->stream.common.set_sample_rate = in_set_sample_rate;//
    in->stream.common.get_buffer_size = in_get_buffer_size;//
    in->stream.common.get_channels = in_get_channels;//
    in->stream.common.get_format = in_get_format;//
    in->stream.common.set_format = in_set_format;//
    in->stream.common.standby = in_standby;//
    in->stream.common.dump = in_dump;//
    in->stream.common.set_parameters = in_set_parameters;//
    in->stream.common.get_parameters = in_get_parameters;//
    in->stream.common.add_audio_effect = in_add_audio_effect;//
    in->stream.common.remove_audio_effect = in_remove_audio_effect;//
    in->stream.set_gain = in_set_gain;//
    in->stream.read = in_read;
    in->stream.get_input_frames_lost = in_get_input_frames_lost;

    in->dev = adev;

    in_set_format(&in->stream.common, config->format);
    //in_set_channels(&in->stream.common, config->channel_mask);
    in_set_sample_rate(&in->stream.common, config->sample_rate);

    config->format = in_get_format(&in->stream.common);
    config->channel_mask = in_get_channels(&in->stream.common);
    config->sample_rate = in_get_sample_rate(&in->stream.common);

    in->standby = true;

    adev->card = -1;
    adev->device = -1;

    *stream_in = &in->stream;
    
    ALOGD("%s <-", __func__);
    
    return 0;

err_open:
    free(in);
    *stream_in = NULL;
    return ret;
}

static void adev_close_input_stream(struct audio_hw_device *dev,
                                     struct audio_stream_in *stream)
{
    struct stream_in *in = (struct stream_in *)stream;

    ALOGD("%s ->", __func__);
    in_standby(&stream->common);
    free(stream);
    ALOGD("%s <-", __func__);
}

static int adev_dump(const audio_hw_device_t *device, int fd)
{
    ALOGD("%s", __func__);
    return 0;
}

static int adev_close(hw_device_t *device)
{
    struct audio_device *adev = (struct audio_device *)device;
    ALOGD("%s ->", __func__);

    free(device);
    ALOGD("%s <-", __func__);
    return 0;
}

static int adev_open(const hw_module_t* module, const char* name,
                     hw_device_t** device)
{
    struct audio_device *adev;
    int ret;
    
    ALOGD("%s ->", __func__);

    if (strcmp(name, AUDIO_HARDWARE_INTERFACE) != 0)
        return -EINVAL;

    adev = (struct audio_device *) calloc(1, sizeof(struct audio_device));
    if (!adev)
        return -ENOMEM;

    adev->hw_device.common.tag = HARDWARE_DEVICE_TAG;
    adev->hw_device.common.version = AUDIO_DEVICE_API_VERSION_2_0;
    adev->hw_device.common.module = (struct hw_module_t *) module;
    adev->hw_device.common.close = adev_close;

    adev->hw_device.init_check = adev_init_check;
    adev->hw_device.set_voice_volume = adev_set_voice_volume;
    adev->hw_device.set_master_volume = adev_set_master_volume;
    adev->hw_device.set_mode = adev_set_mode;
    adev->hw_device.set_mic_mute = adev_set_mic_mute;
    adev->hw_device.get_mic_mute = adev_get_mic_mute;
    adev->hw_device.set_parameters = adev_set_parameters;
    adev->hw_device.get_parameters = adev_get_parameters;
    adev->hw_device.get_input_buffer_size = adev_get_input_buffer_size;
    adev->hw_device.open_output_stream = adev_open_output_stream;
    adev->hw_device.close_output_stream = adev_close_output_stream;
    adev->hw_device.open_input_stream = adev_open_input_stream;
    adev->hw_device.close_input_stream = adev_close_input_stream;
    adev->hw_device.dump = adev_dump;

    *device = &adev->hw_device.common;
    ALOGD("%s <-", __func__);

    return 0;
}

static struct hw_module_methods_t hal_module_methods = {
    .open = adev_open,
};

struct audio_module HAL_MODULE_INFO_SYM = {
    .common = {
        .tag = HARDWARE_MODULE_TAG,
        .module_api_version = AUDIO_MODULE_API_VERSION_0_1,
        .hal_api_version = HARDWARE_HAL_API_VERSION,
        .id = AUDIO_HARDWARE_MODULE_ID,
        .name = "USB audio HW HAL",
        .author = "The Android Open Source Project",
        .methods = &hal_module_methods,
    },
};
