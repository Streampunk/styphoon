#pragma once

#include <eh.h>
#include <thread>
#include <memory>
#include "TyphoonSDKWrapper.h"
#include "CircularBuffer.h"
#include "TyphoonDevice.h"
#include "AudioTransform.h"

namespace streampunk
{

class TyphoonCapture
{
static const uint32_t CaptureBufferSize = 5;

// Class types
public:

    typedef void (*FrameCallback)(void* context);

    struct ChannelConfig
    {
        ULONG SignalStandard;
        ULONG FrameFormat;
        ULONG Source;
        bool  CompressedVideo;
        ULONG IpAddr;
        ULONG VPort; 
        ULONG APort;

        ChannelConfig(
            ULONG signalStandard,
            ULONG frameFormat     = TPH_V210,
            ULONG source          = TPH_SOURCE_SDI,
            bool  compressedVideo = false,
            ULONG ipAddr          = 0,
            ULONG vPort           = 0,
            ULONG aPort           = 0
        ):  SignalStandard(signalStandard),
            FrameFormat(frameFormat),
            Source(source),
            CompressedVideo(compressedVideo),
            IpAddr(ipAddr),
            VPort(vPort),
            APort(aPort) {}
    };

    struct AVBuffer
    {
        unsigned char* videoBuffer  = nullptr;
        size_t videoBufferSize = 0;
        unsigned char* audioBuffer  = nullptr;
        size_t audioBufferSize = 0;
        unsigned char* dataBuffer  = nullptr;
        size_t dataBufferSize = 0;

        void Reset()
        {
            videoBuffer  = nullptr;
            videoBufferSize = 0;
            audioBuffer  = nullptr;
            audioBufferSize = 0;
            dataBuffer  = nullptr;
            dataBufferSize = 0;
        }
    };

public:

    static TyphoonCapture* Create(UINT boardId, ULONG channelId, ChannelConfig& config, FrameCallback frameCallback, void* callbackContext);

    virtual ~TyphoonCapture();

    bool Start();
    bool Stop();

    // Get the _actual_ config of the device/channel. This can be different to what was requested,
    // as it is dependent upon the signal being received
    const ChannelConfig& GetConfig() { return config_; }

    AVBuffer* LockNextFrame(uint32_t timeoutMs);
    void UnlockFrame();

private:

    TyphoonCapture(UINT boardId, ULONG channelId, ChannelConfig& config, FrameCallback frameCallback, void* callbackContext);

    bool Initialize();
    void Cleanup();

    void CaptureThreadProc();
    void RunFrameAcquisitionLoop();
    bool ForwardNextFrame();

    void LogBufferState(uint32_t freeBuffers);

    FrameCallback frameCallback_;
    void* frameCallbackContext_;

    TyphoonDevice::Ref board_;
    std::unique_ptr<std::thread> thread_;
    TyphoonChannel* channel_;

    UINT boardId_;
    UINT channelId_;
    HANDLE hShutdownEvent_;
    HANDLE hFrameEvent_;
    ChannelConfig config_;
    AVBuffer currentFrame_;

    CircularBuffer<CaptureBufferSize> captureBuffer_;
    AudioTransform audioTransform_;
};

};