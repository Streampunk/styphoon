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

#include "TyphoonSDK.h"

namespace streampunk {

// Class to access the Typhoon card's registers
class TyphoonRegister 
{
// Constants/enums
//
public:

    // The register type
    enum Type
    {
        InputFormat = 0,
        MaxType = 1
    };


// Static functions
public:

    // Read the value in the register type for the given channel
    static ULONG Read(TyphoonBoard& board, Type type, ULONG channelIdx);

// Static data
private:

    static ULONG registerLookup[MaxType][TPH_CHANNELS];
};
}