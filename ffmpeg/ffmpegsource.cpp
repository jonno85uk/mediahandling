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
#include <regex>
#include <fmt/core.h>

#include "ffmpegsource.h"
#include "ffmpegstream.h"
#include "mediahandling.h"

using media_handling::ffmpeg::FFMpegSource;
using media_handling::ffmpeg::FFMpegStream;
using media_handling::MediaFramePtr;
using media_handling::MediaStreamPtr;
using media_handling::MediaStreamMap;

constexpr auto ERR_LEN = 1024;
constexpr auto TAG_TIMECODE = "timecode";

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
    throw std::runtime_error("FFMpegSource::initialise failed, filepath=" + file_path_);
  }
}

FFMpegSource::~FFMpegSource()
{
  reset();
}

bool FFMpegSource::initialise()
{
  if (!std::filesystem::exists(file_path_)) {
    return false;
  }

  if (std::filesystem::status(file_path_).type() != std::filesystem::file_type::regular) {
    return false;
  }

  // Ensure resources freed on a re-initialise
  reset();

  const auto [path, start] ([&] () -> std::tuple<std::string, int> {
    if (media_handling::global::auto_detect_img_sequence == false) {
      return {file_path_, -1};
    }

    std::string result;
    if (media_handling::utils::pathIsInSequence(file_path_)) {
      if (auto ptn = media_handling::utils::generateSequencePattern(file_path_)) {
        result = ptn.value();
      }
    }
    // A manually created sequence pattern takes priority
    bool okay = false;
    const auto pattern = MediaPropertyObject::property<std::string>(MediaProperty::SEQUENCE_PATTERN, okay);
    if (okay) {
      // something has been set. An empty pattern is indicative of to open an image in a sequence as _only_ 1 image
      result.clear();
      if (!pattern.empty()) {
        auto par_path = std::filesystem::path(file_path_).parent_path();
        par_path /= pattern;
        result = par_path.string();
      }
    }

    if (result.empty()) {
      // Nothing found then carry on with no modified file-path
      return {file_path_, -1};
    } else {
      const auto start = media_handling::utils::getSequenceStartNumber(file_path_);
      return {result, start};
    }
  }());

  const auto p = path.c_str(); // only because path is unavailable when in debug
  AVDictionary* dict = nullptr;
  if (start > 0) {
    // FFMpeg expects sequences to start at zero by default
    av_dict_set(&dict, "start_number", std::to_string(start).c_str(), 0);
  }
  // Open the file
  AVFormatContext* ctx = nullptr;
  int err_code = avformat_open_input(&ctx, p, nullptr, &dict);
  if (err_code != 0) {
    av_strerror(err_code, err.data(), ERR_LEN);
    logMessage(LogType::CRITICAL, "Failed to open file, code=" + err + "fileName=" + p);
    avformat_free_context(ctx);
    return false;
  }
  format_ctx_.reset(ctx);
  assert(format_ctx_);
  // Read info about the file
  err_code = avformat_find_stream_info(format_ctx_.get(), nullptr);
  if (err_code != 0) {
    av_strerror(err_code, err.data(), ERR_LEN);
    logMessage(LogType::CRITICAL, "Failed to read file info, code=" + err);
    return false;
  }

  findFrameRate();
  // Extract properties
  extractProperties(*format_ctx_);
  if (format_ctx_->metadata)
  {
    extractMetadata(*format_ctx_->metadata);
  }

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
  if (format_ctx_ == nullptr) {
    return {};
  }
  int streams = 0;
  gsl::span<AVStream*> span_streams(format_ctx_->streams, format_ctx_->nb_streams);
  for (auto& stream: span_streams) {
    if (stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
      if (streams == index) {
        auto f_s = this->newMediaStream(*stream);
        std::dynamic_pointer_cast<FFMpegStream>(f_s)->initialise();
        return f_s;
      } else {
        streams++;
      }
    }
  }
  return {};
}


MediaStreamMap FFMpegSource::audioStreams()
{
  int streams = 0;
  MediaStreamMap a_m;
  gsl::span<AVStream*> span_streams(format_ctx_->streams, format_ctx_->nb_streams);
  for (auto& stream: span_streams) {
    if (stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
      auto f_s = this->newMediaStream(*stream);
      std::dynamic_pointer_cast<FFMpegStream>(f_s)->initialise();
      a_m[streams++] = f_s;
    }
  }
  return a_m;
}


MediaStreamPtr FFMpegSource::visualStream(const int index)
{
  if (format_ctx_ == nullptr) {
    return {};
  }
  int streams = 0;
  gsl::span<AVStream*> span_streams(format_ctx_->streams, format_ctx_->nb_streams);
  for (auto& stream: span_streams) {
    if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
      if (streams == index) {
        auto f_s = this->newMediaStream(*stream);
        bool is_okay;
        const auto frate = this->property(MediaProperty::FRAME_RATE, is_okay);
        assert(is_okay);
        f_s->setProperty(MediaProperty::FRAME_RATE, frate);
        std::dynamic_pointer_cast<FFMpegStream>(f_s)->initialise();
        return f_s;
      } else {
        streams++;
      }
    }
  }
  return {};
}

MediaStreamMap FFMpegSource::visualStreams()
{
  int streams = 0;
  MediaStreamMap a_m;
  gsl::span<AVStream*> span_streams(format_ctx_->streams, format_ctx_->nb_streams);
  for (auto& stream: span_streams) {
    if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
      auto f_s = this->newMediaStream(*stream);
      bool is_okay;
      const auto frate = this->property(MediaProperty::FRAME_RATE, is_okay);
      assert(is_okay);
      f_s->setProperty(MediaProperty::FRAME_RATE, frate);
      std::dynamic_pointer_cast<FFMpegStream>(f_s)->initialise();
      a_m[streams++] = f_s;
    }
  }
  return a_m;
}

MediaStreamPtr FFMpegSource::newMediaStream(AVStream& stream)
{
  return std::make_shared<FFMpegStream>(this, &stream);
}


void FFMpegSource::queueStream(const int stream_index) const
{
  if (packeting_.indexes_.count(stream_index) == 1) {
    packeting_.indexes_[stream_index]++;
  } else {
    packeting_.indexes_[stream_index] = 1;
  }
}

void FFMpegSource::unqueueStream(const int stream_index)
{
  if (packeting_.indexes_.count(stream_index) == 1) {
    packeting_.indexes_[stream_index]--;
  } else {
    logMessage(LogType::INFO, "Stream was already unqueued");
  }
}

media_handling::ffmpeg::types::AVPacketPtr FFMpegSource::nextPacket(const int stream_index)
{
  // prevent unnecessary read of demuxed packets
  auto read_packet = [&] () -> media_handling::ffmpeg::types::AVPacketPtr
  {
    while (true) {
      auto pkt = std::shared_ptr<AVPacket>(av_packet_alloc(), types::avPacketDeleter);
      assert(format_ctx_ && pkt);
      const auto ret = av_read_frame(format_ctx_.get(), pkt.get());
      if (ret < 0) {
        av_strerror(ret, err.data(), ERR_LEN);
        logMessage(LogType::INFO, fmt::format("Failed to read frame: {}", err.data()));
        break;
      }
      if (pkt->stream_index == stream_index) {
        return pkt;
      }
      if ( (packeting_.indexes_.count(stream_index) == 1)
           && (packeting_.indexes_.at(stream_index) > 0)) {
        // only queue packets for needed streams
        packeting_.queue_[pkt->stream_index].push(pkt);
      }
    }
    return {};
  };

  if (packeting_.queue_.count(stream_index) == 1) {
    if (!packeting_.queue_.at(stream_index).empty()) {
      auto pkt = packeting_.queue_.at(stream_index).front();
      packeting_.queue_.at(stream_index).pop();
      return pkt;
    } else {
      return read_packet();
    }
  } else {
    return read_packet();
  }
}

void FFMpegSource::resetPacketQueue()
{
  packeting_.queue_.clear();
}


AVFormatContext* FFMpegSource::context() const noexcept
{
  return format_ctx_.get();
}

void FFMpegSource::extractProperties(const AVFormatContext& ctx)
{
  assert(ctx.iformat);
  MediaPropertyObject::setProperty(MediaProperty::FILENAME, file_path_);
  MediaPropertyObject::setProperty(MediaProperty::FILE_FORMAT, std::string(ctx.iformat->long_name));
  MediaPropertyObject::setProperty(MediaProperty::DURATION, Rational(ctx.duration, AV_TIME_BASE));
  MediaPropertyObject::setProperty(MediaProperty::STREAMS, static_cast<int32_t>(ctx.nb_streams));
  MediaPropertyObject::setProperty(MediaProperty::BITRATE, static_cast<BitRate>(ctx.bit_rate));

  extractStreamProperties(format_ctx_->streams, format_ctx_->nb_streams);
}

void FFMpegSource::extractMetadata(const AVDictionary& metadata)
{
  if (av_dict_count(&metadata) < 1)
  {
    return;
  }

  if (AVDictionaryEntry* entry = av_dict_get(&metadata, TAG_TIMECODE, nullptr, 0))
  {
    bool okay;
    const auto frame_rate = this->property<Rational>(MediaProperty::FRAME_RATE, okay);
    std::string tc_str(entry->value);
    TimeCode tc({1,1}, frame_rate);
    if (tc.setTimeCode(tc_str))
    {
      this->setProperty(MediaProperty::START_TIMECODE, tc);
    }
    else
    {
      logMessage(LogType::WARNING, "Failed to configure start timecode");
    }
  }
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
        FFMpegStream(this, stream); // extract any properties which may attach to source
        visual_count++;
        break;
      case AVMEDIA_TYPE_AUDIO:
        FFMpegStream(this, stream); // extract any properties which may attach to source
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
  // this is making a (reasonable) assumption that there is only ever 1 video stream
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

  const auto avrate = av_guess_frame_rate(format_ctx_.get(), ref_stream, nullptr);
  const Rational frate {avrate.num, avrate.den};
  this->setProperty(MediaProperty::FRAME_RATE, frate);
}


void FFMpegSource::reset()
{
  format_ctx_.reset();
  format_ctx_ = nullptr;
}

