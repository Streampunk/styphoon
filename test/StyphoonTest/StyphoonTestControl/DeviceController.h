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

#pragma once

#include <thread>
#include <mutex>
#include <memory>
#include "TyphoonCapture.h"
#include "ntv2enums.h"

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

    bool StartCapture(ULONG frameFormat, bool compressedVideo);
    bool StopCapture();
    bool IsCapturing();

    bool StartPlayback(NTV2FrameBufferFormat frameFormat);
    bool StopPlayback();
    bool IsPlaying();

    bool CaptureToPlaybackRouting(bool enabled);

    void GetStatus(Status& status);
    
    std::string GetCompressedVideoPath() { return compressedVideoPath; }
    void SetCompressedVideoPath(std::string& path) { compressedVideoPath = path; }

    std::string GetAudioPath() { return audioPath; }
    void SetAudioPath(std::string& path) { audioPath = path; }

private:

    static std::string GenerateFilename(std::string& type, std::string& suffix);
    void WriteToStreamFile(const char* buffer, uint32_t bufferSize, bool isAudio);

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
    bool compressedVideo;

    std::string compressedVideoPath;
    std::string audioPath;

    uint32_t audioFileCounter;
};

