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

#ifndef IMEDIASTREAM_H
#define IMEDIASTREAM_H

#include <memory>
#include <any>
#include <map>
#include <iostream>

#include "types.h"
#include "imediaframe.h"
#include "mediapropertyobject.h"

namespace media_handling
{
  /**
   * @brief The "Essence"
   */
  class IMediaStream : public MediaPropertyObject
  {
  public:
    ~IMediaStream() override = default;

    /**
     * @brief frame       Retrieve a frame-sample from the stream
     * @param timestamp   The position in the stream for retrieval
     * @return            Frame sample on success or null
     */
    virtual MediaFramePtr frame(const int64_t timestamp=-1) = 0;

    /**
     * @brief setFrame    Set the frame-sample for the stream
     * @param timestamp   Position in the stream
     * @param sample      Frame sample
     * @return            true==success
     */
    virtual bool setFrame(const int64_t timestamp, MediaFramePtr sample) = 0;

    /**
     * @brief   Obtain the type of this stream
     * @return  type if known, otherwise UNKNOWN
     */
    virtual StreamType type() const = 0;
  };

  using MediaStreamPtr = std::shared_ptr<IMediaStream>;
}

#endif // IMEDIASTREAM_H
