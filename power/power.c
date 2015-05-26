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
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define LOG_TAG "Wingray PowerHAL"
#include <utils/Log.h>

#include <hardware/hardware.h>
#include <hardware/power.h>

#define SCALINGMAXFREQ_PATH "/sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq"
#define BOOST_PATH      "/sys/devices/system/cpu/cpufreq/interactive/boost"
static int boost_fd = -1;
static int boost_warned;

#define MAX_BUF_SZ	10

/* initialize to something safe */
static char screen_off_max_freq[MAX_BUF_SZ] = "456000";
static char scaling_max_freq[MAX_BUF_SZ] = "1000000";

int sysfs_read(const char *path, char *buf, size_t size)
{
	int fd, len;

	fd = open(path, O_RDONLY);
	if (fd < 0)
		return -1;

	do {
		len = read(fd, buf, size);
	} while (len < 0 && errno == EINTR);

	close(fd);

	return len;
}

static void sysfs_write(char *path, char *s)
{
    char buf[80];
    int len;
    int fd = open(path, O_WRONLY);

    if (fd < 0) {
        strerror_r(errno, buf, sizeof(buf));
        ALOGE("Error opening %s: %s\n", path, buf);
        return;
    }

    len = write(fd, s, strlen(s));
    if (len < 0) {
        strerror_r(errno, buf, sizeof(buf));
        ALOGE("Error writing to %s: %s\n", path, buf);
    }

    close(fd);
}

static void stingray_power_init(struct power_module *module)
{
    /*
     * cpufreq interactive governor: timer 20ms, min sample 30ms.
     */

    sysfs_write("/sys/devices/system/cpu/cpufreq/interactive/timer_rate",
                "30000");
    sysfs_write("/sys/devices/system/cpu/cpufreq/interactive/min_sample_time",
                "40000");
    sysfs_write("/sys/devices/system/cpu/cpufreq/interactive/go_hispeed_load",
                "80");
    sysfs_write("/sys/devices/system/cpu/cpufreq/interactive/boost",
		"0");
    //sysfs_write("/sys/devices/system/cpu/cpufreq/interactive/input_boost",
		//"1");
}

static void stingray_power_set_interactive(struct power_module *module, int on)
{
    int len;
    char buf[MAX_BUF_SZ];

    /*
     * Lower maximum frequency when screen is off.  CPU 0 and 1 share a
     * cpufreq policy.
     */

    /* read the current scaling max freq */
    len = sysfs_read(SCALINGMAXFREQ_PATH, buf, sizeof(buf));
    if (!on) {
        /* save the current scaling max freq before updating */
        if (len != -1)
            memcpy(scaling_max_freq, buf, sizeof(buf));

        sysfs_write("/sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq",
                    on ? scaling_max_freq : screen_off_max_freq);
    } else {
        sysfs_write("/sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq",
                    on ? scaling_max_freq : screen_off_max_freq);
    }

    if ((on) && (len = screen_off_max_freq))
        sysfs_write("/sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq", scaling_max_freq);

    sysfs_write("/sys/devices/system/cpu/cpufreq/interactive/boost",
                on ? "1" : "0");

    //sysfs_write("/sys/devices/system/cpu/cpufreq/interactive/boost_factor",
		//on ? "0" : "2");

}

static void stingray_power_hint(struct power_module *module, power_hint_t hint,
                            void *data)
{
    char buf[80];
    int len;

    switch (hint) {
    case POWER_HINT_VSYNC:
        break;

    default:
            break;
    }
}

static struct hw_module_methods_t power_module_methods = {
    .open = NULL,
};

struct power_module HAL_MODULE_INFO_SYM = {
    .common = {
        .tag = HARDWARE_MODULE_TAG,
        .module_api_version = POWER_MODULE_API_VERSION_0_2,
        .hal_api_version = HARDWARE_HAL_API_VERSION,
        .id = POWER_HARDWARE_MODULE_ID,
        .name = "Stingray Power HAL",
        .author = "The Android Open Source Project",
        .methods = &power_module_methods,
    },

    .init = stingray_power_init,
    .setInteractive = stingray_power_set_interactive,
    .powerHint = stingray_power_hint,
};
