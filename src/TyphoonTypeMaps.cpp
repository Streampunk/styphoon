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

#include "TyphoonTypeMaps.h"

TyphoonDisplayModeMap::Entry _displayModeMapTable[] =
{
    { bmdModeNTSC,          TPH_FORMAT_NTSC },
    { bmdModePAL,           TPH_FORMAT_PAL },
    { bmdModeHD1080p25,     TPH_FORMAT_1080p_2500 },
    { bmdModeHD1080p2997,   TPH_FORMAT_1080p_2997 },
    { bmdModeHD1080i50,     TPH_FORMAT_1080i_5000 },
    { bmdModeHD1080i5994,   TPH_FORMAT_1080i_5994 },
    { bmdModeHD720p50,      TPH_FORMAT_720p_5000 },
    { bmdModeHD720p5994,    TPH_FORMAT_720p_5994 },
    { bmdModeUnknown,       TPH_FORMAT_UNKNOWN }
};


const TyphoonDisplayModeMap TPH_DISPLAY_MODE_MAP(_displayModeMapTable, bmdModeUnknown, TPH_FORMAT_UNKNOWN);

TyphoonModeGetSetTranslationMap::Entry _getSetTranslationMap[] =
{
    { TPH_DSG4_REGREAD_FORMAT_1080P_25,   TPH_FORMAT_1080p_2500 },
    { TPH_DSG4_REGREAD_FORMAT_1080P_2997, TPH_FORMAT_1080p_2997 },
    { TPH_DSG4_REGREAD_FORMAT_1080I_50,   TPH_FORMAT_1080i_5000 },
    { TPH_DSG4_REGREAD_FORMAT_1080I_5994, TPH_FORMAT_1080i_5994 },
    { TPH_DSG4_REGREAD_FORMAT_720P_50,    TPH_FORMAT_720p_5000 },
    { TPH_DSG4_REGREAD_FORMAT_720P_5994,  TPH_FORMAT_720p_5994 },
    { TPH_DSG4_REGREAD_FORMAT_PAL,        TPH_FORMAT_PAL },
    { TPH_DSG4_REGREAD_FORMAT_NTSC,       TPH_FORMAT_NTSC }
};

const TyphoonModeGetSetTranslationMap TPH_DISPLAY_MODE_TRANSLATION_MAP(_getSetTranslationMap, TPH_FORMAT_UNKNOWN, TPH_FORMAT_UNKNOWN);

