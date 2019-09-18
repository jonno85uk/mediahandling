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
#include "mediahandling.h"

using media_handling::ffmpeg::FFMpegSource;
using media_handling::ffmpeg::FFMpegStream;
using media_handling::MediaFramePtr;
using media_handling::MediaStreamPtr;

constexpr auto ERR_LEN = 1024;

extern "C" {
#include <libavformat/avformat.h>
}

namespace
{
  std::string err(ERR_LEN, '\0');
}


FFMpegSource::FFMpegSource(std::string file_path) : file_path_(std::move(file_path))
{
  if (!FFMpegSource::initialise()) {
    std::throw_with_nested(std::runtime_error("FFMpegSource::initialise failed, filepath=" + file_path_) );
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
    av_strerror(err_code, err.data(), ERR_LEN);
    logMessage("Failed to open file, code=" + err);
    return false;
  }

  // Read info about the file
  err_code = avformat_find_stream_info(format_ctx_, nullptr);
  if (err_code != 0) {
    av_strerror(err_code, err.data(), ERR_LEN);
    logMessage("Failed to read file info, code=" + err);
    return false;
  }

  assert(format_ctx_);
  // Extract properties
  extractProperties(*format_ctx_);

  findFrameRate();

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


MediaStreamPtr FFMpegSource::audioStream(const int index) const
{
  if (audio_streams_.count(index) > 0)
  {
    return audio_streams_.at(index);
  }
  return nullptr;
}

MediaStreamPtr FFMpegSource::visualStream(const int index) const
{
  if (visual_streams_.count(index) > 0)
  {
    return visual_streams_.at(index);
  }
  return nullptr;
}


MediaStreamPtr FFMpegSource::newMediaStream(AVStream& stream)
{
  assert(format_ctx_);
  assert(format_ctx_->streams);
  assert(format_ctx_->streams[index]);
  return std::make_shared<FFMpegStream>(format_ctx_, &stream);
}

void FFMpegSource::extractProperties(const AVFormatContext& ctx)
{
  assert(ctx.iformat);
  MediaPropertyObject::setProperty(MediaProperty::FILENAME, file_path_);
  MediaPropertyObject::setProperty(MediaProperty::FILE_FORMAT, std::string(ctx.iformat->long_name));
  MediaPropertyObject::setProperty(MediaProperty::DURATION, ctx.duration);
  MediaPropertyObject::setProperty(MediaProperty::STREAMS, static_cast<int32_t>(ctx.nb_streams));
  MediaPropertyObject::setProperty(MediaProperty::BITRATE, ctx.bit_rate);

  extractStreamProperties(format_ctx_->streams, format_ctx_->nb_streams);
}


void FFMpegSource::extractStreamProperties(AVStream** streams, const uint32_t stream_count)
{
  int32_t visual_count = 0;
  int32_t audio_count = 0;

  gsl::span<AVStream*> span_streams(streams, stream_count);
  for (auto& stream: span_streams) {
    assert(stream);
    switch (stream->codecpar->codec_type) {
      case AVMEDIA_TYPE_VIDEO:
        visual_streams_[visual_count] = newMediaStream(*stream);
        visual_count++;
        break;
      case AVMEDIA_TYPE_AUDIO:
        audio_streams_[audio_count] = newMediaStream(*stream);
        audio_count++;
        break;
      default:
        // Unhandled type
        break;
    }
  }

  MediaPropertyObject::setProperty(MediaProperty::VIDEO_STREAMS, visual_count);
  MediaPropertyObject::setProperty(MediaProperty::AUDIO_STREAMS, audio_count);
}


void FFMpegSource::findFrameRate()
{
  gsl::span<AVStream*> streams{format_ctx_->streams, format_ctx_->nb_streams};
  AVStream* ref_stream = nullptr;
  for (auto& stream : streams)  {
    assert(stream);
    if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
      ref_stream = stream;
      break;
    }
  }

  if (ref_stream == nullptr) {
    return;
  }

  const auto avrate = av_guess_frame_rate(format_ctx_, ref_stream, nullptr);
  const Rational frate {avrate.num, avrate.den};
  this->setProperty(MediaProperty::FRAME_RATE, frate);
}
