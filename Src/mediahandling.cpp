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

#include "mediahandling.h"
#include <iostream>

#ifdef OLD_FFMPEG
extern "C" {
#include <libavformat/avformat.h>
#include <libavfilter/avfilter.h>
}
#endif

#include "ffmpegsource.h"

static auto media_backend = media_handling::BackendType::FFMPEG;

void defaultLog(const std::string& msg)
{
  std::cerr << msg << std::endl;
}


static media_handling::LOGGINGFN logging_func = defaultLog;

bool media_handling::initialise(const BackendType backend)
{
  media_backend = backend;
  if (backend == BackendType::FFMPEG) {
#ifdef OLD_FFMPEG // lavf 58.9.100
  avcodec_register_all();
  av_register_all();
  avfilter_register_all();
#endif
  return true;
  }
  logMessage("Chosen backend type is not available");
  return false;
}



void media_handling::assignLoggerCallback(media_handling::LOGGINGFN func)
{
  logging_func = func;
}

void media_handling::logMessage(const std::string& msg) noexcept
{
  if (logging_func == nullptr) {
    return;
  }
  try {
    logging_func(msg);
  }  catch (...) {
    // TODO: stderr
  }
}


media_handling::MediaSourcePtr media_handling::createSource(std::string file_path)
{
  switch (media_backend) {
    case BackendType::FFMPEG:
      return std::make_shared<ffmpeg::FFMpegSource>(file_path);
    case BackendType::GSTREAMER:
    [[fallthrough]];
    case BackendType::INTEL:
    [[fallthrough]];
    default:
      return {};
  }
}
