/* Copyright 2017 Streampunk Media Ltd.

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
*/

#include "stdafx.h"
#include "NTV2Player.h"
#include "ntv2utils.h"
#include "TyphoonCapture.h"
#include "gen2ajaTypeMaps.h"
#include "DeviceController.h"
#include <ctime>
#include <chrono>
#include <fstream>

using namespace streampunk;

#define DEFAULT_COMPRESSED_VIDEO_PATH "C:\\Users\\zztop\\Videos\\Captures"

//#define STORE_AUDIO_CHUNKS

DeviceController::DeviceController(UpdateCallback statusCallback, void* context)
:   statusCallback(statusCallback),
    statusCallbackContext(context),
    status({false, false, false, 0, 0, 0, 0, 0, 0}),
    compressedVideo(false),
    audioFileCounter(0)
{
    audioPath = DeviceController::GenerateFilename(string("Audio"), string(".raw"));
    compressedVideoPath = DeviceController::GenerateFilename(string("Video"), string(".mp4"));
}


DeviceController::~DeviceController()
{
    StopPlayback();
    StopCapture();
}


bool DeviceController::StartCapture(ULONG frameFormat, bool compressedVideo)
{
    bool success(false);

    std::lock_guard<std::mutex> lock(protectState);

    this->compressedVideo = compressedVideo;

    if(!capture)
    {
        TRACE("Initializing Capture...");

//        TyphoonCapture::ChannelConfig config(TPH_FORMAT_1080i_5000, TPH_V210, TPH_SOURCE_SDI);
        TyphoonCapture::ChannelConfig config(0x00000004, frameFormat, TPH_SOURCE_SDI, compressedVideo);

        capture.reset(TyphoonCapture::Create(0, 0, config, DeviceController::FrameArrivedCallback, this));

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


bool DeviceController::StartPlayback(NTV2FrameBufferFormat frameFormat)
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

//        player.reset(new NTV2Player(&DEFAULT_INIT_PARAMS, "0", true, channel, NTV2_FBF_10BIT_YCBCRA, outputDest, NTV2_FORMAT_1080i_5994,false, false, false));
        player.reset(new NTV2Player(&DEFAULT_INIT_PARAMS, "0", true, channel, frameFormat, outputDest, NTV2_FORMAT_1080i_5994,false, false, false));
//        player.reset(new NTV2Player(&DEFAULT_INIT_PARAMS, "0", true, channel, NTV2_FBF_8BIT_YCBCR, outputDest, NTV2_FORMAT_1080i_5994,false, false, false));
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


string DeviceController::GenerateFilename(std::string& type, std::string& suffix)
{
    using namespace std::chrono;

    time_t tt;
    string timestamp("NO_TIMESTAMP");

    tt = system_clock::to_time_t ( system_clock::now() );

    char timedisplay[100];
    struct tm buf;
    errno_t err = localtime_s(&buf, &tt);

    if (std::strftime(timedisplay, sizeof(timedisplay), "%Y-%m-%d-%H-%M-%S", &buf)) 
    {
        timestamp = timedisplay;
    }

    PWSTR szPath = nullptr;
    string fullPath;

    if(SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Videos, 
                                      KF_FLAG_DEFAULT, 
                                      NULL, 
                                      &szPath))) 
    {
        wstring ws(szPath);
        string str(ws.begin(), ws.end());

        fullPath = str + "\\" + type + "_" + timestamp + suffix;
        CoTaskMemFree(szPath);
    }

    return fullPath;
}

#include "AudioTransform.h"

static typhoon::AudioTransform audioTransform;

void DeviceController::WriteToStreamFile(const char* buffer, uint32_t bufferSize, bool isAudio)
{
    const char* filename(nullptr);

    if(isAudio)
    {
        //const unsigned char* outputBuffer(nullptr);
        //uint32_t outputBufferSize(0);
        //
        //tie(outputBuffer, outputBufferSize) = audioTransform.Transform((const unsigned char*)buffer, bufferSize);
        //
        //buffer = (char*)outputBuffer;
        //bufferSize = outputBufferSize;

#ifdef STORE_AUDIO_CHUNKS
        char filenameBuffer[256];
        sprintf_s(filenameBuffer, 256, "%s_%d.raw", audioPath.c_str(), ++audioFileCounter);

        auto chunkFile = std::fstream(filenameBuffer, std::ios::out | std::ios::binary);
        chunkFile.write(buffer, bufferSize);
        chunkFile.close();
#endif

        filename = audioPath.c_str();
    }
    else
    {
        filename = compressedVideoPath.c_str();
    }

    auto myfile = std::fstream(filename, std::ios::out | std::ios::app | std::ios::binary);
    myfile.write(buffer, bufferSize);
    myfile.close();
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

            WriteToStreamFile((char*)frame->audioBuffer, frame->audioBufferSize, true);

            if(compressedVideo)
            {
                WriteToStreamFile((char*)frame->dataBuffer, frame->dataBufferSize, false);
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
//    std::lock_guard<std::mutex> lock(protectState);

}
