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

#include <assert.h>
#include <iostream>
#include "TyphoonDevice.h"

using namespace std;

namespace streampunk {

TyphoonDevice::TyphoonBoard_Factory TyphoonDevice::typhoonBoard_Factory = TyphoonDevice::DefaultTyphoonBoardFactory;
const ULONG TyphoonDevice::DEFAULT_DEVICE_SPECIFIER = 0;


map<ULONG, shared_ptr<TyphoonDevice>> TyphoonDevice::references_;
mutex TyphoonDevice::protectRefCounts_;

TyphoonDevice::Ref::Ref()
{
    {cout << "Creating Typhoon reference at address:" << (uintptr_t)this << endl; }
}

TyphoonDevice::Ref::~Ref()
{
    {cout << "Creating Typhoon reference at address: " << (uintptr_t)this << " reference is: " << (uintptr_t)ref_.get() << endl; }

    if (ref_)
    {
        {cout << "Releasing Typhoon reference at address: " << (uintptr_t)this << " reference is: " << (uintptr_t)ref_.get() << endl; }

        Release();
    }
}


bool TyphoonDevice::Ref::Initialize(ULONG dwDeviceIndex)
{
    assert(!ref_);

    bool success(false);

    if (!ref_)
    {
        {cout << "Initializing Typhoon reference at address:" << (uintptr_t)this << " reference is: " << (uintptr_t)ref_.get() << endl; }
        success = TyphoonDevice::AddRef(dwDeviceIndex, ref_);
        {cout << "Initialized Typhoon reference at address:" << (uintptr_t)this << " reference is: " << (uintptr_t)ref_.get() << endl; }
    }

    return success;
}


void TyphoonDevice::Ref::Release()
{
    assert(ref_);

    if (ref_)
    {
        TyphoonDevice::ReleaseRef(ref_);
    }
}


// Return device information - each parameter is filled in if a non-null pointer is supplied
bool TyphoonDevice::GetFirstDevice(uint32_t& numDevices, uint32_t* index)
{
    numDevices = TyphoonBoard::BoardCount();

    if(numDevices > 0 && index != nullptr)
    {
        *index = 0;
    }

    return true;
}


// Return the driver version number
bool TyphoonDevice::GetDriverVersion(ULONG& vFirmware, ULONG& vFirmware2, ULONG& vDriver)
{
    bool success(false);

    //CNTV2Card tempCard;
    //
    //auto status = CNTV2DeviceScanner::GetFirstDeviceFromArgument("0", tempCard);
    //
    //
    //if(AJA_SUCCESS((int)status))
    //{
    //    success = tempCard.GetDriverVersionComponents(major, minor, point, build);
    //}

    return success;
}


void TyphoonDevice::DumpDeviceInfo()
{
    //TODO: Dump information about all available devices
}


uint32_t TyphoonDevice::GetRefCount(ULONG deviceIndex)
{
    uint32_t refCount(0);

    auto entry = references_.find(deviceIndex);

    if (entry != references_.end())
    {
        refCount = entry->second.use_count();
    }

    return refCount;
}


TyphoonDevice::~TyphoonDevice()
{
    {cout << "Destroying Typhoon device " << deviceIndex_ << " at address:" << (uintptr_t)this << endl; }
}


TyphoonDevice::TyphoonDevice(ULONG deviceIndex)
: deviceIndex_(deviceIndex)
{
    {cout << "Creating Typhoon device " << deviceIndex_ << " at address:" << (uintptr_t)this << endl; }
}


bool TyphoonDevice::AddRef(ULONG deviceIndex, std::shared_ptr<TyphoonDevice>& ref)
{
    bool success(false);

    lock_guard<mutex> lock(protectRefCounts_);

    auto entry = references_.find(deviceIndex);

    // If there is an existing record, increment the ref count,
    // otherwise create and initialize the device
    if (entry != references_.end())
    {
        {cout << "Adding reference to device " << deviceIndex << endl; }

        ref = entry->second;

        success = true;
    }
    else
    {
        {cout << "First initialization of device " << deviceIndex << endl; }

        if(deviceIndex < TyphoonBoard::BoardCount())
        {
            shared_ptr<TyphoonDevice> tempRef(new TyphoonDevice(deviceIndex));

            success = tempRef->Initialize();

            if (success)
            {
                references_[deviceIndex] = tempRef;
                ref = tempRef;
            }
        }
        else
        {
            {cout << "Error, device index: " << deviceIndex << " is invalid" << endl; }
        }
    }

    return success;
}


void TyphoonDevice::ReleaseRef(shared_ptr<TyphoonDevice>& ref)
{
    lock_guard<mutex> lock(protectRefCounts_);

    assert(ref);

    if(ref)
    {
        auto entry = references_.find(ref->deviceIndex_);

        assert(entry != references_.end());

        if (entry != references_.end())
        {
            assert(entry->second.get() == ref.get());

            ref.reset();

            // If there is only the reference in the map remaining, cleanup and remove
            // the record
            if (entry->second.use_count() == 1)
            {
                entry->second->ReleaseDevice();

                references_.erase(entry->second->deviceIndex_);
            }
        }
        else
        {
            cerr << "## ERROR:  Attempt to release a device that isn't recognised: " << ref->deviceIndex_ << endl;
        }
    }
}


bool TyphoonDevice::Initialize()
{
    assert(!device_);
    if(device_)
    {
        cerr << "## ERROR:  Device '" << deviceIndex_ << "' is already initialized" << endl;
        return false;
    }

    unique_ptr<TyphoonBoard> tempDevice(typhoonBoard_Factory());

    if (!tempDevice)
    {
        // If memory really is low, don't log a message to make things worse
        return false;
    }

    bool success = (bool)tempDevice->Open(deviceIndex_);

    // We shouldn't need to do this, but the board seems very timing sensitive
    Sleep(500);

    // Transfer the device pointer to its permanent location
    device_.reset(tempDevice.release());

    return success;
}


void TyphoonDevice::ReleaseDevice()
{
    if(device_)
    {
        device_->Close();
    }
}

}