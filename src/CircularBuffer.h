#pragma once

#include <cstdint>
#include <array>
#include <vector>
#include <mutex>
#include <condition_variable>

namespace streampunk
{

template<size_t _Size>
class CircularBuffer
{
public:

    CircularBuffer()
    :   readLocked_(false),
        writeLocked_(false),
        lastWrittenIdx_(-1),
        nextWriteIdx_(-1),
        readIdx_(-1),
        freeBuffers_(_Size)
    {}

    bool LockBufferForWrite(
        unsigned char*& videoBuffer, 
        size_t requiredVideoBufferSize_, 
        unsigned char*& audioBuffer, 
        size_t requiredAudioBufferSize_, 
        uint32_t waitTimeoutMs)
    {
        bool locked(false);
        std::unique_lock<std::mutex> lock(protectBuffers_);

        if(writeLocked_ == false)
        {
            nextWriteIdx_ = (lastWrittenIdx_ + 1) % _Size;

            // If there is no space left, wait up to specified time for a slot to be read
            if(nextWriteIdx_ == readIdx_ && waitTimeoutMs > 0)
            {
                bufferToWrite_.wait_for(lock, std::chrono::milliseconds(waitTimeoutMs));
            }

            // If this wasn't a timeout, then the read index should have changed
            if(nextWriteIdx_ != readIdx_)
            {
                writeLocked_ = true;
                freeBuffers_--;
                locked = true;

                std::vector<unsigned char>& videoWriteBuffer = videoBuffers_[nextWriteIdx_];
                std::vector<unsigned char>& audioWriteBuffer = audioBuffers_[nextWriteIdx_];

                if(videoWriteBuffer.size() != requiredVideoBufferSize_)
                {
                    videoWriteBuffer.resize(requiredVideoBufferSize_);
                }

                if(audioWriteBuffer.size() != requiredAudioBufferSize_)
                {
                    audioWriteBuffer.resize(requiredAudioBufferSize_);
                }

                videoBuffer = videoWriteBuffer.data();
                audioBuffer = audioWriteBuffer.data();
            }
        }

        return locked;
    }

    void ReleaseBufferFromWrite(uint32_t* freeBufferCount = nullptr)
    {
        std::unique_lock<std::mutex> lock(protectBuffers_);

        if(writeLocked_ == true)
        {
            writeLocked_ = false;
            lastWrittenIdx_ = nextWriteIdx_;
            lock.unlock();
            bufferToRead_.notify_one();
        }

        if(freeBufferCount)
        {
            *freeBufferCount = freeBuffers_;
        }
    }

    bool LockBufferForRead(
        unsigned char*& videoBuffer, 
        size_t& videoBufferSize, 
        unsigned char*& audioBuffer, 
        size_t& audioBufferSize, 
        uint32_t waitTimeoutMs)
    {
        bool locked(false);
        std::unique_lock<std::mutex> lock(protectBuffers_);

        if(readLocked_ == false)
        {
            // If the current read index is the last write location, we can't increment past this
            if(readIdx_ == lastWrittenIdx_ && waitTimeoutMs > 0)
            {
                bufferToRead_.wait_for(lock, std::chrono::milliseconds(waitTimeoutMs));
            }

            // If this wasn't a timeout, then the read index should have changed
            if(readIdx_ != lastWrittenIdx_)
            {
                ++readIdx_;
                readIdx_ %= _Size;

                readLocked_ = true;
                locked = true;

                videoBuffer = videoBuffers_[readIdx_].data();
                videoBufferSize = videoBuffers_[readIdx_].size();
                audioBuffer = audioBuffers_[readIdx_].data();
                audioBufferSize = audioBuffers_[readIdx_].size();

                // We have moved the read index forward, freeing a slot to be written to, so signal this
                lock.unlock();
                bufferToWrite_.notify_one();
            }
        }

        return locked;
    }

    void ReleaseBufferFromRead(uint32_t* freeBufferCount = nullptr)
    {
        std::unique_lock<std::mutex> lock(protectBuffers_);

        if(readLocked_ == true)
        {
            readLocked_ = false;
            freeBuffers_++;
        }

        if(freeBufferCount)
        {
            *freeBufferCount = freeBuffers_;
        }
    }

// class types
private:

    uint32_t lastWrittenIdx_;
    uint32_t nextWriteIdx_;
    bool writeLocked_;

    uint32_t readIdx_;
    bool readLocked_;

    uint32_t freeBuffers_;

    std::array<std::vector<unsigned char>, _Size> videoBuffers_;
    std::array<std::vector<unsigned char>, _Size> audioBuffers_;
    std::mutex protectBuffers_;
    std::condition_variable bufferToRead_;
    std::condition_variable bufferToWrite_;
};

}