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

#ifndef FFMpegSource_H
#define FFMpegSource_H

#include "imediasource.h"
#include "ffmpegstream.h"

extern "C" {
#include <libavformat/avformat.h>
//#include <libavfilter/avfilter.h>
//#include <libavfilter/buffersink.h>
//#include <libavfilter/buffersrc.h>
//#include <libavutil/opt.h>
//#include <libavutil/pixdesc.h>
}

namespace media_handling::ffmpeg
{

  /**
   * @brief The FFMpegSource class
   */
  class FFMpegSource : public IMediaSource
  {
    public:
      FFMpegSource() = default;
      explicit FFMpegSource(std::string file_path);
      ~FFMpegSource() override;

      bool initialise() override;
      void setFilePath(const std::string& file_path) override;
      std::string repr() override;
      void setProperties(const std::map<MediaProperty, std::any>& props) final;
      void setProperty(const MediaProperty prop, std::any value) final;
      std::any property(const MediaProperty prop, bool& is_valid) const final;
      template<typename T>
      T property(MediaProperty prop, bool& is_valid)
      {
        return IMediaSource::property<T>(prop, is_valid);
      }

      MediaStreamPtr audioStream(const int index) final;

      MediaStreamPtr visualStream(const int index) final;

    protected:
      virtual MediaStreamPtr newMediaStream(const int index);

    private:
      friend class FFMpegSourceTestable;
      std::string file_path_;
      std::map<MediaProperty, std::any> properties_;
      uint64_t calculated_length_ {0};

      AVFormatContext* format_ctx_ {nullptr};

      std::map<int, MediaStreamPtr> audio_streams_;
      std::map<int, MediaStreamPtr> visual_streams_;

      bool configureStream(const int value);
      void extractProperties(const AVFormatContext& ctx);
      void extractStreamProperties(AVStream** streams, const uint32_t stream_count);
  };
}

#endif // FFMpegSource_H
