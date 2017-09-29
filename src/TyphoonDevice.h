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

#include <string>
#include <map>
#include <memory>
#include <string>
#include <mutex>
#include <assert.h>
#include "TyphoonSDK.h"

namespace streampunk {

// Class to manage the reservation and release of AJA devices between multiple nodes
class TyphoonDevice 
{
// Constants
//
public:

    static const ULONG DEFAULT_DEVICE_SPECIFIER;
    static const uint32_t DEFAULT_CAPTURE_CHANNEL = 1;
    static const uint32_t DEFAULT_PLAYBACK_CHANNEL = 3;

// Typedefs and nested classes
//
public:
    
    // Factory method prototype to allow the factory methods to be overridden for testing
    typedef TyphoonBoard* (*TyphoonBoard_Factory)(void);

    class Ref
    {
    public:

        // Create a reference - note: for now the init params must be stored in static memory
        Ref();

        // Release() is automatically called within the destructor, when the Ref goes out of scope
        ~Ref();

        // Try to initialize the specified device with the given init params. 
        bool Initialize(ULONG dwDeviceIndex);

        // Manually release the referenced device
        void Release();

        // Allow the reference to be checked in a conditional clause
        operator bool() { return (bool)ref_; }

        TyphoonBoard* operator->() { assert(ref_); return (ref_ ? ref_->device_.get() : nullptr); }
        TyphoonBoard& operator*() { assert(ref_); return *(ref_->device_.get()); }

    private:

        std::shared_ptr<TyphoonDevice> ref_;
    };

    // Return device information - each parameter is filled in if a non-null pointer is supplied
    static bool GetFirstDevice(uint32_t& numDevices, uint32_t* index = nullptr);

    // Return the driver version number
    static bool GetDriverVersion(ULONG& vFirmware, ULONG& vFirmware2, ULONG& vDriver);

    // Test function - shouldn't be needed in production code
    static uint32_t GetRefCount(ULONG deviceIndex);

    // Overrideable factory function for creating the underlying CNTV2Card class
    static TyphoonBoard_Factory typhoonBoard_Factory;

    // default CNTV2Card factory function
    static TyphoonBoard* DefaultTyphoonBoardFactory(void) { return new TyphoonBoard(); }

    virtual ~TyphoonDevice();

private:

    TyphoonDevice(ULONG deviceIndex);

    bool Initialize();
    void ReleaseDevice();

    std::unique_ptr<TyphoonBoard> device_;
    ULONG                         deviceIndex_;

    static bool AddRef(ULONG deviceIndex, std::shared_ptr<TyphoonDevice>& ref);
    static void ReleaseRef(std::shared_ptr<TyphoonDevice>& ref);

    static void DumpDeviceInfo();
    static void DumpInfoItem(std::string& label, std::string& data);

    static std::map<ULONG, std::shared_ptr<TyphoonDevice>> references_;
    static std::mutex protectRefCounts_;
};
}