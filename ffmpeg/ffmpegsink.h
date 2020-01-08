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

#include "imediasink.h"
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
    /**
     * @brief FFMpegSink
     * @param file_path   Absolute or relative path to file to be created
     */
    explicit FFMpegSink(const std::string& file_path);

    ~FFMpegSink() override;

    bool encode(std::shared_ptr<MediaFramePtr> sample) override;

    bool isReady() override;

  private:
    bool ready_ {false};
    types::AVFormatContextUPtr fmt_ctx_ {nullptr};

    bool initialise(const std::string& path);
  };
}



#endif // FFMPEGSINK_H
