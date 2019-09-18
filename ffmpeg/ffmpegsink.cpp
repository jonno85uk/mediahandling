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

#include "ffmpegsink.h"

#include <filesystem>

#include "mediahandling.h"

using media_handling::ffmpeg::FFMpegSink;

constexpr size_t ERR_LEN = 256;

namespace
{
  std::string err(ERR_LEN, '\0');
}

FFMpegSink::FFMpegSink(const std::string& file_path)
{
  if (!initialise(file_path)) {
    std::throw_with_nested(std::runtime_error("FFMpegSink::initialise failed, filepath=" + file_path) );
  }
}

FFMpegSink::~FFMpegSink()
{

}

bool FFMpegSink::encode(std::shared_ptr<MediaFramePtr> sample)
{
  // TODO:
  return false;
}

bool FFMpegSink::isReady()
{
  if (!ready_)
  {
    // TODO:
  }

  return ready_;
}


bool FFMpegSink::initialise(const std::string& path)
{
  std::filesystem::path tmp_path(path);
  if (path.empty() || !std::filesystem::exists(tmp_path.parent_path())) {
    return false;
  }

  MediaPropertyObject::setProperty(media_handling::MediaProperty::FILENAME, path);

  int ret = avformat_alloc_output_context2(&fmt_ctx_, nullptr, nullptr, path.c_str());
  if ( (ret < 0) || (fmt_ctx_ == nullptr)) {
    av_strerror(ret, err.data(), ERR_LEN);
    logMessage("Could not create output context, code=" + err);
    return false;
  }

  ret = avio_open(&fmt_ctx_->pb,  path.c_str(), AVIO_FLAG_WRITE);
  if (ret < 0) {
    av_strerror(ret, err.data(), ERR_LEN);
    logMessage("Could not open output file, code=" + err);
    return false;
  }

  return true;
}
