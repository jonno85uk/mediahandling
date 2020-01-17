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

#ifndef FFMPEGSTREAM_H
#define FFMPEGSTREAM_H

#include "imediastream.h"
#include <optional>
#include <mutex>

#include "ffmpegmediaframe.h"
#include "ffmpegsink.h"
#include "ffmpegtypes.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libavfilter/avfilter.h>
#include <libavutil/frame.h>
}

namespace media_handling::ffmpeg
{
  class FFMpegSource;
  /**
   * @brief The FFMpegStream class
   */
  class FFMpegStream : public IMediaStream
  {
    public:
      FFMpegStream() = default;
      FFMpegStream(FFMpegSource* parent, AVStream* const stream);
      FFMpegStream(FFMpegSink* sink, const AVCodecID codec);
      ~FFMpegStream() override;

    public: // IMediaStream override
      int64_t timestamp() const override;
      MediaFramePtr frame(const int64_t timestamp=-1) final;
      bool writeFrame(MediaFramePtr sample) final;
      StreamType type() const final;
      int32_t sourceIndex() const noexcept final;
      bool setOutputFormat(const PixelFormat format,
                           const Dimensions& dims = {0, 0},
                           InterpolationMethod interp = InterpolationMethod::NEAREST) final;
      bool setOutputFormat(const SampleFormat format, std::optional<int32_t> rate = {}) final;
      bool setInputFormat(const PixelFormat format) final;
      bool setInputFormat(const SampleFormat format) final;

    private:
      FFMpegSource* parent_ {nullptr};
      FFMpegSink* sink_ {nullptr};
      // TODO: use smart ptrs from ffmpegtypes.h
      AVStream* stream_ {nullptr};
      AVCodec* codec_ {nullptr};
      AVCodecContext* codec_ctx_ {nullptr};
      std::shared_ptr<AVCodecContext> sink_codec_ctx_ {nullptr};
      types::AVFrameUPtr sink_frame_ {nullptr};
      AVPacket* pkt_ {nullptr};
      AVDictionary* opts_ {nullptr};
      int pixel_format_{};
      FFMpegMediaFrame::OutputFormat output_format_;

      mutable int64_t last_timestamp_ {-1};
      StreamType type_{StreamType::UNKNOWN};
      bool deinterlacer_setup_ {false};
      int32_t source_index_ {-1};
      std::once_flag setup_encoder_;

    private:
      void extractProperties(const AVStream& stream, const AVCodecContext& context);
      void extractVisualProperties(const AVStream& stream, const AVCodecContext& context);
      void extractAudioProperties(const AVStream& stream, const AVCodecContext& context);
      bool seek(const int64_t timestamp);
      void setupDecoder(const AVCodecID codec_id, AVDictionary* dict) const;
      bool setupEncoder();
      bool setupAudioEncoder(AVStream& stream, AVCodecContext& context, AVCodec& codec) const;
      bool setupVideoEncoder(AVStream& stream, AVCodecContext& context, AVCodec& codec) const;
      bool setupH264Encoder(AVCodecContext& ctx) const;
      bool setupMPEG2Encoder(AVCodecContext& ctx) const;
      bool setupMPEG4Encoder(AVCodecContext& ctx) const;
      bool setupDNXHDEncoder(AVCodecContext& ctx) const;

      /**
       * @brief Extract extra properties from a frame
       * @note  Certain properties are not in AVStream but can be found in AVFrame
       */
      void extractFrameProperties();

      MediaFramePtr frame(AVCodecContext& codec_ctx, const int stream_idx) const;
  };

}

#endif // FFMPEGSTREAM_H
