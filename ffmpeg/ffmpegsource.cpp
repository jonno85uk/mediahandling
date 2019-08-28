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

#include <filesystem>
#include <cassert>
#include <iostream>
#include <sstream>

#include "ffmpegsource.h"
#include "ffmpegstream.h"

using media_handling::ffmpeg::FFMpegSource;
using media_handling::ffmpeg::FFMpegStream;
using media_handling::MediaFramePtr;
using media_handling::MediaStreamPtr;


FFMpegSource::FFMpegSource(std::string file_path) : file_path_(std::move(file_path))
{
  if (!FFMpegSource::initialise()) {
    throw std::exception();
  }
}

FFMpegSource::~FFMpegSource()
{
  avformat_free_context(format_ctx_);
}

bool FFMpegSource::initialise()
{
  if (!std::filesystem::exists(file_path_)) {
    return false;
  }

  if (std::filesystem::status(file_path_).type() != std::filesystem::file_type::regular) {
    return false;
  }


  // Open the file
  int err_code = avformat_open_input(&format_ctx_, file_path_.c_str(), nullptr, nullptr);
  if (err_code != 0) {
    char err[1024];
    av_strerror(err_code, err, 1024);
    std::cerr << err << std::endl;
    return false;
  }

  // Read info about the file
  err_code = avformat_find_stream_info(format_ctx_, nullptr);
  if (err_code != 0) {
    char err[1024];
    av_strerror(err_code, err, 1024);
    std::cerr << err << std::endl;
    return false;
  }

  assert(format_ctx_);
  // Extract properties
  extractProperties(*format_ctx_);

#ifdef VERBOSE_FFMPEG
  av_dump_format(format_ctx_, 0, file_path_.c_str(), 0);
#endif
  return true;
}

void FFMpegSource::setFilePath(const std::string& file_path)
{
  file_path_ = file_path;
  this->setProperty(MediaProperty::FILENAME, file_path_);
}


MediaStreamPtr FFMpegSource::audioStream(const int index)
{
  if (audio_streams_.count(index) > 0)
  {
    return audio_streams_.at(index);
  }
  bool is_valid;
  const auto streams = this->property<int32_t>(MediaProperty::AUDIO_STREAMS, is_valid);
  if (!is_valid || streams < index) {
    return nullptr;
  }
  audio_streams_[index] = this->newMediaStream(index);
  return audio_streams_.at(index);
}

MediaStreamPtr FFMpegSource::visualStream(const int index)
{
  if (visual_streams_.count(index) > 0)
  {
    return visual_streams_.at(index);
  }
  bool is_valid;
  const auto streams = this->property<int32_t>(MediaProperty::VIDEO_STREAMS, is_valid);
  if (!is_valid || streams < index) {
    return nullptr;
  }
  visual_streams_[index] = this->newMediaStream(index);
  return visual_streams_.at(index);
}


MediaStreamPtr FFMpegSource::newMediaStream(const int index)
{
  assert(format_ctx_);
  assert(format_ctx_->streams);
  assert(format_ctx_->streams[index]);
  return std::make_shared<FFMpegStream>(format_ctx_, format_ctx_->streams[index]);
}

void FFMpegSource::extractProperties(const AVFormatContext& ctx)
{
  assert(ctx.iformat);
  this->setProperty(MediaProperty::FILENAME, file_path_);
  this->setProperty(MediaProperty::FILE_FORMAT, std::string(ctx.iformat->long_name));
  this->setProperty(MediaProperty::DURATION, ctx.duration);
  this->setProperty(MediaProperty::STREAMS, static_cast<int32_t>(ctx.nb_streams));
  this->setProperty(MediaProperty::BITRATE, ctx.bit_rate);
  extractStreamProperties(ctx.streams, ctx.nb_streams);
}


void FFMpegSource::extractStreamProperties(AVStream** streams, const uint32_t stream_count)
{
  assert(streams);
  int32_t visual_count = 0;
  int32_t audio_count = 0;
  for (auto ix = 0; ix < static_cast<int32_t>(stream_count); ++ix) {
    const AVStream* stream = streams[ix];
    assert(stream);
    switch (stream->codecpar->codec_type) {
      case AVMEDIA_TYPE_VIDEO:
        visual_streams_[visual_count] = newMediaStream(ix);
        visual_count++;
        break;
      case AVMEDIA_TYPE_AUDIO:
        audio_streams_[audio_count] = newMediaStream(ix);
        audio_count++;
        break;
      default:
        // Unhandled type
        break;
    }
  }

  this->setProperty(MediaProperty::VIDEO_STREAMS, visual_count);
  this->setProperty(MediaProperty::AUDIO_STREAMS, audio_count);
}
