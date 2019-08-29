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

extern "C" {
#include <libavformat/avformat.h>
#include <libavfilter/avfilter.h>
}

namespace media_handling::ffmpeg
{

  /**
   * @brief The FFMpegStream class
   * read -> decode -> filter/effects
   */
  class FFMpegStream : public IMediaStream
  {
    public:
      FFMpegStream() = default;
      FFMpegStream(AVFormatContext* parent, AVStream* const stream);
      ~FFMpegStream() override;

      /* IMediaStream */
      virtual MediaFramePtr frame(const int64_t timestamp) final;
      virtual bool setFrame(const int64_t timestamp, MediaFramePtr sample) final;
      StreamType type() const final;

    private:
      AVFormatContext* parent_;
      AVStream* stream_ {nullptr};
      AVCodec* codec_ {nullptr};
      AVCodecContext* codec_ctx_ {nullptr};
      AVPacket* pkt_ {nullptr};
      AVFrame* frame_ {nullptr};
      AVDictionary* opts_ {nullptr};
      AVFilterGraph* filter_graph_ {nullptr};
      struct Buffers {
          AVFilterContext* sink_ {nullptr};
          AVFilterContext* source_ {nullptr};
      } buffer_ctx_;
      int pixel_format_{};

      bool deinterlacer_setup_ {false};
      int64_t last_timestamp_ {-1};
      StreamType type_{StreamType::UNKNOWN};

      void extractProperties(const AVStream& stream, const AVCodecContext& context);
      void extractVisualProperties(const AVStream& stream, const AVCodecContext& context);
      void extractAudioProperties(const AVStream& stream, const AVCodecContext& context);
      bool seek(const int64_t timestamp);

      void setupForVideo(const AVStream& strm, Buffers& bufs, AVFilterGraph& graph, int& pix_fmt) const;
      void setupForAudio(const AVStream& strm, Buffers& bufs, AVFilterGraph& graph, AVCodecContext& codec_context) const;
      void setupDecoder(const AVCodecID codec_id, AVDictionary* dict) const;

      constexpr PixelFormat convertPixelFormat(const AVPixelFormat format) const;
      constexpr SampleFormat convertSampleFormat(const AVSampleFormat format) const;
      constexpr ChannelLayout convertChannelLayout(const uint64_t layout) const;

      std::optional<FieldOrder> getFieldOrder();

      MediaFramePtr frame(AVFormatContext& format_ctx, AVCodecContext& codec_ctx, AVPacket& pkt, const int stream_idx) const;
  };

}

#endif // FFMPEGSTREAM_H
