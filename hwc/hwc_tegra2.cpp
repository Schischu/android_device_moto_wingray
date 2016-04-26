/*
 * Copyright (C) 2011 The Android Open Source Project
 * Copyright (C) 2012 Eduardo José Tagle <ejtagle@tutopia.com>
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

#define LOG_NDEBUG 0
#define LOG_TAG "hwc"

#include <utils/Log.h>

#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <poll.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <time.h>
#if HAVE_ANDROID_OS
#include <linux/fb.h>
#endif

#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/resource.h>

#include <cutils/compiler.h>
#include <cutils/log.h>
#include <cutils/iosched_policy.h>
#include <cutils/properties.h>
#include <hardware/gralloc.h>
#include <hardware/hardware.h>
#include <hardware/hwcomposer.h>
#include <hardware_legacy/uevent.h>
#include <system/thread_defs.h>
#include <utils/AndroidThreads.h>
#include <utils/String8.h>
#include <utils/Vector.h>

#include "hwcomposer_v0.h"

#define likely(x)       __builtin_expect(!!(x), 1)
#define unlikely(x)     __builtin_expect(!!(x), 0)

// Get the original hw composer
static hwc_module_t* get_hwc(void)
{
    /* The original library implementation */
    static hwc_module_t* orghwc = {NULL};

    if (!orghwc) {
        void* handle;
        ALOGI("Resolving original HWC module...");

        handle = dlopen("/system/lib/hw/hwcomposer.tegra_v0.so",RTLD_LAZY);
        if(!handle) {
            ALOGE("Unable to load original hwc module");
            return NULL;
        }

        orghwc = (hwc_module_t*) dlsym(handle,HAL_MODULE_INFO_SYM_AS_STR);
        if (!orghwc) {
            ALOGE("Unable to resolve HWC v0 HMI");
            dlclose(handle);
            handle = NULL;
            return NULL;
        }
        ALOGI("Loaded the original HWC module");
    }

    return orghwc;
}

struct tegra2_hwc_composer_device_1_t {
    hwc_composer_device_1_t base;
    hwc_composer_device_t* org;
    const hwc_procs_t *procs;
    pthread_t vsync_thread;
    volatile bool vsync_running;

    unsigned long long time_between_frames_ns;// ns
    unsigned long time_between_frames_us; // us

    pthread_mutex_t vsync_mutex;
    pthread_cond_t vsync_cond;
    volatile bool enabled_vsync;

    // NVidia implementation
    int         nvhost_fd;
    unsigned int vblank_syncpt_id;

    // Buffer to do translations to speed them up
    void*       prepare_xlatebuf;
    int         prepare_xlatebufsz;
    void*       set_xlatebuf;
    int         set_xlatebufsz;

    // Misc info
    int         fb_fd;
    int32_t     xres;
    int32_t     yres;
    int32_t     xdpi;
    int32_t     ydpi;
    int32_t     vsync_period;

    volatile bool fbblanked;    // Framebuffer disabled
};

static void copy_layer1_to_layer(hwc_layer_t* dst,hwc_layer_1_t* src)
{
    dst->compositionType = src->compositionType;
    dst->hints = src->hints;
    dst->flags = src->flags;
    dst->handle = src->handle;
    dst->transform = src->transform;
    dst->blending = src->blending;
    // memcpy(&dst->sourceCrop,&src->sourceCrop,sizeof( hwc_rect_t ));
    // memcpy(&dst->displayFrame,&src->displayFrame,sizeof( hwc_rect_t ));
    // memcpy(&dst->visibleRegionScreen,&src->visibleRegionScreen,sizeof( hwc_region_t ));

    dst->sourceCrop.left = src->sourceCrop.left;
    dst->sourceCrop.top = src->sourceCrop.top;
    dst->sourceCrop.bottom = src->sourceCrop.bottom;
    dst->sourceCrop.right = src->sourceCrop.right;
    dst->displayFrame.left = src->displayFrame.left;
    dst->displayFrame.top = src->displayFrame.top;
    dst->displayFrame.bottom = src->displayFrame.bottom;
    dst->displayFrame.right = src->displayFrame.right;

    // dst->visibleRegionScreen.numRects = src->visibleRegionScreen.numRects;
    // dst->visibleRegionScreen.rects->left = src->visibleRegionScreen.rects->left;
    // dst->visibleRegionScreen.rects->top; = src->visibleRegionScreen.rects->top;
    // dst->visibleRegionScreen.rects->bottom = src->visibleRegionScreen.rects->bottom;
    // dst->visibleRegionScreen.rects->right = src->visibleRegionScreen.rects->right;

    memcpy(&dst->visibleRegionScreen,&src->visibleRegionScreen,sizeof( hwc_region_t ));
}

static void copy_layer_to_layer1(hwc_layer_1_t* dst,hwc_layer_t* src)
{
    dst->compositionType = src->compositionType;
    dst->hints = src->hints;
    dst->flags = src->flags;
    dst->handle = src->handle;
    dst->transform = src->transform;
    dst->blending = src->blending;
    // memcpy(&dst->sourceCrop,&src->sourceCrop,sizeof( hwc_rect_t ));
    // memcpy(&dst->displayFrame,&src->displayFrame,sizeof( hwc_rect_t ));
    // memcpy(&dst->visibleRegionScreen,&src->visibleRegionScreen,sizeof( hwc_region_t ));

    dst->sourceCrop.left = src->sourceCrop.left;
    dst->sourceCrop.top = src->sourceCrop.top;
    dst->sourceCrop.bottom = src->sourceCrop.bottom;
    dst->sourceCrop.right = src->sourceCrop.right;
    dst->displayFrame.left = src->displayFrame.left;
    dst->displayFrame.top = src->displayFrame.top;
    dst->displayFrame.bottom = src->displayFrame.bottom;
    dst->displayFrame.right = src->displayFrame.right;

    // dst->visibleRegionScreen.numRects = src->visibleRegionScreen.numRects;
    // dst->visibleRegionScreen.rects->left = src->visibleRegionScreen.rects->left;
    // dst->visibleRegionScreen.rects->top; = src->visibleRegionScreen.rects->top;
    // dst->visibleRegionScreen.rects->bottom = src->visibleRegionScreen.rects->bottom;
    // dst->visibleRegionScreen.rects->right = src->visibleRegionScreen.rects->right;
    memcpy(&dst->visibleRegionScreen,&src->visibleRegionScreen,sizeof( hwc_region_t ));
}

static void copy_display_contents_1_to_layer_list(hwc_layer_list_t* dst,hwc_display_contents_1_t* src)
{
    dst->flags = src->flags;
    unsigned int s,d;
    for (s = 0, d = 0; s < src->numHwLayers; s++) {
        copy_layer1_to_layer(&dst->hwLayers[d++],&src->hwLayers[s]);
    }
    dst->numHwLayers = d;
}

static void copy_layer_list_to_display_contents_1(hwc_display_contents_1_t* dst,hwc_layer_list_t* src)
{
    dst->flags = src->flags;
    unsigned int s,d;
    for (s = 0, d = 0; d < dst->numHwLayers; d++) {
        copy_layer_to_layer1(&dst->hwLayers[d],&src->hwLayers[s++]);
    }
}

static int tegra2_set(struct hwc_composer_device_1 *dev,
        size_t numDisplays, hwc_display_contents_1_t** displays)
{
    if (!numDisplays || !displays)
        return 0;

    tegra2_hwc_composer_device_1_t *pdev = (tegra2_hwc_composer_device_1_t *)dev;

    hwc_display_contents_1_t *contents = displays[0];
    if (!contents || !contents->numHwLayers)
        return 0;

    if (!contents->dpy || !contents->sur)
        return 0;

    // If blanking, make gralloc handle everything to aovid crashes
    if (pdev->fbblanked)
        return -ENODEV;

    int reqsz = sizeof (hwc_layer_list_t) + sizeof(hwc_layer_t) * contents->numHwLayers;

    // Make sure we have enough space on the translation buffer
    if (pdev->set_xlatebufsz < reqsz) {
        if (!pdev->set_xlatebuf) {
            pdev->set_xlatebuf = malloc(reqsz);
        } else {
            pdev->set_xlatebuf = realloc(pdev->set_xlatebuf,reqsz);
        }
        pdev->set_xlatebufsz = reqsz;
    }
    hwc_layer_list_t* lst = (hwc_layer_list_t*) pdev->set_xlatebuf;

    copy_display_contents_1_to_layer_list(lst,contents);
    int ret = pdev->org->set(pdev->org, contents->dpy, contents->sur, lst);
    copy_layer_list_to_display_contents_1(contents,lst);

    //Wait until all buffers are available
    unsigned int d;
    for (d = 0; d < contents->numHwLayers; d++) {

        // Release handles we own...
        if (contents->hwLayers[d].acquireFenceFd >= 0)
            close(contents->hwLayers[d].acquireFenceFd);
        contents->hwLayers[d].acquireFenceFd = -1;

        // And let surfaceFlinger read inmediately...
        contents->hwLayers[d].releaseFenceFd = -1;
    }

    return ret;
}

static int tegra2_prepare(hwc_composer_device_1_t *dev,
        size_t numDisplays, hwc_display_contents_1_t** displays)
{
    if (!numDisplays || !displays)
        return 0;

    tegra2_hwc_composer_device_1_t *pdev =
        (tegra2_hwc_composer_device_1_t *)dev;

    hwc_display_contents_1_t *contents = displays[0];
    if (!contents || !contents->numHwLayers)
        return 0;

    // If blanking, make gralloc handle everything to aovid crashes
    if (pdev->fbblanked)
        return -ENODEV;

    ALOGV("preparing %u layers", contents->numHwLayers);

    int reqsz = sizeof (hwc_layer_list_t) + sizeof(hwc_layer_t) * contents->numHwLayers;
    // Make sure we have enough space on the translation buffer
    if (pdev->prepare_xlatebufsz < reqsz) {
        if (!pdev->prepare_xlatebuf) {
            pdev->prepare_xlatebuf = malloc(reqsz);
        } else {
            pdev->prepare_xlatebuf = realloc(pdev->prepare_xlatebuf,reqsz);
        }
        pdev->prepare_xlatebufsz = reqsz;
    }
    hwc_layer_list_t* lst = (hwc_layer_list_t*) pdev->prepare_xlatebuf;

#ifdef SAMSUNG_T20_HWCOMPOSER
    /* Buggy Samsung hwcomposer workaround */
    if (contents->numHwLayers == 3) {

        ALOGV("layer 0 transform %u. layer 1 transform %u",
            contents->hwLayers[0].transform, contents->hwLayers[1].transform);

        if (contents->hwLayers[0].transform == 0 &&
            contents->hwLayers[0].displayFrame.top == 0 &&
            contents->hwLayers[0].displayFrame.bottom == pdev->yres &&
            // StatusBar
            contents->hwLayers[1].sourceCrop.top == 0 &&
            contents->hwLayers[1].sourceCrop.bottom == pdev->yres &&
            contents->hwLayers[1].displayFrame.top == 0 &&
            contents->hwLayers[1].displayFrame.bottom == pdev->yres) {
            // int bottom_pad = contents->hwLayers[2].sourceCrop.bottom -
            //     contents->hwLayers[2].sourceCrop.top;
            int bottom_pad = 1;

            contents->hwLayers[0].displayFrame.bottom =
                contents->hwLayers[0].displayFrame.bottom - bottom_pad;
            contents->hwLayers[0].sourceCrop.bottom =
                contents->hwLayers[0].sourceCrop.bottom - bottom_pad;
        }
    }
#endif

    copy_display_contents_1_to_layer_list(lst,contents);

    int ret = pdev->org->prepare(pdev->org, lst);

    copy_layer_list_to_display_contents_1(contents,lst);

    return ret;
}

/* VSync thread emulator */
static void *tegra2_hwc_emulated_vsync_thread(void *data)
{
     struct tegra2_hwc_composer_device_1_t *pdev =
            (struct tegra2_hwc_composer_device_1_t *) data;

    ALOGD("VSYNC thread emulator started");

    androidSetThreadPriority(0, HAL_PRIORITY_URGENT_DISPLAY
            + ANDROID_PRIORITY_MORE_FAVORABLE);
    android_set_rt_ioprio(0, 1);

    // Store the time of the start of this thread as an initial timestamp
    struct timespec nexttm;
    clock_gettime(CLOCK_MONOTONIC,&nexttm);
    signed long long nexttm_ns = (nexttm.tv_sec) * 1000000000LL + (nexttm.tv_nsec);

    while (1) {
        int err;

        // Wait while display is blanked
        pthread_mutex_lock(&pdev->vsync_mutex);
        if (unlikely(pdev->fbblanked && pdev->vsync_running)) {

            // When framebuffer is blanked, there must be no interrupts, so we can't wait on it
            pthread_cond_wait(&pdev->vsync_cond, &pdev->vsync_mutex);
        }
        if (unlikely(!pdev->vsync_running))
            break;
        pthread_mutex_unlock(&pdev->vsync_mutex);

        // Estimate time of next emulated VSYNC
        nexttm_ns += pdev->time_between_frames_ns;

        // get current time
        struct timespec currtm;
        clock_gettime(CLOCK_MONOTONIC,&currtm);

        // Calculate the delay for that next time
        signed long long deltatm_ns
            = nexttm_ns - ((currtm.tv_sec) * 1000000000LL + (currtm.tv_nsec));

        // If we lost lock on emulated VSYNC...
        if (deltatm_ns < 0 || deltatm_ns > (signed long long) pdev->time_between_frames_ns) {
            // Restart emulation

            // Store the current time as the time of the start of this thread
            clock_gettime(CLOCK_MONOTONIC,&nexttm);
            nexttm_ns = (nexttm.tv_sec) * 1000000000LL + (nexttm.tv_nsec);

            deltatm_ns = 0;
        }

        // If something to wait ... wait!
        if (deltatm_ns > 0) {

            struct timespec ts;
            ts.tv_sec = 0;
            ts.tv_nsec = deltatm_ns;

            do {
                err = nanosleep (&ts, &ts);
            } while (err < 0 && errno == EINTR);
        }

        // Do the VSYNC call
        if (likely(pdev->enabled_vsync && !pdev->fbblanked)) {

            // Get current time in exactly the same timebase as Choreographer
            struct timespec now;
            clock_gettime(CLOCK_MONOTONIC,&now);

            unsigned long long now_ns = (now.tv_sec) * 1000000000ULL + (now.tv_nsec);

            pdev->procs->vsync(pdev->procs, 0, now_ns);
        }

    };

    pthread_mutex_unlock(&pdev->vsync_mutex);

    ALOGD("VSYNC thread emulator ended");

    return NULL;
}

/* -- NVidia Tegra2/3 specific interfaces to sync to VBlanks */

#include <linux/nvhost_ioctl.h> /* nvidia specific headers from the linux kernel headers*/
//#include <video/tegra_dc_ext.h> /* nvidia specific headers from the linux kernel headers */

/* ... taken from nvidia linux kernel drivers ... */
#define NVSYNCPT_VBLANK0             (26)
#define NVSYNCPT_VBLANK1             (27)

#if 0
static int dc0_get_vblank_syncpt(void)
{
    // Try several DC0 interfaces
    int dc0_fd = open("/dev/tegra_dc0", O_RDWR); // Newer interface
    if (dc0_fd < 0)
        dc0_fd = open("/dev/tegra_dc_0", O_RDWR);// Older interface
    if (dc0_fd < 0) {
        ALOGE("Failed to open NVidia DC0 - Assuming default VBLANK0 syncpoint id");
        return NVSYNCPT_VBLANK0;
    }

    int syncpt = 0;
    if (ioctl(dc0_fd, TEGRA_DC_EXT_GET_VBLANK_SYNCPT, &syncpt) < 0) {
        ALOGE("Failed to get VBLANK0 syncpoint id - Assuming default VBLANK0 syncpoint id");
        close(dc0_fd);
        return NVSYNCPT_VBLANK0;
    }

    close(dc0_fd);
    ALOGD("Got VBLANK0 syncpoint: 0x%08x", syncpt);

    return syncpt;
}
#endif

static int nvhost_open(void)
{
    return open("/dev/nvhost-ctrl", O_RDWR);
}

static void nvhost_close(int ctrl_fd)
{
    if (ctrl_fd >= 0) {
        close(ctrl_fd);
    }
}

#if 0
static int nvhost_get_version(int ctrl_fd)
{
    struct nvhost_get_param_args gpa;
    if (ioctl(ctrl_fd, NVHOST_IOCTL_CTRL_GET_VERSION, &gpa) < 0)
        return NVHOST_SUBMIT_VERSION_V0;
    return gpa.value;
}
#endif

#if 0
static int nvhost_syncpt_read(int ctrl_fd, int id, unsigned int *syncpt)
{
    struct nvhost_ctrl_syncpt_read_args ra;
    ra.id = id;
    if (ioctl(ctrl_fd, NVHOST_IOCTL_CTRL_SYNCPT_READ, &ra) < 0)
        return -1;
    *syncpt = ra.value;
    return 0;
}

static int nvhost_syncpt_wait(int ctrl_fd, int id, int thresh, unsigned int timeout)
{
    struct nvhost_ctrl_syncpt_wait_args wa;
    wa.id = id;
    wa.thresh = thresh;
    wa.timeout = timeout;
    return ioctl(ctrl_fd, NVHOST_IOCTL_CTRL_SYNCPT_WAIT, &wa);
}

/* Wait VSync using NVidia SyncPoints */
static int tegra2_wait_vsync(struct tegra2_hwc_composer_device_1_t *pdev)
{
    unsigned int syncpt = 0;
    unsigned long max_wait_us = pdev->time_between_frames_us; // NVHOST_NO_TIMEOUT
    // unsigned long max_wait_us = 1000000000;

    /* get syncpt threshold */
    if (nvhost_syncpt_read(pdev->nvhost_fd, pdev->vblank_syncpt_id, &syncpt)) {
        ALOGE("Failed to read VBLANK syncpoint value!");
        return -1;
    }

    /* wait for the next value with timeout*/
    if (nvhost_syncpt_wait(pdev->nvhost_fd, pdev->vblank_syncpt_id, syncpt + 1, max_wait_us) < 0) {
        ALOGE("Failed to wait for VBLANK!");
        return -1;
    }

    /* Done waiting ! */
    return 0;
}

/* VSync thread using Nvidia syncpoint waits */
static void *tegra2_hwc_nv_vsync_thread(void *data)
{
     struct tegra2_hwc_composer_device_1_t *pdev =
            (struct tegra2_hwc_composer_device_1_t *) data;

    ALOGD("NVidia VSYNC thread started");

    androidSetThreadPriority(0, HAL_PRIORITY_URGENT_DISPLAY
            + ANDROID_PRIORITY_MORE_FAVORABLE);
    android_set_rt_ioprio(0, 1);

    while (1) {
        int err;

        // Wait while display is blanked
        pthread_mutex_lock(&pdev->vsync_mutex);
        if (unlikely(pdev->fbblanked && pdev->vsync_running)) {

            // When framebuffer is blanked, there must be no interrupts, so we can't wait on it
            pthread_cond_wait(&pdev->vsync_cond, &pdev->vsync_mutex);
        }
        if (unlikely(!pdev->vsync_running))
            break;
        pthread_mutex_unlock(&pdev->vsync_mutex);

        // Wait for the next vsync
        tegra2_wait_vsync(pdev);

        // Do the VSYNC call
        if (likely(pdev->enabled_vsync && !pdev->fbblanked)) {

            // Get current time in exactly the same timebase as Choreographer
            struct timespec now;
            clock_gettime(CLOCK_MONOTONIC,&now);

            unsigned long long now_ns = (now.tv_sec) * 1000000000ULL + (now.tv_nsec);

            pdev->procs->vsync(pdev->procs, 0, now_ns);
        }
    };

    pthread_mutex_unlock(&pdev->vsync_mutex);

    ALOGD("NVidia VSYNC thread ended");

    return NULL;
}
#endif

static int tegra2_eventControl(struct hwc_composer_device_1 *dev, int dpy,
        int event, int enabled)
{
    struct tegra2_hwc_composer_device_1_t *pdev =
            (struct tegra2_hwc_composer_device_1_t *)dev;

    int ret = (!pdev->org->methods || !pdev->org->methods->eventControl)
            ? -EINVAL
            : pdev->org->methods->eventControl(pdev->org,event,enabled);

    if (ret != 0 && event == HWC_EVENT_VSYNC) {
        // ALOGD("Emulated VSYNC ints are %s", enabled ? "On" : "Off" );

        pdev->enabled_vsync = (enabled) ? true : false;
        ret = 0;
    }
    return ret;
}

static int tegra2_blank(struct hwc_composer_device_1 *dev, int disp, int blank)
{
    struct tegra2_hwc_composer_device_1_t *pdev =
            (struct tegra2_hwc_composer_device_1_t *)dev;

    ALOGD("blank: %d", blank);

    // Store framebuffer status
    pthread_mutex_lock(&pdev->vsync_mutex);
    pdev->fbblanked = blank;
    pthread_cond_signal(&pdev->vsync_cond);
    pthread_mutex_unlock(&pdev->vsync_mutex);

    /* Blanking is handled by other means, no need to blank screen here */
    return 0;
}

static int tegra2_query(struct hwc_composer_device_1* dev, int what, int *value)
{
    struct tegra2_hwc_composer_device_1_t *pdev =
            (struct tegra2_hwc_composer_device_1_t *)dev;

    int ret =  (!pdev->org->query)
        ? -EINVAL
        : pdev->org->query(pdev->org,what,value);

    if (ret == 0)
        return ret;

    // If not handled, emulate it if possible
    switch (what) {
    case HWC_BACKGROUND_LAYER_SUPPORTED:
        // we support the background layer
        value[0] = 0;
        break;

    case HWC_VSYNC_PERIOD:
        // vsync period in nanosecond
        value[0] = pdev->vsync_period;
        break;

    default:
        // unsupported query
        return ret;
    }
    return 0;
}

static void tegra2_registerProcs(struct hwc_composer_device_1* dev,
        hwc_procs_t const* procs)
{
    struct tegra2_hwc_composer_device_1_t* pdev =
            (struct tegra2_hwc_composer_device_1_t*)dev;
    pdev->procs = procs;
    if (pdev->org->registerProcs)
        pdev->org->registerProcs(pdev->org,procs);
}

static void tegra2_dump(hwc_composer_device_1* dev, char *buff, int buff_len)
{
    if (buff_len <= 0)
        return;

    struct tegra2_hwc_composer_device_1_t *pdev =
            (struct tegra2_hwc_composer_device_1_t *)dev;

    if (pdev->org->dump)
        pdev->org->dump(pdev->org,buff,buff_len);
    else
        *buff = 0;
}

static int tegra2_close(hw_device_t *device)
{
    struct tegra2_hwc_composer_device_1_t *pdev =
            (struct tegra2_hwc_composer_device_1_t *)device;

    // Stop VSYNC thread, if running
    if (pdev->vsync_running) {
        void * dummy;

        pthread_mutex_lock(&pdev->vsync_mutex);
        pdev->vsync_running = false;
        pthread_cond_signal(&pdev->vsync_cond);
        pthread_mutex_unlock(&pdev->vsync_mutex);

        pthread_join(pdev->vsync_thread, &dummy);
    }

    pthread_mutex_destroy(&pdev->vsync_mutex);
    pthread_cond_destroy(&pdev->vsync_cond);

    // Close NVidia host handle, if being used...
    if (pdev->nvhost_fd >= 0) {
        nvhost_close(pdev->nvhost_fd);
        pdev->nvhost_fd = -1;
    }

    // Close framebuffer handle, if being used...
    if (pdev->fb_fd >= 0) {
        close(pdev->fb_fd);
        pdev->fb_fd = -1;
    }

    int ret = pdev->org->common.close( (hw_device_t *) pdev->org );

    if (pdev->prepare_xlatebuf)
        free(pdev->prepare_xlatebuf);
    if (pdev->set_xlatebuf)
        free(pdev->set_xlatebuf);

    free(pdev);
    return ret;
}


static int tegra2_open(const struct hw_module_t *module, const char *name,
        struct hw_device_t **device)
{
    int ret;
    hwc_module_t* hwc = get_hwc();
    if (!hwc)
        return -ENOSYS;

    struct tegra2_hwc_composer_device_1_t *dev
        = (struct tegra2_hwc_composer_device_1_t *)calloc(1,sizeof(*dev));

    ret = hwc->common.methods->open(&hwc->common,name,(struct hw_device_t **)&(dev->org));
    if (ret < 0) {
        ALOGE("original open call failed");
        free(dev);
        return ret;
    }

    // Fill new struct
    dev->base.common.tag = HARDWARE_DEVICE_TAG;
    dev->base.common.version = HWC_DEVICE_API_VERSION_1_0;
    dev->base.common.module = const_cast<hw_module_t *>(module);
    dev->base.common.close = tegra2_close;

    dev->base.prepare = tegra2_prepare;
    dev->base.set = tegra2_set;
    dev->base.eventControl = tegra2_eventControl;
    dev->base.blank = tegra2_blank;
    dev->base.query = tegra2_query;
    dev->base.registerProcs = tegra2_registerProcs;
    dev->base.dump = tegra2_dump;

    dev->fb_fd = -1;
    struct fb_var_screeninfo info;

    // Open framebuffer
    dev->fb_fd = open("/dev/graphics/fb0", O_RDWR);

    // Get framebuffer info
    if (dev->fb_fd >= 0 && ioctl(dev->fb_fd, FBIOGET_VSCREENINFO, &info) != -1) {

        uint64_t refreshRate = 1000000000000LLU /
            (
             uint64_t( info.upper_margin + info.lower_margin + info.vsync_len + info.yres )
             * ( info.left_margin  + info.right_margin + info.hsync_len + info.xres )
             * info.pixclock
            );

        if (refreshRate == 0) {
            ALOGW("invalid refresh rate, assuming 60 Hz");
            refreshRate = 60;
        }

        dev->xres = info.xres;
        dev->yres = info.yres;
        dev->xdpi = 1000 * (info.xres * 25.4f) / info.width;
        dev->ydpi = 1000 * (info.yres * 25.4f) / info.height;
        dev->vsync_period  = 1000000000 / refreshRate;

        ALOGV("using\n"
            "xres         = %d px\n"
            "yres         = %d px\n"
            "width        = %d mm (%f dpi)\n"
            "height       = %d mm (%f dpi)\n"
            "refresh rate = %d Hz\n",
            dev->xres, dev->yres, info.width, dev->xdpi / 1000.0,
            info.height, dev->ydpi / 1000.0, refreshRate);

    }

    // Try to query the original hw composer for the time between frames...
    uint64_t value = 0;
#if 0
    if (dev->org->query && dev->org->query(dev->org,HWC_VSYNC_PERIOD,&value) == 0 && value != 0) {
        ALOGD("Got time between frames from original hwcomposer: time in ns = %d",value);

    } else {
#endif
        // Try to get the time from the framebuffer device...
        if (dev->fb_fd >= 0) {
            value = (
                 uint64_t( info.upper_margin + info.lower_margin + info.vsync_len + info.yres )
                 * ( info.left_margin  + info.right_margin + info.hsync_len + info.xres )
                 * info.pixclock
                ) / 1000ULL;
            ALOGD("Got time between frames from framebuffer: time in ns = %llu",value);
        }
#if 0
    }
#endif

    if (!value) {
        ALOGD("Unable to get time between frames."
            "Using DispSync refresh rate: time in ns = %llu", value);
        value = 16672550400 / 1000ULL;
    }

    dev->time_between_frames_ns = value;
    dev->time_between_frames_us = (unsigned long)(value / 1000ULL);

    pthread_mutex_init(&dev->vsync_mutex, NULL);
    pthread_cond_init(&dev->vsync_cond, NULL);

    // Find out if we can use the NVidia VBLANK0 syncpoint to get VSYNC
    //  interrupts, or we must completely emulate them...
    dev->nvhost_fd = nvhost_open();
#if 0
    if (dev->nvhost_fd >= 0) {
        ALOGD("Using NVidia VBLANK0 syncpoint as VSYNC");

        // Get the syncpoint id for VBLANK0
        dev->vblank_syncpt_id = dc0_get_vblank_syncpt();

        dev->vsync_running = true;
        if (pthread_create(&dev->vsync_thread, NULL, tegra2_hwc_nv_vsync_thread, dev)) {
            ALOGE("Unable to start VSYNC thread");
        }

    } else
#endif
    {

        ALOGD("Emulating VSYNC interrupts using nanosleep and kernel timers");

        dev->vsync_running = true;
        if (pthread_create(&dev->vsync_thread, NULL, tegra2_hwc_emulated_vsync_thread, dev)) {
            ALOGE("Unable to start VSYNC emulation thread");
        }
    }

    *device = &dev->base.common;

    return 0;
}

static struct hw_module_methods_t tegra2_hwc_module_methods = {
    open: tegra2_open,
};

hwc_module_t HAL_MODULE_INFO_SYM = {
    common: {
        tag: HARDWARE_MODULE_TAG,
        module_api_version: HWC_MODULE_API_VERSION_0_1,
        hal_api_version: HARDWARE_HAL_API_VERSION,
        id: HWC_HARDWARE_MODULE_ID,
        name: "NVIDIA Tegra2 HWC v0 Module Wrapper v0.2",
        author: "ejtagle@tutopia.com",
        methods: &tegra2_hwc_module_methods,
    }
};


