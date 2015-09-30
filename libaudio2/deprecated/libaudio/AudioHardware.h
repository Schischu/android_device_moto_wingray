/*
** Copyright 2008-2010, The Android Open-Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

#ifndef ANDROID_AUDIO_HARDWARE_H
#define ANDROID_AUDIO_HARDWARE_H

#include <stdint.h>
#include <sys/types.h>

#include <utils/threads.h>
#include <utils/SortedVector.h>

#include <hardware_legacy/AudioHardwareBase.h>
#include "AudioPostProcessor.h"

namespace android_audio_legacy {
    using android::AutoMutex;
    using android::Mutex;
    using android::SortedVector;

#include <linux/cpcap_audio.h>
#include <linux/tegra_audio.h>

#define AUDIO_HW_OUT_SAMPLERATE 44100
#define AUDIO_HW_NUM_OUT_BUF 2
#define AUDIO_HW_OUT_LATENCY_MS 0

// FIXME: this is a workaround for issue 3387419 with impact on latency
// to be removed when root cause is fixed
#define AUDIO_HW_NUM_OUT_BUF_LONG 4

#define AUDIO_HW_IN_SAMPLERATE 11025                  // Default audio input sample rate
#define AUDIO_HW_IN_CHANNELS (AudioSystem::CHANNEL_IN_MONO) // Default audio input channel mask
#define AUDIO_HW_IN_BUFFERSIZE (4096)               // Default audio input buffer size
#define AUDIO_HW_IN_FORMAT (AudioSystem::PCM_16_BIT)  // Default audio input sample format

enum {
    AUDIO_HW_GAIN_SPKR_GAIN = 0,
    AUDIO_HW_GAIN_MIC_GAIN,
    AUDIO_HW_GAIN_NUM_DIRECTIONS
};
enum {
    AUDIO_HW_GAIN_USECASE_VOICE= 0,
    AUDIO_HW_GAIN_USECASE_MM,
    AUDIO_HW_GAIN_USECASE_VOICE_REC,
    AUDIO_HW_GAIN_NUM_USECASES
};
enum {
    AUDIO_HW_GAIN_EARPIECE = 0,
    AUDIO_HW_GAIN_SPEAKERPHONE,
    AUDIO_HW_GAIN_HEADSET_W_MIC,
    AUDIO_HW_GAIN_MONO_HEADSET,
    AUDIO_HW_GAIN_HEADSET_NO_MIC,
    AUDIO_HW_GAIN_EMU_DEVICE,
    AUDIO_HW_GAIN_RSVD1,
    AUDIO_HW_GAIN_RSVD2,
    AUDIO_HW_GAIN_RSVD3,
    AUDIO_HW_GAIN_RSVD4,
    AUDIO_HW_GAIN_RSVD5,
    AUDIO_HW_GAIN_NUM_PATHS
};

enum input_state {
    AUDIO_STREAM_IDLE,
    AUDIO_STREAM_CONFIG_REQ,
    AUDIO_STREAM_NEW_RATE_REQ,
    AUDIO_STREAM_CONFIGURED
};

class AudioHardware : public  AudioHardwareBase
{
    class AudioStreamOutTegra;
    class AudioStreamInTegra;
    class AudioStreamSrc;

public:
    // AudioHardwareInterface
                        AudioHardware();
    virtual             ~AudioHardware();
    virtual status_t    initCheck();
    virtual status_t    setVoiceVolume(float volume);
    virtual status_t    setMasterVolume(float volume);

    //virtual status_t    setMode(int mode);

    // mic mute
    virtual status_t    setMicMute(bool state);
    virtual status_t    getMicMute(bool* state);

    //virtual status_t    setParameters(const String8& keyValuePairs);
    //virtual String8     getParameters(const String8& keys);

    // create I/O streams
    virtual AudioStreamOut* openOutputStream(
                                uint32_t devices,
                                int *format=0,
                                uint32_t *channels=0,
                                uint32_t *sampleRate=0,
                                status_t *status=0);
    virtual    void        closeOutputStream(AudioStreamOut* out);

    virtual AudioStreamIn* openInputStream(
                                uint32_t devices,
                                int *format,
                                uint32_t *channels,
                                uint32_t *sampleRate,
                                status_t *status,
                                AudioSystem::audio_in_acoustics acoustics);
    virtual    void        closeInputStream(AudioStreamIn* in);

    //----------------------------------------------------------------------
    // added by AudioHardware
    status_t    init();
protected:
    // voice processing IDs for mEcnsRequested and mEcnsEnabled
    enum {
       PREPROC_AEC = 0x1,                       // AEC is enabled
       PREPROC_NS = 0x2,                        // NS is enabled
    };
    void        setEcnsRequested_l(int ecns, bool enabled);
    bool        isEcRequested() { return !!(mEcnsRequested & PREPROC_AEC); }
	
    // AudioHardwareBase provides default implementation
    //virtual  bool        isModeInCall(int mode);
    //virtual  bool        isInCall();

    // AudioHardwareInterface
    virtual    status_t    dump(int fd, const Vector<String16>& args);

    // added by AudioHardware
                int        getActiveInputRate();


private:

    status_t    dumpInternals(int fd, const Vector<String16>& args);
    uint32_t    getInputSampleRate(uint32_t sampleRate);
    status_t    doStandby(int stop_fd, bool output, bool enable);
    status_t    doRouting_l();
    status_t    doRouting();
    status_t    setVolume_l(float v, int usecase);
    uint8_t     getGain(int direction, int usecase);
    void        readHwGainFile();

    AudioStreamInTegra*   getActiveInput_l();
    status_t    setMicMute_l(bool state);


    class AudioStreamOutTegra : public AudioStreamOut {
    public:
                            AudioStreamOutTegra();
        virtual             ~AudioStreamOutTegra();
#if 0
                status_t    set(AudioHardware* mHardware,
                                uint32_t devices,
                                int *pFormat,
                                uint32_t *pChannels,
                                uint32_t *pRate);
#else
    virtual status_t    set(int *pFormat, uint32_t *pChannels, uint32_t *pRate);
#endif
        virtual uint32_t    sampleRate() const { return AUDIO_HW_OUT_SAMPLERATE; }
        // must be 32-bit aligned - driver only seems to like 4800
        virtual size_t      bufferSize() const { return 4096; }
        virtual uint32_t    channels() const { return AudioSystem::CHANNEL_OUT_STEREO; }
        virtual int         format() const { return AudioSystem::PCM_16_BIT; }
        virtual uint32_t    latency() const { return (1000*AUDIO_HW_NUM_OUT_BUF_LONG*(bufferSize()/frameSize()))/sampleRate()+AUDIO_HW_OUT_LATENCY_MS; }
        virtual status_t    setVolume(float left, float right) { return INVALID_OPERATION; }
        virtual ssize_t     write(const void* buffer, size_t bytes);
        virtual status_t    standby();
        virtual status_t    dump(int fd, const Vector<String16>& args);
        virtual status_t    setParameters(const String8& keyValuePairs);
        virtual String8     getParameters(const String8& keys);
        virtual status_t    getRenderPosition(uint32_t *dspFrames);
        
        //----------------------------------------------------------------------
                status_t    init();
protected:
                status_t    initCheck();
                void        setDriver_l(bool speaker, bool bluetooth, bool spdif, int sampleRate);
                void        flush();
                void        flush_l();

                status_t    online_l();
                bool        getStandby();
                uint32_t    devices() { return mDevices; }

                void        lock() { mSleepReq = true; mLock.lock();  mSleepReq = false; }
                void        unlock() { mLock.unlock(); }
                bool        isLocked() { return mLocked; }
                void        setNumBufs(int numBufs);
                void        lockFd() { mFdLock.lock(); }
                void        unlockFd() { mFdLock.unlock(); }

                int         mBtFdIoCtl;

    private:
                AudioHardware* mHardware;
                Mutex       mLock;
                int         mFd;
                int         mFdCtl;
                int         mBtFd;
                int         mBtFdCtl;
                int         mSpdifFd;
                int         mSpdifFdCtl;
                int         mStartCount;
                int         mRetryCount;
                uint32_t    mDevices;
                Mutex       mFdLock;
                bool        mIsSpkrEnabled;
                bool        mIsBtEnabled;
                bool        mIsSpdifEnabled;
                bool        mIsSpkrEnabledReq;
                bool        mIsBtEnabledReq;
                bool        mIsSpdifEnabledReq;
                int16_t     mSpareSample;
                bool        mHaveSpareSample;
                int         mState;
                bool        mLocked;        // setDriver() doesn't have to lock if true
                int         mDriverRate;
                bool        mInit;
                bool        mSleepReq;
    };

    class AudioStreamInTegra : public AudioStreamIn {
    public:
                            AudioStreamInTegra();
        virtual             ~AudioStreamInTegra();
#if 0
                status_t    set(AudioHardware* mHardware,
                                uint32_t devices,
                                int *pFormat,
                                uint32_t *pChannels,
                                uint32_t *pRate,
                                AudioSystem::audio_in_acoustics acoustics);
#else
       virtual status_t    set(int *pFormat, uint32_t *pChannels, uint32_t *pRate, AudioSystem::audio_in_acoustics acoustics);
#endif
        virtual uint32_t    sampleRate() const { return mSampleRate; }
        virtual size_t      bufferSize() const { return mBufferSize; }
        virtual uint32_t    channels() const { return mChannels; }
        virtual int         format() const { return mFormat; }
        virtual status_t    setGain(float gain) { return INVALID_OPERATION; }
        virtual ssize_t     read(void* buffer, ssize_t bytes);
        virtual status_t    dump(int fd, const Vector<String16>& args);
        virtual status_t    standby();
        virtual status_t    setParameters(const String8& keyValuePairs);
        virtual String8     getParameters(const String8& keys);
        virtual unsigned int  getInputFramesLost() const;
        virtual status_t    addAudioEffect(effect_handle_t effect);
        virtual status_t    removeAudioEffect(effect_handle_t effect);

        //----------------------------------------------------------------------
protected:
        virtual status_t    online_l();
                bool        getStandby() const;
                uint32_t    devices() { return mDevices; }
                void        setDriver_l(bool mic, bool bluetooth, int sampleRate);
                int         source() const { return mSource; }
                void        lock() { mSleepReq = true; mLock.lock(); mSleepReq = false; }
                void        unlock() { mLock.unlock(); }
                bool        isLocked() { return mLocked; }
                void        stop_l();
                void        lockFd() { mFdLock.lock(); }
                void        unlockFd() { mFdLock.unlock(); }

    private:
                void        reopenReconfigDriver();
                void        updateEcnsRequested(effect_handle_t effect, bool enabled);

                AudioHardware* mHardware;
                Mutex       mLock;
                int         mFd;
                int         mFdCtl;
                int         mState;
                int         mRetryCount;
                int         mFormat;
                uint32_t    mChannels;
                uint32_t    mSampleRate;
                size_t      mBufferSize;
                AudioSystem::audio_in_acoustics mAcoustics;
                uint32_t    mDevices;
                bool        mIsMicEnabled;
                bool        mIsBtEnabled;
                int         mSource;
                // 20 millisecond scratch buffer
                int16_t     mInScratch[48000/50];
                bool        mLocked;        // setDriver() doesn't have to lock if true
        mutable uint32_t    mTotalBuffersRead;
        mutable nsecs_t     mStartTimeNs;
                int         mDriverRate;
        mutable Mutex       mFramesLock;
                Mutex       mFdLock;
                bool        mSleepReq;
                int         mEcnsRequested;   // bit field indicating if AEC and/or NS are requested
    };

    static const uint32_t inputSamplingRates[];
    bool        mInit;
    bool        mMicMute;
    bool        mBluetoothNrec;
    uint32_t    mBluetoothId;
    AudioStreamOutTegra*  mOutput;
    SortedVector <AudioStreamInTegra*>   mInputs;

    struct cpcap_audio_stream mCurOutDevice;
    struct cpcap_audio_stream mCurInDevice;

    friend class AudioStreamInTegra;
    Mutex       mLock;

    int mCpcapCtlFd;
    int mHwOutRate;
    int mHwInRate;
    float mMasterVol;
    float mVoiceVol;
    uint8_t mCpcapGain[AUDIO_HW_GAIN_NUM_DIRECTIONS]
                      [AUDIO_HW_GAIN_NUM_USECASES]
                      [AUDIO_HW_GAIN_NUM_PATHS];
    int mSpkrVolume;
    int mMicVolume;

    int mEcnsEnabled;   // bit field indicating if AEC and/or NS are enabled
    int mEcnsRequested; // bit field indicating if AEC and/or NS are requested
    bool mBtScoOn;
};

// ----------------------------------------------------------------------------

}; // namespace android

#endif // ANDROID_AUDIO_HARDWARE_H
