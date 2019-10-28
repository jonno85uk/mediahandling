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


AVPixelFormat media_handling::types::convertPixelFormat(const media_handling::PixelFormat format) noexcept
{
  AVPixelFormat converted {AV_PIX_FMT_NONE};

  switch (format) {
    case PixelFormat::RGB24:
      converted = AV_PIX_FMT_RGB24;
      break;
    case PixelFormat::YUV420:
      converted = AV_PIX_FMT_YUV420P;
      break;
    case PixelFormat::YUV422:
      converted = AV_PIX_FMT_YUV422P;
      break;
    case PixelFormat::YUV444:
      converted = AV_PIX_FMT_YUV444P;
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
    case AV_PIX_FMT_YUV420P:
      converted = PixelFormat::YUV420;
      break;
    case AV_PIX_FMT_YUV422P:
      converted = PixelFormat::YUV422;
      break;
    case AV_PIX_FMT_YUV444P:
      converted = PixelFormat::YUV444;
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
    case AV_CODEC_ID_H264:
      cdc = media_handling::Codec::H264;
      break;
    case AV_CODEC_ID_MPEG2VIDEO:
      cdc = media_handling::Codec::MPEG2_VIDEO;
      break;
    case AV_CODEC_ID_MPEG4:
      cdc = media_handling::Codec::MPEG4;
      break;
    case AV_CODEC_ID_RAWVIDEO:
      cdc = media_handling::Codec::RAW;
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