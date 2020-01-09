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

namespace mh = media_handling;

using media_handling::InterpolationMethod;


void media_handling::types::avFormatContextDeleter(AVFormatContext* context)
{
  avformat_free_context(context);
}

void media_handling::types::avPacketDeleter(AVPacket* packet)
{
  av_packet_free(&packet);
}

void media_handling::types::avframeDeleter(AVFrame* frame)
{
  av_frame_free(&frame);
}

void media_handling::types::swsContextDeleter(SwsContext* context)
{
  sws_freeContext(context);
}

void media_handling::types::swrContextDeleter(SwrContext* context)
{
  swr_free(&context);
}


media_handling::ColourPrimaries media_handling::types::convertColourPrimary(const AVColorPrimaries primary) noexcept
{
  media_handling::ColourPrimaries val;
  switch (primary) {
    case AVCOL_PRI_BT709:
      val = ColourPrimaries::BT_709;
      break;
    case AVCOL_PRI_BT470M:
      val = ColourPrimaries::BT_470M;
      break;
    case AVCOL_PRI_BT470BG:
      [[fallthrough]];
    case AVCOL_PRI_SMPTE170M:
      val = ColourPrimaries::BT_601;
      break;
    case AVCOL_PRI_BT2020:
      val = ColourPrimaries::BT_2020;
      break;
    case AVCOL_PRI_SMPTE240M:
      val = ColourPrimaries::SMPTE_240M;
      break;
    case AVCOL_PRI_SMPTE428:
      val = ColourPrimaries::SMPTE_428;
      break;
    default:
      val = ColourPrimaries::UNKNOWN;
      break;
  }
  return val;
}

mh::TransferCharacteristics mh::types::convertTransferCharacteristics(const AVColorTransferCharacteristic transfer) noexcept
{
  switch (transfer) {
    case AVCOL_TRC_BT709:
      return TransferCharacteristics::BT_709;
    case AVCOL_TRC_GAMMA22:
      return TransferCharacteristics::BT_470M;
    case AVCOL_TRC_GAMMA28:
      return TransferCharacteristics::BT_470BG;
    case AVCOL_TRC_SMPTE170M:
      return TransferCharacteristics::BT_601;
    case AVCOL_TRC_SMPTE240M:
      return TransferCharacteristics::SMPTE_240M;
    case AVCOL_TRC_LINEAR:
      return TransferCharacteristics::LINEAR;
      //    AVCOL_TRC_LOG          = 9,  ///< "Logarithmic transfer characteristic (100:1 range)"
      //    AVCOL_TRC_LOG_SQRT     = 10, ///< "Logarithmic transfer characteristic (100 * Sqrt(10) : 1 range)"
    case AVCOL_TRC_IEC61966_2_4:
      return TransferCharacteristics::IEC_61966_2_4;
    case AVCOL_TRC_BT1361_ECG:
      return TransferCharacteristics::BT_1361;
    case AVCOL_TRC_IEC61966_2_1:
      return TransferCharacteristics::IEC_61966_2_1;
    case AVCOL_TRC_BT2020_10:
      return TransferCharacteristics::BT_2020_10;
    case AVCOL_TRC_BT2020_12:
      return TransferCharacteristics::BT_2020_12;
    case AVCOL_TRC_SMPTE2084:
      return TransferCharacteristics::SMPTE_2084;
    case AVCOL_TRC_SMPTE428:
      return TransferCharacteristics::SMPTE_428;
    case AVCOL_TRC_ARIB_STD_B67:
      return TransferCharacteristics::ARIB_STD_B67;
    default:
      return TransferCharacteristics::UNKNOWN;
  }
}


mh::MatrixCoefficients mh::types::convertMatrixCoefficients(const AVColorSpace matrix) noexcept
{
  switch (matrix) {
    case AVCOL_SPC_RGB:
      return MatrixCoefficients::IEC_61966_2_1;
    case AVCOL_SPC_BT709:
      return MatrixCoefficients::BT_709;
    case AVCOL_SPC_FCC:
      return MatrixCoefficients::FCC;
    case AVCOL_SPC_BT470BG:
      return MatrixCoefficients::BT_470BG;
    case AVCOL_SPC_SMPTE170M:
      return MatrixCoefficients::BT_601_6;
    case AVCOL_SPC_SMPTE240M:
      return MatrixCoefficients::SMPTE_240M;
//  AVCOL_SPC_YCGCO       = 8,  ///< Used by Dirac / VC-2 and H.264 FRext, see ITU-T SG16
    case AVCOL_SPC_BT2020_NCL:
      return MatrixCoefficients::BT_2020_NCL;
    case AVCOL_SPC_BT2020_CL:
      return MatrixCoefficients::BT_2020_CL;
    case AVCOL_SPC_SMPTE2085:
      return MatrixCoefficients::SMPTE_2085;
//  AVCOL_SPC_CHROMA_DERIVED_NCL = 12, ///< Chromaticity-derived non-constant luminance system
//  AVCOL_SPC_CHROMA_DERIVED_CL = 13, ///< Chromaticity-derived constant luminance system
    case AVCOL_SPC_ICTCP:
      return MatrixCoefficients::BT_2100_0;
    default:
      return MatrixCoefficients::UNKNOWN;
  }
}


mh::ColourRange mh::types::convertColourRange(const AVColorRange range) noexcept
{
  switch (range) {
    case AVCOL_RANGE_MPEG:
      return ColourRange::TV;
    case AVCOL_RANGE_JPEG:
      return ColourRange::FULL;
    default:
      return ColourRange::UNKNOWN;
  }
}

int media_handling::types::convertInterpolationMethod(const InterpolationMethod interpolation) noexcept
{
  int av_method;
  switch (interpolation) {
    case InterpolationMethod::NEAREST:
      [[fallthrough]];
    default:
      av_method = 0;
      break;
    case InterpolationMethod::BILINEAR:
      av_method = SWS_BILINEAR;
      break;
    case InterpolationMethod::BICUBLIN:
      av_method = SWS_BICUBLIN;
      break;
    case InterpolationMethod::BICUBIC:
      av_method = SWS_BICUBIC;
      break;
    case InterpolationMethod::LANCZOS:
      av_method = SWS_LANCZOS;
      break;
  }
  return av_method;
}

AVPixelFormat media_handling::types::convertPixelFormat(const media_handling::PixelFormat format) noexcept
{
  AVPixelFormat converted {AV_PIX_FMT_NONE};

  switch (format) {
    case PixelFormat::RGBA:
      converted = AV_PIX_FMT_RGBA;
      break;
    case PixelFormat::RGB24:
      converted = AV_PIX_FMT_RGB24;
      break;
    case PixelFormat::YUV420:
      converted = AV_PIX_FMT_YUV420P;
      break;
    case PixelFormat::YUVJ420:
      converted = AV_PIX_FMT_YUVJ420P;
      break;
    case PixelFormat::YUV422:
      converted = AV_PIX_FMT_YUV422P;
      break;
    case PixelFormat::YUV444:
      converted = AV_PIX_FMT_YUV444P;
      break;
    case PixelFormat::YUV422_P_10_LE:
      converted = AV_PIX_FMT_YUV422P10LE;
      break;
    case PixelFormat::YUV444_P_12_LE:
      converted = AV_PIX_FMT_YUV444P12LE;
      break;
    default:
      converted = AV_PIX_FMT_NONE;
      break;
  }

  return converted;
}


media_handling::PixelFormat media_handling::types::convertPixelFormat(const AVPixelFormat format) noexcept
{
  PixelFormat converted {PixelFormat::UNKNOWN};
  switch (format) {
    case AV_PIX_FMT_RGB24:
      converted = PixelFormat::RGB24;
      break;
    case AV_PIX_FMT_RGBA:
      converted = PixelFormat::RGBA;
      break;
    case AV_PIX_FMT_YUV420P:
      converted = PixelFormat::YUV420;
      break;
    case AV_PIX_FMT_YUVJ420P:
      converted = PixelFormat::YUVJ420;
      break;
    case AV_PIX_FMT_YUV422P:
      converted = PixelFormat::YUV422;
      break;
    case AV_PIX_FMT_YUV444P:
      converted = PixelFormat::YUV444;
      break;
    case AV_PIX_FMT_YUV422P10LE:
      converted = PixelFormat::YUV422_P_10_LE;
      break;
    case AV_PIX_FMT_YUV444P12LE:
      converted = PixelFormat::YUV444_P_12_LE;
      break;
    default:
      converted = PixelFormat::UNKNOWN;
      break;
  }
  return converted;
}


media_handling::SampleFormat media_handling::types::convertSampleFormat(const AVSampleFormat format) noexcept
{
  SampleFormat converted {SampleFormat::NONE};

  switch (format) {
    case AV_SAMPLE_FMT_NONE:
      [[fallthrough]];
    case AV_SAMPLE_FMT_NB:
      converted = SampleFormat::NONE;
      break;
    case AV_SAMPLE_FMT_U8:
      converted = SampleFormat::UNSIGNED_8;
      break;
    case AV_SAMPLE_FMT_S16:
      converted = SampleFormat::SIGNED_16;
      break;
    case AV_SAMPLE_FMT_S32:
      converted = SampleFormat::SIGNED_32;
      break;
    case AV_SAMPLE_FMT_S64:
      converted = SampleFormat::SIGNED_64;
      break;
    case AV_SAMPLE_FMT_FLT:
      converted = SampleFormat::FLOAT;
      break;
    case AV_SAMPLE_FMT_DBL:
      converted = SampleFormat::DOUBLE;
      break;
    case AV_SAMPLE_FMT_U8P:
      converted = SampleFormat::UNSIGNED_8P;
      break;
    case AV_SAMPLE_FMT_S16P:
      converted = SampleFormat::SIGNED_16P;
      break;
    case AV_SAMPLE_FMT_S32P:
      converted = SampleFormat::SIGNED_32P;
      break;
    case AV_SAMPLE_FMT_S64P:
      converted = SampleFormat::SIGNED_64P;
      break;
    case AV_SAMPLE_FMT_FLTP:
      converted = SampleFormat::FLOAT_P;
      break;
    case AV_SAMPLE_FMT_DBLP:
      converted = SampleFormat::DOUBLE_P;
      break;
  }

  return converted;
}


AVSampleFormat media_handling::types::convertSampleFormat(const media_handling::SampleFormat format) noexcept
{
  switch (format)
  {
    case SampleFormat::UNSIGNED_8:
      return AV_SAMPLE_FMT_U8;
    case SampleFormat::SIGNED_16:
      return AV_SAMPLE_FMT_S16;
    case SampleFormat::SIGNED_32:
      return AV_SAMPLE_FMT_S32;
    case SampleFormat::SIGNED_64:
      return AV_SAMPLE_FMT_S64;
    case SampleFormat::FLOAT:
      return AV_SAMPLE_FMT_FLT;
    case SampleFormat::DOUBLE:
      return AV_SAMPLE_FMT_DBL;
    case SampleFormat::UNSIGNED_8P:
      return AV_SAMPLE_FMT_U8P;
    case SampleFormat::SIGNED_16P:
      return AV_SAMPLE_FMT_S16P;
    case SampleFormat::SIGNED_32P:
      return AV_SAMPLE_FMT_S32P;
    case SampleFormat::FLOAT_P:
      return AV_SAMPLE_FMT_FLTP;
    case SampleFormat::DOUBLE_P:
      return AV_SAMPLE_FMT_DBLP;
    case SampleFormat::SIGNED_64P:
      return AV_SAMPLE_FMT_S64P;
    case SampleFormat::NONE:
      [[fallthrough]];
    default:
      return AV_SAMPLE_FMT_NONE;
  }
}


media_handling::ChannelLayout media_handling::types::convertChannelLayout(const uint64_t layout) noexcept
{
  media_handling::ChannelLayout conv_layout = ChannelLayout::UNKNOWN;


  switch (layout)
  {
    case AV_CH_LAYOUT_MONO:
      conv_layout = ChannelLayout::MONO;
      break;
    case AV_CH_LAYOUT_STEREO:
      conv_layout = ChannelLayout::STEREO;
      break;
    case AV_CH_LAYOUT_2POINT1:
      conv_layout = ChannelLayout::STEREO_LFE;
      break;
    case AV_CH_LAYOUT_2_1:
      conv_layout = ChannelLayout::THREE_SURROUND;
      break;
    case AV_CH_LAYOUT_SURROUND:
      conv_layout = ChannelLayout::THREE_STEREO;
      break;
    case AV_CH_LAYOUT_3POINT1:
      conv_layout = ChannelLayout::THREE_SURROUND_LFE;
      break;
    case AV_CH_LAYOUT_4POINT0:
      conv_layout = ChannelLayout::FOUR_SURROUND;
      break;
    case AV_CH_LAYOUT_QUAD:
      conv_layout = ChannelLayout::FOUR_STEREO;
      break;
    case AV_CH_LAYOUT_4POINT1:
      conv_layout = ChannelLayout::FOUR_SURROUND_LFE;
      break;
    case AV_CH_LAYOUT_5POINT0:
      conv_layout = ChannelLayout::FIVE;
      break;
    case AV_CH_LAYOUT_5POINT0_BACK:
      conv_layout = ChannelLayout::FIVE_STEREO;
      break;
    case AV_CH_LAYOUT_5POINT1:
      conv_layout = ChannelLayout::FIVE_LFE;
      break;
    case AV_CH_LAYOUT_5POINT1_BACK:
      conv_layout = ChannelLayout::FIVE_STEREO_LFE;
      break;
    case AV_CH_LAYOUT_6POINT0:
      conv_layout = ChannelLayout::SIX;
      break;
    case AV_CH_LAYOUT_6POINT1:
      conv_layout = ChannelLayout::SIX_LFE;
      break;
    case AV_CH_LAYOUT_7POINT0:
      conv_layout = ChannelLayout::SEVEN;
      break;
    case AV_CH_LAYOUT_7POINT1:
      conv_layout = ChannelLayout::SEVEN_LFE;
      break;
  }

  return conv_layout;
}


uint64_t media_handling::types::convertChannelLayout(const ChannelLayout layout) noexcept
{
  switch (layout)
  {
    case ChannelLayout::MONO:
      return AV_CH_LAYOUT_MONO;
    case ChannelLayout::STEREO:
      return AV_CH_LAYOUT_STEREO;
    case ChannelLayout::STEREO_LFE:
      return AV_CH_LAYOUT_2POINT1;
    case ChannelLayout::THREE_STEREO:
      return AV_CH_LAYOUT_SURROUND;
    case ChannelLayout::THREE_SURROUND:
      return AV_CH_LAYOUT_2_1;
    case ChannelLayout::THREE_SURROUND_LFE:
      return AV_CH_LAYOUT_3POINT1;
    case ChannelLayout::FOUR_STEREO:
      return AV_CH_LAYOUT_QUAD;
    case ChannelLayout::FOUR_SURROUND:
      return AV_CH_LAYOUT_4POINT0;
    case ChannelLayout::FOUR_SURROUND_LFE:
      return AV_CH_LAYOUT_4POINT1;
    case ChannelLayout::FIVE:
      return AV_CH_LAYOUT_5POINT0;
    case ChannelLayout::FIVE_STEREO:
      return AV_CH_LAYOUT_5POINT0_BACK;
    case ChannelLayout::FIVE_LFE:
      return AV_CH_LAYOUT_5POINT1;
    case ChannelLayout::FIVE_STEREO_LFE:
      return AV_CH_LAYOUT_5POINT1_BACK;
    case ChannelLayout::SIX:
      return AV_CH_LAYOUT_6POINT0;
    case ChannelLayout::SIX_LFE:
      return AV_CH_LAYOUT_6POINT1;
    case ChannelLayout::SEVEN:
      return AV_CH_LAYOUT_7POINT0;
    case ChannelLayout::SEVEN_LFE:
      return AV_CH_LAYOUT_7POINT1;
    case ChannelLayout::THREE_STEREO_LFE:
      [[fallthrough]];
    case ChannelLayout::UNKNOWN:
      [[fallthrough]];
    default:
      return 0;
  }
}


media_handling::Codec media_handling::types::convertCodecID(const AVCodecID id) noexcept
{
  media_handling::Codec cdc = media_handling::Codec::UNKNOWN;

  switch (id)
  {
    case AV_CODEC_ID_AAC:
      cdc = media_handling::Codec::AAC;
      break;
    case AV_CODEC_ID_DNXHD:
      cdc = media_handling::Codec::DNXHD;
      break;
    case AV_CODEC_ID_DPX:
      cdc = media_handling::Codec::DPX;
      break;
    case AV_CODEC_ID_H264:
      cdc = media_handling::Codec::H264;
      break;
    case AV_CODEC_ID_JPEG2000:
      cdc = media_handling::Codec::JPEG2000;
      break;
    case AV_CODEC_ID_MJPEG:
      cdc = media_handling::Codec::JPEG;
      break;
    case AV_CODEC_ID_MPEG2VIDEO:
      cdc = media_handling::Codec::MPEG2_VIDEO;
      break;
    case AV_CODEC_ID_MPEG4:
      cdc = media_handling::Codec::MPEG4;
      break;
    case AV_CODEC_ID_PNG:
      cdc = media_handling::Codec::PNG;
      break;
    case AV_CODEC_ID_RAWVIDEO:
      cdc = media_handling::Codec::RAW;
      break;
    case AV_CODEC_ID_TIFF:
      cdc = media_handling::Codec::TIFF;
      break;
    default:
      cdc = media_handling::Codec::UNKNOWN;
      break;
  }

  return cdc;
}

media_handling::SampleFormat media_handling::types::convert(enum AVSampleFormat av_format) noexcept
{
  SampleFormat format = SampleFormat::NONE;
  switch (av_format) {
    case AV_SAMPLE_FMT_NONE:
      [[fallthrough]];
    case AV_SAMPLE_FMT_NB:
      [[fallthrough]];
    default:
      format = SampleFormat::NONE;
      break;
    case AV_SAMPLE_FMT_U8:
      format = SampleFormat::UNSIGNED_8;
      break;
    case AV_SAMPLE_FMT_S16:
      format = SampleFormat::SIGNED_16;
      break;
    case AV_SAMPLE_FMT_S32:
      format = SampleFormat::SIGNED_32;
      break;
    case AV_SAMPLE_FMT_FLT:
      format = SampleFormat::FLOAT;
      break;
    case AV_SAMPLE_FMT_DBL:
      format = SampleFormat::DOUBLE;
      break;
    case AV_SAMPLE_FMT_U8P:
      format = SampleFormat::UNSIGNED_8P;
      break;
    case AV_SAMPLE_FMT_S16P:
      format = SampleFormat::SIGNED_16P;
      break;
    case AV_SAMPLE_FMT_S32P:
      format = SampleFormat::SIGNED_32P;
      break;
    case AV_SAMPLE_FMT_FLTP:
      format = SampleFormat::FLOAT_P;
      break;
    case AV_SAMPLE_FMT_DBLP:
      format = SampleFormat::DOUBLE_P;
      break;
    case AV_SAMPLE_FMT_S64:
      format = SampleFormat::SIGNED_64;
      break;
    case AV_SAMPLE_FMT_S64P:
      format = SampleFormat::SIGNED_64P;
      break;
  }
  return format;
}
