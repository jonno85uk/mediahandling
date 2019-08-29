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

extern "C" {
#include <libavformat/avformat.h>
}

namespace media_handling
{
  class FFMpegMediaFrame : public IMediaFrame
  {
    public:
      explicit FFMpegMediaFrame(AVFrame* const frame, const bool visual);

      ~FFMpegMediaFrame() override;

      // FIXME: rule of five

      std::optional<bool> isAudio() const override;

      std::optional<bool> isVisual() const override;

      int64_t size() const override;

      std::shared_ptr<uint8_t**> data() const override;

      void setData(std::shared_ptr<uint8_t**> frame_data, const int64_t size) override;

      void extractProperties() override;

      int64_t timestamp() const override;

    private:
      AVFrame* ff_frame_ {nullptr};
      bool visual_;
      std::optional<bool> is_audio_;
      std::optional<bool> is_visual_;
      int64_t data_size_ {0};
      std::shared_ptr<uint8_t**> data_ {nullptr};
      int64_t timestamp_ {-1};

      void extractVisualProperties();

      void extractAudioProperties();
  };
}

#endif // FFMPEGMEDIAFRAME_H
