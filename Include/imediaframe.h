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

#ifndef IMEDIAFRAME_H
#define IMEDIAFRAME_H

#include <optional>
#include <memory>

#include "mediapropertyobject.h"

namespace media_handling
{

  class IMediaFrame : public MediaPropertyObject
  {
    public:
      struct FrameData
      {
        Dimensions dims_ {-1, -1};
        int64_t timestamp_ {-1};
        int32_t line_size_ {-1};
        size_t data_size_ {0};
        /**
         * @brief The data of the frame or null
         * @note  Data exists for the lifetime of the IMediaFrame and until the next frame-retrieval
         * @see IMediaFrame::frame
         */
        uint8_t** data_ {nullptr};
        /*
         * @brief The pixel format used on conversion 
         * @see IMediaStream::setOutputFormat
         */
        PixelFormat pix_fmt_ {PixelFormat::UNKNOWN};
        /*
         * @brief The sample format used on conversion 
         * @see IMediaStream::setOutputFormat
         */
        SampleFormat samp_fmt_ {SampleFormat::NONE};
        /*
         * @brief The number of audio samples per channel
         */
        int32_t sample_count_ {-1};
      };
      
      IMediaFrame() = default;

      ~IMediaFrame() override = default;

      virtual std::optional<bool> isAudio() const = 0;

      virtual std::optional<bool> isVisual() const = 0;

      /**
       * @brief Obtain the line size of a plane
       * @param index The plane
       * @return size if found
       */
      virtual std::optional<int64_t> lineSize(const int index) const = 0;

      /**
       * @brief Obtain the sample data of this frame, either read from a stream (decode) or written to (encode)
       * @note  This can be raw or converted data, depending on the subclass implementation
       * @return 
       */
      virtual FrameData data() noexcept = 0;

      virtual void setData(FrameData frame_data) = 0;

      /**
       * @brief It is not always resource-wise to extract all properties for every frame when decoding
       *        so call this at least once before reading any properties
       */
      virtual void extractProperties() = 0;

      /**
       * @brief Timestamp of this read frame
       */
      virtual int64_t timestamp() const noexcept = 0;
  };

  using MediaFramePtr = std::shared_ptr<IMediaFrame>;
}

#endif // IMEDIAFRAME_H
