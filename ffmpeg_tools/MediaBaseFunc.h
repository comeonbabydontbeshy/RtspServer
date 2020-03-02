#pragma once

#include "MediaPublic.h"
#include <iostream>
#include <string>
#include <memory>
#include <mutex>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include "libavcodec/avcodec.h"

#ifdef __cplusplus
}
#endif // __cplusplus

std::string GetFFmpegErr2Str(IN LONG lErrNum);

class CGraphicMgr
{
public:
    CGraphicMgr() = default;

    ~CGraphicMgr() = default;

    static LONG InitGraphicHandle();

    static AVBufferRef* GetGraphicHandle();
private:

    static AVBufferRef* m_pstHwBuf;

    static std::mutex m_mtx;
};