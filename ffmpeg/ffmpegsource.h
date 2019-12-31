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

#include <queue>
#include <map>

#include "imediasource.h"
#include "ffmpegstream.h"
#include "ffmpegtypes.h"
#include "types.h"
#include "gsl-lite.hpp"


extern "C" {
#include <libavformat/avformat.h>
}

namespace media_handling::ffmpeg
{
  /**
   * @brief The FFMpeg implemtation of IMediaSource
   */
  class FFMpegSource : public IMediaSource
  {
    public:
      FFMpegSource() = default;
      explicit FFMpegSource(std::string file_path);
      ~FFMpegSource() override;
      FFMpegSource(const FFMpegSource& cpy) = delete;
      FFMpegSource& operator=(const FFMpegSource& rhs) = delete;
    public: /* IMediaSource overrides */
      bool initialise() override;
      void setFilePath(const std::string& file_path) override;
      MediaStreamPtr audioStream(const int index) final;
      MediaStreamMap audioStreams() final;
      MediaStreamPtr visualStream(const int index) final;
      MediaStreamMap visualStreams() final;
    protected:
      virtual MediaStreamPtr newMediaStream(AVStream& stream);
    private:
      friend class FFMpegSourceTestable;
      friend class FFMpegStream;
      std::string file_path_;
      uint64_t calculated_length_ {0};
      types::AVFormatContextUPtr format_ctx_ {nullptr};
      /**
       * @brief structure holding packets for a stream which was retrieved when retrieving packet for another stream
       * @note  By doing this, unnecessary seeks and av_read_frame are prevented
       */
      struct {
        mutable std::map<int32_t, int32_t> indexes_;
        std::map<int32_t, std::queue<types::AVPacketPtr>> queue_;
      } packeting_;
    private:
      /**
       * @brief Add a stream for packet queueing
       * @param stream_index  FFMpeg stream index
       */
      void queueStream(const int stream_index) const;
      /**
       * @brief Remove a stream from packet queueing
       * @param stream_index  FFMpeg stream index
       */
      void unqueueStream(const int stream_index);
      /**
       * @brief Retrieve the next packet from the source for a particular stream
       * @note  If another stream previously read the packet , it will be retrieved from the queue
       * @param stream_index  FFMpeg stream index
       * @return packet or null
       */
      types::AVPacketPtr nextPacket(const int stream_index);
      /**
       * @brief Clear all data from the packet queue
       */
      void resetPacketQueue();
      /**
       * @brief Retrieve the format context of the source
       * @return  context or null
       */
      AVFormatContext* context() const noexcept;
    private:
      void extractProperties(const AVFormatContext& ctx);
      void extractStreamProperties(AVStream** streams, const uint32_t stream_count);
      void findFrameRate();
      /**
       * @brief Reset the instance to before it was initialised (minus properties)
       * @note  Allows re-initialise with different starting values
       */
      void reset();
  };
}

#endif // FFMpegSource_H
