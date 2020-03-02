#include "MediaHandle.h"

#include <fstream>

CMediaHandle::CMediaHandle(IN const std::string& strFilePath):m_strFilePath(strFilePath)
{

}

LONG CMediaHandle::InitMediaInfo()
{
    LONG lRet = FF_SUCCESS;

    do
    {
        AVDictionary* pcOptions = nullptr;
        lRet = av_dict_set(&pcOptions, "stimeout", "1000000", 0);         /* 单位 : us 详见rtsp.c */
        if (0 < lRet)
        {
            std::cout << "set option failed , msg : " << GetFFmpegErr2Str(lRet) << std::endl;
            lRet = FF_INIT;
            break;
        }
        lRet = av_dict_set(&pcOptions, "rtsp_transport", "TCP", 0);
        if (0 < lRet)
        {
            std::cout << "set option failed , msg : " << GetFFmpegErr2Str(lRet) << " . url : " << m_strFilePath << std::endl;
            lRet = FF_INIT;
            break;
        }
        AVFormatContext* pstFmtCtx = avformat_alloc_context();
        if (nullptr == pstFmtCtx)
        {
            std::cout << "init format context failed " << " . url : " << m_strFilePath << std::endl;
            lRet = FF_INIT;
            break;
        }
        lRet = avformat_open_input(&pstFmtCtx, m_strFilePath.c_str(), nullptr, &pcOptions);
        av_dict_free(&pcOptions);
        if (0 != lRet)
        {
            std::cout << "open format context failed , msg : " << GetFFmpegErr2Str(lRet) << " . url : " << m_strFilePath << std::endl;
            lRet = FF_INIT;
            break;
        }
        m_spAvFormatCtx = std::shared_ptr<AVFormatContext>(pstFmtCtx, [](AVFormatContext* pstFmtContext)
            {
                avformat_close_input(&pstFmtContext);
            });
        m_spAvFormatCtx->probesize = 2 * 1024 * 1024;               /* avformat_find_stream_info 最大读取数据大小(bytes) */
        m_spAvFormatCtx->max_analyze_duration = 10 * AV_TIME_BASE;  /* avformat_find_stream_info 最大解析时间(s) */
        lRet = avformat_find_stream_info(m_spAvFormatCtx.get(), nullptr);
        if (0 != lRet)
        {
            std::cout << "open format context failed , msg : " << GetFFmpegErr2Str(lRet) << " . url : " << m_strFilePath << std::endl;
            lRet = FF_INIT;
            break;
        }
        for (ULONG ulIdx = 0; ulIdx < m_spAvFormatCtx->nb_streams; ulIdx++)
        {
            AVStream* pstStream = m_spAvFormatCtx->streams[ulIdx];
            if (AVMEDIA_TYPE_VIDEO == pstStream->codecpar->codec_type)
            {
                m_pstCodecParm = pstStream->codecpar;
                m_enCodecID = m_pstCodecParm->codec_id;
                m_lVideoIdx = ulIdx;
                m_lWidth = m_pstCodecParm->width;
                m_lHeight = m_pstCodecParm->height;
                break;
            }
        }
        if (-1 == m_lVideoIdx)
        {
            std::cout << "cannot find video index . url : " << m_strFilePath << std::endl;
            lRet = FF_INIT;
            break;
        }
        std::cout << "resolution : " << "(" << m_lWidth << "*" << m_lHeight << "). open success url : " << m_strFilePath << std::endl;

        CalculateVideoRate();

        CalculateDuration();

    } while (0);

    return lRet;
}

LONG CMediaHandle::InitDecoder()
{
    LONG lRet = FF_SUCCESS;

    do
    {
        std::string strDecoderName = GetDecoderNameById(m_enCodecID);
        if (strDecoderName.empty())
        {
            std::cout << "find decoder name by id failed , id : " << m_enCodecID << std::endl;
            lRet = FF_INIT;
            break;
        }
        AVCodec* pstCodec = avcodec_find_decoder_by_name(strDecoderName.c_str());
        if (nullptr == pstCodec)
        {
            std::cout << "find decoder by name failed , name : " << strDecoderName << std::endl;
            lRet = FF_INIT;
            break;
        }
        m_spAvCodecCtx = std::shared_ptr<AVCodecContext>(avcodec_alloc_context3(pstCodec), [](AVCodecContext* pstCodecCtx)
            {

                if (pstCodecCtx->hwaccel_context)
                {
                    av_buffer_unref((AVBufferRef**)&pstCodecCtx->hwaccel_context);
                }
                if (avcodec_is_open(pstCodecCtx))
                {
                    avcodec_close(pstCodecCtx);
                }
                avcodec_free_context(&pstCodecCtx);
            });
        lRet = avcodec_parameters_to_context(m_spAvCodecCtx.get(), m_pstCodecParm);
        if (lRet < 0)
        {
            std::cout << "copy codec parameter to codec context failed , msg : " << GetFFmpegErr2Str(lRet) << std::endl;
            lRet = FF_INIT;
            break;
        }
        lRet = avcodec_open2(m_spAvCodecCtx.get(), pstCodec, nullptr);
        if (0 != lRet)
        {
            std::cout << "open codec failed , msg : " << GetFFmpegErr2Str(lRet) << std::endl;
            lRet = FF_INIT;
            break;
        }
    } while (0);

    return lRet;
}

LONG CMediaHandle::InitEncoder()
{
    LONG lRet = FF_SUCCESS;

    do
    {
        AVCodec* pstEncoder = avcodec_find_encoder_by_name("h264_nvenc");
        if (nullptr == pstEncoder)
        {
            std::cout << "find encoder failed . " << std::endl;
            break;
        }
        m_spAvEncodecCtx = std::shared_ptr<AVCodecContext>(avcodec_alloc_context3(pstEncoder), [](AVCodecContext* pstEncCtx)
            {
                if (avcodec_is_open(pstEncCtx))
                {
                    avcodec_close(pstEncCtx);
                }
                avcodec_free_context(&pstEncCtx);
            });
        m_spAvEncodecCtx->bit_rate = m_spAvFormatCtx->bit_rate;
        m_spAvEncodecCtx->width = m_spAvCodecCtx->width;
        m_spAvEncodecCtx->height = m_spAvCodecCtx->height;
        m_spAvEncodecCtx->gop_size = 10;
        m_spAvEncodecCtx->max_b_frames = 0;
        m_spAvEncodecCtx->time_base = m_spAvFormatCtx->streams[m_lVideoIdx]->time_base;

        bool bSupportFmt = false;
        AVPixelFormat* pstFmt = (enum AVPixelFormat*)pstEncoder->pix_fmts;
        for (; *pstFmt != AV_PIX_FMT_NONE; pstFmt++)
        {
            if (AV_PIX_FMT_NV12 == *pstFmt)
            {
                bSupportFmt = true;
                m_spAvEncodecCtx->pix_fmt = AV_PIX_FMT_NV12;
                break;
            }
        }
        if (!bSupportFmt)
        {
            lRet = FF_INIT;
            break;
        }
        lRet = avcodec_open2(m_spAvEncodecCtx.get(), pstEncoder, nullptr);
        if (0 != lRet)
        {
            std::cout << "codec open failed , msg : " << GetFFmpegErr2Str(lRet) << std::endl;
            lRet = FF_INIT;
            break;
        }

    } while (0);

    return lRet;
}

LONG CMediaHandle::InitOutputFormatInfo()
{
    LONG lRet = FF_SUCCESS;

    do 
    {
        std::string strOutFileName = "./output.ts";
        AVFormatContext* pstOutputFmtCtx = nullptr;
        lRet = avformat_alloc_output_context2(&pstOutputFmtCtx, nullptr, nullptr, strOutFileName.c_str());
        if (0 > lRet)
        {
            std::cout << "alloc output format context failed , msg : " << GetFFmpegErr2Str(lRet);
            lRet = FF_INIT;
            break;
        }
        m_spOutputFmtCtx = std::shared_ptr<AVFormatContext>(pstOutputFmtCtx, [](AVFormatContext *pstFmtCtx ) {
            avformat_close_input(&pstFmtCtx);
            });
        AVCodec* pstCodec = avcodec_find_decoder_by_name("h264_cuvid");
        if (nullptr==pstCodec)
        {
            std::cout << "find decoder by hevc_cuvid failed " << std::endl;
            lRet = FF_INIT;
            break;
        }
        AVStream* pstOutStream = avformat_new_stream(m_spOutputFmtCtx.get(), pstCodec);
        if (nullptr == pstOutStream)
        {
            std::cout << "new stream failed " << GetFFmpegErr2Str(lRet) << std::endl;
            lRet = FF_INIT;
            break;
        }
        lRet = avcodec_parameters_copy(pstOutStream->codecpar, m_spAvFormatCtx->streams[m_lVideoIdx]->codecpar);
        if (0>lRet)
        {
            std::cout << "copy parameters failed , msg : " << GetFFmpegErr2Str(lRet) << std::endl;
            lRet = FF_INIT;
            break;
        }
        lRet = avio_open(&m_spOutputFmtCtx->pb, strOutFileName.c_str(), AVIO_FLAG_WRITE);
        if (0>lRet)
        {
            std::cout << "avio open failed , msg : " << GetFFmpegErr2Str(lRet) << std::endl;
            lRet = FF_INIT;
            break;
        }
        lRet = avformat_write_header(m_spOutputFmtCtx.get(), 0);
        if (0 > lRet)
        {
            std::cout << "avformat write header failed , msg : " << GetFFmpegErr2Str(lRet) << std::endl;
            lRet = FF_INIT;
            break;
        }
    } while (0);

    return lRet;
}

LONG CMediaHandle::Run()
{
    LONG lRet = FF_SUCCESS;

    ULONG ulInterval = 1000000 * 1 / m_ulFrameRate;


    std::fstream f;
    f.open(".//h265", std::ios::out);

    do
    {
        av_usleep(ulInterval);
        std::shared_ptr<AVPacket> spPacket(av_packet_alloc(), [](AVPacket* pstAvPacket)
            {
                av_packet_unref(pstAvPacket);
            });
        lRet = av_read_frame(m_spAvFormatCtx.get(), spPacket.get());
        if (0 > lRet)
        {
            std::cout << "read frame : " << GetFFmpegErr2Str(lRet) << std::endl;
            break;
        }
        lRet = avcodec_send_packet(m_spAvCodecCtx.get(), spPacket.get());
        if (0 != lRet)
        {
            std::cout << "send apcket error , msg : " << GetFFmpegErr2Str(lRet) << std::endl;
            break;
        }
        std::shared_ptr<AVFrame> spFrame(av_frame_alloc(), [](AVFrame* pstFrame)
            {
                av_frame_unref(pstFrame);
            });
        lRet = avcodec_receive_frame(m_spAvCodecCtx.get(), spFrame.get());
        if (lRet == AVERROR(EAGAIN) || lRet == AVERROR_EOF)
        {
            continue;
        }
        else if (lRet < 0)
        {
            std::cout << "decode error , msg : " << GetFFmpegErr2Str(lRet) << std::endl;
            break;
        }
        std::cout << "resolution : " << "(" << spFrame->width << "*" << spFrame->height << ")" << std::endl;
        
        lRet = avcodec_send_frame(m_spAvEncodecCtx.get(), spFrame.get());
        if (0 != lRet)
        {
            std::cout << "send frame error , msg : " << GetFFmpegErr2Str(lRet) << std::endl;
            break;
        }

        std::shared_ptr<AVPacket> spEncPacket(av_packet_alloc(), [](AVPacket* pstAvPacket)
            {
                av_packet_unref(pstAvPacket);
            });
        lRet = avcodec_receive_packet(m_spAvEncodecCtx.get(), spEncPacket.get());
        if (lRet == AVERROR(EAGAIN) || lRet == AVERROR_EOF)
        {
            continue;
        }
        else if (lRet < 0)
        {
            std::cout << "encoder error , msg : " << GetFFmpegErr2Str(lRet) << std::endl;
            break;
        }
        std::cout << "enc packet size : " << spEncPacket->size << std::endl;
        av_packet_rescale_ts(spEncPacket.get(), m_spAvEncodecCtx->time_base,
            m_spOutputFmtCtx->streams[0]->time_base);
        lRet = av_interleaved_write_frame(m_spOutputFmtCtx.get(), spEncPacket.get());
        if (0!=lRet)
        {
            std::cout << "write frame failed , msg : " << GetFFmpegErr2Str(lRet) << std::endl;
        }

        f.write((const char*)spEncPacket->data, spEncPacket->size);
    } while (1);

    av_write_trailer(m_spOutputFmtCtx.get());

    f.close();

    return lRet;
}

std::string CMediaHandle::GetDecoderNameById(IN AVCodecID enCodecId)
{
    auto iter= m_CodecNameMap.find(enCodecId);
    if (m_CodecNameMap.end() == iter)
    {
        return "";
    }
    return m_CodecNameMap[enCodecId];
}

VOID CMediaHandle::CalculateVideoRate()
{
    AVStream* pstStream = m_spAvFormatCtx->streams[m_lVideoIdx];
    AVRational stRation = av_guess_frame_rate(m_spAvFormatCtx.get(), pstStream, nullptr);
    if (!(0 == stRation.num && 1 == stRation.den))
    {
        m_ulFrameRate = av_q2d(stRation);
    }
    std::cout << "fame rate : " << m_ulFrameRate << std::endl;
    return;
}

VOID CMediaHandle::CalculateDuration()
{
    AVStream* pstStream = m_spAvFormatCtx->streams[m_lVideoIdx];
    if (AV_NOPTS_VALUE == pstStream->duration)
    {
        int64_t filesize, duration;
        AVStream* st;

        if (m_spAvFormatCtx->bit_rate >= 0)
        {
            filesize = m_spAvFormatCtx->pb ? avio_size(m_spAvFormatCtx->pb) : 0;
            if (filesize > 0)
            {
                std::cout << "file size ：" << filesize << std::endl;
                for (ULONG i = 0; i < m_spAvFormatCtx->nb_streams; i++)
                {
                    st = m_spAvFormatCtx->streams[i];
                    duration = av_rescale(8 * filesize, st->time_base.den,
                        m_spAvFormatCtx->bit_rate * (int64_t)st->time_base.num); //通过文件大小除以文件平均码率得到文件时长，之所以还有time_base信息，
                    if (st->duration == AV_NOPTS_VALUE)
                    {
                        st->duration = duration;
                    }
                }
            }
        }
    }
    m_ulDuration = pstStream->duration * av_q2d(pstStream->time_base);

    std::cout << "duration : " << m_ulDuration << std::endl;
}

std::unordered_map<AVCodecID, std::string> CMediaHandle::m_CodecNameMap = {
    {AVCodecID::AV_CODEC_ID_H264,"h264_cuvid"},
    {AVCodecID::AV_CODEC_ID_HEVC,"hevc_cuvid"},
    {AVCodecID::AV_CODEC_ID_VP8,"vp8_cuvid"},
    {AVCodecID::AV_CODEC_ID_VP9,"vp9_cuvid"},
    {AVCodecID::AV_CODEC_ID_MPEG1VIDEO,"mpeg1_cuvid"},
    {AVCodecID::AV_CODEC_ID_MPEG2VIDEO,"mpeg2_cuvid"}
};

