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

#ifndef FFMPEGMEDIAFRAME_H
#define FFMPEGMEDIAFRAME_H

#include "imediaframe.h"
#include "ffmpegtypes.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
}

namespace media_handling::ffmpeg
{

  class FFMpegMediaFrame : public IMediaFrame
  {
    public:
      struct InOutFormat
      {
          // Needed as there's no documented way to identify how swr/swscontexts were created
          types::SWSContextPtr sws_context_ {nullptr};
          types::SWRContextPtr swr_context_ {nullptr};
          PixelFormat pix_fmt_ {PixelFormat::UNKNOWN};
          SampleFormat sample_fmt_ {SampleFormat::NONE};
          ChannelLayout layout_ {ChannelLayout::UNKNOWN};
          int32_t sample_rate_ {-1};
          Dimensions dims_;
      };

      FFMpegMediaFrame() = default;
      FFMpegMediaFrame(types::AVFrameUPtr frame, const bool visual);
      FFMpegMediaFrame(types::AVFrameUPtr frame, const bool visual, InOutFormat format);

    public: /* IMediaFrame overrides */
      std::optional<bool> isAudio() const override;
      std::optional<bool> isVisual() const override;
      std::optional<int64_t> lineSize(const int index) const override;
      IMediaFrame::FrameData data() noexcept override;
      void setData(FrameData frame_data) override;
      void extractProperties() override;
      int64_t timestamp() const noexcept override;

    private:
      types::AVFrameUPtr ff_frame_ {nullptr};
      types::AVFrameUPtr conv_frame_ {nullptr};
      std::optional<bool> is_audio_; // TODO: just use one
      std::optional<bool> is_visual_;
      int64_t timestamp_ {-1};
      InOutFormat output_fmt_;
      std::optional<FrameData> frame_data_;

    private:
      void extractVisualProperties();
      void extractAudioProperties();
  };
}

#endif // FFMPEGMEDIAFRAME_H
