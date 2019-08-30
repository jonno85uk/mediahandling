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
      IMediaFrame() = default;

      ~IMediaFrame() override = default;

      virtual std::optional<bool> isAudio() const = 0;

      virtual std::optional<bool> isVisual() const = 0;

      /**
       * @brief Size of the data held
       * @return size in bytes
       */
      virtual int64_t size() const = 0;

      /**
       * @brief Obtain the sample data of this frame, either read from a stream (decode) or written to (encode)
       * @return  data or null
       * @see setData
       */
      virtual std::shared_ptr<uint8_t**> data() const = 0;

      /**
       * @brief             Set the data of this frame for encode
       * @param frame_data
       * @param size        Size in bytes
       * @see data
       */
      virtual void setData(std::shared_ptr<uint8_t**> frame_data, const int64_t size) = 0;

      /**
       * @brief It is not always resource-wise to extract all properties for every frame when decoding
       *        so call this at least once before reading any properties
       */
      virtual void extractProperties() = 0;

      virtual int64_t timestamp() const = 0;
  };

  using MediaFramePtr = std::shared_ptr<IMediaFrame>;
}

#endif // IMEDIAFRAME_H
