#pragma once

#include "MediaPublic.h"
#include "MediaBaseFunc.h"
#include <iostream>
#include <unordered_map>
#include <memory>
#include <string>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include "libavformat/avformat.h"

#include "libavutil/time.h"

#ifdef __cplusplus
}
#endif // __cplusplus

class CMediaHandle
{
public:
    explicit CMediaHandle(IN const std::string& strFilePath);

    LONG InitMediaInfo();

    LONG InitDecoder();

    LONG InitEncoder();

    LONG InitOutputFormatInfo();

    LONG Run();

private:
    std::string GetDecoderNameById(IN AVCodecID enCodecId);

    VOID CalculateVideoRate();

    VOID CalculateDuration();

    std::string m_strFilePath;

    std::shared_ptr<AVFormatContext> m_spAvFormatCtx;

    std::shared_ptr<AVCodecContext> m_spAvCodecCtx;

    std::shared_ptr<AVCodecContext> m_spAvEncodecCtx;

    std::shared_ptr<AVFormatContext> m_spOutputFmtCtx;

    AVCodecID m_enCodecID;

    ULONG m_ulFrameRate = 25;

    LONG m_lVideoIdx = -1;

    ULONG m_ulDuration = 0;

    AVCodecParameters* m_pstCodecParm = nullptr;

    LONG m_lWidth = 0;

    LONG m_lHeight = 0;

    static std::unordered_map<AVCodecID, std::string> m_CodecNameMap;
};