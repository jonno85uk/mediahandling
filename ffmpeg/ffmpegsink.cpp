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
#include "ffmpegstream.h"

using media_handling::ffmpeg::FFMpegSink;

constexpr size_t ERR_LEN = 256;

namespace
{
  std::string err(ERR_LEN, '\0');
}

FFMpegSink::FFMpegSink(std::string file_path, std::vector<Codec> video_codecs, std::vector<Codec> audio_codecs)
  : file_path_(std::move(file_path)),
    codecs_({std::move(video_codecs), std::move(audio_codecs)})
{
  std::filesystem::path tmp_path(file_path_);
  if (file_path_.empty() || !std::filesystem::exists(tmp_path.parent_path())) {
    throw std::runtime_error("FFMpegSink::initialise failed, filepath=" + file_path_);
  }
}

FFMpegSink::~FFMpegSink()
{
  writeTrailer();
}


bool FFMpegSink::initialise()
{
  MediaPropertyObject::setProperty(media_handling::MediaProperty::FILENAME, file_path_);

  // Configure container
  AVFormatContext* ctx = nullptr;
  auto ret = avformat_alloc_output_context2(&ctx, nullptr, nullptr, file_path_.c_str());
  if (ret < 0) {
    av_strerror(ret, err.data(), ERR_LEN);
    logMessage(LogType::CRITICAL, "Could not create output context, code=" + err);
    return false;
  }
  assert(ctx);
  fmt_ctx_.reset(ctx);

  ret = avio_open(&fmt_ctx_.get()->pb,  file_path_.c_str(), AVIO_FLAG_WRITE);
  if (ret < 0) {
    av_strerror(ret, err.data(), ERR_LEN);
    logMessage(LogType::CRITICAL, "Could not open output file, code=" + err);
    return false;
  }

  // Configure Streams
  for (const auto& codec : codecs_.video_) {
    const auto ffv = types::convertCodecID(codec);
    if (AVCodec* av_codec = avcodec_find_encoder(ffv)) {
      if (av_codec->type != AVMEDIA_TYPE_VIDEO) {
        logMessage(LogType::CRITICAL, "An audio codec chosen for video encoding");
        return false;
      } else {
        try {
          streams_.video_.emplace_back(std::make_shared<FFMpegStream>(this, ffv));
        } catch (const std::runtime_error& ex) {
          logMessage(LogType::CRITICAL, ex.what());
        }
      }
    } else {
      logMessage(LogType::WARNING, "Unsupported encoder codec");
    }
  }

  for (const auto& codec : codecs_.audio_) {
    const auto ffa = types::convertCodecID(codec);
    if (AVCodec* av_codec = avcodec_find_encoder(ffa)) {
      if (av_codec->type != AVMEDIA_TYPE_AUDIO) {
        logMessage(LogType::CRITICAL, "An audio codec chosen for video encoding");
        return false;
      } else {
        try {
          streams_.audio_.emplace_back(std::make_shared<FFMpegStream>(this, ffa));
        } catch (const std::runtime_error& ex) {
          logMessage(LogType::CRITICAL, ex.what());
        }
      }
    } else {
      logMessage(LogType::WARNING, "Unsupported encoder codec");
    }
  }

  if (streams_.audio_.empty() && streams_.video_.empty()) {
    logMessage(LogType::CRITICAL, "Failed to setup any streams");
    return false;
  }
  return true;
}

bool FFMpegSink::isReady()
{
  if (!ready_)
  {
    // TODO:
  }

  return ready_;
}

media_handling::MediaStreamPtr FFMpegSink::audioStream(const size_t index)
{
  if (streams_.audio_.empty()) {
    return {};
  }
  if ( (streams_.audio_.size() - 1) >= index) {
    return streams_.audio_.at(index);
  }
  return {};
}

std::vector<media_handling::MediaStreamPtr> FFMpegSink::audioStreams()
{
  return streams_.audio_;
}

media_handling::MediaStreamPtr FFMpegSink::visualStream(const size_t index)
{
  if (streams_.video_.empty()) {
    return {};
  }
  if ( (streams_.video_.size() - 1) >= index) {
    return streams_.video_.at(index);
  }
  return {};
}

std::vector<media_handling::MediaStreamPtr> FFMpegSink::visualStreams()
{
  return streams_.video_;
}


AVFormatContext& FFMpegSink::formatContext() const
{
  assert(fmt_ctx_ != nullptr);
  return *fmt_ctx_;
}

bool FFMpegSink::writeHeader()
{
  bool okay = true;
  if (!header_written_) {
      auto ret = avformat_write_header(fmt_ctx_.get(), nullptr);
      if (ret < 0) {
        av_strerror(ret, err.data(), ERR_LEN);
        const auto msg = fmt::format("Could not write output file header, msg=", err.data());
        okay = false;
      } else {
        header_written_ = true;
      }
  }
  return okay;
}

bool FFMpegSink::writeTrailer()
{
  if ((fmt_ctx_ == nullptr) || (!header_written_)) {
    return false;
  }
  bool okay = true;
  const auto func = [&] {
      auto ret = av_write_trailer(fmt_ctx_.get());
      if (ret < 0) {
        av_strerror(ret, err.data(), ERR_LEN);
        const auto msg = fmt::format("Could not write output file trailer, msg=", err.data());
        okay = false;
      }
  };
  std::call_once(trailer_written_, func);
  return okay;
}
