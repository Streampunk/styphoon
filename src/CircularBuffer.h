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
        readIdx_(-1)
    {}

    bool LockBufferForWrite(unsigned char*& buffer, size_t requiredBufferSize, uint32_t waitTimeoutMs)
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
                locked = true;

                std::vector<unsigned char>& writeBuffer = buffers_[nextWriteIdx_];

                if(writeBuffer.size() != requiredBufferSize)
                {
                    writeBuffer.resize(requiredBufferSize);
                }

                buffer = writeBuffer.data();
            }
        }

        return locked;
    }

    void ReleaseBufferFromWrite()
    {
        std::unique_lock<std::mutex> lock(protectBuffers_);

        if(writeLocked_ == true)
        {
            writeLocked_ = false;
            lastWrittenIdx_ = nextWriteIdx_;
            lock.unlock();
            bufferToRead_.notify_one();
        }
    }

    bool LockBufferForRead(unsigned char*& buffer, size_t& bufferSize, uint32_t waitTimeoutMs)
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

                buffer = buffers_[readIdx_].data();
                bufferSize = buffers_[readIdx_].size();

                // We have moved the read index forward, freeing a slot to be written to, so signal this
                lock.unlock();
                bufferToWrite_.notify_one();
            }
        }

        return locked;
    }

    void ReleaseBufferFromRead()
    {
        std::unique_lock<std::mutex> lock(protectBuffers_);

        if(readLocked_ == true)
        {
            readLocked_ = false;
        }
    }

// class types
private:

    uint32_t lastWrittenIdx_;
    uint32_t nextWriteIdx_;
    bool writeLocked_;

    uint32_t readIdx_;
    bool readLocked_;

    std::array<std::vector<unsigned char>, _Size> buffers_;
    std::mutex protectBuffers_;
    std::condition_variable bufferToRead_;
    std::condition_variable bufferToWrite_;
};

}