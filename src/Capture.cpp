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

/* TODO: add license
**
*/

#include <memory>
#include "Capture.h"
#include "TyphoonTypeMaps.h"

namespace streampunk {

inline Nan::Persistent<v8::Function> &Capture::constructor() {
  static Nan::Persistent<v8::Function> myConstructor;
  return myConstructor;
}

Capture::Capture(uint32_t deviceIndex, uint32_t channelIndex, uint32_t pixelFormat, uint32_t inputSource, bool compressed) 
: deviceIndex_(deviceIndex),
  channelIndex_(channelIndex), 
  genericPixelFormat_(pixelFormat),
  inputSource_(inputSource),
  compressed_(compressed),
  audioEnabled_(false)
  //genericPixelFormat_;
  //nativePixelFormat_
  //width_;
  //height_;
{
  async = new uv_async_t;
  uv_async_init(uv_default_loop(), async, FrameCallback);
  uv_mutex_init(&padlock);
  async->data = this;
}

Capture::~Capture() {
  if (!captureCB_.IsEmpty())
    captureCB_.Reset();
}

NAN_MODULE_INIT(Capture::Init) {

  // Prepare constructor template
  v8::Local<v8::FunctionTemplate> tpl = Nan::New<v8::FunctionTemplate>(New);
  tpl->SetClassName(Nan::New("Capture").ToLocalChecked());
  tpl->InstanceTemplate()->SetInternalFieldCount(1);

  // Prototype
  Nan::SetPrototypeMethod(tpl, "init", DeviceInit);
  Nan::SetPrototypeMethod(tpl, "doCapture", DoCapture);
  Nan::SetPrototypeMethod(tpl, "stop", StopCapture);
  Nan::SetPrototypeMethod(tpl, "enableAudio", EnableAudio);
  Nan::SetPrototypeMethod(tpl, "getDisplayMode", GetDisplayMode);

  constructor().Reset(Nan::GetFunction(tpl).ToLocalChecked());
  Nan::Set(target, Nan::New("Capture").ToLocalChecked(),
               Nan::GetFunction(tpl).ToLocalChecked());
}

NAN_METHOD(Capture::New) {

  if (info.IsConstructCall()) {
    // Invoked as constructor: `new Capture(...)`
    uint32_t deviceIndex = info[0]->IsUndefined() ? 0 : Nan::To<uint32_t>(info[0]).FromJust();
    uint32_t channelIndex = info[1]->IsUndefined() ? 0 : Nan::To<uint32_t>(info[1]).FromJust();
    uint32_t pixelFormat = info[2]->IsUndefined() ? 0 : Nan::To<uint32_t>(info[2]).FromJust();
    uint32_t inputSource = info[3]->IsUndefined() ? 0 : Nan::To<uint32_t>(info[3]).FromJust();
    bool compressed = info[4]->IsUndefined() ? 0 : Nan::To<bool>(info[4]).FromJust();

    Capture* obj = new Capture(deviceIndex, channelIndex, pixelFormat, inputSource, compressed);
    obj->Wrap(info.This());
    info.GetReturnValue().Set(info.This());
  } else {
    // Invoked as plain function `Capture(...)`, turn into construct call.
    const int argc = 5;
    v8::Local<v8::Value> argv[argc] = { info[0], info[1], info[2], info[3], info[4] };
    v8::Local<v8::Function> cons = Nan::New(constructor());
    info.GetReturnValue().Set(Nan::NewInstance(cons, argc, argv).ToLocalChecked());
  }
}

NAN_METHOD(Capture::DeviceInit) {
  Capture* obj = ObjectWrap::Unwrap<Capture>(info.Holder());

  if (obj->initCapture())
      info.GetReturnValue().Set(Nan::New("made it!").ToLocalChecked());
  else
      info.GetReturnValue().Set(Nan::New("sad :-(").ToLocalChecked());
}

NAN_METHOD(Capture::EnableAudio) {
  Capture* obj = ObjectWrap::Unwrap<Capture>(info.Holder());
  HRESULT result;
  //BMDAudioSampleRate sampleRate = info[0]->IsNumber() ?
  //    (BMDAudioSampleRate) Nan::To<uint32_t>(info[0]).FromJust() : bmdAudioSampleRate48kHz;
  //BMDAudioSampleType sampleType = info[1]->IsNumber() ?
  //    (BMDAudioSampleType) Nan::To<uint32_t>(info[1]).FromJust() : bmdAudioSampleType16bitInteger;
  //uint32_t channelCount = info[2]->IsNumber() ? Nan::To<uint32_t>(info[2]).FromJust() : 2;

  result = obj->setupAudioInput(/*sampleRate, sampleType, channelCount*/);

  switch (result) {
    case E_INVALIDARG:
      info.GetReturnValue().Set(
        Nan::New<v8::String>("audio channel count must be 2, 8 or 16").ToLocalChecked());
      break;
    case S_OK:
      info.GetReturnValue().Set(Nan::New<v8::String>("audio enabled").ToLocalChecked());
      break;
    default:
      info.GetReturnValue().Set(Nan::New<v8::String>("failed to start audio").ToLocalChecked());
      break;
  }
}

NAN_METHOD(Capture::DoCapture) {
  v8::Local<v8::Function> cb = v8::Local<v8::Function>::Cast(info[0]);
  Capture* obj = ObjectWrap::Unwrap<Capture>(info.Holder());
  obj->captureCB_.Reset(cb);

  if (obj->capture())
  {
    info.GetReturnValue().Set(Nan::New("Capture started.").ToLocalChecked());
  }
  else
  {
      info.GetReturnValue().Set(Nan::New("Unable to start capture.").ToLocalChecked());
  }
}


NAN_METHOD(Capture::StopCapture) {
  Capture* obj = ObjectWrap::Unwrap<Capture>(info.Holder());

  if (obj->stop())
  {
    info.GetReturnValue().Set(Nan::New<v8::String>("Capture stopped.").ToLocalChecked());
  }
  else
  {
    info.GetReturnValue().Set(Nan::New<v8::String>("Unable to stop capture.").ToLocalChecked());
  }
}


NAN_METHOD(Capture::GetDisplayMode)
{
  Capture* obj = ObjectWrap::Unwrap<Capture>(info.Holder());

  info.GetReturnValue().Set(obj->GetDisplayMode());
}


bool Capture::capture()
{
    bool success = false;

    if (capture_)
    {
        capture_->Start();

        success = true;
    }

    return success;
}


bool Capture::stop()
{
    bool success = false;

    if (capture_)
    {
        capture_->Stop();

        success = true;
    }

    return success;
}


uint32_t Capture::GetDisplayMode()
{
    uint32_t displayMode = bmdModeUnknown;

    if (capture_)
    {
        displayMode = TPH_DISPLAY_MODE_MAP.ToA(capture_->GetConfig().SignalStandard);
    }

    return displayMode;
}


bool Capture::initCapture()
{
    bool  success(false);

    ULONG typhoonPixelFormat = TPH_PIXEL_FORMAT_MAP.ToB(static_cast<GenericPixelFormat>(genericPixelFormat_));

    TyphoonCapture::ChannelConfig config(
        TPH_FORMAT_1080i_5994, 
        typhoonPixelFormat, 
        inputSource_,
        compressed_);

    // Instantiate the TyphoonCapture object, using the specified Typhoon device...
    capture_.reset(TyphoonCapture::Create(deviceIndex_, channelIndex_, config, Capture::_frameArrived, this));

    if (!capture_)
    {
        printf("Capture initialization failed\n");
    }

    return success;
}


HRESULT Capture::setupAudioInput(/*BMDAudioSampleRate sampleRate,
  BMDAudioSampleType sampleType, uint32_t channelCount*/) {

  audioEnabled_ = true;
  // TODO: handle audio properly

  //sampleByteFactor_ = channelCount * (sampleType / 8);
  //HRESULT result = m_deckLinkInput->EnableAudioInput(sampleRate, sampleType, channelCount);

  return S_OK;
}

// Stop video input
bool Capture::cleanupCapture()
{
    bool success = false;

    if (capture_)
    {
        capture_->Stop();
        capture_.reset();

        success = true;
    }

    return success;
}

bool Capture::initInput() {
  return true;
}


void Capture::frameArrived()
{
  uv_async_send(async);
}


void Capture::_frameArrived(void* context)
{
    Capture* localThis = reinterpret_cast<Capture*>(context);

    localThis->frameArrived();
}


//HRESULT    Capture::VideoInputFormatChanged (BMDVideoInputFormatChangedEvents notificationEvents, IDeckLinkDisplayMode* newDisplayMode, BMDDetectedVideoInputFormatFlags detectedSignalFlags) {
//  return S_OK;
//};

void Capture::TestUV() {
  uv_async_send(async);
}


NAUV_WORK_CB(Capture::FrameCallback) {
  Nan::HandleScope scope;
  Capture *capture = static_cast<Capture*>(async->data);
  Nan::Callback cb(Nan::New(capture->captureCB_));

  v8::Local<v8::Value> bv = Nan::Null();
  v8::Local<v8::Value> ba = Nan::Null();
  uv_mutex_lock(&capture->padlock);

  auto nextFrame = capture->capture_->LockNextFrame(0);

  if (nextFrame != nullptr)
  {
    if(capture->compressed_ && nextFrame->dataBuffer != nullptr)
    {
        bv = Nan::CopyBuffer(reinterpret_cast<char*>(nextFrame->dataBuffer), static_cast<uint32_t>(nextFrame->dataBufferSize)).ToLocalChecked();
    }
    else if ((!capture->compressed_) && nextFrame->videoBuffer != nullptr)
    {
        bv = Nan::CopyBuffer(reinterpret_cast<char*>(nextFrame->videoBuffer), static_cast<uint32_t>(nextFrame->videoBufferSize)).ToLocalChecked();
    }

    if (nextFrame->audioBuffer != nullptr && capture->audioEnabled_ == true)
    {
        ba = Nan::CopyBuffer(reinterpret_cast<char*>(nextFrame->audioBuffer), static_cast<uint32_t>(nextFrame->audioBufferSize)).ToLocalChecked();
    }

    capture->capture_->UnlockFrame();
    nextFrame = nullptr;
  }

  v8::Local<v8::Value> argv[2] = { bv, ba };
  cb.Call(2, argv);
}

}
