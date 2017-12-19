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

#define _WINSOCKAPI_

#include <node.h>
#include "node_buffer.h"
#include <stdio.h>
#include <nan.h>

#ifdef WIN32
#include <tchar.h>
#include <conio.h>
#endif

#include "Capture.h"
#include "TyphoonDevice.h"

using namespace v8;
using namespace streampunk;

NAN_METHOD(deviceSdkVersion) 
{
  ULONG firmware1(0);
  ULONG firmware2(0);
  ULONG driverVersion(0);

  char sdkVer [160];

  if(TyphoonDevice::GetDriverVersion(firmware1, firmware2, driverVersion))
  {
    ULONG fw1_1 = (firmware1 & 0xFF00) >> 8;
    ULONG fw1_2 = (firmware1 & 0xFF);
    ULONG fw2_1 = (firmware2 & 0xFF00) >> 8;
    ULONG fw2_2 = (firmware2 & 0xFF);
    ULONG drv_1 = (driverVersion & 0xFF00) >> 8;
    ULONG drv_2 = (driverVersion & 0xF0) >> 4;
    ULONG drv_3 = (driverVersion & 0x0F);

    sprintf_s(sdkVer, "Typhoon SDK Details: fw1=%x.%x fw2=%x.%x driver=%x.%x.%x", 
              fw1_1,
              fw1_2,
              fw2_1,
              fw2_2,
              drv_1,
              drv_2,
              drv_3);
  }
  else
  {
    sprintf_s(sdkVer, "Typhoon SDK Details: Unavailable - ERROR");
  }

  info.GetReturnValue().Set(Nan::New(sdkVer).ToLocalChecked());
}

NAN_METHOD(getFirstDevice) {
  uint32_t numDevices;
  uint32_t firstDeviceIndex;

  if(TyphoonDevice::GetFirstDevice(numDevices, &firstDeviceIndex))
  {
    info.GetReturnValue().Set(Nan::New<v8::Uint32>(firstDeviceIndex));
  }
  else
  {
    info.GetReturnValue().SetUndefined();
  }
}


NAN_MODULE_INIT(Init) {
  Nan::Export(target, "deviceSdkVersion", deviceSdkVersion);
  Nan::Export(target, "getFirstDevice", getFirstDevice);
  streampunk::Capture::Init(target);
}

NODE_MODULE(ajatation, Init);
