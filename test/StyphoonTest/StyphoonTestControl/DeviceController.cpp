#include "stdafx.h"
#include "DeviceController.h"
#include "NTV2Player.h"
#include "ntv2utils.h"
#include "TyphoonCapture.h"
#include "gen2ajaTypeMaps.h"

using namespace streampunk;

DeviceController::DeviceController(UpdateCallback statusCallback, void* context)
:   statusCallback(statusCallback),
    statusCallbackContext(context),
    status({false, false, false, 0, 0, 0, 0, 0, 0})
{
}


DeviceController::~DeviceController()
{
    StopPlayback();
    StopCapture();
}


bool DeviceController::StartCapture()
{
    bool success(false);

    std::lock_guard<std::mutex> lock(protectState);

    if(!capture)
    {
        TRACE("Initializing Capture...");

//        TyphoonCapture::ChannelConfig config(TPH_FORMAT_1080i_5000, TPH_V210, TPH_SOURCE_SDI);
        TyphoonCapture::ChannelConfig config(0x00000004, TPH_V210, TPH_SOURCE_SDI);

        capture.reset(TyphoonCapture::Create(0, 2, config, DeviceController::FrameArrivedCallback, this));

        if(capture)
        {
            TRACE("Starting Capture Running...");

            auto result = capture->Start();

            if(result)
            {
                TRACE("Capture Started!");
                success = true;
            }
            else
            {
                TRACE("Failed to start Capture running - error code: %d", result);
                capture.reset();
            }
        }
        else
        {
            TRACE("Failed to initialize Capture");
        }
    }

    status.capturing = success;

    return success;
}


bool DeviceController::StopCapture()
{
    unique_ptr<TyphoonCapture> temp;

    // Release the Capture object outside of the lock
    {
        std::lock_guard<std::mutex> lock(protectState);

        temp.reset(capture.release());

        status.capturing = false;
    }

    return true;
}


bool DeviceController::IsCapturing()
{
    bool capturing(false);

    std::lock_guard<std::mutex> lock(protectState);

    capturing = (bool)capture;

    return capturing;
}


bool DeviceController::StartPlayback()
{
    bool success(false);

    std::lock_guard<std::mutex> lock(protectState);

    if(!player)
    {
        TRACE("Initializing Playback...");

        uint32_t        channelNumber(3);                    //    Number of the channel to use
        int                noAudio(0);                    //    Disable audio tone?
        const NTV2Channel channel(::GetNTV2ChannelForIndex(channelNumber - 1));
        const NTV2OutputDestination    outputDest(::NTV2ChannelToOutputDestination(channel));

//        player.reset(new NTV2Player(&DEFAULT_INIT_PARAMS, "0", true, channel, NTV2_FBF_10BIT_YCBCR, outputDest, NTV2_FORMAT_1080i_5994,false, false, false));
        player.reset(new NTV2Player(&DEFAULT_INIT_PARAMS, "0", true, channel, NTV2_FBF_8BIT_YCBCR, outputDest, NTV2_FORMAT_1080i_5994,false, false, false));
        player->SetScheduledFrameCallback(this, DeviceController::FrameRequiredCallback);

        auto result = player->Init();

        if(AJA_SUCCESS(result))
        {
            TRACE("Starting Playback Running...");

            result = player->Run();

            if(AJA_SUCCESS(result))
            {
                TRACE("Playback Started!");
                success = true;
            }
            else
            {
                TRACE("Failed to start Playback running - error code: %d", result);
                player.reset();
            }
        }
        else
        {
            TRACE("Failed to initialize Playback - error code: %d", result);
            player.reset();
        }
    }

    status.playing = success;

    return success;
}


bool DeviceController::StopPlayback()
{
    unique_ptr<NTV2Player> temp;

    // Release the Capture object outside of the lock
    {
        std::lock_guard<std::mutex> lock(protectState);

        temp.reset(player.release());

        status.playing = false;
    }

    return true;
}


bool DeviceController::IsPlaying()
{
    bool playing(false);

    std::lock_guard<std::mutex> lock(protectState);

    playing = (bool)player;

    return playing;
}


bool DeviceController::CaptureToPlaybackRouting(bool enabled)
{
    std::lock_guard<std::mutex> lock(protectState);

    status.routing = enabled;

    return true;
}


void DeviceController::GetStatus(Status& status)
{
    std::lock_guard<std::mutex> lock(protectState);

    status = this->status;
}


void DeviceController::FrameArrivedCallback(void* pInstance)
{
    reinterpret_cast<DeviceController*>(pInstance)->FrameArrivedCallback();
}


void DeviceController::FrameArrivedCallback()
{
    {
        std::lock_guard<std::mutex> lock(protectState);

        if(capture)
        {
            auto frame = capture->LockNextFrame(0);

            status.captureFrames++;
            status.captureFrameSize = static_cast<uint32_t>(frame->videoBufferSize);

            if(player && status.routing)
            {
                player->ScheduleFrame((char*)frame->videoBuffer, frame->videoBufferSize, (char*)frame->audioBuffer, frame->audioBufferSize, &status.playbackFramesAvailable);

                status.playbackFrames++;
                status.playbackFrameSize = static_cast<uint32_t>(frame->videoBufferSize);
            }

            capture->UnlockFrame();
        }
    }

    if(statusCallback)
    {
        statusCallback(statusCallbackContext);
    }
}


void DeviceController::FrameRequiredCallback(void* pInstance)
{
    reinterpret_cast<DeviceController*>(pInstance)->FrameRequiredCallback();
}


void DeviceController::FrameRequiredCallback()
{
    std::lock_guard<std::mutex> lock(protectState);

}
