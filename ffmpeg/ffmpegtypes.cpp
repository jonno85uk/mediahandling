/*
  Copyright (c) 2019, Jonathan Noble
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:
      * Redistributions of source code must retain the above copyright
        notice, this list of conditions and the following disclaimer.
      * Redistributions in binary form must reproduce the above copyright
        notice, this list of conditions and the following disclaimer in the
        documentation and/or other materials provided with the distribution.
      * Neither the name of the <organization> nor the
        names of its contributors may be used to endorse or promote products
        derived from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "ffmpegtypes.h"
#include <map>
#include "mediahandling.h"

namespace mh = media_handling;
namespace mft = media_handling::ffmpeg::types;

using media_handling::InterpolationMethod;

constexpr auto EMPTY_STR = "";

namespace
{
  const std::map<mh::SampleFormat, AVSampleFormat> SAMPLE_FORMAT_MAP
  {
    {mh::SampleFormat::NONE, AV_SAMPLE_FMT_NONE},
    {mh::SampleFormat::UNSIGNED_8, AV_SAMPLE_FMT_U8},
    {mh::SampleFormat::SIGNED_16, AV_SAMPLE_FMT_S16},
    {mh::SampleFormat::SIGNED_32, AV_SAMPLE_FMT_S32},
    {mh::SampleFormat::SIGNED_64, AV_SAMPLE_FMT_S64},
    {mh::SampleFormat::FLOAT, AV_SAMPLE_FMT_FLT},
    {mh::SampleFormat::DOUBLE, AV_SAMPLE_FMT_DBL},
    {mh::SampleFormat::UNSIGNED_8P, AV_SAMPLE_FMT_U8P},
    {mh::SampleFormat::SIGNED_16P, AV_SAMPLE_FMT_S16P},
    {mh::SampleFormat::SIGNED_32P, AV_SAMPLE_FMT_S32P},
    {mh::SampleFormat::SIGNED_64P, AV_SAMPLE_FMT_S64P},
    {mh::SampleFormat::FLOAT_P, AV_SAMPLE_FMT_FLTP},
    {mh::SampleFormat::DOUBLE_P, AV_SAMPLE_FMT_DBLP}
  };

  const std::map<mh::InterpolationMethod, int> INTERPOLATION_MAP
  {
    {mh::InterpolationMethod::BILINEAR, 0},
    {InterpolationMethod::BILINEAR, SWS_BILINEAR},
    {InterpolationMethod::BICUBLIN, SWS_BICUBLIN},
    {InterpolationMethod::BICUBIC, SWS_BICUBIC},
    {InterpolationMethod::LANCZOS, SWS_LANCZOS}
  };

  const std::map<mh::Codec, AVCodecID> CODEC_MAP
  {
    {mh::Codec::DNXHD, AV_CODEC_ID_DNXHD},
    {mh::Codec::DPX, AV_CODEC_ID_DPX},
    {mh::Codec::H264, AV_CODEC_ID_H264},
    {mh::Codec::JPEG2000, AV_CODEC_ID_JPEG2000},
    {mh::Codec::JPEG, AV_CODEC_ID_MJPEG},
    {mh::Codec::MPEG2_VIDEO, AV_CODEC_ID_MPEG2VIDEO},
    {mh::Codec::PNG, AV_CODEC_ID_PNG},
    {mh::Codec::RAW, AV_CODEC_ID_RAWVIDEO},
    {mh::Codec::TIFF, AV_CODEC_ID_TIFF},
    {mh::Codec::AAC, AV_CODEC_ID_AAC},
    {mh::Codec::AC3, AV_CODEC_ID_AC3},
    {mh::Codec::ALAC, AV_CODEC_ID_ALAC},
    {mh::Codec::FLAC, AV_CODEC_ID_FLAC},
    {mh::Codec::MP3, AV_CODEC_ID_MP3},
    {mh::Codec::PCM_S16_LE, AV_CODEC_ID_PCM_S16LE},
    {mh::Codec::PCM_S24_LE, AV_CODEC_ID_PCM_S24LE},
    {mh::Codec::VORBIS, AV_CODEC_ID_VORBIS},
    {mh::Codec::WAV, AV_CODEC_ID_WAVPACK}
  };

  const std::map<mh::ColourPrimaries, AVColorPrimaries> PRIMARIES_MAP
  {
    {mh::ColourPrimaries::BT_709, AVCOL_PRI_BT709},
    {mh::ColourPrimaries::BT_470M, AVCOL_PRI_BT470M},
    {mh::ColourPrimaries::BT_709, AVCOL_PRI_BT709},
    {mh::ColourPrimaries::BT_601, AVCOL_PRI_SMPTE170M},
    {mh::ColourPrimaries::BT_2020, AVCOL_PRI_BT2020},
    {mh::ColourPrimaries::BT470BG, AVCOL_PRI_BT470BG},
    {mh::ColourPrimaries::SMPTE_240M, AVCOL_PRI_SMPTE240M},
    {mh::ColourPrimaries::SMPTE_428, AVCOL_PRI_SMPTE428}
  };

  const std::map<mh::TransferCharacteristics, AVColorTransferCharacteristic> TRANSFERS_MAP
  {
    {mh::TransferCharacteristics::BT_709, AVCOL_TRC_BT709},
    {mh::TransferCharacteristics::BT_470M, AVCOL_TRC_GAMMA22},
    {mh::TransferCharacteristics::BT_470BG, AVCOL_TRC_GAMMA28},
    {mh::TransferCharacteristics::BT_601, AVCOL_TRC_SMPTE170M},
    {mh::TransferCharacteristics::SMPTE_240M, AVCOL_TRC_SMPTE240M},
    {mh::TransferCharacteristics::LINEAR, AVCOL_TRC_LINEAR},
    {mh::TransferCharacteristics::IEC_61966_2_4, AVCOL_TRC_IEC61966_2_4},
    {mh::TransferCharacteristics::BT_1361, AVCOL_TRC_BT1361_ECG},
    {mh::TransferCharacteristics::IEC_61966_2_1, AVCOL_TRC_IEC61966_2_1},
    {mh::TransferCharacteristics::BT_2020_10, AVCOL_TRC_BT2020_10},
    {mh::TransferCharacteristics::BT_2020_12, AVCOL_TRC_BT2020_12},
    {mh::TransferCharacteristics::SMPTE_2084, AVCOL_TRC_SMPTE2084},
    {mh::TransferCharacteristics::SMPTE_428, AVCOL_TRC_SMPTE428},
    {mh::TransferCharacteristics::ARIB_STD_B67, AVCOL_TRC_ARIB_STD_B67}
  };

  const std::map<mh::MatrixCoefficients, AVColorSpace> MATRIX_MAP
  {
    {mh::MatrixCoefficients::IEC_61966_2_1, AVCOL_SPC_RGB},
    {mh::MatrixCoefficients::BT_709, AVCOL_SPC_BT709},
    {mh::MatrixCoefficients::FCC, AVCOL_SPC_FCC},
    {mh::MatrixCoefficients::BT_470BG, AVCOL_SPC_BT470BG},
    {mh::MatrixCoefficients::BT_601_6, AVCOL_SPC_SMPTE170M},
    {mh::MatrixCoefficients::SMPTE_240M, AVCOL_SPC_SMPTE240M},
    {mh::MatrixCoefficients::BT_2020_NCL, AVCOL_SPC_BT2020_NCL},
    {mh::MatrixCoefficients::BT_2020_CL, AVCOL_SPC_BT2020_CL},
    {mh::MatrixCoefficients::SMPTE_2085, AVCOL_SPC_SMPTE2085},
    {mh::MatrixCoefficients::BT_2100_0, AVCOL_SPC_ICTCP}
  };

  const std::map<mh::ColourRange, AVColorRange> COLOUR_RANGE_MAP
  {
    {mh::ColourRange::FULL, AVCOL_RANGE_JPEG},
    {mh::ColourRange::TV, AVCOL_RANGE_MPEG}
  };

  const std::map<mh::ChannelLayout, uint64_t> AUDIO_CHANNEL_MAP
  {
    {mh::ChannelLayout::MONO, AV_CH_LAYOUT_MONO},
    {mh::ChannelLayout::STEREO, AV_CH_LAYOUT_STEREO},
    {mh::ChannelLayout::STEREO_LFE, AV_CH_LAYOUT_2POINT1},
    {mh::ChannelLayout::THREE_STEREO, AV_CH_LAYOUT_SURROUND},
    {mh::ChannelLayout::THREE_SURROUND, AV_CH_LAYOUT_2_1},
    {mh::ChannelLayout::THREE_SURROUND_LFE, AV_CH_LAYOUT_3POINT1},
    {mh::ChannelLayout::FOUR_STEREO, AV_CH_LAYOUT_QUAD},
    {mh::ChannelLayout::FOUR_SURROUND, AV_CH_LAYOUT_4POINT0},
    {mh::ChannelLayout::FOUR_SURROUND_LFE, AV_CH_LAYOUT_MONO},
    {mh::ChannelLayout::FIVE, AV_CH_LAYOUT_5POINT0},
    {mh::ChannelLayout::FIVE_STEREO, AV_CH_LAYOUT_5POINT0_BACK},
    {mh::ChannelLayout::FIVE_LFE, AV_CH_LAYOUT_5POINT1},
    {mh::ChannelLayout::FIVE_STEREO_LFE, AV_CH_LAYOUT_5POINT1_BACK},
    {mh::ChannelLayout::SIX, AV_CH_LAYOUT_6POINT0},
    {mh::ChannelLayout::SIX_LFE, AV_CH_LAYOUT_6POINT1},
    {mh::ChannelLayout::SEVEN, AV_CH_LAYOUT_7POINT0},
    {mh::ChannelLayout::SEVEN_LFE, AV_CH_LAYOUT_7POINT1}
  };

  const std::map<mh::Profile, int> PROFILE_MAP
  {
    {mh::Profile::H264_BASELINE,FF_PROFILE_H264_BASELINE},
    {mh::Profile::H264_MAIN,  FF_PROFILE_H264_MAIN},
    {mh::Profile::H264_HIGH, FF_PROFILE_H264_HIGH},
    {mh::Profile::H264_HIGH10, FF_PROFILE_H264_HIGH_10},
    {mh::Profile::H264_HIGH422, FF_PROFILE_H264_HIGH_422},
    {mh::Profile::H264_HIGH444, FF_PROFILE_H264_HIGH_444},
    {mh::Profile::MPEG2_SIMPLE, FF_PROFILE_MPEG2_SIMPLE},
    {mh::Profile::MPEG2_MAIN, FF_PROFILE_MPEG2_MAIN},
    {mh::Profile::MPEG2_HIGH, FF_PROFILE_MPEG2_HIGH},
    {mh::Profile::MPEG2_422, FF_PROFILE_MPEG2_422},
    {mh::Profile::DNXHD, FF_PROFILE_DNXHD},
    {mh::Profile::DNXHR_LB, FF_PROFILE_DNXHR_LB},
    {mh::Profile::DNXHR_SQ, FF_PROFILE_DNXHR_SQ},
    {mh::Profile::DNXHR_HQ, FF_PROFILE_DNXHR_HQ},
    {mh::Profile::DNXHR_HQX, FF_PROFILE_DNXHR_HQX},
    {mh::Profile::DNXHR_444, FF_PROFILE_DNXHR_444},
  };

  const std::map<mh::Preset, std::string_view> PRESET_MAP
  {
    {mh::Preset::X264_VERYSLOW, "veryslow"},
    {mh::Preset::X264_SLOWER, "slower"},
    {mh::Preset::X264_SLOW, "slow"},
    {mh::Preset::X264_MEDIUM, "medium"},
    {mh::Preset::X264_FAST, "fast"},
    {mh::Preset::X264_FASTER, "faster"},
    {mh::Preset::X264_VERYFAST, "veryfast"},
    {mh::Preset::X264_SUPERFAST, "superfast"},
    {mh::Preset::X264_ULTRAFAST, "ultrafast"}
  };

  const std::map<mh::PixelFormat, AVPixelFormat> PIX_FMT_MAP
  {
    {mh::PixelFormat::RGBA, AV_PIX_FMT_RGBA},
    {mh::PixelFormat::RGB24, AV_PIX_FMT_RGB24},
    {mh::PixelFormat::RGB_48_LE, AV_PIX_FMT_RGB48LE},
    {mh::PixelFormat::YUV420, AV_PIX_FMT_YUV420P},
    {mh::PixelFormat::YUVJ420, AV_PIX_FMT_YUVJ420P},
    {mh::PixelFormat::YUV422, AV_PIX_FMT_YUV422P},
    {mh::PixelFormat::YUV444, AV_PIX_FMT_YUV444P},
    {mh::PixelFormat::YUV420_P_10_LE, AV_PIX_FMT_YUV420P10LE},
    {mh::PixelFormat::YUV422_P_10_LE, AV_PIX_FMT_YUV422P10LE},
    {mh::PixelFormat::YUV444_P_12_LE, AV_PIX_FMT_YUV444P12LE},
    {mh::PixelFormat::YUV444_P_16_LE, AV_PIX_FMT_YUV444P16LE}
  };

  const std::map<mh::PictureType, AVPictureType> PICTURE_TYPE_MAP
  {
      {mh::PictureType::UNDEFINED, AV_PICTURE_TYPE_NONE},
      {mh::PictureType::INTRA, AV_PICTURE_TYPE_I},
      {mh::PictureType::PREDICTED, AV_PICTURE_TYPE_P},
      {mh::PictureType::BIDIRECTIONAL_INTRA, AV_PICTURE_TYPE_BI},
      {mh::PictureType::BIDIRECTIONAL_PREDICTED, AV_PICTURE_TYPE_B},
      {mh::PictureType::SWITCHING_INTRA, AV_PICTURE_TYPE_SI},
      {mh::PictureType::SWITCHING_PREDICTED, AV_PICTURE_TYPE_SP},
      {mh::PictureType::S_VIDEO_OBJECT_PLANE, AV_PICTURE_TYPE_S}
  };
}

template <typename T_K, typename T_V>
T_K convertFromFFMpegType(const T_V ff_key, const std::map<T_K, T_V>& mapping, const T_K default_value) noexcept
{
  try {
    for (const auto& [key, value] : mapping) {
      if (value == ff_key) {
        return key;
      }
    }
  } catch (const std::exception& ex) {
    media_handling::logMessage(media_handling::LogType::WARNING,
                               fmt::format("convertFromFFMpegType() -- Caught an exception, ex={}", ex.what()));
  } catch (...) {
    media_handling::logMessage(media_handling::LogType::WARNING,
                               fmt::format("convertFromFFMpegType() -- Caught an unknown exception, ex={}"));
  }
  return default_value;
}

template <typename T_K, typename T_V>
T_V convertToFFMpegType(const T_K mh_key, const std::map<T_K, T_V>& mapping, const T_V default_value) noexcept
{
  try {
    if (mapping.count(mh_key) == 1) {
      return mapping.at(mh_key);
    }
  } catch (const std::exception& ex) {
    media_handling::logMessage(media_handling::LogType::WARNING,
                               fmt::format("convertToFFMpegType() -- Caught an exception, ex={}", ex.what()));
  } catch (...) {
    media_handling::logMessage(media_handling::LogType::WARNING,
                               fmt::format("convertToFFMpegType() -- Caught an unknown exception, ex={}"));
  }
  return default_value;
}

void mft::avFormatContextDeleter(AVFormatContext* context)
{
  avformat_free_context(context);
}

void mft::avPacketDeleter(AVPacket* packet)
{
  av_packet_free(&packet);
}

void mft::avframeDeleter(AVFrame* frame)
{
  av_frame_free(&frame);
}

void mft::swsContextDeleter(SwsContext* context)
{
  sws_freeContext(context);
}

void mft::swrContextDeleter(SwrContext* context)
{
  swr_free(&context);
}

void mft::avCodecContextDeleter(AVCodecContext* context)
{
  avcodec_close(context);
  avcodec_free_context(&context);
}

void mft::avCodecDeleter(AVCodec* codec)
{
  // None
}

void mft::avStreamDeleter(AVStream* stream)
{
  // None. Should be freed by avformat
}

mh::ColourPrimaries mft::convertColourPrimary(const AVColorPrimaries primary) noexcept
{
  return convertFromFFMpegType(primary, PRIMARIES_MAP, mh::ColourPrimaries::UNKNOWN);
}

mh::TransferCharacteristics mft::convertTransferCharacteristics(const AVColorTransferCharacteristic transfer) noexcept
{
  return convertFromFFMpegType(transfer, TRANSFERS_MAP, TransferCharacteristics::UNKNOWN);
}


mh::MatrixCoefficients mft::convertMatrixCoefficients(const AVColorSpace matrix) noexcept
{
  return convertFromFFMpegType(matrix, MATRIX_MAP, MatrixCoefficients::UNKNOWN);
}


mh::ColourRange mft::convertColourRange(const AVColorRange range) noexcept
{
  return convertFromFFMpegType(range, COLOUR_RANGE_MAP, ColourRange::UNKNOWN);
}

int mft::convertInterpolationMethod(const InterpolationMethod interpolation) noexcept
{
  return convertToFFMpegType(interpolation, INTERPOLATION_MAP, 0);
}

AVPixelFormat mft::convertPixelFormat(const media_handling::PixelFormat format) noexcept
{
  return convertToFFMpegType(format, PIX_FMT_MAP, AV_PIX_FMT_NONE);
}


mh::PixelFormat mft::convertPixelFormat(const AVPixelFormat format) noexcept
{
  return convertFromFFMpegType(format, PIX_FMT_MAP, PixelFormat::UNKNOWN);
}

mh::SampleFormat mft::convertSampleFormat(const AVSampleFormat format) noexcept
{
  return convertFromFFMpegType(format, SAMPLE_FORMAT_MAP, SampleFormat::NONE);
}


AVSampleFormat mft::convertSampleFormat(const media_handling::SampleFormat format) noexcept
{
  return convertToFFMpegType(format, SAMPLE_FORMAT_MAP, AV_SAMPLE_FMT_NONE);
}


mh::ChannelLayout mft::convertChannelLayout(const uint64_t layout) noexcept
{
  return convertFromFFMpegType(layout, AUDIO_CHANNEL_MAP, ChannelLayout::MONO);
}


uint64_t mft::convertChannelLayout(const ChannelLayout layout) noexcept
{
  return convertToFFMpegType(layout, AUDIO_CHANNEL_MAP, static_cast<uint64_t>(0));
}


mh::Codec mft::convertCodecID(const AVCodecID id) noexcept
{
  return convertFromFFMpegType(id, CODEC_MAP, Codec::UNKNOWN);
}


AVCodecID mft::convertCodecID(const Codec id) noexcept
{
  return convertToFFMpegType(id, CODEC_MAP, AV_CODEC_ID_NONE);
}


int mft::convertProfile(const Profile prof) noexcept
{
  return convertToFFMpegType(prof, PROFILE_MAP, FF_PROFILE_UNKNOWN);
}


mh::Profile mft::convertProfile(const int prof) noexcept
{
  return convertFromFFMpegType(prof, PROFILE_MAP, Profile::UNKNOWN);
}

std::string_view mft::convertPreset(const Preset pre) noexcept
{
  return convertToFFMpegType(pre, PRESET_MAP, std::string_view(EMPTY_STR));
}


AVPictureType mft::convertPictureType(const PictureType ptype) noexcept
{
  return convertToFFMpegType(ptype, PICTURE_TYPE_MAP, AV_PICTURE_TYPE_NONE);
}

mh::PictureType mft::convertPictureType(const AVPictureType ptype) noexcept
{
  return convertFromFFMpegType(ptype, PICTURE_TYPE_MAP, mh::PictureType::UNDEFINED);
}

