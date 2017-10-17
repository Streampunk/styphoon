// CompressData.cpp : Defines the entry point for the console application.
//

#include "targetver.h"

#include <tchar.h>

#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Setupapi.lib")

#include"TyphoonCapture.h"
#include"TyphoonRegister.h"
#include"TyphoonTypeMaps.h"
#include"BufferStatus.h"
#include <assert.h>
using namespace std;

#define WIN32_LEAN_AND_MEAN

using namespace std;
using namespace streampunk;

TyphoonCapture* TyphoonCapture::Create(UINT boardId, ULONG channelId, ChannelConfig& config, FrameCallback frameCallback, void* callbackContext)
{
    unique_ptr<TyphoonCapture> temp(new TyphoonCapture(boardId, channelId, config, frameCallback, callbackContext));

    // Try to initialize the capture channel. If this fails, cleanup the object
    if(temp->Initialize() == false)
    {
        temp.reset();
    }

    return temp.release();
}

TyphoonCapture::~TyphoonCapture()
{
    Stop();
    Cleanup();
}


bool TyphoonCapture::Start()
{
    bool started(false);

    if(!thread_)
    {
        thread_.reset(new std::thread(&TyphoonCapture::CaptureThreadProc,std::ref(*this)));
        started = true;
    }

    return started;
}


bool TyphoonCapture::Stop()
{
    bool stopped(false);

    if(thread_)
    {
        assert(hShutdownEvent_);

        if (!::SetEvent(hShutdownEvent_))
        {
            // Something very bad has happened, in which case, don't block on the thread exiting
            printf("SetEvent failed (%d)\n", GetLastError());
        }
        else
        {
            thread_->join();
        }

        thread_.reset();

        stopped = true;
    }

    return stopped;
}


TyphoonCapture::TyphoonCapture(UINT boardId, ULONG channelId, ChannelConfig& config, FrameCallback frameCallback, void* callbackContext)
:   boardId_(boardId),
    channelId_(channelId), 
    frameCallback_(frameCallback),
    frameCallbackContext_(callbackContext),
    channel_(nullptr),
    hShutdownEvent_(nullptr), 
    hFrameEvent_(nullptr), 
    config_(config)
{
    assert(frameCallback_ != nullptr);
}


TyphoonCapture::AVBuffer* TyphoonCapture::LockNextFrame(uint32_t timeoutMs)
{
    size_t&         bufferSize = currentFrame_.videoBufferSize;
    unsigned char*& buffer     = currentFrame_.videoBuffer;

    if(config_.CompressedVideo)
    {
        bufferSize = currentFrame_.dataBufferSize;
        buffer     = currentFrame_.dataBuffer;
    }

    bool locked = captureBuffer_.LockBufferForRead(buffer, bufferSize, currentFrame_.audioBuffer, currentFrame_.audioBufferSize, timeoutMs);

    if(locked)
    {
        return &currentFrame_;
    }
    else
    {
        return nullptr;
    }
}


void TyphoonCapture::UnlockFrame()
{
    captureBuffer_.ReleaseBufferFromRead();
}


bool TyphoonCapture::Initialize()
{
    bool success(false);

    assert(!board_);
    assert(hShutdownEvent_ == nullptr);
    assert(hFrameEvent_ == nullptr);

    if (frameCallback_ == nullptr)
    {
        printf("No frame callback specified!\n");
        return false;
    }

    bool result = board_.Initialize(boardId_);

    // We shouldn't need to do this, but the board seems very timing sensitive
    Sleep(100);

    if (result != true)
    {
        printf("Error opening Typhoon board: (%d)\n", result);
    }
    else
    {
        hShutdownEvent_ = CreateEventW(NULL, TRUE, FALSE, NULL);
        hFrameEvent_ = CreateSemaphoreW(NULL, 0, MAXLONG, NULL);

        if (hShutdownEvent_ == NULL || hFrameEvent_ == NULL)
        {
            printf("Error creating Typhoon synchronization objects: %d\n", GetLastError());
        }
        else
        {
            channel_ = board_->GetChannel(channelId_);

            ULONG signalStandard = TyphoonRegister::Read(*board_, TyphoonRegister::InputSignalStandard, channelId_);
            ULONG design = TyphoonRegister::GetDesign(*board_);

            // Try to determine the incoming signal standard in a form that can be used to open the channel.
            // If this isn't possible, use the default as specified during construction
            ULONG incomingSignalStandard = TPH_DISPLAY_MODE_TRANSLATION_MAP.ToB(signalStandard);

            if(incomingSignalStandard != TPH_FORMAT_UNKNOWN)
            {
                printf("Setting signal standard to (%d)\n", incomingSignalStandard);
                config_.SignalStandard = incomingSignalStandard;
            }
            else
            {
                printf("Incoming signal type (%d) not recognised, defaulting to (%d)\n", incomingSignalStandard, config_.SignalStandard);
            }

            if(design != TPH_DESIGN_4ENCODER_NO_FEC)
            {
                printf("Error - Typhoon board is not configured in 4-encoder mode. It is in mode (%d)\n", design);
            }
            else
            {
                success = true;
            }
        }
    }

    if(success == false)
    {
        Cleanup();
    }

    return success;
}


void TyphoonCapture::Cleanup()
{
    channel_ = nullptr;

    if(hShutdownEvent_ != nullptr)
    {
        CloseHandle(hShutdownEvent_);
        hShutdownEvent_ = nullptr;
    }

    if(hFrameEvent_ != nullptr)
    {
        CloseHandle(hFrameEvent_);
        hFrameEvent_ = nullptr;
    }

    if(board_)
    {
        board_.Release();
    }
}


void TyphoonCapture::CaptureThreadProc()
{
    
    int flag = 0;
    
    printf("Capture thread started for Typhoon channel %d\n", channelId_);
    
    ULONG dmaFlags;

    if(config_.CompressedVideo)
    {
        printf("**** Setting compressed video\n");
        dmaFlags = TPH_FLAG_AUDIO | TPH_FLAG_DATA | TPH_FLAG_VIDEO;
//        dmaFlags = TPH_FLAG_DATA;
    }
    else
    {
        dmaFlags = TPH_FLAG_AUDIO | TPH_FLAG_VIDEO;
    }

    printf("Typhoon: Opening Channel %d; SignalStandard = %d; DMAFlags = %d; FrameFormat = %d; Source = %d\n",
           channelId_,
           config_.SignalStandard, 
           dmaFlags, 
           config_.FrameFormat,
           config_.Source);

    //ULONG frameFormat(TPH_DSG4_REGWRITE_V210);
    //
    //if(config_.FrameFormat == TPH_UYVY)
    //{
    //    frameFormat = TPH_DSG4_REGWRITE_UYVY;
    //}
    //
    //bool setFrameFormat = TyphoonRegister::Write(*board_, TyphoonRegister::FrameFormat, channelId_, frameFormat);
    
    bool openedChannel = static_cast<bool>(channel_->Open(
        config_.SignalStandard, 
        dmaFlags, 
        TPH_V210, // Setting the frame format here merely sets the max expected incoming pixel bit-size, so always use V210 (i.e. 10bit)
        config_.Source,
        TPH_MEMORY_INT));

    // We shouldn't need to do this, but the board seems very timing sensitive
    Sleep(100);

    bool setFrameFormat = false;

    if(openedChannel == false)
    {
        printf("Unable to open Typhoon channel\n");
    }
    else
    {
        ULONG frameFormat(TPH_DSG4_REGWRITE_V210);
        //ULONG frameFormat(TPH_DSG4_REGWRITE_UYVY);

        if(config_.FrameFormat == TPH_UYVY)
        {
            frameFormat = TPH_DSG4_REGWRITE_UYVY;
        }

        setFrameFormat = TyphoonRegister::Write(*board_, TyphoonRegister::FrameFormat, channelId_, frameFormat);
    }

    bool setSemaphore(false);

    if(setFrameFormat == false)
    {
        printf("Unable to set frame format channel\n");
    }
    else
    {
        Sleep(5);
        setSemaphore = static_cast<bool>(channel_->SetSemaphore(hFrameEvent_));
    }

    bool ethernetInitialized(false);

    if(setSemaphore == false)
    {
        printf("Unable to set FrameEvent semaphore for Typhoon channel\n");
    }
    else
    {
        if(config_.Source == TPH_SOURCE_ETHERNET)
        {
            Sleep(5);
            ethernetInitialized = static_cast<bool>(channel_->SetEthernetParams(config_.IpAddr, config_.VPort, config_.APort));
        }
        else
        {
            ethernetInitialized = true;
        }
    }

    bool started(false);

    if(ethernetInitialized == false)
    {
        printf("Unable to initialize ethernet settings\n");
    }
    else
    {
        Sleep(10);
        started = static_cast<bool>(channel_->Start());
    }

    // Run the capture loop until the shutdown event is set
    if(started == false)
    {
        printf("Unable to start Typhoon channel running\n");
    }
    else
    {
        RunFrameAcquisitionLoop();
    }

    // Cleanup thread
    if(started == true)
    {
        Sleep(5);
        channel_->Stop();
    }

    if(openedChannel == true)
    {
        Sleep(10);
        channel_->Close();
    }

    if(setSemaphore == true)
    {
        Sleep(10);
        channel_->SetSemaphore(NULL);
    }
}


void TyphoonCapture::RunFrameAcquisitionLoop()
{
    HANDLE waitHandles[] = { hFrameEvent_, hShutdownEvent_ };
    bool continueCapture(true);

    while(continueCapture)
    {
        auto waitResult = ::WaitForMultipleObjects(2, waitHandles, FALSE, INFINITE);

        if(waitResult == WAIT_OBJECT_0)
        {
            if(ForwardNextFrame() == false)
            {
                printf("Error getting frame buffer from Typhood card\n");
            }
        }
        else
        {
            // If this isn't just the shutdown being signalled, something has gone wrong
            if(waitResult != (WAIT_OBJECT_0 + 1))
            {
                printf("Unexpected result waiting for frame data, exiting (%n)\n", waitResult);
            }

            continueCapture = false;
        }
    }
}


bool TyphoonCapture::ForwardNextFrame()
{
    TPH_FRAME_ITEM frameItem;

    bool result = static_cast<bool>(channel_->GetFrame(&frameItem));

    if (result)
    {
        unsigned char* videoFrameBuffer = nullptr;
        unsigned char* audioFrameBuffer = nullptr;

        uint32_t freeBuffers = 0;

        ULONG videoBufferSize = frameItem.VideoBufferSize;
        PVOID64 videoBuffer = frameItem.pBufferVideo;

        if(config_.CompressedVideo)
        {
            videoBufferSize = frameItem.DataBufferSize;
            videoBuffer = frameItem.pBufferData;
        }
        else if(config_.FrameFormat != TPH_UYVY)
        {
            // If we are receiving 10bit video frame data then we need to do some byte manipulation
            // to get the frame into correct v210 format
            char tempBuf[4];

            for (int j = 0; j < frameItem.VideoBufferSize/4; ++j) {
                            
                /**
                    Word swapped and saved to temp buffer
                */
                tempBuf[0] = *((char *)frameItem.pBufferVideo + (j * 4 + 3));
                tempBuf[1] = *((char *)frameItem.pBufferVideo + (j * 4 + 2));
                tempBuf[2] = *((char *)frameItem.pBufferVideo + (j * 4 + 1));
                tempBuf[3] = *((char *)frameItem.pBufferVideo + (j * 4 + 0));

                /**
                    Temporary buffer copied back 
                */
                *((char *)frameItem.pBufferVideo + (j * 4 + 0)) = tempBuf[0];
                *((char *)frameItem.pBufferVideo + (j * 4 + 1)) = tempBuf[1];
                *((char *)frameItem.pBufferVideo + (j * 4 + 2)) = tempBuf[2];
                *((char *)frameItem.pBufferVideo + (j * 4 + 3)) = tempBuf[3];
            }
        }

        const unsigned char* audioTransformBuffer(nullptr);
        uint32_t audioTransformBufferSize(0);

        tie(audioTransformBuffer, audioTransformBufferSize) = 
            audioTransform_.Transform((const unsigned char*)frameItem.pBufferAudio, frameItem.AudioBufferSize);

        result = captureBuffer_.LockBufferForWrite(videoFrameBuffer, videoBufferSize, audioFrameBuffer, audioTransformBufferSize, 10);

        if(result)
        {
            memcpy_s(videoFrameBuffer, videoBufferSize, videoBuffer, videoBufferSize);
            memcpy_s(audioFrameBuffer, audioTransformBufferSize, audioTransformBuffer, audioTransformBufferSize);

            captureBuffer_.ReleaseBufferFromWrite(&freeBuffers);
        }
        else
        {
            printf("Warning dropping frame from Typhoon as no buffer space\n");
        }

        channel_->ReleaseFrame(&frameItem);

        frameCallback_(frameCallbackContext_);

        if(result)
        {
            LogBufferState(freeBuffers);
        }
    }

    return result;
}


void TyphoonCapture::LogBufferState(uint32_t freeBuffers)
{
    float usedCircBufferPercent = (float)(freeBuffers * 100) / (float)CaptureBufferSize;

    BufferStatus::AddSample(BufferStatus::CaptureCircBuffer, usedCircBufferPercent);
}


