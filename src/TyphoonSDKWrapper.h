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

/*
TyphoonSDKWapper.h: this file is necessary because the TyphoonSDK.h file has no #pragma once or #ifdef
block to protect it from multiple inclusions; and als because there are supplementary API 
constants not supplied by CoreEl which are needed to use the full Typhoon API control set
*/
#pragma once

#include "TyphoonSDK.h"

// Additional constants for using the Typhoon API
#define TPH_DESIGN_REGISTER 0x0
#define TPH_DESIGN_BITMASK 0xFF000000
#define TPH_DESIGN_RSHIFT 24

#define TPH_DESIGN_4ENCODER_NO_FEC 4

#define TPH_DSG4_REGREAD_FORMAT_1080P_60    0x00000000
#define TPH_DSG4_REGREAD_FORMAT_1080P_5994  0x00000001
#define TPH_DSG4_REGREAD_FORMAT_1080P_50    0x00000002
#define TPH_DSG4_REGREAD_FORMAT_1080P_30    0x00000003
#define TPH_DSG4_REGREAD_FORMAT_1080P_2997  0x00000004
#define TPH_DSG4_REGREAD_FORMAT_1080P_25    0x00000005
#define TPH_DSG4_REGREAD_FORMAT_1080p_24    0x00000006
#define TPH_DSG4_REGREAD_FORMAT_1080p_2398  0x00000007
#define TPH_DSG4_REGREAD_FORMAT_720P_60     0x00000010
#define TPH_DSG4_REGREAD_FORMAT_720P_5994   0x00000011
#define TPH_DSG4_REGREAD_FORMAT_720P_50     0x00000012
#define TPH_DSG4_REGREAD_FORMAT_720P_30     0x00000013
#define TPH_DSG4_REGREAD_FORMAT_720P_2997   0x00000014
#define TPH_DSG4_REGREAD_FORMAT_720P_25     0x00000015
#define TPH_DSG4_REGREAD_FORMAT_1080I_60    0x00000018
#define TPH_DSG4_REGREAD_FORMAT_1080I_5994  0x00000019
#define TPH_DSG4_REGREAD_FORMAT_1080I_50    0x0000001A
#define TPH_DSG4_REGREAD_FORMAT_NTSC        0x0000001E
#define TPH_DSG4_REGREAD_FORMAT_PAL         0x0000001F

// Provide extra unknown field for communicating to client code
#define TPH_FORMAT_UNKNOWN  0xFFFFFFFF
