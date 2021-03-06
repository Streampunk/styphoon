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

#include <cstdint>
#include <tuple>
#include <assert.h>

namespace streampunk
{
 
namespace typhoon
{
 
class AudioTransform
{
    static const uint32_t CHANNEL_SEPARATION_BYTES = 0x2000;
    static const uint32_t NUM_USEABLE_CHANNELS = 2;
    static const uint32_t MAX_BUFFER_SIZE = CHANNEL_SEPARATION_BYTES * NUM_USEABLE_CHANNELS;

public:

    AudioTransform(uint32_t sampleSize = 24)
    {
        if(sampleSize == 16)
        {
            convertTo16Bit = true;
        }
        else
        {
            convertTo16Bit = false;
        }
    }


    std::tuple<const unsigned char*, uint32_t> Transform(const unsigned char* inputBuffer, uint32_t inputBufferSize)
    {
        // Start the buffer pointers at the start of each channel's data
        const unsigned char* leftChannelBuffer = inputBuffer;
        const unsigned char* rightChannelBuffer = inputBuffer + CHANNEL_SEPARATION_BYTES;

        // First, get the buffer size, and make sure that both channels have the same amount of data
        uint32_t leftChannelBufferSize = *(reinterpret_cast<const uint32_t*>(leftChannelBuffer));
        uint32_t rightChannelBufferSize = *(reinterpret_cast<const uint32_t*>(rightChannelBuffer));

        if((leftChannelBufferSize != rightChannelBufferSize) || (leftChannelBufferSize % 4 != 0))
        {
            // Something is wrong with the input data - return zero samples
            printf("Typhoon Error: unexpected audio buffer configuration, ignoring audio samples");
            return std::make_tuple(nullptr, 0);
        }

        leftChannelBuffer += sizeof(uint32_t);
        rightChannelBuffer += sizeof(uint32_t);

        uint32_t currentNumSamples = leftChannelBufferSize / 4;
        uint32_t outputBufferIdx(0);

        for(uint32_t sampleIdx = 0; sampleIdx < currentNumSamples; sampleIdx++)
        {
            if(convertTo16Bit)
            { 
                // Accumulate channel L and write to file
                int sample = *(int *)leftChannelBuffer;

                sample = (sample & 0xFF000000) >> 24 | (sample & 0x00FF0000) >> 8 | (sample & 0x0000FF00) << 8 | (sample & 0x000000FF) << 24;
                int sample16 = (sample) & 0x0000FFFF;
                memcpy_s(&outputBuffer[outputBufferIdx], MAX_BUFFER_SIZE - outputBufferIdx, (void *)&sample16, 2);

                outputBufferIdx += 2;
                leftChannelBuffer += 4; 

                // Accumulate channel R and write to file
                sample = *(int *)rightChannelBuffer;
                sample = (sample & 0xFF000000) >> 24 | (sample & 0x00FF0000) >> 8 | (sample & 0x0000FF00) << 8 | (sample & 0x000000FF) << 24;
                sample16 = (sample) & 0x0000FFFF;
                memcpy_s(&outputBuffer[outputBufferIdx], MAX_BUFFER_SIZE - outputBufferIdx, (void *)&sample16, 2);

                outputBufferIdx += 2;
                rightChannelBuffer += 4;
            }
            else
            {
                // Accumulate channel L and write to file
                int sample = *(int *)leftChannelBuffer;
                sample = (sample & 0xFF000000) >> 24 | (sample & 0x00FF0000) >> 8 | (sample & 0x0000FF00) << 8 | (sample & 0x000000FF) << 24;
                sample = sample & 0x00FFFFFF;
                memcpy_s(&outputBuffer[outputBufferIdx], MAX_BUFFER_SIZE - outputBufferIdx, (void *)&sample, 3);

                outputBufferIdx += 3;
                leftChannelBuffer += 4;

                // Accumulate channel R and write to file
                sample = *(int *)rightChannelBuffer;
                sample = (sample & 0xFF000000) >> 24 | (sample & 0x00FF0000) >> 8 | (sample & 0x0000FF00) << 8 | (sample & 0x000000FF) << 24;
                sample = sample & 0x00FFFFFF;
                memcpy_s(&outputBuffer[outputBufferIdx], MAX_BUFFER_SIZE - outputBufferIdx, (void *)&sample, 3);

                outputBufferIdx += 3;
                rightChannelBuffer += 4;
            }
        }

        return std::make_tuple(outputBuffer, outputBufferIdx);
    }


private:

    unsigned char outputBuffer[MAX_BUFFER_SIZE];
    bool convertTo16Bit;
};
}
}