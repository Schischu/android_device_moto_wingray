/*
 * Copyright (C) 2011 The Android Open Source Project
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

#define LOG_TAG "audio_hw_stingray"
#define LOG_NDEBUG 0

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <math.h>
#include <sys/ioctl.h>

#include <cutils/log.h>
#include <cutils/properties.h>
#include <cutils/str_parms.h>

#include <hardware/audio.h>
#include <hardware/hardware.h>

#include <system/audio.h>

#include <linux/cpcap_audio.h>
#include <linux/tegra_audio.h>

struct audio_device {
    struct audio_hw_device device;

    int init;

    int fd_cpcap_ctl;   //int mCpcapCtlFd;

    int cpcap_out_rate; //int mHwOutRate;
    int cpcap_in_rate;  //int mHwInRate;

    struct cpcap_audio_stream cpcap_out_device; //struct cpcap_audio_stream mCurOutDevice;
    struct cpcap_audio_stream cpcap_in_device;  //struct cpcap_audio_stream mCurInDevice;

    float vol_master;
};

struct stream_out {
    struct audio_stream_out stream;

    struct audio_device *dev;               //AudioHardware* mHardware;

    int              fd_out;
    int              fd_out_ctl;
    int              fd_out_bt;
    int              fd_out_bt_ctl;
    int              fd_out_bt_io_ctl;
    int              fd_out_spdif;
    int              fd_out_spdif_ctl;
    
    int standby;
};

struct stream_in {
    struct audio_stream_in stream;

    struct audio_device *dev;               //AudioHardware* mHardware;
};

static uint32_t out_get_sample_rate(const struct audio_stream *stream)
{
    struct stream_out *out = (struct stream_out *)stream;

    ALOGV("%s ->< %u", __func__, out->dev->cpcap_out_rate);

    //We only support 44.1kHz
    return out->dev->cpcap_out_rate;
}

static int out_set_sample_rate(struct audio_stream *stream, uint32_t rate)
{
    ALOGV("%s -><", __func__);
    return 0;
}

static size_t out_get_buffer_size(const struct audio_stream *stream)
{
    ALOGV("%s ->< 4096", __func__);

    //We do not care about the output buffer size, so lets just say 4096
    return 4096;
}

static audio_channel_mask_t out_get_channels(const struct audio_stream *stream)
{
    ALOGV("%s ->< AUDIO_CHANNEL_OUT_STEREO", __func__);

    //We only support stereo output
    return AUDIO_CHANNEL_OUT_STEREO;
}

static audio_format_t out_get_format(const struct audio_stream *stream)
{
    ALOGV("%s ->< AUDIO_FORMAT_PCM_16_BIT", __func__);

    //We only support 16 bit
    return AUDIO_FORMAT_PCM_16_BIT;
}

static int out_set_format(struct audio_stream *stream, audio_format_t format)
{
    ALOGV("%s -><", __func__);
    return 0;
}

static int out_standby(struct audio_stream *stream)
{
    struct stream_out *out = (struct stream_out *)stream;
    struct cpcap_audio_stream standby;
    
    ALOGV("%s ->", __func__);
    
    standby.id = CPCAP_AUDIO_OUT_STANDBY;
    standby.on = 1;
    
    ALOGV("AudioStreamOutTegra::flush()");
    if (ioctl(out->fd_out_ctl, TEGRA_AUDIO_OUT_FLUSH) < 0)
       ALOGE("could not flush playback: %s", strerror(errno));
    //if (::ioctl(out->fd_out_bt_ctl, TEGRA_AUDIO_OUT_FLUSH) < 0)
    //   ALOGE("could not flush bluetooth: %s", strerror(errno));
    //if (mSpdifFdCtl >= 0 && ::ioctl(mSpdifFdCtl, TEGRA_AUDIO_OUT_FLUSH) < 0)
    //   ALOGE("could not flush spdif: %s", strerror(errno));
    ALOGV("AudioStreamOutTegra::flush() returns");

    if (ioctl(out->dev->fd_cpcap_ctl, CPCAP_AUDIO_OUT_SET_OUTPUT, &standby) < 0)
    {
        ALOGE("%s: could not turn off current output device (%d, on %d): %s",
              __func__, standby.id, standby.on,
              strerror(errno));
    }

    if (ioctl(out->dev->fd_cpcap_ctl, CPCAP_AUDIO_OUT_GET_OUTPUT, &out->dev->cpcap_out_device) < 0) {
        ALOGE("%s: could not get current output device after standby: %s", __func__, strerror(errno));
    }

    ALOGV("%s: after standby %s, output device %d is %s", __func__,
         "enable" , out->dev->cpcap_out_device.id,
         out->dev->cpcap_out_device.on ? "on" : "off");
    
    out->standby = 1;
    
    return 0;
}

static int out_dump(const struct audio_stream *stream, int fd)
{
    ALOGV("%s -><", __func__);
    return 0;
}

static int out_set_parameters(struct audio_stream *stream, const char *kvpairs)
{
    return 0;
}

static char * out_get_parameters(const struct audio_stream *stream, const char *keys)
{
    ALOGV("%s -><", __func__);
    return strdup("");
}

static uint32_t out_get_latency(const struct audio_stream_out *stream)
{
    ALOGV("%s ->< 0", __func__);
    
    //TODO: How to get latency value
    return 0;
}

static int out_set_volume(struct audio_stream_out *stream, float left,
                          float right)
{
    ALOGV("%s -><", __func__);
    return 0;
}

static ssize_t out_write(struct audio_stream_out *stream, const void* buffer,
                         size_t bytes)
{
    struct stream_out *out = (struct stream_out *)stream;
    struct cpcap_audio_stream standby;

    //ALOGV("%s -> bytes: %u", __func__, bytes);

    if (out->standby == 1)
    {
        standby.id = CPCAP_AUDIO_OUT_STANDBY;
        standby.on = 0;

        if (ioctl(out->dev->fd_cpcap_ctl, CPCAP_AUDIO_OUT_SET_OUTPUT, &standby) < 0)
        {
            ALOGE("could not set output (%d, on %d): %s",
                  standby.id, standby.on,
                  strerror(errno));
        }

        if (ioctl(out->dev->fd_cpcap_ctl, CPCAP_AUDIO_OUT_GET_OUTPUT, &out->dev->cpcap_out_device) < 0) {
            ALOGE("%s: could not get current output device after standby: %s", __func__, strerror(errno));
        }

        ALOGV("%s: after standby %s, output device %d is %s", __func__,
             "enable" , out->dev->cpcap_out_device.id,
             out->dev->cpcap_out_device.on ? "on" : "off");
    
        
        ALOGV("AudioStreamOutTegra::flush()");
        if (ioctl(out->fd_out_ctl, TEGRA_AUDIO_OUT_FLUSH) < 0)
           ALOGE("could not flush playback: %s", strerror(errno));
        //if (::ioctl(out->fd_out_bt_ctl, TEGRA_AUDIO_OUT_FLUSH) < 0)
        //   ALOGE("could not flush bluetooth: %s", strerror(errno));
        //if (mSpdifFdCtl >= 0 && ::ioctl(mSpdifFdCtl, TEGRA_AUDIO_OUT_FLUSH) < 0)
        //   ALOGE("could not flush spdif: %s", strerror(errno));
        ALOGV("AudioStreamOutTegra::flush() returns");
        
        int numBufs = 4;
        ALOGV("AudioStreamOutTegra::setNumBufs(%d)", numBufs);
        if (ioctl(out->fd_out_ctl, TEGRA_AUDIO_OUT_SET_NUM_BUFS, &numBufs) < 0)
           ALOGE("could not set number of output buffers: %s", strerror(errno));
        
        if (ioctl(out->dev->fd_cpcap_ctl, CPCAP_AUDIO_OUT_SET_RATE, out->dev->cpcap_out_rate) < 0) {
            ALOGE("could not set output rate: %s", strerror(errno));
        }
        
        out->standby = 0;
    }

#if 0
    if (out->dev->cpcap_out_device.id != CPCAP_AUDIO_OUT_SPEAKER)
    {
        out->dev->cpcap_out_device.id = CPCAP_AUDIO_OUT_SPEAKER;
        out->dev->cpcap_out_device.on = 1;

        if (ioctl(out->dev->fd_cpcap_ctl, CPCAP_AUDIO_OUT_SET_OUTPUT, &out->dev->cpcap_out_device) < 0)
        {
            ALOGE("could not set output (%d, on %d): %s",
                  out->dev->cpcap_out_device.id, out->dev->cpcap_out_device.on,
                 strerror(errno));
        }
    }
#endif

    bytes = write(out->fd_out, buffer, bytes);

#if 0
    /* XXX: fake timing for audio output */
    usleep(bytes * 1000000 / audio_stream_out_frame_size(stream) /
           out_get_sample_rate(&stream->common));
#endif
    //ALOGV("%s -< bytes: %u", __func__, bytes);
    return bytes;
}

static int out_get_render_position(const struct audio_stream_out *stream,
                                   uint32_t *dsp_frames)
{
    ALOGV("%s -><", __func__);
    return -EINVAL;
}

static int out_add_audio_effect(const struct audio_stream *stream, effect_handle_t effect)
{
    ALOGV("%s -><", __func__);
    return 0;
}

static int out_remove_audio_effect(const struct audio_stream *stream, effect_handle_t effect)
{
    ALOGV("%s -><", __func__);
    return 0;
}

#ifndef ICS_AUDIO_BLOB
static int out_get_next_write_timestamp(const struct audio_stream_out *stream,
                                        int64_t *timestamp)
{
    //ALOGV("%s ->< -EINVAL", __func__);

    //TODO: Do we need this?
    //TODO: enable when supported by driver
    return -EINVAL;
}
#endif

/** audio_stream_in implementation **/
static uint32_t in_get_sample_rate(const struct audio_stream *stream)
{
    ALOGV("%s -><", __func__);
    return 8000;
}

static int in_set_sample_rate(struct audio_stream *stream, uint32_t rate)
{
    ALOGV("%s -><", __func__);
    return 0;
}

static size_t in_get_buffer_size(const struct audio_stream *stream)
{
    ALOGV("%s -><", __func__);
    return 320;
}

static audio_channel_mask_t in_get_channels(const struct audio_stream *stream)
{
    return AUDIO_CHANNEL_IN_MONO;
}

static audio_format_t in_get_format(const struct audio_stream *stream)
{
    ALOGV("%s -><", __func__);
    return AUDIO_FORMAT_PCM_16_BIT;
}

static int in_set_format(struct audio_stream *stream, audio_format_t format)
{
    ALOGV("%s -><", __func__);
    return 0;
}

static int in_standby(struct audio_stream *stream)
{
    ALOGV("%s -><", __func__);
    return 0;
}

static int in_dump(const struct audio_stream *stream, int fd)
{
    ALOGV("%s -><", __func__);
    return 0;
}

static int in_set_parameters(struct audio_stream *stream, const char *kvpairs)
{
    ALOGV("%s -><", __func__);
    return 0;
}

static char * in_get_parameters(const struct audio_stream *stream,
                                const char *keys)
{
    ALOGV("%s -><", __func__);
    return strdup("");
}

static int in_set_gain(struct audio_stream_in *stream, float gain)
{
    ALOGV("%s -><", __func__);
    return 0;
}

static ssize_t in_read(struct audio_stream_in *stream, void* buffer,
                       size_t bytes)
{
    ALOGV("%s ->", __func__);
    /* XXX: fake timing for audio input */
    usleep(bytes * 1000000 / audio_stream_in_frame_size(stream) /
           in_get_sample_rate(&stream->common));
           
    ALOGV("%s -< bytes: %u", __func__, bytes);
    return bytes;
}

static uint32_t in_get_input_frames_lost(struct audio_stream_in *stream)
{
    ALOGV("%s -><", __func__);
    return 0;
}

static int in_add_audio_effect(const struct audio_stream *stream, effect_handle_t effect)
{
    ALOGV("%s -><", __func__);
    return 0;
}

static int in_remove_audio_effect(const struct audio_stream *stream, effect_handle_t effect)
{
    ALOGV("%s -><", __func__);
    return 0;
}

static int adev_open_output_stream(struct audio_hw_device *dev,
                                   audio_io_handle_t handle,
                                   audio_devices_t devices,
                                   audio_output_flags_t flags,
                                   struct audio_config *config,
                                   struct audio_stream_out **stream_out,
                                   const char *address __unused)
{
    struct audio_device *adev = (struct audio_device *)dev;
    struct stream_out *out;
    int ret;

    ALOGV("%s ->", __func__);

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
#ifndef ICS_AUDIO_BLOB
    out->stream.get_next_write_timestamp = out_get_next_write_timestamp;
#endif

    out->fd_out           = open("/dev/audio0_out", O_RDWR);
    out->fd_out_ctl       = open("/dev/audio0_out_ctl", O_RDWR);

    out->fd_out_bt        = open("/dev/audio1_out", O_RDWR);
    out->fd_out_bt_ctl    = open("/dev/audio1_out_ctl", O_RDWR);
    out->fd_out_bt_io_ctl = open("/dev/audio1_ctl", O_RDWR);

    out->fd_out_spdif     = open("/dev/spdif_out", O_RDWR);
    out->fd_out_spdif_ctl = open("/dev/spdif_out_ctl", O_RDWR);

    if (out->fd_out < 0       || out->fd_out_ctl < 0       ||
        out->fd_out_bt < 0    || out->fd_out_bt_ctl < 0    || out->fd_out_bt_io_ctl < 0 ||
        out->fd_out_spdif < 0 || out->fd_out_spdif_ctl < 0)
    {
        ret = -1;
        goto err_open;
    }

    out->standby = 0;

    out->dev = adev;

    *stream_out = &out->stream;
    ALOGV("%s -<", __func__);
    return 0;

err_open:
    free(out);
    *stream_out = NULL;
    ALOGV("%s -< err: %u", __func__, ret);
    return ret;
}

static void adev_close_output_stream(struct audio_hw_device *dev,
                                     struct audio_stream_out *stream)
{
    ALOGV("%s ->", __func__);
    free(stream);
    ALOGV("%s -<", __func__);
}

static int adev_set_parameters(struct audio_hw_device *dev, const char *kvpairs)
{
    ALOGV("%s -><", __func__);
    return -ENOSYS;
}

static char * adev_get_parameters(const struct audio_hw_device *dev,
                                  const char *keys)
{
    ALOGV("%s -><", __func__);
    return NULL;
}

static int adev_init_check(const struct audio_hw_device *dev)
{
    struct audio_device *adev = (struct audio_device *)dev;
    int ret = 0;

    ALOGV("%s ->", __func__);

    ret =  adev->init ? 0 : -1;

    ALOGV("%s -< ret: %d", __func__, ret);
    return ret;
}

static int adev_set_voice_volume(struct audio_hw_device *dev, float volume)
{
    ALOGV("%s ->< %f", __func__, volume);
    return -ENOSYS;
}

static int adev_set_master_volume(struct audio_hw_device *dev, float volume)
{
    struct audio_device *adev = (struct audio_device *)dev;
    float volume_multiplied = volume * CPCAP_AUDIO_OUT_VOL_MAX;
    unsigned int volume_cpcap;

    ALOGV("%s -> %f", __func__, volume);
    
    volume_cpcap = volume_multiplied + 0.5f;
    
    if (ioctl(adev->fd_cpcap_ctl, CPCAP_AUDIO_OUT_SET_VOLUME, volume_cpcap) < 0) {
        ALOGE("could not set volume: %s", strerror(errno));
    }
    
    adev->vol_master = volume;
    
    ALOGV("%s -< ret: %d", __func__, 0);
    return 0;
}

#ifndef ICS_AUDIO_BLOB
static int adev_get_master_volume(struct audio_hw_device *dev, float *volume)
{
    struct audio_device *adev = (struct audio_device *)dev;

    ALOGV("%s ->", __func__);

    *volume = adev->vol_master;

    ALOGV("%s -< ret: %d *volume: %f", __func__, 0, *volume);
    return 0;
}

static int adev_set_master_mute(struct audio_hw_device *dev, bool muted)
{
    ALOGV("%s -><", __func__);
    return -ENOSYS;
}

static int adev_get_master_mute(struct audio_hw_device *dev, bool *muted)
{
    ALOGV("%s -><", __func__);
    return -ENOSYS;
}
#endif

static int adev_set_mode(struct audio_hw_device *dev, audio_mode_t mode)
{
    return 0;
}

static int adev_set_mic_mute(struct audio_hw_device *dev, bool state)
{
    ALOGV("%s -><", __func__);
    return -ENOSYS;
}

static int adev_get_mic_mute(const struct audio_hw_device *dev, bool *state)
{
    ALOGV("%s -><", __func__);
    return -ENOSYS;
}

static size_t adev_get_input_buffer_size(const struct audio_hw_device *dev,
                                         const struct audio_config *config)
{
    ALOGV("%s -><", __func__);
    return 320;
}

static int adev_open_input_stream(struct audio_hw_device *dev,
                                  audio_io_handle_t handle,
                                  audio_devices_t devices,
                                  struct audio_config *config,
                                  struct audio_stream_in **stream_in,
                                  audio_input_flags_t flags __unused,
                                  const char *address __unused,
                                  audio_source_t source __unused)
{
    struct audio_device *adev = (struct audio_device *)dev;
    struct stream_in *in;
    int ret;

    ALOGV("%s ->", __func__);

    in = (struct stream_in *)calloc(1, sizeof(struct stream_in));
    if (!in)
        return -ENOMEM;

    in->stream.common.get_sample_rate = in_get_sample_rate;
    in->stream.common.set_sample_rate = in_set_sample_rate;
    in->stream.common.get_buffer_size = in_get_buffer_size;
    in->stream.common.get_channels = in_get_channels;
    in->stream.common.get_format = in_get_format;
    in->stream.common.set_format = in_set_format;
    in->stream.common.standby = in_standby;
    in->stream.common.dump = in_dump;
    in->stream.common.set_parameters = in_set_parameters;
    in->stream.common.get_parameters = in_get_parameters;
    in->stream.common.add_audio_effect = in_add_audio_effect;
    in->stream.common.remove_audio_effect = in_remove_audio_effect;
    in->stream.set_gain = in_set_gain;
    in->stream.read = in_read;
    in->stream.get_input_frames_lost = in_get_input_frames_lost;

    *stream_in = &in->stream;
    ALOGV("%s -<", __func__);
    return 0;

err_open:
    free(in);
    *stream_in = NULL;
    ALOGV("%s -> err: %u", __func__, ret);
    return ret;
}

static void adev_close_input_stream(struct audio_hw_device *dev,
                                   struct audio_stream_in *in)
{
    ALOGV("%s -><", __func__);
    return;
}

static int adev_dump(const audio_hw_device_t *device, int fd)
{
    ALOGV("%s -><", __func__);
    return 0;
}

static int adev_close(hw_device_t *device)
{
    ALOGV("%s ->", __func__);
    free(device);
    ALOGV("%s -<", __func__);
    return 0;
}

static int adev_open(const hw_module_t* module, const char* name,
                     hw_device_t** device)
{
    struct audio_device *adev;
    int ret;

    ALOGV("%s ->", __func__);

    if (strcmp(name, AUDIO_HARDWARE_INTERFACE) != 0)
    {
        ALOGV("%s -< -EINVAL", __func__);
        return -EINVAL;
    }

    adev = calloc(1, sizeof(struct audio_device));
    if (!adev)
    {
        ALOGV("%s -< -ENOMEM", __func__);
        return -ENOMEM;
    }

    adev->device.common.tag = HARDWARE_DEVICE_TAG;
    adev->device.common.version = AUDIO_DEVICE_API_VERSION_2_0;
    adev->device.common.module = (struct hw_module_t *) module;
    adev->device.common.close = adev_close;

    adev->device.init_check = adev_init_check;
    adev->device.set_voice_volume = adev_set_voice_volume;
    adev->device.set_master_volume = adev_set_master_volume;
#ifndef ICS_AUDIO_BLOB
    adev->device.get_master_volume = adev_get_master_volume;
    adev->device.set_master_mute = adev_set_master_mute;
    adev->device.get_master_mute = adev_get_master_mute;
#endif
    adev->device.set_mode = adev_set_mode;
    adev->device.set_mic_mute = adev_set_mic_mute;
    adev->device.get_mic_mute = adev_get_mic_mute;
    adev->device.set_parameters = adev_set_parameters;
    adev->device.get_parameters = adev_get_parameters;
#ifndef ICS_AUDIO_BLOB
    adev->device.get_input_buffer_size = adev_get_input_buffer_size;
    adev->device.open_output_stream = adev_open_output_stream;
    adev->device.open_input_stream = adev_open_input_stream;
#endif
    adev->device.close_output_stream = adev_close_output_stream;
    adev->device.close_input_stream = adev_close_input_stream;
    adev->device.dump = adev_dump;

    adev->fd_cpcap_ctl = open("/dev/audio_ctl", O_RDWR);
    if (adev->fd_cpcap_ctl < 0) {
        ALOGE("open /dev/audio_ctl failed: %s", strerror(errno));
        return -ENODEV;
    }

    if (ioctl(adev->fd_cpcap_ctl, CPCAP_AUDIO_OUT_GET_OUTPUT, &adev->cpcap_out_device) < 0) {
        ALOGE("could not get output device: %s", strerror(errno));
    }
    if (ioctl(adev->fd_cpcap_ctl, CPCAP_AUDIO_IN_GET_INPUT, &adev->cpcap_in_device) < 0) {
        ALOGE("could not get input device: %s", strerror(errno));
    }
    
    if (ioctl(adev->fd_cpcap_ctl, CPCAP_AUDIO_OUT_GET_RATE, &adev->cpcap_out_rate) < 0) {
        ALOGE("could not get output rate: %s", strerror(errno));
    }
    ALOGE("output (%d, on %d)", adev->cpcap_out_device.id, adev->cpcap_out_device.on);
    
    if (adev->cpcap_out_rate == 0) {
        adev->cpcap_out_rate = 44100;

        if (ioctl(adev->fd_cpcap_ctl, CPCAP_AUDIO_OUT_SET_RATE, adev->cpcap_out_rate) < 0) {
            ALOGE("could not set output rate: %s", strerror(errno));
        }
    }
    
    if (ioctl(adev->fd_cpcap_ctl, CPCAP_AUDIO_IN_GET_RATE, &adev->cpcap_in_rate) < 0) {
        ALOGE("could not get input rate: %s", strerror(errno));
    }
    
    unsigned int volume_output = 0;
    if (ioctl(adev->fd_cpcap_ctl, CPCAP_AUDIO_OUT_GET_VOLUME, &volume_output) < 0) {
        ALOGE("could not get output volume: %s", strerror(errno));
    }
    
    adev->vol_master = (volume_output * 1.0f) / CPCAP_AUDIO_OUT_VOL_MAX;
    
    adev->init = 1;

    *device = &adev->device.common;

    ALOGV("%s -<", __func__);
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
        .name = "Default audio HW HAL",
        .author = "The Android Open Source Project",
        .methods = &hal_module_methods,
    },
};
