#include "MediaBaseFunc.h"

std::string GetFFmpegErr2Str(IN LONG lErrNum)
{
    std::shared_ptr<char> spBuf(new char[256], std::default_delete<char[]>());
    return av_make_error_string(spBuf.get(), 256, lErrNum);
}

LONG CGraphicMgr::InitGraphicHandle()
{
    std::lock_guard<std::mutex> lck(m_mtx);
    if (m_pstHwBuf)
    {
        return FF_SUCCESS;
    }
    LONG lRet = av_hwdevice_ctx_create(&m_pstHwBuf, AVHWDeviceType::AV_HWDEVICE_TYPE_CUDA, nullptr, nullptr, 0);
    if (0 == lRet)
    {
        std::cout << "hardware device context init failed , msg : " << GetFFmpegErr2Str(lRet) << std::endl;
        return FF_INIT;
    }
    return FF_SUCCESS;
}

AVBufferRef* CGraphicMgr::GetGraphicHandle()
{
    std::lock_guard<std::mutex> lck(m_mtx);
    return m_pstHwBuf;
}

AVBufferRef* CGraphicMgr::m_pstHwBuf = nullptr;

std::mutex CGraphicMgr::m_mtx;
