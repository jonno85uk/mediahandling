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

#ifndef FFMPEGTYPES_H
#define FFMPEGTYPES_H

#include <memory>

#include "types.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#include <libavformat/avformat.h>
}

namespace media_handling::types
{
  void avFormatContextDeleter(AVFormatContext* context);
  void avPacketDeleter(AVPacket* packet);
  void avframeDeleter(AVFrame* frame);
  void swsContextDeleter(SwsContext* context);
  void swrContextDeleter(SwrContext* context);
  void avCodecContextDeleter(AVCodecContext* context);
  void avCodecDeleter(AVCodec* codec);
  void avStreamDeleter(AVStream* stream);

  template <auto fn>
  using deleter_from_fn = std::integral_constant<decltype(fn), fn>;
  using AVFrameUPtr = std::unique_ptr<AVFrame, deleter_from_fn<avframeDeleter>>;
  using SWSContextUPtr = std::unique_ptr<SwsContext, deleter_from_fn<swsContextDeleter>>;
  using SWSContextPtr = std::shared_ptr<SwsContext>;
  using SWRContextUPtr = std::unique_ptr<SwrContext, deleter_from_fn<swrContextDeleter>>;
  using SWRContextPtr = std::shared_ptr<SwrContext>;
  using AVPacketPtr = std::shared_ptr<AVPacket>;
  using AVFormatContextUPtr = std::unique_ptr<AVFormatContext, deleter_from_fn<avFormatContextDeleter>>;
  using AVCodecContextUPtr = std::unique_ptr<AVCodecContext, deleter_from_fn<avCodecContextDeleter>>;
  using AVStreamUPtr = std::unique_ptr<AVStream, deleter_from_fn<avStreamDeleter>>;


  ColourPrimaries convertColourPrimary(const AVColorPrimaries primary) noexcept;
  TransferCharacteristics convertTransferCharacteristics(const AVColorTransferCharacteristic transfer) noexcept;
  MatrixCoefficients convertMatrixCoefficients(const AVColorSpace matrix) noexcept;
  ColourRange convertColourRange(const AVColorRange range) noexcept;
  int convertInterpolationMethod(const InterpolationMethod interpolation) noexcept;
  AVPixelFormat convertPixelFormat(const PixelFormat format) noexcept;
  PixelFormat convertPixelFormat(const AVPixelFormat format) noexcept;
  SampleFormat convertSampleFormat(const AVSampleFormat format) noexcept;
  AVSampleFormat convertSampleFormat(const SampleFormat format) noexcept;
  ChannelLayout convertChannelLayout(const uint64_t layout) noexcept;
  uint64_t convertChannelLayout(const ChannelLayout layout) noexcept;
  Codec convertCodecID(const AVCodecID id) noexcept;
  AVCodecID convertCodecID(const Codec id) noexcept;
  /**
   * @brief Convert from FFMpeg type to media_handling type
   * @param format FFMpeg sample format
   * @return SampleFormat
   */
  SampleFormat convert(enum AVSampleFormat format) noexcept;
}

#endif // FFMPEGTYPES_H
