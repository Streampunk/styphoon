#pragma once

#include <thread>
#include <mutex>
#include <memory>
#include "TyphoonCapture.h"

class NTV2Player;

class DeviceController
{
public:

    typedef void (*UpdateCallback)(void* context);

    struct Status
    {
        bool capturing;
        bool playing;
        bool routing;
        UINT32 captureFrames;
        UINT32 captureFramesAvailable;
        UINT32 captureFrameSize;
        UINT32 playbackFrames;
        UINT32 playbackFramesAvailable;
        UINT32 playbackFrameSize;
    };

public:
    DeviceController(UpdateCallback statusCallback, void* context);
    virtual ~DeviceController();

    bool StartCapture();
    bool StopCapture();
    bool IsCapturing();

    bool StartPlayback();
    bool StopPlayback();
    bool IsPlaying();

    bool CaptureToPlaybackRouting(bool enabled);

    void GetStatus(Status& status);
    
private:

    static void FrameArrivedCallback(void* pInstance);
    void FrameArrivedCallback();

    static void FrameRequiredCallback(void* pInstance);
    void FrameRequiredCallback();

    std::mutex     protectState;
    UpdateCallback statusCallback;
    void*          statusCallbackContext;
    Status         status;
    std::unique_ptr<streampunk::TyphoonCapture> capture;
    std::unique_ptr<NTV2Player> player;
};

