// CompressData.cpp : Defines the entry point for the console application.
//

#include "targetver.h"

#include <tchar.h>

#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Setupapi.lib")

#include"TyphoonCapture.h"
#include"TyphoonRegister.h"
//#include <Windows.h>
//#include <Ws2tcpip.h>
//#include <Winsock2.h>
//#include <process.h>
//#include <initguid.h>
//#include <stdio.h>
//#include <fstream> 
//#include <string>
//#include <iostream>
//#include <conio.h>
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
    bool locked = captureBuffer_.LockBufferForRead(currentFrame.videoBuffer, currentFrame.videoBufferSize, timeoutMs);

    if(locked)
    {
        return &currentFrame;
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
    Sleep(500);

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

            ULONG format = TyphoonRegister::Read(*board_, TyphoonRegister::InputFormat, channelId_);

            success = true;
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
        dmaFlags = TPH_FLAG_AUDIO | TPH_FLAG_DATA;
    }
    else
    {
        dmaFlags = TPH_FLAG_AUDIO | TPH_FLAG_VIDEO;
    }

    //bool openedChannel = channel_->Open(TPH_FORMAT_1080i_5994, TPH_FLAG_VIDEO, TPH_SOURCE_SDI, TPH_V210, TPH_MEMORY_INT);

    bool openedChannel = channel_->Open(
        config_.SignalStandard, 
        dmaFlags, 
        config_.FrameFormat,
        config_.Source,
        TPH_MEMORY_INT);

    // We shouldn't need to do this, but the board seems very timing sensitive
    Sleep(500);

    bool setSemaphore(false);

    if(openedChannel == false)
    {
        printf("Unable to open Typhoon channel\n");
    }
    else
    {
        Sleep(5);
        setSemaphore = channel_->SetSemaphore(hFrameEvent_);
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
            ethernetInitialized = channel_->SetEthernetParams(config_.IpAddr, config_.VPort, config_.APort);
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
        started = channel_->Start();
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

    bool result = channel_->GetFrame(&frameItem);

    if (result)
    {
        unsigned char* frameBuffer = nullptr;

        result = captureBuffer_.LockBufferForWrite(frameBuffer, frameItem.VideoBufferSize, 10);

        if(result)
        {
            memcpy_s(frameBuffer, frameItem.VideoBufferSize, frameItem.pBufferVideo, frameItem.VideoBufferSize);
            captureBuffer_.ReleaseBufferFromWrite();
        }
        else
        {
            printf("Warning dropping frame from Typhoon as no buffer space\n");
        }

        channel_->ReleaseFrame(&frameItem);

        frameCallback_(frameCallbackContext_);
    }

    return result;
}


