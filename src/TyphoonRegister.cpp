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

#include <assert.h>
#include "TyphoonRegister.h"

using namespace std;

namespace streampunk {

ULONG TyphoonRegister::registerLookup[TyphoonRegister::MaxType][TPH_CHANNELS] =
{
    //   CH1,    CH2,    CH3,    CH4
    // InputSignalStandard Register
    { 0x11C4, 0x25C4, 0x35C4, 0X45C4 },
    { 0x1004, 0x2004, 0x3004, 0x4004 }
};

ULONG TyphoonRegister::Read(TyphoonBoard& board, Type type, ULONG channelIdx)
{
    assert(type < MaxType);
    assert(channelIdx < TPH_CHANNELS);

    ULONG value(0);

    if((type < MaxType) && (channelIdx < TPH_CHANNELS))
    {
        value = board.RegRead(registerLookup[type][channelIdx]);
    }

    return value;
}

// Write the value in the register type for the given channel
bool TyphoonRegister::Write(TyphoonBoard& board, Type type, ULONG channelIdx, ULONG value)
{
    assert(type < MaxType);
    assert(channelIdx < TPH_CHANNELS);
    bool success(false);

    if((type < MaxType) && (channelIdx < TPH_CHANNELS))
    {
        success = static_cast<bool>(board.RegWrite(registerLookup[type][channelIdx], value));
    }

    return success;
}

ULONG TyphoonRegister::GetDesign(TyphoonBoard& board)
{
    ULONG value(0);

    // Read the design of the Typhoon board
    value = (board.RegRead(TPH_DESIGN_REGISTER) & TPH_DESIGN_BITMASK) >> TPH_DESIGN_RSHIFT;

    return value;
}

}