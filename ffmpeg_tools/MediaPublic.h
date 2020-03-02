#pragma once

#include <stdint.h>

using LONG = int32_t;
using ULONG = uint32_t;
using UCHAR = uint8_t;
using CHAR = int8_t;
using VOID = void;

#define IN
#define OUT

enum ErrCode
{
    FF_SUCCESS,
    FF_ERR,
    FF_INIT,
};

enum PacketType
{
    H264,
};
