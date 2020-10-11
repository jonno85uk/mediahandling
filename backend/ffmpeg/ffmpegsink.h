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

#ifndef FFMPEGSINK_H
#define FFMPEGSINK_H

#include <vector>
#include <mutex>
#include <atomic>

#include "imediasink.h"
#include "imediastream.h"
#include "ffmpegtypes.h"

extern "C" {
#include <libavformat/avformat.h>
}

namespace media_handling::ffmpeg
{
  class FFMpegSink : public IMediaSink
  {
    public:
      FFMpegSink() = delete;
      // TODO: setup a codec per stream. Currently max of 2 streams, 1 per type
      /**
       * @brief FFMpegSink
       * @param file_path   Absolute or relative path to file to be created
       */
      FFMpegSink(std::string file_path, std::vector<Codec> video_codecs, std::vector<Codec> audio_codecs);

    public: // MediaPropertyObject override
      void setProperty(const MediaProperty prop, const std::any& value) override;

    public: // IMediaSink Overrides
      ~FFMpegSink() override;
      bool initialise() override;
      bool isReady() override;
      MediaStreamPtr audioStream(const size_t index) override;
      std::vector<MediaStreamPtr> audioStreams() override;
      MediaStreamPtr visualStream(const size_t index) override;
      std::vector<MediaStreamPtr> visualStreams() override;
      std::set<Codec> supportedAudioCodecs() const override;
      std::set<Codec> supportedVideoCodecs() const override;
    public:
      AVFormatContext& formatContext() const;
      bool writeHeader();
      bool writeTrailer();
      void finish();
    private:
      std::string file_path_;
      struct {
          std::vector<Codec> video_;
          std::vector<Codec> audio_;
      } codecs_;
      struct {
          std::vector<MediaStreamPtr> video_;
          std::vector<MediaStreamPtr> audio_;
      } streams_;
      types::AVFormatContextUPtr fmt_ctx_ {nullptr};
      bool header_written_;
      std::once_flag trailer_written_;
      std::atomic<bool> ready_ {false};

  };
}



#endif // FFMPEGSINK_H
