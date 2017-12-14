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
#include "CppUnitTest.h"
#include <string>
#include <memory>
#include <array>
#include <thread>
#include "CircularBuffer.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace std;
using namespace streampunk;

namespace StyphoonTest
{   
    array<string, 13> TestData = 
    {
        "Test String 1",
        "Second Test String",
        "Test String Number 3",
        "String 4",
        "Another string, 5 this time",
        "Sixth String",
        "String Seven",
        "8, this one",
        "Number 9, number 9, number9",
        "Ten",
        "Up to eleven now",
        "This is almost there - twelve",
        "Last but not least, 13",
    };

    TEST_CLASS(CircularBufferTest)
    {
    public:
        
        void FastProducer()
        {
            for(auto i = 0; i < TestData.size(); i++)
            {
                auto numBytesRequired = TestData[i].size() + 1;
                unsigned char* buffer = nullptr;

                bool locked = testBuffer->LockBufferForWrite(buffer, numBytesRequired, 0);

                if(locked == false)
                {
                    ++producerBlockedCount;

                    // Lock again for long enough to allow a write
                    //
                    locked = testBuffer->LockBufferForWrite(buffer, numBytesRequired, 1000);

                    Assert::IsTrue(locked, L"Fast Producer Buffer locked for write");
                }

                memcpy_s(buffer, numBytesRequired, TestData[i].c_str(), numBytesRequired);

                testBuffer->ReleaseBufferFromWrite();

                _sleep(5);
            }
        }

        void SlowProducer()
        {
            for(auto i = 0; i < TestData.size(); i++)
            {
                auto numBytesRequired = TestData[i].size() + 1;
                unsigned char* buffer = nullptr;

                bool locked = testBuffer->LockBufferForWrite(buffer, numBytesRequired, 0);

                Assert::IsTrue(locked, L"Slow Producer Buffer locked for write");

                memcpy_s(buffer, numBytesRequired, TestData[i].c_str(), numBytesRequired);

                testBuffer->ReleaseBufferFromWrite();

                _sleep(100);
            }
        }

        void FastConsumer()
        {
            for(auto i = 0; i < TestData.size(); i++)
            {
                size_t numBytesToRead(0);
                unsigned char* buffer = nullptr;

                bool locked = testBuffer->LockBufferForRead(buffer, numBytesToRead, 0);

                if(locked == false)
                {
                    ++consumerBlockedCount;

                    // Lock again for long enough to allow a write
                    //
                    locked = testBuffer->LockBufferForRead(buffer, numBytesToRead, 1000);

                    Assert::IsTrue(locked, L"Fast Consumer Buffer locked for read");
                }

                Assert::AreEqual(0, TestData[i].compare((char*)buffer), L"Output strings");

                testBuffer->ReleaseBufferFromRead();

                _sleep(5);
            }
        }

        void SlowConsumer()
        {
            for(auto i = 0; i < TestData.size(); i++)
            {
                size_t numBytesToRead(0);
                unsigned char* buffer = nullptr;

                bool locked = testBuffer->LockBufferForRead(buffer, numBytesToRead, 0);

                Assert::IsTrue(locked, L"Slow Consumer Buffer locked for read");

                Assert::AreEqual(0, TestData[i].compare((char*)buffer), L"Output strings");

                testBuffer->ReleaseBufferFromRead();

                _sleep(100);
            }
        }

        uint32_t producerBlockedCount;
        uint32_t consumerBlockedCount;

        unique_ptr<CircularBuffer<4>> testBuffer;

        TEST_METHOD_INITIALIZE(methodName)   
        {
            producerBlockedCount = 0;
            consumerBlockedCount = 0;
            testBuffer.reset(new CircularBuffer<4>());
        }

        TEST_METHOD(TestFastProducerSlowConsumer)
        {
            std::thread producer(&CircularBufferTest::FastProducer,std::ref(*this));
            std::thread consumer(&CircularBufferTest::SlowConsumer,std::ref(*this));

            producer.join();
            consumer.join();

            Assert::IsTrue(producerBlockedCount > 0);
        }

        TEST_METHOD(TestSlowProducerFastConsumer)
        {
            std::thread producer(&CircularBufferTest::SlowProducer,std::ref(*this));
            std::thread consumer(&CircularBufferTest::FastConsumer,std::ref(*this));

            producer.join();
            consumer.join();

            Assert::IsTrue(consumerBlockedCount > 0);
        }

    };
}